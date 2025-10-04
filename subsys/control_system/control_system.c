/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zvb/control_system/control_system.h>

int control_system_set_setpoint(const struct control_system *cs, q31_t setpoint)
{
	int ret;

	ret = cs->api->set_setpoint(cs, setpoint);
	if (ret) {
		return ret;
	}

	atomic_set(&cs->state->setpoint, setpoint);
	return ret;
}

void control_system_get_setpoint(const struct control_system *cs, q31_t *setpoint)
{
	*setpoint = atomic_get(&cs->state->setpoint);
}

int control_system_set_process_var(const struct control_system *cs, q31_t process_var)
{
	int ret;

	ret = cs->api->set_process_var(cs, process_var);
	if (ret) {
		return ret;
	}

	atomic_set(&cs->state->process_var, process_var);
	return ret;
}

void control_system_get_process_var(const struct control_system *cs, q31_t *process_var)
{
	*process_var = atomic_get(&cs->state->process_var);
}

int control_system_sample(const struct control_system *cs, q31_t *sample)
{
	int ret;

	ret = cs->api->sample(cs, sample);
	if (ret) {
		return ret;
	}

	atomic_set(&cs->state->sample, *sample);
	return ret;
}

void control_system_get_sample(const struct control_system *cs, q31_t *sample)
{
	*sample = atomic_get(&cs->state->sample);
}

const char *control_system_get_name(const struct control_system *cs)
{
	return cs->name;
}
