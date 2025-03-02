/*
 * Copyright (c) 2024-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#include <zephyr/drivers/uart.h>
#include <zephyr/version.h>

#include <stdint.h>

#include <ufw/octet-ring.h>
#include <ufwz/endpoint-uart-fifo.h>

/* The k_pipe API was significantly revamped in Zephyr v4.1.0, and it's
 * actually much nicer now. Still, for some backward compatibility, let's
 * implement this UART FIFO source endpoint with both APIs. */

static void
ufwz_uart_fifo_source_cb(const struct device *dev, void *user_data)
{
    struct ufwz_uart_fifo_data *data = user_data;
    unsigned char buf[16];

    while (uart_irq_update(dev) && uart_irq_rx_ready(dev)) {
        const int rc = uart_fifo_read(dev, buf, 16);
        if (rc <= 0) {
            continue;
        }
#ifdef CONFIG_PIPES
        size_t done;
        k_pipe_put(data->pipe, buf, rc, &done, rc, K_NO_WAIT);
#else
        k_pipe_write(data->pipe, buf, rc, K_NO_WAIT);
#endif /* CONFIG_PIPES */
    }
}

int
ufwz_uart_fifo_source(void *driver, void *buffer, const size_t n)
{
    struct ufwz_uart_fifo_data *data = driver;

#ifdef CONFIG_PIPES
    size_t done = 0u;
    const int rc = k_pipe_get(data->pipe, buffer, n,
                              &done, 1u, data->timeout);
#else
    const int rc = k_pipe_read(data->pipe, buffer, n, data->timeout);
#endif /* CONFIG_PIPES */

    if (rc == -EAGAIN) {
        /* k_pipe_get/read() use this in case of timeout. Meh. */
        return -ETIMEDOUT;
    } else if (rc < 0) {
        return rc;
    }

#ifdef CONFIG_PIPES
    return (int)done;
#else
    return rc;
#endif /* CONFIG_PIPES */
}

int
ufwz_uart_fifo_source_init(const struct device *dev,
                           struct ufwz_uart_fifo_data *data)
{
    const int rc =
        uart_irq_callback_user_data_set(dev, ufwz_uart_fifo_source_cb, data);

    if (rc < 0) {
        return 0;
    }

    uart_irq_rx_enable(dev);
    return rc;
}
