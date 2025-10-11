/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zvb/control_system/transfer/differential.h>
#include <zephyr/dsp/dsp.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

CONTROL_SYSTEM_TRANSFER_DIFFERENTIAL_DEFINE(test_cst);

ZTEST_SUITE(control_system_transfer_differential, NULL, NULL, NULL, NULL, NULL);

struct test_config {
	q31_t threshold;
	q31_t insig[5];
	q31_t osig[5];
};

static const struct test_config test_configs[] = {
	{
		.threshold = 10,
		.insig = {
			1000,
			-1000,
			20000,
			21000,
			10000,
		},
		.osig = {
			1000,
			-2000,
			21000,
			1000,
			-11000,
		}
	},
	{
		.threshold = 10,
		.insig = {
			-1000,
			-1000,
			20000,
			21000,
			10000,
		},
		.osig = {
			-1000,
			0,
			21000,
			1000,
			-11000,
		}
	},
};

ZTEST(control_system_transfer_differential, test_differential)
{
	int ret;
	q31_t osig;

	ARRAY_FOR_EACH_PTR(test_configs, config) {
		ret = control_system_transfer_differential_configure(&test_cst);
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
