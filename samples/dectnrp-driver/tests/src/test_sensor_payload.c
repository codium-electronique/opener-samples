/*
 * Copyright (c) 2026 Codium Electronique
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Tests of the dectnrp-driver sample sensor payload record.
 */

#include <string.h>
#include <zephyr/ztest.h>

#include "sensor_payload.h"

/* Shared test vector, the Rust decoder (nrp-ping sensors.rs) checks the same bytes. */
static const uint8_t vector[SENSOR_PAYLOAD_SIZE] = {
	0xc0, 0xde, 0x01, 0x0f, 0x29, 0x09, 0x18, 0x10, 0x0c, 0x00, 0xde, 0xff, 0xeb, 0x03,
	0xd2, 0x00, 0x6a, 0xff, 0x5c, 0xfe, 0x48, 0x0f, 0x41, 0x09, 0x00, 0xf5, 0x00, 0x01};

static struct sensor_payload_values values;
static uint8_t payload[SENSOR_PAYLOAD_SIZE];

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	memset(&values, 0, sizeof(values));
	values.flags = SENSOR_PAYLOAD_CLIMATE | SENSOR_PAYLOAD_ACCEL | SENSOR_PAYLOAD_MAGN |
		       SENSOR_PAYLOAD_BATTERY;
	values.temperature_centi_c = 2345;
	values.humidity_centi_pct = 4120;
	values.accel_mg[0] = 12;
	values.accel_mg[1] = -34;
	values.accel_mg[2] = 1003;
	values.magn_mgauss[0] = 210;
	values.magn_mgauss[1] = -150;
	values.magn_mgauss[2] = -420;
	values.battery_mv = 3912;
	values.battery_pct = 65;
	values.charger_status = 0x09;
	values.charger_error = 0x00;
	values.battery_ma = 245;
	values.vbus_status = 0x01;
	memset(payload, 0xee, sizeof(payload));
}

ZTEST_SUITE(sensor_payload, NULL, NULL, before, NULL, NULL);

ZTEST(sensor_payload, test_pack_vector)
{
	int diff;

	sensor_payload_pack(&values, payload);
	diff = memcmp(payload, vector, sizeof(vector));
	zassert_equal(diff, 0, "payload differs from the shared vector");
}

ZTEST(sensor_payload, test_pack_partial_flags)
{
	uint8_t zeros[SENSOR_PAYLOAD_SIZE] = {0};
	int diff;

	/* Only the battery group valid: bytes 4-19 are zero, 20-27 are the vector's. */
	values.flags = SENSOR_PAYLOAD_BATTERY;
	sensor_payload_pack(&values, payload);
	zassert_equal(payload[0], SENSOR_PAYLOAD_MAGIC0);
	zassert_equal(payload[1], SENSOR_PAYLOAD_MAGIC1);
	zassert_equal(payload[2], SENSOR_PAYLOAD_VERSION);
	zassert_equal(payload[3], SENSOR_PAYLOAD_BATTERY);
	diff = memcmp(&payload[4], zeros, 16);
	zassert_equal(diff, 0, "invalid groups are not zero");
	diff = memcmp(&payload[20], &vector[20], 8);
	zassert_equal(diff, 0, "battery group differs");

	/* No group valid: everything after the flags is zero. */
	values.flags = 0;
	memset(payload, 0xee, sizeof(payload));
	sensor_payload_pack(&values, payload);
	zassert_equal(payload[0], SENSOR_PAYLOAD_MAGIC0);
	zassert_equal(payload[1], SENSOR_PAYLOAD_MAGIC1);
	zassert_equal(payload[2], SENSOR_PAYLOAD_VERSION);
	zassert_equal(payload[3], 0);
	diff = memcmp(&payload[4], zeros, 24);
	zassert_equal(diff, 0, "payload is not zero");
}

ZTEST(sensor_payload, test_battery_pct)
{
	uint8_t pct;

	pct = sensor_payload_battery_pct(3200);
	zassert_equal(pct, 0);
	pct = sensor_payload_battery_pct(3270);
	zassert_equal(pct, 0);
	pct = sensor_payload_battery_pct(4020);
	zassert_equal(pct, 80);
	pct = sensor_payload_battery_pct(4200);
	zassert_equal(pct, 100);
	pct = sensor_payload_battery_pct(4300);
	zassert_equal(pct, 100);
	/* Halfway between 3840 (50) and 3870 (60). */
	pct = sensor_payload_battery_pct(3855);
	zassert_equal(pct, 55);
	/* Halfway between 3690 (10) and 3730 (20). */
	pct = sensor_payload_battery_pct(3710);
	zassert_equal(pct, 15);
	/* 65.25 truncated, the level of the shared vector. */
	pct = sensor_payload_battery_pct(3912);
	zassert_equal(pct, 65);
}

ZTEST(sensor_payload, test_clamp_i16)
{
	int16_t value;

	value = sensor_payload_clamp_i16(40000);
	zassert_equal(value, 32767);
	value = sensor_payload_clamp_i16(-40000);
	zassert_equal(value, -32768);
	value = sensor_payload_clamp_i16(-5);
	zassert_equal(value, -5);
	value = sensor_payload_clamp_i16(INT64_MAX);
	zassert_equal(value, 32767);
	value = sensor_payload_clamp_i16(INT64_MIN);
	zassert_equal(value, -32768);
	value = sensor_payload_clamp_i16((int64_t)1 << 32);
	zassert_equal(value, 32767);
	value = sensor_payload_clamp_i16(-((int64_t)1 << 32));
	zassert_equal(value, -32768);
}

ZTEST(sensor_payload, test_clamp_u16)
{
	uint16_t value;

	value = sensor_payload_clamp_u16(-1);
	zassert_equal(value, 0);
	value = sensor_payload_clamp_u16(70000);
	zassert_equal(value, 65535);
	value = sensor_payload_clamp_u16(INT64_MAX);
	zassert_equal(value, 65535);
	value = sensor_payload_clamp_u16(INT64_MIN);
	zassert_equal(value, 0);
	value = sensor_payload_clamp_u16((int64_t)1 << 32);
	zassert_equal(value, 65535);
}
