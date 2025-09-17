#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <bluetooth/conn_ctx.h> 
#include <bluetooth/services/nus.h>
#include <bluetooth/services/nus_client.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/scan.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>

#include "main.h"
#include "ble_app.h"
#include "audio_handle.h"


LOG_MODULE_DECLARE(LOG_MODULE_NAME);


BT_CONN_CTX_DEF(conns, CONFIG_BT_MAX_CONN, sizeof(struct bt_nus_client));


static struct k_work scan_work;

static struct bt_conn *default_conn;
// static struct bt_nus_client nus_client;



static uint8_t ble_data_received(struct bt_nus_client *nus,
						const uint8_t *data, uint16_t len)
{
	ARG_UNUSED(nus);
	static uint32_t timeCount1 = 0;
	uint8_t my_index = 99;		//invalid value
	// uint8_t nus_index = 99;

	/*How many connections are there in the Connection Context Library?*/
	// size_t num_nus_conns = bt_conn_ctx_count(&conns_ctx_lib);
	// for (size_t i = 0; i < num_nus_conns; i++) 
	// {
	// 	const struct bt_conn_ctx *ctx = bt_conn_ctx_get_by_id(&conns_ctx_lib, i);
	// 	if (ctx) {
	// 		if (ctx->data == nus) {
	// 			nus_index = i;
	// 			bt_conn_ctx_release(&conns_ctx_lib,
	// 					    (void *)ctx->data);
	// 			break;
	// 		}else {
	// 			bt_conn_ctx_release(&conns_ctx_lib,
	// 					    (void *)ctx->data);
	// 		}
	// 	}
	// }

	my_index = bt_conn_index(nus->conn);
	LOG_INF("dongle[%d] rec[%d]: 0x%02x, 0x%02x, 0x%02x, 0x%02x\n", my_index, len, data[0], data[1], data[2], data[3]);

	int ret = 0;
	static struct audio_payload rx_payload;

	if (len > MAX_PAYLOAD_SIZE) {
		LOG_ERR("Payload length %d exceeds maximum %d", len, MAX_PAYLOAD_SIZE);
		return -EMSGSIZE;
	}

	rx_payload.dev_id = my_index;

	if(0 == (timeCount1++ % 50))
	{
		if((rx_payload.dev_id == MIC_ID1) && switch_mic_flag)		//now choose microphone 1
		{
			leds_toggle(MIC_LED1);
		}
		else if((rx_payload.dev_id == MIC_ID2) && !switch_mic_flag)		//now choose microphone 2
		{
			leds_toggle(MIC_LED2);
		}
	}
#if 1	
	memcpy(rx_payload.data, data, len);
	rx_payload.length = len;
	ret = k_msgq_put(&m_msgq_rx_payloads, &rx_payload, K_MSEC(3));
	if (ret)  {
		LOG_INF("Audio message queue is full");
		return -ENOMEM;
	}
#endif
	return BT_GATT_ITER_CONTINUE;
}

static void conn_cnt_foreach(struct bt_conn *conn, void *data)
{
	size_t *cur_cnt = data;

	(*cur_cnt)++;
}

static size_t count_conn(void)
{
	size_t conn_count = 0;

	bt_conn_foreach(BT_CONN_TYPE_LE, conn_cnt_foreach, &conn_count);
	__ASSERT_NO_MSG(conn_count <= CONFIG_BT_MAX_CONN);

	return conn_count;
}


static void discovery_complete(struct bt_gatt_dm *dm,
			       void *context)
{
	struct bt_nus_client *nus = context;

	bt_gatt_dm_data_print(dm);

	bt_nus_handles_assign(dm, nus);
	/** Enable notification */
	bt_nus_subscribe_receive(nus);

	bt_gatt_dm_data_release(dm);

	/*How many connections are there in the Connection Context Library?*/
	size_t num_nus_conns = count_conn();
	LOG_INF("Service discovery completed. num_nus_conns = %d\n", num_nus_conns);

	if(num_nus_conns < CONFIG_BT_MAX_CONN)
	{
		int err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
		if (err) {
			LOG_ERR("Scanning failed to start (err %d)", err);
		} else {
			LOG_INF("Scanning started");
		}

		// (void)k_work_submit(&scan_work);
	}
	else
	{
		set_led_on(CON_STATUS_LED);
	}
}

static void discovery_service_not_found(struct bt_conn *conn,
					void *context)
{
	LOG_INF("Service not found");
}

static void discovery_error(struct bt_conn *conn,
			    int err,
			    void *context)
{
	LOG_WRN("Error while discovering GATT database: (%d)", err);
}

struct bt_gatt_dm_cb discovery_cb = {
	.completed         = discovery_complete,
	.service_not_found = discovery_service_not_found,
	.error_found       = discovery_error,
};

static void gatt_discover(struct bt_conn *conn)
{
	int err;

	struct bt_nus_client *nus_client = bt_conn_ctx_get(&conns_ctx_lib, conn);
	if (!nus_client) {
		return;
	}

	err = bt_gatt_dm_start(conn,
			       BT_UUID_NUS_SERVICE,
			       &discovery_cb,
			       nus_client);
	if (err) {
		LOG_ERR("could not start the discovery procedure, error "
			"code: %d", err);
	}

	bt_conn_ctx_release(&conns_ctx_lib, (void *) nus_client);
}

static void exchange_func(struct bt_conn *conn, uint8_t err, struct bt_gatt_exchange_params *params)
{
	if (!err) {
		uint16_t att_mtu = bt_gatt_get_mtu(conn);
        uint16_t payload = att_mtu - 3; /* ATT header */
        LOG_INF("MTU exchange successful: ATT MTU=%u, payload=%u\n", att_mtu, payload);
	} else {
		LOG_WRN("MTU exchange failed (err %" PRIu8 ")", err);
	}
}

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		LOG_INF("Failed to connect to %s, 0x%02x %s", addr, conn_err,
			bt_hci_err_to_str(conn_err));

		if (default_conn == conn) {
			bt_conn_unref(default_conn);
			default_conn = NULL;

			(void)k_work_submit(&scan_work);
		}

		return;
	}

	LOG_INF("Connected: %s", addr);

	/** update MTU and security level */
	static struct bt_gatt_exchange_params exchange_params;
	exchange_params.func = exchange_func;
	err = bt_gatt_exchange_mtu(conn, &exchange_params);
	if (err) {
		LOG_WRN("MTU exchange failed (err %d)", err);
	}

	// err = bt_conn_set_security(conn, BT_SECURITY_L2);
	// if (err) {
	// 	LOG_WRN("Failed to set security: %d", err);
	// }
#if 1
	/*Allocate memory for this connection using the connection context library. For reference,
	this code was taken from hids.c
	*/
	struct bt_nus_client *nus_client = bt_conn_ctx_alloc(&conns_ctx_lib, conn);
	if (!nus_client) {
		LOG_WRN("There is no free memory to allocate the connection context");
	}
	
	struct bt_nus_client_init_param init = {
		.cb = {
			.received = ble_data_received,
			// .sent = ble_data_sent,
		}
	};

	memset(nus_client, 0, bt_conn_ctx_block_size_get(&conns_ctx_lib));
	err = bt_nus_client_init(nus_client, &init);
	bt_conn_ctx_release(&conns_ctx_lib, (void *)nus_client);
	if (err) {
		LOG_ERR("NUS Client initialization failed (err %d)", err);
	}else{
		LOG_INF("NUS Client module initialized");
	}
#endif
	gatt_discover(conn);

	err = bt_scan_stop();
	if ((!err) && (err != -EALREADY)) {
		LOG_ERR("Stop LE scan failed (err %d)", err);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	int err;
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Disconnected: %s, reason 0x%02x %s", addr, reason, bt_hci_err_to_str(reason));

	err = bt_conn_ctx_free(&conns_ctx_lib, conn);

	bt_conn_unref(conn);
	default_conn = NULL;

	(void)k_work_submit(&scan_work);
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		LOG_INF("Security changed: %s level %u", addr, level);
	} else {
		LOG_WRN("Security failed: %s level %u err %d %s", addr, level, err,
			bt_security_err_to_str(err));
	}

	gatt_discover(conn);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed
};

static void scan_filter_match(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match,
			      bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	LOG_INF("Filters matched. Address: %s connectable: %d",
		addr, connectable);
}

static void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	LOG_WRN("Connecting failed");
}

static void scan_connecting(struct bt_scan_device_info *device_info,
			    struct bt_conn *conn)
{
	default_conn = bt_conn_ref(conn);
}


BT_SCAN_CB_INIT(scan_cb, scan_filter_match, NULL,
		scan_connecting_error, scan_connecting);

static void try_add_address_filter(const struct bt_bond_info *info, void *user_data)
{
	int err;
	char addr[BT_ADDR_LE_STR_LEN];
	uint8_t *filter_mode = user_data;

	bt_addr_le_to_str(&info->addr, addr, sizeof(addr));

	struct bt_conn *conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &info->addr);

	if (conn) {
		bt_conn_unref(conn);
		return;
	}

	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_ADDR, &info->addr);
	if (err) {
		LOG_ERR("Address filter cannot be added (err %d): %s", err, addr);
		return;
	}

	LOG_INF("Address filter added: %s", addr);
	*filter_mode |= BT_SCAN_ADDR_FILTER;
}

static int scan_start(void)
{
	int err;
	uint8_t filter_mode = 0;

	err = bt_scan_stop();
	if (err) {
		LOG_ERR("Failed to stop scanning (err %d)", err);
		return err;
	}

	bt_scan_filter_remove_all();

	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_NUS_SERVICE);
	if (err) {
		LOG_ERR("UUID filter cannot be added (err %d", err);
		return err;
	}
	filter_mode |= BT_SCAN_UUID_FILTER;

	bt_foreach_bond(BT_ID_DEFAULT, try_add_address_filter, &filter_mode);

	err = bt_scan_filter_enable(filter_mode, false);
	if (err) {
		LOG_ERR("Filters cannot be turned on (err %d)", err);
		return err;
	}

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		LOG_ERR("Scanning failed to start (err %d)", err);
		return err;
	}

	LOG_INF("Scan started");
	return 0;
}

static void scan_work_handler(struct k_work *item)
{
	ARG_UNUSED(item);

	(void)scan_start();
}

static void scan_init(void)
{
	struct bt_scan_init_param scan_init = {
		.connect_if_match = true,
		.conn_param = BT_LE_CONN_PARAM(12, 12, 0, 400),		//15ms CI
	};

	bt_scan_init(&scan_init);
	bt_scan_cb_register(&scan_cb);

	k_work_init(&scan_work, scan_work_handler);
	LOG_INF("Scan module initialized");
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing cancelled: %s", addr);
}

// static void pairing_confirm(struct bt_conn *conn)
// {
// 	char addr[BT_ADDR_LE_STR_LEN];

// 	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

// 	bt_conn_auth_pairing_confirm(conn);

// 	LOG_INF("Pairing confirmed: %s", addr);
// }

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing completed: %s, bonded: %d", addr, bonded);
}


static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_WRN("Pairing failed conn: %s, reason %d %s", addr, reason,
		bt_security_err_to_str(reason));
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
	.cancel = auth_cancel,
	// .pairing_confirm = pairing_confirm
};

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed
};


int ble_app_init(void)
{
    int err = 0;

	err = bt_conn_auth_cb_register(&conn_auth_callbacks);
	if (err) {
		LOG_ERR("Failed to register authorization callbacks.");
		return err;
	}

	err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
	if (err) {
		printk("Failed to register authorization info callbacks.\n");
		return err;
	}

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return err;
	}
	LOG_INF("Bluetooth initialized");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	scan_init();
	err = scan_start();
	if (err) {
		return err;
	}

	printk("Starting Bluetooth Central UART sample\n");

    return 0;
}