/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Nordic UART Bridge Service (NUS) sample
 */
#include <uart_async_adapter.h>

#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/usb/usb_device.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <soc.h>

#include <dk_buttons_and_leds.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/logging/log.h>

#include "main.h"
#include "drv_mic.h"
#include "mic_work_event.h"
#include "ble_app.h"


LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BT_OPUS_LOG_LEVEL);

#define FW_VERSION					"1.1.7"

K_FIFO_DEFINE(fifo_uart_rx_data);


void error(void)
{
	LOG_ERR("Device enter error!!!!!!!!!!!!!");
	dk_set_leds_state(DK_ALL_LEDS_MSK, DK_NO_LEDS_MSK);

	while (true) {
		/* Spin for ever */
		k_sleep(K_MSEC(1000));
	}
}

void button_changed(uint32_t button_state, uint32_t has_changed)
{
	static bool button_flag = false;
	uint32_t buttons = button_state & has_changed;

	// LOG_INF("Button pressed at %d	 button_flag=0x%08X\n", k_cycle_get_32(), buttons);
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
}


static void configure_gpio(void)
{
	int err;

	err = dk_buttons_init(button_changed);
	if (err) {
		LOG_ERR("Cannot init buttons (err: %d)", err);
	}

	err = dk_leds_init();
	if (err) {
		LOG_ERR("Cannot init LEDs (err: %d)", err);
	}
}

int main(void)
{
	// int blink_status = 0;
	int err = 0;

	LOG_WRN("BLE microphone sample is running, the version is %s\n", FW_VERSION);

	configure_gpio();

	drv_audio_init();

	err = app_event_manager_init();
	if (err) {
		LOG_ERR("Unable to init Application Event Manager (%d)", err);
		return err;
	}

	err = ble_app_init();
	if (err) {
		error();
	}

	// for (;;) {
	// 	// for(uint8_t i = 0; i<4; i++)
	// 	// {
	// 	// 	dk_set_led(i, (blink_status) % 2);
	// 	// }
	// 	// blink_status++;

	// 	dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
	// 	k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	// }
}


