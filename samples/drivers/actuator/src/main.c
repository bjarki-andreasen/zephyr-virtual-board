/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zvb/drivers/actuator.h>

const struct device *sample_actuator = DEVICE_DT_GET(DT_ALIAS(sample_actuator));

static const q31_t sample_setpoints[] = {
	INT32_MIN,
	INT32_MIN / 2,
	0,
	INT32_MAX / 2,
	INT32_MAX,
	INT32_MIN,
	INT32_MAX,
};

static const int32_t sample_interval_ms = 2000;

int main(void)
{
	int ret;

	if (!device_is_ready(sample_actuator)) {
		printk("%s not ready\n", sample_actuator->name);
		return 0;
	}

	while (1) {
		ARRAY_FOR_EACH(sample_setpoints, i) {
			printk("setting setpoint %i\n", sample_setpoints[i]);
			ret = actuator_set_setpoint(sample_actuator, sample_setpoints[i]);
			if (ret) {
				printk("failed to set setpoint (ret = %i)\n", ret);
				return 0;
			}

			k_msleep(sample_interval_ms);
		}
	}

	return 0;
}
