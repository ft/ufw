#include <stdbool.h>
#include <stdint.h>

#include <ufw/compat/errno.h>
#include <ufw/endpoints.h>
#include <ufw/octet-buffer.h>
#include <ufw/variable-length-integer.h>

union varint32 {
    uint32_t u;
    int32_t s;
};

union varint64 {
    uint64_t u;
    int64_t s;
};

static inline bool
varint_done(const unsigned char octet)
{
    return ((octet & VARINT_CONTINUATION_MASK) == 0u);
}

static void
varint_encode(uint64_t n, OctetBuffer *b)
{
    unsigned char *buf = b->data + b->offset;
    for (;;) {
        *buf = n & VARINT_DATA_MASK;
        n >>= VARINT_DATA_BITS;
        if (n == 0) {
            b->used = buf - b->data + 1u;
            return;
        }
        *buf |= VARINT_CONTINUATION_MASK;
        buf++;
    }
}

static int
varint_decode(OctetBuffer *b, const size_t maxoctets, union varint64 *n)
{
    const unsigned char *buf = b->data + b->offset;
    n->u = 0u;

    for (size_t i = 0u; i < maxoctets; ++i) {
        const unsigned char datum = buf[i];
        n->u |= (uint64_t)(datum & VARINT_DATA_MASK) << (i * VARINT_DATA_BITS);
        if (varint_done(datum)) {
            b->offset += i + 1u;
            return 0;
        }
    }

    return -EBADMSG;
}

static int
varint_from_source(Source *source, const size_t maxoctets, union varint64 *n)
{
    n->u = 0u;

    for (size_t i = 0u; i < maxoctets; ++i) {
        unsigned char data;
        const int rc = source_get_octet(source, &data);
        const unsigned char bits = data & VARINT_DATA_MASK;
        n->u |= (uint64_t)bits << (i * VARINT_DATA_BITS);
        if (rc < 0) {
            return rc;
        } else if (varint_done(data)) {
            return 0;
        }
    }

    return -EBADMSG;
}

int
varint_decode_u32(OctetBuffer *b, uint32_t *n)
{
    union varint64 data;
    const int rc = varint_decode(b, VARINT_32BIT_MAX_OCTETS, &data);
    *n = data.u & UINT32_MAX;
    return rc;
}

int
varint_decode_s32(OctetBuffer *b, int32_t *n)
{
    union varint64 data64;
    union varint32 data32;
    const int rc = varint_decode(b, VARINT_32BIT_MAX_OCTETS, &data64);
    data32.u = data64.u & UINT32_MAX;
    *n = data32.s;
    return rc;
}

int
varint_decode_u64(OctetBuffer *b, uint64_t *n)
{
    union varint64 data;
    const int rc = varint_decode(b, VARINT_64BIT_MAX_OCTETS, &data);
    *n = data.u;
    return rc;
}

int
varint_decode_s64(OctetBuffer *b, int64_t *n)
{
    union varint64 data;
    const int rc = varint_decode(b, VARINT_64BIT_MAX_OCTETS, &data);
    *n = data.s;
    return rc;
}

int
varint_encode_u32(const uint32_t n, OctetBuffer *b)
{
    if (octet_buffer_avail(b) < VARINT_32BIT_MAX_OCTETS) {
        return -EINVAL;
    }
    varint_encode(n & UINT32_MAX, b);
    return 0;
}

int
varint_encode_s32(const int32_t n, OctetBuffer *b)
{
    if (octet_buffer_avail(b) < VARINT_32BIT_MAX_OCTETS) {
        return -EINVAL;
    }
    union varint64 data;
    data.s = n;
    varint_encode(data.u & UINT32_MAX, b);
    return 0;
}

int
varint_encode_u64(const uint64_t n, OctetBuffer *b)
{
    if (octet_buffer_avail(b) < VARINT_64BIT_MAX_OCTETS) {
        return -EINVAL;
    }
    varint_encode(n, b);
    return 0;
}

int
varint_encode_s64(const int64_t n, OctetBuffer *b)
{
    if (octet_buffer_avail(b) < VARINT_64BIT_MAX_OCTETS) {
        return -EINVAL;
    }
    union varint64 data;
    data.s = n;
    varint_encode(data.u, b);
    return 0;
}

int
varint_u32_from_source(Source *source, uint32_t *n)
{
    union varint64 data;
    const int rc =
        varint_from_source(source, VARINT_32BIT_MAX_OCTETS, &data);
    if (rc < 0) {
        return rc;
    }
    *n = data.u & UINT32_MAX;
    return 0;
}

int
varint_s32_from_source(Source *source, int32_t *n)
{
    union varint64 data64;
    union varint32 data32;
    const int rc =
        varint_from_source(source, VARINT_32BIT_MAX_OCTETS, &data64);
    if (rc < 0) {
        return rc;
    }
    data32.u = data64.u & UINT32_MAX;
    *n = data32.s;
    return 0;
}

int
varint_u64_from_source(Source *source, uint64_t *n)
{
    union varint64 data;
    const int rc =
        varint_from_source(source, VARINT_64BIT_MAX_OCTETS, &data);
    if (rc < 0) {
        return rc;
    }
    *n = data.u;
    return 0;
}

int
varint_s64_from_source(Source *source, int64_t *n)
{
    union varint64 data;
    const int rc =
        varint_from_source(source, VARINT_64BIT_MAX_OCTETS, &data);
    if (rc < 0) {
        return rc;
    }
    *n = data.s;
    return 0;
}

int
varint_u32_to_sink(const uint32_t n, Sink *sink)
{
    unsigned char raw[VARINT_32BIT_MAX_OCTETS];
    OctetBuffer buf;

    octet_buffer_space(&buf, raw, VARINT_32BIT_MAX_OCTETS);
    varint_encode_u32(n, &buf);
    return sink_put_chunk(sink, buf.data, buf.used);
}

int
varint_s32_to_sink(const int32_t n,  Sink *sink)
{
    unsigned char raw[VARINT_32BIT_MAX_OCTETS];
    OctetBuffer buf;

    octet_buffer_space(&buf, raw, VARINT_32BIT_MAX_OCTETS);
    varint_encode_s32(n, &buf);
    return sink_put_chunk(sink, buf.data, buf.used);
}

int
varint_u64_to_sink(const uint64_t n, Sink *sink)
{
    unsigned char raw[VARINT_64BIT_MAX_OCTETS];
    OctetBuffer buf;

    octet_buffer_space(&buf, raw, VARINT_64BIT_MAX_OCTETS);
    varint_encode_u64(n, &buf);
    return sink_put_chunk(sink, buf.data, buf.used);
}

int
varint_s64_to_sink(const int64_t n,  Sink *sink)
{
    unsigned char raw[VARINT_64BIT_MAX_OCTETS];
    OctetBuffer buf;

    octet_buffer_space(&buf, raw, VARINT_64BIT_MAX_OCTETS);
    varint_encode_s64(n, &buf);
    return sink_put_chunk(sink, buf.data, buf.used);
}

size_t
varint_u64_length(uint64_t n)
{
    for (size_t octets = 1u; /*forever*/; ++octets) {
        n >>= VARINT_DATA_BITS;
        if (n == 0) {
            return octets;
        }
    }
}

size_t
varint_s64_length(int64_t n)
{
    union varint64 data;
    data.s = n;
    return varint_u64_length(data.u);
}

size_t
varint_u32_length(uint32_t n)
{
    return varint_u64_length((uint64_t)n);
}

size_t
varint_s32_length(int32_t n)
{
    union varint32 data;
    data.s = n;
    return varint_u64_length((uint64_t)data.u);
}
