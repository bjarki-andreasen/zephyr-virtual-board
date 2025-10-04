/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zvb/control_system/transfer.h>

int control_system_transfer_transfer(const struct control_system_transfer *cst,
				     q31_t insig, q31_t *osig)
{
	return cst->api->transfer(cst, insig, osig);
}
