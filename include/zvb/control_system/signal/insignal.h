/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_CONTROL_SYSTEM_SIGNAL_INSIGNAL_H_
#define ZEPHYR_INCLUDE_CONTROL_SYSTEM_SIGNAL_INSIGNAL_H_

#include <zephyr/kernel.h>
#include <zephyr/dsp/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @cond INTERNAL_HIDDEN
 */

struct control_system_signal_insignal {
	q31_t insig_center;
	q31_t insig_scale_fract;
	uint8_t insig_shift;
	int8_t insig_scale_shift;
};

/**
 * @endcond
 */

void control_system_signal_insignal_configure(struct control_system_signal_insignal *sig,
					      int64_t min_insig,
					      int64_t max_insig);

void control_system_signal_insignal_sample(struct control_system_signal_insignal *sig,
					   int64_t insig,
					   q31_t *osig);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_CONTROL_SYSTEM_SIGNAL_INSIGNAL_H_ */
