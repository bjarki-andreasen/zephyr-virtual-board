/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_CONTROL_SYSTEM_VARIABLE_H_
#define ZEPHYR_INCLUDE_CONTROL_SYSTEM_VARIABLE_H_

#include <zephyr/kernel.h>
#include <zephyr/dsp/types.h>
#include <zephyr/sys/atomic.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @cond INTERNAL_HIDDEN
 */

struct control_system_variable {
	const char *name;
	atomic_t variable;
};

/**
 * @endcond
 */

/**
 * @brief Define a control system variable
 *
 * @param _name Name of control system variable
 * @param _value Initial value of control system variable
 */
#define CONTROL_SYSTEM_VARIABLE_DEFINE(_name, _value)						\
	static STRUCT_SECTION_ITERABLE(control_system_variable, _name) = {			\
		.name = STRINGIFY(_name),							\
		.variable = ATOMIC_INIT(_value),						\
	}

/**
 * @brief Set the value of a control system variable
 *
 * @param var Control system variable
 * @param value Value to set
 */
void control_system_variable_set(struct control_system_variable *var, q31_t value);

/**
 * @brief Get the value of a control system variable
 *
 * @param var Control system variable
 *
 * @retval Variable value
 */
q31_t control_system_variable_get(struct control_system_variable *var);

/**
 * @brief Get the name of a control system variable
 *
 * @param var Control system variable
 *
 * @retval Variable name
 */
const char *control_system_variable_name_get(struct control_system_variable *var);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_CONTROL_SYSTEM_VARIABLE_H_ */
