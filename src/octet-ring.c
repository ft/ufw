/*
 * Copyright (c) 2024-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @addtogroup ringbuffer Ringbuffer
 * @{
 *
 * @file octet-ring.c
 * @brief Octet (as in `uint8_t`) ring buffer implementation
 *
 * This uses the generation macros from `ring-buffer.h` to implement
 * ringbuffers for `uint8_t`. This is only available on systems that are
 * octet-addressable.
 *
 * @}
 */

#include <ufw/octet-ring.h>
#include <ufw/ring-buffer.h>
#include <ufw/ring-buffer-iter.h>

RING_BUFFER(octet_ring,      uint8_t)
RING_BUFFER_ITER(octet_ring, uint8_t)
