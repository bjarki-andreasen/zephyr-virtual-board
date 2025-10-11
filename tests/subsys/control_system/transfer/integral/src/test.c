/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zvb/control_system/transfer/integral.h>
#include <zephyr/dsp/dsp.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

CONTROL_SYSTEM_TRANSFER_INTEGRAL_DEFINE(test_cst);

ZTEST_SUITE(control_system_transfer_integral, NULL, NULL, NULL, NULL, NULL);

ZTEST(control_system_transfer_integral, test_invalid_configs)
{
	zassert_not_ok(control_system_transfer_integral_configure(&test_cst, 0, 0));
	zassert_not_ok(control_system_transfer_integral_configure(&test_cst, 0, 31));
	zassert_not_ok(control_system_transfer_integral_configure(&test_cst, 10, 31));
}

struct test_config {
	q31_t sample_interval_fract;
	uint8_t sample_interval_shift;
	q31_t threshold;
	q31_t insig[5];
	q31_t osig[5];
};

static const struct test_config test_configs[] = {
	{
		.sample_interval_fract = INT32_MAX,
		.sample_interval_shift = 0,
		.threshold = 10,
		.insig = {
			1000,
			-1000,
			100,
			50,
			1000,
		},
		.osig = {
			1000,
			0,
			100,
			150,
			1150,
		}
	},
	{
		.sample_interval_fract = INT32_MAX,
		.sample_interval_shift = 1,
		.threshold = 20,
		.insig = {
			1000,
			-1000,
			100,
			50,
			1000,
		},
		.osig = {
			2000,
			0,
			200,
			300,
			2300,
		}
	},
};

ZTEST(control_system_transfer_integral, test_integral)
{
	int ret;
	q31_t osig;

	ARRAY_FOR_EACH_PTR(test_configs, config) {
		ret = control_system_transfer_integral_configure(
			&test_cst,
			config->sample_interval_fract,
			config->sample_interval_shift
		);
		zassert_ok(ret);

		ARRAY_FOR_EACH(config->insig, i) {
			ret = control_system_transfer_transfer(&test_cst,
							       config->insig[i],
							       &osig);
			zassert_ok(ret);
			zassert_within(osig, config->osig[i], config->threshold);
		}
	}
}
