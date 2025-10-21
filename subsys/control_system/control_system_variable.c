/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zvb/control_system/variable.h>

void control_system_variable_set(struct control_system_variable *var, q31_t value)
{
	atomic_set(&var->variable, value);
}

q31_t control_system_variable_get(struct control_system_variable *var)
{
	return atomic_get(&var->variable);
}

const char *control_system_variable_name_get(struct control_system_variable *var)
{
	return var->name;
}
