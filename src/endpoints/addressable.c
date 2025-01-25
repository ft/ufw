/*
 * Copyright (c) 2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file addressable.c
 * @brief Addressable endpoint implementation
 */

#include <ufw/endpoints.h>

ssize_t
run_addressable_source(void *driver, void *buffer, const size_t n)
{
    struct addressable_source *cfg = driver;
    return cfg->run(cfg->data, cfg->address, buffer, n);
}

ssize_t
run_addressable_sink(void *driver, const void *buffer, const size_t n)
{
    struct addressable_sink *cfg = driver;
    return cfg->run(cfg->data, cfg->address, buffer, n);
}

#if 0
ssize_t
addrsource_get_chunk(Source *source, const uint32_t addr, void *buf, size_t n)
{
    AddressableSource *cfg = source->data;
    cfg->address = addr;
}
#endif
