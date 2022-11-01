/*
 * Copyright (c) 2022 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file t-length-prefix.c
 * @brief Length Prefix Encoding Unit Tests
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <inttypes.h>

#include <ufw/compat/errno.h>
#include <ufw/compiler.h>
#include <ufw/length-prefix.h>
#include <ufw/octet-buffer.h>
#include <ufw/test/tap.h>

#define BUFFER_SIZE 1024u
#define MEM_SIZE BUFFER_SIZE
#define WIRE_SIZE (BUFFER_SIZE + VARINT_64BIT_MAX_OCTETS)

unsigned char mema[BUFFER_SIZE];
unsigned char memb[BUFFER_SIZE];
unsigned char wire[WIRE_SIZE];

int
main(UNUSED int argc, UNUSED char *argv[])
{
    LengthPrefixBuffer lpb;
    int rc;

    memset(wire, 0, WIRE_SIZE);
    memset(memb, 0, MEM_SIZE);
    for (size_t i = 0u; i < MEM_SIZE; ++i) {
        mema[i] = i & 0xffu;
    }

    plan(14);

    rc = lenp_memory_encode(&lpb, mema, 127);
    ok(rc == 0, "lenp: Encoding succeeded");
    ok(lpb.prefix.used == 1, "lenp: 127 takes 1 octet to encode");

    rc = lenp_memory_encode(&lpb, mema, 128);
    ok(rc == 0, "lenp: Encoding succeeded");
    ok(lpb.prefix.used == 2, "lenp: 128 takes 1 octet to encode");

    Sink sink;
    OctetBuffer sinkb;
    octet_buffer_space(&sinkb, wire, WIRE_SIZE);
    sink_to_buffer(&sink, &sinkb);

    const size_t m = varint_u64_length(MEM_SIZE);
    const ssize_t n = lenp_memory_to_sink(&sink, mema, MEM_SIZE);
    if (ok(m == 2, "lenp: MEM_SIZE encodes in two octets") &&
        ok((n <= 0) == false, "lenp: Encode to sink succeeded"))
    {
        ok((size_t)n == (m + MEM_SIZE), "lenp: Prefix+payload size is correct");
    }
    cmp_mem(wire, ((unsigned char[]){0x80u, 0x08u}), m, "lenp: Prefix is correct");
    octet_buffer_repeat(&sinkb);
    uint64_t value_u64;
    rc = varint_decode_u64(&sinkb, &value_u64);
    ok(rc == 2, "lenp: Decoding size value succeeded");
    ok(MEM_SIZE == value_u64,
       "lenp: Encoded size is MEM_SIZE (%"PRId64")", MEM_SIZE);
    cmp_mem(wire + m, mema, MEM_SIZE, "lenp: Encoded memory is correct");

    Source source;
    OctetBuffer sourceb, finalbuffer;
    octet_buffer_use(&sourceb, wire, m + MEM_SIZE);
    source_from_buffer(&source, &sourceb);
    octet_buffer_space(&finalbuffer, memb, MEM_SIZE);

    const ssize_t k = lenp_buffer_from_source(&source, &finalbuffer);
    if (ok((k <= 0) == false, "lenp: Decode from source succeeded")) {
        ok(k == MEM_SIZE,
           "lenp: Reading back prefixed buffer yields right size");
    }
    cmp_mem(mema, memb, MEM_SIZE, "lenp: Final memory is correct");

    return EXIT_SUCCESS;
}
