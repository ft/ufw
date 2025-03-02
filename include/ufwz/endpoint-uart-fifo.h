/*
 * Copyright (c) 2024-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#ifndef INC_INCLUDE_UFWZ_ENDPOINT_UART_FIFO_H_ade9b63d
#define INC_INCLUDE_UFWZ_ENDPOINT_UART_FIFO_H_ade9b63d

#include <zephyr/kernel.h>

#include <zephyr/device.h>

struct ufwz_uart_fifo_data {
    struct k_pipe *pipe;
    k_timeout_t timeout;
};

int ufwz_uart_fifo_source(void *driver, void *buffer, size_t n);
int ufwz_uart_fifo_source_init(const struct device *dev,
                               struct ufwz_uart_fifo_data *data);

#define UFWZ_UART_FIFO_SOURCE(DRIVER)                   \
    CHUNK_SOURCE_INIT(ufwz_uart_fifo_source, DRIVER)

#define UFWZ_DEFINE_UART_FIFO_DATA(name_, size_, timeout_)      \
    K_PIPE_DEFINE(name_##_pipe, (size_), 4);                    \
    struct ufwz_uart_fifo_data name_ = {                        \
        .pipe = &(name_##_pipe),                                \
        .timeout = (timeout_)                                   \
    };

#endif /* INC_INCLUDE_UFWZ_ENDPOINT_UART_FIFO_H_ade9b63d */
