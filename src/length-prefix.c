/*
 * Copyright (c) 2022 micro framework workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file length-prefix.c
 * @brief Length prefix framing implementation
 *
 * This implements 64-bit variable-length-integer length prefix framing.
 *
 * For unreliable data channels, RFC1055 (SLIP) implements a robust,
 * automatically re-synchronising kind of framing. SLIP however has two large
 * downsides: A large worst case overhead, and the inability to make use of
 * techniques such as DMA.
 *
 * For reliable channels, such as TCP, length prefixing immediately fixes the
 * latter, because it does not transform the its payload. For naïve integer
 * encodings, length prefixing either poses a significant overhead, if mainly
 * used with tiny frames, or it significantly limits the maximum representable
 * frame size.
 *
 * Using variable-length integers solves this issue: Small frames only take an
 * overhead of a single octet. Using 64 bit integers allow for, for all intends
 * and purposes, infinitely large frames.
 *
 * Of course, there is a limit, but with unsigned 64 bit values, 16 exbi bytes
 * (~18.4e18 bytes) is large by any standard. Since the endpoint API uses
 * negative numbers to encode errors, ufw's implementation allows a maximum of
 * half that.
 *
 * The actual limits depends on the size of ‘size_t’ if that is smaller than
 * ‘uint64_t’. None of that should make any sort of a difference in embedded
 * systems.
 *
 * This implements two kinds of encoders: One that directly sends into a sink
 * and another, that keeps the encoded variant inside of a data type, in case
 * the result is required multiple times.
 */

#include <stddef.h>
#include <stdint.h>

#include <ufw/compat/errno.h>
#include <ufw/compat/ssize-t.h>
#include <ufw/octet-buffer.h>
#include <ufw/endpoints.h>
#include <ufw/length-prefix.h>
#include <ufw/variable-length-integer.h>

int
lenp_memory_encode(LengthPrefixBuffer *lpb, void *buf, size_t n)
{
    if (n > SSIZE_MAX) {
        return -EINVAL;
    }
#if SSIZE_MAX > UINT64_MAX
    if (n > UINT64_MAX) {
        return -EINVAL;
    }
#endif
    octet_buffer_space(&lpb->prefix, lpb->prefix_, VARINT_64BIT_MAX_OCTETS);
    /* The type definition ensures, this will not return an error. */
    (void)varint_encode_u64(&lpb->prefix, n);
    return octet_buffer_use(&lpb->payload, buf, n);
}

int
lenp_buffer_encode(LengthPrefixBuffer *lpb, OctetBuffer *b)
{
    return lenp_memory_encode(lpb, b->data, octet_buffer_avail(b));
}

int
lenp_buffer_encode_n(LengthPrefixBuffer *lpb, OctetBuffer *b, size_t n)
{
    const size_t rest = octet_buffer_avail(b);
    if (n > rest) {
        return -EINVAL;
    }
    const int rc = lenp_memory_encode(lpb, b->data, rest);
    b->offset += rest;
    return rc;
}

ssize_t
lenp_memory_to_sink(Sink *sink, void *buf, size_t n)
{
    if (n > SSIZE_MAX) {
        return -EINVAL;
    }
#if SSIZE_MAX > UINT64_MAX
    if (n > UINT64_MAX) {
        return -EINVAL;
    }
#endif
    const size_t numlen = varint_u64_length((uint64_t) n);
    if (n > (SSIZE_MAX - numlen)) {
        return -EINVAL;
    }
#if SSIZE_MAX > UINT64_MAX
    if (n > (UINT64_MAX - numlen)) {
        return -EINVAL;
    }
#endif
    const ssize_t rv = numlen + n;
    {
        const int rc = varint_u64_to_sink(sink, (uint64_t)n);
        if (rc < 0) {
            return (ssize_t)rc;
        }
    }

    {
        const ssize_t rc = sink_put_chunk(sink, buf, n);
        if (rc < 0) {
            return rc;
        } else if (rc == 0) {
            return 0;
        }
    }

    return rv;
}

ssize_t
lenp_buffer_to_sink(Sink *sink, OctetBuffer *b)
{
    return lenp_memory_to_sink(sink, b->data + b->offset, octet_buffer_avail(b));
}

ssize_t
lenp_buffer_to_sink_n(Sink *sink, OctetBuffer *b, size_t n)
{
    const size_t rest = octet_buffer_avail(b);
    if (n > rest) {
        return -EINVAL;
    }
    const int rc = lenp_memory_to_sink(sink, b->data + b->offset, rest);
    b->offset += rest;
    return rc;
}

ssize_t
lenp_memory_from_source(Source *source, void *mem, size_t size)
{
    uint64_t len = 0u;
    {
        const int rc = varint_u64_from_source(source, &len);
        if (rc < 0) {
            return (ssize_t)rc;
        }
    }
    if (len > size) {
        return -ENOMEM;
    }
    return source_get_chunk(source, mem, len);
}

ssize_t
lenp_buffer_from_source(Source *source, OctetBuffer *b)
{
    const ssize_t rc =
        lenp_memory_from_source(source, b->data + b->offset,
                                octet_buffer_avail(b));

    if (rc >= 0) {
        b->offset += rc;
    }

    return rc;
}