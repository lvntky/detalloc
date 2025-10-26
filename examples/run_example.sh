#!/bin/bash

# compile.sh - Compile detalloc examples without using Makefile
# Place this script in the examples/ directory
# Usage: ./compile.sh [source.c] [output_name]

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Project paths (relative to examples directory)
INCLUDE_DIR="../include"
LIB_DIR="../lib"
LIB_NAME="detalloc"

# Function to print colored messages
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# Function to show usage
show_usage() {
    cat << EOF
${GREEN}Detalloc Examples Compilation Script${NC}

Usage:
  $0                          # Interactive mode
  $0 <source.c>               # Use source basename as output
  $0 <source.c> <output>      # Specify both source and output

Examples:
  $0                          # Will prompt for input
  $0 version.c                # Compiles to 'version'
  $0 version.c my_program     # Compiles to 'my_program'

Options:
  -h, --help                  # Show this help message
  -d, --debug                 # Compile with debug flags (-g -O0)
  -v, --verbose               # Show compilation command

Environment Variables:
  CC          Compiler (default: gcc)
  CFLAGS      Additional compiler flags
  LDFLAGS     Additional linker flags
EOF
}

# Parse command line arguments
DEBUG_MODE=0
VERBOSE_MODE=0
SOURCE_FILE=""
OUTPUT_NAME=""

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_usage
            exit 0
            ;;
        -d|--debug)
            DEBUG_MODE=1
            shift
            ;;
        -v|--verbose)
            VERBOSE_MODE=1
            shift
            ;;
        *)
            if [ -z "$SOURCE_FILE" ]; then
                SOURCE_FILE="$1"
            elif [ -z "$OUTPUT_NAME" ]; then
                OUTPUT_NAME="$1"
            else
                print_error "Too many arguments"
                show_usage
                exit 1
            fi
            shift
            ;;
    esac
done

# Interactive mode if no source file provided
if [ -z "$SOURCE_FILE" ]; then
    echo -e "${GREEN}=== Detalloc Examples Compiler ===${NC}\n"

    # List available .c files in current directory
    C_FILES=(*.c)
    if [ -e "${C_FILES[0]}" ]; then
        print_info "Available C files:"
        for i in "${!C_FILES[@]}"; do
            echo "  $((i+1)). ${C_FILES[$i]}"
        done
        echo ""
    fi

    read -p "Enter source file name (e.g., version.c): " SOURCE_FILE

    if [ -z "$SOURCE_FILE" ]; then
        print_error "Source file name cannot be empty"
        exit 1
    fi
fi

# Check if source file exists
if [ ! -f "$SOURCE_FILE" ]; then
    print_error "Source file '$SOURCE_FILE' not found"
    exit 1
fi

# Generate output name if not provided
if [ -z "$OUTPUT_NAME" ]; then
    # Remove path and extension from source file
    OUTPUT_NAME=$(basename "$SOURCE_FILE" .c)
    read -p "Enter output name [default: $OUTPUT_NAME]: " INPUT_OUTPUT
    if [ ! -z "$INPUT_OUTPUT" ]; then
        OUTPUT_NAME="$INPUT_OUTPUT"
    fi
fi

# Check if library exists
if [ ! -f "$LIB_DIR/lib${LIB_NAME}.a" ]; then
    print_error "Library not found at $LIB_DIR/lib${LIB_NAME}.a"
    print_info "Please run 'make release' in the project root first"
    exit 1
fi

# Check if header exists
if [ ! -f "$INCLUDE_DIR/detalloc.h" ]; then
    print_error "Header file not found at $INCLUDE_DIR/detalloc.h"
    exit 1
fi

# Compiler settings
CC="${CC:-gcc}"
BASE_CFLAGS="-std=c99 -Wall -Wextra -Wpedantic"

if [ $DEBUG_MODE -eq 1 ]; then
    BASE_CFLAGS="$BASE_CFLAGS -g -O0 -DDEBUG"
    print_info "Compiling in DEBUG mode"
else
    BASE_CFLAGS="$BASE_CFLAGS -O2"
fi

# Build the compilation command (use static library)
COMPILE_CMD="$CC $BASE_CFLAGS $CFLAGS -I$INCLUDE_DIR $SOURCE_FILE $LIB_DIR/lib${LIB_NAME}.a $LDFLAGS -o $OUTPUT_NAME"

# Show compilation info
print_info "Compiling: $SOURCE_FILE"
print_info "Output:    $OUTPUT_NAME"

if [ $VERBOSE_MODE -eq 1 ]; then
    echo -e "\n${YELLOW}Compilation command:${NC}"
    echo "$COMPILE_CMD"
    echo ""
fi

# Compile
if $COMPILE_CMD; then
    print_success "Compilation successful!"
    print_info "Run with: ./$OUTPUT_NAME"
else
    print_error "Compilation failed"
    exit 1
fi
