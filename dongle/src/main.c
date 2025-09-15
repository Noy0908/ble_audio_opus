/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Nordic UART Service Client sample
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/uart.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/logging/log.h>

#include "main.h"
#include "ble_app.h"


LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BLE_DONGLE_APP_LOG_LEVEL);


#define FW_VERSION					"1.0.2"

static const struct gpio_dt_spec leds[] = {
	GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(led3), gpios),
};


static int leds_init(void)
{
	if (!device_is_ready(leds[0].port)) {
		LOG_ERR("LEDs port not ready");
		return -ENODEV;
	}

	for (size_t i = 0; i < ARRAY_SIZE(leds); i++) {
		int err = gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT);

		if (err) {
			LOG_ERR("Unable to configure LED%u, err %d.", i, err);
			return err;
		}

		gpio_pin_set(leds[0].port, leds[i].pin, 0);
	}

	return 0;
}


int leds_toggle(uint8_t idx) 
{
	gpio_pin_toggle(leds[idx].port, leds[idx].pin);

	return 0;
}



static void button_changed(uint32_t button_state, uint32_t has_changed)
{
	static bool button_flag = false;
	uint32_t buttons = button_state & has_changed;

	LOG_INF("Button pressed at %d	 button_flag=0x%08X\n", k_cycle_get_32(), buttons);
#if 0
	if(buttons & KEY_MICROPHONE_SWITCH)
	{
		button_flag = !button_flag;
	 

	// wake up device and trigger micphone to work
		if(button_flag)
		{
			struct mic_work_event *mic_event = new_mic_work_event();
			mic_event->type = MIC_STATUS_START;
			APP_EVENT_SUBMIT(mic_event);
		}
		else
		{
			struct mic_work_event *mic_event = new_mic_work_event();
			mic_event->type = MIC_STATUS_STOP;
			APP_EVENT_SUBMIT(mic_event);
		}
	}
#ifdef CONFIG_BT_NUS_SECURITY_ENABLED
	else
	{
		confirm_pair_passkey(buttons);
	}
#endif

#endif
}


static void configure_gpio(void)
{
	int err;

	err = dk_buttons_init(button_changed);
	if (err) {
		LOG_ERR("Cannot init buttons (err: %d)", err);
	}

	err = leds_init();
	if (err) {
		LOG_ERR("Cannot init LEDs (err: %d)", err);
	}
}


int main(void)
{
	int err;

	LOG_WRN("BLE dongle sample is running, the version is %s\n", FW_VERSION);

	configure_gpio();

	err = ble_app_init();
	if (err) {
		return 0;
	}

	// for (;;) {
	// 	for(uint8_t i = 0; i<4; i++)
	// 	{
	// 		// dk_set_led(i, (blink_status) % 2);
	// 		leds_toggle(i);
	// 	}

	// 	k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	// }

}
