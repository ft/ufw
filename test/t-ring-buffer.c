/*
 * Copyright (c) 2017-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <ufw/compiler.h>
#include <ufw/test/tap.h>

#include <ufw/ring-buffer.h>
#include <ufw/ring-buffer-iter.h>

RING_BUFFER_API(byte_buffer, uint16_t)
RING_BUFFER_ITER_API(byte_buffer, uint16_t)

RING_BUFFER(byte_buffer, uint16_t)
RING_BUFFER_ITER(byte_buffer, uint16_t)

#define BUFSIZE 16

int
main(UNUSED int argc, UNUSED char *argv[])
{
    const uint16_t data[] = { 11, 22, 33, 44, 55, 66, 77, 88 };
    uint16_t buffer[BUFSIZE];
    byte_buffer foo;
    rb_iter iter;

    plan(16);

    /* Initialise ring-buffer with buffer as its data-storage. */
    byte_buffer_init(&foo, buffer, BUFSIZE);

    /* Put data into the buffer. */
    for (size_t i = 0; i < sizeof(data)/sizeof(*data); ++i)
        byte_buffer_put(&foo, data[i]);

    /* Iterate across the buffer; this consumes nothing. */
    byte_buffer_iter(&iter, &foo, RING_BUFFER_ITER_OLD_TO_NEW);
    for (size_t i = 0; !rb_iter_done(&iter); rb_iter_advance(&iter), i++) {
        uint16_t value = byte_buffer_inspect(&foo, &iter);
        unless(ok(value == data[i], "(iterate) data[%lu] found", i)) {
            pru16(value, data[i]);
        }
    }

    /* Consume data until the buffer is empty. */
    for (size_t i = 0; !byte_buffer_empty(&foo); ++i) {
        uint16_t value = byte_buffer_get(&foo);
        unless (ok(value == data[i], "(get) data[%lu] found", i)) {
            pru16(value, data[i]);
        }
    }

    return EXIT_SUCCESS;
}
