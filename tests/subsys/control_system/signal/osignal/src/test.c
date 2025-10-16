/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zvb/control_system/signal/osignal.h>
#include <zephyr/dsp/dsp.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

static struct control_system_signal_osignal test_osignal;

ZTEST_SUITE(control_system_signal_osignal, NULL, NULL, NULL, NULL, NULL);

struct test_config {
	int64_t min_osig;
	int64_t max_osig;
	int64_t threshold;
	int64_t osigs[5];
};

static const q31_t test_insigs[] = {
	INT32_MIN,
	INT32_MIN / 2,
	0,
	INT32_MAX / 2,
	INT32_MAX,
};

static const struct test_config test_configs[] = {
	{
		.min_osig = 500,
		.max_osig = 1500,
		.threshold = 2,
		.osigs = {
			500,
			750,
			1000,
			1250,
			1500,
		},
	},
	{
		.min_osig = INT64_MIN,
		.max_osig = INT64_MAX,
		.threshold = 0x200000000,
		.osigs = {
			INT64_MIN,
			INT64_MIN / 2,
			0,
			INT64_MAX / 2,
			INT64_MAX,
		},
	},
};

ZTEST(control_system_signal_osignal, test_osignal)
{
	int64_t osig;

	ARRAY_FOR_EACH_PTR(test_configs, config) {
		control_system_signal_osignal_configure(&test_osignal,
							config->min_osig,
							config->max_osig);

		ARRAY_FOR_EACH(test_insigs, i) {
			control_system_signal_osignal_sample(&test_osignal,
							     test_insigs[i],
							     &osig);

			if (osig <= INT64_MIN + config->threshold) {
				zassert_between_inclusive(osig,
							  config->osigs[i],
							  config->osigs[i] + config->threshold);
			} else if (osig >= INT64_MAX - config->threshold) {
				zassert_between_inclusive(osig,
							  config->osigs[i] - config->threshold,
							  config->osigs[i]);
			} else {
				zassert_within(osig, config->osigs[i], config->threshold);
			}
		}
	}
}
