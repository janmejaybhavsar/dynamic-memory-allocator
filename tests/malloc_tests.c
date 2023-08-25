#include <criterion/criterion.h>
#include <errno.h>
#include <signal.h>

#include "debug.h"
#include "sfmm.h"
#include "tests.h"
#define TEST_TIMEOUT 15

Test(sfmm_malloc_suite, malloc_0_bytes, .timeout = TEST_TIMEOUT) {
  sf_errno = 0;
  void *x = sf_malloc(0);
  cr_assert_null(x, "x is not NULL!");
  cr_assert(sf_errno == 0, "sf_errno is not 0");
}

Test(sfmm_malloc_suite, malloc_too_many_bytes, .timeout = TEST_TIMEOUT) {
  sf_errno = 0;
  void *x = sf_malloc(4096 * 100);
  cr_assert_null(x, "x is not NULL!, x=%p", x);
  cr_assert(sf_errno == ENOMEM, "sf_errno is not ENOMEM, sf_errno=%d",
            sf_errno);
}

Test(sfmm_malloc_suite, malloc_4096_bytes, .timeout = TEST_TIMEOUT) {
  sf_errno = 0;
  void *x = sf_malloc(4096);
  int expected_size = 4104;
  cr_assert(sf_errno == 0, "sf_errno is not 0");
  assert_allocated_block(x, expected_size);
}

Test(sfmm_malloc_suite, malloc_avoid_potential_splinter,
     .timeout = TEST_TIMEOUT) {
  // available space will be 4056
  sf_errno = 0;
  void *x = sf_malloc(
      4040);  // can technically fit 4040, but we want to avoid splinter
  int expected_size = 4056;
  cr_assert(sf_errno == 0, "sf_errno is not 0");
  assert_allocated_block(x, expected_size);
  // (malloc should prefer to use bigger block, instead of double heapsize)
  cr_assert(sf_mem_end() - sf_mem_start() == PAGE_SZ,
            "heap size is not 4096");
  /*
  No "splinters" of smaller size than this are ever to be created.
  If splitting a block to be allocated would result in a splinter, then the block should
  not be split; rather, the block should be used as-is to satisfy the allocation request
  (i.e., you will "over-allocate" by issuing a block slightly larger than that required).
  */
}

Test(sfmm_malloc_suite, malloc_64_bytes, .timeout = TEST_TIMEOUT) {
  sf_errno = 0;
  char* y = sf_malloc(64);
  assert_allocated_block(y, 72);
  *y = 'a';
  cr_assert(*y == 'a', "y is not 'a'");
  assert_allocated_block(y, 72);
  cr_assert_eq(0, sf_errno, "sf_errno is not 0");
}

Test(sfmm_malloc_suite, malloc_from_quicklist_32, .timeout = TEST_TIMEOUT) {
  sf_errno = 0;
  sf_block *y = get_block(sf_malloc(23));  // create mem_grow block
  // change y to quicklist block without using sf_free
  y->header |= 0x4;  // set quicklist bit to 1
  y->header |= 0x1;  // set alloc bit to 1
  // clear payload
  y->body.links.next = NULL;
  y->body.links.prev = NULL;
  // add block to quicklist
  sf_quick_lists[0].first = y;
  sf_quick_lists[0].length++;
  // quicklist now has one block
  assert_quick_list_block_count(32, 1);
  // needs to malloc from quicklist
  sf_block *x = get_block(sf_malloc(23));
  cr_assert(x == y, "did not allocate block to quicklist");
  cr_assert(sf_quick_lists[0].length == 0, "quicklist length is not 0");
  cr_assert(sf_quick_lists[0].first == NULL, "quicklist first is not NULL");
  int expected_size = 32;
  cr_assert(sf_errno == 0, "sf_errno is not 0");
  cr_assert((x->header & ~0x7) == expected_size, "block size is not 4104");
  cr_assert((x->header & 0x1) == 1, "alloc bit is not 1");
  cr_assert((x->header & 0x2) == 2, "prev alloc bit is not 1");
  cr_assert((x->header & 0x4) == 0, "quicklist bit is not 0");
  sf_block *next = (void *)x + expected_size;
  cr_assert((next->header & 0x2) != 0, "prev alloc bit of next block is 0");
  cr_assert_eq(0, sf_errno, "sf_errno is not 0");
}

Test(sfmm_malloc_suite, malloc_from_quicklist_64, .timeout = TEST_TIMEOUT) {
  sf_errno = 0;
  void *y = sf_malloc(56);  
  // change y to quicklist block
  sf_free(y);
  // quicklist now has one block
  assert_quick_list_block_count(64, 1);
  assert_quicklist_block(y, 64);
  // needs to malloc from quicklist
  void *x = sf_malloc(56);
  cr_assert(x == y, "did not allocate block to quicklist");
  cr_assert(sf_quick_lists[1].length == 0, "quicklist length is not 0");
  cr_assert(sf_quick_lists[0].first == NULL, "quicklist first is not NULL");
  int expected_size = 64;
  assert_allocated_block(x, expected_size);
  cr_assert_eq(0, sf_errno, "sf_errno is not 0");
}

Test(sfmm_malloc_suite, malloc_quicklist_flush_test, .timeout = TEST_TIMEOUT) {
  sf_errno = 0;
  double *a = sf_malloc(sizeof(double));
  *a = 1.0;
  /*void *b = */ sf_malloc(sizeof(double));
  void *c = sf_malloc(sizeof(double));
  /*void *d =  */ sf_malloc(sizeof(double));
  long *e = sf_malloc(sizeof(double));
  *e = 4378642379;
  /*void *f =  */ sf_malloc(sizeof(double));
  void *g = sf_malloc(sizeof(double));
  /*void *h =  */ sf_malloc(sizeof(double));
  void *i = sf_malloc(sizeof(double));
  /*void *j =  */ sf_malloc(sizeof(double));
  double *k = sf_malloc(sizeof(double));
  *k = 32.0;
  /*void *l =  */ sf_malloc(sizeof(double));
  assert_free_block_count(0, 1);
  sf_free(a);
  assert_quick_list_block_count(32, 1);
  sf_free(c);
  assert_quick_list_block_count(32, 2);
  sf_free(e);
  assert_quick_list_block_count(32, 3);
  sf_free(g);
  assert_quick_list_block_count(32, 4);
  sf_free(i);
  assert_quick_list_block_count(32, 5);
  sf_free(k);
  assert_quick_list_block_count(32, 1);
  assert_free_block_count(0, 6);
  sf_malloc(sizeof(double));
  assert_quick_list_block_count(32, 0);
  assert_free_block_count(0, 6);
}