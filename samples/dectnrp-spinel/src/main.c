/*
 * Copyright (c) 2026 Codium Electronique
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(dectnrp_spinel_sample, LOG_LEVEL_INF);

int main(void)
{
	/* The Spinel interface starts on its own; the host drives the radio. */
	LOG_INF("DECT NR+ Spinel radio co-processor ready");

	return 0;
}
