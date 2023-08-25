/*
 * MIN_PAYLOAD_SIZE
 * Each free block will have a footer, which occupies the last memory row of the
block.
 *
 * In an allocated block, the footer is not present, and the space that it would
otherwise occupy may be used for payload.
 *
 * Therefore, the attributes required by the block must be allocated to fit
inside the block
 *
 * If the payload is smaller than MIN_PAYLOAD_SIZE, then the remaining space
will be allocated as padding
 *
 ***** FREE BLOCK *****
 * - ~8 header~ seperate from payload / not included
 * - 8 next pointer
 * - 8 prev pointer
 * - 8 footer
 *************************
 */
#define MIN_PAYLOAD_SIZE 24
#define MIN_BLOCK_SIZE 32
#include "sfmm.h"

extern size_t calc_malloc_block_size(size_t size);
extern int append_quicklist(sf_block *block);
extern sf_block *write_block_header(sf_block *block, size_t size, int quicklist,
                                    int prev_alloc, int alloc);
extern sf_footer *write_block_footer(sf_block *block);
extern sf_block *write_free_block(sf_block *block, size_t size, int quicklist,
                                  int prev_alloc, int alloc, sf_block *next,
                                  sf_block *prev);
extern sf_block *convert_to_free(sf_block *block);
extern int init_free_lists();
extern sf_block *get_free_list_head(size_t size);
extern sf_block *split_block(sf_block *block, size_t size);
extern sf_block *get_block_end(sf_block *block);
extern sf_block *alloc_block(sf_block *block, size_t size, int prev_alloc);
extern sf_block *append_free_list(sf_block *block);
extern sf_block *remove_free_list(size_t size);
extern sf_block *remove_quicklist(size_t size);
extern sf_block *coallesce_prev(sf_block *block);
extern sf_block *coallesce_next(sf_block *block);
extern sf_block *coallesce(sf_block *block);
extern sf_block *get_prev_block(sf_block *block);
extern sf_block *realloc_more_mem(void *pp, size_t rsize);
extern sf_block *realloc_less_mem(void *pp, size_t rsize);
extern void *memalign_malloc(void *pp, size_t alignment, size_t size);

// helpers
extern int set_prev_alloc_bit(sf_block *block, int prev_alloc);
extern int set_alloc_bit(sf_block *block, int alloc);
extern int set_quick_list_bit(sf_block *block, int quicklist);
extern int set_block_size(sf_block *block, size_t size);
extern size_t get_block_size(sf_block *block);
extern int get_prev_alloc_bit(sf_block *block);
extern int get_alloc_bit(sf_block *block);
extern int get_quick_list_bit(sf_block *block);
extern int remove_footer(sf_block *block);
extern int get_free_list_index(size_t size);
extern int is_pointer_invalid(void *pp);
extern int flush_quicklist(int quick_index);
extern sf_block *remove_specific_quicklist(int quick_index);
extern int is_exact_block_in_freelist(sf_block *block);
extern sf_block *get_sf_block(void *pp);