/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Alex Cave <alex.cave@outlook.com.au>
 */
#define DT_DRV_COMPAT st_vl53l4cd

#include "vl53l4cd.h"
#include "vl53l4cd_api.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>

LOG_MODULE_REGISTER(VL53L4CD, CONFIG_SENSOR_LOG_LEVEL);

struct vl53l4cd_config {
	struct i2c_dt_spec i2c;
#ifdef CONFIG_VL53L4CD_XSHUT
	struct gpio_dt_spec xshut;
#endif
#ifdef CONFIG_VL53L4CD_INTERRUPT_MODE
	struct gpio_dt_spec gpio1;
#endif
};



static int vl53l4cd_sample_fetch(const struct device *dev,
		enum sensor_channel chan)
{
	vl53l4cd_dev_t *drv_data = dev->data;

	LOG_DBG("VL53L4CD fetch, starts the ranging.");
	int status = VL53L4CD_StartRanging(drv_data);
	return -status;
}

static int vl53l4cd_channel_get(const struct device *dev,
		enum sensor_channel chan,
		struct sensor_value *val)
{
	vl53l4cd_dev_t *drv_data = dev->data;
	VL53L4CD_ResultsData_t results;
	int status = VL53L4CD_ERROR_NONE;

	LOG_DBG("VL53L4CD channel used to get distance measurement");


	if(! VL53L4CD_IsDataReady(drv_data)) {
		LOG_DBG("VL53L4CD data not ready");
		return EAGAIN;
	}

	/* (Mandatory) Clear HW interrupt to restart measurements */
	if (VL53L4CD_ClearInterrupt(drv_data)) {
		LOG_ERR("Failed to clear interrupt");
		return -status;
	}

	/* Read measured distance. RangeStatus = 0 means valid data */
	if (VL53L4CD_GetResult(drv_data, &results)) {
		LOG_ERR("Failed to get result (range)");
		return -status;
	}

	LOG_DBG("Status = %6u, Distance = %6u, Signal = %6u",
		results.range_status,
		results.distance_mm,
		results.signal_per_spad_kcps);

	val->val1 = results.distance_mm;
	val->val2 = results.sigma_mm;

	if( results.range_status != 0) {
		LOG_DBG("VL53L4CD measurement is not valid (status %u)", results.range_status);
		return EAGAIN;
	}

	return 0;
}

static int vl53l4cd_attr_get(const struct device *dev,
		enum sensor_channel chan,
		enum sensor_attribute attr,
		struct sensor_value *val)
{
	LOG_ERR("VL53L4CD attribute get not implemented");
	return -ENOTSUP;
}

static int vl53l4cd_attr_set(const struct device *dev,
		enum sensor_channel chan,
		enum sensor_attribute attr,
		const struct sensor_value *val)
{
	int status;
	vl53l4cd_dev_t *drv_data = dev->data;


	if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
		uint32_t timing_budget_ms = val->val1;
		uint32_t inter_measurement_ms = val->val2;

		LOG_DBG("Set timing budget (ms) %u and inter-measurement time (ms) %u", timing_budget_ms, inter_measurement_ms);

		/* Calculate timing budget and inter-measurement time based on desired sampling frequency */
		status = VL53L4CD_SetRangeTiming(drv_data, timing_budget_ms, inter_measurement_ms);
		if(status)
		{
			LOG_ERR("VL53L4CD_SetRangeTiming failed with status %d", status);
			return -status;
		}
		return 0;
	}

	if(attr == SENSOR_ATTR_UPPER_THRESH) {
		uint16_t distance_low_mm = val->val1;
		uint16_t distance_high_mm = 0;

		LOG_DBG("Set near threshold attribute with low distance %u mm and high distance %u mm", distance_low_mm, distance_high_mm);
		status = VL53L4CD_SetDetectionThresholds(drv_data, distance_low_mm, distance_high_mm, 0);
		if(status)
		{
			LOG_ERR("VL53L4CD_SetDetectionThresholds failed with status %u", status);
			return -status;
		}

		return 0;
	}


	if(attr == SENSOR_ATTR_UPPER_THRESH) {
		uint16_t distance_low_mm = 0;
		uint16_t distance_high_mm = val->val1;

		LOG_DBG("Set far threshold attribute with low distance %u mm and high distance %u mm", distance_low_mm, distance_high_mm);
		status = VL53L4CD_SetDetectionThresholds(drv_data, distance_low_mm, distance_high_mm, 1);
		if(status)
		{
			LOG_ERR("VL53L4CD_SetDetectionThresholds failed with status %u", status);
			return -status;
		}

		return 0;
	}


	return -ENOTSUP;
}

#ifdef CONFIG_VL53L4CD_INTERRUPT_MODE

static int vl53l4cd_read_sensor(vl53l4cd_dev_t  *drv_data)
{
	VL53L4CD_ResultsData_t results;
	int status = VL53L4CD_ERROR_NONE;

	if(! VL53L4CD_IsDataReady(drv_data)) {
		LOG_DBG("VL53L4CD data not ready");
		return EAGAIN;
	}

	/* (Mandatory) Clear HW interrupt to restart measurements */
	if (VL53L4CD_ClearInterrupt(drv_data)) {
		LOG_ERR("Failed to clear interrupt");
		return -status;
	}

	/* Read measured distance. RangeStatus = 0 means valid data */
	if (VL53L4CD_GetResult(drv_data, &results)) {
		LOG_ERR("Failed to get result (range)");
		return -status;
	}

	LOG_DBG("Status = %6u, Distance = %6u, Signal = %6u",
		results.range_status,
		results.distance_mm,
		results.signal_per_spad_kcps);


	if( results.range_status != 0) {
		LOG_DBG("VL53L4CD measurement is not valid (status %u)", results.range_status);
		return EAGAIN;
	}

	return 0;
}

static void vl53l4cd_worker(struct k_work *work)
{
	vl53l4cd_dev_t  *drv_data = CONTAINER_OF(work, vl53l4cd_dev_t, work);

	vl53l4cd_read_sensor(drv_data);
}

static void vl53l4cd_gpio_callback(const struct device *dev,
		struct gpio_callback *cb, uint32_t pins)
{
	vl53l4cd_dev_t *drv_data = CONTAINER_OF(cb, vl53l4cd_dev_t, gpio_cb);

	k_work_submit(&drv_data->work);
}

static int vl53l4cd_init_interrupt(const struct device *dev)
{
	vl53l4cd_dev_t *drv_data = dev->data;
	const struct vl53l4cd_config *config = dev->config;
	int ret;

	drv_data->dev = dev;

	if (!gpio_is_ready_dt(&config->gpio1)) {
		LOG_ERR("%s: device %s is not ready", dev->name, config->gpio1.port->name);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->gpio1, GPIO_INPUT | GPIO_PULL_UP);
	if (ret < 0) {
		LOG_ERR("[%s] Unable to configure GPIO interrupt", dev->name);
		return -EIO;
	}

	gpio_init_callback(&drv_data->gpio_cb,
					vl53l4cd_gpio_callback,
					BIT(config->gpio1.pin));

	ret = gpio_add_callback(config->gpio1.port, &drv_data->gpio_cb);
	if (ret < 0) {
		LOG_ERR("Failed to set gpio callback!");
		return -EIO;
	}

	drv_data->work.handler = vl53l4cd_worker;

	return 0;
}
#endif

static DEVICE_API(sensor, vl53l4cd_api_funcs) = {
	.sample_fetch = vl53l4cd_sample_fetch,
	.channel_get = vl53l4cd_channel_get,
	.attr_get = vl53l4cd_attr_get,
	.attr_set = vl53l4cd_attr_set,
};

static int vl53l4cd_init(const struct device *dev)
{
	LOG_WRN("--------------------------------------------------------");
    LOG_DBG("Initializing VL53L4CD sensor");

	int ret = 0;
	vl53l4cd_dev_t *drv_data = dev->data;
	const struct vl53l4cd_config *config = dev->config;

	LOG_DBG("Configuring I2C interface");
	drv_data->i2c = &config->i2c;
	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("I2C bus is not ready");
		return -EIO;
	}

#ifdef CONFIG_VL53L4CD_XSHUT
	LOG_DBG("Pulling XSHUT high to start the sensor");

	if (config->xshut.port) {
		int gpio_ret = gpio_pin_set_dt(&config->xshut, 1);

		if (gpio_ret < 0) {
			LOG_ERR("[%s] Unable to set XSHUT gpio (error %d)", dev->name, gpio_ret);
			return -EIO;
		}
		/* Boot duration is 1.2 ms max */
		k_sleep(K_MSEC(2));
	}
#endif

#ifdef CONFIG_VL53L4CD_INTERRUPT_MODE
	if (config->gpio1.port) {
		ret = vl53l4cd_init_interrupt(dev);
		if (ret < 0) {
			LOG_ERR("Failed to initialize interrupt!");
			return -EIO;
		}
	}
#endif

	LOG_DBG("Checking sensor presence");
	ret = VL53L4CD_detect(drv_data);
	if (ret) {
		LOG_ERR("VL53L4CD sensor detect failed: %d", ret);
		return -ret;
	}


	LOG_DBG("Initializing sensor");
	ret = VL53L4CD_SensorInit(drv_data);
	if (ret) {
		LOG_ERR("VL53L4CD sensor initialization failed: %d", ret);
		return -ret;
	}

	LOG_WRN("--------------------------------------------------------");
    return 0;
}

#define VL53L4CD_INIT(i) \
	static const struct vl53l4cd_config vl53l4cd_config_##i = { \
		.i2c = I2C_DT_SPEC_INST_GET(i), \
		IF_ENABLED(CONFIG_VL53L4CD_XSHUT, ( \
		.xshut = GPIO_DT_SPEC_INST_GET_OR(i, xshut_gpios, { 0 }),)) \
		IF_ENABLED(CONFIG_VL53L4CD_INTERRUPT_MODE, ( \
		.gpio1 = GPIO_DT_SPEC_INST_GET_OR(i, int_gpios, { 0 }),)) \
	}; \
	\
	static vl53l4cd_dev_t vl53l4cd_data_##i; \
	\
	SENSOR_DEVICE_DT_INST_DEFINE(i, \
				     vl53l4cd_init, \
				     NULL, \
				     &vl53l4cd_data_##i, \
				     &vl53l4cd_config_##i, \
				     POST_KERNEL, \
				     CONFIG_SENSOR_INIT_PRIORITY, \
				     &vl53l4cd_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(VL53L4CD_INIT)