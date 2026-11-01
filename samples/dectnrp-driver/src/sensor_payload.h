/*
 * Copyright (c) 2026 Codium Electronique
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Sensor payload record (version 1) carried in the sample frame's user plane data.
 *
 * Fixed 28 byte record, little-endian:
 *
 *  0  magic (C0 DE), 2 version, 3 flags,
 *  4  temperature i16 0.01 degC, 6 humidity u16 0.01 %RH,
 *  8  accel x y z i16 mg, 14 magn x y z i16 mgauss,
 * 20  battery u16 mV, 22 battery level u8 % (FF unknown),
 * 23  charger status u8, 24 charger error u8, 25 battery current i16 mA, 27 VBUS status u8.
 *
 * A group whose flag bit is clear is sent as zeros.
 */

#ifndef SENSOR_PAYLOAD_H_
#define SENSOR_PAYLOAD_H_

#include <stdint.h>
#include <zephyr/sys/util.h>

#define SENSOR_PAYLOAD_SIZE          28
#define SENSOR_PAYLOAD_MAGIC0        0xC0
#define SENSOR_PAYLOAD_MAGIC1        0xDE
#define SENSOR_PAYLOAD_VERSION       1
#define SENSOR_PAYLOAD_CLIMATE       BIT(0)
#define SENSOR_PAYLOAD_ACCEL         BIT(1)
#define SENSOR_PAYLOAD_MAGN          BIT(2)
#define SENSOR_PAYLOAD_BATTERY       BIT(3)
#define SENSOR_PAYLOAD_LEVEL_UNKNOWN 0xFF

struct sensor_payload_values {
	uint8_t flags;
	int16_t temperature_centi_c;
	uint16_t humidity_centi_pct;
	int16_t accel_mg[3];
	int16_t magn_mgauss[3];
	uint16_t battery_mv;
	uint8_t battery_pct;
	uint8_t charger_status;
	uint8_t charger_error;
	int16_t battery_ma;
	uint8_t vbus_status;
};

/** Pack the values into the 28 byte record; groups without their flag bit are zeroed. */
void sensor_payload_pack(const struct sensor_payload_values *values,
			 uint8_t payload[SENSOR_PAYLOAD_SIZE]);

/** Estimate the battery level in % from the cell voltage (single-cell LiPo, 4.2 V). */
uint8_t sensor_payload_battery_pct(uint16_t battery_mv);

/** Clamp to the range of a 16-bit field, values never wrap. */
int16_t sensor_payload_clamp_i16(int64_t value);
uint16_t sensor_payload_clamp_u16(int64_t value);

/**
 * Read the sensors found in the devicetree. Fills every group it can read and sets its flag,
 * logs a warning naming each group that failed.
 *
 * @retval 0 at least one group was read.
 * @retval -ENODEV no group could be read.
 */
int sensor_payload_read(struct sensor_payload_values *values);

#endif /* SENSOR_PAYLOAD_H_ */
