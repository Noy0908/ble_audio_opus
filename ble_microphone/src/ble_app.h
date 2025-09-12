#ifndef __BLE_APP_H__
#define __BLE_APP_H__


#define DEVICE_NAME                     CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN	                (sizeof(DEVICE_NAME) - 1)


/** device status bit */
#define STATUS_CONNECTED   (1 << 0)  // bit0:(1) Connected  	(0) Disconnected
#define STATUS_PAIRED      (1 << 1)  // bit1:(1) Paired  		(0) Unpaired
#define STATUS_NUS_READY   (1 << 2)  // bit2:(1) NUS Ready		(0) NUT Not Ready
#define STATUS_ADVERTISING (1 << 3)  // bit3:(1) Advertising 	(0) not advertising
// #define STATUS_BONDED      (1 << 4)  // bit4:(1) stored bond 	(0) no bond


#ifdef CONFIG_BT_NUS_SECURITY_ENABLED
void confirm_pair_passkey(uint32_t buttons);
#endif

int ble_app_init(void);

#endif