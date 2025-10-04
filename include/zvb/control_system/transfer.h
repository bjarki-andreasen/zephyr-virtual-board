/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_CONTROL_SYSTEM_TRANSFER_H_
#define ZEPHYR_INCLUDE_CONTROL_SYSTEM_TRANSFER_H_

#include <zephyr/kernel.h>
#include <zephyr/dsp/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @cond INTERNAL_HIDDEN
 */

struct control_system_transfer;

typedef int (*control_system_transfer_api_transfer)(const struct control_system_transfer *cst,
						    q31_t insig,
						    q31_t *osig);

struct control_system_transfer_api {
	control_system_transfer_api_transfer transfer;
};

struct control_system_transfer {
	void *data;
	const void *config;
	const struct control_system_transfer_api *api;
};

/**
 * @endcond
 */

/**
 * @brief Define a control system transfer instance.
 *
 * @details Statically define and initialize control system transfer instance.
 *
 * @param _name Name and symbol of the instance.
 * @param _data Pointer to implementation specific data.
 * @param _config Pointer to implementation specific configuration.
 * @param _api Pointer to control system transfer API implementation.
 */
#define CONTROL_SYSTEM_TRANSFER_DEFINE(_name, _data, _config, _api)				\
	static const struct control_system_transfer _name = {					\
		.data = _data,									\
		.config = _config,								\
		.api = _api,									\
	};

int control_system_transfer_transfer(const struct control_system_transfer *cst,
				     q31_t insig,
				     q31_t *osig);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_CONTROL_SYSTEM_OSIGNAL_H_ */
