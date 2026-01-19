/*
 * Copyright (c) 2017-2026 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#include <stdbool.h>
#include <stddef.h>

#include "ufw/ring-buffer-iter.h"

/**
 * @addtogroup ringbuffer Ringbuffer
 * @{
 *
 * @file ring-buffer-iter.c
 * @brief Interators for ringbuffers from `ufw/ring-buffer.h`.
 *
 * @}
 */

bool
rb_iter_done(const rb_iter *iter)
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
    default:
        /* Should never happen; break if it does. */
        assert(false);
        break;
    }

    iter->steps--;
    return iter->index;
}
