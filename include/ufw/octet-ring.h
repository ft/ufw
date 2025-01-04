/*
 * Copyright (c) 2024-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#ifndef INC_UFW_OCTET_RING_H_ca6dbe81
#define INC_UFW_OCTET_RING_H_ca6dbe81

#include <stdint.h>

#include <ufw/ring-buffer.h>
#include <ufw/ring-buffer-iter.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

RING_BUFFER_API(octet_ring, uint8_t)
RING_BUFFER_ITER_API(octet_ring, uint8_t)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* INC_UFW_OCTET_RING_H_ca6dbe81 */
