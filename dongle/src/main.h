#ifndef __MAIN_H__
#define __MAIN_H__

#include <dk_buttons_and_leds.h>


#define LOG_MODULE_NAME             dongle

#define RUN_LED_BLINK_INTERVAL          1000


/* UART payload buffer element size. */
#define UART_BUF_SIZE               20


#define MIC_LED1              		DK_LED1
#define MIC_LED2              		DK_LED2
#define USB_AUDIO_LED              	DK_LED3
#define CON_STATUS_LED              DK_LED4

#define MIC_ID1              		0
#define MIC_ID2              		1

#define KEY_MICROPHONE_SWITCH       DK_BTN1_MSK


extern bool switch_mic_flag;

extern int leds_toggle(uint8_t idx);

extern void set_led_on(uint8_t idx);


#endif