/*
 * Copyright (c) 2022 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#ifndef INC_UFW_SOURCES_AND_SINKS_H
#define INC_UFW_SOURCES_AND_SINKS_H

#include <stddef.h>

#include <ufw/compat/ssize-t.h>

#include <ufw/toolchain.h>
#include <ufw/types.h>

/*
 * Generic Sources and Sinks
 */

extern Source source_zero;
extern Sink sink_null;

/*
 * File descriptor based Sources and Sinks
 */

#ifdef UFW_HAVE_POSIX_READ
ssize_t run_read(void*, void*, size_t);
void source_from_filedesc(Source*, int*);
#endif /* UFW_HAVE_POSIX_READ */

#ifdef UFW_HAVE_POSIX_WRITE
ssize_t run_write(void*, const void*, size_t);
void sink_to_filedesc(Sink*, int*);
#endif /* UFW_HAVE_POSIX_WRITE */

/*
 * Buffer based Sources and Sinks
 */

void source_from_buffer(Source*, OctetBuffer*);
void sink_to_buffer(Sink*, OctetBuffer*);

#endif /* INC_UFW_SOURCES_AND_SINKS_H */
