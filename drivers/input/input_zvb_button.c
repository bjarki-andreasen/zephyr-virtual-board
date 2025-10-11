/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zvb/drivers/zvb_bus.h>

#define DT_DRV_COMPAT zvb_button

struct driver_data {
	const struct device *dev;
	struct zvb_bus_receive_callback callback;
};

struct driver_config {
	const struct device *bus;
	uint16_t code;
};

static void driver_receive_handler(const struct device *bus,
				   const struct zvb_bus_receive_callback *callback,
				   const uint8_t *data,
				   size_t size)
{
	struct driver_data *dev_data = CONTAINER_OF(callback, struct driver_data, callback);
	const struct device *dev = dev_data->dev;
	const struct driver_config *dev_config = dev->config;

	input_report_key(dev, dev_config->code, data[0], true, K_FOREVER);
}

static int driver_init(const struct device *dev)
{
	struct driver_data *dev_data = dev->data;
	const struct driver_config *dev_config = dev->config;

	if (!device_is_ready(dev_config->bus)) {
		return -ENODEV;
	}

	return zvb_bus_add_receive_callback(dev_config->bus, &dev_data->callback);
}

#define DRIVER_INST_DEFINE(inst)								\
												\
	static struct driver_data data##inst = {						\
		.dev = DEVICE_DT_INST_GET(inst),						\
		.callback = ZVB_BUS_DT_INST_RECEIVE_CALLBACK_INIT(inst, driver_receive_handler),\
	};											\
												\
	static struct driver_config config##inst = {						\
		.bus = DEVICE_DT_GET(DT_INST_BUS(inst)),					\
		.code = DT_INST_PROP(inst, zephyr_code),					\
	};											\
												\
	DEVICE_DT_INST_DEFINE(									\
		inst,										\
		driver_init,									\
		NULL,										\
		&data##inst,									\
		&config##inst,									\
		POST_KERNEL,									\
		UTIL_INC(CONFIG_ZVB_BUS_ZVB_INIT_PRIORITY),					\
		NULL										\
	);

DT_INST_FOREACH_STATUS_OKAY(DRIVER_INST_DEFINE)
