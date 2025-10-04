/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/socket.h>
#include <zvb/drivers/zvb_bus.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <poll.h>

#define DT_DRV_COMPAT zvb_zvb_bus

LOG_MODULE_REGISTER(zvb_zvb_bus, CONFIG_ZVB_BUS_LOG_LEVEL);

#define DRIVER_PING_ADDRESS 0xFF

static K_SEM_DEFINE(receive_sem, 1, 1);
static K_SEM_DEFINE(transmit_sem, 0, 1);
static sys_slist_t callbacks;
static uint8_t receive_buf[CONFIG_ZVB_BUS_ZVB_TRANSFER_BUF_SIZE];
static uint8_t transmit_buf[CONFIG_ZVB_BUS_ZVB_TRANSFER_BUF_SIZE];
static uint32_t tick;
static int socket_fd;
static struct sockaddr_in socket_addr;
static struct k_work_delayable driver_ping_dwork;

static inline int driver_socket_send(const uint8_t *data, size_t size)
{
	return zsock_sendto(socket_fd,
			    data,
			    size,
			    0,
			    (struct sockaddr *)&socket_addr,
			    sizeof(socket_addr));
}

static void driver_ping_dwork_handler(struct k_work *work)
{
	uint8_t data[sizeof(uint8_t) + sizeof(uint32_t)];

	data[0] = DRIVER_PING_ADDRESS;
	sys_put_le32(tick, &data[1]);
	tick++;
	driver_socket_send(data, sizeof(data));
	k_work_schedule(&driver_ping_dwork, K_MSEC(CONFIG_ZVB_BUS_ZVB_PING_INTERVAL_MS));
}

static int driver_api_add_receive_callback(const struct device *dev,
					   struct zvb_bus_receive_callback *callback)
{
	ARG_UNUSED(dev);

	if (callback->handler == NULL) {
		return -EINVAL;
	}

	k_sem_take(&receive_sem, K_FOREVER);
	sys_slist_find_and_remove(&callbacks, &callback->node);
	sys_slist_append(&callbacks, &callback->node);
	k_sem_give(&receive_sem);

	return 0;
}

static int driver_api_remove_receive_callback(const struct device *dev,
					      struct zvb_bus_receive_callback *callback)
{
	ARG_UNUSED(dev);

	k_sem_take(&receive_sem, K_FOREVER);
	sys_slist_find_and_remove(&callbacks, &callback->node);
	k_sem_give(&receive_sem);

	return 0;
}

static int driver_api_transmit(const struct device *dev,
			       uint8_t addr,
			       const uint8_t *data,
			       size_t size)
{
	int ret;

	ARG_UNUSED(dev);

	if (sizeof(transmit_buf) <= size) {
		return -ENOMEM;
	}

	k_sem_take(&transmit_sem, K_FOREVER);
	transmit_buf[0] = addr;
	memcpy(&transmit_buf[1], data, size);
	ret = driver_socket_send(transmit_buf, size + 1);
	k_sem_give(&transmit_sem);

	return ret;
}

static DEVICE_API(zvb_bus, driver_api) = {
	.add_receive_callback = driver_api_add_receive_callback,
	.remove_receive_callback = driver_api_remove_receive_callback,
	.transmit = driver_api_transmit,
};

static void handle_received_data(const uint8_t *data, size_t size)
{
	uint8_t msg_addr;
	const uint8_t *msg;
	size_t msg_size;
	struct zvb_bus_receive_callback *callback;

	if (size < 2) {
		LOG_WRN("Got too small packet");
		return;
	}

	msg_addr = data[0];
	msg = &data[1];
	msg_size = size - 1;

	LOG_DBG("Received packet: addr: %u", msg_addr);
	LOG_HEXDUMP_DBG(msg, msg_size, "data: ");

	k_sem_take(&receive_sem, K_FOREVER);
	SYS_SLIST_FOR_EACH_CONTAINER(&callbacks, callback, node) {
		if (callback->addr == msg_addr) {
			callback->handler(DEVICE_DT_INST_GET(0), callback, msg, msg_size);
		}
	}
	k_sem_give(&receive_sem);
}

static void driver_thread_routine(void *p1, void *p2, void *p3)
{
	int ret;
	struct zsock_pollfd pollfd;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	socket_fd = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (socket_fd < 0) {
		LOG_ERR("Failed to create socket");
		return;
	}

	socket_addr.sin_family = AF_INET;
	net_addr_pton(AF_INET, CONFIG_ZVB_BUS_ZVB_HOST_ADDR, &socket_addr.sin_addr);
	socket_addr.sin_port = htons(CONFIG_ZVB_BUS_ZVB_HOST_PORT);

	k_sem_give(&transmit_sem);

	k_work_init_delayable(&driver_ping_dwork, driver_ping_dwork_handler);
	k_work_schedule(&driver_ping_dwork, K_NO_WAIT);

	pollfd.fd = socket_fd;
	pollfd.events = POLLIN;

	while (1) {
		ret = zsock_poll(&pollfd, 1, -1);
		if (ret < 0) {
			LOG_ERR("Poll failed");
			return;
		}

		ret = zsock_recv(socket_fd, receive_buf, sizeof(receive_buf), 0);
		if (ret < 0) {
			LOG_ERR("Receive failed");
			break;
		}

		handle_received_data(receive_buf, ret);
	}
}

K_KERNEL_THREAD_DEFINE(
	zvb_bus_receive_thread,
	CONFIG_ZVB_BUS_ZVB_THREAD_STACK_SIZE,
	driver_thread_routine,
	NULL,
	NULL,
	NULL,
	CONFIG_ZVB_BUS_ZVB_THREAD_PRIORITY,
	0,
	0
);

DEVICE_DT_INST_DEFINE(
	0,
	NULL,
	NULL,
	NULL,
	NULL,
	POST_KERNEL,
	CONFIG_ZVB_BUS_ZVB_INIT_PRIORITY,
	&driver_api
);
