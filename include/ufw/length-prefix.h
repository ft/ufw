/*
 * Copyright (c) 2022 micro framework workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file length-prefix.h
 * @brief Length prefix framing implementation
 */

#ifndef INC_UFW_LENGTH_PREFIX_H
#define INC_UFW_LENGTH_PREFIX_H

#include <stddef.h>

#include <ufw/octet-buffer.h>
#include <ufw/endpoints.h>
#include <ufw/variable-length-integer.h>

typedef struct ufw_length_prefix_buffer {
    unsigned char prefix_[VARINT_64BIT_MAX_OCTETS];
    OctetBuffer prefix;
    OctetBuffer payload;
} LengthPrefixBuffer;

int lenp_memory_encode(LengthPrefixBuffer*, void*, size_t);
int lenp_buffer_encode(LengthPrefixBuffer*, OctetBuffer*);
int lenp_buffer_encode_n(LengthPrefixBuffer*, OctetBuffer*, size_t);

ssize_t lenp_memory_to_sink(Sink*, void*, size_t);
ssize_t lenp_buffer_to_sink(Sink*, OctetBuffer*);
ssize_t lenp_buffer_to_sink_n(Sink*, OctetBuffer*, size_t);

ssize_t lenp_memory_from_source(Source*, void*, size_t);
ssize_t lenp_buffer_from_source(Source*, OctetBuffer*);

#endif /* INC_UFW_LENGTH_PREFIX_H */
