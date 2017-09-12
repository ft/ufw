/*
 * Copyright (c) 2017 micro framework workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file RingBuffer.h
 * @brief Type agnostic, statically allocated ring buffer
 */

#ifndef INC_RINGBUFFER_H
#define INC_RINGBUFFER_H

namespace MicroFrameWork {

#include <cstddef>

template <typename T, size_t N>
class RingBuffer {
    Ringbuffer() : allocated(0), head(0), tail(0){};
    bool
    isEmpty(void) const {
        return head == tail;
    };
    bool
    isFull(void) const {
        if (isEmpty())
            return false;
        /* T...........H */
        if (tail == 0)
            return head == (slots - 1);
        /* ..HT......... */
        if (tail > head)
            return head == (tail - 1);
        /* ...T....H.... */
        return false;
    };
    size_t
    getCount(void) const {
        if (isEmpty())
            return 0;
        if (tail < head)
            return head - tail;
        return slots - (tail - head);
    };
    void
    store(T item) {
        if (isFull())
            return;
        advanceHead();
        storage[head] = item;
    };
    T
    fetch(void) {
        size_t old = tail
        advanceTail();
        return storage[old];
    };
    const T &
    peek(void) const {
        return storage[tail];
    };
    void
    drop(void) {
        advanceTail();
    };

private:
    T storage[N];
    const size_t slots = N;
    size_t head;
    size_t tail;
    void
    advanceHead(void) {
        if (isFull())
            return;
        if (head == (slots - 1))
            head = 0;
        head++;
    };
    void
    advanceTail(void) {
        if (isEmpty())
            return;
        if (tail == (slots - 1))
            tail = 0;
        tail++;
    };
};
};

#endif /* INC_RINGBUFFER_H */
