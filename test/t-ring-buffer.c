#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <c/ring-buffer.h>
#include <c/ring-buffer-iter.h>
#include <c/ring-buffer-iter.c>
#include <common/compiler.h>

#include <tap.h>

RING_BUFFER_API(byte_buffer, uint8_t)
RING_BUFFER_ITER_API(byte_buffer, uint8_t)

RING_BUFFER(byte_buffer, uint8_t)
RING_BUFFER_ITER(byte_buffer, uint8_t)

int
main(UNUSED int argc, UNUSED char *argv[])
{
    const size_t bufsize = 16;
    const uint8_t data[] = { 11, 22, 33, 44, 55, 66, 77, 88 };
    uint8_t buffer[bufsize];
    byte_buffer foo;
    rb_iter iter;

    plan(16);

    /* Initialise ring-buffer with buffer as its data-storage. */
    byte_buffer_init(&foo, buffer, bufsize);

    /* Put data into the buffer. */
    for (size_t i = 0; i < sizeof(data); ++i)
        byte_buffer_put(&foo, data[i]);

    /* Iterate across the buffer; this consumes nothing. */
    byte_buffer_iter(&iter, &foo, RING_BUFFER_ITER_OLD_TO_NEW);
    for (size_t i = 0; !rb_iter_done(&iter); rb_iter_advance(&iter), i++)
        cmp_ok(byte_buffer_inspect(&foo, &iter), "==", data[i],
               "(iterate) data[%lu] found", i);

    /* Consume data until the buffer is empty. */
    for (size_t i = 0; !byte_buffer_empty(&foo); ++i)
        cmp_ok(byte_buffer_get(&foo), "==", data[i],
               "(get) data[%lu] found", i);

    return EXIT_SUCCESS;
}
