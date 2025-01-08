/*
 * Copyright (c) 2022-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#ifndef INC_UFW_RFC1055_H
#define INC_UFW_RFC1055_H

/**
 * @addtogroup protoslip Serial Line IP (SLIP)
 *
 * Implementation of the RFC1055 SLIP framing protocol
 *
 * See https://www.rfc-editor.org/rfc/rfc1055.txt for details.
 *
 * @{
 *
 * @file ufw/rfc1055.h
 * @brief RFC1055: Transmission of IP Datagrams over Serial Lines: SLIP
 *
 * @}
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <ufw/bit-operations.h>
#include <ufw/endpoints.h>

#define RFC1055_WORST_CASE(n,with_sof) (((n) * 2u) + ((with_sof) ? 2u : 1u))
#define RFC1055_WORST_CLASSIC(n) RFC1055_WORST_CASE(n, false)
#define RFC1055_WORST_WITHSOF(n) RFC1055_WORST_CASE(n, true)

#define RFC1055_WITH_SOF BITL(0)
#define RFC1055_DEFAULT  0u

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct rfc1055_context {
    enum {
        RFC1055_SEARCH_FOR_START,
        RFC1055_SEARCH_FOR_END,
        RFC1055_NORMAL
    } state;
    uint32_t flags;
} RFC1055Context;

#define RFC1055_CONTEXT_INIT_DEFAULT           \
    { .state = RFC1055_NORMAL,                 \
      .flags = RFC1055_DEFAULT  }
#define RFC1055_CONTEXT_INIT_WITH_SOF          \
    { .state = RFC1055_SEARCH_FOR_START,       \
      .flags = RFC1055_WITH_SOF }

void rfc1055_context_init(RFC1055Context*, uint32_t);
int rfc1055_encode(const RFC1055Context*, Source*, Sink*);
int rfc1055_decode(RFC1055Context*, Source*, Sink*);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* INC_UFW_RFC1055_H */
