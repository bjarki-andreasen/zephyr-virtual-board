/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/pwm/pwm_fake.h>
#include <zephyr/fff.h>
#include <zvb/control_system/osignal/pwm.h>
#include <zephyr/dsp/dsp.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

DEFINE_FFF_GLOBALS;

CONTROL_SYSTEM_OSIGNAL_PWM_DEFINE(
	test_cso,
	DEVICE_DT_GET(DT_NODELABEL(fake_pwm)),
	0,
	0
);

ZTEST_SUITE(control_system_osignal_pwm, NULL, NULL, NULL, NULL, NULL);

ZTEST(control_system_osignal_pwm, test_invalid_configs)
{
	zassert_not_ok(control_system_osignal_pwm_configure(&test_cso, 0, 0, 0));
	zassert_not_ok(control_system_osignal_pwm_configure(&test_cso, 1000, 0, 0));
	zassert_not_ok(control_system_osignal_pwm_configure(&test_cso, 1000, 1000, 0));
	zassert_not_ok(control_system_osignal_pwm_configure(&test_cso, 1000, 1000, 999));
	zassert_not_ok(control_system_osignal_pwm_configure(&test_cso, 1000, 0, 1001));
	zassert_not_ok(control_system_osignal_pwm_configure(&test_cso, 1000, 1000, 1001));
}

struct test_config {
	uint32_t period_ns;
	uint32_t min_pulse_ns;
	uint32_t max_pulse_ns;
	uint32_t period_cycles;
	/*
	 * Cycles for signal range [-2147483648, ..., 2147483647] in
	 * steps of 0x10000000.
	 */
	uint32_t pulse_cycles[17];
};

static const struct test_config test_configs[] = {
	{
		.period_ns = 1000,
		.min_pulse_ns = 0,
		.max_pulse_ns = 1000,
		.period_cycles = 10,
		.pulse_cycles = {
			0,
			0,
			1,
			1,
			2,
			3,
			3,
			4,
			5,
			5,
			6,
			6,
			7,
			8,
			8,
			9,
			10,
		},
	},
	{
		.period_ns = 1000,
		.min_pulse_ns = 200,
		.max_pulse_ns = 800,
		.period_cycles = 10,
		.pulse_cycles = {
			2,
			2,
			2,
			3,
			3,
			3,
			4,
			4,
			5,
			5,
			5,
			6,
			6,
			6,
			7,
			7,
			8,
		},
	},
	{
		.period_ns = 1000000,
		.min_pulse_ns = 10000,
		.max_pulse_ns = 1000000,
		.period_cycles = 10000,
		.pulse_cycles = {
			100,
			718,
			1337,
			1956,
			2575,
			3193,
			3812,
			4431,
			5050,
			5668,
			6287,
			6906,
			7525,
			8143,
			8762,
			9381,
			10000,
		},
	},
};

ZTEST(control_system_osignal_pwm, test_sweep)
{
	int ret;
	int32_t sig;
	int64_t period_cycles;
	int64_t pulse_cycles;
	int64_t expected_pulse_cycles;

	ARRAY_FOR_EACH_PTR(test_configs, config) {
		ret = control_system_osignal_pwm_configure(&test_cso,
							   config->period_ns,
							   config->min_pulse_ns,
							   config->max_pulse_ns);
		zassert_ok(ret);

		sig = INT32_MIN;
		ARRAY_FOR_EACH(config->pulse_cycles, i) {
			ret = control_system_osignal_set_signal(&test_cso, (int32_t)sig);
			zassert_ok(ret);
			period_cycles = fake_pwm_set_cycles_fake.arg2_val;
			pulse_cycles = fake_pwm_set_cycles_fake.arg3_val;
			zassert_within(period_cycles, config->period_cycles, 1);
			expected_pulse_cycles = config->pulse_cycles[i];
			zassert_within(pulse_cycles, expected_pulse_cycles, 1);

			if (sig == 0x70000000) {
				/* Saturate */
				sig += 0x0FFFFFFF;
			} else {
				sig += 0x10000000;
			}
		}
	}
}
