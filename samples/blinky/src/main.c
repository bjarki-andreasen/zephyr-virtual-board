/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>

static const struct device *led0 = DEVICE_DT_GET(DT_NODELABEL(led0));

int main(void)
{
	k_msleep(1000);

	while (1) {
		led_on(led0, 0);
		k_msleep(500);
		led_off(led0, 0);
		k_msleep(500);
	}

	return 0;
}
