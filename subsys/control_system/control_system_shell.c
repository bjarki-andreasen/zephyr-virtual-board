/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zvb/control_system/control_system.h>
#include <zvb/control_system/variable.h>
#include <zephyr/dsp/dsp.h>
#include <zephyr/shell/shell.h>
#include <zephyr/kernel.h>

static bool monitor_enabled;

static void monitor_print_control_system_variables(const struct shell *sh)
{
	STRUCT_SECTION_FOREACH(control_system_variable, var) {
		shell_fprintf_normal(sh,
				     "[\"%s\",%" PRIu64 ",%" PRId32 "]\n",
				     control_system_variable_name_get(var),
				     k_ticks_to_us_floor64(k_uptime_ticks()),
				     control_system_variable_get(var));
	}
}

static void monitor_bypass_callback(const struct shell *sh, uint8_t *data, size_t len)
{
	ARG_UNUSED(sh);

	for (size_t i = 0; i < len; i++) {
		/* Disable monitor if ctrl+z or ctrl+c is entered */
		if (data[i] == 26 || data[i] == 3) {
			monitor_enabled = false;
			return;
		}
	}
}

static int cmd_list(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	STRUCT_SECTION_FOREACH(control_system_variable, var) {
		shell_print(sh, "%s", control_system_variable_name_get(var));
	}

	return 0;
}

static int cmd_monitor(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	int32_t interval_us;

	err = 0;
	interval_us = shell_strtol(argv[1], 10, &err);
	if (err) {
		shell_error(sh, "invalid interval: %s", argv[1]);
		return -EINVAL;
	}

	shell_set_bypass(sh, monitor_bypass_callback);
	monitor_enabled = true;
	while (monitor_enabled) {
		monitor_print_control_system_variables(sh);
		shell_process(sh);
		k_usleep(interval_us);
	}
	shell_set_bypass(sh, NULL);

	return 0;
}

#define LIST_HELP SHELL_HELP("List control systems", "")

#define MONITOR_HELP SHELL_HELP("Enable monitor", "[interval_us]")

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_control_system,
	SHELL_CMD(list, NULL, LIST_HELP, cmd_list),
	SHELL_CMD_ARG(monitor, NULL, MONITOR_HELP, cmd_monitor, 2, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(control_system, &sub_control_system, "Control system commands", NULL);
