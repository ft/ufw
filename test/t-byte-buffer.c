/*
 * Copyright (c) 2024-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file t-byte-buffer.c
 * @brief Byte Buffer Unit Tests
 */

#include <stdlib.h>
#include <string.h>

#include <ufw/compat/errno.h>
#include <ufw/compiler.h>
#include <ufw/byte-buffer.h>
#include <ufw/test/tap.h>

#define BUFFER_SIZE 1024u
static unsigned char buffer[BUFFER_SIZE];

#define SMALL_BUFFER_SIZE 32u
static unsigned char small_buffer[SMALL_BUFFER_SIZE];

static int
fill_cb_simple(const size_t idx, unsigned char *data)
{
    *data = idx * 0x11u;
    return 0;
}

static int
fill_cb_fun(const size_t idx, unsigned char *data)
{
    if (idx > 20u) {
        return -1;
    }
    if (idx & 1) {
        *data = idx * 10u;
    } else {
        *data = idx & 0xfeu;
    }
    return 0;
}

int
main(UNUSED int argc, UNUSED char **argv)
{
    int rc;
    const char *foobar = "foobar";
    const size_t n = strlen(foobar);

    ByteBuffer b = BYTE_BUFFER_EMPTY(buffer, BUFFER_SIZE);
    ByteBuffer s = BYTE_BUFFER_EMPTY(small_buffer, SMALL_BUFFER_SIZE);
    ByteBuffer thevoid;
    byte_buffer_null(&thevoid);

    plan(43);

    /* Some checks on a NULLed ByteBuffer */
    ok(byte_buffer_rewind(&thevoid) == -EINVAL, "thevoid cannot be rewound");
    ok(byte_buffer_avail(&thevoid) == 0u, "thevoid has no space left");
    ok(byte_buffer_rest(&thevoid) == 0u, "thevoid has no data to give");

    /* And now for the real thing */
    ok(b.used == 0u, "Empty buffer has nothing marked as used");
    ok(b.offset == 0u, "Empty buffer has nothing marked as processed");
    ok(byte_buffer_rest(&b) == 0u, "Empty buffer has nothing in rest");
    ok(byte_buffer_avail(&b) == BUFFER_SIZE,
       "Empty buffer has everything available for use");

    byte_buffer_clear(&b);

    rc = byte_buffer_add(&b, foobar, n);
    ok(rc == 0,
       "%zu bytes fit into empty %zu bytes buffer", n, BUFFER_SIZE);
    cmp_mem(b.data, foobar, n, "Copied %zu bytes match source", n);

    cmp_mem(b.data + n, ((unsigned char[]){0,0,0,0,0,0,0,0}),
            8u, "Next 8 bytes are zero");
    rc = byte_buffer_add(&b, foobar, n);
    ok(rc == 0,
       "%zu bytes fit into empty %zu bytes buffer again", n, BUFFER_SIZE);
    cmp_mem(b.data + n, foobar, n,
            "Copied %zu bytes at offset %zu match source", n, n);
    /* Simulate close to full buffer */
    b.used = BUFFER_SIZE - n + 1;
    ok(byte_buffer_avail(&b) < n, "Make sure foobar does not fit");
    rc = byte_buffer_add(&b, foobar, n);
    ok(rc == -ENOMEM,
       "add fails with -ENOMEM if indicated memory does not fit anymore.");
    /* Leave this section in a knows state */
    b.used = 2 * n;

    {
        char buf[8];

        memset(buf, 0, sizeof(buf));
        rc = byte_buffer_consume(&b, buf, n);
        ok(rc == 0, "Could consume %zu bytes", n);
        cmp_mem(buf, foobar, n, "Consumed bytes are correct");
        ok(b.offset == n, "Process mark is at %zu", n);

        memset(buf, 0, sizeof(buf));
        rc = byte_buffer_consume(&b, buf, n);
        ok(rc == 0, "Could consume %zu bytes again", n);
        cmp_mem(buf, foobar, n, "Consumed bytes are correct again");
        ok(b.offset == 2*n, "Process mark is at %zu", 2*n);

        rc = byte_buffer_consume(&b, buf, n);
        ok(rc == -ENODATA, "Buffer does not have %zu byte to consume", n);
        rc = byte_buffer_consume_at_most(&b, buf, n);
        ok(rc == -ENODATA, "Buffer does not have any bytes to consume");

        byte_buffer_repeat(&b);
        memset(buf, 0, sizeof(buf));
        rc = byte_buffer_consume(&b, buf, n);
        ok(rc == 0, "Could consume %zu bytes after rewind", n);
        cmp_mem(buf, foobar, n, "Consumed bytes are correct after rewind");
        ok(b.offset == n, "Process mark is at %zu after rewind", n);
    }

    byte_buffer_clear(&b);
    b.offset = b.used = BUFFER_SIZE - n;
    rc = byte_buffer_add(&b, foobar, n);
    ok(rc == 0, "Adding foobar to the very end of buffer worked");
    byte_buffer_rewind(&b);
    cmp_mem(b.data, foobar, n, "rewind: Copies data from end to front", n);
    ok(b.offset == 0, "rewind: Process mark is zero");
    ok(b.used == n, "rewind: used is %zu", n);

    {
        const size_t m = SMALL_BUFFER_SIZE;
        static unsigned char buf[SMALL_BUFFER_SIZE];
        byte_buffer_clear(&s);
        byte_buffer_fill(&s, 0x5au);
        memset(buf, 0x5a, m);
        ok(byte_buffer_avail(&s) == 0, "_fill fills a buffer totally");
        cmp_mem(s.data, buf, m, "_fill fills buffer correctly");

        byte_buffer_clear(&s);
        byte_buffer_fillx(&s, 0x10u, 3u);
        ok(byte_buffer_avail(&s) == 0, "_fillx fills a buffer totally");
        for (size_t i = 0u; i < m; ++i) {
            buf[i] = i*3 + 0x10u;
        }
        cmp_mem(s.data, buf, m, "_fillx fills buffer correctly");

        byte_buffer_clear(&s);
        byte_buffer_fillx(&s, 0x10u, -3);
        for (size_t i = 0u; i < m; ++i) {
            buf[i] = (ssize_t)i * (-3) + 0x10u;
        }
        cmp_mem(s.data, buf, m,
                "_fillx fills buffer correctly (neg. increment)");

        byte_buffer_clear(&s);
        byte_buffer_fill_cb(&s, 0, fill_cb_simple);
        ok(byte_buffer_avail(&s) == 0, "a: _fill_cb fills a buffer totally");
        ok(byte_buffer_rest(&s) == m, "a: _fill_cb rest starts at offset");
        for (size_t i = 0u; i < m; ++i) {
            buf[i] = i * 0x11u;
        }
        cmp_mem(s.data, buf, m, "a: _fill_cb fills buffer correctly");

        byte_buffer_clear(&s);
        memset(buf, 0, m);
        byte_buffer_fill_cb(&s, 0x10u, fill_cb_simple);
        ok(byte_buffer_avail(&s) == 0, "b: _fill_cb fills a buffer totally");
        ok(byte_buffer_rest(&s) == m - 0x10u, "b: _fill_cb rest starts at offset");
        for (size_t i = 0x10u; i < m; ++i) {
            buf[i] = i * 0x11u;
        }
        cmp_mem(s.data, buf, m, "b: _fill_cb fills buffer correctly");

        byte_buffer_clear(&s);
        memset(buf, 0, m);
        byte_buffer_fill_cb(&s, 10u, fill_cb_fun);
        ok(byte_buffer_avail(&s) == (m - 21u),
           "c: _fill_cb leaves room from offset 21 onward");
        ok(byte_buffer_rest(&s) == 21u - 10u,
           "c: _fill_cb fills from offset to cb end");
        for (size_t i = 10u; i <= 20u; ++i) {
            if (i & 1u) {
                buf[i] = i * 10u;
            } else {
                buf[i] = i & 0xfeu;;
            }
        }
        cmp_mem(s.data, buf, m, "c: _fill_cb fills buffer correctly");
    }

    return EXIT_SUCCESS;
}
