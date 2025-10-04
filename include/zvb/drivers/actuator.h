/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ACTUATOR_H_
#define ZEPHYR_INCLUDE_DRIVERS_ACTUATOR_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/dsp/types.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

typedef int (*actuator_api_set_setpoint)(const struct device *dev, q31_t setpoint);
typedef int (*actuator_api_get_process_var)(const struct device *dev, q31_t *process_var);

__subsystem struct actuator_driver_api {
	actuator_api_set_setpoint set_setpoint;
	actuator_api_get_process_var get_process_var;
};

/** @endcond */

static inline int actuator_set_setpoint(const struct device *dev, q31_t setpoint)
{
	return DEVICE_API_GET(actuator, dev)->set_setpoint(dev, setpoint);
}

static inline int actuator_get_process_var(const struct device *dev, q31_t *process_var)
{
	return DEVICE_API_GET(actuator, dev)->get_process_var(dev, process_var);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_ACTUATOR_H_ */
