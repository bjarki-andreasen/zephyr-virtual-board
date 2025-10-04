/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zvb/control_system/osignal.h>

int control_system_osignal_set_signal(const struct control_system_osignal *cso, q31_t sig)
{
	return cso->api->set_signal(cso, sig);
}
