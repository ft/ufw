/*
 * Copyright (c) 2024 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#ifndef INC_INCLUDE_UFWZ_ENDPOINT_UART_POLL_H_0c903589
#define INC_INCLUDE_UFWZ_ENDPOINT_UART_POLL_H_0c903589

int ufwz_uart_octet_sink(void *driver, unsigned char value);
int ufwz_uart_octet_source(void *driver, void *value);

#define UFWZ_UART_POLL_SOURCE(DRIVER)                           \
    OCTET_SOURCE_INIT(ufwz_uart_octet_source, (void*)DRIVER)

#define UFWZ_UART_POLL_SINK(DRIVER)                             \
    OCTET_SINK_INIT(ufwz_uart_octet_sink, (void*)DRIVER)

#endif /* INC_INCLUDE_UFWZ_ENDPOINT_UART_POLL_H_0c903589 */
