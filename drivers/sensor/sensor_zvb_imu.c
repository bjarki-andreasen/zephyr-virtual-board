/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * We only support the ACCEL XYZ and GYRO XYZ types,
 * the DATA_READY trigger, and the streaming API.
 * This lets us simplify this driver to the absolute
 * bare minimum while being compatible with the sensor
 * streaming API.
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zvb/drivers/zvb_bus.h>

#define DT_DRV_COMPAT zvb_imu

#define RX_BUF_SIZE 12
#define CHAN_GROUP_SIZE 6
#define CHAN_PER_GROUP 3

LOG_MODULE_REGISTER(zvb_imu, CONFIG_SENSOR_LOG_LEVEL);

struct driver_data {
	struct zvb_bus_receive_callback callback;
	uint8_t addr;
	struct mpsc iodev_sqe_q;
};

struct driver_config {
	const struct device *bus;
};

static void driver_receive_handler(const struct device *bus,
				   const struct zvb_bus_receive_callback *callback,
				   const uint8_t *data,
				   size_t size)
{
	struct driver_data *dev_data = CONTAINER_OF(callback, struct driver_data, callback);
	struct mpsc_node *node;
	struct rtio_iodev_sqe *iodev_sqe;
	uint8_t *rx_buf;
	uint32_t rx_buf_len;

	if (size != RX_BUF_SIZE) {
		LOG_ERR("Invalid data received");
		return;
	}

	node = mpsc_pop(&dev_data->iodev_sqe_q);

	if (node == NULL) {
		return;
	}

	iodev_sqe = CONTAINER_OF(node, struct rtio_iodev_sqe, q);

	if (rtio_sqe_rx_buf(iodev_sqe, RX_BUF_SIZE, RX_BUF_SIZE, &rx_buf, &rx_buf_len)) {
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		LOG_ERR("Failed to alloc rx buf");
		return;
	}

	memcpy(rx_buf, data, RX_BUF_SIZE);
	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

static void driver_api_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct driver_data *dev_data = dev->data;
	const struct sensor_read_config *read_config = iodev_sqe->sqe.iodev->data;

	if (read_config->count != 1 ||
	    read_config->is_streaming == false ||
	    read_config->triggers[0].trigger != SENSOR_TRIG_DATA_READY ||
	    read_config->triggers[0].opt != SENSOR_STREAM_DATA_INCLUDE) {
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
		return;
	}

	mpsc_push(&dev_data->iodev_sqe_q, &iodev_sqe->q);
}

static bool driver_chan_spec_is_valid(struct sensor_chan_spec channel)
{
	if (channel.chan_type != SENSOR_CHAN_ACCEL_XYZ &&
	    channel.chan_type != SENSOR_CHAN_GYRO_XYZ) {
		return false;
	}

	if (channel.chan_idx != 0) {
		return false;
	}

	return true;
}

static int driver_decoder_get_frame_count(const uint8_t *buffer,
					  struct sensor_chan_spec channel,
					  uint16_t *frame_count)
{
	ARG_UNUSED(buffer);

	if (!driver_chan_spec_is_valid(channel)) {
		return -ENOENT;
	}

	*frame_count = 1;
	return 0;
}

static int driver_decoder_get_size_info(struct sensor_chan_spec channel,
					size_t *base_size,
					size_t *frame_size)
{
	if (!driver_chan_spec_is_valid(channel)) {
		return -ENOENT;
	}

	*base_size = sizeof(struct sensor_three_axis_data);
	*frame_size = sizeof(struct sensor_three_axis_data);
	return 0;
}

static int driver_decoder_decode(const uint8_t *buffer,
				 struct sensor_chan_spec channel,
				 uint32_t *fit,
				 uint16_t max_count,
				 void *data_out)
{
	struct sensor_three_axis_data *data = data_out;
	const uint8_t *pos;
	uint16_t lsbs;

	if (*fit) {
		return 0;
	}

	if (!driver_chan_spec_is_valid(channel)) {
		return -ENOENT;
	}

	if (max_count != 1) {
		return -ENOENT;
	}

	/*
	 * Note should get the timestamp and store it with the SQE buffer
	 * when we get it, but for simplicity we put it here.
	 */
	data->header.base_timestamp_ns = k_uptime_get() * NSEC_PER_MSEC;
	data->header.reading_count = 1;

	if (channel.chan_type == SENSOR_CHAN_ACCEL_XYZ) {
		pos = buffer;
		/* Value is +-16, which fits 2 ** 5, so no complex conversions needed */
		data->shift = 5;
	} else {
		pos = buffer + CHAN_GROUP_SIZE;
		/* Value is +-2048, which fits 2 ** 12, so no complex conversions needed */
		data->shift = 12;
	}

	for (uint8_t i = 0; i < CHAN_PER_GROUP; i++) {
		/* LSBs are stored in little endian in the buffer */
		lsbs = sys_get_le16(pos);
		/*
		 * Acc and gyro readings stored in 12 bit signed (11 bit)
		 * so shift up to 31 bit (q31)
		 */
		data->readings->v[i] = ((q31_t)lsbs) << 19;
		pos += sizeof(uint16_t);
	}

	*fit = CHAN_GROUP_SIZE;
	return 1;
}

static bool driver_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	ARG_UNUSED(buffer);
	return trigger == SENSOR_TRIG_DATA_READY;
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = driver_decoder_get_frame_count,
	.get_size_info = driver_decoder_get_size_info,
	.decode = driver_decoder_decode,
	.has_trigger = driver_decoder_has_trigger,
};

int driver_api_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
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

	if (!device_is_ready(dev_config->bus)) {
		return -ENODEV;
	}

	mpsc_init(&dev_data->iodev_sqe_q);

	return zvb_bus_add_receive_callback(dev_config->bus, &dev_data->callback);
}

#define DRIVER_INST_DEFINE(inst)								\
												\
	static struct driver_data data##inst = {						\
		.callback = {									\
			.addr = DT_INST_REG_ADDR(inst),						\
			.handler = driver_receive_handler,					\
		},										\
	};											\
												\
	static struct driver_config config##inst = {						\
		.bus = DEVICE_DT_GET(DT_INST_BUS(inst)),					\
	};											\
												\
	SENSOR_DEVICE_DT_INST_DEFINE(								\
		inst,										\
		driver_init,									\
		NULL,										\
		&data##inst,									\
		&config##inst,									\
		POST_KERNEL,									\
		UTIL_INC(CONFIG_ZVB_BUS_ZVB_INIT_PRIORITY),					\
		&driver_api									\
	);

DT_INST_FOREACH_STATUS_OKAY(DRIVER_INST_DEFINE)
