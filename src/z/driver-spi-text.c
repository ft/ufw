/*
 * Copyright (c) 2024-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>

#include <ufw/compiler.h>

#define DT_DRV_COMPAT ufw_spi_text

static int
ufw_spi_text_init(const struct device *dev)
{
    printk("text-spi: Initialising %s...\n", dev->name);
    return 0;
}

static int
ufw_spi_text_transceive(UNUSED const struct device *dev,
                        UNUSED const struct spi_config *spi_cfg,
                        UNUSED const struct spi_buf_set *tx_bufs,
                        UNUSED const struct spi_buf_set *rx_bufs)
{
    printk("text-spi: In there!\n");
    return -ENOTSUP;
}

#ifdef CONFIG_SPI_ASYNC
static int
ufw_spi_text_transceive_async(UNUSED const struct device *dev,
                              UNUSED const struct spi_config *spi_cfg,
                              UNUSED const struct spi_buf_set *tx_bufs,
                              UNUSED const struct spi_buf_set *rx_bufs,
                              UNUSED spi_callback_t cb,
                              UNUSED void *userdata)
{
    return -ENOTSUP;
}
#endif

static int
ufw_spi_text_release(UNUSED const struct device *dev,
                     UNUSED const struct spi_config *spi_cfg)
{
    return -ENOTSUP;
}

static const struct spi_driver_api ufw_spi_text_api = {
    .transceive = ufw_spi_text_transceive,
#ifdef CONFIG_SPI_ASYNC
    .transceive_async = ufw_spi_text_transceive_async,
#endif
    .release = ufw_spi_text_release,
};

#define UFW_SPI_TEXT_INIT(n)                                            \
    DEVICE_DT_INST_DEFINE(n, ufw_spi_text_init, NULL, NULL, NULL,       \
                          POST_KERNEL,                                  \
                          CONFIG_UFW_SPI_TEXT_INIT_PRIORITY,            \
                          &ufw_spi_text_api);

DT_INST_FOREACH_STATUS_OKAY(UFW_SPI_TEXT_INIT)
