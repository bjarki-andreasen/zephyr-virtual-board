/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ZVB_BUS_H_
#define ZEPHYR_INCLUDE_DRIVERS_ZVB_BUS_H_

#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

struct zvb_bus_receive_callback;

typedef void (*zvb_bus_receive_handler_t)(const struct device *dev,
					  const struct zvb_bus_receive_callback *callback,
					  const uint8_t *data,
					  size_t size);

typedef int (*zvb_bus_api_add_receive_callback)(const struct device *dev,
						struct zvb_bus_receive_callback *callback);

typedef int (*zvb_bus_api_remove_receive_callback)(const struct device *dev,
						   struct zvb_bus_receive_callback *callback);

typedef int (*zvb_bus_api_transmit)(const struct device *dev,
				    uint8_t addr,
				    const uint8_t *data,
				    size_t size);

struct zvb_bus_receive_callback {
	sys_snode_t node;
	uint8_t addr;
	zvb_bus_receive_handler_t handler;
};

struct zvb_bus_dt_spec {
	const struct device *dev;
	uint8_t addr;
};

__subsystem struct zvb_bus_driver_api {
	zvb_bus_api_add_receive_callback add_receive_callback;
	zvb_bus_api_remove_receive_callback remove_receive_callback;
	zvb_bus_api_transmit transmit;
};

/** @endcond */

/** Add receive callback for messages */
static inline int zvb_bus_add_receive_callback(const struct device *dev,
					       struct zvb_bus_receive_callback *callback)
{
	return DEVICE_API_GET(zvb_bus, dev)->add_receive_callback(dev, callback);
}

/** Remove receive callback for messages */
static inline int zvb_bus_remove_receive_callback(const struct device *dev,
						  struct zvb_bus_receive_callback *callback)
{
	return DEVICE_API_GET(zvb_bus, dev)->remove_receive_callback(dev, callback);
}

/** Transmit message */
static inline int zvb_bus_transmit(const struct device *dev,
				   uint8_t addr,
				   const uint8_t *data,
				   size_t size)
{
	return DEVICE_API_GET(zvb_bus, dev)->transmit(dev, addr, data, size);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_ZVB_BUS_H_ */
