/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zvb/control_system/osignal/pwm.h>
#include <zephyr/dsp/dsp.h>
#include <zephyr/drivers/pwm.h>

static int osignal_pwm_api_set_signal(const struct control_system_osignal *cso, q31_t sig)
{
	struct control_system_osignal_pwm_data *data = cso->data;
	const struct control_system_osignal_pwm_config *config = cso->config;
	uint32_t pulse_cycles;

	zdsp_mult_q31(&data->pulse_range_cycles, &sig, &sig, 1);
	pulse_cycles = (uint32_t)(data->pulse_center_cycles + sig + 1);
	return pwm_set_cycles(config->dev,
			      config->channel,
			      data->period_cycles,
			      pulse_cycles,
			      config->flags);
}

const struct control_system_osignal_api __control_system_osignal_pwm_api = {
	.set_signal = osignal_pwm_api_set_signal,
};

int control_system_osignal_pwm_configure(const struct control_system_osignal *cso,
					 uint32_t period_ns,
					 uint32_t min_pulse_ns,
					 uint32_t max_pulse_ns)
{
	struct control_system_osignal_pwm_data *data = cso->data;
	const struct control_system_osignal_pwm_config *config = cso->config;
	int ret;
	uint64_t cycles_per_sec;
	uint64_t cycles;

	if (max_pulse_ns <= min_pulse_ns ||
	    period_ns < max_pulse_ns) {
		return -EINVAL;
	}

	ret = pwm_get_cycles_per_sec(config->dev, config->channel, &cycles_per_sec);
	if (ret) {
		return ret;
	}

	cycles = (cycles_per_sec * period_ns) / NSEC_PER_SEC;
	if (cycles > UINT32_MAX || cycles == 0) {
		return -EINVAL;
	}

	data->period_cycles = (uint32_t)cycles;

	cycles = (cycles_per_sec * max_pulse_ns) / NSEC_PER_SEC;
	if (cycles > INT32_MAX) {
		return -EINVAL;
	}

	data->pulse_center_cycles = (int32_t)cycles;

	cycles = ((cycles_per_sec * (max_pulse_ns - min_pulse_ns)) / NSEC_PER_SEC) / 2;
	data->pulse_range_cycles = (int32_t)cycles;
	data->pulse_center_cycles -= (int32_t)cycles;
	return 0;
}
