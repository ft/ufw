/*
 * Copyright (c) 2024 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#include <zephyr/kernel.h>

int
ufwz_slab_alloc(void *driver, void **memory)
{
    return k_mem_slab_alloc(driver, memory, K_NO_WAIT);
}

void
ufwz_slab_free(void *driver, void *memory)
{
    k_mem_slab_free(driver, memory);
}
