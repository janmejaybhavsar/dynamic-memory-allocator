#include "sfmm.h"
#include <criterion/criterion.h>

extern void assert_quick_list_block_count(size_t size, int count);
extern void assert_free_block_count(size_t size, int count);
extern void assert_free_list_size(int index, int size);
extern void assert_block_size(void* pp, size_t size);
extern void assert_allocated_block(void* pp, size_t size);
extern void assert_free_block(void* pp, size_t size);
extern void assert_quicklist_block(void* pp, size_t size);
extern void assert_block_prev_alloc(void* pp, int alloc);
extern sf_block* get_block(void* pntr);
extern void assert_pntr_equal(void* p1, void* p2);
extern void assert_pntr_not_equal(void* p1, void* p2);
extern void assert_pntr_aligned(void* p, size_t alignment);