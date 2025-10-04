/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zvb/control_system/transfer/linear.h>
#include <zephyr/dsp/dsp.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>


CONTROL_SYSTEM_TRANSFER_LINEAR_DEFINE(test_cst);

ZTEST_SUITE(control_system_transfer_linear, NULL, NULL, NULL, NULL, NULL);

ZTEST(control_system_transfer_linear, test_invalid_configs)
{
	zassert_not_ok(control_system_transfer_linear_configure(&test_cst, 0, 0));
	zassert_not_ok(control_system_transfer_linear_configure(&test_cst, 1, 0));
}

struct test_config {
	q31_t min_insig;
	q31_t max_insig;
	q31_t threshold; /* (2 ** 32) / (max_insig - min_insig) * 4 */
	q31_t insig[17];
};

static const struct test_config test_configs[] = {
	{
		.min_insig = 4999,
		.max_insig = 15001,
		.threshold = 1717643,
		.insig = {
			4999,
			5624,
			6249,
			6874,
			7499,
			8125,
			8750,
			9375,
			10000,
			10625,
			11250,
			11875,
			12501,
			13126,
			13751,
			14376,
			15001,
		},
	},
	{
		.min_insig = -10000000,
		.max_insig = 2000000000,
		.threshold = 8,
		.insig = {
			-10000000,
			115625000,
			241250000,
			366875000,
			492500000,
			618125000,
			743750000,
			869375000,
			995000000,
			1120625000,
			1246250000,
			1371875000,
			1497500000,
			1623125000,
			1748750000,
			1874375000,
			2000000000,
		},
	},
};

ZTEST(control_system_transfer_linear, test_linear)
{
	int ret;
	q31_t osig;
	int64_t actual_osig;
	int64_t expected_osig;

	ARRAY_FOR_EACH_PTR(test_configs, config) {
		ret = control_system_transfer_linear_configure(&test_cst,
							       config->min_insig,
							       config->max_insig);
		zassert_ok(ret);

		expected_osig = INT32_MIN;
		ARRAY_FOR_EACH(config->insig, i) {
			ret = control_system_transfer_transfer(&test_cst,
							       config->insig[i],
							       &osig);
			zassert_ok(ret);
			actual_osig = osig;
			zassert_within(actual_osig, expected_osig, config->threshold);

			if (expected_osig == 0x70000000) {
				/* Saturate */
				expected_osig += 0x0FFFFFFF;
			} else {
				expected_osig += 0x10000000;
			}
		}
	}
}
