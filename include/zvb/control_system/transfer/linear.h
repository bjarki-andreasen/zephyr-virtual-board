/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_CONTROL_SYSTEM_TRANSFER_LINEAR_H_
#define ZEPHYR_INCLUDE_CONTROL_SYSTEM_TRANSFER_LINEAR_H_

#include <zvb/control_system/transfer.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @cond INTERNAL_HIDDEN
 */

struct control_system_transfer_linear_data {
	q31_t insig_scale_shift;
	q31_t insig_scale_fract;
	q31_t insig_center;
};

extern const struct control_system_transfer_api __control_system_transfer_linear_api;

/**
 * @endcond
 */

#define CONTROL_SYSTEM_TRANSFER_LINEAR_DEFINE(_name)						\
	static struct control_system_transfer_linear_data _CONCAT(_name, _data);		\
												\
	CONTROL_SYSTEM_TRANSFER_DEFINE(								\
		_name,										\
		&_CONCAT(_name, _data),								\
		NULL,										\
		&__control_system_transfer_linear_api						\
	)

int control_system_transfer_linear_configure(const struct control_system_transfer *cst,
					     q31_t min_insig,
					     q31_t max_insig);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_CONTROL_SYSTEM_TRASNFER_LINEAR_H_ */
