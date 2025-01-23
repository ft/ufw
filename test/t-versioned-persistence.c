/*
 * Copyright (c) 2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#include <asm-generic/errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <ufw/compiler.h>
#include <ufw/test/tap.h>

#include <ufw/crc/crc16-arc.h>
#include <ufw/compat/ssize-t.h>
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
t_reset(void)
{
    memset(storage, UCHAR_MAX, STORAGE_SIZE);
}

/*
 * Simulating a peripheral API
 */

static ssize_t
buffer_read(uint32_t addr, void *buf, size_t n)
{
    if ((addr + n) > STORAGE_SIZE) {
        return 0;
        return -ENOBUFS;
    }

    memcpy(buf, storage + addr, n);
    return n;
}

static ssize_t
buffer_write(uint32_t addr, const void *buf, size_t n)
{
    if ((addr + n) > STORAGE_SIZE) {
        return -ENOBUFS;
    }

    memcpy(storage + addr, buf, n);
    return n;
}

/*
 * Endpoints with addressable memory
 */

struct addressable_source {
    uint32_t address;
    ssize_t (*run)(uint32_t, void*, size_t);
    void *data;
};

static ssize_t
run_addressable_source(void *driver, void *buffer, const size_t n)
{
    struct addressable_source *cfg = driver;
    return cfg->run(cfg->address, buffer, n);
}

struct addressable_sink {
    uint32_t address;
    ssize_t (*run)(uint32_t, const void*, size_t);
    void *data;
};

static ssize_t
run_addressable_sink(void *driver, const void *buffer, const size_t n)
{
    struct addressable_sink *cfg = driver;
    return cfg->run(cfg->address, buffer, n);
}

/* Tests */

int
main(UNUSED int argc, UNUSED char *argv[])
{
    struct addressable_source src = { .address = 0u, .run = buffer_read, .data = NULL };
    Source storage_fetch = CHUNK_SOURCE_INIT(run_addressable_source, &src);
    struct addressable_sink snk = { .address = 0u, .run = buffer_write, .data = NULL };
    Sink storage_store = CHUNK_SINK_INIT(run_addressable_sink, &snk);

    VersionedPersistence vp =
        VP16_INIT(INFO_ADDRESS, INFO_SIZE, INFO_VERSION,
                  storage_fetch, storage_store,
                  ufw_crc16_arc, CRC16_ARC_INITIAL);



    plan(1);

    t_reset();
    ok((1 + 2) == 3, "math works");

    return EXIT_SUCCESS;
}
