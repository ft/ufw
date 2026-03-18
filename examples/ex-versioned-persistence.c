/*
 * Copyright (c) 2024-2026 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file ex-versioned-persistence.c
 *
 * @brief Minimal example for VersionedPersistence
 *
 * This is an example how to set up a `VersionedPersistence` instance and
 *  its auxillary features to have a consistent and version -
 * compatibility checked persistent memory section in an EEPROM.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <ufw/compiler.h>
#include <ufw/crc/crc16-arc.h>
#include <ufw/endpoints.h>
#include <ufw/versioned-persistence.h>

struct PACKED configuration {
    int datum_a;
    int datum_b;
    bool flag;
};

#define SOME_DEFAULT_CONFIG {.datum_a = 1, .datum_b = 2, .flag = true}
#define PERSISTENCE_START_ADDRESS (0x2)
#define PERSISTENCE_SIZE (sizeof(struct configuration))
#define PERSISTENCE_VERSION (1)

/* Mock EEPROM memory which contains valid VersionedPersistence instance. */
static uint8_t mock_memory[PERSISTENCE_START_ADDRESS +
                           VP_SECTION_SIZE(PERSISTENCE_SIZE)] = {
    0x0,  0x0, 0xd6, 0x9a, 0x1,  0x0, 0x9, 0x0, 0x32, 0xb5,
    0x2a, 0x0, 0x0,  0x0,  0xff, 0x0, 0x0, 0x0, 0x1};

/**
 * `VersionedPersistence` use `Sink` and `Source` endpoints to handle
 * data streams First we need our generic read and write callback functions
 * to and from our persistent memory.
 */
static int
eeprom_read(void *buf, size_t offset, size_t len)
{
    memcpy(buf, &mock_memory[offset], len);
    return 0;
}

static int
eeprom_write(const void *buf, size_t offset, size_t len)
{
    memcpy(&mock_memory[offset], buf, len);
    return 0;
}

/*
 * As you can see these functions do not take an `offset` parameter as
 * I/O function normally do. Rather than that the `driver` function argument
 * allows us to pass user data associated with our endpoint.In this case we can
 * use it to specify a read/write head to the persistent memory block to allow
 *us to read and write data from any offset.
 */
static int endpoint_driver = 0;

/*
 * We now need to implement the endpoints `seek` callback which will set our
 * read/write head to the specified offset into the memory.
 * Note that this is the global offset from the start of the memory,
 * `VersionedPersistence` will handle all address calculations for sections
 * not starting at the beginning of the memory.
 */
static int
endpoint_seek(void *driver, size_t offset)
{
    *(int *)driver = offset;
    return 0;
}

/*
 *The userdata can now be used in the read/write functions. Here
 *`eeprom_read`/`eeprom_write` can be any I/O function provided by the driver
 *of your peristent memory or operating system.
 */
static ssize_t
endpoint_read(void *driver, void *buf, size_t len)
{
    int offset = *(int *)driver;

    int retval = eeprom_read(buf, offset, len);

    if (retval == 0) {
        return len;
    } else {
        return retval;
    }
}

static ssize_t
endpoint_write(void *driver, const void *buf, size_t len)
{
    int offset = *(int *)driver;

    int retval = eeprom_write(buf, offset, len);

    if (retval == 0) {
        return len;
    } else {
        return retval;
    }
}

/*
 *Now we are ready to setup our endpoints. There are two types of endpoints
 *`OCTET` for data streams, which are done octet by octet and `CHUNK`, which
 *support variable sized data chunks. Our EEPROM driver supports block read and
 *writes therefore we initialize with `CHUNK_SOURCE_INIT`. The provided
 *initializer macro does unfortunately not take the `seek` callback as a
 *paramter so we have to set it in some function before initializing the
 *`VersionedPersistence`.
 */

static Source persistence_source;
static Sink persistence_sink;

static inline void
init_endpoints(void)
{
    persistence_source =
        (Source)CHUNK_SOURCE_INIT(endpoint_read, &endpoint_driver);
    persistence_sink = (Sink)CHUNK_SINK_INIT(endpoint_write, &endpoint_driver);

    persistence_source.ext.seek = (EndpointSeek)endpoint_seek;
    persistence_sink.ext.seek = (EndpointSeek)endpoint_seek;
}

/*
 *We now have most of the scaffolding in place to start to create our
 *`VersionedPersistence`. Start by using the `VP_INIT` macro to define it. The
 *initialisation macro can also take in a ufw `ByteBuffer` as an auxillary
 *buffer to work with, but it is optional and not used by the
 *`VersionedPersistence` itself so we set it to `NULL` for simplicity.
 */

static VersionedPersistence persistent;

int
main(void)
{
    init_endpoints();

    persistent = (VersionedPersistence)VP_INIT(
        PERSISTENCE_START_ADDRESS, PERSISTENCE_SIZE, PERSISTENCE_VERSION, NULL,
        persistence_source, persistence_sink);
    vp_init(&persistent);

    /*
     *After initialising, the first thing we need to do is validate the
     *`VersionedPersistence` metadata headers and payloads checksums to make
     *sure the data from the EEPROM hasn't been	corrupted and also that the
     *version of the stored data is the the same as the firmware expects. To do
     *that we can call `vp_usable`.
     */

    if (vp_usable(&persistent) == false) {
        /* The most basic way to recover from unusable VersionedPersistence is
         * to just save some default data to it.
         * After calling vp_save, the VersionedPersistence is valid again.*/
        struct configuration default_config =
            (struct configuration)SOME_DEFAULT_CONFIG;
        vp_save(&persistent, (void *)&default_config);
    } else {
        printf("Data read from persistence is valid!\n");
    }
    /* After successfull validation or error handling we can now operate
     * on our persistent section.*/
    struct configuration some_device_config = {0};

    ssize_t retval = vp_fetch(&persistent, &some_device_config);
    if (retval <= 0) {
        printf("Error reading config from persistence: %ld\n", retval);
        return retval;
    }

    /* Now the firmware can work with the data from the persistent
     * section.
     * Expected output: "Datum A from VersionedPersistence is 42"*/
    printf("Datum A from VersionedPersistence is %d\n",
           some_device_config.datum_a);
    return 0;
}
