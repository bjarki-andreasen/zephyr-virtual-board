/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zvb/control_system/signal/insignal.h>
#include <zvb/control_system/signal/osignal.h>
#include <zephyr/dsp/dsp.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

ZTEST_SUITE(control_system_signal, NULL, NULL, NULL, NULL, NULL);

struct test_osignal_config {
	int64_t min_osig;
	int64_t max_osig;
	int64_t threshold;
	int64_t osigs[5];
};

static const q31_t test_osignal_insigs[] = {
	INT32_MIN,
	INT32_MIN / 2,
	0,
	INT32_MAX / 2,
	INT32_MAX,
};

static struct control_system_signal_osignal test_osignal;

static const struct test_osignal_config test_osignal_configs[] = {
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

static void test_assert_within_int64(int64_t a, int64_t b, int64_t d)
{
	if (a <= INT64_MIN + d) {
		zassert_between_inclusive(a, INT64_MIN, b + d);
	} else if (a >= INT64_MAX - d) {
		zassert_between_inclusive(a, b - d, INT64_MAX);
	} else {
		zassert_within(a, b, d);
	}
}

ZTEST(control_system_signal, test_osignal)
{
	int64_t osig;

	ARRAY_FOR_EACH_PTR(test_osignal_configs, config) {
		control_system_signal_osignal_configure(&test_osignal,
							config->min_osig,
							config->max_osig);

		ARRAY_FOR_EACH(test_osignal_insigs, i) {
			control_system_signal_osignal_sample(&test_osignal,
							     test_osignal_insigs[i],
							     &osig);

			test_assert_within_int64(osig, config->osigs[i], config->threshold);
		}
	}
}

struct test_insignal_config {
	int64_t min_insig;
	int64_t max_insig;
	int64_t threshold;
	int64_t insigs[5];
};

static struct control_system_signal_insignal test_insignal;

static const int64_t test_insignal_osigs[] = {
	INT32_MIN,
	INT32_MIN / 2,
	0,
	INT32_MAX / 2,
	INT32_MAX,
};

static const struct test_insignal_config test_insignal_configs[] = {
	{
		.min_insig = INT32_MIN,
		.max_insig = INT32_MAX,
		.threshold = 4,
		.insigs = {
			INT32_MIN,
			INT32_MIN / 2,
			0,
			INT32_MAX / 2,
			INT32_MAX,
		},
	},
	{
		.min_insig = INT64_MIN,
		.max_insig = INT64_MAX,
		.threshold = 4,
		.insigs = {
			INT64_MIN,
			INT64_MIN / 2,
			0,
			INT64_MAX / 2,
			INT64_MAX,
		},
	},
	{
		.min_insig = INT32_MIN,
		.max_insig = INT32_MAX,
		.threshold = 4,
		.insigs = {
			((int64_t)INT32_MIN) * 2,
			INT32_MIN / 2,
			0,
			INT32_MAX / 2,
			((int64_t)INT32_MAX) * 2,
		},
	},
	{
		.min_insig = ((int64_t)INT32_MIN) - 1000000,
		.max_insig = INT32_MAX,
		.threshold = 4,
		.insigs = {
			((int64_t)INT32_MIN) - 1000000,
			INT32_MIN / 2 - 750000,
			-500000,
			INT32_MAX / 2 - 250000,
			INT32_MAX,
		},
	},
};

static void test_assert_within_q31(q31_t a, q31_t b, q31_t d)
{
	if (a <= INT32_MIN + d) {
		zassert_between_inclusive(a, INT32_MIN, b + d);
	} else if (a >= INT32_MAX - d) {
		zassert_between_inclusive(a, b - d, INT32_MAX);
	} else {
		zassert_within(a, b, d);
	}
}

ZTEST(control_system_signal, test_insignal)
{
	q31_t osig;

	ARRAY_FOR_EACH_PTR(test_insignal_configs, config) {
		control_system_signal_insignal_configure(&test_insignal,
							 config->min_insig,
							 config->max_insig);

		ARRAY_FOR_EACH(test_insignal_osigs, i) {
			control_system_signal_insignal_sample(&test_insignal,
							      config->insigs[i],
							      &osig);

			test_assert_within_q31(osig, test_insignal_osigs[i], config->threshold);
		}
	}
}
