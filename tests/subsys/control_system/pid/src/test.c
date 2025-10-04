/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zvb/control_system/control/pid.h>
#include <zephyr/dsp/dsp.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

CONTROL_SYSTEM_CONTROL_PID_DEFINE(test_cs)

static void *test_setup(void)
{
	return NULL;
}

static void test_before(void *f)
{
	ARG_UNUSED(f);
}

ZTEST_SUITE(control_system_pid, NULL, test_setup, test_before, NULL, NULL);

ZTEST(control_system_pid, test_p)
{
	control_system_control_pid_configure(&test_cs, 0, 0, 0, 0, 0, 0);
	q31_t out;
	zassert_ok(control_system_sample(&test_cs, &out));
}
