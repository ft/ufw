/*
 * Copyright (c) 2023-2024 micro framework workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#ifndef INC_UFW_ALLOCATOR_H_3ff7ef48
#define INC_UFW_ALLOCATOR_H_3ff7ef48

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

#define MAKE_STDHEAD_BLOCKALLOC(_blocksize) \
    { .type = UFW_ALLOC_GENERIC,            \
      .blocksize = (_blocksize),            \
      .driver = NULL,                       \
      .alloc.generic = ufw_malloc,          \
      .free = ufw_mfree }

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* INC_UFW_ALLOCATOR_H_3ff7ef48 */
