/*
 * Copyright (c) 2026 Codium Electronique
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/sys/byteorder.h>

#include "sensor_payload.h"

struct battery_point {
	uint16_t mv;
	uint8_t pct;
};

/* Resting voltages of a common single-cell LiPo with 4.2 V termination. */
static const struct battery_point battery_table[] = {
	{3270, 0},  {3610, 5},  {3690, 10}, {3730, 20}, {3770, 30}, {3800, 40},
	{3840, 50}, {3870, 60}, {3950, 70}, {4020, 80}, {4110, 90}, {4200, 100},
};

int16_t sensor_payload_clamp_i16(int32_t value)
{
	if (value > INT16_MAX) {
		return INT16_MAX;
	}
	if (value < INT16_MIN) {
		return INT16_MIN;
	}
	return (int16_t)value;
}

uint16_t sensor_payload_clamp_u16(int32_t value)
{
	if (value > UINT16_MAX) {
		return UINT16_MAX;
	}
	if (value < 0) {
		return 0;
	}
	return (uint16_t)value;
}

uint8_t sensor_payload_battery_pct(uint16_t battery_mv)
{
	const size_t count = ARRAY_SIZE(battery_table);
	const struct battery_point *first = &battery_table[0];
	const struct battery_point *last = &battery_table[count - 1];

	if (battery_mv <= first->mv) {
		return first->pct;
	}
	if (battery_mv >= last->mv) {
		return last->pct;
	}

	for (size_t i = 1; i < count; i++) {
		const struct battery_point *low = &battery_table[i - 1];
		const struct battery_point *high = &battery_table[i];

		if (battery_mv <= high->mv) {
			uint32_t span_mv = high->mv - low->mv;
			uint32_t span_pct = high->pct - low->pct;
			uint32_t offset_mv = battery_mv - low->mv;
			uint32_t pct = low->pct + (offset_mv * span_pct) / span_mv;

			return (uint8_t)pct;
		}
	}

	return last->pct;
}

void sensor_payload_pack(const struct sensor_payload_values *values,
			 uint8_t payload[SENSOR_PAYLOAD_SIZE])
{
	memset(payload, 0, SENSOR_PAYLOAD_SIZE);
	payload[0] = SENSOR_PAYLOAD_MAGIC0;
	payload[1] = SENSOR_PAYLOAD_MAGIC1;
	payload[2] = SENSOR_PAYLOAD_VERSION;
	payload[3] = values->flags;

	if ((values->flags & SENSOR_PAYLOAD_CLIMATE) != 0) {
		sys_put_le16((uint16_t)values->temperature_centi_c, &payload[4]);
		sys_put_le16(values->humidity_centi_pct, &payload[6]);
	}
	if ((values->flags & SENSOR_PAYLOAD_ACCEL) != 0) {
		for (size_t i = 0; i < 3; i++) {
			sys_put_le16((uint16_t)values->accel_mg[i], &payload[8 + 2 * i]);
		}
	}
	if ((values->flags & SENSOR_PAYLOAD_MAGN) != 0) {
		for (size_t i = 0; i < 3; i++) {
			sys_put_le16((uint16_t)values->magn_mgauss[i], &payload[14 + 2 * i]);
		}
	}
	if ((values->flags & SENSOR_PAYLOAD_BATTERY) != 0) {
		sys_put_le16(values->battery_mv, &payload[20]);
		payload[22] = values->battery_pct;
		payload[23] = values->charger_status;
		payload[24] = values->charger_error;
		sys_put_le16((uint16_t)values->battery_ma, &payload[25]);
		payload[27] = values->vbus_status;
	}
}
