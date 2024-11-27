#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <ufw/compat/errno.h>

#include <ufw/byte-buffer.h>
#include <ufw/compiler.h>
#include <ufw/endpoints.h>

#include <ufw/test/tap.h>

struct epr_data {
    bool do_init;
    unsigned int count;
    unsigned int retries;
};

#define EPR_DATA {       \
    .do_init = true,     \
    .count = 0u,         \
    .retries = 0u }

static void
retry_init(UNUSED DataKind kind, struct ufw_ep_retry *epr)
{
    struct epr_data *data = epr->data;
    printf("# %s: do_init: %s\n", __func__,
           data->do_init ? "true" : "false");
    if (data->do_init) {
        printf("# %s: old_count: %u\n", __func__, data->count);
        data->count = 0u;
    }
}

static ssize_t
retry_run(UNUSED DataKind kind, UNUSED void *drv, void *vdata, ssize_t rc)
{
    struct epr_data *data = vdata;
    printf("# %s: do_init: %u++ retries: %u\n",
           __func__, data->count, data->retries);
    data->count++;
    if (data->count > data->retries) {
        return rc;
    }
    return 1;
}

#define SRC_SIZE (1024ul)
#define SNK_SIZE (2*1024ul)
static unsigned char srcb[SRC_SIZE];
static unsigned char snkb[SNK_SIZE];

#define AUX_SIZE (128u)
static unsigned char auxb[AUX_SIZE];

static InstrumentableBuffer isrc = INSTRUMENTABLE_BUFFER(srcb, SRC_SIZE);
static InstrumentableBuffer isnk = INSTRUMENTABLE_BUFFER(snkb, SNK_SIZE);

static struct epr_data retry_data = EPR_DATA;
static Source src;

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
    test_reset_meta(&isrc);
    test_reset_meta(&isnk);

    /* Clear out buffers that are used by sinks */
    byte_buffer_clear(&isnk.buffer);

    /* Fill buffers associated with sources to a well know and simple pattern.
     * The octet source counts up, the chunk source counts down. This is to be
     * able to tell them apart by data as well as behaviour. */
    byte_buffer_fillx(&isrc.buffer, 0, 1);
    byte_buffer_repeat(&isrc.buffer);

    /* Setup instrumentable source */
    instrumentable_source(DATA_KIND_CHUNK, &src, &isrc);
    src.retry.init = retry_init;
    src.retry.run = retry_run;
    src.retry.data = &retry_data;
    isrc.buffer.used = 256u;
}

int
main(UNUSED int argc, UNUSED char **argv)
{
    ssize_t rc;
    ByteBuffer aux = BYTE_BUFFER_EMPTY(auxb, AUX_SIZE);

    plan(8);

    /* Let the thing run without error indication. Our endpoints do not
     * implement the get_buffer extension, so sts_n will fall back to
     * byte-for-byte transfers. So the source gets run 128 times here. */
    test_reset();
    rc = sts_n(&src, &sink_null, 128u);
    ok(rc == 128u, "Trivial source runs");
    ok(isrc.read.stat.accesses == 128u,
       "sts_n without buffer extension runs 128 times");

    /* If we're not handling EAGAIN, the endpoint handler will do it for us. It
     * will just rerun the source until it gets a value. We must instruct our
     * source to do this. If we just blindly return EAGAIN, we end up in an
     * infinite loop. chunk=8, until=16 runs the source three times until it
     * succeeds. */
    test_reset();
    instrumentable_until_success_at(&isrc.read.error, 16u, -EAGAIN);
    instrumentable_chunksize(&isrc, 8u);
    instrumentable_reset_stats(&isrc.read.stat);
    rc = sts_n_aux(&src, &sink_null, &aux, 128u);
    ok(rc == 128, "Source ran until success");
    ok(isrc.read.stat.accesses == 16u + 128u / 8u, "Source ran %zu times",
       16u + 128u / 8u);
    ok(retry_data.count == 0u, "Retry runner did not run");

    /* Now lets run until we catch an error. Namely EAGAIN. This configures the
     * retry runner to retry two times before giving up, running the retry run-
     * ner three times. */
    test_reset();
    byte_buffer_rewind(&isrc.buffer);
    instrumentable_until_error_at(&isrc.read.error, 16u, -EAGAIN);
    instrumentable_chunksize(&isrc, 8u);
    instrumentable_reset_stats(&isrc.read.stat);
    src.retry.ctrl = EP_RETRY_CTRL_EAGAIN;
    retry_data.retries = 2u;
    rc = sts_n_aux(&src, &sink_null, &aux, 128u);
    ok(rc == -EAGAIN, "Error code gets forwarded");
    ok(isrc.read.stat.accesses == 3u, "Source ran three times");
    ok(retry_data.count == 3u, "Retry runner ran two times");

    return EXIT_SUCCESS;
}
