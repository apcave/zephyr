.. zephyr:code-sample:: vl53l4cd
   :name: VL53L4CD Time Of Flight sensor
   :relevant-api: sensor_interface

   Get distance data from a VL53L4CD sensor (polling mode).

Overview
********

This sample periodically measures distance between vl53l4cd sensor
and target. The result is displayed on the console.
It shows the usage of all available channels including private ones.

Requirements
************

This sample uses the VL53L4CD sensor controlled using the I2C interface.

References
**********

 - VL53L4CD: https://www.st.com/en/imaging-and-photonics-solutions/vl53l4cd.html

Building and Running
********************

This project outputs sensor data to the console. It requires a VL53L4CD
sensor, which is present on the i2c bus as defined in the board's device tree and app.overlay file.
The sample can be built and run on any board with a compatible sensor and I2C interface.
For example, to build and flash the sample for the Seeed Studio XIAO nRF52840 Sense,
you can use the following command:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/vl53l4cd/
   :goals: build flash


Sample Output
=============

.. code-block:: console

[00:00:00.296,600] <wrn> VL53L4CD: --------------------------------------------------------
[00:00:00.798,034] <inf> api: stmVL53L4CD: module_id : 0xeb. module_type : 0xaa
[00:00:00.887,481] <wrn> VL53L4CD: --------------------------------------------------------
*** Booting Zephyr OS build v4.3.0-6886-g7367513d916a ***
[00:00:00.887,786] <inf> range: Initializing range sensor...
[00:00:00.887,817] <inf> range: Low power mode enabled
[00:00:00.891,479] <inf> range: Proximity mode enabled
[00:00:00.893,310] <inf> range: Start the sensor ranging...
[00:00:00.894,805] <inf> range: Place object within 100mm of the sensor to see proximity mode in action
[00:00:06.860,412] <inf> range: vl53l4cd@29: Distance (mm) 66, Standard Deviation (mm) 1
[00:00:07.839,660] <inf> range: vl53l4cd@29: Distance (mm) 73, Standard Deviation (mm) 2
[00:00:08.814,849] <inf> range: vl53l4cd@29: Distance (mm) 75, Standard Deviation (mm) 1
[00:00:09.787,689] <inf> range: vl53l4cd@29: Distance (mm) 66, Standard Deviation (mm) 1
[00:00:10.762,878] <inf> range: vl53l4cd@29: Distance (mm) 64, Standard Deviation (mm) 1
[00:00:11.742,126] <inf> range: vl53l4cd@29: Distance (mm) 59, Standard Deviation (mm) 1
[00:00:12.717,315] <inf> range: vl53l4cd@29: Distance (mm) 55, Standard Deviation (mm) 1
