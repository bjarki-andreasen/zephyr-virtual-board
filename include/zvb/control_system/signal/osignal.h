/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_CONTROL_SYSTEM_SIGNAL_OSIGNAL_H_
#define ZEPHYR_INCLUDE_CONTROL_SYSTEM_SIGNAL_OSIGNAL_H_

#include <zephyr/kernel.h>
#include <zephyr/dsp/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @cond INTERNAL_HIDDEN
 */

struct control_system_signal_osignal {
	q31_t osig_fract;
	int64_t osig_center;
	uint8_t osig_shift;
};

/**
 * @endcond
 */

void control_system_signal_osignal_configure(struct control_system_signal_osignal *sig,
					     int64_t min_osig,
					     int64_t max_osig);

void control_system_signal_osignal_sample(struct control_system_signal_osignal *sig,
					  q31_t insig,
					  int64_t *osig);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_CONTROL_SYSTEM_SIGNAL_OSIGNAL_H_ */
