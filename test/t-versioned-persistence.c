/*
 * Copyright (c) 2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ufw/compiler.h>
#include <ufw/test/tap.h>

#include <ufw/compat/errno.h>
#include <ufw/compat/ssize-t.h>
#include <ufw/crc/crc16-arc.h>
#include <ufw/endpoints.h>
#include <ufw/versioned-persistence.h>

/*
 * Test setup
 */

#define STORAGE_SIZE 1024

struct info_data {
    uint32_t a;
    uint32_t b;
    uint16_t c;
    uint16_t d;
    uint32_t e;
    uint16_t f;
    uint32_t g;
    uint32_t h;
    uint16_t i;
    uint32_t j;
};

#define INFO_DATA_DEFAULT { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }
#define INFO_SIZE    (sizeof(struct info_data))
#define INFO_ADDRESS 24u
#define INFO_VERSION  5u

static unsigned char storage[STORAGE_SIZE];

static void
t_reset(VersionedPersistence *vp)
{
    memset(storage, UCHAR_MAX, STORAGE_SIZE);
    vp_invalidate(vp, VP_DATA_META | VP_DATA_PAYLOAD);
#ifdef TEST_DEBUG
    thexdump(storage, STORAGE_SIZE);
#endif /* TEST_DEBUG */
}

/*
 * Simulating a peripheral API
 */

static ssize_t
buffer_read(UNUSED void *data, uint32_t addr, void *buf, size_t n)
{
    printf("# DEBUG: read()\n");
    if ((addr + n) > STORAGE_SIZE) {
        return -ENOBUFS;
    }

    memcpy(buf, storage + addr, n);
    return n;
}

static ssize_t
buffer_write(UNUSED void *data, uint32_t addr, const void *buf, size_t n)
{
    if ((addr + n) > STORAGE_SIZE) {
        return -ENOBUFS;
    }

    memcpy(storage + addr, buf, n);
    return n;
}

/* Tests */

int
main(UNUSED int argc, UNUSED char *argv[])
{
    struct addressable_source src = { .address = 0U, .run = buffer_read, .data = NULL };
    Source storage_fetch = CHUNK_SOURCE_INIT(run_addressable_source, &src);
    struct addressable_sink snk = { .address = 0U, .run = buffer_write, .data = NULL };
    Sink storage_store = CHUNK_SINK_INIT(run_addressable_sink, &snk);

    VersionedPersistence vp =
        VP_SIMPLE_INIT(INFO_ADDRESS, INFO_SIZE, INFO_VERSION,
                       storage_fetch, storage_store);


    plan(1);

    t_reset(&vp);

    const int openrc = vp_open(&vp);
    ok(openrc == -EBADF, "After t_reset, vp cannot be opened: Meta data error.");

    return EXIT_SUCCESS;
}
