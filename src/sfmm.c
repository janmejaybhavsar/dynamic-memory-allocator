/**
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include "sfmm.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "mem_library.h"

void *sf_malloc(size_t size) {
  if (size == 0) return NULL;
  void *allowed_pntr = 0;
  if (sf_mem_start() == sf_mem_end()) {
    // first time malloc is called
    // initialize heap
    allowed_pntr = sf_mem_grow();  // prologue block
    // allocate block of minimum size at prologue so it cannot be used
    alloc_block(allowed_pntr, MIN_BLOCK_SIZE, 0);
    allowed_pntr = get_block_end(
        allowed_pntr);  // set new allowed pointer to after prolouge
    init_free_lists();  // initialize all the free lists
    // turn rest of newly grown memory into free block
    int blocksize = (sf_mem_end() - allowed_pntr) - sizeof(sf_header);
    sf_block *block = allowed_pntr;
    write_free_block(block, blocksize, 0, 1, 0, 0, 0);
    // add block to free list
    append_free_list(block);
    // write epilogue
    sf_block *epilogue_pntr = sf_mem_end() - sizeof(sf_header);
    write_block_header(epilogue_pntr, 0, 0, 0, 1);
  }
  // determine the size of the block
  // add size, headersize, footersize, next/prev pointers (free), padding to
  // make it a multiple of 8
  int blocksize = calc_malloc_block_size(size);
  // debug("blocksize: %d", blocksize);

  // check if there is a block in the quicklist that fits
  sf_block *block = remove_quicklist(blocksize);
  if (block != NULL) {
    // debug("found block in quicklist");
    // block found in quicklist
    // alloc block
    alloc_block(block, get_block_size(block), get_prev_alloc_bit(block));
    // return pointer to payload of block
    void *payload = (void *)((char *)block + sizeof(sf_header));
    return payload;
  }

  // check if there is a block in the free list that fits
  block = remove_free_list(blocksize);
  // debug("crashes here");

  if (block != NULL) {
    // block found in free list
    alloc_block(block, get_block_size(block), get_prev_alloc_bit(block));
    // add second block to free list
    void *payload = (void *)((char *)block + sizeof(sf_header));
    return payload;
  }

  // no block found in free list or quicklist
  // grow heap
  // get pointer for epilogue
  sf_block *epilogue_pntr = sf_mem_end() - sizeof(sf_header);
  int epilogue_prev_alloc = get_prev_alloc_bit(epilogue_pntr);
  sf_block *new_memblock = epilogue_pntr;
  void *old_end_of_memory = sf_mem_end();
  // grow heap
  allowed_pntr = sf_mem_grow();
  // check if allowed_pntr is NULL
  if (allowed_pntr == NULL) {
    sf_errno = ENOMEM;
    return NULL;
  }
  // get pointer for new epilogue
  sf_block *new_epilogue_pntr = sf_mem_end() - sizeof(sf_header);
  // write new epilogue
  write_block_header(new_epilogue_pntr, 0, 0, 0, 1);
  // get block size
  int new_blocksize =
      (sf_mem_end() - old_end_of_memory);  // don't subtract header size because
                                           // coalescing previous epilogue
  // change old epilogue to head of a free block
  write_free_block(epilogue_pntr, new_blocksize, 0, epilogue_prev_alloc, 0, 0,
                   0);
  // sf_show_block(epilogue_pntr);
  // don't forger to coalesce
  new_memblock = coallesce(epilogue_pntr);

  // append after potential coallasing
  append_free_list(new_memblock);
  // add block to free list
  // call sf_malloc again
  return sf_malloc(size);
}

void sf_free(void *pp) {
  if (pp == NULL) {
    abort();
  }
  // get block
  sf_block *block = (void *)((char *)pp - sizeof(sf_header));
  if (is_pointer_invalid(pp)) {
    abort();
  }
  convert_to_free(block);
  sf_block *next = get_block_end(block);
  // check if can add to quicklist
  if (append_quicklist(block) == 0) {
    set_prev_alloc_bit(next, get_alloc_bit(block));
    return;
  }
  set_prev_alloc_bit(next, get_alloc_bit(block));
  // check if can coalesce
  block = coallesce(block);
  // add to free list
  append_free_list(block);
  return;
}

void *sf_realloc(void *pp, size_t rsize) {
  // check if pointer is valid
  if (pp == NULL) {
    sf_errno = EINVAL;
    return NULL;
  }
  sf_block *block = get_sf_block(pp);
  // debug("block: %p", block);
  // sf_show_block(block);
  if (is_pointer_invalid(pp)) {
    sf_errno = EINVAL;
    return NULL;
  }
  if (rsize == 0) {
    sf_free(pp);
    return NULL;
  }
  if (calc_malloc_block_size(rsize) == get_block_size(block)) {
    return pp;
  }
  if (rsize > get_block_size(block)) {
    return realloc_more_mem(pp, rsize);
  }
  if (rsize < get_block_size(block)) {
    return realloc_less_mem(pp, rsize);
  }
  return NULL;
}

void *sf_memalign(size_t size, size_t align) {
  // check if size is less than minimum alignment of 8
  if (size == 0) {
    // debug("size is 0");
    return NULL;
  }
  if (align < 8) {
    // debug("align is less than 8");
    sf_errno = EINVAL;
    return NULL;
  }
  // if align is not a power of two return null
  if (align && !(align & (align - 1)) == 0) {
    // debug("align(%ld) is not a power of 2", align);
    sf_errno = EINVAL;
    return NULL;
  }
  /*
  attempt to allocate a block whose size is at
  least the requested size, plus the alignment size,
  plus the minimum block size, plus the size required for a
  block header and footer
  */
  size_t blocksize = size + sizeof(sf_footer) + align + MIN_BLOCK_SIZE;
  void *pp = sf_malloc(blocksize);  // malloc calls calc_malloc_block_size which
                                    // adds 8 bytes for the header
  if (sf_errno == ENOMEM) {
    return NULL;
  }
  if (pp == NULL) {
    return NULL;
  }
  blocksize = get_block_size(get_sf_block(pp));
  // malloc will automatically add 8 bytes for the header
  return memalign_malloc(pp, align, calc_malloc_block_size(size));
}
