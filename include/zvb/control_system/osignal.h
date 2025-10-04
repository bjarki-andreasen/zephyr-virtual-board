/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_CONTROL_SYSTEM_OSIGNAL_H_
#define ZEPHYR_INCLUDE_CONTROL_SYSTEM_OSIGNAL_H_

#include <zephyr/kernel.h>
#include <zephyr/dsp/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @cond INTERNAL_HIDDEN
 */

struct control_system_osignal;

typedef int (*control_system_osignal_api_set_signal)(const struct control_system_osignal *cso,
						     q31_t sig);

struct control_system_osignal_api {
	control_system_osignal_api_set_signal set_signal;
};

struct control_system_osignal {
	void *data;
	const void *config;
	const struct control_system_osignal_api *api;
};

/**
 * @endcond
 */

/**
 * @brief Define a control system output signal instance.
 *
 * @details Statically define and initialize control system output signal instance.
 *
 * @param _name Name and symbol of the instance.
 * @param _data Pointer to implementation specific data.
 * @param _config Pointer to implementation specific configuration.
 * @param _api Pointer to control system output signal API implementation.
 */
#define CONTROL_SYSTEM_OSIGNAL_DEFINE(_name, _data, _config, _api)				\
	static const struct control_system_osignal _name = {					\
		.data = _data,									\
		.config = _config,								\
		.api = _api,									\
	};

int control_system_osignal_set_signal(const struct control_system_osignal *cso, q31_t sig);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_CONTROL_SYSTEM_OSIGNAL_H_ */
