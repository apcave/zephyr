/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Alex Cave <alex.cave@outlook.com.au>
 */
#pragma once

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>

typedef struct {
	const struct i2c_dt_spec *i2c;
#ifdef CONFIG_VL53L4CD_INTERRUPT_MODE
	struct gpio_callback gpio_cb;
	struct k_work work;
	const struct device *dev;
#endif
} vl53l4cd_dev_t;


