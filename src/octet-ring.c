/*
 * Copyright (c) 2024-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#include <ufw/octet-ring.h>
#include <ufw/ring-buffer.h>
#include <ufw/ring-buffer-iter.h>

RING_BUFFER(octet_ring,      uint8_t)
RING_BUFFER_ITER(octet_ring, uint8_t)
