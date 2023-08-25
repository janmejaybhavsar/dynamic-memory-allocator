![github repo badge: Language](https://img.shields.io/badge/Language-C-181717?color=blue) ![github repo badge: Testing](https://img.shields.io/badge/Testing-Criterion-181717?color=orange) ![github repo badge: OS](https://img.shields.io/badge/OS-Linux-181717?color=yellow) [![Dynamic Memory Allocation Tests](https://github.com/daminals/Dynamic-Memory-Allocator/actions/workflows/dynamicmemalloc.yml/badge.svg)](https://github.com/daminals/Dynamic-Memory-Allocator/actions/workflows/dynamicmemalloc.yml)
# Dynamic Memory Allocator

This is a dynamic memory allocator implemented in C, which allows users to allocate, reallocate, and free memory at runtime.

## Features

- Efficient allocation and deallocation of memory
- Ability to allocate memory blocks of different sizes
- Allocation alignment to ensure proper data structure alignment and improve performance
  - Support for any multiple of 8-byte alignment through memalign function

## Installation

The dependencies for this project are all included in the Makefile, and you can install and run it utilizing the following commands
```bash
git clone https://github.com/daminals/Dynamic-Memory-Allocator.git
cd Dynamic-Memory_allocator
make clean all
```
You will now be able to play with the different functions in this library
To run the test cases, you should first install the Criterion library, and then you can run 
```bash
bin/sfmm_tests
```


## Usage

To use the allocator, you can call the following functions:

- _void *sf_malloc(size_t size)_ - Allocates a block of memory of the given size and returns a pointer to the first byte of the block.
- _void *sf_realloc(void *ptr, size_t size)_ - Reallocates a block of memory pointed to by ptr to the given size, and returns a pointer to the first byte of the new block.
- _void sf_free(void *ptr)_ - Frees the memory block pointed to by ptr.
void set_debug_mode(bool mode) - Sets the allocator's debug mode on or off.
- _void *memalign(size_t size, size_t align)_ - Allocates a block of memory of the given size and alignment, and returns a pointer to the first byte of the block.

Here's an example of how to use the allocator to allocate memory:

```c
  double* ptr = sf_malloc(sizeof(double));
  double* ptr2 = sf_malloc(1016);
  double* ptr3 = sf_malloc(sizeof(double));
  ptr2 = sf_realloc(ptr2, 1024);
  sf_free(ptr);
  *ptr3 = 3.14;
  sf_free(ptr2);
  sf_free(ptr3);
```

## Error Handling

The allocator includes error handling to catch invalid memory accesses, out-of-bounds accesses, and other memory-related errors. If an error is detected, the allocator will set errno accordingly and return NULL.
