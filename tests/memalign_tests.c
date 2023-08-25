#include <criterion/criterion.h>
#include <errno.h>
#include <signal.h>

#include "debug.h"
#include "sfmm.h"
#include "tests.h"
#define TEST_TIMEOUT 15

Test(sfmm_memalign_suite, align_not_power_of_two, .timeout = TEST_TIMEOUT) {
  // if align is not a power of two or is less than the minimum block size,
  // then NULL is returned and sf_errno is set to EINVAL.
  void* pntr = sf_memalign(32, 3);
  int expected_errno = EINVAL;
  cr_assert_null(pntr, "pntr is not NULL!");
  cr_assert_eq(expected_errno, sf_errno, "sf_errno is not EINVAL");
}

Test(sfmm_memalign_suite, invalid_align_less_than_default) {
  sf_errno = 0;
  void* ptr = sf_memalign(16, 4);  // Align of 4 is less than default 8
  cr_assert_null(
      ptr,
      "sf_memalign should return NULL for invalid align less than default");
  cr_assert_eq(sf_errno, EINVAL,
               "sf_memalign should set sf_errno to EINVAL for invalid align "
               "less than default");
}

Test(sfmm_memalign_suite, size_is_0, .timeout = TEST_TIMEOUT) {
  // if size = 0, null is returned and sf_errno is not set.
  sf_errno = 0;
  void* pntr = sf_memalign(0, 32);
  cr_assert_eq(0, sf_errno, "sf_errno is not 0");
  cr_assert_null(pntr, "pntr is not NULL!");
}

Test(sfmm_memalign_suite, align_24, .timeout = TEST_TIMEOUT) {
  // if align is not a power of two or is less than the minimum block size,
  // then NULL is returned and sf_errno is set to EINVAL.
  sf_errno = 0;
  void* pntr = sf_memalign(32, 24);
  cr_assert_null(pntr, "pntr is not NULL!");
  int expected_errno = EINVAL;
  cr_assert_eq(expected_errno, sf_errno, "sf_errno is not 0, it is %d",
               sf_errno);
}

Test(sfmm_memalign_suite, size_4_align_16, .timeout = TEST_TIMEOUT) {
  // if align is not a power of two or is less than the minimum block size,
  // then NULL is returned and sf_errno is set to EINVAL.
  size_t alignment = 16;
  size_t requested_size = 4;
  void* testpp = sf_malloc(400);
  int caseA = 0;
  if ((size_t)testpp % alignment == 0) {
    caseA = 1;
  }
  sf_free(testpp);
  // ---------------------------------------------- //
  void* pntr = sf_memalign(4, alignment);
  int expected_errno = 0;
  cr_assert_eq(expected_errno, sf_errno, "sf_errno is not 0, it is %d",
               sf_errno);
  assert_pntr_aligned(pntr, alignment);
  // if there are two free blocks
  if (caseA) {  // if pp is already aligned
    assert_free_block_count(0, 1);
    assert_allocated_block(pntr, 32);
  } else {
    int free_block_size = 40;
    assert_free_block_count(free_block_size, 1);
    int available_size = 4056 - free_block_size;
    assert_allocated_block(pntr, 32);
    assert_free_block_count(0, 2);
    int allocated_size = 2*sizeof(sf_header) + (requested_size+4) + 32 + alignment;
    allocated_size -= free_block_size;
    assert_free_block_count(available_size-allocated_size, 1);
  }
}

Test(sfmm_memalign_suite, size_8_align_16, .timeout = TEST_TIMEOUT) {
  // if align is not a power of two or is less than the minimum block size,
  // then NULL is returned and sf_errno is set to EINVAL.
  size_t alignment = 16;
  size_t requested_size = 8;
  void* testpp = sf_malloc(400);
  int caseA = 0;
  if ((size_t)testpp % alignment == 0) {
    caseA = 1;
  }
  sf_free(testpp);
  // ---------------------------------------------- //
  void* pntr = sf_memalign(8, alignment);
  int expected_errno = 0;
  cr_assert_eq(expected_errno, sf_errno, "sf_errno is not 0, it is %d",
               sf_errno);
  assert_pntr_aligned(pntr, alignment);
  // if there are two free blocks
  if (caseA) {  // if pp is already aligned
    assert_free_block_count(0, 1);
    assert_allocated_block(pntr, 32);
  } else {
    int free_block_size = 40;
    assert_free_block_count(free_block_size, 1);
    int available_size = 4056 - free_block_size;
    assert_allocated_block(pntr, 32);
    assert_free_block_count(0, 2);
    int allocated_size = 2*sizeof(sf_header) + requested_size + 32 + alignment;
    allocated_size -= free_block_size;
    assert_free_block_count(available_size-allocated_size, 1);
  }
}

Test(sfmm_memalign_suite, size_16_align_16, .timeout = TEST_TIMEOUT) {
  // if align is not a power of two or is less than the minimum block size,
  // then NULL is returned and sf_errno is set to EINVAL.
  size_t alignment = 16;
  size_t requested_size = 16;
  void* testpp = sf_malloc(400);
  int caseA = 0;
  if ((size_t)testpp % alignment == 0) {
    caseA = 1;
  }
  sf_free(testpp);
  // ---------------------------------------------- //
  void* pntr = sf_memalign(requested_size, alignment);
  int expected_errno = 0;
  cr_assert_eq(expected_errno, sf_errno, "sf_errno is not 0, it is %d",
               sf_errno);
  assert_pntr_aligned(pntr, alignment);
  // if there are two free blocks
  if (caseA) {  // if pp is already aligned
    assert_free_block_count(0, 1);
    assert_allocated_block(pntr, 32);
  } else {
    int free_block_size = 40;
    assert_free_block_count(free_block_size, 1);
    int available_size = 4056 - free_block_size;
    assert_allocated_block(pntr, 40);
    assert_free_block_count(0, 2);
    int allocated_size = 2*sizeof(sf_header) + requested_size + 32 + alignment;
    allocated_size -= free_block_size;
    assert_free_block_count(available_size-allocated_size, 1);
  }
}

Test(sfmm_memalign_suite, size_24_align_16, .timeout = TEST_TIMEOUT) {
  // if align is not a power of two or is less than the minimum block size,
  // then NULL is returned and sf_errno is set to EINVAL.
  size_t alignment = 16;
  size_t requested_size = 24;
  void* testpp = sf_malloc(400);
  int caseA = 0;
  if ((size_t)testpp % alignment == 0) {
    caseA = 1;
  }
  sf_free(testpp);
  // ---------------------------------------------- //
  void* pntr = sf_memalign(requested_size, alignment);
  int expected_errno = 0;
  cr_assert_eq(expected_errno, sf_errno, "sf_errno is not 0, it is %d",
               sf_errno);
  assert_pntr_aligned(pntr, alignment);
  // if there are two free blocks
  if (caseA) {  // if pp is already aligned
    assert_free_block_count(0, 1);
    assert_allocated_block(pntr, 32);
  } else {
    int free_block_size = 40;
    assert_free_block_count(free_block_size, 1);
    int available_size = 4056 - free_block_size;
    assert_allocated_block(pntr, 48);
    assert_free_block_count(0, 2);
    int allocated_size = 2*sizeof(sf_header) + requested_size + 32 + alignment;
    allocated_size -= free_block_size;
    assert_free_block_count(available_size-allocated_size, 1);
  }
}

Test(sfmm_memalign_suite, malloc_memalign_size_4_align_16, .timeout = TEST_TIMEOUT) {
  // if align is not a power of two or is less than the minimum block size,
  // then NULL is returned and sf_errno is set to EINVAL.
  size_t alignment = 16;
  size_t requested_size = 4;
  void* testpp = sf_malloc(sizeof(double));
  // block size should be multiple of 16 (32)
  // to make testing computation easier
  void* alignmentpp = sf_malloc(400);
  int caseA = 0;
  if ((size_t)alignmentpp % alignment == 0) {
    caseA = 1;
  }
  sf_free(alignmentpp);
  void* pntr = sf_memalign(sizeof(double), alignment);
  int expected_errno = 0;
  cr_assert_eq(expected_errno, sf_errno, "sf_errno is not 0, it is %d",
               sf_errno);
  assert_pntr_aligned(pntr, alignment);
  // if there are two free blocks
  if (caseA) {  // if pp is already aligned
    assert_free_block_count(0, 1);
    assert_allocated_block(pntr, 32);
    sf_free(pntr);
    assert_quicklist_block(pntr, 32);
  } else {
    int free_block_size = 40;
    assert_free_block_count(free_block_size, 1);
    int available_size = 4056 - free_block_size;
    assert_allocated_block(pntr, 32);
    assert_free_block_count(0, 2);
    int allocated_size = 2*sizeof(sf_header) + (requested_size+4) + 32 + alignment;
    allocated_size -= free_block_size;
    available_size -= 32; // size of the block that was allocated
    assert_free_block_count(available_size-allocated_size, 1);
    sf_free(pntr);
    assert_quicklist_block(pntr, 32);
  }
  assert_allocated_block(testpp, 32);
}

// this alignment 32

Test(sfmm_memalign_suite, size_4_align_32, .timeout = TEST_TIMEOUT) {
  // if align is not a power of two or is less than the minimum block size,
  // then NULL is returned and sf_errno is set to EINVAL.
  size_t alignment = 16;
  size_t requested_size = 4;
  void* testpp = sf_malloc(400);
  int caseA = 0;
  if ((size_t)testpp % alignment == 0) {
    caseA = 1;
  }
  sf_free(testpp);
  // ---------------------------------------------- //
  void* pntr = sf_memalign(4, alignment);
  int expected_errno = 0;
  cr_assert_eq(expected_errno, sf_errno, "sf_errno is not 0, it is %d",
               sf_errno);
  assert_pntr_aligned(pntr, alignment);
  // if there are two free blocks
  if (caseA) {  // if pp is already aligned
    assert_free_block_count(0, 1);
    assert_allocated_block(pntr, 32);
  } else {
    int free_block_size = 40;
    assert_free_block_count(free_block_size, 1);
    int available_size = 4056 - free_block_size;
    assert_allocated_block(pntr, 32);
    assert_free_block_count(0, 2);
    int allocated_size = 2*sizeof(sf_header) + (requested_size+4) + 32 + alignment;
    allocated_size -= free_block_size;
    assert_free_block_count(available_size-allocated_size, 1);
  }
}

Test(sfmm_memalign_suite, size_8_align_32, .timeout = TEST_TIMEOUT) {
  // if align is not a power of two or is less than the minimum block size,
  // then NULL is returned and sf_errno is set to EINVAL.
  size_t alignment = 32;
  size_t requested_size = 8;
  void* testpp = sf_malloc(400);
  int caseA = 0;
  if ((size_t)testpp % alignment == 0) {
    caseA = 1;
  }
  sf_free(testpp);
  // ---------------------------------------------- //
  void* pntr = sf_memalign(8, alignment);
  int expected_errno = 0;
  cr_assert_eq(expected_errno, sf_errno, "sf_errno is not 0, it is %d",
               sf_errno);
  assert_pntr_aligned(pntr, alignment);
  // if there are two free blocks
  if (caseA) {  // if pp is already aligned
    assert_free_block_count(0, 1);
    assert_allocated_block(pntr, 32);
  } else {
    int free_block_size = 56;
    assert_free_block_count(free_block_size, 1);
    int available_size = 4056 - free_block_size;
    assert_allocated_block(pntr, 32);
    assert_free_block_count(0, 2);
    int allocated_size = 2*sizeof(sf_header) + (requested_size) + 32 + alignment;
    allocated_size -= free_block_size;
    assert_free_block_count(available_size-allocated_size, 1);
  }
}

Test(sfmm_memalign_suite, size_16_align_32, .timeout = TEST_TIMEOUT) {
  // if align is not a power of two or is less than the minimum block size,
  // then NULL is returned and sf_errno is set to EINVAL.
  size_t alignment = 32;
  size_t requested_size = 16;
  void* testpp = sf_malloc(400);
  int caseA = 0;
  if ((size_t)testpp % alignment == 0) {
    caseA = 1;
  }
  sf_free(testpp);
  // ---------------------------------------------- //
  void* pntr = sf_memalign(requested_size, alignment);
  int expected_errno = 0;
  cr_assert_eq(expected_errno, sf_errno, "sf_errno is not 0, it is %d",
               sf_errno);
  assert_pntr_aligned(pntr, alignment);
  // if there are two free blocks
  if (caseA) {  // if pp is already aligned
    assert_free_block_count(0, 1);
    assert_allocated_block(pntr, 32);
  } else {
    int free_block_size = 56;
    assert_free_block_count(free_block_size, 1);
    int available_size = 4056 - free_block_size;
    assert_allocated_block(pntr, 40);
    assert_free_block_count(0, 2);
    int allocated_size = 2*sizeof(sf_header) + requested_size + 32 + alignment;
    allocated_size -= free_block_size;
    assert_free_block_count(available_size-allocated_size, 1);
  }
}

Test(sfmm_memalign_suite, size_24_align_32, .timeout = TEST_TIMEOUT) {
  // if align is not a power of two or is less than the minimum block size,
  // then NULL is returned and sf_errno is set to EINVAL.
  size_t alignment = 32;
  size_t requested_size = 24;
  void* testpp = sf_malloc(400);
  int caseA = 0;
  if ((size_t)testpp % alignment == 0) {
    caseA = 1;
  }
  sf_free(testpp);
  // ---------------------------------------------- //
  void* pntr = sf_memalign(requested_size, alignment);
  int expected_errno = 0;
  cr_assert_eq(expected_errno, sf_errno, "sf_errno is not 0, it is %d",
               sf_errno);
  assert_pntr_aligned(pntr, alignment);
  // if there are two free blocks
  if (caseA) {  // if pp is already aligned
    assert_free_block_count(0, 1);
    assert_allocated_block(pntr, 32);
  } else {
    int free_block_size = 56;
    assert_free_block_count(free_block_size, 1);
    int available_size = 4056 - free_block_size;
    assert_allocated_block(pntr, 48);
    assert_free_block_count(0, 2);
    int allocated_size = 2*sizeof(sf_header) + requested_size + 32 + alignment;
    allocated_size -= free_block_size;
    assert_free_block_count(available_size-allocated_size, 1);
  }
}
