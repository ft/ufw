/*
 * Copyright (c) 2024-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#ifndef INC_INCLUDE_UFWZ_ENDPOINT_UART_FIFO_H_ade9b63d
#define INC_INCLUDE_UFWZ_ENDPOINT_UART_FIFO_H_ade9b63d

#include <zephyr/kernel.h>

#include <zephyr/device.h>

int ufwz_uart_fifo_source(void *driver, void *value);
int ufwz_uart_fifo_source_init(const struct device *dev, struct k_pipe *data);

#define UFWZ_UART_FIFO_SOURCE(DRIVER)                   \
    OCTET_SOURCE_INIT(ufwz_uart_fifo_source, DRIVER)

#endif /* INC_INCLUDE_UFWZ_ENDPOINT_UART_FIFO_H_ade9b63d */
