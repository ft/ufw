/*
 * Copyright (c) 2021-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#ifndef INC_UFW_CONVOLUTION_LOW_PASS_H
#define INC_UFW_CONVOLUTION_LOW_PASS_H

/**
 * @addtogroup clpfilter Convolution Low Pass Filters
 *
 * Macro-polymorphic low pass filters
 *
 * @{
 *
 * @file ufw/convolution-low-pass.h
 * @brief Convolution Low Pass Filters
 *
 * `__cplusplus` note: This file is macro-only, so we don't need the extern C
 * block in this header.
 *
 * @}
 */


#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/*
 * We need to disable this clang-tidy test in here. It does not play well with
 * types in polymorphic macros, and we're not marking all places manually.
 *
 * NOLINTBEGIN(bugprone-macro-parentheses)
 */

/**
 * Implement sliding-window convolution filters.
 *
 * The operations +, =, /, must be implemented for the desired TYPE.
 * Also, the sum of the all elements must be ensured to never exceed TYPE's
 * range limitations.
 */
#define CONV_LOW_PASS(NAME, TYPE)                                             \
    CONV_LOW_PASS_INIT__(NAME, TYPE)                                          \
    CONV_LOW_PASS_UPDATE__(NAME, TYPE)

#define CONV_LOW_PASS_API(NAME, TYPE)                                         \
    CONV_LOW_PASS_TYPE__(NAME, TYPE)                                          \
    CONV_LOW_PASS_INIT_API__(NAME, TYPE)                                      \
    CONV_LOW_PASS_UPDATE_API__(NAME, TYPE)                                    \
    CONV_LOW_PASS_GETTERS__(NAME, TYPE)

#define CONV_LOW_PASS_TYPE__(NAME, TYPE)                                      \
    typedef struct {                                                          \
        TYPE *win;                                                            \
        unsigned int len;                                                     \
        TYPE avg;                                                             \
        bool first;                                                           \
        unsigned int cur;                                                     \
    } NAME;

#define CONV_LOW_PASS_INIT_API__(NAME, TYPE) \
    void NAME##_init(NAME *, TYPE *, unsigned int);

#define CONV_LOW_PASS_INIT__(NAME, TYPE)                                      \
    void NAME##_init(NAME *w, TYPE *buffer, unsigned int len) {               \
        w->win = buffer;                                                      \
        w->len = len;                                                         \
        for (unsigned int i = 0; i < w->len; ++i)                             \
            w->win[i] = (TYPE)0;                                              \
        w->first = true;                                                      \
        w->cur = 0;                                                           \
        w->avg = (TYPE)0;                                                     \
    }

#define CONV_LOW_PASS_GETTERS__(NAME, TYPE)                                   \
    inline static TYPE NAME##_avg(NAME *w) {                                  \
        return w->avg;                                                        \
    }                                                                         \
                                                                              \
    inline static bool NAME##_has_min_values(NAME *w, unsigned int c) {       \
        if (c > w->len)                                                       \
            return false;                                                     \
        if (w->first)                                                         \
            return w->cur >= c;                                               \
        else                                                                  \
            return true;                                                      \
    }

#define CONV_LOW_PASS_UPDATE_API__(NAME, TYPE)                                \
    void NAME##_update(NAME *, TYPE);

#define CONV_LOW_PASS_UPDATE__(NAME, TYPE)                                    \
                                                                              \
    void NAME##_update(NAME *w, TYPE value) {                                 \
        TYPE sum = (TYPE)0;                                                   \
                                                                              \
        const unsigned int div = (w->first) ? (w->cur + 1) : (w->len);        \
                                                                              \
        w->win[w->cur] = value;                                               \
        w->cur++;                                                             \
        if (w->cur >= w->len)                                                 \
                w->first = false;                                             \
        w->cur %= w->len;                                                     \
                                                                              \
        for (unsigned int i = 0; i < div; ++i)                                \
            sum += w->win[i];                                                 \
                                                                              \
        w->avg = (TYPE)(sum / (TYPE)(div == 0 ? 1 : div));                    \
    }


#define CONV_LOW_PASS_MEDIAN_API(NAME, TYPE) \
    TYPE NAME##_median(NAME *, TYPE *);

/**
 * Implement moving median resolution based on convolution buffer.
 *
 * Provide a source for standard qsort for this (as found in stdlib.h).
 * TYPE must be directly comparable with <, >, ==.
 */
#define CONV_LOW_PASS_MEDIAN(NAME, TYPE)                                      \
    static int NAME##_compare(void const *left, void const *right) {          \
        const TYPE lv = *(TYPE const *)left;                                  \
        const TYPE rv = *(TYPE const *)right;                                 \
        if (lv < rv)                                                          \
            return -1;                                                        \
        else if (lv == rv)                                                    \
            return 0;                                                         \
        else                                                                  \
            return 1;                                                         \
    }                                                                         \
                                                                              \
    TYPE NAME##_median(NAME *w, TYPE *tmp) {                                  \
        const unsigned int in_use = (w->first) ? (w->cur) : (w->len);         \
        if (in_use == 0)                                                      \
            return 0;                                                         \
                                                                              \
        memcpy(tmp, w->win, in_use * sizeof(TYPE));                           \
        qsort(tmp, in_use, sizeof(TYPE), NAME##_compare);                     \
                                                                              \
        if ((in_use % 2) == 1)                                                \
            return tmp[in_use / 2];                                           \
        else                                                                  \
            return (tmp[in_use / 2] + tmp[(in_use / 2) - 1]) / 2;             \
    }

/* NOLINTEND(bugprone-macro-parentheses) */

#endif /* INC_UFW_CONVOLUTION_LOW_PASS_H */
