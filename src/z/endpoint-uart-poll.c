/*
 * Copyright (c) 2024-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#include <zephyr/kernel.h>

#include <zephyr/drivers/uart.h>

#include <ufw/compiler.h>
#include <ufwz/endpoint-uart-poll.h>

/* For transmissions in polling mode, we just use uart_poll_out without any
 * further wrapping. */

int
ufwz_uart_octet_sink(void *driver, unsigned char value)
{
    uart_poll_out(driver, value);
    return 1;
}

/*
 * Receiving in poll mode sucks a little. Normally, it would be much more
 * advisable to use the FIFO API for UARTs. Some however, like the one for
 * native-sim, don't allow for that. So here we are. What we do is to run a
 * small, low-priority thread, that polls for UART data and yields back to the
 * scheduler when it can't read any data. This polling feeds a pipe, that a
 * real processing thread can pend for data on.
 */

int
ufwz_uart_octet_source(void *driver, void *value)
{
    struct ufwz_uart_poll_thread_data *data = driver;
    struct k_pipe *p = data->pipe;
    size_t done = 0u;
    while (done == 0u) {
        k_pipe_get(p, value, 1u, &done, 1u, K_FOREVER);
    }
    return 1;
}

void
ufwz_uart_poll_thread_cb(void *data, UNUSED void *a, UNUSED void *b)
{
    struct ufwz_uart_poll_thread_data *cfg = data;

    if (cfg->sem != NULL) {
        k_sem_take(cfg->sem, K_FOREVER);
    }
    for (;;) {
        unsigned char value = 0u;
        while (uart_poll_in(cfg->ifc, &value) == 0) {
            size_t done = 0;
            while (done == 0) {
                k_pipe_put(cfg->pipe, &value, 1u, &done, 1u, K_FOREVER);
            }
        }
        if (cfg->yieldtime > 0) {
            k_msleep(cfg->yieldtime);
        } else {
            k_yield();
        }
    }
}
