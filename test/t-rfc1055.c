#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ufw/compat/errno.h>

#include <ufw/compiler.h>
#include <ufw/rfc1055.h>
#include <ufw/endpoints.h>

#include <ufw/test/tap.h>

#define MEMORY_SIZE (1024ul)

static unsigned char source_memory[MEMORY_SIZE];
static unsigned char sink_memory[MEMORY_SIZE];

#define RAW_EOF 0xcu
#define RAW_ESC 0xdbu
#define ESC_EOF 0xdcu
#define ESC_ESC 0xddu

#define MIN(a,b) (((a) < (b)) ? (a) : (b))

/*
 * Example payload for encoding tests.
 *
 * This has all special characters in there, so it should hit a lot of code
 * paths through the encoder.
 */
static unsigned char payload[] = {
    RAW_EOF,
    'a', 'b', 'c',
    RAW_EOF,
    'd', 'e', 'f',
    RAW_ESC,
    'g', 'h', 'i',
    ESC_ESC, 'j', 'k', 'l',
    ESC_EOF, 'm', 'n', 'o',
    ESC_EOF, 'e', 'n', 'd' };

/*
 * Expected encoding result. This can actually be used in the case of the
 * classic encoder and the one that uses a SOF delimiter, because the
 * difference really is just that delimiter. So the classic one can compare to
 * this buffer starting at offset 1.
 */
static unsigned char expect_with_sof[] = {
    RAW_EOF,
    RAW_ESC, ESC_EOF,
    'a', 'b', 'c',
    RAW_ESC, ESC_EOF,
    'd', 'e', 'f',
    RAW_ESC, ESC_ESC,
    'g', 'h', 'i',
    ESC_ESC, 'j', 'k', 'l',
    ESC_EOF, 'm', 'n', 'o',
    ESC_EOF, 'e', 'n', 'd',
    RAW_EOF };

int
main(UNUSED int argc, UNUSED char **argv)
{
    InstrumentableBuffer source_buffer;
    InstrumentableBuffer sink_buffer;
    Source source;
    Sink sink;
    int rc;

    plan(19);

    /*
     * General encoding tests
     */

    /* Connect buffer abstraction to allocated memory */
    octet_buffer_space(&source_buffer.buffer, source_memory, MEMORY_SIZE);
    octet_buffer_space(&sink_buffer.buffer, sink_memory, MEMORY_SIZE);

    /* Make buffer abstractions accessible via sink/source interface */
    instrumentable_source(&source, &source_buffer);
    instrumentable_sink(&sink, &sink_buffer);

    /* Setup RFC1055 context without start-of-frame delimiter use (classic
     * mode) */
    RFC1055Context rfc1055_classic;
    rfc1055_context_init(&rfc1055_classic, RFC1055_DEFAULT);

    /* Load payload buffer into source */
    octet_buffer_add(&source_buffer.buffer, payload, sizeof(payload));

    /* Run RFC1055 encoder without SOF delimiter */
    rc = rfc1055_encode(&rfc1055_classic, &source, &sink);
    unless(ok(rc == 0, "RFC1055 classic encode signals success")) {
        printf("# errno: %s\n", strerror(-rc));
    }
    ok(sink_buffer.buffer.used == (sizeof(expect_with_sof) - 1),
       "RFC1055 classic encode yields correct length");
    cmp_mem(expect_with_sof + 1, sink_buffer.buffer.data,
            MIN(sizeof(expect_with_sof), sink_buffer.buffer.used),
            "RFC1055 classic encode(...) works");

    /* Move read point back to begining of source to be able to repeat the
     * encoding process with the other RFC1055 context. */
    octet_buffer_repeat(&source_buffer.buffer);
    /* Discard and information in sink memory. */
    octet_buffer_clear(&sink_buffer.buffer);

    /* Setup another RFC1055 context that uses start-of-frame. */
    RFC1055Context rfc1055_with_sof;
    rfc1055_context_init(&rfc1055_with_sof, RFC1055_WITH_SOF);

    /* Run RFC1055 encoder without SOF delimiter */
    rc = rfc1055_encode(&rfc1055_with_sof, &source, &sink);
    ok(rc == 0, "RFC1055 with-start-of-frame encode signals success");
    if (rc != 0) {
        printf("# errno: %s\n", strerror(-rc));
    }
    ok(sink_buffer.buffer.used == (sizeof(expect_with_sof)),
       "RFC1055 with-start-of-frame encode yields correct length");
    cmp_mem(expect_with_sof, sink_buffer.buffer.data,
            MIN(sizeof(expect_with_sof), sink_buffer.buffer.used),
            "RFC1055 with-start-of-frame encode(...) works");

    /*
     * Encoding error tests
     *
     * Encoding itself cannot fail. SLIP allows encoding arbitrary sequences of
     * bytes. But the sink can run out of space or error otherwise.
     */

    instrumentable_error_at(&sink_buffer, 10, -EIO);
    octet_buffer_repeat(&source_buffer.buffer);
    octet_buffer_clear(&sink_buffer.buffer);
    rfc1055_context_init(&rfc1055_with_sof, RFC1055_WITH_SOF);
    rc = rfc1055_encode(&rfc1055_with_sof, &source, &sink);
    ok(rc == -EIO, "RFC1055 encoder passes error code correctly, %d", rc);
    instrumentable_no_error(&sink_buffer);

    /*
     * General decoding tests
     */

    /* Re-initialise buffers for decoding tests */
    octet_buffer_space(&source_buffer.buffer, source_memory, MEMORY_SIZE);
    octet_buffer_space(&sink_buffer.buffer, sink_memory, MEMORY_SIZE);

    /* Load valid, classic RFC1055 encoded buffer into source */
    octet_buffer_add(&source_buffer.buffer,
                     expect_with_sof + 1,
                     sizeof(expect_with_sof) - 1);

    rc = rfc1055_decode(&rfc1055_classic, &source, &sink);
    ok(rc == 0, "RFC1055 classic decode signals success");
    if (rc != 0) {
        printf("# errno: %s\n", strerror(-rc));
    }
    ok(sink_buffer.buffer.used == sizeof(payload),
       "RFC1055 classic decode yields correct length (%zu ?= %zu)",
       sink_buffer.buffer.used, sizeof(payload));
    cmp_mem(payload, sink_buffer.buffer.data,
            MIN(sizeof(payload), sink_buffer.buffer.used),
            "RFC1055 classic decode(...) works");

    /* Back to the start again, with-start-of-frame this time */
    octet_buffer_clear(&source_buffer.buffer);
    octet_buffer_add(&source_buffer.buffer, expect_with_sof, sizeof(expect_with_sof));
    octet_buffer_clear(&sink_buffer.buffer);

    rc = rfc1055_decode(&rfc1055_with_sof, &source, &sink);
    ok(rc == 0, "RFC1055 classic decode signals success");
    if (rc != 0) {
        printf("# errno: %s\n", strerror(-rc));
    }
    ok(sink_buffer.buffer.used == sizeof(payload),
       "RFC1055 classic decode yields correct length (%zu ?= %zu)",
       sink_buffer.buffer.used, sizeof(payload));
    cmp_mem(payload, sink_buffer.buffer.data,
            MIN(sizeof(payload), sink_buffer.buffer.used),
            "RFC1055 classic decode(...) works");

    /*
     * Decoding error tests
     *
     * As with the encoder, the sources and sinks could signal errors, which
     * the decoder should forward. In the decoding case, however, the input
     * data stream can be invalid, too.
     *
     * With SLIP, the one special character within a frame is RAW_ESC. It can
     * only be followed by two octets: ESC_EOF and ESC_ESC. Any other octet is
     * invalid and the decoder should error out.
     */

    unsigned char with_error[] = {
        /* We'll be running with-sof enabled, so these first octets will be
         * ignored. */
        'i', 'g', 'n', 'o', 'r', 'e',
        /* This looks like a frame, but the "RAW_ESC o" sequence will cause a
         * decoding error. Then if we restart the decoder, we'll run into a
         * "RAW_EOF RAW_EOF" sequence, which will be interpreted as start of
         * frame. The next frame is broken again. This situation could happen
         * if the character after the RAW_ESC is dropped in the channel. The
         * decoder may thus not just ignore the character after the RAW_ESC as
         * part of an invalid sequence. When restarted, the decoder finally
         * runs into a frame, containing "foo". */
        RAW_EOF, 'f', RAW_ESC, 'o', 'o', RAW_EOF,
        RAW_EOF, 'f', RAW_ESC, RAW_EOF,
        RAW_EOF, 'f', 'o', 'o', RAW_EOF
    };

    octet_buffer_space(&sink_buffer.buffer, sink_memory, MEMORY_SIZE);
    octet_buffer_space(&source_buffer.buffer, source_memory, MEMORY_SIZE);
    octet_buffer_add(&source_buffer.buffer, with_error, sizeof(with_error));

    rc = rfc1055_decode(&rfc1055_with_sof, &source, &sink);
    unless (ok(rc == -EBADMSG, "RFC1055 decode signals BADMSG")) {
        printf("# errno: %s\n", strerror(-rc));
    }

    octet_buffer_clear(&sink_buffer.buffer);
    rc = rfc1055_decode(&rfc1055_with_sof, &source, &sink);
    unless (ok(rc == -EBADMSG, "RFC1055 decode signals BADMSG")) {
        printf("# errno: %s\n", strerror(-rc));
    }

    octet_buffer_clear(&sink_buffer.buffer);
    rc = rfc1055_decode(&rfc1055_with_sof, &source, &sink);
    unless (ok(rc == 0, "RFC1055 decode signals success")) {
        printf("# errno: %s\n", strerror(-rc));
    }
    unless(ok(sink_buffer.buffer.used == 3,
              "RFC1055 Frame after error has correct size")) {
        printf("# actual size: %zu\n", sink_buffer.buffer.used);
    }
    cmp_mem(sink_buffer.buffer.data, "foo", 3,
            "RFC1055 Frame after error has correct contents");

    /* Finally check that errors are passed properly */
    instrumentable_error_at(&sink_buffer, 10, -EIO);

    octet_buffer_space(&sink_buffer.buffer, sink_memory, MEMORY_SIZE);
    octet_buffer_space(&source_buffer.buffer, source_memory, MEMORY_SIZE);
    octet_buffer_add(&source_buffer.buffer,
                     expect_with_sof,
                     sizeof(expect_with_sof));

    rfc1055_context_init(&rfc1055_with_sof, RFC1055_WITH_SOF);
    rc = rfc1055_decode(&rfc1055_with_sof, &source, &sink);

    ok(rc == -EIO, "RFC1055 decoder passes error code correctly, %d", rc);
    instrumentable_no_error(&sink_buffer);

    return EXIT_SUCCESS;
}
