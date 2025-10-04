# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_ACTUATOR actuator)
add_subdirectory_ifdef(CONFIG_INPUT input)
add_subdirectory_ifdef(CONFIG_LED led)
add_subdirectory_ifdef(CONFIG_SENSOR sensor)
add_subdirectory_ifdef(CONFIG_ZVB_BUS zvb_bus)
