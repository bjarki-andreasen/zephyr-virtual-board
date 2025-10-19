/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_CONTROL_SYSTEM_TYPES_H_
#define ZEPHYR_INCLUDE_CONTROL_SYSTEM_TYPES_H_

#include <zephyr/kernel.h>
#include <zephyr/dsp/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Convert nano value between 1000000000 and -1000000000 to Q31 */
#define CONTROL_SYSTEM_NANO(_nano) \
	(((_nano) * 0x7FFFFFFFLL) / 1000000000LL)

/** Convert micro value between 1000000 and -1000000 to Q31 */
#define CONTROL_SYSTEM_MICRO(_micro) \
	CONTROL_SYSTEM_NANO(_micro * 1000LL)

/** Convert milli value between 1000 and -1000 to Q31 */
#define CONTROL_SYSTEM_MILLI(_milli) \
	CONTROL_SYSTEM_MICRO(_milli * 1000LL)

/** Convert shifted nano value to Q31 */
#define CONTROL_SYSTEM_NANO_SHIFTED(_nano, _shift) \
	CONTROL_SYSTEM_NANO(_nano >> _shift)

/** Convert shifted micro value to Q31 */
#define CONTROL_SYSTEM_MICRO_SHIFTED(_micro, _shift) \
	CONTROL_SYSTEM_NANO_SHIFTED(_micro * 1000LL, _shift)

/** Convert shifted milli value to Q31 */
#define CONTROL_SYSTEM_MILLI_SHIFTED(_milli, _shift) \
	CONTROL_SYSTEM_MICRO_SHIFTED(_milli * 1000LL, _shift)

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_CONTROL_SYSTEM_TYPES_H_ */
