# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_ACTUATOR_DRI0042 actuator_dri0042.c)
zephyr_library_sources_ifdef(CONFIG_ACTUATOR_SERVO_PWM actuator_servo_pwm.c)
zephyr_library_sources_ifdef(CONFIG_ACTUATOR_ZVB actuator_zvb.c)
