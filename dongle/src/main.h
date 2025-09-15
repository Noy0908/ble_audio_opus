#ifndef __MAIN_H__
#define __MAIN_H__

#include <dk_buttons_and_leds.h>


#define LOG_MODULE_NAME             dongle

#define RUN_LED_BLINK_INTERVAL          1000


/* UART payload buffer element size. */
#define UART_BUF_SIZE               20

#define CON_STATUS_LED              DK_LED1
#define USB_AUDIO_LED              	DK_LED2
#define MIC_LED1              		DK_LED3
#define MIC_LED2              		DK_LED4

		
#define KEY_PASSKEY_ACCEPT          DK_BTN1_MSK
#define KEY_PASSKEY_REJECT          DK_BTN2_MSK

#define NUS_WRITE_TIMEOUT           K_MSEC(150)
#define UART_WAIT_FOR_BUF_DELAY     K_MSEC(50)
#define UART_RX_TIMEOUT             50000 /* Wait for RX complete event time in microseconds. */




struct uart_data_t {
	void *fifo_reserved;
	uint8_t  data[UART_BUF_SIZE];
	uint16_t len;
};


extern int leds_toggle(uint8_t idx);


#endif