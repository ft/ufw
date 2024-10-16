/*
 * Copyright (c) 2023-2024 micro framework workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file continuable-sink.h
 * @brief API for continuable sinks
 *
 * A continuable sink is a type of Sink, interfacing a memory buffer, that
 * allows for the sink to still accept data, even if there is some sort of
 * problem with the associated buffer.
 *
 * This is useful when implementing communication protocols, when it is
 * desirable to be able to gracefully react to memory related issues, when
 * the remote side cannot immediately see that reaction.
 *
 * The main idea here is to use a BlockAllocator to fetch the main buffer
 * for the sink to operate on. If this allocation fails, or if this buffer
 * would be overflown, the sink makes a note of this and keeps accepting,
 * but discarding additional data.
 *
 * As an extension, it if possible to provide the sink with a fallback buf-
 * fer, that the sink can store some data in, even if the block allocation
 * failed.
 *
 * Allocation only happens once and that is the first time data arrives for
 * the sink to store. When allocation succeeds, a user specifiable callback
 * is called that allows further setup of the ByteBuffer before the sink
 * takes up its normal operation.
 *
 * If no allocator is provided, the sink will only operate on the fallback
 * buffer. If that is also not provided, the sink basically behaves like
 * the trivial "null" sink (see ufw/endpoints.h for details).
 */

#ifndef INC_UFW_ENDPOINTS_CONTINUABLE_SINK_H_11229582
#define INC_UFW_ENDPOINTS_CONTINUABLE_SINK_H_11229582

#include <stddef.h>
#include <stdint.h>

#include <ufw/allocator.h>
#include <ufw/byte-buffer.h>
#include <ufw/endpoints.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct ContinuableIssue {
    /*
     * Error number value to encode possible conditions:
     *
     * - EBUSY: Memory allocation failed. This causes the sink runner to store
     *   the beginning of the frame into the fallback buffer. Any data that
     *   does not fit into this buffer will be discarded until the end of the
     *   current frame is reached. When this errno value is retrieved by
     *   regp_recv(), it will send a EBUSY reponse to the remote side.
     *
     * - ENOMEM: The frame is too large to fit into the allocated buffer. When
     *   this happens, any data that cannot be store in the buffer be discarded
     *   until the end of the current frame is reached. With this errno value,
     *   regp_recv() will send a ERXOVERFLOW response to the remote side.
     *
     * If everything went well, this value should be zero.
     */
    int id;
    size_t datacount;
} ContinuableIssue;

typedef struct ContinuableSink {
    /* This is the allocator that will be used to allocate a fresh receive
     * buffer, for this sink to store data into. This may be NULL, in which
     * case it is obviously not used. In that case, some other memory has to
     * be linked to the frame byte buffer below. */
    BlockAllocator *alloc;
    /* An byte buffer to navigate the dynamically allocated buffer. */
    ByteBuffer buffer;
    /* This ByteBuffer will be linked to the static header store below. */
    ByteBuffer *fallback;
    /* A callback to run after allocation. This can be used to do any desired
     * setup of the ByteBuffer instance. This is called after the allocated
     * buffer is linked into the ByteBuffer. In our case, this will move the
     * buffer offset so we can store a RPFrame instance at the beginning of the
     * block. */
    void (*postalloc)(ByteBuffer*);
    ContinuableIssue error;
} ContinuableSink;

#define CONTINUABLE_SINK(ALLOC,FB,CB) {     \
        .alloc = (ALLOC),                   \
        .buffer = BYTE_BUFFER(NULL, 0u),    \
        .fallback = (FB),                   \
        .postalloc = (CB),                  \
        .error.id = 0,                      \
        .error.datacount = 0u               }

void continuable_sink_init(Sink*, ContinuableSink*);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* INC_UFW_ENDPOINTS_CONTINUABLE_SINK_H_11229582 */
