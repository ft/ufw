/*
 * Copyright (c) 2024 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#ifndef INC_INCLUDE_UFWZ_ENDPOINT_UART_FIFO_H_ade9b63d
#define INC_INCLUDE_UFWZ_ENDPOINT_UART_FIFO_H_ade9b63d

#include <zephyr/device.h>

#include <stdint.h>

#include <ufw/octet-ring.h>

struct ufwz_uart_fifo_source_data {
    uint8_t *buffer;
    size_t buffer_size;
    octet_ring ring;
};

int ufwz_uart_fifo_source(void *driver, void *value);
int ufwz_uart_fifo_source_init(const struct device *dev,
                               struct ufwz_uart_fifo_source_data *data);

#define DEFINE_UART_FIFO_SOURCE_DATA(NAME, SIZE)        \
    struct ufwz_uart_fifo_source_data NAME = {          \
        .buffer = (uint8_t[SIZE]) { 0u },               \
        .buffer_size = SIZE,                            \
    }

#define UFWZ_UART_FIFO_SOURCE(DRIVER)                   \
    OCTET_SOURCE_INIT(ufwz_uart_fifo_source, DRIVER)

#endif /* INC_INCLUDE_UFWZ_ENDPOINT_UART_FIFO_H_ade9b63d */
