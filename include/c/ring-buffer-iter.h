#ifndef INC_RING_BUFFER_ITER_H
#define INC_RING_BUFFER_ITER_H

#include <stdbool.h>
#include <stddef.h>

#include "c/ring-buffer.h"

/**
 * @addtogroup ufw
 * @{
 * @addtogroup ringbuffer
 * @{
 */

typedef enum {
    RING_BUFFER_ITER_OLD_TO_NEW,
    RING_BUFFER_ITER_NEW_TO_OLD
} rb_iter_mode;

typedef struct {
    size_t steps;
    size_t index;
    size_t size;
    rb_iter_mode mode;
} rb_iter;

/* _done and _advance are target type agnostic */

bool rb_iter_done(rb_iter *);
size_t rb_iter_advance(rb_iter *);

/* Construction and inspection are not; polymorphic macros */

#define RING_BUFFER_ITER_API(NAME,TYPE)         \
    RING_BUFFER_ITER_CONSTRUCT_API__(NAME)      \
    RING_BUFFER_ITER_INSPECT_API__(NAME,TYPE)

#define RING_BUFFER_ITER(NAME,TYPE)             \
    RING_BUFFER_ITER_CONSTRUCT__(NAME,TYPE)

#define RING_BUFFER_ITER_CONSTRUCT_API__(NAME)  \
    void NAME##_iter(rb_iter *, NAME *,         \
                     rb_iter_mode);

#define RING_BUFFER_ITER_CONSTRUCT__(NAME,TYPE)                 \
    void                                                        \
    NAME##_iter(rb_iter *iter, NAME *c,                         \
                rb_iter_mode mode)                              \
    {                                                           \
        iter->size = c->datasize;                               \
        iter->mode = mode;                                      \
        iter->steps = NAME##_size(c);                           \
                                                                \
        switch (mode) {                                         \
        case RING_BUFFER_ITER_OLD_TO_NEW:                       \
            iter->index = c->tail;                              \
            break;                                              \
        case RING_BUFFER_ITER_NEW_TO_OLD:                       \
            iter->index =                                       \
                (c->head == 0) ? c->datasize - 1 : c->head - 1; \
            break;                                              \
        }                                                       \
    }

#define RING_BUFFER_ITER_INSPECT_API__(NAME,TYPE)       \
    static inline TYPE                                  \
    NAME##_inspect(NAME *c,                             \
                   rb_iter *iter)                       \
    {                                                   \
        return c->data[iter->index];                    \
    }

/**
 * @}
 * @}
 */

#endif /* INC_RING_BUFFER_ITER_H */
