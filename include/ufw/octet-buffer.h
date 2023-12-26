/*
 * Copyright (c) 2022 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#ifndef INC_UFW_OCTET_BUFFER_H
#define INC_UFW_OCTET_BUFFER_H

#include <stddef.h>

#include <ufw/compat/ssize-t.h>

typedef struct ufw_octet_buffer {
    unsigned char *data;
    size_t size;
    size_t used;
    size_t offset;
} OctetBuffer;

#define OCTET_BUFFER_INIT(DATA, SIZE, LENGTH, OFFSET) { \
        .data = DATA,                                   \
        .size = SIZE,                                   \
        .used = LENGTH,                                 \
        .offset = OFFSET }

int octet_buffer_set(OctetBuffer*, void*, size_t, size_t, size_t);
void octet_buffer_null(OctetBuffer*);

int octet_buffer_use(OctetBuffer*, void*, size_t);
int octet_buffer_space(OctetBuffer*, void*, size_t);

int octet_buffer_add(OctetBuffer*, const void*, size_t);
int octet_buffer_consume(OctetBuffer*, void*, size_t);
ssize_t octet_buffer_consume_at_most(OctetBuffer*, void*, size_t);

int octet_buffer_rewind(OctetBuffer*);
void octet_buffer_clear(OctetBuffer*);
void octet_buffer_repeat(OctetBuffer*);

size_t octet_buffer_avail(OctetBuffer*);
size_t octet_buffer_rest(OctetBuffer*);

#endif /* INC_UFW_OCTET_BUFFER_H */
