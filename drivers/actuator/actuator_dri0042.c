/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zvb/drivers/actuator.h>
#include <zvb/control_system/signal/osignal.h>

#define DT_DRV_COMPAT dfrobot_dri0042

#define DRIVER_PWM_PULSE_MIN_NS 200

enum driver_direction {
	DRIVER_DIRECTION_STOP = 0,
	DRIVER_DIRECTION_FORWARD,
	DRIVER_DIRECTION_BACKWARD,
};

struct driver_data {
	struct control_system_signal_osignal pulse_osig;
	enum driver_direction dir;
};

struct driver_config {
	struct pwm_dt_spec pwm_spec;
	struct gpio_dt_spec in1_spec;
	struct gpio_dt_spec in2_spec;
};

static const uint8_t driver_dir_map[3][2] = {
	{0, 0},
	{1, 0},
	{0, 1},
};

static int driver_set_direction(const struct device *dev, enum driver_direction dir)
{
	const struct driver_config *dev_config = dev->config;

	if (gpio_pin_set_dt(&dev_config->in1_spec, driver_dir_map[dir][0]) ||
	    gpio_pin_set_dt(&dev_config->in2_spec, driver_dir_map[dir][1])) {
		return -EIO;
	}

	return 0;
}

static int driver_api_set_setpoint(const struct device *dev,
				   q31_t setpoint)
{
	struct driver_data *dev_data = dev->data;
	const struct driver_config *dev_config = dev->config;
	int64_t pulse_ns;
	enum driver_direction new_dir;

	control_system_signal_osignal_sample(&dev_data->pulse_osig,
					     setpoint,
					     &pulse_ns);

	if (pulse_ns >= 0) {
		new_dir = DRIVER_DIRECTION_FORWARD;
	} else {
		new_dir = DRIVER_DIRECTION_BACKWARD;
		pulse_ns = -pulse_ns;
	}

	if (pulse_ns < DRIVER_PWM_PULSE_MIN_NS) {
		pulse_ns = 0;
		new_dir = DRIVER_DIRECTION_STOP;
	}

	if (dev_data->dir != new_dir) {
		if (driver_set_direction(dev, new_dir)) {
			return -EIO;
		}

		dev_data->dir = new_dir;
	}

	return pwm_set_pulse_dt(&dev_config->pwm_spec, pulse_ns);
}

static DEVICE_API(actuator, driver_api) = {
	.set_setpoint = driver_api_set_setpoint,
};

static int driver_init(const struct device *dev)
{
	struct driver_data *dev_data = dev->data;
	const struct driver_config *dev_config = dev->config;

	if (!pwm_is_ready_dt(&dev_config->pwm_spec) ||
	    !gpio_is_ready_dt(&dev_config->in1_spec) ||
	    !gpio_is_ready_dt(&dev_config->in2_spec)) {
		return -ENODEV;
	}

	control_system_signal_osignal_configure(&dev_data->pulse_osig,
						-((int64_t)dev_config->pwm_spec.period),
						dev_config->pwm_spec.period);

	if (gpio_pin_configure_dt(&dev_config->in1_spec, GPIO_OUTPUT_INACTIVE) ||
	    gpio_pin_configure_dt(&dev_config->in2_spec, GPIO_OUTPUT_INACTIVE)) {
		return -ENODEV;
	}

	return 0;
}

#define DRIVER_INST_DEFINE(inst)								\
												\
	static struct driver_data data##inst;							\
												\
	static struct driver_config config##inst = {						\
		.pwm_spec = PWM_DT_SPEC_INST_GET(inst),						\
		.in1_spec = GPIO_DT_SPEC_INST_GET(inst, in1_gpios),				\
		.in2_spec = GPIO_DT_SPEC_INST_GET(inst, in2_gpios),				\
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
