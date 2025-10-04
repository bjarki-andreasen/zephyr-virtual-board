/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_CONTROL_SYSTEM_CONTROLLER_PID_H_
#define ZEPHYR_INCLUDE_CONTROL_SYSTEM_CONTROLLER_PID_H_

#include <zvb/control_system/control_system.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @cond INTERNAL_HIDDEN
 */

struct control_system_control_pid_data {
	q31_t sp;
	q31_t pv;
	q31_t accumulated_error;
	q31_t last_error;
	q31_t p_scale_fract;
	int8_t p_scale_shift;
	q31_t i_scale_fract;
	int8_t i_scale_shift;
	q31_t d_scale_fract;
	int8_t d_scale_shift;
};

extern const struct control_system_api __control_system_control_pid_api;

/**
 * @endcond
 */

#define CONTROL_SYSTEM_CONTROL_PID_DEFINE(_name)						\
	static struct control_system_control_pid_data _CONCAT(_name, _data);			\
												\
	CONTROL_SYSTEM_DEFINE(									\
		_name,										\
		&_CONCAT(_name, _data),								\
		NULL,										\
		&__control_system_control_pid_api						\
	)

int control_system_control_pid_configure(const struct control_system *cs,
					 q31_t p_scale_fract,
					 int8_t p_scale_shift,
					 q31_t i_scale_fract,
					 int8_t i_scale_shift,
					 q31_t d_scale_fract,
					 int8_t d_scale_shift);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_CONTROL_SYSTEM_CONTROLLER_PID_H_ */
