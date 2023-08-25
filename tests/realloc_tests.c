#include <criterion/criterion.h>
#include <errno.h>
#include <signal.h>

#include "debug.h"
#include "sfmm.h"
#include "tests.h"
#define TEST_TIMEOUT 15

// original available heap space is 4056

/*
  * Test invalid pointers
  * invalid pointers are defined as:
  * These are the conditions that would make a pointer invalid:
  * - The pointer is NULL.
  * - The pointer is not 8-byte aligned.
  * - The block size is less than the minimum block size of 32.
  * - The block size is not a multiple of 8
  * - The header of the block is before the start of the first block of the
 heap, or the footer of the block is after the end of the last block in the
 heap.
  * -  The allocated bit in the header is 0.
  * -  The in quick list bit in the header is 1.
  * -  The prev_alloc field in the header is 0, indicating that the previous
    block is free, but the alloc field of the previous block header is not 0.
  * Return 1 if pointer is invalid, 0 otherwise
*/

Test(sfmm_realloc_suite, realloc_invalid_pntr_NULL, .timeout = TEST_TIMEOUT) {
  // free invalid pointer
  void* invalid_pntr = NULL;
  // should abort program if pntr invalid
  void* x = sf_realloc(invalid_pntr, 32);
  int expected_errno = EINVAL;
  cr_assert_null(x, "x is not NULL!");
  cr_assert_eq(expected_errno, sf_errno, "sf_errno is not EINVAL");
}

Test(sfmm_realloc_suite, invalid_pntr_error, .timeout = TEST_TIMEOUT) {
  // realloc invalid pointer
  // create an invalid pointer
  void* invalid_pntr = (void*)12;
  void* x = sf_realloc(invalid_pntr, 32);
  int expected_errno = EINVAL;
  cr_assert_null(x, "x is not NULL!");
  cr_assert_eq(expected_errno, sf_errno, "sf_errno is not EINVAL");
}

Test(sfmm_realloc_suite, realloc_invalid_pntr_not_8_aligned,
     .timeout = TEST_TIMEOUT) {
  // free invalid pointer
  void* invalid_pntr = sf_malloc(20);
  invalid_pntr += 13;
  // should abort program if pntr invalid
  void* x = sf_realloc(invalid_pntr, 32);
  int expected_errno = EINVAL;
  cr_assert_null(x, "x is not NULL!");
  cr_assert_eq(expected_errno, sf_errno, "sf_errno is not EINVAL");
}

Test(sfmm_realloc_suite, realloc_invalid_pntr_b4_heap_start,
     .timeout = TEST_TIMEOUT) {
  // free invalid pointer
  void* invalid_pntr = sf_mem_start() - 128;
  // should abort program if pntr invalid
  void* x = sf_realloc(invalid_pntr, 32);
  int expected_errno = EINVAL;
  cr_assert_null(x, "x is not NULL!");
  cr_assert_eq(expected_errno, sf_errno, "sf_errno is not EINVAL");
}

Test(sfmm_realloc_suite, realloc_invalid_pntr_after_heap_end,
     .timeout = TEST_TIMEOUT) {
  // free invalid pointer
  void* invalid_pntr = sf_mem_end() + 128;
  // should abort program if pntr invalid
  void* x = sf_realloc(invalid_pntr, 32);
  int expected_errno = EINVAL;
  cr_assert_null(x, "x is not NULL!");
  cr_assert_eq(expected_errno, sf_errno, "sf_errno is not EINVAL");
}

Test(sfmm_realloc_suite, realloc_invalid_pntr_not_alloc,
     .timeout = TEST_TIMEOUT) {
  // free invalid pointer
  void* invalid_pntr = sf_malloc(20);
  sf_free(invalid_pntr);
  // should abort program if pntr invalid
  void* x = sf_realloc(invalid_pntr, 32);
  int expected_errno = EINVAL;
  cr_assert_null(x, "x is not NULL!");
  cr_assert_eq(expected_errno, sf_errno, "sf_errno is not EINVAL");
}

Test(sfmm_realloc_suite, realloc_invalid_pntr_bad_alignment,
     .timeout = TEST_TIMEOUT) {
  // free invalid pointer
  void* invalid_pntr = sf_malloc(20);
  // change block size
  get_block(invalid_pntr)->header = 33 & ~0x7;
  void* x = sf_realloc(invalid_pntr, 32);
  int expected_errno = EINVAL;
  cr_assert_null(x, "x is not NULL!");
  cr_assert_eq(expected_errno, sf_errno, "sf_errno is not EINVAL");
}

Test(sfmm_realloc_suite, realloc_invalid_pntr_set_qcklst,
     .timeout = TEST_TIMEOUT) {
  // free invalid pointer
  void* invalid_pntr = sf_malloc(20);
  // change block size
  get_block(invalid_pntr)->header |= 0x4;
  void* x = sf_realloc(invalid_pntr, 32);
  int expected_errno = EINVAL;
  cr_assert_null(x, "x is not NULL!");
  cr_assert_eq(expected_errno, sf_errno, "sf_errno is not EINVAL");
}

Test(sfmm_realloc_suite, realloc_invalid_pntr_set_alloc_0,
     .timeout = TEST_TIMEOUT) {
  // free invalid pointer
  void* invalid_pntr = sf_malloc(20);
  // change block size
  get_block(invalid_pntr)->header &= ~0x1;
  void* x = sf_realloc(invalid_pntr, 32);
  int expected_errno = EINVAL;
  cr_assert_null(x, "x is not NULL!");
  cr_assert_eq(expected_errno, sf_errno, "sf_errno is not EINVAL");
}

Test(sfmm_realloc_suite, realloc_invalid_pntr_prev_alloc_1_but_not_past_block,
     .timeout = TEST_TIMEOUT) {
  // free invalid pointer
  void* valid = sf_malloc(20);
  void* invalid_pntr = sf_malloc(20);
  // change block size
  get_block(invalid_pntr)->header &= ~0x1;
  // write footer for valid block
  sf_footer* footer = (sf_footer*)((void*)valid + 32 - sizeof(sf_footer));
  *footer = (sf_footer)get_block(invalid_pntr)->header;
  void* x = sf_realloc(invalid_pntr, 32);
  int expected_errno = EINVAL;
  cr_assert_null(x, "x is not NULL!");
  cr_assert_eq(expected_errno, sf_errno, "sf_errno is not EINVAL");
}

// Working realloc tests

Test(sfmm_realloc_suite, realloc_same_size, .timeout = TEST_TIMEOUT) {
  sf_errno = 0;
  double* ptr = sf_malloc(sizeof(double));
  *ptr = 43567890.987654;
  assert_allocated_block(ptr, 32);
  double* ptr2 = sf_realloc(ptr, sizeof(double));
  cr_assert(*ptr2 == 43567890.987654, "ptr2 is not 43567890.987654");
  assert_allocated_block(ptr, 32);
  assert_pntr_equal(ptr, ptr2);
  cr_assert_eq(0, sf_errno, "sf_errno is not 0");
}

Test(sfmm_realloc_suite, realloc_greater_size, .timeout = TEST_TIMEOUT) {
  sf_errno = 0;
  double* ptr = sf_malloc(sizeof(double));
  *ptr = 43567890.987654;
  assert_allocated_block(ptr, 32);
  assert_free_block_count((4056 - 32), 1);
  double* ptr2 = sf_realloc(ptr, 56);
  cr_assert(*ptr2 == 43567890.987654, "ptr2 is not 43567890.987654");
  assert_allocated_block(ptr2, 64);
  assert_quicklist_block(ptr, 32);
  assert_free_block_count((4056 - (64 + 32)), 1);
  assert_pntr_not_equal(ptr, ptr2);
  cr_assert_eq(0, sf_errno, "sf_errno is not 0");
}

Test(sfmm_realloc_suite, realloc_next_page, .timeout = TEST_TIMEOUT) {
  sf_errno = 0;
  double* ptr = sf_malloc(sizeof(double));
  *ptr = 5667655.3;
  assert_allocated_block(ptr, 32);
  assert_free_block_count((4056 - 32), 1);
  double* ptr2 = sf_realloc(ptr, 4096);
  cr_assert(*ptr2 == 5667655.3, "ptr2 is not 5667655.3");
  assert_allocated_block(ptr2, 4104);
  assert_quicklist_block(ptr, 32);
  assert_free_block_count(((4056 + 4096) - (4104 + 32)), 1);
  assert_pntr_not_equal(ptr, ptr2);
  cr_assert_eq(0, sf_errno, "sf_errno is not 0");
}

Test(sfmm_realloc_suite, realloc_smaller_size_no_splinter,
     .timeout = TEST_TIMEOUT) {
  sf_errno = 0;
  double* ptr = sf_malloc(48);
  *ptr = 1024325.3;
  assert_allocated_block(ptr, 56);
  double* ptr2 = sf_realloc(ptr, sizeof(double));
  cr_assert(*ptr2 == 1024325.3, "ptr2 is not 1024325.3");
  assert_allocated_block(ptr2, 56);
  assert_free_block_count(4000, 1);
  assert_pntr_equal(ptr, ptr2);
  cr_assert_eq(0, sf_errno, "sf_errno is not 0");
}

Test(sfmm_realloc_suite, realloc_smaller_size_split_block,
     .timeout = TEST_TIMEOUT) {
  sf_errno = 0;
  double* ptr = sf_malloc(56);
  *ptr = 1.0;
  assert_allocated_block(ptr, 64);
  double* ptr2 = sf_realloc(ptr, sizeof(double));
  cr_assert(*ptr2 == 1.0, "ptr2 is not 1.0");
  assert_allocated_block(ptr2, 32);
  assert_free_block_count((4056 - 32), 1);
  assert_pntr_equal(ptr, ptr2);
  cr_assert_eq(0, sf_errno, "sf_errno is not 0");
}