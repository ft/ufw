/*
 * Copyright (c) 2022 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file posix-source-sink.c
 * @brief Sources and sinks implementable on POSIX systems
 */

#include <ufw/toolchain.h>

#ifdef WITH_UNISTD_H
#include <unistd.h>
#endif /* WITH_UNISTD_H */

#include <ufw/compat/errno.h>
#include <ufw/compat/ssize-t.h>

#include <ufw/types.h>
#include <ufw/sources-and-sinks.h>

#ifdef UFW_HAVE_POSIX_READ
ssize_t
run_read(void *driver, void *data, size_t n)
{
    int *fd = driver;
    const int rc = read(*fd, data, n);
    return (rc < 0) ? -errno : rc;
}

void
source_from_filedesc(Source *instance, int *fd)
{
    chunk_source_init(instance, run_read, fd);
}
#endif /* UFW_HAVE_POSIX_READ */

#ifdef UFW_HAVE_POSIX_WRITE
ssize_t
run_write(void *driver, const void *data, size_t n)
{
    int *fd = driver;
    const int rc = write(*fd, data, n);
    return (rc < 0) ? -errno : rc;
}

void
sink_to_filedesc(Sink *instance, int *fd)
{
    chunk_sink_init(instance, run_write, fd);
}
#endif /* UFW_HAVE_POSIX_WRITE */
