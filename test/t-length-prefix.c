/*
 * Copyright (c) 2022-2024 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file t-length-prefix.c
 * @brief Length Prefix Encoding Unit Tests
 */

/* See the src/registers/utilities.c for reasons to this. */
#include <ufw/toolchain.h>

#ifdef WITH_SYS_TYPES_H
#include <sys/types.h>
#endif /* WITH_SYS_TYPES_H */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ufw/byte-buffer.h>
#include <ufw/compat/errno.h>
#include <ufw/compiler.h>
#include <ufw/length-prefix.h>
#include <ufw/test/tap.h>

#define BUFFER_SIZE 1024u
#define MEM_SIZE BUFFER_SIZE
#define WIRE_SIZE (BUFFER_SIZE + VARINT_64BIT_MAX_OCTETS)

unsigned char mema[BUFFER_SIZE];
unsigned char memb[BUFFER_SIZE];
unsigned char wire[WIRE_SIZE];

static void
t_varint_prefix(void)
{
    LengthPrefixBuffer lpb;
    int rc;

    memset(wire, 0, WIRE_SIZE);
    memset(memb, 0, MEM_SIZE);
    for (size_t i = 0u; i < MEM_SIZE; ++i) {
        mema[i] = i & 0xffu;
    }

    rc = lenp_memory_encode(&lpb, mema, 127);
    ok(rc == 0, "lenp: Encoding succeeded");
    ok(lpb.prefix.used == 1, "lenp: 127 takes 1 octet to encode");

    rc = lenp_memory_encode(&lpb, mema, 128);
    ok(rc == 0, "lenp: Encoding succeeded");
    ok(lpb.prefix.used == 2, "lenp: 128 takes 1 octet to encode");

    Sink sink;
    ByteBuffer sinkb;
    byte_buffer_space(&sinkb, wire, WIRE_SIZE);
    sink_to_buffer(&sink, &sinkb);

    const size_t m = varint_u64_length(MEM_SIZE);
    const ssize_t n = lenp_memory_to_sink(&sink, mema, MEM_SIZE);
    if (ok(m == 2, "lenp: MEM_SIZE encodes in two octets") &&
        ok((n <= 0) == false, "lenp: Encode to sink succeeded"))
    {
        ok((size_t)n == (m + MEM_SIZE), "lenp: Prefix+payload size is correct");
    }
    cmp_mem(wire, ((unsigned char[]){0x80u, 0x08u}), m, "lenp: Prefix is correct");
    byte_buffer_repeat(&sinkb);
    uint64_t value_u64;
    rc = varint_decode_u64(&sinkb, &value_u64);
    ok(rc == 2, "lenp: Decoding size value succeeded");
    ok(MEM_SIZE == value_u64,
       "lenp: Encoded size is MEM_SIZE (%"PRId64")", MEM_SIZE);
    cmp_mem(wire + m, mema, MEM_SIZE, "lenp: Encoded memory is correct");

    Source source;
    ByteBuffer sourceb, finalbuffer;
    byte_buffer_use(&sourceb, wire, m + MEM_SIZE);
    source_from_buffer(&source, &sourceb);
    byte_buffer_space(&finalbuffer, memb, MEM_SIZE);

    const ssize_t k = lenp_buffer_from_source(&source, &finalbuffer);
    if (ok((k <= 0) == false, "lenp: Decode from source succeeded")) {
        ok(k == MEM_SIZE,
           "lenp: Reading back prefixed buffer yields right size");
    }
    cmp_mem(mema, memb, MEM_SIZE, "lenp: Final memory is correct");
}

static void
t_fixint_prefix(void)
{
    LengthPrefixBuffer lpb;
    int rc;

    memset(wire, 0, WIRE_SIZE);
    memset(memb, 0, MEM_SIZE);
    for (size_t i = 0u; i < MEM_SIZE; ++i) {
        mema[i] = i & 0xffu;
    }

    rc = flenp_memory_encode(LENP_OCTET, &lpb, mema, 200);
    ok(rc == 0, "lenp: Encoding succeeded (LENP_OCTET, 200)");
    ok(lpb.prefix.used == 1, "lenp: 200 takes 1 octet to encode");

    rc = flenp_memory_encode(LENP_OCTET, &lpb, mema, 256);
    ok(rc == -EINVAL, "lenp: Encoding failed EINVAL (LENP_OCTET, 256)");

    rc = flenp_memory_encode(LENP_LE_16BIT, &lpb, mema, 200);
    ok(rc == 0, "lenp: Encoding succeeded (LENP_LE_16BIT, 200)");
    ok(lpb.prefix.used == 2, "lenp: 200 takes 2 octets to encode");

    rc = flenp_memory_encode(LENP_LE_16BIT, &lpb, mema, UINT16_MAX + 1);
    ok(rc == -EINVAL,
       "lenp: Encoding failed EINVAL (LENP_LE_16BIT, UINT16_MAX + 1)");

    rc = flenp_memory_encode(LENP_BE_32BIT, &lpb, mema, 200);
    ok(rc == 0, "lenp: Encoding succeeded (LENP_BE_32BIT, 200)");
    ok(lpb.prefix.used == 4, "lenp: 200 takes 4 octets to encode");

    rc = flenp_memory_encode(LENP_BE_32BIT, &lpb, mema, UINT32_MAX + 1);
    ok(rc == -EINVAL,
       "lenp: Encoding failed EINVAL (LENP_BE_32BIT, UINT32_MAX + 1)");

    Sink sink;
    ByteBuffer sinkb;
    byte_buffer_space(&sinkb, wire, WIRE_SIZE);
    sink_to_buffer(&sink, &sinkb);

    {
        const ssize_t n = flenp_memory_to_sink(LENP_OCTET, &sink, mema, 200);
        if (ok(n >= 0, "lenp,octet: Encode to sink succeeded")) {
            ok((size_t)n == 201, "lenp: Prefix+payload size is correct");
            cmp_mem(wire, ((unsigned char[]){0xc8u}), 1,
                    "lenp,octet: Prefix is correct");
            cmp_mem(wire + 1, mema, 200,
                    "lenp,octet: Encoded memory is correct");
        }
    }

    byte_buffer_clear(&sinkb);

    {
        const ssize_t n = flenp_memory_to_sink(
            LENP_LE_16BIT, &sink, mema, MEM_SIZE);
        if (ok(n >= 0, "lenp,16le: Encode to sink succeeded")) {
            ok((size_t)n == (2u + MEM_SIZE),
               "lenp,16le: Prefix+payload size is correct");
            cmp_mem(wire, ((unsigned char[]){0x00u, 0x04u}), 2,
                    "lenp,16le: Prefix is correct");
            cmp_mem(wire + 2, mema, MEM_SIZE,
                    "lenp,16le: Encoded memory is correct");
        }
    }

    byte_buffer_clear(&sinkb);

    {
        const ssize_t n = flenp_memory_to_sink(
            LENP_LE_32BIT, &sink, mema, MEM_SIZE);
        if (ok(n >= 0, "lenp,32le: Encode to sink succeeded")) {
            ok((size_t)n == (4u + MEM_SIZE),
               "lenp,32le: Prefix+payload size is correct");
            cmp_mem(wire, ((unsigned char[]){0x00u, 0x04u, 0x00u, 0x00u}), 4,
                    "lenp,32le: Prefix is correct");
            cmp_mem(wire + 4, mema, MEM_SIZE,
                    "lenp,32le: Encoded memory is correct");
        }
    }

    byte_buffer_clear(&sinkb);

    {
        const ssize_t n = flenp_memory_to_sink(
            LENP_BE_16BIT, &sink, mema, MEM_SIZE);
        if (ok(n >= 0, "lenp,16be: Encode to sink succeeded")) {
            ok((size_t)n == (2u + MEM_SIZE),
               "lenp,16be: Prefix+payload size is correct");
            cmp_mem(wire, ((unsigned char[]){0x04u, 0x00u}), 2,
                    "lenp,16be: Prefix is correct");
            cmp_mem(wire + 2, mema, MEM_SIZE,
                    "lenp,16be: Encoded memory is correct");
        }
    }

    byte_buffer_clear(&sinkb);

    {
        const ssize_t n = flenp_memory_to_sink(
            LENP_BE_32BIT, &sink, mema, MEM_SIZE);
        if (ok(n >= 0, "lenp,32be: Encode to sink succeeded")) {
            ok((size_t)n == (4u + MEM_SIZE),
               "lenp,32be: Prefix+payload size is correct");
            cmp_mem(wire, ((unsigned char[]){0x00u, 0x00u, 0x04u, 0x00u}), 4,
                    "lenp,32be: Prefix is correct");
            cmp_mem(wire + 4, mema, MEM_SIZE,
                    "lenp,32be: Encoded memory is correct");
        }
    }
}

int
main(UNUSED int argc, UNUSED char *argv[])
{
    plan(14 + 29);

    t_varint_prefix(); /* 14 */
    t_fixint_prefix(); /* 29 */

    return EXIT_SUCCESS;
}
