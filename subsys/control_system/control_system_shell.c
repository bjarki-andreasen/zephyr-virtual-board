/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zvb/control_system/control_system.h>
#include <zephyr/dsp/dsp.h>
#include <zephyr/shell/shell.h>
#include <zephyr/kernel.h>

static bool monitor_enabled;

static void monitor_print_control_systems(const struct shell *sh)
{
	int64_t uptime_us;
	q31_t setpoint;
	q31_t process_var;
	q31_t sample;
	const char *name;
	const char *const fmt = "{\"nm\":\"%s\",\"ts\":%" PRIu64
				".%06" PRIu64 ",\"sp\":%" PRIq(6)
				",\"pv\":%" PRIq(6) ",\"sa\":%"
				PRIq(6) "}\n";

	STRUCT_SECTION_FOREACH(control_system, cs) {
		uptime_us = k_ticks_to_us_floor64(k_uptime_ticks());
		control_system_get_setpoint(cs, &setpoint);
		control_system_get_process_var(cs, &process_var);
		control_system_get_sample(cs, &sample);
		name = control_system_get_name(cs);
		shell_fprintf_normal(sh,
				     fmt,
				     name,
				     uptime_us / USEC_PER_SEC,
				     uptime_us % USEC_PER_SEC,
				     PRIq_arg(setpoint, 6, 0),
				     PRIq_arg(process_var, 6, 0),
				     PRIq_arg(sample, 6, 0));
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

	STRUCT_SECTION_FOREACH(control_system, cs) {
		shell_print(sh, "%s", control_system_get_name(cs));
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
		monitor_print_control_systems(sh);
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
