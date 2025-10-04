/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zvb/control_system/control/pid.h>
#include <zephyr/dsp/dsp.h>

static int pid_api_set_setpoint(const struct control_system *cs, q31_t setpoint)
{
	struct control_system_control_pid_data *data = cs->data;

	data->sp = setpoint;
	return 0;
}

static int pid_api_set_process_var(const struct control_system *cs, q31_t process_var)
{
	struct control_system_control_pid_data *data = cs->data;

	data->pv = process_var;
	return 0;
}

static int pid_api_sample(const struct control_system *cs, q31_t *sample)
{
	struct control_system_control_pid_data *data = cs->data;
	q31_t error;
	q31_t temp;

	/* Compute error (SV - PV) */
	zdsp_sub_q31(&data->sp, &data->pv, &error, 1);

	/*
	 * Add proportional component to sample. We write the result directly to the
	 * sample to save one copy from the temp value to the sample.
	 */
	zdsp_scale_q31(&error, data->p_scale_fract, data->p_scale_shift, sample, 1);

	/*
	 * Add integral component to sample. Note we are using the accumulated_error
	 * as both input and output, which is typically not possible as we will be
	 * destroying the input as we overwrite it with the output. For zephyr's DSP
	 * subsystem however, this is explicitly supported.
	 */
	zdsp_scale_q31(&error, data->i_scale_fract, data->i_scale_shift, &temp, 1);
	zdsp_add_q31(&temp, &data->accumulated_error, &data->accumulated_error, 1);
	zdsp_add_q31(sample, &data->accumulated_error, sample, 1);

	/* Add differential component to sample. */
	zdsp_sub_q31(&error, &data->last_error, &temp, 1);
	zdsp_scale_q31(&temp, data->d_scale_fract, data->d_scale_shift, &temp, 1);
	zdsp_add_q31(sample, &data->accumulated_error, sample, 1);

	data->last_error = error;
	return 0;
}

const struct control_system_api __control_system_control_pid_api = {
	.set_setpoint = pid_api_set_setpoint,
	.set_process_var = pid_api_set_process_var,
	.sample = pid_api_sample,
};

int control_system_control_pid_configure(const struct control_system *cs,
					 q31_t p_scale_fract,
					 int8_t p_scale_shift,
					 q31_t i_scale_fract,
					 int8_t i_scale_shift,
					 q31_t d_scale_fract,
					 int8_t d_scale_shift)
{
	struct control_system_control_pid_data *data = cs->data;

	data->p_scale_fract = p_scale_fract;
	data->p_scale_shift = p_scale_shift;
	data->i_scale_fract = i_scale_fract;
	data->i_scale_shift = i_scale_shift;
	data->d_scale_fract = d_scale_fract;
	data->d_scale_shift = d_scale_shift;
	return 0;
}
