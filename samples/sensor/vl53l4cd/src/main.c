/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Alex Cave <alex.cave@outlook.com.au>
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>

LOG_MODULE_REGISTER(range, LOG_LEVEL_INF);

enum APP_MODE {
	APP_MODE_DEFAULT,
	APP_MODE_LOW_POWER,
	APP_MODE_ACCURACY,
	APP_MODE_HIGH_SPEED,
} app_mode = APP_MODE_DEFAULT;

static const struct device *sensor = DEVICE_DT_GET(DT_ALIAS(distance0));

static void fetch_and_display(const struct device *sensor, uint32_t count, enum APP_MODE mode,
			      bool proximity);
{
	int rc;
	struct sensor_value timing;

	switch (mode) {
	case APP_MODE_DEFAULT:
		break;
	case APP_MODE_LOW_POWER:
		LOG_INF("Timing budget of 100ms, and ranging period 1000ms (10% active ranging and "
			"90% low power)");
		timing.val1 = 100;
		timing.val2 = 1000;
		break;
	case APP_MODE_ACCURACY:
		LOG_INF("Timing budget of 200ms, and ranging period 200ms (100% active ranging)");
		timing.val1 = 200;
		timing.val2 = 0;
		break;
	case APP_MODE_HIGH_SPEED:
		LOG_INF("Timing budget of 10ms, and ranging period 10ms (100% active ranging)");
		LOG_INF("highest ranging frequency (100Hz)");
		timing.val1 = 10;
		timing.val2 = 0;
		break;
	default:
		LOG_ERR("Invalid application mode");
		return;
	}

	rc = sensor_attr_set(sensor, SENSOR_CHAN_DISTANCE, SENSOR_ATTR_SAMPLING_FREQUENCY, &timing);
	if (rc < 0) {
		LOG_ERR("ERROR: Set low power attribute failed: %d", rc);
		return;
	}

	if (proximity) {
		LOG_INF("Proximity mode enabled");
		struct sensor_value threshold;

		threshold.val1 = 100;
		rc = sensor_attr_set(sensor, SENSOR_CHAN_DISTANCE, SENSOR_ATTR_UPPER_THRESH,
				     &threshold);
		if (rc < 0) {
			LOG_ERR("ERROR: Set proximity attribute failed: %d", rc);
			return;
		}
	}

	LOG_INF("Start the sensor ranging...");
	rc = sensor_sample_fetch(sensor);
	if (rc < 0) {
		LOG_ERR("ERROR: Fetch failed: %d", rc);
		return;
	}

	LOG_INF("Place object within 100mm of the sensor to see proximity mode in action");
	while (true) {
		struct sensor_value distance;
		rc = sensor_channel_get(sensor, SENSOR_CHAN_DISTANCE, &distance);
		if (rc == EAGAIN) {
			LOG_DBG("VL53L4CD data not ready");
			continue;
		} else if (rc < 0) {
			LOG_ERR("ERROR: get failed: %d", rc);
			return;
		}
		LOG_INF("%s: Distance (mm) %d, Standard Deviation (mm) %d", sensor->name,
			distance.val1, distance.val2);
		k_sleep(K_MSEC(500));
	}
}

int main(void)
{
	LOG_INF("Initializing range sensor...");
	if (!device_is_ready(sensor)) {
		LOG_ERR("Error: Device \"%s\" is not ready", sensor->name);
		return -1;
	}

	fetch_and_display(sensor, 1000, app_mode, true);

	return 0;
}
