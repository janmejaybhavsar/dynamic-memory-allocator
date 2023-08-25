
#include <criterion/criterion.h>
#include <errno.h>
#include <signal.h>
#include "debug.h"
#include "sfmm.h"
#include "tests.h"
#include "mem_library.h"
#define TEST_TIMEOUT 15

Test(sfmm_helper_suite, calculate_malloc_block_size_1, .timeout = TEST_TIMEOUT) {
  // feed input smaller than minimum block size:
  int size = calc_malloc_block_size(5);
  cr_assert_eq(size, 32, "Expected size to be 32, but got %d", size);
}

Test(sfmm_helper_suite, calculate_malloc_block_size_2, .timeout = TEST_TIMEOUT) {
  // feed input not a multiple of 8:
  int size = calc_malloc_block_size(33);
  cr_assert_eq(size, 48, "Expected size to be 48, but got %d", size);
}

Test(sfmm_helper_suite, calculate_malloc_block_size_3, .timeout = TEST_TIMEOUT) {
  // feed stark example input:
  int size = calc_malloc_block_size(25);
  cr_assert_eq(size, 40, "Expected size to be 40, but got %d", size);
}
