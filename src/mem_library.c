#include "mem_library.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "sfmm.h"

/*
 * Calculate the size of the block to be allocated.
 */
size_t calc_malloc_block_size(size_t size) {
  // add size, headersize, footersize, next/prev pointers (free), padding to
  // make it a multiple of 8
  size_t blocksize = size;
  // header size 4 block space, 1 in quicklist, 1 prev alloc, 1 alloc
  blocksize += 8;
  // don't allocate space for footer because we can use the payload area instead
  if (size < MIN_PAYLOAD_SIZE) {
    blocksize += MIN_PAYLOAD_SIZE - size;
  }
  if (blocksize % 8 != 0) {
    blocksize += 8 - (blocksize % 8);
  }
  // minimum block size is 32
  if (blocksize < 32) {
    blocksize = 32;
  }
  return blocksize;
}

/*
 * write the header for a block
 */
sf_block *write_block_header(sf_block *block, size_t size, int quicklist,
                             int prev_alloc, int alloc) {
  // write header
  // write the block size with 3 lsb's implicitly 0
  set_block_size(block, size);
  // set in quick list to 1
  if (quicklist != 0) {
    block->header |= 0x4;
  } else {
    block->header &= ~0x4;
  }

  // set the prev allocated bit to 1
  if (prev_alloc != 0) {
    block->header |= 0x2;
  } else {
    block->header &= ~0x2;
  }
  // set the allocated bit to 1
  if (alloc != 0) {
    block->header |= 0x1;
  } else {
    block->header &= ~0x1;
  }

  // if block is free write footer
  if (alloc == 0 && quicklist == 0) {
    write_block_footer(block);
  }
  // write padding
  // write payload
  return block;
}
/*
 * allocate block
 */
sf_block *alloc_block(sf_block *block, size_t size, int prev_alloc) {
  write_block_header(block, size, 0, prev_alloc, 1);
  set_prev_alloc_bit(get_block_end(block), 1);
  // remove footer
  remove_footer(block);
  return block;
}
/*
 * Set block size
 */
int set_block_size(sf_block *block, size_t size) {
  // write header
  // write the block size with 3 lsb's implicitly 0
  block->header = size & ~0x7;
  return 0;
}
/*
 * Get block size
 */
size_t get_block_size(sf_block *block) {
  // read header
  // read the block size with 3 lsb's implicitly 0
  return block->header & ~0x7;
}
/*
 * Set quick list bit
 */
int set_quick_list_bit(sf_block *block, int bit) {
  // write header
  // set in quick list to 0 or 1
  if (bit == 0) {
    block->header &= ~0x4;
    if (get_alloc_bit(block) == 0)  // if not quicklist and not allocated
      write_block_footer(block);    // update block footer
  } else {
    block->header |= 0x4;
    remove_footer(block);  // remove footer (quicklist block has no footer)
  }
  // quicklist block has no footer
  return 0;
}
/*
 * Get quick list bit
 */
int get_quick_list_bit(sf_block *block) {
  // read header
  // read in quick list to 0 or 1
  return ((block->header & 0x4) == 0) ? 0 : 1;
}
/*
 * Get prev allocated bit
 */
int get_prev_alloc_bit(sf_block *block) {
  // read header
  // read the prev allocated bit to 0 or 1
  return ((block->header & 0x2) == 0) ? 0 : 1;
}
/*
 * Set prev allocated bit
 */
int set_prev_alloc_bit(sf_block *block, int bit) {
  // write header
  // set the prev allocated bit to 0 or 1
  if (bit == 0) {
    block->header &= ~0x2;
  } else {
    block->header |= 0x2;
  }
  // if not alloc or quicklist, update footer
  if (get_alloc_bit(block) == 0 && get_quick_list_bit(block) == 0) {
    write_block_footer(block);
  }
  return 0;
}
/*
 * Get allocated bit
 */
int get_alloc_bit(sf_block *block) {
  // read header
  // read the allocated bit to 0 or 1
  return ((block->header & 0x1) == 0) ? 0 : 1;
}
/*
 * Set allocated bit
 */
int set_alloc_bit(sf_block *block, int bit) {
  // write header
  // set the allocated bit to 0 or 1
  if (bit == 0) {
    block->header &= ~0x1;
    // write footer if not allocated
    if (get_quick_list_bit(block) == 0)  // if not quicklist and not allocated
      write_block_footer(block);
  } else {
    block->header |= 0x1;
    remove_footer(block);  // remove footer (allocated block has no footer)
  }
  return 0;
}

/*
 * write the header for a free block
 * next and prev are pointer references to the next and previous blocks
 */
sf_block *write_free_block(sf_block *block, size_t size, int quicklist,
                           int prev_alloc, int alloc, sf_block *next,
                           sf_block *prev) {
  // write header
  write_block_header(block, size, quicklist, prev_alloc, alloc);
  // set next and prev pointers
  block->body.links.next = next;
  block->body.links.prev = prev;
  // clone header to footer:
  write_block_footer(block);
  // write padding
  // write payload
  return block;
}
/*
 * Write next pointer
 */
int set_next_pntr(sf_block block, sf_block *next) {
  // write next pointer
  block.body.links.next = next;
  return 0;
}
/*
 * Write prev pointer
 */
int set_prev_pntr(sf_block block, sf_block *prev) {
  // write prev pointer
  block.body.links.prev = prev;
  return 0;
}

/*
 * Write block footer
 */
sf_footer *write_block_footer(sf_block *block) {
  // write footer
  // read size from header
  size_t size = get_block_size(block);
  // clone header to footer:
  // compute the address of the footer
  sf_footer *footer = (sf_footer *)((void *)block + size - sizeof(sf_footer));
  // debug("footer address: %p", footer);
  *footer = (sf_footer)block->header;
  return footer;
}
/*
 * Convert an allocated block type to a free block type.
 */
sf_block *convert_to_free(sf_block *block) {
  // write header
  write_free_block(block, get_block_size(block), 0, get_prev_alloc_bit(block),
                   0, NULL, NULL);
  return block;
}

/*
 * Split a free block into two blocks.
 * The first block is the size of the requested size.
 * Returns a pointer to the second block.
 * Returns NULL if the block cannot be split.
 */
sf_block *split_free_block(sf_block *block, size_t size) {
  // if block is not free, return null
  if (get_alloc_bit(block) == 1) {
    return NULL;
  }
  // sf_show_block(block);
  int prev_alloc = get_prev_alloc_bit(block);
  // get size
  int block_size = get_block_size(block) - size;
  if (block_size < MIN_BLOCK_SIZE) {
    return NULL;
  }
  if (size < MIN_BLOCK_SIZE) {
    return NULL;
  }
  // split block by writing header after new block size (smaller block)
  // since head of split block, set prev_alloc to whatever it was on current
  // block
  sf_block *block1 = write_free_block(block, size, 0, prev_alloc, 0, 0, 0);
  // write header for new block, which should have size block_size (bigger
  // block)
  sf_block *block2 =
      write_free_block(get_block_end(block1), block_size, 0, 0, 0, 0, 0);

  return block2;
}

/*
 * Split a block into two blocks.
 * The first block is the size of the requested size.
 * Returns a pointer to the second block.
 * Returns NULL if the block cannot be split.
 */
sf_block *split_block(sf_block *block, size_t size) {
  // sf_show_block(block);
  int prev_alloc = get_prev_alloc_bit(block);
  // get size
  int block_size = get_block_size(block) - size;
  if (block_size < MIN_BLOCK_SIZE) {
    return NULL;
  }
  if (size < MIN_BLOCK_SIZE) {
    return NULL;
  }
  int alloc = get_alloc_bit(block);
  int quicklist = get_quick_list_bit(block);
  // split block by writing header after new block size (smaller block)
  // since head of split block, set prev_alloc to whatever it was on current
  // block
  sf_block *block1 =
      write_block_header(block, size, quicklist, prev_alloc, alloc);
  // write header for new block, which should have size block_size (bigger
  // block)
  sf_block *block2 =
      write_free_block(get_block_end(block1), block_size, 0, alloc, 0, 0, 0);
  set_prev_alloc_bit(get_block_end(block2), 0);
  return block2;
}

/*
 * Return pointer to end of a block
 * Also the start of the next block.
 */
sf_block *get_block_end(sf_block *block) {
  // get size
  size_t size = get_block_size(block);
  // compute the address of the footer
  sf_block *end = (void *)block + size;
  return end;
}
/*
 * Remove footer
 */
int remove_footer(sf_block *block) {
  // get size
  size_t size = get_block_size(block);
  // compute the address of the footer
  sf_footer *footer = (sf_footer *)((void *)block + size - sizeof(sf_footer));
  *footer = 0x0;
  return 0;
}

/*
 * Initialize free lists with dummy blocks.
 * Returns 0 if successful.
 * Returns -1 if unsuccessful.
 */
int init_free_lists() {
  // initialize free lists
  // every free list has a dummy block as the head of the respective doubly
  // linked list
  for (int i = 0; i < NUM_FREE_LISTS; i++) {
    sf_free_list_heads[i].body.links.next = &sf_free_list_heads[i];
    sf_free_list_heads[i].body.links.prev = &sf_free_list_heads[i];
  }
  return 0;
}
/*
 * Append a block to the free list.
 * Returns head of freelist if successful.
 * Returns NULL if unsuccessful.
 */
sf_block *append_free_list(sf_block *block) {
  // get block size:
  if (block == NULL) {
    return NULL;
  }
  if (get_alloc_bit(block) == 1) {
    return NULL;
  }
  if (get_block_size(block) < MIN_BLOCK_SIZE) {
    return NULL;
  }
  sf_block *potential_coalesce = coallesce(block);
  if (potential_coalesce != NULL) {
    block = potential_coalesce;
  }

  size_t size = get_block_size(block);
  // get the index of the free list
  sf_block *dummy_pointer = get_free_list_head(size);
  // get the first block in the free list
  sf_block *next = dummy_pointer->body.links.next;
  // append by size order (smallest to largest)
  while (next != dummy_pointer && get_block_size(next) < size) {
    // debug("next: %p", next);
    // debug("block: %p", block);
    // debug("dummy_pointer: %p", dummy_pointer);
    next = next->body.links.next;
  }

  // append the block to the free list
  block->body.links.next = next;
  block->body.links.prev = next->body.links.prev;
  next->body.links.prev->body.links.next = block;
  next->body.links.prev = block;
  return dummy_pointer;
}
/*
 * Remove a block from the free list.
 * Returns pointer to the block if successful.
 * Returns NULL if no block in the free list .
 */
sf_block *remove_free_list(size_t size) {
  // get the index of the free list
  sf_block *dummy_pointer = get_free_list_head(size);
  // get the first block in the free list
  if (dummy_pointer == NULL) {
    // free lists have not been initialized
    init_free_lists();
    return NULL;
  }

  sf_block *next = dummy_pointer->body.links.next;
  // iterate through the free list
  while (next != dummy_pointer) {
    if (get_block_size(next) == size) {
      // remove the block from the free list
      next->body.links.prev->body.links.next = next->body.links.next;
      next->body.links.next->body.links.prev = next->body.links.prev;
      return next;
    }
    next = next->body.links.next;
  }
  // debug("did not find exact fit");
  // there is no block which is an exact fit
  // find a block of at least size+MIN_BLOCK_SIZE
  int current_list = get_free_list_index(size + MIN_BLOCK_SIZE);
  int current_list_backup = current_list;
  while (current_list < NUM_FREE_LISTS) {
    dummy_pointer = &sf_free_list_heads[current_list];
    next = dummy_pointer->body.links.next;
    while (next != dummy_pointer) {
      // debug("blocksize, size: %d, %d", get_block_size(next),
      //       size + MIN_BLOCK_SIZE);
      if (get_block_size(next) >= size + MIN_BLOCK_SIZE) {
        // preserve next and prev pointers
        sf_block *this_prev = next->body.links.prev;
        sf_block *this_next = next->body.links.next;
        // remove the block from the free list
        this_prev->body.links.next = this_next;
        this_next->body.links.prev = this_prev;
        // split the block
        sf_block *block1 = split_free_block(next, size);
        // add the unwanted block to the free list
        append_free_list(block1);
        // debug("found suitable fit in list %d", current_list);
        return next;
      }
      next = next->body.links.next;
    }
    // debug("did not find suitable fit in list %d", current_list);
    current_list++;
    // move to next free list
  }
  debug("last ditch effort");
  while (current_list_backup < NUM_FREE_LISTS) {
    dummy_pointer = &sf_free_list_heads[current_list_backup];
    next = dummy_pointer->body.links.next;
    while (next != dummy_pointer) {
      // last ditch option, find a block of at least size and do not split
      if (get_block_size(next) >= size) {
        // preserve next and prev pointers
        sf_block *this_prev = next->body.links.prev;
        sf_block *this_next = next->body.links.next;
        // remove the block from the free list
        this_prev->body.links.next = this_next;
        this_next->body.links.prev = this_prev;
        return next;
      }
      next = next->body.links.next;
    }
    current_list_backup++;
  }

  return NULL;
}

/*
 * Get the head of the correct free list.
 */
sf_block *get_free_list_head(size_t size) {
  // get the index of the free list
  int power_of_2 = 1;  // i^th power of 2
  // debug("array: %p", &sf_free_list_heads[6] );
  for (int i = 0; i < NUM_FREE_LISTS; i++) {
    if (size <= (MIN_BLOCK_SIZE * power_of_2)) {
      // debug("returned list: %d", i);
      return &sf_free_list_heads[i];
    }
    power_of_2 *= 2;
  }
  // if size is larger than the largest free list, return the last free list
  return &sf_free_list_heads[NUM_FREE_LISTS - 1];
}

/*
 * Get the index of the correct free list.
 */
int get_free_list_index(size_t size) {
  // get the index of the free list
  int power_of_2 = 1;  // i^th power of 2
  // debug("array: %p", &sf_free_list_heads[6] );
  for (int i = 0; i < NUM_FREE_LISTS; i++) {
    if (size <= (MIN_BLOCK_SIZE * power_of_2)) {
      // debug("returned list: %d", i);
      return i;
    }
    power_of_2 *= 2;
  }
  // if size is larger than the largest free list, return the last free list
  return NUM_FREE_LISTS - 1;
}

/*
 * Get the correct quicklist index
 */
int get_quick_list_head(size_t size) {
  // get the index of the quick list
  if (size > MIN_BLOCK_SIZE + (NUM_QUICK_LISTS - 1) * 8) {
    return -1;
  }
  size -= MIN_BLOCK_SIZE;
  size /= 8;
  return size;
}
/*
 * Convert block to quicklist block
 * TODO: remove footer
 */
sf_block *convert_to_quicklist_block(sf_block *block) {
  // get size
  size_t size = get_block_size(block);
  block->body.links.next = NULL;
  set_quick_list_bit(block, 1);
  set_alloc_bit(block, 1);
  set_prev_alloc_bit(get_block_end(block), 1);
  // remove footer
  sf_footer *footer = (sf_footer *)((void *)block + size - sizeof(sf_footer));
  *footer = (sf_footer)0x0;
  return block;
}

/*
 * Append to quicklist
 * Quicklist is a singly linked list using LIFO format
 * Returns 0 if successful.
 * Returns -1 if unsuccessful.
 */
int append_quicklist(sf_block *block) {
  // get the index of the quick list
  int quick_index = get_quick_list_head(get_block_size(block));
  if (quick_index == -1) {
    return -1;
  }
  if (sf_quick_lists[quick_index].length >= QUICK_LIST_MAX) {
    flush_quicklist(quick_index);
  }
  // append the block to the end of the quick list
  sf_block *next = sf_quick_lists[quick_index].first;
  set_quick_list_bit(block, 1);
  set_alloc_bit(block, 1);
  set_prev_alloc_bit(get_block_end(block), 1);
  if (next == NULL) {
    sf_quick_lists[quick_index].first = block;
    sf_quick_lists[quick_index].length++;
    return 0;
  }
  while (next != NULL) {
    sf_block *peek = next->body.links.next;
    if (peek == NULL) {
      next->body.links.next = block;
      sf_quick_lists[quick_index].length++;
      return 0;
    }
    next = next->body.links.next;
  }
  return 0;
}

/*
 * Remove from quicklist
 * Quicklist is a singly linked list using LIFO format
 */
sf_block *remove_quicklist(size_t size) {
  // get the index of the quick list
  int quick_index = get_quick_list_head(size);
  if (quick_index == -1) {
    return NULL;
  }
  // get the first block in the quick list
  sf_block *next = sf_quick_lists[quick_index].first;
  if (next == NULL) {
    return NULL;
  }
  if (sf_quick_lists[quick_index].length == 1) {
    sf_quick_lists[quick_index].first = NULL;
    sf_quick_lists[quick_index].length--;
    set_quick_list_bit(next, 0);
    set_alloc_bit(next, 0);
    set_prev_alloc_bit(get_block_end(next), 0);

    return next;
  }
  // iterate through the quick list
  while (next != NULL) {
    sf_block *peek = next->body.links.next->body.links.next;
    if (peek == NULL) {
      // remove the block from the quick list
      sf_block *temp = next->body.links.next;
      next->body.links.next = NULL;
      sf_quick_lists[quick_index].length--;
      set_quick_list_bit(temp, 0);
      set_alloc_bit(temp, 0);
      set_prev_alloc_bit(get_block_end(temp), 0);
      return temp;
    }
    next = next->body.links.next;
  }
  return NULL;
}
/*
 * Remove from specific quicklist
 * Quicklist is a singly linked list using LIFO format
 */
sf_block *remove_specific_quicklist(int quick_index) {
  // get the index of the quick list
  if (quick_index == -1) {
    return NULL;
  }
  // get the first block in the quick list
  sf_block *next = sf_quick_lists[quick_index].first;
  if (next == NULL) {
    return NULL;
  }
  if (sf_quick_lists[quick_index].length == 1) {
    sf_quick_lists[quick_index].first = NULL;
    sf_quick_lists[quick_index].length--;
    set_quick_list_bit(next, 0);
    set_alloc_bit(next, 0);
    set_prev_alloc_bit(get_block_end(next), 0);
    return next;
  }
  // iterate through the quick list
  while (next != NULL) {
    sf_block *peek = next->body.links.next->body.links.next;
    if (peek == NULL) {
      // remove the block from the quick list
      sf_block *temp = next->body.links.next;
      next->body.links.next = NULL;
      sf_quick_lists[quick_index].length--;
      set_quick_list_bit(temp, 0);
      set_alloc_bit(temp, 0);
      set_prev_alloc_bit(get_block_end(temp), 0);
      return temp;
    }
    next = next->body.links.next;
  }
  return NULL;
}
/*
 * Flush quicklist
 * Quicklist is a singly linked list using LIFO format
 * Returns 1 if successful.
 * Returns 0 if unsuccessful.
 */
int flush_quicklist(int quick_index) {
  // get the index of the quick list
  if (quick_index == -1) {
    return 0;
  }
  // get the first block in the quick list
  sf_block *next = sf_quick_lists[quick_index].first;
  if (next == NULL) {
    return 0;
  }

  if (sf_quick_lists[quick_index].length < QUICK_LIST_MAX) {
    return 0;
  }
  // iterate through the quick list
  for (int i = 0; i < QUICK_LIST_MAX; i++) {
    sf_block *quickblock = remove_specific_quicklist(quick_index);
    if (quickblock == NULL) {
      return 0;
    }
    // check if can coallesce
    quickblock = coallesce(quickblock);
    append_free_list(quickblock);
  }
  return 1;
}

/*
 * Get header from footer
 */
sf_block *get_header_from_footer(sf_footer *footer) {
  // read the block size with the 3 LSBs implicitly 0
  size_t size = *footer & ~0x7;
  // get the header
  sf_block *header = (sf_block *)((void *)footer - size + sizeof(sf_footer));
  return header;
}
/*
 * Remove exact block from free list
 */
sf_block *remove_exact_block_free_list(sf_block *block) {
  if (get_alloc_bit(block) == 1) {
    return NULL;
  }
  if (block->body.links.next == NULL) {
    return NULL;
  }
  if (block->body.links.prev == NULL) {
    return NULL;
  }
  // get the next and previous blocks
  sf_block *next = block->body.links.next;
  sf_block *prev = block->body.links.prev;
  // why would next and prev pointers not be set??
  if (next == NULL) {
    return NULL;
  }
  if (prev == NULL) {
    return NULL;
  }
  // anyway...

  // remove the block from the free list
  next->body.links.prev = prev;
  prev->body.links.next = next;

  // set the next and previous pointers to NULL
  block->body.links.next = NULL;
  block->body.links.prev = NULL;
  return block;
}

/*
 * Coallesce the block with the previous block if it is free
 */
sf_block *coallesce_prev(sf_block *block) {
  if (get_prev_alloc_bit(block) == 1) {
    return NULL;
  }
  if (get_alloc_bit(block) == 1) {
    return NULL;
  }
  // get the previous block
  sf_footer *prev_footer = (sf_footer *)((void *)block - sizeof(sf_footer));
  sf_block *prev = get_header_from_footer(prev_footer);
  // check if the previous block is free
  if (get_alloc_bit(prev) == 1) {
    set_prev_alloc_bit(block, 1);
    return NULL;
  }
  // remove the previous block from the free list
  if (remove_exact_block_free_list(prev) == NULL) {
    return NULL;
  }
  // coallesce the blocks
  size_t size = get_block_size(prev) + get_block_size(block);
  int prev_alloc = get_prev_alloc_bit(prev);
  // wipe block header
  remove_footer(block);
  block->header = 0x0;
  // write new block header
  write_free_block(prev, size, 0, prev_alloc, 0, 0, 0);
  // return the coallesced block
  return prev;
}

/*
 * Coallesce the block with the next block if it is free
 */
sf_block *coallesce_next(sf_block *block) {
  // check if this block is free
  if (get_alloc_bit(block) == 1) {  // if this block is allocated
    return NULL;
  }
  // get the next block
  sf_block *next = get_block_end(block);

  // remove the next block from the free list
  if (remove_exact_block_free_list(next) == NULL) {
    // debug("next block is not in the free list");
    return NULL;
  }
  // check if the next block has the correct prev_alloc bit
  if (get_prev_alloc_bit(next) == 1) {
    // debug("next block has incorrect prev_alloc bit");
    return NULL;
  }
  // check if the next block is free
  if (get_alloc_bit(next) == 1) {
    // debug("next block is allocated");
    return NULL;
  }

  // coallesce the blocks
  size_t size = get_block_size(block) + get_block_size(next);
  int prev_alloc = get_prev_alloc_bit(block);
  // wipe block header
  remove_footer(next);
  next->header = 0x0;
  // write new block header
  write_free_block(block, size, 0, prev_alloc, 0, 0, 0);
  // return the coallesced block
  return block;
}

/*
 * Is pointer invalid?
  *
  * These are the conditions that would make a pointer invalid:
  * - The pointer is NULL.
  * - The pointer is not 8-byte aligned.
  * - The block size is less than the minimum block size of 32.
  * -  The block size is not a multiple of 8
  * -  The header of the block is before the start of the first block of the
 heap, or the footer of the block is after the end of the last block in the
 heap.
  * -  The allocated bit in the header is 0.
  * -  The in quick list bit in the header is 1.
  * -  The prev_alloc field in the header is 0, indicating that the previous
    block is free, but the alloc field of the previous block header is not 0.
  * Return 1 if pointer is invalid, 0 otherwise
 */
int is_pointer_invalid(void *pp) {
  // check if pointer is NULL
  // if (pp == NULL) {
  //   // debug("Pointer is NULL");
  //   return 1;
  // }
  // get the block
  sf_block *block = get_sf_block(pp);
  // check if pointer is 8-byte aligned
  uintptr_t block_pointer_int = (uintptr_t)block;
  if (block_pointer_int % 8 != 0) {
    // debug("Pointer is not 8-byte aligned");
    return 1;
  }
  // get the block
  // check if block size is less than the minimum block size of 32
  if (get_block_size(block) < 32) {
    return 1;
  }
  // check if block size is not a multiple of 8
  if (get_block_size(block) % 8 != 0) {
    // debug("Block size is not a multiple of 8");
    return 1;
  }
  // check if header of the block is before the start of the first block of the
  // heap
  if ((void *)block < sf_mem_start()) {
    // debug(
    //     "Header of the block is before the start of the first block of the "
    //     "heap");
    return 1;
  }
  // check if footer of the block is after the end of the last block in the heap
  if ((void *)block + get_block_size(block) > sf_mem_end()) {
    // debug("Footer of the block is after the end of the last block in the
    // heap");
    return 1;
  }
  // check if allocated bit in the header is 0
  if (get_alloc_bit(block) == 0) {
    // debug("Allocated bit in the header is 0");
    return 1;
  }
  // check if in quick list bit in the header is 1
  if (get_quick_list_bit(block) == 1) {
    // debug("In quick list bit in the header is 1");
    return 1;
  }
  // check if prev_alloc field in the header is 0, indicating that the previous
  // block is free, but the alloc field of the previous block header is not 0
  if (get_prev_alloc_bit(block) == 0) {
    // get the previous block
    sf_footer *prev_footer = (sf_footer *)((void *)block - sizeof(sf_footer));
    sf_block *prev = get_header_from_footer(prev_footer);
    if (get_alloc_bit(prev) != 0) {
      // debug(
      //     "Prev_alloc field in the header is 0, indicating that the previous
      //     " "block is free, but the alloc field of the previous block header
      //     is " "not 0");
      return 1;
    }
  }
  return 0;
}
/*
 * total coallesce
 */
sf_block *coallesce(sf_block *block) {
  sf_block *coallesced_block = block;
  sf_block *potential = coallesce_prev(block);
  while (potential != NULL) {
    coallesced_block = potential;
    potential = coallesce_prev(coallesced_block);
  }
  potential = coallesce_next(coallesced_block);
  while (potential != NULL) {
    coallesced_block = potential;
    potential = coallesce_next(coallesced_block);
  }
  return coallesced_block;
}

/*
 * Exact block in freelist?
 */
int is_exact_block_in_freelist(sf_block *block) {
  // get the index of the free list
  sf_block *dummy_pointer = get_free_list_head(get_block_size(block));
  // get the first block in the free list
  sf_block *next = dummy_pointer->body.links.next;
  // iterate through the free list
  // debug("dummy_pointer: %p", dummy_pointer);
  // debug("next: %p", next);
  while (next != dummy_pointer) {
    if (next == block) {
      // remove the block from the free list
      return 1;
    }
  }
  return 0;
}

sf_block *get_prev_block(sf_block *block) {
  sf_footer *prev_footer = (sf_footer *)((void *)block - sizeof(sf_footer));
  sf_block *prev = get_header_from_footer(prev_footer);
  return prev;
}
sf_block *get_sf_block(void *pp) {
  sf_block *block = (void *)((char *)pp - sizeof(sf_header));
  return block;
}

sf_block *realloc_more_mem(void *pp, size_t rsize) {
  void *new_pp = sf_malloc(rsize);
  if (new_pp == NULL) {
    return NULL;
  }
  memcpy(new_pp, pp, get_block_size(get_sf_block(pp)) - sizeof(sf_header));
  sf_free(pp);
  return new_pp;
}

sf_block *realloc_less_mem(void *pp, size_t rsize) {
  sf_block *potential =
      split_block(get_sf_block(pp), calc_malloc_block_size(rsize));
  if (potential != NULL) {
    debug("potential: %p", potential);
    append_free_list(potential);
    return pp;
  }
  // int orig_size = get_block_size(get_sf_block(pp)) - 8;
  // erase all the bits between rsize and orig_size
  // memset((void*)((char*)pp+rsize), 0, orig_size - rsize);

  // block is size 64 (malloc 56)
  // rsize is 56 (malloc 48) rsize = 48
  // printf("%ld",rsize);

  return pp;
}
/*
 * Split block such that the first block is free and the second block is
 * allocated
 */
sf_block *memalign_split_block_1(sf_block *block, size_t size) {
  int prev_alloc = get_prev_alloc_bit(block);
  // get size
  int block_size = get_block_size(block) - size;

  if (block_size < MIN_BLOCK_SIZE) {
    return NULL;
  }
  if (size < MIN_BLOCK_SIZE) {
    return NULL;
  }
  // split block by writing header after new block size (smaller block)
  // since head of split block, set prev_alloc to whatever it was on current
  // block
  sf_block *block1 = write_free_block(block, size, 0, prev_alloc, 0, 0, 0);
  // write header for new block, which should have size block_size (bigger
  // block)
  sf_block *block2 = alloc_block(get_block_end(block1), block_size, 0);
  append_free_list(block1);
  return block2;
}
/*
 * Split block such that the second block is free and the first block is
 * allocated
 */
sf_block *memalign_split_block_2(sf_block *block, size_t size) {
  int prev_alloc = get_prev_alloc_bit(block);
  // get size
  int block_size = get_block_size(block) - size;
  // debug("block_size: %d", block_size);
  if (block_size < MIN_BLOCK_SIZE) {
    return NULL;
  }
  if (size < MIN_BLOCK_SIZE) {
    return NULL;
  }
  // split block by writing header after new block size (smaller block)
  // since head of split block, set prev_alloc to whatever it was on current
  // block
  sf_block *block1 = alloc_block(block, size, prev_alloc);
  // write header for new block, which should have size block_size (bigger
  // block)
  sf_block *block2 =
      write_free_block(get_block_end(block1), block_size, 0, 1, 0, 0, 0);
  set_prev_alloc_bit(get_block_end(block2), 0);
  append_free_list(block2);
  return block1;
}
/*
 * Memalign alloc new block
 */
void *memalign_malloc(void *pp, size_t alignment, size_t expected_size) {
  // if pp is aligned
  if (((uintptr_t)pp % (alignment)) == 0) {
    // split uneeded end of block
    sf_block *block = get_sf_block(pp);
    // debug("start of pp aligned, pp: %p", pp);
    memalign_split_block_2(block, expected_size);
    return pp;
  }
  sf_block *block = get_sf_block(pp);
  size_t split_size = MIN_BLOCK_SIZE;
  pp = (void *)((char *)pp + MIN_BLOCK_SIZE);
  // find the first address that is aligned with at least MIN_BLOCK_SIZE bytes
  // after start of block
  while (((uintptr_t)pp % (alignment)) != 0) {
    pp = (void *)((char *)pp + sizeof(sf_header));
    split_size += sizeof(sf_header);
  }
  // debug("pp should be aligned: %ld", ((uintptr_t)pp % (alignment)));

  // split beginning of block
  sf_block *potential = memalign_split_block_1(block, split_size);
  if (potential != NULL) {
    block = potential;
  }
  // split uneeded end of block
  memalign_split_block_2(block, expected_size);
  return pp;
}