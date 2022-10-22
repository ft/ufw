/*
 * Copyright (c) 2022 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file endpoints/trivial.c
 * @brief Sources and sinks that are very simple to implement
 *
 * This implements a source like /dev/zero and a sink like /dev/null.
 */

#include <string.h>

#include <ufw/compiler.h>
#include <ufw/endpoints.h>

static ssize_t
run_source_zero(UNUSED void *driver, void *data, size_t n)
{
    memset(data, 0, n);
    return n;
}

static ssize_t
run_sink_null(UNUSED void *driver, UNUSED const void *data, size_t n)
{
    return n;
}

Source source_zero = CHUNK_SOURCE_INIT(run_source_zero, NULL);
Sink   sink_null   = CHUNK_SINK_INIT(  run_sink_null,   NULL);
