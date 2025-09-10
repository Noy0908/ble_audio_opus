#ifndef __BLE_APP_H__
#define __BLE_APP_H__


#define DEVICE_NAME                     CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN	                (sizeof(DEVICE_NAME) - 1)


#ifdef CONFIG_BT_NUS_SECURITY_ENABLED
void confirm_pair_passkey(uint32_t buttons);
#endif

int ble_app_init(void);

#endif