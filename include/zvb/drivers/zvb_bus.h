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

/**
 * @brief Statically initialize ZVB bus receive callback from devicetree node
 *
 * @param _node_id Devicetree node identifier
 * @param _handler Handler called when data is received from target device
 *
 * @see callback must be initialized before being passed to this API
 *
 * @returns initializer for ZVB bus receive callback
 */
#define ZVB_BUS_DT_RECEIVE_CALLBACK_INIT(_node_id, _handler) \
	{.addr = DT_REG_ADDR(_node_id), .handler = _handler}

/**
 * @brief Statically initialize ZVB bus receive callback from devicetree driver instance
 *
 * @param _inst Devicetree driver instance identifier
 * @param _handler Handler called when data is received from target device
 *
 * @see ZVB_BUS_DT_RECEIVE_CALLBACK_INIT()
 */
#define ZVB_BUS_DT_INST_RECEIVE_CALLBACK_INIT(_inst, _handler) \
	{.addr = DT_REG_ADDR(DT_DRV_INST(_inst)), .handler = _handler}

/**
 * @brief Initialize ZVB bus receive callback
 *
 * @param callback Callback instance to initialize
 * @param addr Address of target device to receive data from
 * @param handler Handler called when data is received from target device
 *
 * @see callback must be initialized before being passed to this API
 *
 * @retval 0 if successful
 * @retval -errno code if failure
 */
static inline void zvb_bus_receive_callback_init(struct zvb_bus_receive_callback *callback,
						 uint8_t addr,
						 zvb_bus_receive_handler_t handler)
{
	callback->addr = addr;
	callback->handler = handler;
}

/**
 * @brief Add receive callback for messages
 *
 * @param dev ZVB Bus device instance
 * @param callback Callback to add
 *
 * @note callback must be initialized before being passed to this API
 *
 * @see zvb_bus_receive_callback_init()
 * @see ZVB_BUS_RECEIVE_CALLBACK_INIT()
 *
 * @retval 0 if successful
 * @retval -errno code if failure
 */
static inline int zvb_bus_add_receive_callback(const struct device *dev,
					       struct zvb_bus_receive_callback *callback)
{
	return DEVICE_API_GET(zvb_bus, dev)->add_receive_callback(dev, callback);
}

/**
 * @brief Remove receive callback for messages
 *
 * @param dev ZVB Bus device instance
 * @param callback Callback to remove
 *
 * @note callback must have been added using @ref zvb_bus_add_receive_callback()
 * before being removed with this API.
 *
 * @retval 0 if successful
 * @retval -errno code if failure
 */
static inline int zvb_bus_remove_receive_callback(const struct device *dev,
						  struct zvb_bus_receive_callback *callback)
{
	return DEVICE_API_GET(zvb_bus, dev)->remove_receive_callback(dev, callback);
}

/**
 * @brief Transmit message to target device on bus
 *
 * @param dev ZVB Bus device instance
 * @param addr Address of target device
 * @param data Message data
 * @param size Size of message data in bytes
 *
 * @retval 0 if successful
 * @retval -errno code if failure
 */
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
