/*
 * Copyright (c) 2017-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#ifndef INC_UFW_RING_BUFFER_H
#define INC_UFW_RING_BUFFER_H

/**
 * @addtogroup ringbuffer Ringbuffer
 *
 * Macro-polymorphic ring-buffer implementation
 *
 * @{
 */

/**
 * @file ring-buffer.h
 * @brief Polymorphic ring-buffer implementation
 */

/**
 * @}
 */

/* __cplusplus note: This file is macro-only, so we don't need the extern C
 * block in this header. */

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

/* Polymorphic ring-buffer implementation */

#define RING_BUFFER_API(NAME,TYPE)              \
    RING_BUFFER_TYPE__(NAME,TYPE)               \
    RING_BUFFER_INIT_API__(NAME,TYPE)           \
    RING_BUFFER_SIZE_API__(NAME)                \
    RING_BUFFER_EMPTY_API__(NAME)               \
    RING_BUFFER_FULL_API__(NAME)                \
    RING_BUFFER_CLEAR_API__(NAME)               \
    RING_BUFFER_GET_API__(NAME,TYPE)            \
    RING_BUFFER_PUT_API__(NAME,TYPE)            \
    RING_BUFFER_OVERRIDE_API__(NAME)

#define RING_BUFFER(NAME,TYPE)                  \
    STATIC_RING_BUFFER_ADVANCE_HEAD__(NAME)     \
    STATIC_RING_BUFFER_ADVANCE_TAIL__(NAME)     \
    RING_BUFFER_INIT__(NAME,TYPE)               \
    RING_BUFFER_SIZE__(NAME)                    \
    RING_BUFFER_EMPTY__(NAME)                   \
    RING_BUFFER_FULL__(NAME)                    \
    RING_BUFFER_CLEAR__(NAME)                   \
    RING_BUFFER_GET__(NAME,TYPE)                \
    RING_BUFFER_PUT__(NAME,TYPE)                \
    RING_BUFFER_OVERRIDE__(NAME)

#define RING_BUFFER_TYPE__(NAME,TYPE)           \
    typedef struct {                            \
        TYPE *data;                             \
        size_t head;                            \
        size_t tail;                            \
        size_t datasize;                        \
        bool override_if_full;                  \
    } NAME;

#define RING_BUFFER_INIT_API__(NAME,TYPE)       \
    void NAME##_init(NAME *, TYPE *, size_t);

#define RING_BUFFER_INIT__(NAME,TYPE)                   \
    void                                                \
    NAME##_init(NAME *c, TYPE *buf, size_t size)        \
    {                                                   \
        assert(c != NULL);                              \
                                                        \
        c->data = buf;                                  \
        c->datasize = size;                             \
        c->head = 0u;                                   \
        c->tail = size;                                 \
        c->override_if_full = false;                    \
                                                        \
        for (size_t i = 0; i < size; ++i)               \
            c->data[i] = 0u;                            \
    }

#define STATIC_RING_BUFFER_ADVANCE_HEAD__(NAME) \
    static inline void                          \
    NAME##_advance_head(NAME *);                \
    static inline void                          \
    NAME##_advance_head(NAME *c)                \
    {                                           \
        c->head = (c->head + 1) % c->datasize;  \
    }

#define STATIC_RING_BUFFER_ADVANCE_TAIL__(NAME) \
    static inline void                          \
    NAME##_advance_tail(NAME *);                \
    static inline void                          \
    NAME##_advance_tail(NAME *c) {              \
        c->tail = (c->tail + 1) % c->datasize;  \
                                                \
        if (c->tail == c->head)                 \
            c->tail = c->datasize;              \
    }

#define RING_BUFFER_GET_API__(NAME,TYPE)        \
    TYPE NAME##_get(NAME *);

#define RING_BUFFER_GET__(NAME,TYPE)            \
    TYPE                                        \
    NAME##_get(NAME *c)                         \
    {                                           \
        if (NAME##_empty(c))                    \
            return 0u;                          \
                                                \
        TYPE rc = c->data[c->tail];             \
        NAME##_advance_tail(c);                 \
        return rc;                              \
    }

#define RING_BUFFER_PUT_API__(NAME,TYPE)        \
    void NAME##_put(NAME *, TYPE);

#define RING_BUFFER_PUT__(NAME,TYPE)            \
    void                                        \
    NAME##_put(NAME *c, TYPE item)              \
    {                                           \
        if (NAME##_full(c)) {                   \
            if (c->override_if_full)            \
                NAME##_advance_tail(c);         \
            else                                \
                return;                         \
        }                                       \
                                                \
        if (NAME##_empty(c))                    \
            c->tail = c->head;                  \
                                                \
        c->data[c->head] = item;                \
        NAME##_advance_head(c);                 \
    }

#define RING_BUFFER_EMPTY_API__(NAME)           \
    bool NAME##_empty(const NAME *);

#define RING_BUFFER_EMPTY__(NAME)               \
    bool                                        \
    NAME##_empty(const NAME *c)                 \
    {                                           \
        return (c->tail == c->datasize);        \
    }

#define RING_BUFFER_FULL_API__(NAME)            \
    bool NAME##_full(const NAME *);

#define RING_BUFFER_FULL__(NAME)                \
    bool                                        \
    NAME##_full(const NAME *c)                  \
    {                                           \
        return (c->head == c->tail);            \
    }

#define RING_BUFFER_SIZE_API__(NAME)            \
    size_t NAME##_size(const NAME *);

#define RING_BUFFER_SIZE__(NAME)                        \
    size_t                                              \
    NAME##_size(const NAME *c)                          \
    {                                                   \
        if (NAME##_empty(c))                            \
            return 0u;                                  \
                                                        \
        if (c->tail < c->head)                          \
            return (c->head - c->tail);                 \
                                                        \
        return ((c->datasize - c->tail) + c->head);     \
    }

#define RING_BUFFER_CLEAR_API__(NAME)           \
    void NAME##_clear(NAME *);

#define RING_BUFFER_CLEAR__(NAME)               \
    void                                        \
    NAME##_clear(NAME *c)                       \
    {                                           \
        c->tail = c->datasize;                  \
    }

#define RING_BUFFER_OVERRIDE_API__(NAME)        \
    void NAME##_override_if_full(NAME *, bool);

#define RING_BUFFER_OVERRIDE__(NAME)            \
    void                                        \
    NAME##_override_if_full(NAME *c,            \
                            bool state)         \
    {                                           \
        c->override_if_full = state;            \
    }

#endif /* INC_UFW_RING_BUFFER_H */
