/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_POSITION_POSITION_H_
#define ZEPHYR_INCLUDE_POSITION_POSITION_H_

#include <zephyr/dsp/types.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

struct transform_vector {
	q31_t x;
	q31_t y;
	q31_t z;
};

struct transform_basis {
	struct transform_vector x;
	struct transform_vector y;
	struct transform_vector z;
};

struct transform {
	struct transform_basis basis;
	struct transform_vector origin;
	struct transform_vector rotation;
};

void position_set_position(const struct transform_vector *vector);
void position_set_rotation(const struct transform_vector *vector);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSITION_POSITION_H_ */
