/*
 * Copyright (c) 2024-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @addtogroup ringbuffer Ringbuffer
 * @{
 *
 * @file ufw/octet-ring.h
 * @brief Octet (as in `uint8_t`) ring buffer API
 *
 * This uses the generation macros from `ufw/ring-buffer.h` to implement
 * ringbuffers for `uint8_t`. This is only available on systems that are
 * octet-addressable.
 *
 * @}
 */

#ifndef INC_UFW_OCTET_RING_H_ca6dbe81
#define INC_UFW_OCTET_RING_H_ca6dbe81

#include <stdint.h>

#include <ufw/ring-buffer-iter.h>
#include <ufw/ring-buffer.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

RING_BUFFER_API(octet_ring, uint8_t)
RING_BUFFER_ITER_API(octet_ring, uint8_t)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* INC_UFW_OCTET_RING_H_ca6dbe81 */
