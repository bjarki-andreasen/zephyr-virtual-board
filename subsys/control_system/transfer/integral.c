/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zvb/control_system/transfer/integral.h>
#include <zephyr/dsp/dsp.h>

static int transfer_integral_api_transfer(const struct control_system_transfer *cst,
					  q31_t insig,
					  q31_t *osig)
{
	struct control_system_transfer_integral_data *data = cst->data;

	zdsp_scale_q31(
		&insig,
		data->sample_interval_fract,
		data->sample_interval_shift,
		osig,
		1
	);

	zdsp_add_q31(osig, &data->insig_accumulated, &data->insig_accumulated, 1);
	*osig = data->insig_accumulated;
	return 0;
}

const struct control_system_transfer_api __control_system_transfer_integral_api = {
	.transfer = transfer_integral_api_transfer,
};

int control_system_transfer_integral_configure(const struct control_system_transfer *cst,
					       q31_t sample_interval_fract,
					       uint8_t sample_interval_shift)
{
	struct control_system_transfer_integral_data *data = cst->data;

	if (sample_interval_fract == 0 || sample_interval_shift > 30) {
		return -EINVAL;
	}

	data->sample_interval_fract = sample_interval_fract;
	data->insig_accumulated = 0;
	data->sample_interval_shift = sample_interval_shift;
	return 0;
}
