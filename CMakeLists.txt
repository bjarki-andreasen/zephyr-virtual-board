# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_include_directories(include)
zephyr_include_directories(include)
add_subdirectory(drivers)
add_subdirectory(subsys)
