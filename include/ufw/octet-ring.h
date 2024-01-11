/*
 * Copyright (c) 2024 micro framework workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#ifndef INC_UFW_OCTET_RING_H_ca6dbe81
#define INC_UFW_OCTET_RING_H_ca6dbe81

#include <stdint.h>

#include <ufw/ring-buffer.h>
#include <ufw/ring-buffer-iter.h>

RING_BUFFER_API(octet_ring, uint8_t)
RING_BUFFER_ITER_API(octet_ring, uint8_t)

#endif /* INC_UFW_OCTET_RING_H_ca6dbe81 */
