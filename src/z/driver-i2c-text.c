/*
 * Copyright (c) 2024 micro framework workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>

#define DT_DRV_COMPAT ufw_i2c_text

static int
ufw_i2c_text_init(const struct device *dev)
{
    printk("text-i2c: Initialising %s...\n", dev->name);
    return 0;
}

static int ufw_i2c_text_configure(const struct device *dev,
                                  uint32_t dev_config)
{
    printk("text-i2c: Configuring %s...\n", dev->name);
    return -ENOTSUP;
}

static int ufw_i2c_text_transfer(const struct device *dev,
                                 struct i2c_msg *msgs,
                                 uint8_t num_msgs, uint16_t addr)
{
    printk("text-i2c: Transmitting with %s...\n", dev->name);
    return -ENOTSUP;
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
