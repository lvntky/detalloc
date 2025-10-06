/**
 * @file detalloc.h
 * @brief Deterministic Real-Time Memory Allocator (Detalloc)
 *
 * A constant-time (O(1)) memory allocator for hard real-time systems.
 * Guarantees bounded latency and deterministic behavior for allocation/free.
 *
 * ## Key Features
 * - **O(1)** allocation and deallocation
 * - Deterministic behavior (same inputs → same timing)
 * - No system calls after initialization
 * - Fixed, bounded memory usage (pool-based)
 * - Optional runtime **statistics** and **validation** hooks
 *
 * @version 0.1.0
 * @date 2025
 */

#ifndef DETALLOC_H
#define DETALLOC_H

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* Includes                                                                   */
/* ========================================================================== */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* ========================================================================== */
/* Visibility / Inline                                                        */
/* ========================================================================== */
/**
 * @def DETALLOC_API
 * @brief Export/import macro for building/using shared libraries.
 */
#ifndef DETALLOC_API
#  if defined(_WIN32) && !defined(__GNUC__)
#    ifdef DETALLOC_BUILD
#      define DETALLOC_API __declspec(dllexport)
#    else
#      define DETALLOC_API __declspec(dllimport)
#    endif
#  else
#    define DETALLOC_API __attribute__((visibility("default")))
#  endif
#endif

/**
 * @def DET_INLINE
 * @brief Always-inline helper for hot-path functions where supported.
 */
#ifndef DET_INLINE
#  if defined(__GNUC__) || defined(__clang__)
#    define DET_INLINE static inline __attribute__((always_inline))
#  else
#    define DET_INLINE static inline
#  endif
#endif

/* ========================================================================== */
/* Version                                                                    */
/* ========================================================================== */
/** @name Version
 *  @{ */
#define DETALLOC_VERSION_MAJOR 0  /**< Major version */
#define DETALLOC_VERSION_MINOR 1  /**< Minor version */
#define DETALLOC_VERSION_PATCH 0  /**< Patch version */
/** @} */

/* ========================================================================== */
/* Configuration Constants                                                    */
/* ========================================================================== */
/** @name Configuration Constants
 *  @{ */
/** Maximum number of pool size classes. */
#ifndef DET_MAX_POOLS
#define DET_MAX_POOLS 16
#endif

/** Default pool sizes (in bytes). */
#ifndef DET_DEFAULT_POOL_SIZES
#define DET_DEFAULT_POOL_SIZES { 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096 }
#endif

/** Cache line size for alignment (adjust per platform). */
#ifndef DET_CACHE_LINE_SIZE
#define DET_CACHE_LINE_SIZE 64
#endif

/** Alignment for all allocations (must be a power of 2). */
#ifndef DET_ALIGN
#define DET_ALIGN 8
#endif
/** @} */

/* ========================================================================== */
/* Error Codes                                                                */
/* ========================================================================== */
/**
 * @brief Error/status codes returned by Detalloc.
 */
typedef enum {
    DET_OK = 0,              /**< Operation successful */
    DET_ERR_INVALID_PARAM,   /**< A parameter is invalid */
    DET_ERR_OUT_OF_MEMORY,   /**< Buffer cannot accommodate configuration */
    DET_ERR_POOL_FULL,       /**< Appropriate pool has no free blocks */
    DET_ERR_INVALID_PTR,     /**< Pointer not owned by allocator/pool */
    DET_ERR_CORRUPTED,       /**< Internal structures appear corrupted */
    DET_ERR_NOT_INITIALIZED  /**< Allocator not initialized */
} det_error_t;

/* ========================================================================== */
/* Configuration Structures                                                   */
/* ========================================================================== */
/**
 * @brief Pool configuration for a single size class.
 */
typedef struct {
    size_t block_size;     /**< Size of each block in bytes */
    size_t num_blocks;     /**< Number of blocks in this pool */
    bool   cache_aligned;  /**< Align blocks to cache line boundary */
} det_pool_config_t;

/**
 * @brief Global allocator configuration.
 *
 * Provide a sequence of pool configs (up to #DET_MAX_POOLS). Unused entries
 * should have `block_size = 0`. The @ref num_pools field is authoritative.
 */
typedef struct {
    det_pool_config_t pools[DET_MAX_POOLS];  /**< Pool configurations */
    size_t            num_pools;             /**< Number of active pools */

    bool  enable_stats;       /**< Enable runtime stats collection */
    bool  enable_validation;  /**< Enable internal validation */
    bool  thread_safe;        /**< Enable thread-safety (adds slight overhead) */

    /**
     * @brief Optional error callback.
     * @param error Detalloc error code
     * @param msg   Human-readable diagnostic
     */
    void (*error_handler)(det_error_t error, const char *msg);
} det_config_t;

/* ========================================================================== */
/* Statistics                                                                 */
/* ========================================================================== */
/**
 * @brief Per-pool statistics.
 * @note Available when `enable_stats=true`. Overhead ~5%.
 */
typedef struct {
    size_t   total_allocs;        /**< Total allocations from this pool */
    size_t   total_frees;         /**< Total deallocations to this pool */
    size_t   current_usage;       /**< Currently allocated blocks */
    size_t   peak_usage;          /**< Peak simultaneous allocations */
    size_t   failed_allocs;       /**< Allocation failures (pool full) */
    uint64_t total_alloc_cycles;  /**< CPU cycles spent in allocation */
    uint64_t total_free_cycles;   /**< CPU cycles spent in free */
} det_pool_stats_t;

/**
 * @brief Global allocator statistics.
 * @note Available when `enable_stats=true`.
 */
typedef struct {
    size_t total_memory;                    /**< Total managed memory (bytes) */
    size_t used_memory;                     /**< Current usage (bytes) */
    size_t peak_memory;                     /**< Peak usage (bytes) */
    size_t num_active_pools;                /**< Number of active pools */
    det_pool_stats_t pool_stats[DET_MAX_POOLS]; /**< Per-pool stats */
    uint64_t wcet_alloc;                    /**< Worst-case cycles for alloc */
    uint64_t wcet_free;                     /**< Worst-case cycles for free */
} det_stats_t;

/* ========================================================================== */
/* Opaque Handle                                                              */
/* ========================================================================== */
/**
 * @brief Opaque allocator handle.
 */
typedef struct det_allocator det_allocator_t;

/* ========================================================================== */
/* Core API                                                                   */
/* ========================================================================== */
/**
 * @brief Compute required buffer size for an allocator configuration.
 *
 * Pure function, does not modify global state.
 *
 * @param config Allocator configuration (non-NULL)
 * @return Required memory size in bytes, or 0 on error
 *
 * @par Complexity
 * O(p) where p = number of pools (bounded by #DET_MAX_POOLS).
 */
DETALLOC_API size_t det_alloc_size(const det_config_t *config);

/**
 * @brief Initialize allocator over a user-provided memory buffer.
 *
 * This is the only routine with potentially variable time cost due to
 * pool setup. All subsequent operations are O(1).
 *
 * @param memory Pointer to pre-allocated memory buffer (non-NULL)
 * @param size   Size of memory buffer in bytes (>= det_alloc_size())
 * @param config Allocator configuration (non-NULL)
 * @return Allocator handle on success, or NULL on error
 *
 * @note The memory buffer must remain valid for the allocator lifetime.
 * @par Complexity
 * O(n) where n is total number of blocks across pools.
 */
DETALLOC_API det_allocator_t *det_alloc_init(void *memory,
                                             size_t size,
                                             const det_config_t *config);

/**
 * @brief Allocate a memory block of at least @p size bytes.
 *
 * @param alloc Allocator handle
 * @param size  Requested size in bytes
 * @return Pointer to allocated memory on success, or NULL if unavailable
 *
 * @note Returned pointer is aligned to #DET_ALIGN.
 * @note Memory is not zero-initialized (use det_calloc()).
 * @par Complexity
 * O(1) guaranteed.
 * @par Thread Safety
 * Safe if `thread_safe=true` in config.
 */
DETALLOC_API void *det_alloc(det_allocator_t *alloc, size_t size);

/**
 * @brief Allocate and zero-initialize a memory block.
 *
 * @param alloc Allocator handle
 * @param size  Requested size in bytes
 * @return Pointer to zeroed memory on success, or NULL on failure
 *
 * @par Complexity
 * O(size) for zeroing, O(1) for allocation.
 */
DETALLOC_API void *det_calloc(det_allocator_t *alloc, size_t size);

/**
 * @brief Free a previously allocated block.
 *
 * @param alloc Allocator handle
 * @param ptr   Pointer returned by det_alloc()/det_calloc() (NULL is a no-op)
 *
 * @warning Freeing an invalid pointer is undefined behavior.
 * @note Double-free detection is available in debug/validation builds.
 * @par Complexity
 * O(1) guaranteed.
 * @par Thread Safety
 * Safe if `thread_safe=true` in config.
 */
DETALLOC_API void det_free(det_allocator_t *alloc, void *ptr);

/**
 * @brief Get the usable size of an allocated block.
 *
 * @param alloc Allocator handle
 * @param ptr   Pointer to allocated memory
 * @return Size in bytes of the block, or 0 if invalid
 *
 * @par Complexity
 * O(1) guaranteed.
 */
DETALLOC_API size_t det_alloc_usable_size(det_allocator_t *alloc, void *ptr);

/**
 * @brief Destroy allocator; releases internal structures.
 *
 * Does **not** free the underlying user-provided memory buffer.
 *
 * @param alloc Allocator handle
 *
 * @par Complexity
 * O(1).
 */
DETALLOC_API void det_alloc_destroy(det_allocator_t *alloc);

/* ========================================================================== */
/* Statistics & Diagnostics                                                   */
/* ========================================================================== */
/**
 * @brief Get allocator statistics snapshot.
 *
 * Only available if `enable_stats=true` in configuration.
 *
 * @param alloc Allocator handle
 * @param stats Output parameter for statistics (non-NULL)
 * @return #DET_OK on success, error code otherwise
 *
 * @note Statistics collection adds minimal overhead (~5%).
 * @par Complexity
 * O(1).
 */
DETALLOC_API det_error_t det_get_stats(det_allocator_t *alloc, det_stats_t *stats);

/**
 * @brief Reset statistics counters (peaks are preserved).
 *
 * @param alloc Allocator handle
 *
 * @par Complexity
 * O(1).
 */
DETALLOC_API void det_reset_stats(det_allocator_t *alloc);

/**
 * @brief Validate internal structures and invariants.
 *
 * Only available in debug builds or when `enable_validation=true`.
 *
 * @param alloc Allocator handle
 * @return `true` if valid; `false` if corruption detected
 *
 * @warning Expensive — use for debugging only.
 * @par Complexity
 * O(n) where n is total number of blocks.
 */
DETALLOC_API bool det_validate(det_allocator_t *alloc);

/**
 * @brief Print allocator state for debugging.
 *
 * Outputs to `stderr` by default (or a custom handler if configured).
 *
 * @param alloc Allocator handle
 *
 * @par Complexity
 * O(n) where n is total number of blocks.
 */
DETALLOC_API void det_debug_print(det_allocator_t *alloc);

/* ========================================================================== */
/* Utilities                                                                  */
/* ========================================================================== */
/**
 * @brief Return a sensible default configuration.
 *
 * @return Default configuration structure.
 */
DETALLOC_API det_config_t det_default_config(void);

/** @name Use-Case Constants
 *  @{ */
#define DET_USE_CASE_AUDIO       1  /**< Audio processing (low latency) */
#define DET_USE_CASE_ROBOTICS    2  /**< Robotics control loops */
#define DET_USE_CASE_NETWORKING  3  /**< Network packet processing */
#define DET_USE_CASE_EMBEDDED    4  /**< Embedded systems (minimal memory) */
/** @} */

/**
 * @brief Create configuration tuned for a common use case.
 *
 * @param use_case One of #DET_USE_CASE_AUDIO, #DET_USE_CASE_ROBOTICS, etc.
 * @return Optimized configuration.
 */
DETALLOC_API det_config_t det_config_for_use_case(int use_case);

/**
 * @brief Suggest pool configuration from expected allocation sizes.
 *
 * @param sizes Array of expected allocation sizes (bytes)
 * @param num   Number of elements in @p sizes
 * @param cfg   Output: suggested configuration
 * @return #DET_OK on success, error code otherwise
 */
DETALLOC_API det_error_t det_suggest_config(const size_t *sizes,
                                            size_t num,
                                            det_config_t *cfg);

/**
 * @brief Get a human-readable error message.
 *
 * @param error Error code
 * @return Static string (do not free)
 */
DETALLOC_API const char *det_error_string(det_error_t error);

/**
 * @brief Get library version string.
 *
 * @return String in the form "major.minor.patch"
 */
DETALLOC_API const char *det_version_string(void);

/* ========================================================================== */
/* Advanced (Optional)                                                        */
/* ========================================================================== */
/**
 * @brief Set custom allocator for internal metadata only.
 *
 * Must be called before det_alloc_init().
 *
 * @param malloc_fn Custom malloc function
 * @param free_fn   Custom free function
 * @return #DET_OK on success, error otherwise
 */
DETALLOC_API det_error_t det_set_internal_allocator(void *(*malloc_fn)(size_t),
                                                    void (*free_fn)(void *));

/**
 * @brief Register an allocation hook (called on every allocation).
 *
 * @param hook     Callback: `(ptr, size, userdata)`
 * @param userdata Opaque pointer passed back on each call
 */
DETALLOC_API void det_set_alloc_hook(void (*hook)(void *ptr, size_t size, void *userdata),
                                     void *userdata);

/**
 * @brief Register a deallocation hook (called on every free).
 *
 * @param hook     Callback: `(ptr, userdata)`
 * @param userdata Opaque pointer passed back on each call
 */
DETALLOC_API void det_set_free_hook(void (*hook)(void *ptr, void *userdata),
                                    void *userdata);

/**
 * @brief Prefetch memory for an impending allocation (performance hint).
 *
 * @param alloc Allocator handle
 * @param size  Expected allocation size in bytes
 *
 * @note This is a hint; behavior is platform-dependent.
 * @par Complexity
 * O(1).
 */
DETALLOC_API void det_prefetch(det_allocator_t *alloc, size_t size);

/* ========================================================================== */
/* Macros & Inline Helpers                                                    */
/* ========================================================================== */
/**
 * @brief Type-safe allocation macro.
 * @code
 * MyType *p = DET_NEW(alloc, MyType);
 * @endcode
 */
#define DET_NEW(alloc, type) \
    ((type *)det_alloc((alloc), sizeof(type)))

/**
 * @brief Type-safe array allocation macro.
 * @code
 * int *arr = DET_NEW_ARRAY(alloc, int, 128);
 * @endcode
 */
#define DET_NEW_ARRAY(alloc, type, count) \
    ((type *)det_alloc((alloc), sizeof(type) * (count)))

/**
 * @brief Safe free macro (sets pointer to NULL).
 */
#define DET_FREE(alloc, ptr) \
    do { det_free((alloc), (ptr)); (ptr) = NULL; } while (0)

/**
 * @brief Align size up to the next multiple of @p align.
 */
#define DET_ALIGN_UP(size, align) \
    (((size) + (align) - 1) & ~((align) - 1))

/**
 * @brief Test if a pointer is aligned to @p align.
 */
#define DET_IS_ALIGNED(ptr, align) \
    (((uintptr_t)(ptr) & ((align) - 1)) == 0)

/**
 * @brief Fast check if @p size fits a @p pool_size (inline hot path).
 */
#ifdef DET_INLINE_FUNCTIONS
DET_INLINE bool det_size_fits_pool(size_t size, size_t pool_size) {
    return size <= pool_size;
}
#endif /* DET_INLINE_FUNCTIONS */

/* ========================================================================== */
/* Platform Helpers (Timing / Barriers)                                       */
/* ========================================================================== */
/**
 * @brief Get a CPU cycle counter (best-effort, platform-specific).
 *
 * Used internally for WCET measurements and statistics.
 *
 * @return Current CPU cycle count (monotonic best-effort on unknown targets)
 */
DET_INLINE uint64_t det_get_cycles(void) {
#if defined(__x86_64__) || defined(__i386__)
    uint32_t lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
#elif defined(__aarch64__)
    uint64_t val;
    __asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(val));
    return val;
#elif defined(__arm__)
    uint32_t val;
    __asm__ __volatile__("mrc p15, 0, %0, c9, c13, 0" : "=r"(val));
    return (uint64_t)val;
#else
    static uint64_t counter = 0;
    #if defined(__GNUC__) || defined(__clang__)
    return __sync_fetch_and_add(&counter, 1);
    #else
    return ++counter;
    #endif
#endif
}

/**
 * @brief Full memory barrier for lock-free operations.
 */
DET_INLINE void det_memory_barrier(void) {
#if defined(__GNUC__) || defined(__clang__)
    __sync_synchronize();
#elif defined(_MSC_VER)
    _ReadWriteBarrier();
#else
    (void)0;
#endif
}

/* ========================================================================== */
/* Compile-Time Controls                                                      */
/* ========================================================================== */
/**
 * @def DET_ALLOC_NO_STATS
 * @brief If defined, statistics counters compile to no-ops.
 */
#ifdef DET_ALLOC_NO_STATS
# define DET_STATS_INC(x)      ((void)0)
# define DET_STATS_ADD(x, v)   ((void)0)
#else
# define DET_STATS_INC(x)      do { (x)++; } while (0)
# define DET_STATS_ADD(x, v)   do { (x) += (v); } while (0)
#endif

/**
 * @def DET_ALLOC_VALIDATE
 * @brief If defined, validation asserts become active.
 */
#ifdef DET_ALLOC_VALIDATE
/**
 * @brief Validation failure handler (internal).
 *
 * @param cond Expression text that failed
 * @param file Source file
 * @param line Line number
 */
DETALLOC_API void det_assert_fail(const char *cond, const char *file, int line);
# define DET_ASSERT(cond) do { if (!(cond)) det_assert_fail(#cond, __FILE__, __LINE__); } while (0)
#else
# define DET_ASSERT(cond) ((void)0)
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* DETALLOC_H */
