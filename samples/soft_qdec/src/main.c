/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/atomic.h>

static const struct gpio_dt_spec phase_a = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), phase_a_gpios);
static const struct gpio_dt_spec phase_b = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), phase_b_gpios);
static const struct device *ipc_dev = DEVICE_DT_GET(DT_PROP(DT_PATH(zephyr_user), ipc));
static uint32_t phases_last;
static uint32_t phases_current;
static atomic_t accumulated;
static struct ipc_ept ep;

static void get_current_phases(void)
{
	phases_current = (gpio_pin_get_dt(&phase_a) << 1) | gpio_pin_get_dt(&phase_b);
}

static void update_last_phases(void)
{
	phases_last = phases_current;
}

static bool phases_changed(void)
{
	return phases_current != phases_last;
}

static void accumulate_step(void)
{
	switch (phases_last) {
	case 0:
		if (phases_current == 1) {
			atomic_inc(&accumulated);
		} else if (phases_current == 2) {
			atomic_dec(&accumulated);
		}

		break;

	case 1:
		if (phases_current == 3) {
			atomic_inc(&accumulated);
		} else if (phases_current == 0) {
			atomic_dec(&accumulated);
		}

		break;

	case 2:
		if (phases_current == 0) {
			atomic_inc(&accumulated);
		} else if (phases_current == 3) {
			atomic_dec(&accumulated);
		}

		break;

	case 3:
		if (phases_current == 2) {
			atomic_inc(&accumulated);
		} else if (phases_current == 1) {
			atomic_dec(&accumulated);
		}

		break;

	default:
		break;
	}
}

static void ep_recv(const void *data, size_t len, void *priv)
{
	uint8_t ser[sizeof(int32_t)];

	sys_put_le32(accumulated, ser);
	atomic_set(&accumulated, 0);
	ipc_service_send(&ep, ser, sizeof(ser));
}

static const struct ipc_ept_cfg ep_cfg = {
	.cb = {
		.received = ep_recv,
	},
};

int main(void)
{
	int ret;

	gpio_pin_configure_dt(&phase_a, GPIO_INPUT);
	gpio_pin_configure_dt(&phase_b, GPIO_INPUT);

	get_current_phases();
	update_last_phases();

	ret = ipc_service_open_instance(ipc_dev);
	if (ret < 0) {
		return ret;
	}

	ret = ipc_service_register_endpoint(ipc_dev, &ep, &ep_cfg);
	if (ret < 0) {
		return ret;
	}

	while (1) {
		get_current_phases();

		if (!phases_changed()) {
			continue;
		}

		accumulate_step();
		update_last_phases();
	}

	return 0;
}
