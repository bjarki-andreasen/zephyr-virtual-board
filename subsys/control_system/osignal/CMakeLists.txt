# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(osignal.c)
zephyr_library_sources_ifdef(CONFIG_CONTROL_SYSTEM_OSIGNAL_PWM pwm.c)
