#ifndef __MAIN_H__
#define __MAIN_H__

#include <dk_buttons_and_leds.h>


#define LOG_MODULE_NAME             central_uart


/* UART payload buffer element size. */
#define UART_BUF_SIZE               20

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


#endif