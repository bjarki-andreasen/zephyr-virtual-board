# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(transfer.c)
zephyr_library_sources_ifdef(CONFIG_CONTROL_SYSTEM_TRANSFER_DIFFERENTIAL differential.c)
zephyr_library_sources_ifdef(CONFIG_CONTROL_SYSTEM_TRANSFER_LINEAR linear.c)
zephyr_library_sources_ifdef(CONFIG_CONTROL_SYSTEM_TRANSFER_INTEGRAL integral.c)
