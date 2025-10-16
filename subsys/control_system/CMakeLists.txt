# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

add_subdirectory_ifdef(CONFIG_CONTROL_SYSTEM_CONTROL control)
add_subdirectory_ifdef(CONFIG_CONTROL_SYSTEM_SIGNAL signal)
add_subdirectory_ifdef(CONFIG_CONTROL_SYSTEM_TRANSFER transfer)

zephyr_library_sources_ifdef(CONFIG_CONTROL_SYSTEM_SHELL control_system_shell.c)
zephyr_library_sources(control_system.c)

zephyr_iterable_section(NAME control_system GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT})
zephyr_linker_sources(DATA_SECTIONS control_system.ld)
