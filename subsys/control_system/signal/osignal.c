/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zvb/control_system/signal/osignal.h>
#include <zephyr/dsp/dsp.h>

void control_system_signal_osignal_configure(struct control_system_signal_osignal *sig,
					     int64_t min_osig,
					     int64_t max_osig)
{
	int64_t osig_range;

	__ASSERT_NO_MSG(max_osig > min_osig);

	osig_range = max_osig / 2 - min_osig / 2;
	sig->osig_center = min_osig + 1 + osig_range;
	sig->osig_shift = 0;
	while(osig_range > INT32_MAX) {
		osig_range >>= 1;
		sig->osig_shift += 1;
	}
	sig->osig_fract = (q31_t)osig_range;
}

void control_system_signal_osignal_sample(struct control_system_signal_osignal *sig,
					  q31_t insig,
					  int64_t *osig)
{
	zdsp_mult_q31(&insig, &sig->osig_fract, &insig, 1);
	*osig = insig;
	*osig <<= sig->osig_shift;
	*osig += sig->osig_center;
}
