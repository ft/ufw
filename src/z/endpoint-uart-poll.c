/*
 * Copyright (c) 2024 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#include <zephyr/drivers/uart.h>

#include <ufwz/endpoint-uart-poll.h>

int
ufwz_uart_octet_source(void *driver, void *value)
{
    const int rc = uart_poll_in(driver, value);
    return rc < 0 ? -EAGAIN : 1;
}

int
ufwz_uart_octet_sink(void *driver, unsigned char value)
{
    uart_poll_out(driver, value);
    return 1;
}
