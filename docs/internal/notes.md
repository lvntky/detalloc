Implementation Phases
Phase 1: Simple Pool Allocator (Week 1-2)

Single fixed size (e.g., 64 bytes)
Bitmap-based allocation
Basic tests and benchmarks
Goal: Prove O(1) concept

Phase 2: Multiple Size Classes (Week 3-4)

8-10 different pool sizes
Size-to-pool mapping
Fragmentation handling
Goal: Practical usability

Phase 3: Optimization (Week 5-6)

Cache-line alignment
Prefetching hints
SIMD for bitmap operations
Goal: Minimize latency

Phase 4: Features (Week 7-8)

Statistics collection
Debug instrumentation
Memory validation
Goal: Production-ready

Phase 5: Documentation & Examples (Week 9-10)

API documentation
Real-world examples
Performance comparison
Goal: Release v1.0

Critical Benchmarks to Implement

Latency Histogram: Distribution of allocation times
WCET Test: Maximum observed time over millions of ops
Jitter Test: Variance in timing
Fragmentation: Memory efficiency over time
Cache Performance: Miss rates and bandwidth
Comparison: vs malloc, vs other RT allocators

Resources
Papers to Read

"The Slab Allocator" - Jeff Bonwick (1994)
"Hoard: A Scalable Memory Allocator" - Berger et al. (2000)
"Constant-Time Dynamic Memory Allocation" - Masmano et al. (2008)

Similar Projects to Study

TLSF (Two-Level Segregate Fit) - O(1) but allows coalescing
EASTL - Electronic Arts STL with custom allocators
Jemalloc - Not strictly O(1) but very fast
Portable Lock-Free Memory Allocator - IBM research


first:
Read the TLSF paper for inspiration
Implement basic pool allocator
Write comprehensive tests
Benchmark against malloc
Document worst-case guarantees