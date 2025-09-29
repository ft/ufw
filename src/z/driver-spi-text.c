/*
 * Copyright (c) 2024-2025 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>

#include <ufw/compiler.h>
#include <ufw/sx.h>

#define DT_DRV_COMPAT ufw_spi_text

#define print(fmt, ...) printk("\x1b[0mspi-text: " fmt, __VA_ARGS__)

struct ufw_spi_text_pdata {
    uint64_t fallback;
    struct sx_node *rxring;
};

static int
ufw_spi_text_init(const struct device *dev)
{
    printk("spi-text: Initialising %s...\n", dev->name);
    return 0;
}

static void
ufw_spi_text_io(struct ufw_spi_text_pdata *driver,
                const char *symbol,
                const struct spi_buf_set *sbs)
{
    for (size_t j = 0; j < sbs->count; ++j) {
        print("(%s", symbol);
        unsigned char *data = sbs->buffers[0].buf;
        for (size_t k = 0; k < sbs->buffers[j].len; ++k) {
            if (driver != NULL) {
                /* TODO: Add support for driver->rxring. */
                data[k] = driver->fallback & UCHAR_MAX;
                driver->fallback++;
            }
            printk(" %u", data[k]);
        }
        printk(")\n");
    }
}

static int
ufw_spi_text_transceive(const struct device *spi,
                        UNUSED const struct spi_config *spi_cfg,
                        const struct spi_buf_set *tx_bufs,
                        const struct spi_buf_set *rx_bufs)
{
    const size_t tx = tx_bufs->count;
    const size_t rx = rx_bufs->count;
    const size_t n = (tx > rx) ? tx : rx;
    struct ufw_spi_text_pdata *data = spi->data;

    for (size_t i = 0; i < n; ++i) {
        if (i < tx) {
            ufw_spi_text_io(NULL, "spi-tx", tx_bufs + i);
        }
        if (i < rx) {
            ufw_spi_text_io(data, "spi-rx", rx_bufs + i);
        }
    }

    return 0;
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

void
ufw_spi_text_loadrx(struct device *spi, struct sx_node *node)
{
    struct ufw_spi_text_pdata *data = spi->data;

    if (data->rxring == NULL) {
        data->rxring = node;
    } else {
        data->rxring = sx_append(data->rxring, node);
    }
}

#define UFW_SPI_TEXT_INIT(n)                                            \
    static struct ufw_spi_text_pdata ufw_spi_text_pdata_##n = {         \
        .fallback = 0u,                                                 \
        .rxring = NULL                                                  \
    };                                                                  \
    DEVICE_DT_INST_DEFINE(n, ufw_spi_text_init, NULL,                   \
                          &ufw_spi_text_pdata_##n, NULL,                \
                          POST_KERNEL,                                  \
                          CONFIG_UFW_SPI_TEXT_INIT_PRIORITY,            \
                          &ufw_spi_text_api);

DT_INST_FOREACH_STATUS_OKAY(UFW_SPI_TEXT_INIT)
