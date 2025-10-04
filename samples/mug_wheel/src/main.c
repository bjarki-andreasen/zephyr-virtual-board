/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/dsp/dsp.h>
#include <zephyr/kernel.h>
#include <zephyr/rtio/rtio.h>
#include <zvb/drivers/actuator.h>
#include <zvb/control_system/control/pid.h>

#include <math.h>

#define APP_WHEEL_ENCODER_SENSOR_NODE \
	DT_PROP(DT_PATH(zephyr_user), wheel_encoder_sensor)

#define APP_WHEEL_ENCODER_SENSOR_DEVICE \
	DEVICE_DT_GET(APP_WHEEL_ENCODER_SENSOR_NODE)

#define APP_WHEEL_ENCODER_SENSOR_DECODER \
	SENSOR_DECODER_DT_GET(APP_WHEEL_ENCODER_SENSOR_NODE)

#define APP_IMU_SENSOR_NODE \
	DT_PROP(DT_PATH(zephyr_user), imu_sensor)

#define APP_IMU_SENSOR_DEVICE \
	DEVICE_DT_GET(APP_IMU_SENSOR_NODE)

#define APP_IMU_SENSOR_DECODER \
	SENSOR_DECODER_DT_GET(APP_IMU_SENSOR_NODE)

#define APP_WHEEL_ACTUATOR_NODE \
	DT_PROP(DT_PATH(zephyr_user), wheel_actuator)

#define APP_WHEEL_ACTUATOR_DEVICE \
	DEVICE_DT_GET(APP_WHEEL_ACTUATOR_NODE)

#define APP_ARM_ACTUATOR_NODE \
	DT_PROP(DT_PATH(zephyr_user), arm_actuator)

#define APP_ARM_ACTUATOR_DEVICE \
	DEVICE_DT_GET(APP_ARM_ACTUATOR_NODE)

#define APP_CS_INTERVAL_MS 50
#define APP_SENSOR_LEAD_MS 40
#define APP_CS_INTERVAL_S_Q31 2147483647 / (MSEC_PER_SEC / APP_CS_INTERVAL_MS)

RTIO_DEFINE_WITH_MEMPOOL(rtio, 4, 4, 4, 64, 4);

RTIO_DEFINE_WITH_MEMPOOL(wheel_encoder_sensor_rtio, 2, 2, 2, 32, 4);

static struct sensor_chan_spec wheel_encoder_sensor_channel = {
	.chan_type = SENSOR_CHAN_ROTATION,
	.chan_idx = 0,
};

static struct sensor_read_config wheel_encoder_sensor_iodev_read_config = {
	.sensor = APP_WHEEL_ENCODER_SENSOR_DEVICE,
	.is_streaming = false,
	.channels = &wheel_encoder_sensor_channel,
	.count = 1,
	.max = 1,
};

RTIO_IODEV_DEFINE(
	wheel_encoder_sensor_iodev,
	&__sensor_iodev_api,
	&wheel_encoder_sensor_iodev_read_config
);

static const struct sensor_decoder_api *wheel_encoder_sensor_decoder =
	APP_WHEEL_ENCODER_SENSOR_DECODER;

static struct sensor_chan_spec imu_sensor_channels[] = {
	{
		.chan_type = SENSOR_CHAN_GYRO_XYZ,
		.chan_idx = 0,
	},
	{
		.chan_type = SENSOR_CHAN_ACCEL_XYZ,
		.chan_idx = 0,
	},
};

static struct sensor_read_config imu_sensor_iodev_read_config = {
	.sensor = APP_IMU_SENSOR_DEVICE,
	.is_streaming = false,
	.channels = imu_sensor_channels,
	.count = 2,
	.max = 2,
};

RTIO_IODEV_DEFINE(
	imu_sensor_iodev,
	&__sensor_iodev_api,
	&imu_sensor_iodev_read_config
);

static const struct sensor_decoder_api *imu_sensor_decoder =
	APP_IMU_SENSOR_DECODER;

static const struct device *wheel_actuator = APP_WHEEL_ACTUATOR_DEVICE;
static const struct device *arm_actuator = APP_ARM_ACTUATOR_DEVICE;

CONTROL_SYSTEM_CONTROL_PID_DEFINE(rotation_z_pid);
CONTROL_SYSTEM_CONTROL_PID_DEFINE(rotation_x_pid);
CONTROL_SYSTEM_CONTROL_PID_DEFINE(velocity_z_pid);
CONTROL_SYSTEM_CONTROL_PID_DEFINE(lean_x_pid);

K_SEM_DEFINE(cs_ready_sem, 0, 1);

static q31_t tilt_from_accel(q31_t vertical, q31_t horizontal)
{
	double adjacent = (double)(vertical);
	double opposite = (double)horizontal;

	if (opposite == 0) {
		return 0;
	}

	return (q31_t)(atan(opposite/adjacent) * INT32_MAX);
}

static void cs_thread_routine(void *p1, void *p2, void *p3)
{
	int64_t ticks;
	struct rtio_sqe *sqe;
	struct rtio_cqe *cqe;
	uint8_t *buf;
	uint32_t buf_len;
	uint32_t decoder_fit;
	int ret;
	struct sensor_three_axis_data sensor3;
	struct sensor_q31_data sensor1;
	q31_t interval_seconds = APP_CS_INTERVAL_S_Q31;
	q31_t rotation_z = 0;
	q31_t rotation_x = 0;
	q31_t wheel_angular_velocity_z = 0;
	q31_t sample;
	q31_t last_arm_sample = 0;

	k_sem_take(&cs_ready_sem, K_FOREVER);

	ticks = k_uptime_ticks();

	while (true) {
		ticks += k_ms_to_ticks_floor64(APP_CS_INTERVAL_MS - APP_SENSOR_LEAD_MS);
		k_sleep(K_TIMEOUT_ABS_TICKS(ticks));

		sqe = rtio_sqe_acquire(&wheel_encoder_sensor_rtio);
		__ASSERT_NO_MSG(sqe != NULL);
		rtio_sqe_prep_read_with_pool(sqe, &wheel_encoder_sensor_iodev, RTIO_PRIO_HIGH, NULL);
		rtio_submit(&wheel_encoder_sensor_rtio, 0);

		sqe = rtio_sqe_acquire(&rtio);
		__ASSERT_NO_MSG(sqe != NULL);
		rtio_sqe_prep_read_with_pool(sqe, &imu_sensor_iodev, RTIO_PRIO_HIGH, NULL);
		rtio_submit(&rtio, 0);

		ticks += k_ms_to_ticks_floor64(APP_SENSOR_LEAD_MS);
		k_sleep(K_TIMEOUT_ABS_TICKS(ticks));

		cqe = rtio_cqe_consume(&wheel_encoder_sensor_rtio);
		__ASSERT_NO_MSG(cqe != NULL);
		__ASSERT_NO_MSG(cqe->result == 0);
		buf = NULL;
		buf_len = 0;
		rtio_cqe_get_mempool_buffer(&wheel_encoder_sensor_rtio, cqe, &buf, &buf_len);
		__ASSERT_NO_MSG(buf != NULL);
		__ASSERT_NO_MSG(buf_len > 0);
		rtio_cqe_release(&wheel_encoder_sensor_rtio, cqe);

		decoder_fit = 0;
		ret = wheel_encoder_sensor_decoder->decode(
			buf,
			wheel_encoder_sensor_channel,
			&decoder_fit, 1,
			&sensor1
		);
		__ASSERT_NO_MSG(ret == 1);
		rtio_release_buffer(&wheel_encoder_sensor_rtio, buf, buf_len);

		zdsp_mult_q31(
			&sensor1.readings->value,
			&interval_seconds,
			&wheel_angular_velocity_z,
			1
		);

		cqe = rtio_cqe_consume(&rtio);
		__ASSERT_NO_MSG(cqe != NULL);
		__ASSERT_NO_MSG(cqe->result == 0);
		buf = NULL;
		buf_len = 0;
		rtio_cqe_get_mempool_buffer(&rtio, cqe, &buf, &buf_len);
		__ASSERT_NO_MSG(buf != NULL);
		__ASSERT_NO_MSG(buf_len > 0);
		rtio_cqe_release(&rtio, cqe);

#if 0
		decoder_fit = 0;
		ret = imu_sensor_decoder->decode(buf,
						 imu_sensor_channels[1],
						 &decoder_fit, 1,
						 &sensor3);
		__ASSERT_NO_MSG(ret == 1);

		tilt_z = tilt_from_accel(sensor3.readings->y, sensor3.readings->x);
		tilt_x = tilt_from_accel(sensor3.readings->y, sensor3.readings->z);
#endif

		decoder_fit = 0;
		ret = imu_sensor_decoder->decode(
			buf,
			imu_sensor_channels[0],
			&decoder_fit, 1,
			&sensor3
		);
		__ASSERT_NO_MSG(ret == 1);
		rtio_release_buffer(&rtio, buf, buf_len);

		control_system_set_process_var(&velocity_z_pid, wheel_angular_velocity_z);
		control_system_sample(&velocity_z_pid, &sample);

		control_system_set_setpoint(&rotation_z_pid, 0);

		zdsp_mult_q31(&sensor3.readings->z, &interval_seconds, &sample, 1);
		zdsp_add_q31(&rotation_z, &sample, &rotation_z, 1);
		zdsp_mult_q31(&sensor3.readings->x, &interval_seconds, &sample, 1);
		zdsp_add_q31(&rotation_x, &sample, &rotation_x, 1);

		control_system_set_process_var(&rotation_z_pid, rotation_z);
		control_system_set_process_var(&rotation_x_pid, rotation_x);
		control_system_set_process_var(&lean_x_pid, last_arm_sample);
		control_system_sample(&rotation_z_pid, &sample);
		actuator_set_setpoint(wheel_actuator, sample);
		control_system_sample(&lean_x_pid, &sample);
		control_system_set_setpoint(&rotation_x_pid, sample);
		control_system_sample(&rotation_x_pid, &sample);
		actuator_set_setpoint(arm_actuator, 0);
		last_arm_sample = sample;
	}
}

K_KERNEL_THREAD_DEFINE(
	cs_thread,
	4096,
	cs_thread_routine,
	NULL,
	NULL,
	NULL,
	0,
	0,
	0
);

int main(void)
{
	actuator_set_setpoint(wheel_actuator, 0);

	control_system_control_pid_configure(&rotation_z_pid,
					     175000000,
					     10,
					     300000000,
					     3,
					     0,
					     0);

	control_system_control_pid_configure(&rotation_x_pid,
					     375000000,
					     7,
					     240000000,
					     4,
					     0,
					     0);

	control_system_control_pid_configure(&velocity_z_pid,
					     15000000,
					     0,
					     8000000,
					     2,
					     0,
					     0);

	control_system_control_pid_configure(&lean_x_pid,
					     3200000,
					     4,
					     600000,
					     0,
					     0,
					     0);

	k_sem_give(&cs_ready_sem);

	k_msleep(1000);
	control_system_set_setpoint(&rotation_z_pid, 0);

	return 0;
}
