/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zvb/control_system/transfer/normal.h>
#include <zephyr/dsp/dsp.h>

static int transfer_normal_api_transfer(const struct control_system_transfer *cst,
					q31_t insig,
					q31_t *osig)
{
	struct control_system_transfer_normal_data *data = cst->data;

	zdsp_sub_q31(&insig, &data->insig_center, &insig, 1);
	zdsp_scale_q31(&insig, data->insig_scale_fract, data->insig_scale_shift, osig, 1);
	return 0;
}

const struct control_system_transfer_api __control_system_transfer_normal_api = {
	.transfer = transfer_normal_api_transfer,
};

int control_system_transfer_normal_configure(const struct control_system_transfer *cst,
					     q31_t min_insig,
					     q31_t max_insig)
{
	struct control_system_transfer_normal_data *data = cst->data;
	int64_t insig_range;
	int64_t scale_fract;

	if (max_insig == 0 || max_insig <= min_insig) {
		return -EINVAL;
	}

	insig_range = max_insig - min_insig + 1;
	insig_range >>= 1;
	scale_fract = INT32_MAX;
	scale_fract <<= 32;
	scale_fract /= insig_range;
	data->insig_scale_shift = -1;
	while(scale_fract > INT32_MAX) {
		scale_fract >>= 1;
		data->insig_scale_shift += 1;
	}

	data->insig_scale_fract = (q31_t)scale_fract;
	data->insig_center = max_insig - insig_range;
	return 0;
}
