/*
 * Copyright (c) 2022 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file t-varint.c
 * @brief Variable Length Integer Unit Tests
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* newlib: This needs to be included after stdio.h at the moment (3.3.0) for
 * some reason. */
#include <inttypes.h>

#include <ufw/compat/errno.h>
#include <ufw/compiler.h>
#include <ufw/endpoints.h>
#include <ufw/octet-buffer.h>
#include <ufw/test/tap.h>
#include <ufw/variable-length-integer.h>

static void
pe(int e)
{
    if (e < 0)
        printf("# errno: %s\n", strerror(-e));
}

/*
 * 64 Bit test declarations
 */

/* unsigned */

struct test_vui64 {
    uint64_t value;
    size_t octets;
    unsigned char expect[VARINT_64BIT_MAX_OCTETS];
} tests_vui64[] = {
    { UINT64_MAX, VARINT_64BIT_MAX_OCTETS, { 0xffu, 0xffu, 0xffu, 0xffu,
                                             0xffu, 0xffu, 0xffu, 0xffu,
                                             0xffu, 0x01u } },
    {         0u,                       1, { 0u } },
    {       128u,                       2, { 0x80u, 0x01u } },
    {      1234u,                       2, { 0xd2u, 0x09u } }
};

size_t tests_vui64_n = sizeof(tests_vui64) / sizeof(*tests_vui64);

/* signed */

struct test_vsi64 {
    int64_t value;
    size_t octets;
    unsigned char expect[VARINT_64BIT_MAX_OCTETS];
} tests_vsi64[] = {
    /* VARINT_64BIT_MAX_OCTETS - 1 because we needs the 10th octet for the 64th
     * bit only. This bit is only used in negative numbers. */
    { INT64_MAX, VARINT_64BIT_MAX_OCTETS - 1, { 0xffu, 0xffu, 0xffu, 0xffu,
                                                0xffu, 0xffu, 0xffu, 0xffu,
                                                0x7fu } },
    { INT64_MIN, VARINT_64BIT_MAX_OCTETS    , { 0x80u, 0x80u, 0x80u, 0x80u,
                                                0x80u, 0x80u, 0x80u, 0x80u,
                                                0x80u, 0x01u } },
    {        -1, VARINT_64BIT_MAX_OCTETS    , { 0xffu, 0xffu, 0xffu, 0xffu,
                                                0xffu, 0xffu, 0xffu, 0xffu,
                                                0xffu, 0x01u } },
    {       128,                           2, { 0x80u, 0x01u } },
    {      -128, VARINT_64BIT_MAX_OCTETS    , { 0x80u, 0xffu, 0xffu, 0xffu,
                                                0xffu, 0xffu, 0xffu, 0xffu,
                                                0xffu, 0x01u } },
    {      1234,                           2, { 0xd2u, 0x09u } },
    {     -1234, VARINT_64BIT_MAX_OCTETS    , { 0xaeu, 0xf6u, 0xffu, 0xffu,
                                                0xffu, 0xffu, 0xffu, 0xffu,
                                                0xffu, 0x01u } }
};

size_t tests_vsi64_n = sizeof(tests_vsi64) / sizeof(*tests_vsi64);

/*
 * 32 Bit test declarations
 */

/* unsigned */

struct test_vui32 {
    uint32_t value;
    size_t octets;
    unsigned char expect[VARINT_32BIT_MAX_OCTETS];
} tests_vui32[] = {
    { UINT32_MAX, VARINT_32BIT_MAX_OCTETS, { 0xffu, 0xffu, 0xffu, 0xffu, 0x0fu } },
    {         0u,                       1, { 0u } },
    {       128u,                       2, { 0x80u, 0x01u } },
    {      1234u,                       2, { 0xd2u, 0x09u } }
};

size_t tests_vui32_n = sizeof(tests_vui32) / sizeof(*tests_vui32);

/* signed */

struct test_vsi32 {
    int32_t value;
    size_t octets;
    unsigned char expect[VARINT_32BIT_MAX_OCTETS];
} tests_vsi32[] = {
    { INT32_MAX, VARINT_32BIT_MAX_OCTETS, { 0xffu, 0xffu, 0xffu, 0xffu, 0x07u } },
    { INT32_MIN, VARINT_32BIT_MAX_OCTETS, { 0x80u, 0x80u, 0x80u, 0x80u, 0x08u } },
    {        -1, VARINT_32BIT_MAX_OCTETS, { 0xffu, 0xffu, 0xffu, 0xffu, 0x0fu } },
    {       128,                       2, { 0x80u, 0x01u } },
    {      -128, VARINT_32BIT_MAX_OCTETS, { 0x80u, 0xffu, 0xffu, 0xffu, 0x0fu } },
    {      1234,                       2, { 0xd2u, 0x09u } },
    {     -1234, VARINT_32BIT_MAX_OCTETS, { 0xaeu, 0xf6u, 0xffu, 0xffu, 0x0fu } }
};

size_t tests_vsi32_n = sizeof(tests_vsi32) / sizeof(*tests_vsi32);

int
main(UNUSED int argc, UNUSED char **argv)
{
    /* Three specification per table. Two times decoding, one time encoding. */
    plan(3 * 3 * tests_vui64_n
       + 3 * 3 * tests_vsi64_n
       + 3 * 3 * tests_vui32_n
       + 3 * 3 * tests_vsi32_n);

    /* 64 bit unsigned varint - encode */

    for (size_t i = 0u; i < tests_vui64_n; ++i) {
        struct test_vui64 *t = tests_vui64 + i;
        OctetBuffer b;
        unsigned char buf[VARINT_64BIT_MAX_OCTETS];
        octet_buffer_space(&b, buf, VARINT_64BIT_MAX_OCTETS);
        int rc = varint_encode_u64(t->value, &b);
        ok(rc == 0, "u64: Encoding %"PRIu64" signals success", t->value);
        pe(rc);
        ok(b.used == t->octets,
           "u64: Buffer signals correct occupation %zu ?= %zu",
           b.used, t->octets);
        cmp_mem(b.data, t->expect, t->octets,
                "u64: %"PRIu64" encodes correctly", t->value);
    }

    /* 64 bit unsigned varint - decode */

    for (size_t i = 0u; i < tests_vui64_n; ++i) {
        struct test_vui64 *t = tests_vui64 + i;
        OctetBuffer b;
        octet_buffer_space(&b, t->expect, VARINT_64BIT_MAX_OCTETS);
        uint64_t value;
        int rc = varint_decode_u64(&b, &value);
        ok(rc == 0, "u64: Decoding %"PRIu64" signals success", t->value);
        pe(rc);
        ok(b.offset == t->octets,
           "u64: Buffer signals correct offset %zu ?= %zu",
           b.offset, t->octets);
        cmp_mem(&value, &t->value, sizeof(uint64_t),
                "u64: %"PRIu64" decodes correctly", t->value);
    }

    /* 32 bit unsigned varint - encode */

    for (size_t i = 0u; i < tests_vui32_n; ++i) {
        struct test_vui32 *t = tests_vui32 + i;
        OctetBuffer b;
        unsigned char buf[VARINT_32BIT_MAX_OCTETS];
        octet_buffer_space(&b, buf, VARINT_32BIT_MAX_OCTETS);
        int rc = varint_encode_u32(t->value, &b);
        ok(rc == 0, "u32: Encoding %"PRIu32" signals success", t->value);
        pe(rc);
        ok(b.used == t->octets,
           "u32: Buffer signals correct occupation %zu ?= %zu",
           b.used, t->octets);
        cmp_mem(b.data, t->expect, t->octets,
                "u32: %"PRIu32" encodes correctly", t->value);
    }

    /* 32 bit unsigned varint - decode */

    for (size_t i = 0u; i < tests_vui32_n; ++i) {
        struct test_vui32 *t = tests_vui32 + i;
        OctetBuffer b;
        octet_buffer_space(&b, t->expect, VARINT_32BIT_MAX_OCTETS);
        uint32_t value;
        int rc = varint_decode_u32(&b, &value);
        ok(rc == 0, "u32: Decoding %"PRIu32" signals success", t->value);
        pe(rc);
        ok(b.offset == t->octets,
           "u32: Buffer signals correct offset %zu ?= %zu",
           b.offset, t->octets);
        cmp_mem(&value, &t->value, sizeof(uint32_t),
                "u32: %"PRIu32" decodes correctly", t->value);
    }

    /* 64 bit signed varint - encode */

    for (size_t i = 0u; i < tests_vsi64_n; ++i) {
        struct test_vsi64 *t = tests_vsi64 + i;
        OctetBuffer b;
        signed char buf[VARINT_64BIT_MAX_OCTETS];
        octet_buffer_space(&b, buf, VARINT_64BIT_MAX_OCTETS);
        int rc = varint_encode_s64(t->value, &b);
        ok(rc == 0, "s64: Encoding %"PRId64" signals success", t->value);
        pe(rc);
        ok(b.used == t->octets,
           "s64: Buffer signals correct occupation %zu ?= %zu",
           b.used, t->octets);
        cmp_mem(b.data, t->expect, t->octets,
                "s64: %"PRId64" encodes correctly", t->value);
    }

    /* 64 bit signed varint - decode */

    for (size_t i = 0u; i < tests_vsi64_n; ++i) {
        struct test_vsi64 *t = tests_vsi64 + i;
        OctetBuffer b;
        octet_buffer_space(&b, t->expect, VARINT_64BIT_MAX_OCTETS);
        int64_t value;
        int rc = varint_decode_s64(&b, &value);
        ok(rc == 0, "s64: Decoding %"PRId64" signals success", t->value);
        pe(rc);
        ok(b.offset == t->octets,
           "s64: Buffer signals correct offset %zu ?= %zu",
           b.offset, t->octets);
        cmp_mem(&value, &t->value, sizeof(int64_t),
                "u64: %"PRId64" decodes correctly", t->value);
    }

    /* 32 bit signed varint - encode */

    for (size_t i = 0u; i < tests_vsi32_n; ++i) {
        struct test_vsi32 *t = tests_vsi32 + i;
        OctetBuffer b;
        signed char buf[VARINT_32BIT_MAX_OCTETS];
        octet_buffer_space(&b, buf, VARINT_32BIT_MAX_OCTETS);
        int rc = varint_encode_s32(t->value, &b);
        ok(rc == 0, "s32: Encoding %"PRId32" signals success", t->value);
        pe(rc);
        ok(b.used == t->octets,
           "s32: Buffer signals correct occupation %zu ?= %zu",
           b.used, t->octets);
        cmp_mem(b.data, t->expect, t->octets,
                "s32: %"PRId32" encodes correctly", t->value);
    }

    /* 32 bit signed varint - decode */

    for (size_t i = 0u; i < tests_vsi32_n; ++i) {
        struct test_vsi32 *t = tests_vsi32 + i;
        OctetBuffer b;
        octet_buffer_space(&b, t->expect, VARINT_32BIT_MAX_OCTETS);
        int32_t value;
        int rc = varint_decode_s32(&b, &value);
        ok(rc == 0, "s32: Decoding %"PRId32" signals success", t->value);
        pe(rc);
        ok(b.offset == t->octets,
           "s32: Buffer signals correct offset %zu ?= %zu",
           b.offset, t->octets);
        cmp_mem(&value, &t->value, sizeof(uint32_t),
                "s32: %"PRId32" decodes correctly", t->value);
    }

    /*
     * Now do the decode tests again, with the source API. The tests above co-
     * ver the buffer based API. The encoder API for the sink interface reuses
     * that code. The decoder via source implements the same algorithm as does
     * the buffer based API. The latter just implements it on its own to avoid
     * some overhead associated with calling the octet fetching source API.
     */

    /* 64 bit unsigned varint - decode */

    for (size_t i = 0u; i < tests_vui64_n; ++i) {
        struct test_vui64 *t = tests_vui64 + i;
        OctetBuffer b;
        Source source;
        octet_buffer_use(&b, t->expect, VARINT_64BIT_MAX_OCTETS);
        source_from_buffer(&source, &b);
        uint64_t value;
        int rc = varint_u64_from_source(&source, &value);
        ok(rc == 0, "(source) u64: Decoding %"PRIu64" signals success", t->value);
        pe(rc);
        ok(b.offset == t->octets,
           "(source) u64: Buffer signals correct offset %zu ?= %zu",
           b.offset, t->octets);
        cmp_mem(&value, &t->value, sizeof(uint64_t),
                "(source) u64: %"PRIu64" decodes correctly", t->value);
    }

    /* 32 bit unsigned varint - decode */

    for (size_t i = 0u; i < tests_vui32_n; ++i) {
        struct test_vui32 *t = tests_vui32 + i;
        OctetBuffer b;
        Source source;
        octet_buffer_use(&b, t->expect, VARINT_32BIT_MAX_OCTETS);
        source_from_buffer(&source, &b);
        uint32_t value;
        int rc = varint_u32_from_source(&source, &value);
        ok(rc == 0, "(source) u32: Decoding %"PRIu32" signals success", t->value);
        pe(rc);
        ok(b.offset == t->octets,
           "(source) u32: Buffer signals correct offset %zu ?= %zu",
           b.offset, t->octets);
        cmp_mem(&value, &t->value, sizeof(uint32_t),
                "(source) u32: %"PRIu32" decodes correctly", t->value);
    }

    /* 64 bit signed varint - decode */

    for (size_t i = 0u; i < tests_vsi64_n; ++i) {
        struct test_vsi64 *t = tests_vsi64 + i;
        OctetBuffer b;
        Source source;
        octet_buffer_use(&b, t->expect, VARINT_64BIT_MAX_OCTETS);
        source_from_buffer(&source, &b);
        int64_t value;
        int rc = varint_s64_from_source(&source, &value);
        ok(rc == 0, "(source) s64: Decoding %"PRId64" signals success", t->value);
        pe(rc);
        ok(b.offset == t->octets,
           "(source) s64: Buffer signals correct offset %zu ?= %zu",
           b.offset, t->octets);
        cmp_mem(&value, &t->value, sizeof(int64_t),
                "(source) u64: %"PRId64" decodes correctly", t->value);
    }

    /* 32 bit signed varint - decode */

    for (size_t i = 0u; i < tests_vsi32_n; ++i) {
        struct test_vsi32 *t = tests_vsi32 + i;
        OctetBuffer b;
        Source source;
        octet_buffer_use(&b, t->expect, VARINT_32BIT_MAX_OCTETS);
        source_from_buffer(&source, &b);
        int32_t value;
        int rc = varint_s32_from_source(&source, &value);
        ok(rc == 0, "(source) s32: Decoding %"PRId32" signals success", t->value);
        pe(rc);
        ok(b.offset == t->octets,
           "(source) s32: Buffer signals correct offset %zu ?= %zu",
           b.offset, t->octets);
        cmp_mem(&value, &t->value, sizeof(uint32_t),
                "(source) s32: %"PRId32" decodes correctly", t->value);
    }

    return EXIT_SUCCESS;
}
