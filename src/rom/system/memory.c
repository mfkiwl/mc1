// -*- mode: c; tab-width: 2; indent-tabs-mode: nil; -*-
//--------------------------------------------------------------------------------------------------
// Copyright (c) 2019 Marcus Geelnard
//
// This software is provided 'as-is', without any express or implied warranty. In no event will the
// authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose, including commercial
// applications, and to alter it and redistribute it freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not claim that you wrote
//     the original software. If you use this software in a product, an acknowledgment in the
//     product documentation would be appreciated but is not required.
//
//  2. Altered source versions must be plainly marked as such, and must not be misrepresented as
//     being the original software.
//
//  3. This notice may not be removed or altered from any source distribution.
//--------------------------------------------------------------------------------------------------

#include <system/memory.h>

#include <system/mem_fill.h>
#include <system/vconsole.h>


//--------------------------------------------------------------------------------------------------
// Private
//--------------------------------------------------------------------------------------------------

// Uncomment this to enable memory allocator debugging.
// NOTE: Currently the logic does not work without this enabled (something is buggy).
#define ENABLE_DEBUG

#define MAX_NUM_POOLS 4         // Maximum number of supported memory pools.
#define MIN_MAX_NUM_ALLOCS 128  // Minimum size of a memory pool.
#define ALLOC_ALIGN 4           // Alignment in bytes (must be a power of two).

// A single allocation block.
typedef struct {
  size_t start;   // Absolute start address.
  size_t size;    // Block size (number of bytes).
} alloc_block_t;

// A memory pool.
typedef struct {
  size_t start;
  size_t size;
  int max_num_allocs;
  int num_allocs;
  unsigned type;
  alloc_block_t* blocks;
} mem_pool_t;

// The actual memory pool descriptors are allocated in BSS memory.
static mem_pool_t s_pools[MAX_NUM_POOLS];
static int s_num_pools;

static int calc_max_num_allocs(size_t size) {
  // We aim at using about 3% of the memory pool for the allocation array.
  int max_num_allocs = (int)(size / (32u * sizeof(alloc_block_t)));
  max_num_allocs = max_num_allocs < MIN_MAX_NUM_ALLOCS ? MIN_MAX_NUM_ALLOCS : max_num_allocs;

  // If we can't fit a reasonable allocation array in the RAM, fail!
  if ((2 * sizeof(alloc_block_t) * (size_t)max_num_allocs) > size) {
    return 0;
  }

  return max_num_allocs;
}

static int init_mem_pool(mem_pool_t* pool, void* start, size_t size, unsigned type) {
  int max_num_allocs = calc_max_num_allocs(size);
  if (max_num_allocs < 1) {
    return 0;
  }

  size_t blocks_array_size = sizeof(alloc_block_t) * (size_t)max_num_allocs;
  pool->max_num_allocs = max_num_allocs;
  pool->num_allocs = 0;
  pool->blocks = (alloc_block_t*)start;
  pool->start = ((size_t)start) + blocks_array_size;
  pool->size = size - blocks_array_size;
  pool->type = type;

  // Print memory pool information.
  vcon_print("Memory pool:\n  0x");
  vcon_print_hex(pool->start);
  vcon_print(", ");
  vcon_print_dec(pool->size);
  vcon_print(" bytes free\n  Type: ");
  vcon_print_hex(pool->type);
  vcon_print("\n");

  return 1;
}

static size_t ensure_aligned(size_t size) {
  // Round up to nearest ALLOC_ALIGN byte boundary.
  return (size + (ALLOC_ALIGN-1)) & ~(ALLOC_ALIGN-1);
}

static void* allocate_from(mem_pool_t* pool, size_t size) {
  // We can't do empty allocations nor allocate more blocks than max_num_allocs.
  if (size == 0 || pool->num_allocs >= pool->max_num_allocs) {
#ifdef ENABLE_DEBUG
    vcon_print("allocate_from: No more blocks\n");
#endif // ENABLE_DEBUG
    return NULL;
  }

  // Make sure that every allocated block is aligned.
  size = ensure_aligned(size);

  // Do a linear search for the best allocation slot. That is, find the smallest slot that is
  // large enough to hold the requested size.
  // TODO(m): With an additional array of blocks, sorted by the allocation size, we could do a
  // binary search instead of a linear search.
  int best_idx = -1;
  size_t best_block_start = 0;
  size_t best_free_bytes = ~0u;
  for (int i = 0; i <= pool->num_allocs; ++i) {
    size_t prev_block_end = (i > 0) ? pool->blocks[i - 1].start + pool->blocks[i - 1].size :
                                      pool->start;
    size_t block_start = (i < pool->num_allocs) ? pool->blocks[i].start :
                                                  (pool->start + pool->size);
    size_t free_bytes = block_start - prev_block_end;
    if (size <= free_bytes && free_bytes < best_free_bytes) {
#ifdef ENABLE_DEBUG
      vcon_print("allocate_from: Found idx: ");
      vcon_print_dec(i);
      vcon_print("\n");
#endif // ENABLE_DEBUG
      best_idx = i;
      best_block_start = prev_block_end;
      best_free_bytes = free_bytes;
    }
  }

  // Could we not find a slot?
  if (best_idx < 0) {
#ifdef ENABLE_DEBUG
    vcon_print("allocate_from: No block found\n");
#endif // ENABLE_DEBUG
    return NULL;
  }

  // Move all following blocks to the right.
  for (int i = pool->num_allocs; i > best_idx; --i) {
    pool->blocks[i] = pool->blocks[i - 1];
  }
  ++pool->num_allocs;

  // Insert our new allocation block.
  alloc_block_t* block = &pool->blocks[best_idx];
  block->start = best_block_start;
  block->size = size;
  return (void*)block->start;
}

static int free_from(mem_pool_t* pool, void* ptr) {
  // TODO(m): Do a binary search instead of a linear search.
  for (int i = 0; i < pool->num_allocs; ++i) {
    if (pool->blocks[i].start == (size_t)ptr) {
      // Move all following blocks to the left.
      --pool->num_allocs;
      for (int j = i; j < pool->num_allocs; ++j) {
        pool->blocks[j] = pool->blocks[j + 1];
      }
      return 1;
    }
  }
  return 0;
}


//--------------------------------------------------------------------------------------------------
// Public
//--------------------------------------------------------------------------------------------------

void mem_init() {
  s_num_pools = 0;
}

void mem_add_pool(void* start, size_t size, unsigned type) {
  if (s_num_pools < MAX_NUM_POOLS) {
    if (init_mem_pool(&s_pools[s_num_pools], start, size, type)) {
      ++s_num_pools;
    }
  }
}

void* mem_alloc(size_t num_bytes, unsigned types) {
  void* ptr = NULL;

  // Try all the available pools. The first added pool has priority.
  for (int i = 0; i < s_num_pools; ++i) {
    mem_pool_t* pool = &s_pools[i];
    if ((pool->type & types) != 0) {
      ptr = allocate_from(pool, num_bytes);
      if (ptr != NULL) {
        // Optionally zero-fill the newly allocated buffer.
        if ((types & MEM_CLEAR) != 0) {
          mem_fill(ptr, 0, num_bytes);
        }
        break;
      }
    }
  }

#ifdef ENABLE_DEBUG
  vcon_print("mem_alloc:\t0x");
  vcon_print_hex((size_t)ptr);
  vcon_print(", ");
  vcon_print_dec(num_bytes);
  vcon_print(" bytes\n");
#endif // ENABLE_DEBUG

  return ptr;
}

void mem_free(void* ptr) {
#ifdef ENABLE_DEBUG
  vcon_print("mem_free:\t0x");
  vcon_print_hex((size_t)ptr);
  vcon_print("\n");
#endif // ENABLE_DEBUG

  for (int i = 0; i < s_num_pools; ++i) {
    if (free_from(&s_pools[i], ptr)) {
      break;
    }
  }
}
