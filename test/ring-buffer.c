#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <c/ring-buffer.h>
#include <c/ring-buffer-iter.h>
#include <c/ring-buffer-iter.c>
#include <compiler.h>

RING_BUFFER_API(byte_buffer, uint8_t)
RING_BUFFER_ITER_API(byte_buffer, uint8_t)

RING_BUFFER(byte_buffer, uint8_t)
RING_BUFFER_ITER(byte_buffer, uint8_t)

int
main(UNUSED int argc, UNUSED char *argv[])
{
    const size_t bufsize = 16;
    uint8_t buffer[bufsize];
    byte_buffer foo;
    rb_iter iter;

    /* Initialise ring-buffer with buffer as its data-storage. */
    byte_buffer_init(&foo, buffer, bufsize);

    /* Put two bytes into the buffer. */
    byte_buffer_put(&foo, 23);
    byte_buffer_put(&foo, 42);

    /* Iterate across the buffer; this consumes nothing. */
    byte_buffer_iter(&iter, &foo, RING_BUFFER_ITER_OLD_TO_NEW);
    for (size_t i = 0; !rb_iter_done(&iter); rb_iter_advance(&iter), i++)
        printf("# %lu: %u (iterate)\n", i, byte_buffer_inspect(&foo, &iter));

    /* Consume data until the buffer is empty. */
    for (size_t i = 0; !byte_buffer_empty(&foo); ++i)
        printf("# %lu: %u (fetch)\n", i, byte_buffer_get(&foo));

    return EXIT_SUCCESS;
}
