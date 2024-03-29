/*
 * Copyright (c) 2022-2024 micro framework workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file length-prefix.c
 * @brief Length prefix framing implementation
 *
 * This module implements two length prefix encoding schemes: One based on
 * variable-length integers as specified in Google's protobuf serialisation
 * format. The other is a family of encodings based on different fixed word
 * width integer encodings.
 *
 * Rationale: For unreliable data channels, RFC1055 (SLIP) implements a robust,
 * automatically re-synchronising kind of framing. SLIP however has two large
 * downsides: A large worst case overhead, and the inability to make use of
 * techniques such as DMA. For a SLIP implementation see the rfc1055.h module.
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
 *
 * The previously described API uses variable length integers, combining low
 * overhead with small frames while allowing for arbitrarily large frames as
 * well. While this is a rather elegant solution, there are systems that use a
 * fixed number of octets to perform a similar job. These systems use different
 * kinds of octet orders and word sizes in their encoding. We can use the API
 * from binary-format.h to deal with that, but it leads to a whole family of
 * different possible implementations.
 *
 * This module also implements this family of encodings for 8, 16, and 32 bits,
 * with little and big endian octet orders. Other word widths are possible of
 * course, but these two seem most applicable to embedded systems. So we will
 * stick with these for now. The code is written in a style that makes is easy
 * to extend these encoding kinds later.
 */

#include <stddef.h>
#include <stdint.h>

#include <ufw/compat/errno.h>
#include <ufw/compat/ssize-t.h>

#include <ufw/binary-format.h>
#include <ufw/byte-buffer.h>
#include <ufw/endpoints.h>
#include <ufw/length-prefix.h>
#include <ufw/variable-length-integer.h>

typedef unsigned char (*octet_parse)(const void*);
typedef void* (*octet_generate)(void*, const unsigned char);

static unsigned char
ref_u8(const void *buf)
{
    const unsigned char *data = buf;
    return *data;
}

static void*
set_u8(void *buf, const unsigned char value)
{
    unsigned char *data = buf;
    *data = value;
    return data + 1u;
}

typedef uint16_t (*u16_parse)(const void*);
typedef uint32_t (*u32_parse)(const void*);
typedef void* (*u16_generate)(void*, const uint16_t);
typedef void* (*u32_generate)(void*, const uint32_t);

struct u8serdes {
    octet_parse parse;
    octet_generate generate;
};

struct u16serdes {
    u16_parse parse;
    u16_generate generate;
};

struct u32serdes {
    u32_parse parse;
    u32_generate generate;
};

struct flenp_serdes {
    unsigned int size;
    union {
        struct u8serdes u8;
        struct u16serdes u16;
        struct u32serdes u32;
    } cb;
    uint64_t maximum;
};

#define KIND(K,S,T,P,G,M)      \
    [LENP_ ## K] = {           \
        .size = S,             \
        .cb.T.parse = P,       \
        .cb.T.generate = G,    \
        .maximum = M }

static struct flenp_serdes kind[] = {
    KIND(VARIABLE, 0u,  u8, NULL,        NULL,        0u),
    KIND(OCTET,    1u,  u8, ref_u8,      set_u8,      255u),
    KIND(LE_16BIT, 2u, u16, bf_ref_u16l, bf_set_u16l, UINT16_MAX),
    KIND(LE_32BIT, 4u, u32, bf_ref_u32l, bf_set_u32l, UINT32_MAX),
    KIND(BE_16BIT, 2u, u16, bf_ref_u16b, bf_set_u16b, UINT16_MAX),
    KIND(BE_32BIT, 4u, u32, bf_ref_u32b, bf_set_u32b, UINT32_MAX)
};

static int
encode_prefix(const LengthPrefixKind k, ByteBuffer *b,
              unsigned char *mem, size_t n)
{
    if (n > SSIZE_MAX) {
        return -EINVAL;
    }
#if SSIZE_MAX > UINT64_MAX
    if (n > UINT64_MAX) {
        return -EINVAL;
    }
#endif
    if (k != LENP_VARIABLE && n > kind[k].maximum) {
        return -EINVAL;
    }
    if (k == LENP_VARIABLE) {
        byte_buffer_space(b, mem, VARINT_64BIT_MAX_OCTETS);
    } else {
        byte_buffer_use(b, mem, kind[k].size);
    }
    switch (kind[k].size) {
    case 1:  kind[k].cb.u8.generate( mem, n); break;
    case 2:  kind[k].cb.u16.generate(mem, n); break;
    case 4:  kind[k].cb.u32.generate(mem, n); break;
    default: varint_encode_u64(b, n);         break;
    }
    return 0;
}

int
flenp_memory_encode(const LengthPrefixKind k, LengthPrefixBuffer *lpb,
                    void *buf, size_t n)
{
    const int rc = encode_prefix(k, &lpb->prefix, lpb->prefix_, n);

    if (rc < 0) {
        return rc;
    }

    return byte_buffer_use(&lpb->payload, buf, n);
}

int
flenp_buffer_encode(const LengthPrefixKind k,
                    LengthPrefixBuffer *lpb, ByteBuffer *b)
{
    const size_t rest = byte_buffer_rest(b);
    return flenp_memory_encode(k, lpb, b->data + b->offset, rest);
}


int
flenp_buffer_encode_n(const LengthPrefixKind k,
                      LengthPrefixBuffer *lpb, ByteBuffer *b, size_t n)
{
    const size_t rest = byte_buffer_rest(b);
    if (n > rest) {
        return -EINVAL;
    }
    const int rc = flenp_memory_encode(k, lpb, b->data + b->offset, n);
    b->offset += rest;
    return rc;
}

int
flenp_chunks_use(const LengthPrefixKind k, LengthPrefixChunks *lpc)
{
    size_t size = 0u;
    for (size_t i = lpc->payload.active; i < lpc->payload.chunks; ++i) {
        size += byte_buffer_rest(lpc->payload.chunk + i);
    }

    const int rc = encode_prefix(k, &lpc->prefix, lpc->prefix_, size);

    if (rc < 0) {
        return rc;
    }

    return 0;
}

ssize_t
flenp_memory_to_sink(const LengthPrefixKind k, Sink *sink, void *buf, size_t n)
{
    LengthPrefixBuffer lpb;
    const int rc = encode_prefix(k, &lpb.prefix, lpb.prefix_, n);

    if (rc < 0) {
        return rc;
    }

    const size_t numlen = lpb.prefix.used;
    if (n > (SSIZE_MAX - numlen)) {
        return -EINVAL;
    }
#if SSIZE_MAX > UINT64_MAX
    if (n > (UINT64_MAX - numlen)) {
        return -EINVAL;
    }
#endif

    {
        const int rcsink = sink_put_chunk(sink, lpb.prefix_, numlen);
        if (rcsink < 0) {
            return (ssize_t)rcsink;
        }
    }

    {
        const ssize_t rcsink = sink_put_chunk(sink, buf, n);
        if (rcsink < 0) {
            return rcsink;
        } else if (rcsink == 0) {
            return 0;
        }
    }

    return (ssize_t)(numlen + n);
}

ssize_t
flenp_buffer_to_sink(const LengthPrefixKind k, Sink *sink, ByteBuffer *b)
{
    return flenp_memory_to_sink(k, sink, b->data + b->offset,
                                byte_buffer_avail(b));
}

ssize_t
flenp_buffer_to_sink_n(const LengthPrefixKind k,
                       Sink *sink, ByteBuffer *b, size_t n)
{
    const size_t rest = byte_buffer_avail(b);
    if (n > rest) {
        return -EINVAL;
    }
    const int rc = flenp_memory_to_sink(k, sink, b->data + b->offset, rest);
    b->offset += rest;
    return rc;
}

ssize_t
flenp_chunks_to_sink(const LengthPrefixKind k, Sink *sink, ByteChunks *oc)
{
    size_t size = 0u;
    for (size_t i = oc->active; i < oc->chunks; ++i) {
        size += byte_buffer_rest(oc->chunk + i);
    }

    LengthPrefixBuffer lpb;
    const int rc = encode_prefix(k, &lpb.prefix, lpb.prefix_, size);

    if (rc < 0) {
        return rc;
    }

    const size_t numlen = lpb.prefix.used;
    if (size > (SSIZE_MAX - numlen)) {
        return -EINVAL;
    }
#if SSIZE_MAX > UINT64_MAX
    if (n > (UINT64_MAX - numlen)) {
        return -EINVAL;
    }
#endif

    {
        const int rcsink = sink_put_chunk(sink, lpb.prefix_, numlen);
        if (rcsink < 0) {
            return (ssize_t)rcsink;
        }
    }

    for (size_t i = oc->active; i < oc->chunks; ++i) {
        const size_t n = byte_buffer_rest(oc->chunk + i);
        const ssize_t rcsink = sink_put_chunk(
            sink, oc->chunk[i].data + oc->chunk[i].offset, n);
        if (rcsink < 0) {
            return rcsink;
        } else if (rcsink == 0) {
            return 0;
        }
    }

    return (ssize_t)(numlen + size);
}

static ssize_t
decode_prefix(const LengthPrefixKind k, Source *source, uint64_t *len)
{
    if (k == LENP_VARIABLE) {
        return varint_u64_from_source(source, len);
    }

    unsigned char buf[4];
    const unsigned int n = kind[k].size;
    {
        const ssize_t rcsrc = source_get_chunk(source, buf, n);
        if (rcsrc < 0) {
            return rcsrc;
        }
    }

    switch (kind[k].size) {
    case 1: *len = kind[k].cb.u8.parse( buf); break;
    case 2: *len = kind[k].cb.u16.parse(buf); break;
    case 4: *len = kind[k].cb.u32.parse(buf); break;
    default: return -EINVAL;
    }

    return 0;
}

ssize_t
flenp_memory_from_source(const LengthPrefixKind k,
                         Source *source, void *mem, size_t size)
{
    uint64_t len = 0u;
    {
        const ssize_t rc = decode_prefix(k, source, &len);
        if (rc < 0) {
            return rc;
        }
    }
    if (len > size) {
        return -ENOMEM;
    }
    return source_get_chunk(source, mem, len);
}

ssize_t
flenp_buffer_from_source(const LengthPrefixKind k,
                         Source *source, ByteBuffer *b)
{
    const ssize_t rc =
        flenp_memory_from_source(k, source, b->data + b->offset,
                                 byte_buffer_avail(b));

    if (rc >= 0) {
        b->offset += rc;
    }

    return rc;
}

ssize_t
flenp_decode_source_to_sink(const LengthPrefixKind k,
                            Source *source, Sink *sink)
{
    uint64_t len = 0u;
    {
        const ssize_t rc = decode_prefix(k, source, &len);
        if (rc < 0) {
            return rc;
        }
    }

    return sts_n(source, sink, len);
}
