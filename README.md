# Detalloc — Deterministic Real-Time Memory Allocator

<img src="./docs/det_logo.png" alt="Detalloc logo" width="480">

**Detalloc** is a **constant-time (O(1))**, **deterministic** memory allocator built for **hard real-time systems** and **latency-critical applications**.  
Every allocation, every free, and every metadata update executes in **predictable, bounded time**.

> Designed for environments where unpredictability is unacceptable — audio DSP loops, robotics control, embedded systems, and high-frequency software pipelines.

---

## Why Detalloc Exists

Most general-purpose allocators (e.g. `malloc`, `jemalloc`, `tcmalloc`) optimize for throughput, not predictability.  
They have variable latency, global locks, and system calls that can stall real-time threads.

**Detalloc** takes a different approach:

- **No system calls after init** — fully static memory pool.
- **O(1)** for all operations — allocation, deallocation, lookup.
- **Deterministic execution time** — same input, same timing.
- **No fragmentation growth** — pools are pre-partitioned.
- **Optional thread safety** — without compromising determinism.
- **WCET instrumentation** — precise cycle counters for performance auditing.

It’s a tool for those who care about **temporal correctness**, not just correctness.

---

## Key Features

| Capability | Description |
|-------------|-------------|
| **O(1) allocation/free** | Each operation has constant time, independent of pool size |
| **Preallocated memory pools** | No dynamic memory requests, ever |
| **Zero syscalls after init** | Fully deterministic runtime |
| **WCET tracking** | Measure and verify worst-case execution times |
| **Pool-level statistics** | Peak usage, allocation/failure counters, total cycles |
| **Validation & Debug hooks** | Detect corruption or misalignment in debug builds |
| **Thread-safe optional mode** | Lock-free or spin-lock mode for multi-core systems |
| **Platform-portable** | Tested on x86_64, ARM, and embedded toolchains |

---

## Example Usage

```c
#include "detalloc.h"

#define MEMORY_SIZE 4096
static uint8_t buffer[MEMORY_SIZE];

int main(void) {
    det_config_t cfg = det_default_config();
    size_t required = det_alloc_size(&cfg);

    det_allocator_t *alloc = det_alloc_init(buffer, required, &cfg);
    if (!alloc) {
        fprintf(stderr, "Initialization failed: %s\n", det_error_string(DET_ERR_INVALID_PARAM));
        return 1;
    }

    int *values = DET_NEW_ARRAY(alloc, int, 128);
    for (int i = 0; i < 128; ++i)
        values[i] = i * 42;

    det_free(alloc, values);

    det_debug_print(alloc);
    det_alloc_destroy(alloc);
}
```

Output:
```
[detalloc] total pools: 10
[detalloc] peak usage: 512 bytes
[detalloc] WCET alloc: 38 cycles, free: 29 cycles
```

---

## Configuration Overview

Detalloc uses **configurable pools** defined at startup.

```c
det_config_t cfg = det_default_config();
cfg.enable_stats = true;
cfg.enable_validation = true;
cfg.thread_safe = false;
```

You can also tailor it for your use case:

```c
cfg = det_config_for_use_case(DET_USE_CASE_AUDIO);
```

Available presets:
| Constant | Description |
|-----------|-------------|
| `DET_USE_CASE_AUDIO` | Audio processing (low latency) |
| `DET_USE_CASE_ROBOTICS` | Robotics control loops |
| `DET_USE_CASE_NETWORKING` | Network packet buffers |
| `DET_USE_CASE_EMBEDDED` | Ultra-minimal embedded footprint |

Or generate one dynamically:
```c
size_t sizes[] = { 64, 128, 256, 1024 };
det_suggest_config(sizes, 4, &cfg);
```

---

## Design Principles

**Detalloc** is inspired by allocator research and low-latency system design philosophies:

1. **Predictability > Throughput** — A consistent 0.5 µs beat is superior to occasional 0.1 µs spikes.
2. **Memory locality** — Fixed-size blocks are cache-aligned.
3. **Static analysis friendly** — WCET can be proven analytically.
4. **Portable determinism** — No reliance on undefined OS semantics.
5. **Auditable** — Every branch of the critical path is measurable in cycles.

---

## Internal Architecture

The allocator core uses **size-segregated free lists**, with **fixed block pools** configured at initialization.

```
[ Memory Buffer ]
 ├── Pool 0 (8 bytes x N)
 ├── Pool 1 (16 bytes x N)
 ├── Pool 2 (32 bytes x N)
 ├── ...
 └── Metadata Table
```

Each pool maintains:
- A singly-linked **free list** for O(1) allocation.
- A **usage bitmap** for validation.
- Per-pool **statistics counters** (optionally compiled out).

No block splitting, no merging, no sbrk(), no fragmentation drift.

---

## Advanced Features

- **Custom Allocator Hooks**  
  Plug in your own functions for instrumentation:
  ```c
  det_set_alloc_hook(on_alloc, userdata);
  det_set_free_hook(on_free, userdata);
  ```

- **Internal Allocator Override**  
  For metadata structures:
  ```c
  det_set_internal_allocator(custom_malloc, custom_free);
  ```

- **Prefetching Hint**
  ```c
  det_prefetch(alloc, 256);
  ```

- **Debug Inspection**
  ```c
  det_debug_print(alloc);
  ```

---

## Statistics Example

```c
det_stats_t stats;
det_get_stats(alloc, &stats);
printf("Total memory: %zu\nPeak memory: %zu\n", stats.total_memory, stats.peak_memory);
```

| Field | Description |
|--------|-------------|
| `wcet_alloc` | Worst-case cycles per allocation |
| `peak_memory` | Maximum simultaneous memory usage |
| `pool_stats[]` | Per-size-class statistics |

---

## Determinism Validation

```c
if (!det_validate(alloc)) {
    fprintf(stderr, "Allocator corruption detected\n");
}
```

Run with `enable_validation=true` to catch:
- Double frees
- Invalid pointer returns
- Bitmap corruption
- Alignment drift

---

## Performance Metrics

| Platform | Operation | Avg Cycles | WCET Cycles | Notes |
|-----------|------------|-------------|--------------|-------|
| x86_64 GCC 14 | `det_alloc(64)` | 32 | 38 | Deterministic |
| ARMv8-A | `det_free()` | 27 | 33 | Cache aligned |
| STM32F4 | `det_alloc(16)` | 43 | 47 | Bare-metal mode |

Measured via `det_get_cycles()` in Release builds.

---

## Portability

| Platform | Status | Notes |
|-----------|--------|-------|
| Linux (x86_64) | Ready | GCC / Clang |
| macOS (ARM64) | Ready | M1 tested |
| FreeRTOS | Ready | Static linking only |
| STM32 (bare metal) | Ready | via OpenCM3 |
| Windows (MSVC) | In progress | Build flag adjustments needed |

---

## Building

A minimal example with CMake:

```cmake
cmake_minimum_required(VERSION 3.20)
project(detalloc LANGUAGES C)
add_library(detalloc detalloc.c)
target_include_directories(detalloc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(detalloc PUBLIC c99)
```

Then:

```bash
mkdir build && cd build
cmake ..
make
```

Run your test binary:
```bash
./example
```

---

## Documentation

Generate documentation using **Doxygen + m.css**:

```bash
doxygen Doxyfile
```

The output will be located in:
```
docs/build/html/index.html
```

---

## License

MIT License  
(c) 2025 Levent Kaya

Detalloc is provided without warranty. Use it, measure it, modify it — but always respect determinism.

---

## Philosophy

> “Speed doesn’t matter if you can’t predict it.”  
> — Real-time systems proverb

Detalloc is a statement against the unpredictable.  
It’s for engineers who think in microseconds, who design systems where 0.5 µs of jitter is a failure, and who want their allocator to behave like clockwork.

---

## Repository Structure

```
detalloc/
 ├── detalloc.h        # Public header
 ├── detalloc.c        # Implementation
 ├── tests/            # Unit and latency tests
 ├── examples/         # Usage examples
 ├── docs/             # Doxygen + m.css setup
 ├── Doxyfile
 └── README.md
```

---

## Roadmap

- [ ] Optional lock-free ring allocator variant  
- [ ] RTOS integration layer (FreeRTOS, Zephyr)  
- [ ] Deterministic aligned arena API  
- [ ] Static WCET verification tool  
- [ ] Rust FFI bindings  

---

## Contributing

Detalloc welcomes contributions that improve **determinism, portability, or analysis**.

Before sending a PR:
- Run `clang-format` (LLVM style)
- Include `det_validate()` in debug builds
- Ensure WCET deltas remain within ±2 cycles

---

## Citation

If you use Detalloc in research or production, cite it as:

```
Kaya, L. (2025). Detalloc: Deterministic Real-Time Memory Allocator.
https://github.com/lvntky/detalloc
```

---

Detalloc is not just a memory allocator.  
It is a guarantee that your system will never miss its deadline.
