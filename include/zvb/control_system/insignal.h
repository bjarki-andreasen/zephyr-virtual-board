/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_CONTROL_SYSTEM_INSIGNAL_H_
#define ZEPHYR_INCLUDE_CONTROL_SYSTEM_INSIGNAL_H_

#include <zephyr/kernel.h>
#include <zephyr/dsp/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @cond INTERNAL_HIDDEN
 */

struct control_system_insignal;

typedef int (*control_system_insignal_api_get_signal)(const struct control_system_insignal *csi,
						      q31_t *sig);

typedef int (*control_system_insignal_api_read)(const struct control_system_insignal *csi);

struct control_system_insignal_api {
	control_system_insignal_api_get_signal get_signal;
	control_system_insignal_api_read read;
};

struct control_system_insignal {
	void *data;
	const void *config;
	const struct control_system_insignal_api *api;
};

/**
 * @endcond
 */

/**
 * @brief Define a control system input signal instance.
 *
 * @details Statically define and initialize control system input signal instance.
 *
 * @param _name Name and symbol of the instance.
 * @param _data Pointer to implementation specific data.
 * @param _config Pointer to implementation specific configuration.
 * @param _api Pointer to control system input signal API implementation.
 */
#define CONTROL_SYSTEM_INSIGNAL_DEFINE(_name, _data, _config, _api)				\
	static const struct control_system_insignal _name = {					\
		.data = _data,									\
		.config = _config,								\
		.api = _api,									\
	};

int control_system_insignal_get_signal(const struct control_system_insignal *csi, q31_t *sig);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_CONTROL_SYSTEM_INSIGNAL_H_ */
