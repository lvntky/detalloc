# detalloc: Deterministic Real-Time Memory Allocator
# C99 Standard - Constant-time operations guaranteed

PROJECT = detalloc
VERSION = 0.1.0

# Compiler and tools
CC = gcc
AR = ar
RANLIB = ranlib
RM = rm -f
MKDIR = mkdir -p

# Directories
SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
LIB_DIR = lib
TEST_DIR = tests
BENCH_DIR = benchmarks
EXAMPLE_DIR = examples
BUILD_DIR = build
DOCS_DIR = docs

# Installation
PREFIX ?= /usr/local
INSTALL_LIB_DIR = $(PREFIX)/lib
INSTALL_INC_DIR = $(PREFIX)/include

# Base compiler flags
CFLAGS = -std=c99 -Wall -Wextra -Wpedantic -Werror
CFLAGS += -I$(INC_DIR)
CFLAGS += -fPIC
CFLAGS += -Wstrict-aliasing -Wcast-align
CFLAGS += -fno-omit-frame-pointer

# Real-time specific flags
RT_FLAGS = -DRT_ALLOC_STATS -DRT_ALLOC_VALIDATE

# Debug flags (includes sanitizers but no thread sanitizer for RT code)
DEBUG_FLAGS = -g -O0 -DDEBUG
DEBUG_FLAGS += -fsanitize=address -fsanitize=undefined
DEBUG_FLAGS += $(RT_FLAGS)

# Release flags (optimized for determinism, not just speed)
RELEASE_FLAGS = -O2 -DNDEBUG
RELEASE_FLAGS += -fno-tree-vectorize  # Disable auto-vectorization for determinism
RELEASE_FLAGS += -fno-strict-aliasing
RELEASE_FLAGS += $(RT_FLAGS)

# Hardcore optimization (use with caution - test determinism)
PERF_FLAGS = -O3 -march=native -mtune=native
PERF_FLAGS += -DNDEBUG -DRT_ALLOC_NO_STATS

# Linker flags
BASE_LDFLAGS = -pthread -lm
DEBUG_LDFLAGS = $(BASE_LDFLAGS) -fsanitize=address -fsanitize=undefined
RELEASE_LDFLAGS = $(BASE_LDFLAGS)
LDFLAGS = $(BASE_LDFLAGS)

# Library names
STATIC_LIB = $(LIB_DIR)/lib$(PROJECT).a
SHARED_LIB = $(LIB_DIR)/lib$(PROJECT).so.$(VERSION)
SHARED_LIB_LINK = $(LIB_DIR)/lib$(PROJECT).so

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SOURCES))

# Test files
TEST_SOURCES = $(wildcard $(TEST_DIR)/*.c)
TEST_BINS = $(patsubst $(TEST_DIR)/%.c,$(BUILD_DIR)/test_%,$(TEST_SOURCES))

# Benchmark files
BENCH_SOURCES = $(wildcard $(BENCH_DIR)/*.c)
BENCH_BINS = $(patsubst $(BENCH_DIR)/%.c,$(BUILD_DIR)/bench_%,$(BENCH_SOURCES))

# Example files
EXAMPLE_SOURCES = $(wildcard $(EXAMPLE_DIR)/*.c)
EXAMPLE_BINS = $(patsubst $(EXAMPLE_DIR)/%.c,$(BUILD_DIR)/%,$(EXAMPLE_SOURCES))

# Default target
.PHONY: all
all: release

# Release build
.PHONY: release
release: CFLAGS += $(RELEASE_FLAGS)
release: LDFLAGS := $(RELEASE_LDFLAGS)
release: directories $(STATIC_LIB) $(SHARED_LIB)

# Debug build
.PHONY: debug
debug: CFLAGS += $(DEBUG_FLAGS)
debug: LDFLAGS := $(DEBUG_LDFLAGS)
debug: directories $(STATIC_LIB) $(SHARED_LIB)

# Performance build
.PHONY: perf
perf: CFLAGS += $(PERF_FLAGS)
perf: LDFLAGS := $(RELEASE_LDFLAGS)
perf: directories $(STATIC_LIB) $(SHARED_LIB)

# Create directories
.PHONY: directories
directories:
	@$(MKDIR) $(OBJ_DIR) $(LIB_DIR) $(BUILD_DIR) $(DOCS_DIR)

# Compile source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@
	@$(CC) $(CFLAGS) -MM -MT $@ $< > $(OBJ_DIR)/$*.d

# Build static library
$(STATIC_LIB): $(OBJECTS)
	@$(MKDIR) $(dir $@)
	$(AR) rcs $@ $^
	$(RANLIB) $@

# Build shared library
$(SHARED_LIB): $(OBJECTS)
	@$(MKDIR) $(dir $@)
	$(CC) -shared -Wl,-soname,lib$(PROJECT).so.0 -o $@ $^ $(LDFLAGS)
	@ln -sf lib$(PROJECT).so.$(VERSION) $(SHARED_LIB_LINK)

# Build tests
.PHONY: tests
tests: CFLAGS += $(DEBUG_FLAGS)
tests: LDFLAGS := $(DEBUG_LDFLAGS)
tests: $(STATIC_LIB) $(TEST_BINS)

$(BUILD_DIR)/test_%: $(TEST_DIR)/%.c $(STATIC_LIB)
	@$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) $< -L$(LIB_DIR) -l$(PROJECT) $(LDFLAGS) -o $@

# Build benchmarks
.PHONY: benchmarks
benchmarks: CFLAGS += $(RELEASE_FLAGS)
benchmarks: LDFLAGS := $(RELEASE_LDFLAGS)
benchmarks: $(STATIC_LIB) $(BENCH_BINS)

$(BUILD_DIR)/bench_%: $(BENCH_DIR)/%.c $(STATIC_LIB)
	@$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) $< -L$(LIB_DIR) -l$(PROJECT) $(LDFLAGS) -o $@

# Build examples
.PHONY: examples
examples: release $(EXAMPLE_BINS)

$(BUILD_DIR)/%: $(EXAMPLE_DIR)/%.c $(STATIC_LIB)
	@$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) $(RELEASE_FLAGS) $< -L$(LIB_DIR) -l$(PROJECT) $(RELEASE_LDFLAGS) -o $@

# Run tests
.PHONY: test
test: tests
	@echo "=== Running Unit Tests ==="
	@for test in $(TEST_BINS); do \
		echo "Running $$test..."; \
		$$test || exit 1; \
	done
	@echo "=== All tests passed ==="

# Run benchmarks
.PHONY: bench
bench: benchmarks
	@echo "=== Running Benchmarks ==="
	@for bench in $(BENCH_BINS); do \
		echo ""; \
		$$bench; \
	done

# Determinism test - run benchmarks multiple times and compare results
.PHONY: determinism-test
determinism-test: benchmarks
	@echo "=== Testing Determinism (10 runs) ==="
	@for bench in $(BENCH_BINS); do \
		echo "Testing $$bench..."; \
		for i in 1 2 3 4 5 6 7 8 9 10; do \
			$$bench | grep "cycles" | tee -a /tmp/rt_det_$$i.log; \
		done; \
		if [ `cat /tmp/rt_det_*.log | sort -u | wc -l` -eq 1 ]; then \
			echo "✓ Deterministic"; \
		else \
			echo "✗ Non-deterministic results detected"; \
			cat /tmp/rt_det_*.log | sort | uniq -c; \
		fi; \
		rm -f /tmp/rt_det_*.log; \
	done

# Latency analysis
.PHONY: latency-test
latency-test: benchmarks
	@echo "=== Latency Analysis ==="
	@echo "Testing worst-case execution time (WCET)..."
	@$(BUILD_DIR)/bench_latency || true

# Real-time performance under load
.PHONY: stress-test
stress-test: benchmarks
	@echo "=== Stress Testing ==="
	@$(BUILD_DIR)/bench_stress || echo "Create bench_stress.c for stress testing"

# Install
.PHONY: install
install: release
	@$(MKDIR) $(INSTALL_LIB_DIR) $(INSTALL_INC_DIR)/$(PROJECT)
	install -m 644 $(STATIC_LIB) $(INSTALL_LIB_DIR)/
	install -m 755 $(SHARED_LIB) $(INSTALL_LIB_DIR)/
	ln -sf lib$(PROJECT).so.$(VERSION) $(INSTALL_LIB_DIR)/lib$(PROJECT).so
	ln -sf lib$(PROJECT).so.$(VERSION) $(INSTALL_LIB_DIR)/lib$(PROJECT).so.0
	install -m 644 $(INC_DIR)/*.h $(INSTALL_INC_DIR)/$(PROJECT)/
	ldconfig || true

# Uninstall
.PHONY: uninstall
uninstall:
	$(RM) $(INSTALL_LIB_DIR)/lib$(PROJECT).*
	$(RM) -r $(INSTALL_INC_DIR)/$(PROJECT)
	ldconfig || true

# Clean
.PHONY: clean
clean:
	$(RM) -r $(OBJ_DIR) $(LIB_DIR) $(BUILD_DIR)

.PHONY: distclean
distclean: clean
	$(RM) -r $(DOCS_DIR) *.tar.gz *.zip

# Format code
.PHONY: format
format:
	@find $(SRC_DIR) $(INC_DIR) $(TEST_DIR) $(BENCH_DIR) $(EXAMPLE_DIR) \
		-name '*.c' -o -name '*.h' | xargs clang-format -i

# Static analysis
.PHONY: analyze
analyze:
	@echo "=== Static Analysis ==="
	cppcheck --enable=all --std=c99 --suppress=missingIncludeSystem \
		--inline-suppr -I$(INC_DIR) $(SRC_DIR)

# Documentation
.PHONY: docs
docs:
	@echo "=== Generating Documentation (m.css dark) ==="
	@PROJECT_NUMBER=$(VERSION) ./docs/Doxygen/gen_docs.sh

# Memory check (limited use for RT systems, but useful for correctness)
.PHONY: memcheck
memcheck: debug tests
	@echo "=== Memory Leak Check ==="
	@for test in $(TEST_BINS); do \
		echo "Checking $$test"; \
		valgrind --leak-check=full --show-leak-kinds=all \
			--error-exitcode=1 $$test; \
	done

# Performance profiling
.PHONY: profile
profile: CFLAGS += -pg
profile: LDFLAGS += -pg
profile: clean benchmarks
	@echo "=== Running with gprof ==="
	@$(BUILD_DIR)/bench_alloc
	@gprof $(BUILD_DIR)/bench_alloc gmon.out > $(DOCS_DIR)/profile.txt
	@echo "Profile saved to $(DOCS_DIR)/profile.txt"

# Callgrind analysis
.PHONY: callgrind
callgrind: release benchmarks
	@echo "=== Callgrind Analysis ==="
	@valgrind --tool=callgrind --callgrind-out-file=$(DOCS_DIR)/callgrind.out \
		$(BUILD_DIR)/bench_alloc
	@echo "Analyze with: kcachegrind $(DOCS_DIR)/callgrind.out"

# Cache analysis
.PHONY: cachegrind
cachegrind: release benchmarks
	@echo "=== Cache Performance Analysis ==="
	@valgrind --tool=cachegrind --cachegrind-out-file=$(DOCS_DIR)/cachegrind.out \
		$(BUILD_DIR)/bench_alloc
	@cg_annotate $(DOCS_DIR)/cachegrind.out

# Help
.PHONY: help
help:
	@echo "RT-Alloc: Deterministic Real-Time Memory Allocator"
	@echo ""
	@echo "Build Targets:"
	@echo "  all              - Build release version (default)"
	@echo "  release          - Build optimized release version"
	@echo "  debug            - Build debug version with sanitizers"
	@echo "  perf             - Build high-performance version"
	@echo ""
	@echo "Test Targets:"
	@echo "  tests            - Build test suite"
	@echo "  test             - Build and run tests"
	@echo "  determinism-test - Verify deterministic behavior"
	@echo "  latency-test     - Measure worst-case execution time"
	@echo "  stress-test      - Stress test under load"
	@echo "  memcheck         - Check for memory leaks"
	@echo ""
	@echo "Benchmark Targets:"
	@echo "  benchmarks       - Build benchmarks"
	@echo "  bench            - Run all benchmarks"
	@echo "  profile          - Profile with gprof"
	@echo "  callgrind        - Analyze with callgrind"
	@echo "  cachegrind       - Cache performance analysis"
	@echo ""
	@echo "Other Targets:"
	@echo "  examples         - Build example programs"
	@echo "  install          - Install library system-wide"
	@echo "  uninstall        - Remove installed library"
	@echo "  clean            - Remove build artifacts"
	@echo "  format           - Format code with clang-format"
	@echo "  analyze          - Run static analysis"
	@echo "  docs             - Generate documentation"
	@echo "  help             - Show this help"

.PRECIOUS: $(OBJ_DIR)/%.o

-include $(OBJECTS:.o=.d)
