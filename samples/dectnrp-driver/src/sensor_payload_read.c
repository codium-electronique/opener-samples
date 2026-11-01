/*
 * Copyright (c) 2026 Codium Electronique
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Read the sensors found in the devicetree into a struct sensor_payload_values.
 *
 * Each group is compiled only when its devicetree compatible is enabled, so the code is not
 * tied to a board.
 */

#include "sensor_payload.h"

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/npm13xx_charger.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(dectnrp_driver_sample, CONFIG_SAMPLE_DRIVER_DECTNRP_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(sensirion_sht4x)
/* SHT4x: temperature in degC and humidity in %RH, both sent in 0.01 units. */
static int read_climate(struct sensor_payload_values *values)
{
	const struct device *dev = DEVICE_DT_GET_ONE(sensirion_sht4x);
	struct sensor_value temperature;
	struct sensor_value humidity;
	int ret;

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}
	ret = sensor_sample_fetch(dev);
	if (ret != 0) {
		return ret;
	}
	ret = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temperature);
	if (ret != 0) {
		return ret;
	}
	ret = sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &humidity);
	if (ret != 0) {
		return ret;
	}

	int64_t temperature_milli = sensor_value_to_milli(&temperature);
	int64_t humidity_milli = sensor_value_to_milli(&humidity);

	values->temperature_centi_c = sensor_payload_clamp_i16((int32_t)(temperature_milli / 10));
	values->humidity_centi_pct = sensor_payload_clamp_u16((int32_t)(humidity_milli / 10));
	values->flags |= SENSOR_PAYLOAD_CLIMATE;
	return 0;
}
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(st_lsm6dsl)
/* LSM6DSL accelerometer: m/s^2 from the driver, sent in mg. */
static int read_accel(struct sensor_payload_values *values)
{
	const struct device *dev = DEVICE_DT_GET_ONE(st_lsm6dsl);
	struct sensor_value accel[3];
	int ret;

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}
	ret = sensor_sample_fetch_chan(dev, SENSOR_CHAN_ACCEL_XYZ);
	if (ret != 0) {
		return ret;
	}
	ret = sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, accel);
	if (ret != 0) {
		return ret;
	}

	for (int axis = 0; axis < 3; axis++) {
		int32_t accel_mg = sensor_ms2_to_mg(&accel[axis]);

		values->accel_mg[axis] = sensor_payload_clamp_i16(accel_mg);
	}
	values->flags |= SENSOR_PAYLOAD_ACCEL;
	return 0;
}

#ifdef CONFIG_LSM6DSL_SENSORHUB
/* LIS2MDL on the LSM6DSL sensor hub: gauss from the driver, sent in mgauss. */
static int read_magn(struct sensor_payload_values *values)
{
	const struct device *dev = DEVICE_DT_GET_ONE(st_lsm6dsl);
	struct sensor_value magn[3];
	int ret;

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}
	ret = sensor_sample_fetch_chan(dev, SENSOR_CHAN_MAGN_XYZ);
	if (ret != 0) {
		return ret;
	}
	ret = sensor_channel_get(dev, SENSOR_CHAN_MAGN_XYZ, magn);
	if (ret != 0) {
		return ret;
	}

	for (int axis = 0; axis < 3; axis++) {
		int64_t magn_mgauss = sensor_value_to_milli(&magn[axis]);

		values->magn_mgauss[axis] = sensor_payload_clamp_i16((int32_t)magn_mgauss);
	}
	values->flags |= SENSOR_PAYLOAD_MAGN;
	return 0;
}
#endif /* CONFIG_LSM6DSL_SENSORHUB */
#endif /* st_lsm6dsl */

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_npm1300_charger)
/* nPM13xx charger: cell voltage (V) and current (A) sent in mV and mA, plus the raw status bytes.
 */
static int read_battery(struct sensor_payload_values *values)
{
	const struct device *dev = DEVICE_DT_GET_ONE(nordic_npm1300_charger);
	struct sensor_value voltage;
	struct sensor_value current;
	struct sensor_value status;
	struct sensor_value error;
	struct sensor_value vbus;
	int ret;

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}
	ret = sensor_sample_fetch(dev);
	if (ret != 0) {
		return ret;
	}
	ret = sensor_channel_get(dev, SENSOR_CHAN_GAUGE_VOLTAGE, &voltage);
	if (ret != 0) {
		return ret;
	}
	ret = sensor_channel_get(dev, SENSOR_CHAN_GAUGE_AVG_CURRENT, &current);
	if (ret != 0) {
		return ret;
	}
	ret = sensor_channel_get(dev, SENSOR_CHAN_NPM13XX_CHARGER_STATUS, &status);
	if (ret != 0) {
		return ret;
	}
	ret = sensor_channel_get(dev, SENSOR_CHAN_NPM13XX_CHARGER_ERROR, &error);
	if (ret != 0) {
		return ret;
	}
	ret = sensor_channel_get(dev, SENSOR_CHAN_NPM13XX_CHARGER_VBUS_STATUS, &vbus);
	if (ret != 0) {
		return ret;
	}

	int64_t voltage_mv = sensor_value_to_milli(&voltage);
	int64_t current_ma = sensor_value_to_milli(&current);

	values->battery_mv = sensor_payload_clamp_u16((int32_t)voltage_mv);
	values->battery_pct = sensor_payload_battery_pct(values->battery_mv);
	values->battery_ma = sensor_payload_clamp_i16((int32_t)current_ma);
	values->charger_status = (uint8_t)status.val1;
	values->charger_error = (uint8_t)error.val1;
	values->vbus_status = (uint8_t)vbus.val1;
	values->flags |= SENSOR_PAYLOAD_BATTERY;
	return 0;
}
#endif

int sensor_payload_read(struct sensor_payload_values *values)
{
	int ret;

	*values = (struct sensor_payload_values){
		.battery_pct = SENSOR_PAYLOAD_LEVEL_UNKNOWN,
	};

#if DT_HAS_COMPAT_STATUS_OKAY(sensirion_sht4x)
	ret = read_climate(values);
	if (ret != 0) {
		LOG_WRN("climate (SHT4x) not read, %d", ret);
	}
#endif
#if DT_HAS_COMPAT_STATUS_OKAY(st_lsm6dsl)
	ret = read_accel(values);
	if (ret != 0) {
		LOG_WRN("accel (LSM6DSL) not read, %d", ret);
	}
#ifdef CONFIG_LSM6DSL_SENSORHUB
	ret = read_magn(values);
	if (ret != 0) {
		LOG_WRN("magn (LIS2MDL) not read, %d", ret);
	}
#endif
#endif
#if DT_HAS_COMPAT_STATUS_OKAY(nordic_npm1300_charger)
	ret = read_battery(values);
	if (ret != 0) {
		LOG_WRN("battery (nPM13xx) not read, %d", ret);
	}
#endif

	if (values->flags == 0) {
		return -ENODEV;
	}
	return 0;
}
