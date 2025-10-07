# detalloc

**Deterministic, constant-time pool allocator** for hard real-time systems.  
Phase-1 surface is intentionally small: single fixed-size pool with O(1) alloc/free.

## Design Goals
- Bounded latency (WCET tracked) • No syscalls after init • Cache-friendly bitmaps

## Quick Start

```c
#include <detalloc.h>

#define POOL_BLOCK_SIZE 64
#define POOL_BLOCKS     1024

static uint8_t arena[1<<20]; /* example: use det_alloc_size() to size properly */

int main(void) {
    det_config_t cfg = det_default_config();
    cfg.block_size = POOL_BLOCK_SIZE;
    cfg.num_blocks = POOL_BLOCKS;

    size_t need = det_alloc_size(&cfg);
    det_allocator_t* A = det_alloc_init(arena, sizeof(arena), &cfg);

    void* p = det_alloc(A);
    det_free(A, p);

    det_alloc_destroy(A);
}
