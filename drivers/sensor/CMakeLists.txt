# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

zephyr_library_amend()
zephyr_library_sources_ifdef(CONFIG_SENSOR_ZVB_IMU sensor_zvb_imu.c)
zephyr_library_sources_ifdef(CONFIG_SENSOR_ZVB_SENSOR sensor_zvb_sensor.c)
