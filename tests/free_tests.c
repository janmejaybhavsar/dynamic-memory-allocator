#include <criterion/criterion.h>
#include <errno.h>
#include <signal.h>

#include "debug.h"
#include "sfmm.h"
#include "tests.h"
#define TEST_TIMEOUT 15

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

Test(sfmm_free_suite, free_invalid_pntr_NULL, .timeout = TEST_TIMEOUT,
     .signal = SIGABRT) {
  // free invalid pointer
  void* invalid_pntr = NULL;
  // should abort program if pntr invalid
  sf_free(invalid_pntr);
}

Test(sfmm_free_suite, free_invalid_pntr_random_num, .timeout = TEST_TIMEOUT,
     .signal = SIGABRT) {
  // free invalid pointer
  void* invalid_pntr = (void*)12;
  // should abort program if pntr invalid
  sf_free(invalid_pntr);
}

Test(sfmm_free_suite, free_invalid_pntr_not_8_aligned, .timeout = TEST_TIMEOUT,
     .signal = SIGABRT) {
  // free invalid pointer
  void* invalid_pntr = sf_malloc(20);
  invalid_pntr += 13;
  // should abort program if pntr invalid
  sf_free(invalid_pntr);
}

Test(sfmm_free_suite, free_invalid_pntr_b4_heap_start, .timeout = TEST_TIMEOUT,
     .signal = SIGABRT) {
  // free invalid pointer
  void* invalid_pntr = sf_mem_start() - 128;
  // should abort program if pntr invalid
  sf_free(invalid_pntr);
}

Test(sfmm_free_suite, free_invalid_pntr_after_heap_end, .timeout = TEST_TIMEOUT,
     .signal = SIGABRT) {
  // free invalid pointer
  void* invalid_pntr = sf_mem_end() + 128;
  // should abort program if pntr invalid
  sf_free(invalid_pntr);
}

Test(sfmm_free_suite, free_invalid_pntr_not_alloc, .timeout = TEST_TIMEOUT,
     .signal = SIGABRT) {
  // free invalid pointer
  void* invalid_pntr = sf_malloc(20);
  sf_free(invalid_pntr);
  // should abort program if pntr invalid
  sf_free(invalid_pntr);
}

Test(sfmm_free_suite, free_invalid_pntr_bad_alignment, .timeout = TEST_TIMEOUT,
     .signal = SIGABRT) {
  // free invalid pointer
  void* invalid_pntr = sf_malloc(20);
  // change block size
  get_block(invalid_pntr)->header = 33 & ~0x7;
  ;
  sf_free(invalid_pntr);
}

Test(sfmm_free_suite, free_invalid_pntr_set_qcklst, .timeout = TEST_TIMEOUT,
     .signal = SIGABRT) {
  // free invalid pointer
  void* invalid_pntr = sf_malloc(20);
  // change block size
  get_block(invalid_pntr)->header |= 0x4;
  sf_free(invalid_pntr);
}

Test(sfmm_free_suite, free_invalid_pntr_set_alloc_0, .timeout = TEST_TIMEOUT,
     .signal = SIGABRT) {
  // free invalid pointer
  void* invalid_pntr = sf_malloc(20);
  // change block size
  get_block(invalid_pntr)->header &= ~0x1;
  sf_free(invalid_pntr);
}

Test(sfmm_free_suite, free_invalid_pntr_prev_alloc_1_but_not_past_block,
     .timeout = TEST_TIMEOUT, .signal = SIGABRT) {
  // free invalid pointer
  void* valid = sf_malloc(20);
  void* invalid_pntr = sf_malloc(20);
  // change block size
  get_block(invalid_pntr)->header &= ~0x1;
  // write footer for valid block
  sf_footer* footer = (sf_footer*)((void*)valid + 32 - sizeof(sf_footer));
  *footer = (sf_footer)get_block(invalid_pntr)->header;
  sf_free(invalid_pntr);
}

// Working free tests

Test(sfmm_free_suite, free_valid_pntr, .timeout = TEST_TIMEOUT) {
  // free valid pointer
  double* ptr = sf_malloc(sizeof(double));
  assert_block_size(ptr, 32);
  assert_free_block_count(4024, 1);
  // should not abort program if pntr valid
  sf_free(ptr);
  assert_quick_list_block_count(32, 1);
  assert_quick_list_block_count(0, 1);
  assert_free_block_count(0, 1);
  cr_assert_eq(0, sf_errno, "sf_errno is not 0");
}

Test(sfmm_free_suite, free_valid_pntr_2, .timeout = TEST_TIMEOUT) {
  // free valid pointer
  double* ptr = sf_malloc(sizeof(double));
  double* ptr2 = sf_malloc(sizeof(double));
  assert_block_size(ptr, 32);
  assert_block_size(ptr2, 32);
  assert_free_block_count(3992, 1);
  // should not abort program if pntr valid
  sf_free(ptr);
  assert_quick_list_block_count(32, 1);
  assert_quick_list_block_count(0, 1);
  assert_free_block_count(0, 1);
  assert_free_block_count(3992, 1);
  sf_free(ptr2);
  assert_quick_list_block_count(32, 2);
  assert_quick_list_block_count(0, 2);
  assert_free_block_count(0, 1);
  assert_free_block_count(3992, 1);
  cr_assert_eq(0, sf_errno, "sf_errno is not 0");
}

Test(sfmm_free_suite, free_valid_pntr_3, .timeout = TEST_TIMEOUT) {
  // free valid pointer
  double* ptr = sf_malloc(sizeof(double));
  assert_block_size(ptr, 32);
  double* ptr2 = sf_malloc(56);
  assert_block_size(ptr2, 64);
  double* ptr3 = sf_malloc(sizeof(double));
  assert_block_size(ptr3, 32);
  assert_free_block_count(3928, 1);
  // should not abort program if pntr valid
  sf_free(ptr);
  assert_quick_list_block_count(32, 1);
  assert_quick_list_block_count(0, 1);
  assert_free_block_count(0, 1);
  assert_free_block_count(3928, 1);
  sf_free(ptr2);
  assert_quick_list_block_count(32, 1);
  assert_quick_list_block_count(64, 1);
  assert_quick_list_block_count(0, 2);
  assert_free_block_count(0, 1);
  assert_free_block_count(3928, 1);
  sf_free(ptr3);
  assert_quick_list_block_count(32, 2);
  assert_quick_list_block_count(0, 3);
  assert_quick_list_block_count(64, 1);
  assert_free_block_count(0, 1);
  assert_free_block_count(3928, 1);
  cr_assert_eq(0, sf_errno, "sf_errno is not 0");
}

Test(sfmm_free_suite, free_valid_pntr_4, .timeout = TEST_TIMEOUT) {
  // free valid pointer
  double* ptr = sf_malloc(sizeof(double));
  double* ptr2 = sf_malloc(1016);
  double* ptr3 = sf_malloc(sizeof(double));
  assert_block_size(ptr, 32);
  assert_block_size(ptr2, 1024);
  assert_block_size(ptr3, 32);
  assert_free_block_count(2968, 1);
  // should not abort program if pntr valid
  sf_free(ptr);
  assert_quick_list_block_count(32, 1);
  assert_quick_list_block_count(0, 1);
  assert_free_block_count(0, 1);
  assert_free_block_count(2968, 1);
  sf_free(ptr2);
  assert_quick_list_block_count(32, 1);
  assert_quick_list_block_count(0, 1);
  assert_free_block_count(1024, 1);
  assert_free_block_count(0, 2);
  assert_free_block_count(1024, 1);
  assert_free_block_count(2968, 1);
  sf_free(ptr3);
  assert_quick_list_block_count(32, 2);
  assert_quick_list_block_count(0, 2);
  assert_free_block_count(0, 2);
  assert_free_block_count(1024, 1);
  assert_free_block_count(2968, 1);
  cr_assert_eq(0, sf_errno, "sf_errno is not 0");
}

Test(sfmm_free_suite, free_coalesce_quicklist, .timeout = TEST_TIMEOUT) {
  sf_errno = 0;
  double *a = sf_malloc(sizeof(double));
  *a = 1.0;
  void *b =  sf_malloc(sizeof(double));
  void *c = sf_malloc(sizeof(double));
  void *d =  sf_malloc(sizeof(double));
  long *e = sf_malloc(sizeof(double));
  *e = 4378642379;
  void *f =  sf_malloc(sizeof(double));
  // void *g = sf_malloc(sizeof(double));
  /*void *h =  */ sf_malloc(sizeof(double));
  /*void *i = */sf_malloc(sizeof(double));
  /*void *j =  */ sf_malloc(sizeof(double));
  assert_free_block_count(0, 1);
  sf_free(a);
  assert_quick_list_block_count(32, 1);
  sf_free(c);
  assert_quick_list_block_count(32, 2);
  sf_free(b);
  assert_quick_list_block_count(32, 3);
  sf_free(e);
  assert_quick_list_block_count(32, 4);
  sf_free(f);
  assert_quick_list_block_count(32, 5);
  sf_free(d);
  assert_quick_list_block_count(32, 1);
  assert_free_block_count(0, 3);
  assert_free_block_count(32*3, 1);
  assert_free_block_count(32*2, 1);
  sf_malloc(sizeof(double));
  assert_quick_list_block_count(32, 0);
  assert_free_block_count(0, 3);
}