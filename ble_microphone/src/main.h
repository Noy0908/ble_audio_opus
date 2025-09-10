#ifndef __MAIN_H__

#define __MAIN_H__

#include <dk_buttons_and_leds.h>

#define LOG_MODULE_NAME 				microphone

#define RUN_STATUS_LED                  DK_LED1
#define RUN_LED_BLINK_INTERVAL          1000

#define CON_STATUS_LED                  DK_LED2

#define MIC_STATUS_LED                  DK_LED3

#define KEY_PASSKEY_ACCEPT              DK_BTN1_MSK
#define KEY_PASSKEY_REJECT              DK_BTN2_MSK
#define KEY_MICROPHONE_SWITCH           DK_BTN3_MSK

#define UART_BUF_SIZE                   CONFIG_BT_NUS_UART_BUFFER_SIZE
#define UART_WAIT_FOR_BUF_DELAY         K_MSEC(50)
#define UART_WAIT_FOR_RX                CONFIG_BT_NUS_UART_RX_WAIT_TIME



struct uart_data_t {
	void *fifo_reserved;
	uint8_t data[UART_BUF_SIZE];
	uint16_t len;
};

#endif