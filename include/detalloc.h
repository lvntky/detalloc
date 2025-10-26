/**
 * @file detalloc.h
 * @brief Detalloc — Phase 1: Single-Pool, Constant-Time Allocator
 *
 * Minimal API for a fixed-size, bitmap-based pool allocator with O(1)
 * alloc/free. Designed for hard real-time use; no syscalls after init; uses
 * user-provided memory.
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
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ========================================================================== */
/* Visibility / Inline                                                        */
/* ========================================================================== */
/**
 * @def DETALLOC_API
 * @brief Export/import macro for building/using shared libraries.
 */
#ifndef DETALLOC_API
#if defined(_WIN32) && !defined(__GNUC__)
#ifdef DETALLOC_BUILD
#define DETALLOC_API __declspec(dllexport)
#else
#define DETALLOC_API __declspec(dllimport)
#endif
#else
#define DETALLOC_API __attribute__((visibility("default")))
#endif
#endif

/**
 * @def DET_INLINE
 * @brief Always-inline helper for hot-path functions where supported.
 */
#ifndef DET_INLINE
#if defined(__GNUC__) || defined(__clang__)
#define DET_INLINE static inline __attribute__((always_inline))
#else
#define DET_INLINE static inline
#endif
#endif

/* ========================================================================== */
/* Version                                                                    */
/* ========================================================================== */
#define DETALLOC_VERSION_MAJOR 0
#define DETALLOC_VERSION_MINOR 1
#define DETALLOC_VERSION_PATCH 0

/* ========================================================================== */
/* Defaults & Align                                                           */
/* ========================================================================== */
/** Default block size for Phase 1 (can be overridden at init-time). */
#ifndef DET_DEFAULT_BLOCK_SIZE
#define DET_DEFAULT_BLOCK_SIZE 64
#endif

/** Default alignment for returned blocks (power of two). */
#ifndef DET_DEFAULT_ALIGN
#define DET_DEFAULT_ALIGN 8
#endif

/** Align up helper. */
#ifndef DET_ALIGN_UP
#define DET_ALIGN_UP(sz, a) (((sz) + ((a)-1)) & ~((a)-1))
#endif

/* ========================================================================== */
/* Error Codes                                                                */
/* ========================================================================== */
/**
 * @brief Error/status codes returned by Detalloc.
 */
typedef enum {
  DET_OK = 0,             /**< Operation successful */
  DET_ERR_INVALID_PARAM,  /**< A parameter is invalid */
  DET_ERR_OUT_OF_MEMORY,  /**< Buffer cannot accommodate configuration */
  DET_ERR_POOL_FULL,      /**< Pool has no free blocks */
  DET_ERR_INVALID_PTR,    /**< Pointer not owned by allocator/pool */
  DET_ERR_NOT_INITIALIZED /**< Allocator not initialized */
} det_error_t;

/* ========================================================================== */
/* Opaque Handle                                                              */
/* ========================================================================== */
/**
 * @brief Opaque allocator handle (single pool in Phase 1).
 *
 * Implementation maintains:
 *  - user buffer base/limit
 *  - bitmap for free/used slots
 *  - fixed block size & count
 *  - optional lock if thread_safe=true
 */
typedef struct det_allocator det_allocator_t;

/* ========================================================================== */
/* Configuration (Phase 1)                                                    */
/* ========================================================================== */
/**
 * @brief Phase-1 configuration: one fixed-size pool.
 */
typedef struct {
  size_t block_size; /**< Size of each block in bytes (e.g., 64). */
  size_t num_blocks; /**< Number of blocks in the pool. */
  size_t align; /**< Alignment for allocations (default DET_DEFAULT_ALIGN). */
  bool thread_safe; /**< Optional: enable internal locking (constant-time). */
} det_config_t;

/* ========================================================================== */
/* Core API                                                                   */
/* ========================================================================== */
/**
 * @brief Compute required buffer size for a given Phase-1 config.
 *
 * Calculates metadata + bitmap + aligned payload area.
 *
 * @param config Non-NULL configuration
 * @return Required bytes, or 0 on error
 *
 * @par Complexity
 * O(1).
 */
DETALLOC_API size_t det_alloc_size(const det_config_t *config);

/**
 * @brief Initialize allocator over a user-provided memory buffer.
 *
 * No dynamic allocation; all metadata lives inside @p memory.
 *
 * @param memory Pointer to pre-allocated memory buffer (non-NULL)
 * @param size   Size of memory buffer in bytes (>= det_alloc_size(config))
 * @param config Non-NULL Phase-1 configuration
 * @return Allocator handle on success, or NULL on error
 *
 * @note The buffer must remain valid for the allocator lifetime.
 * @par Complexity
 * O(1).
 */
DETALLOC_API det_allocator_t *det_alloc_init(void *memory, size_t size,
                                             const det_config_t *config);

/**
 * @brief Allocate a single fixed-size block.
 *
 * @param alloc Allocator handle
 * @return Pointer to block on success, or NULL if pool is full
 *
 * @note Returned pointer is aligned to config.align.
 * @par Complexity
 * O(1) worst-case.
 */
DETALLOC_API void *det_alloc(det_allocator_t *alloc);

/**
 * @brief Allocate a zero-initialized block.
 *
 * @param alloc Allocator handle
 * @return Pointer to zeroed block, or NULL if pool is full
 *
 * @par Complexity
 * O(block_size) for zeroing, O(1) for allocation.
 */
DETALLOC_API void *det_calloc(det_allocator_t *alloc);

/**
 * @brief Free a previously allocated block (NULL is a no-op).
 *
 * @param alloc Allocator handle
 * @param ptr   Pointer returned by det_alloc()/det_calloc()
 *
 * @warning Undefined behavior if @p ptr was not allocated by this allocator.
 * @par Complexity
 * O(1) worst-case.
 */
DETALLOC_API void det_free(det_allocator_t *alloc, void *ptr);

/**
 * @brief Return usable size of a block.
 *
 * @param alloc Allocator handle
 * @param ptr   Pointer to allocated block
 * @return Fixed block size (or 0 if invalid)
 *
 * @par Complexity
 * O(1).
 */
DETALLOC_API size_t det_alloc_usable_size(det_allocator_t *alloc, void *ptr);

/**
 * @brief Destroy allocator structures (metadata cleanup only).
 *
 * The user-provided memory buffer is **not** freed.
 *
 * @param alloc Allocator handle
 *
 * @par Complexity
 * O(1).
 */
DETALLOC_API void det_alloc_destroy(det_allocator_t *alloc);

/* ========================================================================== */
/* Convenience                                                                */
/* ========================================================================== */
/**
 * @brief Return a sensible Phase-1 default config.
 *
 * Defaults:
 *  - block_size = DET_DEFAULT_BLOCK_SIZE
 *  - num_blocks = 0 (must be set by user)
 *  - align      = DET_DEFAULT_ALIGN
 *  - thread_safe = false
 */
DETALLOC_API det_config_t det_default_config(void);

/**
 * @brief Get library version string "major.minor.patch".
 */
DETALLOC_API const char *det_version_string(void);

/* ========================================================================== */
/* Macros                                                                     */
/* ========================================================================== */
/** Allocate a typed object (one fixed-size block must fit it). */
#define DET_NEW(alloc, type) ((type *)det_alloc((alloc)))

/** Free and null a pointer. */
#define DET_FREE(alloc, ptr)                                                   \
  do {                                                                         \
    det_free((alloc), (ptr));                                                  \
    (ptr) = NULL;                                                              \
  } while (0)

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* DETALLOC_H */
