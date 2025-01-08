/*
 * Copyright (c) 2023-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @addtogroup blockallocator Block Allocator Abstraction
 * @{
 *
 * @file allocator.c
 * @brief Common block-allocator implementations
 *
 * @}
 */

#include <stddef.h>
#include <stdlib.h>

#include <ufw/compat/errno.h>

#include <ufw/allocator.h>
#include <ufw/compiler.h>

int
ufw_malloc(UNUSED void *driver, void **m, size_t n)
{
    *m = malloc(n);
    return (*m == NULL) ? -ENOMEM : 0;
}

void
ufw_mfree(UNUSED void *driver, void *m)
{
    free(m);
}

int
block_alloc(BlockAllocator *ba, void **m)
{
    if (ba->type == UFW_ALLOC_GENERIC) {
        return ba->alloc.generic(ba->driver, m, ba->blocksize);
    } else {
        return ba->alloc.slab(ba->driver, m);
    }
}

void
block_free(BlockAllocator *ba, void *m)
{
    ba->free(ba->driver, m);
}
