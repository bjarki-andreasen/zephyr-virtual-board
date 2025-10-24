/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include <zvb/drivers/actuator.h>
#include <zvb/control_system/signal/osignal.h>

#define DT_DRV_COMPAT zephyr_servo_pwm

struct driver_data {
	struct control_system_signal_osignal pulse_osig;
};

struct driver_config {
	struct pwm_dt_spec pwm_spec;
	uint32_t pulse_min_ns;
	uint32_t pulse_max_ns;
};

static int driver_api_set_setpoint(const struct device *dev,
				   q31_t setpoint)
{
	struct driver_data *dev_data = dev->data;
	const struct driver_config *dev_config = dev->config;
	int64_t pulse_ns;

	control_system_signal_osignal_sample(&dev_data->pulse_osig,
					     setpoint,
					     &pulse_ns);

	return pwm_set_pulse_dt(&dev_config->pwm_spec, pulse_ns);
}

static DEVICE_API(actuator, driver_api) = {
	.set_setpoint = driver_api_set_setpoint,
};

static int driver_init(const struct device *dev)
{
	struct driver_data *dev_data = dev->data;
	const struct driver_config *dev_config = dev->config;

	if (!pwm_is_ready_dt(&dev_config->pwm_spec)) {
		return -ENODEV;
	}

	control_system_signal_osignal_configure(&dev_data->pulse_osig,
						dev_config->pulse_min_ns,
						dev_config->pulse_max_ns);

	return 0;
}

#define DRIVER_INST_DEFINE(inst)								\
												\
	static struct driver_data data##inst;							\
												\
	static struct driver_config config##inst = {						\
		.pwm_spec = PWM_DT_SPEC_INST_GET(inst),						\
		.pulse_min_ns = DT_INST_PROP(inst, pulse_min_ns),				\
		.pulse_max_ns = DT_INST_PROP(inst, pulse_max_ns),				\
	};											\
												\
	DEVICE_DT_INST_DEFINE(									\
		inst,										\
		driver_init,									\
		NULL,										\
		&data##inst,									\
		&config##inst,									\
		POST_KERNEL,									\
		UTIL_INC(CONFIG_PWM_INIT_PRIORITY),						\
		&driver_api									\
	);

DT_INST_FOREACH_STATUS_OKAY(DRIVER_INST_DEFINE)
