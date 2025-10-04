/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_CONTROL_SYSTEM_H_
#define ZEPHYR_INCLUDE_CONTROL_SYSTEM_H_

#include <zephyr/kernel.h>
#include <zephyr/dsp/types.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/atomic.h>

#ifdef __cplusplus
extern "C" {
#endif

struct control_system;

/**
 * @cond INTERNAL_HIDDEN
 */

typedef int (*control_system_api_set_setpoint)(const struct control_system *cs,
					       q31_t setpoint);

typedef int (*control_system_api_set_process_var)(const struct control_system *cs,
						  q31_t process_var);

typedef int (*control_system_api_sample)(const struct control_system *cs,
					 q31_t *sample);

struct control_system_api {
	control_system_api_set_setpoint set_setpoint;
	control_system_api_set_process_var set_process_var;
	control_system_api_sample sample;
};

struct control_system_state {
	atomic_t setpoint;
	atomic_t process_var;
	atomic_t sample;
};

struct control_system {
	void *data;
	const void *config;
	const struct control_system_api *api;
	const char *name;
	struct control_system_state *state;
};

/**
 * @endcond
 */

/**
 * @brief Define a control system instance.
 *
 * @details Statically define, initialize and register control system instance
 * to control system subsystem.
 *
 * @param _name Name and symbol of the instance.
 * @param _data Pointer to implementation specific data.
 * @param _config Pointer to implementation specific configuration.
 * @param _api Pointer to control system API implementation.
 */
#define CONTROL_SYSTEM_DEFINE(_name, _data, _config, _api)					\
	static struct control_system_state _CONCAT(_name, _state);				\
	static const STRUCT_SECTION_ITERABLE(control_system, _name) = {				\
		.data = _data,									\
		.config = _config,								\
		.api = _api,									\
		.name = #_name,									\
		.state = &_CONCAT(_name, _state),						\
	};

/**
 * @brief Set setpoint for the system.
 *
 * @param cs Control system instance.
 * @param setpoint Setpoint to set.
 *
 * @retval 0 of success.
 * @retval -errno code if error.
 */
int control_system_set_setpoint(const struct control_system *cs, q31_t setpoint);

/**
 * @brief Get latest setpoint for the system.
 *
 * @param cs Control system instance.
 * @param setpoint Destination for latest setpoint.
 */
void control_system_get_setpoint(const struct control_system *cs, q31_t *setpoint);

/**
 * @brief Set process variable of the system.
 *
 * @param cs Control system instance.
 * @param process_var Process variable to set.
 */
int control_system_set_process_var(const struct control_system *cs, q31_t process_var);

/**
 * @brief Get latest process variable of the system.
 *
 * @param cs Control system instance.
 * @param process_var Destination for latest process variable.
 */
void control_system_get_process_var(const struct control_system *cs, q31_t *process_var);

/**
 * @brief Produce sample from control system.
 *
 * @details Called at sampling time interval to produce next sample
 * which is used as the input signal for the system.
 *
 * @param cs Control system instance.
 * @param process_var Destination for produces sample.
 *
 * @retval 0 of success.
 * @retval -errno code if error.
 */
int control_system_sample(const struct control_system *cs, q31_t *sample);

/**
 * @brief Get latest sample produced by the control system.
 *
 * @param cs Control system instance.
 * @param sample Destination for latest sample produced by the control system.
 */
void control_system_get_sample(const struct control_system *cs, q31_t *sample);

/**
 * @brief Get the name of control system.
 *
 * @param cs Control system instance.
 *
 * @retval Name of control system instance.
 */
const char *control_system_get_name(const struct control_system *cs);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_CONTROL_SYSTEM_H_ */
