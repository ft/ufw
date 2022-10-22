/*
 * Copyright (c) 2022 chip-remote workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#ifndef INC_UFW_RFC1055_H
#define INC_UFW_RFC1055_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <ufw/bit-operations.h>
#include <ufw/endpoints.h>

#define RFC1055_WORST_CASE(n) (((n) * 2u) + 2u)

#define RFC1055_WITH_SOF BITL(0)
#define RFC1055_DEFAULT  0u

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

#endif /* INC_UFW_RFC1055_H */
