/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_CONTROL_SYSTEM_TRANSFER_DIFFERENTIAL_H_
#define ZEPHYR_INCLUDE_CONTROL_SYSTEM_TRANSFER_DIFFERENTIAL_H_

#include <zvb/control_system/transfer.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @cond INTERNAL_HIDDEN
 */

struct control_system_transfer_differential_data {
	q31_t last_insig;
};

extern const struct control_system_transfer_api __control_system_transfer_differential_api;

/**
 * @endcond
 */

#define CONTROL_SYSTEM_TRANSFER_DIFFERENTIAL_DEFINE(_name)					\
	static struct control_system_transfer_differential_data _CONCAT(_name, _data);		\
												\
	CONTROL_SYSTEM_TRANSFER_DEFINE(								\
		_name,										\
		&_CONCAT(_name, _data),								\
		NULL,										\
		&__control_system_transfer_differential_api					\
	)

int control_system_transfer_differential_configure(const struct control_system_transfer *cst);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_CONTROL_SYSTEM_TRASNFER_DIFFERENTIAL_H_ */
