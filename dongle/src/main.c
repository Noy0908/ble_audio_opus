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

#include <zephyr/logging/log.h>

#include "main.h"
#include "ble_app.h"


LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BLE_DONGLE_APP_LOG_LEVEL);

static const struct device *uart = DEVICE_DT_GET(DT_CHOSEN(nordic_nus_uart));
static struct k_work_delayable uart_work;


K_SEM_DEFINE(nus_write_sem, 0, 1);


static K_FIFO_DEFINE(fifo_uart_tx_data);
static K_FIFO_DEFINE(fifo_uart_rx_data);


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
	gpio_pin_toggle(leds[0].port, leds[idx].pin);

	return 0;
}



// static void ble_data_sent(struct bt_nus_client *nus, uint8_t err,
// 					const uint8_t *const data, uint16_t len)
// {
// 	ARG_UNUSED(nus);
// 	ARG_UNUSED(data);
// 	ARG_UNUSED(len);

// 	k_sem_give(&nus_write_sem);

// 	if (err) {
// 		LOG_WRN("ATT error code: 0x%02X", err);
// 	}
// }



static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
	ARG_UNUSED(dev);

	static size_t aborted_len;
	struct uart_data_t *buf;
	static uint8_t *aborted_buf;
	static bool disable_req;

	switch (evt->type) {
	case UART_TX_DONE:
		LOG_DBG("UART_TX_DONE");
		if ((evt->data.tx.len == 0) ||
		    (!evt->data.tx.buf)) {
			return;
		}

		if (aborted_buf) {
			buf = CONTAINER_OF(aborted_buf, struct uart_data_t,
					   data[0]);
			aborted_buf = NULL;
			aborted_len = 0;
		} else {
			buf = CONTAINER_OF(evt->data.tx.buf,
					   struct uart_data_t,
					   data[0]);
		}

		k_free(buf);

		buf = k_fifo_get(&fifo_uart_tx_data, K_NO_WAIT);
		if (!buf) {
			return;
		}

		if (uart_tx(uart, buf->data, buf->len, SYS_FOREVER_MS)) {
			LOG_WRN("Failed to send data over UART");
		}

		break;

	case UART_RX_RDY:
		LOG_DBG("UART_RX_RDY");
		buf = CONTAINER_OF(evt->data.rx.buf, struct uart_data_t, data[0]);
		buf->len += evt->data.rx.len;

		if (disable_req) {
			return;
		}

		if ((evt->data.rx.buf[buf->len - 1] == '\n') ||
		    (evt->data.rx.buf[buf->len - 1] == '\r')) {
			disable_req = true;
			uart_rx_disable(uart);
		}

		break;

	case UART_RX_DISABLED:
		LOG_DBG("UART_RX_DISABLED");
		disable_req = false;

		buf = k_malloc(sizeof(*buf));
		if (buf) {
			buf->len = 0;
		} else {
			LOG_WRN("Not able to allocate UART receive buffer");
			k_work_reschedule(&uart_work, UART_WAIT_FOR_BUF_DELAY);
			return;
		}

		uart_rx_enable(uart, buf->data, sizeof(buf->data),
			       UART_RX_TIMEOUT);

		break;

	case UART_RX_BUF_REQUEST:
		LOG_DBG("UART_RX_BUF_REQUEST");
		buf = k_malloc(sizeof(*buf));
		if (buf) {
			buf->len = 0;
			uart_rx_buf_rsp(uart, buf->data, sizeof(buf->data));
		} else {
			LOG_WRN("Not able to allocate UART receive buffer");
		}

		break;

	case UART_RX_BUF_RELEASED:
		LOG_DBG("UART_RX_BUF_RELEASED");
		buf = CONTAINER_OF(evt->data.rx_buf.buf, struct uart_data_t,
				   data[0]);

		if (buf->len > 0) {
			k_fifo_put(&fifo_uart_rx_data, buf);
		} else {
			k_free(buf);
		}

		break;

	case UART_TX_ABORTED:
		LOG_DBG("UART_TX_ABORTED");
		if (!aborted_buf) {
			aborted_buf = (uint8_t *)evt->data.tx.buf;
		}

		aborted_len += evt->data.tx.len;
		buf = CONTAINER_OF(aborted_buf, struct uart_data_t,
				   data[0]);

		uart_tx(uart, &buf->data[aborted_len],
			buf->len - aborted_len, SYS_FOREVER_MS);

		break;

	default:
		break;
	}
}

static void uart_work_handler(struct k_work *item)
{
	struct uart_data_t *buf;

	buf = k_malloc(sizeof(*buf));
	if (buf) {
		buf->len = 0;
	} else {
		LOG_WRN("Not able to allocate UART receive buffer");
		k_work_reschedule(&uart_work, UART_WAIT_FOR_BUF_DELAY);
		return;
	}

	uart_rx_enable(uart, buf->data, sizeof(buf->data), UART_RX_TIMEOUT);
}

static int uart_init(void)
{
	int err;
	struct uart_data_t *rx;

	if (!device_is_ready(uart)) {
		LOG_ERR("UART device not ready");
		return -ENODEV;
	}

	rx = k_malloc(sizeof(*rx));
	if (rx) {
		rx->len = 0;
	} else {
		return -ENOMEM;
	}

	k_work_init_delayable(&uart_work, uart_work_handler);

	err = uart_callback_set(uart, uart_cb, NULL);
	if (err) {
		return err;
	}

	return uart_rx_enable(uart, rx->data, sizeof(rx->data),
			      UART_RX_TIMEOUT);
}


int main(void)
{
	int err;

	leds_init();

	err = uart_init();
	if (err != 0) {
		LOG_ERR("uart_init failed (err %d)", err);
		return 0;
	}

	err = ble_app_init();
	if (err) {
		return 0;
	}


	// struct uart_data_t nus_data = {
	// 	.len = 0,
	// };

	// for (;;) {
	// 	/* Wait indefinitely for data to be sent over Bluetooth */
	// 	struct uart_data_t *buf = k_fifo_get(&fifo_uart_rx_data,
	// 					     K_FOREVER);

	// 	int plen = MIN(sizeof(nus_data.data) - nus_data.len, buf->len);
	// 	int loc = 0;

	// 	while (plen > 0) {
	// 		memcpy(&nus_data.data[nus_data.len], &buf->data[loc], plen);
	// 		nus_data.len += plen;
	// 		loc += plen;
	// 		if (nus_data.len >= sizeof(nus_data.data) ||
	// 		   (nus_data.data[nus_data.len - 1] == '\n') ||
	// 		   (nus_data.data[nus_data.len - 1] == '\r')) {
	// 			err = bt_nus_client_send(&nus_client, nus_data.data, nus_data.len);
	// 			if (err) {
	// 				LOG_WRN("Failed to send data over BLE connection"
	// 					"(err %d)", err);
	// 			}

	// 			err = k_sem_take(&nus_write_sem, NUS_WRITE_TIMEOUT);
	// 			if (err) {
	// 				LOG_WRN("NUS send timeout");
	// 			}

	// 			nus_data.len = 0;
	// 		}

	// 		plen = MIN(sizeof(nus_data.data), buf->len - loc);
	// 	}

	// 	k_free(buf);
	// }
}
