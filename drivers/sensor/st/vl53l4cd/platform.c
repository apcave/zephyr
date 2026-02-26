/**
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Alex Cave <alex.cave@outlook.com.au>
 */

#include "platform.h"
#include "VL53L4CD_api.h"

#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(platform, CONFIG_VL53L4CD_LOG_LEVEL);

#define SUPPRESS_UNUSED_WARNING(x) \
	((void) (x))

uint8_t write_multi(
		Dev_t dev,
		uint16_t reg_address,
		uint8_t *pdata,
		uint32_t count)
{
	VL53L4CD_Error status = VL53L4CD_ERROR_NONE;
	int32_t status_int = 0;
	uint8_t buffer[count + 2];

	/* To be able to write to the 16-bit registers/addresses on the VL53L4CDx */
	buffer[1] = (uint8_t)(reg_address & 0x00ff);
	buffer[0] = (uint8_t)((reg_address & 0xff00) >> 8);


	memcpy(&buffer[2], pdata, count);

	status_int = i2c_write_dt(dev->i2c, buffer, count + 2);

	if (status_int < 0) {
		status = VL53L4CD_ERROR_CONTROL_INTERFACE;
		LOG_ERR("Failed to write");
	}

	return status;
}

uint8_t read_multi(
		Dev_t dev,
		uint16_t reg_address,
		uint8_t *pdata,
		uint32_t count)
{
	VL53L4CD_Error status = VL53L4CD_ERROR_NONE;
	int32_t status_int = 0;

	reg_address = sys_cpu_to_be16(reg_address);

	status_int = i2c_write_read_dt(dev->i2c, (uint8_t *)(&reg_address), 2, pdata, count);

	if (status_int < 0) {
		status = VL53L4CD_ERROR_CONTROL_INTERFACE;
		LOG_ERR("Failed to read");
	}

	return status;
}


uint8_t VL53L4CD_RdDWord(Dev_t dev, uint16_t RegisterAdress, uint32_t *value)
{
	uint8_t status = 0;
	uint8_t data_read[4];

	status = read_multi(dev, RegisterAdress, (uint8_t*)data_read, 4);
	*value =  ((data_read[0] << 24) | (data_read[1]<<16) |
			(data_read[2]<<8)| (data_read[3]));

	return status;
}

uint8_t VL53L4CD_RdWord(Dev_t dev, uint16_t RegisterAdress, uint16_t *value)
{
	uint8_t status = 0;
	uint8_t data_read[2];

	status = read_multi(dev, RegisterAdress, (uint8_t*)data_read, 2);
	*value = (data_read[0] << 8) | (data_read[1]);
	return status;
}

uint8_t VL53L4CD_RdByte(Dev_t dev, uint16_t RegisterAdress, uint8_t *value)
{
	uint8_t status = 0;
	uint8_t data_read[1];

	status = read_multi(dev, RegisterAdress, (uint8_t*)data_read, 1);
	*value = data_read[0];
	return status;
}

uint8_t VL53L4CD_WrByte(Dev_t dev, uint16_t RegisterAdress, uint8_t value)
{
	uint8_t data_write[1];

	data_write[0] = value & 0xFF;
	return(write_multi(dev, RegisterAdress, (uint8_t*)data_write, 1));
}

uint8_t VL53L4CD_WrWord(Dev_t dev, uint16_t RegisterAdress, uint16_t value)
{
	uint8_t data_write[2];

	data_write[0] = (value >> 8) & 0xFF;
	data_write[1] = value & 0xFF;
	return(write_multi(dev, RegisterAdress, (uint8_t*)data_write, 2));
}

uint8_t VL53L4CD_WrDWord(Dev_t dev, uint16_t RegisterAdress, uint32_t value)
{
	uint8_t data_write[4];

	data_write[0] = (value >> 24) & 0xFF;
	data_write[1] = (value >> 16) & 0xFF;
	data_write[2] = (value >> 8) & 0xFF;
	data_write[3] = value & 0xFF;
	return(write_multi(dev, RegisterAdress, (uint8_t*)data_write, 4));
}

uint8_t VL53L4CD_ReadMulti(
        Dev_t dev,
        uint16_t RegisterAdress,
        uint8_t *p_values,
        uint32_t size)
{
	return(read_multi(dev, RegisterAdress, (uint8_t*)p_values, size));
}

uint8_t	VL53L4CD_WaitMs(Dev_t dev, uint32_t time_ms)
{
	k_sleep(K_MSEC(time_ms));
	return 0;
}

uint8_t VL53L4CD_IsDataReady(Dev_t dev)
{
	uint8_t isReady = 0;
	do {
		VL53L4CD_WaitMs(dev, 5);
		VL53L4CD_CheckForDataReady(dev, &isReady);
	} while (isReady == 0);
	return 1;
}
