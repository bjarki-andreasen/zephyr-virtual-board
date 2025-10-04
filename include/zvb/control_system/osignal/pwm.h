/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_CONTROL_SYSTEM_OSIGNAL_PWM_H_
#define ZEPHYR_INCLUDE_CONTROL_SYSTEM_OSIGNAL_PWM_H_

#include <zvb/control_system/osignal.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @cond INTERNAL_HIDDEN
 */

struct control_system_osignal_pwm_config {
	const struct device *dev;
	uint32_t channel;
	uint32_t flags;
};

struct control_system_osignal_pwm_data {
	uint32_t period_cycles;
	int32_t pulse_center_cycles;
	int32_t pulse_range_cycles;
};

extern const struct control_system_osignal_api __control_system_osignal_pwm_api;

/**
 * @endcond
 */

#define CONTROL_SYSTEM_OSIGNAL_PWM_DEFINE(							\
		_name,										\
		_dev,										\
		_channel,									\
		_flags										\
	)											\
	static struct control_system_osignal_pwm_data _CONCAT(_name, _data);			\
												\
	static const struct control_system_osignal_pwm_config _CONCAT(_name, _config) = {	\
		.dev = _dev,									\
		.channel = _channel,								\
		.flags = _flags,								\
	};											\
												\
	CONTROL_SYSTEM_OSIGNAL_DEFINE(								\
		_name,										\
		&_CONCAT(_name, _data),								\
		&_CONCAT(_name, _config),							\
		&__control_system_osignal_pwm_api						\
	)

int control_system_osignal_pwm_configure(const struct control_system_osignal *cso,
					 uint32_t period_ns,
					 uint32_t min_pulse_ns,
					 uint32_t max_pulse_ns);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_CONTROL_SYSTEM_OSIGNAL_PWM_H_ */
