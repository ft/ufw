/*
 * Copyright (c) 2024-2026 ufw workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

#include <ufw/compiler.h>

#define DT_DRV_COMPAT ufw_i2c_text

#define print(fmt, ...) printk("\x1b[0mi2c-text: " fmt, __VA_ARGS__)

static int
ufw_i2c_text_init(const struct device *dev)
{
    printk("i2c-text: Initialising %s...\n", dev->name);
    return 0;
}

static int ufw_i2c_text_configure(const struct device *dev,
                                  UNUSED uint32_t dev_config)
{
    printk("i2c-text: Configuring %s...\n", dev->name);
    return 0;
}

static int ufw_i2c_text_transfer(const struct device *dev,
                                 struct i2c_msg *msgs,
                                 uint8_t num_msgs,
                                 uint16_t addr)
{
#if CONFIG_UFW_I2C_TEXT_DEBUG
    printk("i2c-text: Transmitting with %s...\n", dev->name);
    printk("  num_msgs: %u\n", num_msgs);
#endif /* CONFIG_UFW_I2C_TEXT_DEBUG */
    for (size_t i = 0; i < num_msgs; ++i) {
        const uint32_t len = msgs[i].len;
        const uint8_t flags = msgs[i].flags;
        const uint8_t *data = msgs[i].buf;
        char *type = (flags & I2C_MSG_READ) ? "read" : "write";

#if CONFIG_UFW_I2C_TEXT_DEBUG
        printk("    msg[%zu].len   = %u\n",  i, len);
        printk("    msg[%zu].flags = %lu\n", i, flags);

        if (flags & I2C_MSG_READ) {
            printk("      | msg-read\n");
        } else {
            printk("      | msg-write\n");
        }
        if (flags & I2C_MSG_STOP) {
            printk("      | msg-stop\n");
        }
        if (flags & I2C_MSG_RESTART) {
            printk("      | msg-restart\n");
        }
        if (flags & I2C_MSG_ADDR_10_BITS) {
            printk("      | msg-addr10\n");
        }
#endif /* CONFIG_UFW_I2C_TEXT_DEBUG */

        print("(i2c-%s #x%04x", type, addr);
        for (size_t j = 0; j < len; ++j) {
            printk(" #x%02x", data[j]);
        }
        printk(")\n");
    }
    return 0;
}

static const struct i2c_driver_api ufw_i2c_text_api = {
    .configure = ufw_i2c_text_configure,
    .transfer  = ufw_i2c_text_transfer,
};

#define UFW_I2C_TEXT_INIT(n)                                            \
    I2C_DEVICE_DT_INST_DEFINE(n, ufw_i2c_text_init, NULL, NULL, NULL,   \
                              POST_KERNEL,                              \
                              CONFIG_UFW_I2C_TEXT_INIT_PRIORITY,        \
                              &ufw_i2c_text_api);

DT_INST_FOREACH_STATUS_OKAY(UFW_I2C_TEXT_INIT)
