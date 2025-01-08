/*
 * Copyright (c) 2023-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#ifndef INC_UFW_ALLOCATOR_H_3ff7ef48
#define INC_UFW_ALLOCATOR_H_3ff7ef48

/**
 * @addtogroup blockallocator Block Allocator Abstraction
 *
 * Constraint memory allocation for embedded systems
 *
 * In embedded systems, dynamic allocation is an issue, mostly because of the
 * nondeterministic nature of time it takes to do allocation, as well as issues
 * with memory fragmentation. However, some jobs do require the allocation of
 * buffers. There are allocators that solve most of the issues, by being less
 * general in operation. These are allocators that allow the allocation of a
 * single buffer of fixed size. They are not a standard allocator in the C
 * library, however. This is an abstraction of allocators of that type, so
 * portable code can be written, using such allocators.
 *
 * For testing purposes, this module implements an allocator that satisfies the
 * Allocator API based on the standard C library's `malloc()` allocator.
 *
 * @{
 *
 * @file allocator.h
 * @brief Block Allocator API
 *
 * @}
 */

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum Allocator {
    UFW_ALLOC_GENERIC,
    UFW_ALLOC_SLAB
} Allocator;

typedef int (*GenericAlloc)(void*, void**, size_t);
typedef int (*SlabAlloc)(void*, void**);
typedef void (*GenericFree)(void*, void*);

typedef struct BlockAllocator {
    Allocator type;
    size_t blocksize;
    void *driver;
    union {
        GenericAlloc generic;
        SlabAlloc slab;
    } alloc;
    GenericFree free;
} BlockAllocator;

int ufw_malloc(void*, void**, size_t);
void ufw_mfree(void*, void*);

int block_alloc(BlockAllocator*, void**);
void block_free(BlockAllocator*, void*);

#define MAKE_GENERIC_BLOCKALLOC(_driver, _alloc, _free, _blocksize) \
    { .type = UFW_ALLOC_GENERIC,                                    \
      .blocksize = (_blocksize),                                    \
      .driver = (_driver),                                          \
      .alloc.generic = (_alloc),                                    \
      .free = (_free) }

#define MAKE_SLAB_BLOCKALLOC(_driver, _alloc, _free, _blocksize) \
    { .type = UFW_ALLOC_SLAB,                                    \
      .blocksize = (_blocksize),                                 \
      .driver = (_driver),                                       \
      .alloc.slab = (_alloc),                                    \
      .free = (_free) }

#define MAKE_STDHEAP_BLOCKALLOC(_blocksize) \
    { .type = UFW_ALLOC_GENERIC,            \
      .blocksize = (_blocksize),            \
      .driver = NULL,                       \
      .alloc.generic = ufw_malloc,          \
      .free = ufw_mfree }

/*
 * This is for backward compatibility. Previously there only was the STDHEAD
 * variant. However, this is due to a typo "HEAD vs. HEAP". In case users
 * already started using this, we add both names.
 *
 * This will be removed in ufw 7.0.0
 */
#define MAKE_STDHEAD_BLOCKALLOC(_blocksize) MAKE_STDHEAP_BLOCKALLOC(_blocksize)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* INC_UFW_ALLOCATOR_H_3ff7ef48 */
