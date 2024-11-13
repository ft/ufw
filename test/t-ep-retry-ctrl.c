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

#define EPR_DATA ((struct epr_data){            \
    .do_init = true,                            \
    .count = 0u,                                \
    .retries = 0u })

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

#define BS (1024u * 1024u)
static unsigned char b[BS];
#define ABS (BS >> 8u)
static unsigned char auxb[ABS];

int
main(UNUSED int argc, UNUSED char **argv)
{
    ssize_t rc;
    struct epr_data retry_data = EPR_DATA;
    Source src;
    InstrumentableBuffer src_buffer = INSTRUMENTABLE_BUFFER(b, BS);
    instrumentable_source(DATA_KIND_CHUNK, &src, &src_buffer);
    src.retry.init = retry_init;
    src.retry.run = retry_run;
    src.retry.data = &retry_data;
    src_buffer.chunksize = 5u;
    src_buffer.buffer.used = 256u;
    ByteBuffer aux = BYTE_BUFFER_EMPTY(auxb, ABS);

    plan(8);

    /* Let the thing run without error indication. Our endpoints do not
     * implement the get_buffer extension, so sts_n will fall back to
     * byte-for-byte transfers. So the source gets run 128 times here. */
    rc = sts_n(&src, &sink_null, 128u);
    ok(rc == 128u, "Trivial source runs");
    ok(src_buffer.read.stat.accesses == 128u,
       "sts_n without buffer extension runs 128 times");

    /* If we're not handling EAGAIN, the endpoint handler will do it for us. It
     * will just rerun the source until it gets a value. We must instruct our
     * source to do this. If we just blindly return EAGAIN, we end up in an
     * infinite loop. chunk=8, until=16 runs the source three times until it
     * succeeds. */
    instrumentable_until_success_at(&src_buffer.read.error, 16u, -EAGAIN);
    instrumentable_chunksize(&src_buffer, 8u);
    instrumentable_reset_stats(&src_buffer.read.stat);
    rc = sts_n_aux(&src, &sink_null, &aux, 128u);
    ok(rc == 128, "Source ran until success");
    ok(src_buffer.read.stat.accesses == 3u, "Source ran three times");
    ok(retry_data.count == 0u, "Retry runner did not run");

    /* Now lets run until we catch an error. Namely EAGAIN. This configures the
     * retry runner to retry two times before giving up, running the retry run-
     * ner three times. */
    byte_buffer_rewind(&src_buffer.buffer);
    instrumentable_until_error_at(&src_buffer.read.error, 16u, -EAGAIN);
    instrumentable_chunksize(&src_buffer, 8u);
    instrumentable_reset_stats(&src_buffer.read.stat);
    src.retry.ctrl = EP_RETRY_CTRL_EAGAIN;
    retry_data.retries = 2u;
    rc = sts_n_aux(&src, &sink_null, &aux, 128u);
    printf("# DEBUG: %zd\n", rc);
    ok(rc == -EAGAIN, "Error code gets forwarded");
    ok(src_buffer.read.stat.accesses == 3u, "Source ran three times");
    ok(retry_data.count == 3u, "Retry runner ran two times");

    return EXIT_SUCCESS;
}
