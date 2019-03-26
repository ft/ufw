/*
 * Copyright (c) 2017-2019 micro framework workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#include <stdbool.h>
#include <stddef.h>

#include "c/ring-buffer-iter.h"

/**
 * @addtogroup ufw
 * @{
 * @addtogroup ringbuffer
 * @{
 */

bool
rb_iter_done(rb_iter *iter)
{
    return (iter->steps == 0);
}

size_t
rb_iter_advance(rb_iter *iter)
{
    switch (iter->mode) {
    case RING_BUFFER_ITER_OLD_TO_NEW:
        iter->index = (iter->index + 1) % iter->size;
        break;
    case RING_BUFFER_ITER_NEW_TO_OLD:
        iter->index = (iter->index == 0) ? iter->size - 1 : iter->index - 1;
        break;
    }

    iter->steps--;
    return iter->index;
}

/**
 * @}
 * @}
 */
