/*
 * Copyright (c) 2024 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#ifndef INC_INCLUDE_UFWZ_SLAB_ALLOCATOR_H_b734f213
#define INC_INCLUDE_UFWZ_SLAB_ALLOCATOR_H_b734f213

#include <ufw/allocator.h>

int ufwz_slab_alloc(void *driver, void **memory);
void ufwz_slab_free(void *driver, void *memory);

#define UFWZ_SLAB_BLOCKALLOC(SLAB, SIZE)                                \
    MAKE_SLAB_BLOCKALLOC(SLAB, ufwz_slab_alloc, ufwz_slab_free, SIZE);


#endif /* INC_INCLUDE_UFWZ_SLAB_ALLOCATOR_H_b734f213 */
