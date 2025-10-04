/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>
#include <zvb/drivers/zvb_bus.h>

#define DT_DRV_COMPAT zvb_led

struct driver_config {
	const struct device *bus;
	uint8_t addr;
};

static int driver_api_on(const struct device *dev, uint32_t led)
{
	const struct driver_config *dev_config = dev->config;
	const uint8_t data = 1;

	if (led != 0) {
		return -ENODEV;
	}

	return zvb_bus_transmit(dev_config->bus, dev_config->addr, &data, 1);
}

static int driver_api_off(const struct device *dev, uint32_t led)
{
	const struct driver_config *dev_config = dev->config;
	const uint8_t data = 0;

	if (led != 0) {
		return -ENODEV;
	}

	return zvb_bus_transmit(dev_config->bus, dev_config->addr, &data, 1);
}

static DEVICE_API(led, driver_api) = {
	.on = driver_api_on,
	.off = driver_api_off,
};

static int driver_init(const struct device *dev)
{
	const struct driver_config *dev_config = dev->config;

	if (!device_is_ready(dev_config->bus)) {
		return -ENODEV;
	}

	return 0;
}

#define DRIVER_INST_DEFINE(inst)								\
												\
	static struct driver_config config##inst = {						\
		.bus = DEVICE_DT_GET(DT_INST_BUS(inst)),					\
		.addr = DT_INST_REG_ADDR(inst),							\
	};											\
												\
	DEVICE_DT_INST_DEFINE(									\
		inst,										\
		driver_init,									\
		NULL,										\
		NULL,										\
		&config##inst,									\
		POST_KERNEL,									\
		UTIL_INC(CONFIG_ZVB_BUS_ZVB_INIT_PRIORITY),					\
		&driver_api									\
	);

DT_INST_FOREACH_STATUS_OKAY(DRIVER_INST_DEFINE)
