/*
 * Copyright (c) 2024 ufw workers, All rights reserved.
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
    if (uart_irq_update(dev) == false || uart_irq_rx_ready(dev) == false) {
        return;
    }

    struct ufwz_uart_fifo_source_data *drv = user_data;

    uint8_t c;
    while (uart_fifo_read(dev, &c, 1) == 1) {
        octet_ring_put(&drv->ring, c);
    }
}

int
ufwz_uart_fifo_source(void *driver, void *value)
{
    struct ufwz_uart_fifo_source_data *drv = driver;

    if (octet_ring_empty(&drv->ring)) {
        return -EAGAIN;
    }

    unsigned char *v = value;
    *v = octet_ring_get(&drv->ring);
    return 1;
}

int
ufwz_uart_fifo_source_init(const struct device *dev,
                           struct ufwz_uart_fifo_source_data *data)
{
    octet_ring_init(&data->ring, data->buffer, data->buffer_size);
    const int rc =
        uart_irq_callback_user_data_set(dev, ufwz_uart_fifo_source_cb, data);

    if (rc < 0) {
        return 0;
    }

    uart_irq_rx_enable(dev);
    return rc;
}
