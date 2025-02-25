/*
 * Copyright (c) 2024-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#include <zephyr/drivers/uart.h>

#include <stdint.h>

#include <ufw/octet-ring.h>
#include <ufwz/endpoint-uart-fifo.h>

static void
ufwz_uart_fifo_source_cb(const struct device *dev, void *user_data)
{
    struct k_pipe *pipe = user_data;
    unsigned char buf[16];

    while (uart_irq_update(dev) && uart_irq_rx_ready(dev)) {
        const int rc = uart_fifo_read(dev, buf, 16);
        if (rc <= 0) {
            continue;
        }
        size_t done;
        k_pipe_put(pipe, buf, rc, &done, 1u, K_NO_WAIT);
    }
}

int
ufwz_uart_fifo_source(void *driver, void *value)
{
    struct k_pipe *pipe = driver;

    size_t done = 0u;
    while (done == 0u) {
        k_pipe_get(pipe, value, 1u, &done, 1u, K_FOREVER);
    }
    return 1;
}

int
ufwz_uart_fifo_source_init(const struct device *dev, struct k_pipe *pipe)
{
    const int rc =
        uart_irq_callback_user_data_set(dev, ufwz_uart_fifo_source_cb, pipe);

    if (rc < 0) {
        return 0;
    }

    uart_irq_rx_enable(dev);
    return rc;
}
