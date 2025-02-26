.. zephyr:code-sample:: stepper/a4988
   :name: A4988 Stepper Driver
   :relevant-api: stepper_interface

   Rotate a A4988 stepper motor.

Overview
********

This sample applications rotates the A4988 stepper motor at a constant speed.


Building and Running
********************

The sample applications spins the stepper and outputs the events to the console. It requires
an A4988 stepper driver. It should work with any platform with enough gpios to spare.
It does not work on QEMU.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/stepper/a4988
   :board: stm32_min_dev
   :goals: build flash
