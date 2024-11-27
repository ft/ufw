#include <stdlib.h>
#include <string.h>

#include <ufw/compat/errno.h>
#include <ufw/compiler.h>
#include <ufw/endpoints.h>

#include <ufw/test/tap.h>

#define SRC_SIZE (1024ul)
#define SNK_SIZE (2*1024ul)
static unsigned char src_bo[SRC_SIZE];
static unsigned char src_bc[SRC_SIZE];
static unsigned char snk_bo[SNK_SIZE];
static unsigned char snk_bc[SNK_SIZE];

#define AUX_SIZE (128u)
static unsigned char auxb[AUX_SIZE];

static InstrumentableBuffer isrc_bo = INSTRUMENTABLE_BUFFER(src_bo, SRC_SIZE);
static InstrumentableBuffer isnk_bo = INSTRUMENTABLE_BUFFER(snk_bo, SNK_SIZE);
static InstrumentableBuffer isrc_bc = INSTRUMENTABLE_BUFFER(src_bc, SRC_SIZE);
static InstrumentableBuffer isnk_bc = INSTRUMENTABLE_BUFFER(snk_bc, SNK_SIZE);

static void
test_reset_meta(InstrumentableBuffer *b)
{
    instrumentable_reset_error(&b->read.error);
    instrumentable_reset_error(&b->write.error);
    instrumentable_reset_stats(&b->read.stat);
    instrumentable_reset_stats(&b->write.stat);
    instrumentable_chunksize(b, 5u);
}

/* Helper to set all instrumentable buffers into a known state before executing
 * a new set of tests. */
static void
test_reset(void)
{
    /* Reset all status counters and error setup. */
    test_reset_meta(&isrc_bo);
    test_reset_meta(&isrc_bc);
    test_reset_meta(&isnk_bo);
    test_reset_meta(&isnk_bc);

    /* Clear out buffers that are used by sinks */
    byte_buffer_clear(&isnk_bo.buffer);
    byte_buffer_clear(&isnk_bc.buffer);

    /* Fill buffers associated with sources to a well know and simple pattern.
     * The octet source counts up, the chunk source counts down. This is to be
     * able to tell them apart by data as well as behaviour. */
    byte_buffer_fillx(&isrc_bo.buffer, 0, 1);
    byte_buffer_repeat(&isrc_bo.buffer);

    byte_buffer_fillx(&isrc_bc.buffer, 0, -1);
    byte_buffer_repeat(&isrc_bc.buffer);
}

int
main(UNUSED int argc, UNUSED char **argv)
{
    ssize_t rc;
    Source src_o, src_c;
    Sink snk_o, snk_c;
    instrumentable_source(DATA_KIND_OCTET, &src_o, &isrc_bo);
    instrumentable_sink(DATA_KIND_OCTET,   &snk_o, &isnk_bo);
    instrumentable_source(DATA_KIND_CHUNK, &src_c, &isrc_bc);
    instrumentable_sink(DATA_KIND_CHUNK,   &snk_c, &isnk_bc);
    ByteBuffer aux = BYTE_BUFFER_EMPTY(auxb, AUX_SIZE);

    tap_init();
    test_reset();

    /*
     * Trivial endpoints first. source_empty returns nothing. source_zero
     * produces zeros only. sink_null accepts anything and throws it away.
     */

    rc = sts_n(&source_empty, &sink_null, 128u);
    ok(rc == -ENODATA, "empty->null returns -ENODATA (%zd)", rc);

    rc = sts_n(&source_zero, &sink_null, 128u);
    ok(rc == 128u, "zero->null times 128 works (%zd)", rc);

    rc = sts_n_aux(&source_zero, &sink_null, &aux, 128u);
    ok(rc == 128u, "zero->null through aux works (%zd)", rc);

    test_reset();
    rc = sts_n_aux(&source_zero, &snk_o, &aux, 128u);
    ok(rc == 128u, "zero->snk_o through aux works (%zd)", rc);
    {
        static unsigned char buf[128u];
        memset(buf, 0, 128);
        cmp_mem(isnk_bo.buffer.data, buf, 128,
                "zero actually produces zeroes");
    }

    test_reset();
    rc = sts_drain_aux(&src_o, &snk_o, &aux);
    ok(rc == SRC_SIZE, "drain: o->o through aux works (%zd)", rc);
    cmp_mem(isrc_bo.buffer.data, isnk_bo.buffer.data,
            isrc_bo.buffer.size, "drain: Source(o) and sink(o) memory match");

    test_reset();
    rc = sts_drain_aux(&src_o, &snk_c, &aux);
    ok(rc == SRC_SIZE, "drain: o->c through aux works (%zd)", rc);
    cmp_mem(isrc_bo.buffer.data, isnk_bc.buffer.data,
            isrc_bo.buffer.size, "drain: Source(o) and sink(c) memory match");

    test_reset();
    rc = sts_drain_aux(&src_c, &snk_o, &aux);
    ok(rc == SRC_SIZE, "drain: c->o through aux works (%zd)", rc);
    cmp_mem(isrc_bc.buffer.data, isnk_bo.buffer.data,
            isrc_bc.buffer.size, "drain: Source(c) and sink(o) memory match");

    test_reset();
    rc = sts_drain_aux(&src_c, &snk_c, &aux);
    ok(rc == SRC_SIZE, "drain: c->c through aux works (%zd)", rc);
    cmp_mem(isrc_bc.buffer.data, isnk_bc.buffer.data,
            isrc_bc.buffer.size, "drain: Source(c) and sink(c) memory match");

    noplan();
    return EXIT_SUCCESS;
}
