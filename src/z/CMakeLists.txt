message(STATUS "Configuring ufw-zephyr")

zephyr_library_named(ufw-zephyr)
target_link_libraries(ufw-zephyr PUBLIC ufw)

zephyr_library_sources_ifdef(CONFIG_UFW_SHELL_ADDONS       shell-addons.c)
zephyr_library_sources_ifdef(CONFIG_UFW_I2C_TEXT           driver-i2c-text.c)
zephyr_library_sources_ifdef(CONFIG_UFW_SPI_TEXT           driver-spi-text.c)
zephyr_library_sources_ifdef(CONFIG_UFW_SLAB_ALLOCATOR     slab-allocator.c)
zephyr_library_sources_ifdef(CONFIG_UFW_ENDPOINT_UART_POLL endpoint-uart-poll.c)
zephyr_library_sources_ifdef(CONFIG_UFW_ENDPOINT_UART_FIFO endpoint-uart-fifo.c)
