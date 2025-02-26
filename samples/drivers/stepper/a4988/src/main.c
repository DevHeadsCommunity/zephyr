/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Marvin Ouma
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/stepper.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/stepper/stepper_trinamic.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(a4988log);

const struct device *stepper = DEVICE_DT_GET(DT_ALIAS(stepper));
const struct device *const console = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
uint32_t dtr = 0;

int32_t ping_pong_target_position =
	CONFIG_STEPS_PER_REV * 1 * DT_PROP(DT_ALIAS(stepper), micro_step_res);

K_SEM_DEFINE(steps_completed_sem, 0, 1);

void stepper_callback(const struct device *dev, const enum stepper_event event, void *user_data)
{
	switch (event) {
	case STEPPER_EVENT_STEPS_COMPLETED:
		k_sem_give(&steps_completed_sem);
		break;
	default:
		break;
	}
}

int main(void)
{
	if (usb_enable(NULL)) {
		return 0;
	}

	while (!dtr) {
		uart_line_ctrl_get(console, UART_LINE_CTRL_DTR, &dtr);
		k_sleep(K_MSEC(100));
	}

	LOG_INF("Starting A4988 stepper sample");
	if (!device_is_ready(stepper)) {
		LOG_INF("Device %s is not ready", stepper->name);
		return 0;
	}
	LOG_INF("stepper is %p, name is %s", stepper, stepper->name);
	LOG_INF("target position %d", ping_pong_target_position);
	stepper_set_event_callback(stepper, stepper_callback, NULL);
	stepper_enable(stepper, true);
	stepper_set_reference_position(stepper, 0);
	stepper_set_microstep_interval(stepper, 200);
	stepper_move_by(stepper, ping_pong_target_position);

	for (;;) {
		if (k_sem_take(&steps_completed_sem, K_FOREVER) == 0) {
			ping_pong_target_position = 1;
			stepper_run(stepper, 1);
		}
	}
	return 0;
}
