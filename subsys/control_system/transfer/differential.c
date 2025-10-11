/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zvb/control_system/transfer/differential.h>
#include <zephyr/dsp/dsp.h>

static int transfer_differential_api_transfer(const struct control_system_transfer *cst,
					      q31_t insig,
					      q31_t *osig)
{
	struct control_system_transfer_differential_data *data = cst->data;

	zdsp_sub_q31(&insig, &data->last_insig, osig, 1);
	data->last_insig = insig;
	return 0;
}

const struct control_system_transfer_api __control_system_transfer_differential_api = {
	.transfer = transfer_differential_api_transfer,
};

int control_system_transfer_differential_configure(const struct control_system_transfer *cst)
{
	struct control_system_transfer_differential_data *data = cst->data;

	data->last_insig = 0;
	return 0;
}
