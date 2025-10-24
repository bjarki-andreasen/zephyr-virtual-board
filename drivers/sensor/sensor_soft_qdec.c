/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/atomic.h>

#define DT_DRV_COMPAT soft_qdec

struct driver_buffer_data {
	uint64_t base_timestamp_ns;
	int8_t shift;
	q31_t value;
};

struct driver_data {
	struct k_sem bound_sem;
	struct rtio_iodev_sqe *iodev_sqe;
	struct ipc_ept ep;
};

struct driver_config {
	const struct device *ipc;
	struct ipc_ept_cfg ep_cfg;
	uint32_t steps_per_rotation;
};

static void driver_ep_bound(void *priv)
{
	const struct device *dev = priv;
	struct driver_data *dev_data = dev->data;

	k_sem_give(&dev_data->bound_sem);
}

static void driver_ep_received(const void *data, size_t len, void *priv)
{
	const struct device *dev = priv;
	struct driver_data *dev_data = dev->data;
	const struct driver_config *dev_config = dev->config;
	int32_t accumulated;
	int ret;
	uint8_t *rx_buf;
	uint32_t rx_buf_len;
	struct driver_buffer_data *buffer_data;
	struct rtio_iodev_sqe *iodev_sqe;

	iodev_sqe = dev_data->iodev_sqe;
	dev_data->iodev_sqe = NULL;

	if (len != sizeof(accumulated)) {
		rtio_iodev_sqe_err(iodev_sqe, -EIO);
		return;
	}

	accumulated = sys_get_le32(data);
	ret = rtio_sqe_rx_buf(iodev_sqe,
			      sizeof(struct driver_buffer_data),
			      sizeof(struct driver_buffer_data),
			      &rx_buf,
			      &rx_buf_len);
	if (ret) {
		rtio_iodev_sqe_err(iodev_sqe, -EIO);
		return;
	}

	buffer_data = (struct driver_buffer_data *)rx_buf;
	buffer_data->base_timestamp_ns = k_ticks_to_ns_floor64(k_uptime_ticks());
	buffer_data->value = (INT32_MAX / dev_config->steps_per_rotation) * accumulated;
	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

static void driver_api_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct driver_data *dev_data = dev->data;
	const uint8_t data = 1;
	int ret;

	dev_data->iodev_sqe = iodev_sqe;

	ret = ipc_service_send(&dev_data->ep, &data, sizeof(data));
	if (ret == sizeof(data)) {
		return;
	}

	rtio_iodev_sqe_err(dev_data->iodev_sqe, -EIO);
}

static int driver_decoder_get_frame_count(const uint8_t *buffer,
					  struct sensor_chan_spec channel,
					  uint16_t *frame_count)
{
	ARG_UNUSED(buffer);

	if (channel.chan_type != SENSOR_CHAN_ROTATION || channel.chan_idx != 0) {
		*frame_count = 0;
		return -ENOENT;
	}

	*frame_count = 1;
	return 0;
}

static int driver_decoder_get_size_info(struct sensor_chan_spec channel,
					size_t *base_size,
					size_t *frame_size)
{
	ARG_UNUSED(channel);
	*base_size = sizeof(struct driver_buffer_data);
	*frame_size = sizeof(struct driver_buffer_data);
	return 0;
}

static int driver_decoder_decode(const uint8_t *buffer,
				 struct sensor_chan_spec channel,
				 uint32_t *fit,
				 uint16_t max_count,
				 void *data_out)
{
	const struct driver_buffer_data *buffer_data = (const struct driver_buffer_data *)buffer;
	struct sensor_q31_data *data = data_out;

	if (channel.chan_type != SENSOR_CHAN_ROTATION || channel.chan_idx != 0) {
		return -ENOENT;
	}

	ARG_UNUSED(max_count);

	if (*fit) {
		return 0;
	}

	data->header.base_timestamp_ns = buffer_data->base_timestamp_ns;
	data->header.reading_count = 1;
	data->shift = buffer_data->shift;
	data->readings->value = buffer_data->value;

	*fit = 1;
	return 1;
}

static bool driver_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	ARG_UNUSED(buffer);
	ARG_UNUSED(trigger);
	return false;
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = driver_decoder_get_frame_count,
	.get_size_info = driver_decoder_get_size_info,
	.decode = driver_decoder_decode,
	.has_trigger = driver_decoder_has_trigger,
};

static int driver_api_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();
	return 0;
}

static DEVICE_API(sensor, driver_api) = {
	.submit = driver_api_submit,
	.get_decoder = driver_api_get_decoder,
};

static int driver_init(const struct device *dev)
{
	struct driver_data *dev_data = dev->data;
	const struct driver_config *dev_config = dev->config;
	int ret;

	k_sem_init(&dev_data->bound_sem, 0, 1);

	if (!device_is_ready(dev_config->ipc)) {
		return -ENODEV;
	}

	ret = ipc_service_open_instance(dev_config->ipc);
	if (ret < 0) {
		return ret;
	}

	ret = ipc_service_register_endpoint(dev_config->ipc,
					    &dev_data->ep,
					    &dev_config->ep_cfg);
	if (ret < 0) {
		return ret;
	}

	k_sem_take(&dev_data->bound_sem, K_FOREVER);
	return 0;
}

#define DRIVER_INST_DEFINE(inst)								\
	static struct driver_data data##inst;							\
	static struct driver_config config##inst = {						\
		.ipc = DEVICE_DT_GET(DT_INST_PROP(inst, ipc)),					\
		.ep_cfg = {									\
			.cb = {									\
				.bound = driver_ep_bound,					\
				.received = driver_ep_received,					\
			},									\
			.priv = (void *)DEVICE_DT_INST_GET(inst)				\
		},										\
		.steps_per_rotation = DT_INST_PROP(inst, steps_per_rotation),			\
	};											\
												\
	DEVICE_DT_INST_DEFINE(									\
		inst,										\
		driver_init,									\
		NULL,										\
		&data##inst,									\
		&config##inst,									\
		POST_KERNEL,									\
		UTIL_INC(CONFIG_IPC_SERVICE_REG_BACKEND_PRIORITY),				\
		&driver_api									\
	);

DT_INST_FOREACH_STATUS_OKAY(DRIVER_INST_DEFINE)
