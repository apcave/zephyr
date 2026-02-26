# SPDX-License-Identifier: Apache-2.0

zephyr_library()

if(CONFIG_VL53L4CD)
	zephyr_library_sources(
		platform.c
		VL53L4CD_api.c
		VL53L4CD_calibration.c
		VL53L4CD_sensor.c
	)
endif()
