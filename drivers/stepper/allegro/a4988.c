/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Marvin Ouma
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT allegro_a4988

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/stepper.h>
#include "../step_dir/step_dir_stepper_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(a4988, CONFIG_STEPPER_LOG_LEVEL);

#define MSX_PIN_COUNT       3
#define MSX_PIN_STATE_COUNT 5

static enum stepper_micro_step_resolution a4988_msx_resolutions[MSX_PIN_STATE_COUNT] = {
	STEPPER_MICRO_STEP_1, STEPPER_MICRO_STEP_2,  STEPPER_MICRO_STEP_4,
	STEPPER_MICRO_STEP_8, STEPPER_MICRO_STEP_16,
};

struct a4988_config {
	struct step_dir_stepper_common_config common;
	struct gpio_dt_spec sleep_pin;
	struct gpio_dt_spec enable_pin;
	struct gpio_dt_spec reset;
	struct gpio_dt_spec *msx_pins;
	enum stepper_micro_step_resolution *msx_resolutions;
};

struct a4988_data {
	struct step_dir_stepper_common_data common;
	enum stepper_micro_step_resolution resolution;
};

STEP_DIR_STEPPER_STRUCT_CHECK(struct a4988_config, struct a4988_data);

static int a4988_stepper_enable(const struct device *dev, const bool enable)
{
	/* enable and sleep pins need to be set logic low to be on  */
	const struct a4988_config *config = dev->config;
	int ret;

	if (config->sleep_pin.port != NULL) {
		ret = gpio_pin_set_dt(&config->sleep_pin, enable);
		if (ret < 0) {
			LOG_WRN("Failed to set sleep pin %d", ret);
		}
	}

	LOG_DBG("Stepper motor controller %s %s", dev->name, enable ? "enabled" : "disabled");
	return gpio_pin_set_dt(&config->enable_pin, !enable);
}

/* Set microstepping mode
 *
 */
static int a4988_stepper_set_micro_step_res(const struct device *dev,
					    enum stepper_micro_step_resolution micro_step_res)
{
	struct a4988_data *data = dev->data;
	const struct a4988_config *config = dev->config;
	int ret;

	if (!config->msx_pins) {
		LOG_ERR("Microstep resolution pins are not configured");
		return 0; // NOT CONNECTED
	}

	for (uint8_t i = 0; i < MSX_PIN_STATE_COUNT; ++i) {
		if (micro_step_res != config->msx_resolutions[i]) {
			continue;
		}

		ret = gpio_pin_set_dt(&config->msx_pins[0], i & 0x01);
		if (ret < 0) {
			LOG_ERR("Failed to set MS1 pin: %d", ret);
			return ret;
		}

		ret = gpio_pin_set_dt(&config->msx_pins[1], (i & 0x02) >> 1);
		if (ret < 0) {
			LOG_ERR("Failed to set MS2 pin: %d", ret);
			return ret;
		}

		ret = gpio_pin_set_dt(&config->msx_pins[2], (i & 0x04) >> 1);
		if (ret < 0) {
			LOG_ERR("Failed to set MS3 pin: %d", ret);
			return ret;
		}

		data->resolution = micro_step_res;
		return 0;
	}

	LOG_ERR("Unsupported microstep resolution: %d", micro_step_res);
	return -EINVAL;
}

static int a4988_stepper_configure_msx_pins(const struct device *dev)
{
	const struct a4988_config *config = dev->config;
	int ret;

	for (uint8_t i = 0; i < MSX_PIN_COUNT; i++) {
		if (!gpio_is_ready_dt(&config->msx_pins[i])) {
			LOG_ERR("MSX pin %u is not ready", i);
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->msx_pins[i], GPIO_OUTPUT);
		if (ret < 0) {
			LOG_ERR("Failed to configure msx pin %u", i);
			return ret;
		}
	}
	return 0;
}

static int a4988_stepper_get_micro_step_res(const struct device *dev,
					    enum stepper_micro_step_resolution *micro_step_res)
{

	struct a4988_data *data = dev->data;
	*micro_step_res = data->resolution;
	return 0;
}

static int a4988_stepper_init(const struct device *dev)
{
	const struct a4988_config *config = dev->config;
	const struct a4988_data *data = dev->data;
	int ret;

	if (config->sleep_pin.port != NULL) {
		ret = gpio_pin_configure_dt(&config->sleep_pin, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure sleep pin: %d", ret);
			return -ENODEV;
		}
	}

	if (!gpio_is_ready_dt(&config->enable_pin)) {
		LOG_ERR("GPIO pins are not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->enable_pin, GPIO_OUTPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure enable pin: %d", ret);
		return ret;
	}

	if (config->msx_pins) {
		ret = a4988_stepper_configure_msx_pins(dev);
		if (ret < 0) {
			LOG_ERR("Failed to configure MSX pins: %d", ret);
			return ret;
		}

		ret = a4988_stepper_set_micro_step_res(dev, data->resolution);
		if (ret < 0) {
			LOG_ERR("Failed to set microstep resolution: %d", ret);
			return ret;
		}
	}

	ret = step_dir_stepper_common_init(dev);
	if (ret < 0) {
		LOG_ERR("Failed to init step dir common stepper: %d", ret);
		return ret;
	}
	return 0;
}

static DEVICE_API(stepper, a4988_stepper_api) = {
	.enable = a4988_stepper_enable,
	.move_by = step_dir_stepper_common_move_by,
	.is_moving = step_dir_stepper_common_is_moving,
	.set_reference_position = step_dir_stepper_common_set_reference_position,
	.get_actual_position = step_dir_stepper_common_get_actual_position,
	.move_to = step_dir_stepper_common_move_to,
	.set_microstep_interval = step_dir_stepper_common_set_microstep_interval,
	.run = step_dir_stepper_common_run,
	.set_event_callback = step_dir_stepper_common_set_event_callback,
	.set_micro_step_res = a4988_stepper_set_micro_step_res,
	.get_micro_step_res = a4988_stepper_get_micro_step_res,
};

#define A4988_STEPPER_DEFINE(inst, msx_table)                                                        \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, msx_gpios), (                                       \
	static const struct gpio_dt_spec a4988_stepper_msx_pins_##inst[] = {                     \
		DT_INST_FOREACH_PROP_ELEM_SEP(                                                     \
			inst, msx_gpios, GPIO_DT_SPEC_GET_BY_IDX, (,)                              \
		),                                                                                 \
	};                                                                                         \
	BUILD_ASSERT(                                                                              \
		ARRAY_SIZE(a4988_stepper_msx_pins_##inst) == MSX_PIN_COUNT,                      \
		"Three microstep config pins needed");                                               \
	)) \
                                                                                                     \
	static const struct a4988_config a4988_config_##inst = {                                     \
		.common = STEP_DIR_STEPPER_DT_INST_COMMON_CONFIG_INIT(inst),                         \
		.enable_pin = GPIO_DT_SPEC_INST_GET(inst, en_gpios),                                 \
		.msx_resolutions = msx_table,                                                        \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, msx_gpios),				   \
			   (.msx_pins = (struct gpio_dt_spec *)a4988_stepper_msx_pins_##inst)) };         \
	static struct a4988_data a4988_data_##inst = {                                               \
		.common = STEP_DIR_STEPPER_DT_INST_COMMON_DATA_INIT(inst),                           \
		.resolution = DT_INST_PROP(inst, micro_step_res),                                    \
	};                                                                                           \
	DEVICE_DT_INST_DEFINE(inst, a4988_stepper_init, NULL, &a4988_data_##inst,                    \
			      &a4988_config_##inst, POST_KERNEL, CONFIG_STEPPER_INIT_PRIORITY,       \
			      &a4988_stepper_api);

DT_INST_FOREACH_STATUS_OKAY_VARGS(A4988_STEPPER_DEFINE, a4988_msx_resolutions)
