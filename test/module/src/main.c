/*
 * Copyright (c) 2024 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#include <zephyr/kernel.h>
#include <stdlib.h>
#include <ufw/meta.h>

int
main(void)
{
    for (;;) {
        k_usleep(1000);
    }
    return EXIT_SUCCESS;
}
