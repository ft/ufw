/*
 * Copyright (c) 2023-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @addtogroup endpoints Endpoints
 * @{
 */

/**
 * @file continuable-sink.c
 * @brief Implementation for continuable sinks
 */

/**
 * @}
 */

#include <ufw/allocator.h>
#include <ufw/compat/errno.h>
#include <ufw/endpoints.h>
#include <ufw/endpoints/continuable-sink.h>

static int
cs_add(ContinuableSink *cs, const void *data, const size_t n)
{
    ByteBuffer *b = cs->buffer.data != NULL ? &cs->buffer : cs->fallback;
    if (b == NULL) {
        return -ENOMEM;
    }
    const size_t rest = byte_buffer_rest(b);
    const size_t tosave = n < rest ? n : rest;
    byte_buffer_add(b, data, tosave);
    return tosave < n ? -ENOMEM : 0;
}

static ssize_t
run_continuable_sink(void *driver, const void *data, size_t n)
{
    ContinuableSink *cs = driver;

    if (cs->error.id != 0) {
        /* If there's still room in a buffer, save as much as possible, but
         * we're already in a bad state. */
        (void)cs_add(cs, data, n);
        cs->error.datacount += n;
        return n;
    }

    if (cs->alloc == NULL && cs->fallback == NULL) {
        /* No buffers? Can't do much with that. */
        cs->error.id = ENOMEM;
        cs->error.datacount += n;
        return n;
    }

    if (cs->buffer.data == NULL && cs->alloc != NULL) {
        /* We do have an allocator, but no allocated buffer. If allocation
         * would have failed previously, we wouldn't be here, because error.id
         * would be set to something non-zero. That means, this is the first
         * (and only) time we're trying to allocate the buffer. */
        void *buf;
        int rc = block_alloc(cs->alloc, &buf);
        if (rc < 0) {
            /* Allocation failed. Make a note of that... */
            cs->error.id = EBUSY;
            cs->error.datacount = n;
            cs->buffer.data = NULL;
            /* ...and try to save as much data as possible in the fallback
             * buffer, if we have one. */
            (void)cs_add(cs, data, n);
            return n;
        }
        /* Allocation succeeded. Call postalloc if we have it. */
        cs->buffer.data = buf;
        cs->buffer.size = cs->alloc->blocksize;
        if (cs->postalloc != NULL) {
            cs->postalloc(&cs->buffer);
        }
    }

    /* In normal operation, we're here. */
    const size_t m = cs->buffer.data != NULL
        ? cs->buffer.used
        : (cs->fallback != NULL
           ? cs->fallback->used
           : 0u);
    const int rc = cs_add(cs, data, n);

    if (rc < 0) {
        cs->error.id = ENOMEM;
        cs->error.datacount = m+n;
    }

    return n;
}

void
continuable_sink_init(Sink *instance, ContinuableSink *driver)
{
    driver->buffer.data = NULL;
    driver->buffer.size = 0u;
    driver->buffer.offset = 0u;
    driver->buffer.used = 0u;
    driver->error.id = 0u;
    driver->error.datacount = 0u;
    chunk_sink_init(instance, run_continuable_sink, driver);
}
