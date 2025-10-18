/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zvb/control_system/signal/insignal.h>
#include <zephyr/dsp/dsp.h>

void control_system_signal_insignal_configure(struct control_system_signal_insignal *sig,
					      int64_t min_insig,
					      int64_t max_insig)
{
	int64_t insig_range;
	int64_t insig_center;
	int64_t scale_fract;

	__ASSERT_NO_MSG(max_insig > min_insig);

	insig_range = max_insig / 2 - min_insig / 2;
	insig_center = min_insig + 1 + insig_range;
	sig->insig_shift = 0;
	while(insig_range > INT32_MAX) {
		insig_range >>= 1;
		insig_center >>= 1;
		sig->insig_shift += 1;
	}

	sig->insig_center = (q31_t)insig_center;

	scale_fract = INT32_MAX;
	scale_fract <<= 32;
	scale_fract /= insig_range;
	sig->insig_scale_shift = -1;
	while(scale_fract > INT32_MAX) {
		scale_fract >>= 1;
		sig->insig_scale_shift += 1;
	}

	sig->insig_scale_fract = (q31_t)scale_fract;
}

void control_system_signal_insignal_sample(struct control_system_signal_insignal *sig,
					   int64_t insig,
					   q31_t *osig)
{
	insig >>= sig->insig_shift;
	insig -= sig->insig_center;

	if (insig > INT32_MAX) {
		*osig = INT32_MAX;
		return;
	}

	if (insig < INT32_MIN) {
		*osig = INT32_MIN;
		return;
	}

	*osig = (q31_t)insig;
	zdsp_scale_q31(osig, sig->insig_scale_fract, sig->insig_scale_shift, osig, 1);
}
