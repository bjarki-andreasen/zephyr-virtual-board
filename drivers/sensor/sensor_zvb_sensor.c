/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zvb/drivers/zvb_bus.h>

#define DT_DRV_COMPAT zvb_sensor

LOG_MODULE_REGISTER(zvb_sensor, CONFIG_SENSOR_LOG_LEVEL);

__packed struct driver_transmit_data {
	uint32_t channel_type;
	uint32_t channel_index;
};

struct driver_data {
	struct zvb_bus_receive_callback callback;
	const struct device *const dev;
	struct mpsc iodev_sqe_q;
	struct rtio_iodev_sqe *txn_head;
	struct rtio_iodev_sqe *txn_curr;
	struct k_sem lock;
	struct driver_transmit_data tx_data[CONFIG_SENSOR_ZVB_SENSOR_MAX_CHANNELS];
};

struct driver_config {
	const struct device *bus;
	uint8_t addr;
};

__packed struct driver_buffer_data_header {
	uint64_t base_timestamp_ns;
	uint32_t frame_count;
};

__packed struct driver_buffer_data_frame {
	uint32_t channel_type;
	uint32_t channel_index;
	uint32_t shift;
	q31_t readings[3];
};

__packed struct driver_buffer_data {
	struct driver_buffer_data_header header;
	struct driver_buffer_data_frame frames[];
};

static void driver_lock(const struct device *dev)
{
	struct driver_data *dev_data = dev->data;

	k_sem_take(&dev_data->lock, K_FOREVER);
}

static void driver_unlock(const struct device *dev)
{
	struct driver_data *dev_data = dev->data;

	k_sem_give(&dev_data->lock);
}

static void driver_txn_start_locked(const struct device *dev)
{
	struct driver_data *dev_data = dev->data;
	const struct driver_config *dev_config = dev->config;
	struct rtio_iodev_sqe *iodev_sqe = dev_data->txn_curr;
	const struct sensor_read_config *read_config = iodev_sqe->sqe.iodev->data;
	uint16_t count;
	size_t tx_data_size;

	if (ARRAY_SIZE(dev_data->tx_data) < read_config->count) {
		count = ARRAY_SIZE(dev_data->tx_data);
		LOG_WRN("read_config count limited to %u", count);
	} else {
		count = read_config->count;
	}

	tx_data_size = 0;
	for (uint16_t i = 0; i < count; i++) {
		dev_data->tx_data[i].channel_type = read_config->channels[i].chan_type;
		dev_data->tx_data[i].channel_index = read_config->channels[i].chan_idx;
		tx_data_size += sizeof(struct driver_transmit_data);
	}

	zvb_bus_transmit(dev_config->bus,
			 dev_config->addr,
			 (const uint8_t *)dev_data->tx_data,
			 tx_data_size);
}

static void driver_sqe_next_locked(const struct device *dev)
{
	struct driver_data *dev_data = dev->data;
	struct mpsc_node *node;
	struct rtio_iodev_sqe *iodev_sqe;

	dev_data->txn_head = NULL;
	dev_data->txn_curr = NULL;

	node = mpsc_pop(&dev_data->iodev_sqe_q);
	if (node == NULL) {
		return;
	}

	iodev_sqe = CONTAINER_OF(node, struct rtio_iodev_sqe, q);

	dev_data->txn_head = iodev_sqe;
	dev_data->txn_curr = iodev_sqe;

	driver_txn_start_locked(dev);
}

static void driver_sqe_cancel_locked(const struct device *dev,
				     struct rtio_iodev_sqe **iodev_sqe,
				     int *result)
{
	struct driver_data *dev_data = dev->data;

	*iodev_sqe = dev_data->txn_head;
	*result = -EIO;

	driver_sqe_next_locked(dev);
}

static void driver_txn_next_locked(const struct device *dev,
				   struct rtio_iodev_sqe **iodev_sqe,
				   int *result)
{
	struct driver_data *dev_data = dev->data;

	dev_data->txn_curr = rtio_txn_next(dev_data->txn_curr);
	if (dev_data->txn_curr != NULL) {
		driver_txn_start_locked(dev);
		return;
	}

	*iodev_sqe = dev_data->txn_head;
	*result = 0;

	driver_sqe_next_locked(dev);
}

static void driver_txn_receive_locked(const struct device *dev,
				      const uint8_t *data,
				      size_t size,
				      struct rtio_iodev_sqe **iodev_sqe,
				      int *result)
{
	struct driver_data *dev_data = dev->data;
	uint64_t base_timestamp_ns;
	uint8_t *rx_buf;
	uint32_t rx_buf_len;
	struct driver_buffer_data *buf_data;
	const uint8_t *data_it;
	struct driver_buffer_data_frame *frame_it;

	if (dev_data->txn_head == NULL) {
		return;
	}

	if (size == 0 || (size % sizeof(struct driver_buffer_data_frame))) {
		driver_sqe_cancel_locked(dev, iodev_sqe, result);
		return;
	}

	base_timestamp_ns = k_ticks_to_ns_floor64(k_uptime_ticks());

	if (rtio_sqe_rx_buf(dev_data->txn_curr, size, size, &rx_buf, &rx_buf_len)) {
		driver_sqe_cancel_locked(dev, iodev_sqe, result);
		return;
	}

	buf_data = (struct driver_buffer_data *)rx_buf;
	buf_data->header.base_timestamp_ns = base_timestamp_ns;
	buf_data->header.frame_count = 0;

	data_it = data;
	frame_it = buf_data->frames;
	while (data_it != data + size) {
		frame_it->channel_type = sys_get_le32(data_it);
		data_it += sizeof(uint32_t);
		frame_it->channel_index = sys_get_le32(data_it);
		data_it += sizeof(uint32_t);
		frame_it->shift = sys_get_le32(data_it);
		data_it += sizeof(uint32_t);
		frame_it->readings[0] = sys_get_le32(data_it);
		data_it += sizeof(uint32_t);
		frame_it->readings[1] = sys_get_le32(data_it);
		data_it += sizeof(uint32_t);
		frame_it->readings[2] = sys_get_le32(data_it);
		data_it += sizeof(uint32_t);
		frame_it++;
		buf_data->header.frame_count++;
	}

	driver_txn_next_locked(dev, iodev_sqe, result);
}

static void driver_receive_handler(const struct device *bus,
				   const struct zvb_bus_receive_callback *callback,
				   const uint8_t *data,
				   size_t size)
{
	struct driver_data *dev_data = CONTAINER_OF(callback, struct driver_data, callback);
	const struct device *dev = dev_data->dev;
	struct rtio_iodev_sqe *iodev_sqe = NULL;
	int result = 0;

	driver_lock(dev);
	driver_txn_receive_locked(dev, data, size, &iodev_sqe, &result);
	driver_unlock(dev);

	if (iodev_sqe == NULL) {
		return;
	}

	if (result < 0) {
		rtio_iodev_sqe_err(iodev_sqe, result);
	} else {
		rtio_iodev_sqe_ok(iodev_sqe, result);
	}
}

static void driver_submit_locked(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct driver_data *dev_data = dev->data;

	mpsc_push(&dev_data->iodev_sqe_q, &iodev_sqe->q);

	if (dev_data->txn_head != NULL) {
		return;
	}

	driver_sqe_next_locked(dev);
}

static void driver_api_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	driver_lock(dev);
	driver_submit_locked(dev, iodev_sqe);
	driver_unlock(dev);
}

static int driver_decoder_get_frame_count(const uint8_t *buffer,
					  struct sensor_chan_spec channel,
					  uint16_t *frame_count)
{
	ARG_UNUSED(buffer);
	ARG_UNUSED(channel);

	*frame_count = 1;
	return 0;
}

static int driver_decoder_get_size_info(struct sensor_chan_spec channel,
					size_t *base_size,
					size_t *frame_size)
{
	switch (channel.chan_type) {
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_GYRO_XYZ:
		*base_size = 38;
		*frame_size = 38;
		break;

	case SENSOR_CHAN_ROTATION:
		*base_size = 30;
		*frame_size = 30;
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

static void driver_decoder_decode_three_axis(const struct driver_buffer_data *buf_data,
					     const struct driver_buffer_data_frame *frame,
					     void *data_out)
{
	struct sensor_three_axis_data *data = data_out;

	data->header.base_timestamp_ns = buf_data->header.base_timestamp_ns;
	data->header.reading_count = 1;
	data->shift = (int8_t)frame->shift;
	data->readings->x = frame->readings[0];
	data->readings->y = frame->readings[1];
	data->readings->z = frame->readings[2];
}

static void driver_decoder_decode_q31(const struct driver_buffer_data *buf_data,
				      const struct driver_buffer_data_frame *frame,
				      void *data_out)
{
	struct sensor_q31_data *data = data_out;

	data->header.base_timestamp_ns = buf_data->header.base_timestamp_ns;
	data->header.reading_count = 1;
	data->shift = (int8_t)frame->shift;
	data->readings->value = frame->readings[0];
}

static int driver_decoder_decode(const uint8_t *buffer,
				 struct sensor_chan_spec channel,
				 uint32_t *fit,
				 uint16_t max_count,
				 void *data_out)
{
	const struct driver_buffer_data *buf_data = (const struct driver_buffer_data *)buffer;
	const struct driver_buffer_data_frame *frame;

	ARG_UNUSED(max_count);

	if (*fit) {
		return 0;
	}

	frame = NULL;
	for (uint32_t i = 0; i < buf_data->header.frame_count; i++) {
		if (buf_data->frames[i].channel_type != channel.chan_type ||
		    buf_data->frames[i].channel_index != channel.chan_idx) {
			continue;
		}

		frame = &buf_data->frames[i];
		break;
	}

	if (frame == NULL) {
		return -ENOENT;
	}

	switch (frame->channel_type) {
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_GYRO_XYZ:
		driver_decoder_decode_three_axis(buf_data, frame, data_out);
		break;

	case SENSOR_CHAN_ROTATION:
		driver_decoder_decode_q31(buf_data, frame, data_out);
		break;

	default:
		return -ENOTSUP;
	}

	*fit = 1;
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

	if (!device_is_ready(dev_config->bus)) {
		return -ENODEV;
	}

	mpsc_init(&dev_data->iodev_sqe_q);
	k_sem_init(&dev_data->lock, 1, 1);

	return zvb_bus_add_receive_callback(dev_config->bus, &dev_data->callback);
}

#define DRIVER_INST_DEFINE(inst)								\
												\
	static struct driver_data data##inst = {						\
		.callback = {									\
			.addr = DT_INST_REG_ADDR(inst),						\
			.handler = driver_receive_handler,					\
		},										\
		.dev = DEVICE_DT_INST_GET(inst),						\
	};											\
												\
	static struct driver_config config##inst = {						\
		.bus = DEVICE_DT_GET(DT_INST_BUS(inst)),					\
		.addr = DT_INST_REG_ADDR(inst),							\
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
