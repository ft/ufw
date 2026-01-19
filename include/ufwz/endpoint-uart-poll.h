/*
 * Copyright (c) 2024-2026 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#ifndef INC_INCLUDE_UFWZ_ENDPOINT_UART_POLL_H_0c903589
#define INC_INCLUDE_UFWZ_ENDPOINT_UART_POLL_H_0c903589

#include <stdint.h>

int ufwz_uart_octet_sink(void *driver, unsigned char value);
int ufwz_uart_octet_source(void *driver, void *value);

#define UFWZ_UART_POLL_SOURCE(name)                     \
    OCTET_SOURCE_INIT(                                  \
        ufwz_uart_octet_source,                         \
        (void*)(&ufwz_uart_poll_thread_data_##name))

#define UFWZ_UART_POLL_SINK(DRIVER)                             \
    OCTET_SINK_INIT(ufwz_uart_octet_sink, (void*)(DRIVER))

struct ufwz_uart_poll_thread_data {
    const struct device *ifc;
    struct k_pipe *pipe;
    struct k_sem *sem;
    int32_t yieldtime;
};

void ufwz_uart_poll_thread_cb(void *data, void *a, void *b);

#define UFWZ_UART_POLL_THREAD(name, ifc_, pipesize, yt)                 \
    K_PIPE_DEFINE(ufwz_uart_poll_thread_pipe_##name, pipesize, 4);      \
    static struct ufwz_uart_poll_thread_data                            \
    ufwz_uart_poll_thread_data_##name = {                               \
        .ifc = (ifc_),                                                  \
        .pipe = &ufwz_uart_poll_thread_pipe_##name,                     \
        .sem = NULL,                                                    \
        .yieldtime = (yt)                                               \
    };                                                                  \
    K_THREAD_DEFINE(ufwz_uart_poll_thread_##name,                       \
                    128,                                                \
                    ufwz_uart_poll_thread_cb,                           \
                    &ufwz_uart_poll_thread_data_##name,                 \
                    NULL, NULL,                                         \
                    14, 0, 0)

#define UFWZ_UART_POLL_THREAD_DELAYABLE(name, ifc_, pipesize, yt)       \
    K_SEM_DEFINE(ufwz_uart_poll_thread_sem_##name, 0, 1);               \
    K_PIPE_DEFINE(ufwz_uart_poll_thread_pipe_##name, pipesize, 4);      \
    static struct ufwz_uart_poll_thread_data                            \
    ufwz_uart_poll_thread_data_##name = {                               \
        .ifc = (ifc_),                                                  \
        .pipe = &ufwz_uart_poll_thread_pipe_##name,                     \
        .sem = &ufwz_uart_poll_thread_sem_##name,                       \
        .yieldtime = (yt)                                               \
    };                                                                  \
    K_THREAD_DEFINE(ufwz_uart_poll_thread_##name,                       \
                    128,                                                \
                    ufwz_uart_poll_thread_cb,                           \
                    &ufwz_uart_poll_thread_data_##name,                 \
                    NULL, NULL,                                         \
                    14, 0, 0)

#define UFWZ_UART_POLL_THREAD_START(name)               \
    do {                                                \
        k_sem_give(&ufwz_uart_poll_thread_sem_##name);  \
    } while (0)

#endif /* INC_INCLUDE_UFWZ_ENDPOINT_UART_POLL_H_0c903589 */
