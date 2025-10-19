/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zvb/control_system/types.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

ZTEST_SUITE(control_system_types, NULL, NULL, NULL, NULL, NULL);

ZTEST(control_system_types, test_conversions)
{
	zassert_equal(CONTROL_SYSTEM_NANO(1000000000), INT32_MAX);
	zassert_equal(CONTROL_SYSTEM_NANO(-1000000000), INT32_MIN + 1);
	zassert_equal(CONTROL_SYSTEM_NANO(0), 0);
	zassert_equal(CONTROL_SYSTEM_MICRO(1000000), INT32_MAX);
	zassert_equal(CONTROL_SYSTEM_MICRO(-1000000), INT32_MIN + 1);
	zassert_equal(CONTROL_SYSTEM_MICRO(0), 0);
	zassert_equal(CONTROL_SYSTEM_MILLI(1000), INT32_MAX);
	zassert_equal(CONTROL_SYSTEM_MILLI(-1000), INT32_MIN + 1);
	zassert_equal(CONTROL_SYSTEM_MILLI(0), 0);
	zassert_equal(CONTROL_SYSTEM_NANO_SHIFTED(1000000000, 0), INT32_MAX);
	zassert_equal(CONTROL_SYSTEM_NANO_SHIFTED(-1000000000, 0), INT32_MIN + 1);
	zassert_equal(CONTROL_SYSTEM_NANO_SHIFTED(0, 0), 0);
	zassert_equal(CONTROL_SYSTEM_NANO_SHIFTED(4000000000, 2), INT32_MAX);
	zassert_equal(CONTROL_SYSTEM_NANO_SHIFTED(-4000000000, 2), INT32_MIN + 1);
	zassert_equal(CONTROL_SYSTEM_NANO_SHIFTED(0, 2), 0);
	zassert_equal(CONTROL_SYSTEM_MICRO_SHIFTED(1000000, 0), INT32_MAX);
	zassert_equal(CONTROL_SYSTEM_MICRO_SHIFTED(-1000000, 0), INT32_MIN + 1);
	zassert_equal(CONTROL_SYSTEM_MICRO_SHIFTED(0, 0), 0);
	zassert_equal(CONTROL_SYSTEM_MICRO_SHIFTED(4000000, 2), INT32_MAX);
	zassert_equal(CONTROL_SYSTEM_MICRO_SHIFTED(-4000000, 2), INT32_MIN + 1);
	zassert_equal(CONTROL_SYSTEM_MICRO_SHIFTED(0, 2), 0);
	zassert_equal(CONTROL_SYSTEM_MILLI_SHIFTED(1000, 0), INT32_MAX);
	zassert_equal(CONTROL_SYSTEM_MILLI_SHIFTED(-1000, 0), INT32_MIN + 1);
	zassert_equal(CONTROL_SYSTEM_MILLI_SHIFTED(0, 0), 0);
	zassert_equal(CONTROL_SYSTEM_MILLI_SHIFTED(4000, 2), INT32_MAX);
	zassert_equal(CONTROL_SYSTEM_MILLI_SHIFTED(-4000, 2), INT32_MIN + 1);
	zassert_equal(CONTROL_SYSTEM_MILLI_SHIFTED(0, 2), 0);
}
