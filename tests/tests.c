#include "tests.h"

#include <criterion/criterion.h>
#include <stdio.h>

#include "sfmm.h"

// helper functions
sf_block* get_block(void* pntr) {
  return (sf_block*)((char*)pntr - sizeof(sf_header));
}
void* get_pp(void* block) { return (void*)((char*)block + sizeof(sf_header)); }

sf_block* get_freelist_index(size_t size) {
  // get the index of the free list
  int power_of_2 = 1;  // i^th power of 2
  // debug("array: %p", &sf_free_list_heads[6] );
  for (int i = 0; i < NUM_FREE_LISTS; i++) {
    if (size <= (32 * power_of_2)) {
      // debug("returned list: %d", i);
      return &sf_free_list_heads[i];
    }
    power_of_2 *= 2;
  }
  // if size is larger than the largest free list, return the last free list
  return &sf_free_list_heads[NUM_FREE_LISTS - 1];
}

int block_in_freelist(sf_block* block, size_t size) {
  sf_block* dummy_pointer = get_freelist_index(size);
  if (dummy_pointer == NULL) {
    return 0;
  }
  // get the first block in the free list
  sf_block* next = dummy_pointer->body.links.next;
  // iterate through the free list
  while (next != dummy_pointer) {
    if (next == block) {
      // block is in free list
      return 1;
    }
    next = next->body.links.next;
  }
  return 0;
}

int block_in_quicklist(sf_block* block, size_t size) {
  // get the index of the quick list
  if (size > 32 + (NUM_QUICK_LISTS - 1) * 8) {
    return 0;
  }
  size -= 32;
  size /= 8;
  int quick_index = size;
  // get the index of the quick list
  if (quick_index == -1) {
    return 0;
  }
  // get the first block in the quick list
  sf_block* next = sf_quick_lists[quick_index].first;
  if (next == NULL) {
    return 0;
  }
  if (sf_quick_lists[quick_index].length == 1) {
    if (sf_quick_lists[quick_index].first == block) {
      return 1;
    }
    return 0;
  }
  // iterate through the quick list
  while (next != NULL) {
    if (next == block) {
      // block is in the quick list
      return 1;
    }
    next = next->body.links.next;
  }
  return 0;
}

// assert functions
void assert_header_equals_footer(sf_block* block) {
  sf_footer* footer =
      (sf_footer*)((char*)block + (block->header & ~0x7) - sizeof(sf_footer));
  int footer_value = *footer;
  int header_value = block->header;
  cr_assert(header_value == footer_value, "Header and footer do not match");
}

void assert_block_size(void* pp, size_t size) {
  sf_block* bp = get_block(pp);
  cr_assert_eq((bp->header & ~0x7), size,
               "Block size is wrong (exp=%ld, found=%ld)", size,
               (bp->header & ~0x7));
}

void assert_block_alloc(void* pp, int alloc) {
  sf_block* bp = get_block(pp);
  cr_assert_eq((bp->header & 0x1), alloc,
               "Alloc bit is wrong (exp=%d, found=%d)", alloc,
               (bp->header & 0x1));
}

void assert_block_prev_alloc(void* pp, int prev_alloc) {
  sf_block* bp = get_block(pp);
  if (prev_alloc != 0) prev_alloc = 0x2;
  cr_assert_eq((bp->header & 0x2), prev_alloc,
               "Block prev_alloc is wrong (exp=%d, found=%d)", prev_alloc,
               (bp->header & 0x2));
}

void assert_block_quicklist(void* pp, int quicklist) {
  if (quicklist != 0) quicklist = 0x4;
  sf_block* bp = get_block(pp);
  cr_assert_eq((bp->header & 0x4), quicklist,
               "Block quicklist is wrong (exp=%d, found=%d)", quicklist,
               (bp->header & 0x4));
}

void assert_allocated_block(void* pp, size_t size) {
  assert_block_size(pp, size);
  assert_block_alloc(pp, 1);
  assert_block_quicklist(pp, 0);
  void* pp_next = get_pp((void*)get_block(pp) + size);
  assert_block_prev_alloc(pp_next, 1);
  cr_assert_eq(block_in_freelist(get_block(pp), size), 0,
               "Block is in the free list");
  cr_assert_eq(block_in_quicklist(get_block(pp), size), 0,
               "Block is in the quick list");
}

void assert_free_block(void* pp, size_t size) {
  assert_block_size(pp, size);
  assert_block_alloc(pp, 0);
  assert_block_quicklist(pp, 0);
  assert_header_equals_footer(get_block(pp));
  cr_assert_eq(block_in_freelist(get_block(pp), size), 1,
               "Block is not in the free list");
  cr_assert_eq(block_in_quicklist(get_block(pp), size), 0,
               "Block is in the quick list");
}

void assert_quicklist_block(void* pp, size_t size) {
  assert_block_size(pp, size);
  assert_block_alloc(pp, 1);
  assert_block_quicklist(pp, 1);
  void* pp_next = get_pp((void*)get_block(pp) + size);
  assert_block_prev_alloc(pp_next, 1);
  cr_assert_eq(block_in_freelist(get_block(pp), size), 0,
               "Block is in the free list");
  cr_assert_eq(block_in_quicklist(get_block(pp), size), 1,
               "Block is not in the quick list");
}

void assert_pntr_equal(void* p1, void* p2) {
  cr_assert_eq(p1, p2, "Pointers are not equal (p1=%p, p2=%p)", p1, p2);
}

void assert_pntr_not_equal(void* p1, void* p2) {
  cr_assert_neq(p1, p2, "Pointers are equal (p1=%p, p2=%p)", p1, p2);
}

void assert_pntr_aligned(void* p, size_t alignment) {
  cr_assert_eq((size_t)p % alignment, 0, "Pointer is not aligned (p=%p)", p);
}

// void assert_alignment_location(size_t alignment, size_t blocksize, size_t offset) {
//   // first possible pp_pointer
//   size_t start = (size_t)sf_mem_start() + 32 + 8;
//   int true_offset = start % alignment;
//   if (true_offset == offset) {
//     // memalign part
//     void* pp = sf_memalign(alignment, blocksize);
//     assert_pntr_aligned(pp, alignment);
//     // assert_allocated_block(pp, blocksize+8);
//     // memalignn tests
//     int memalignsize = 32 + alignment + blocksize + sizeof(sf_header) + sizeof(sf_footer);
//     if (offset == 0) {
//       assert_free_list_size(0,1);
//       if (blocksize <= 24)
//         assert_block_size(pp, 32);
//       else {
//         // pad block size so 8 byte alignment
//         int pad = blocksize % 8;
//         assert_block_size(pp, blocksize + pad + 8);
//       }
//     }
//     return;
//   }
//   int mallocsize = true_offset + offset + alignment - 8;
//   void* pp = sf_malloc(mallocsize);

// }