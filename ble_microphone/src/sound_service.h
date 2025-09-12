#ifndef __SOUND_SERVICE_H__
#define __SOUND_SERVICE_H__

#include "drv_mic.h"
#include "opus.h"
#include "opus_application_config.h"


#define MAX_PAYLOAD_SIZE	(80 + 4) // 80 bytes for Opus payload + 4 bytes for packet ID

/** @brief ble opus payload.
 *
 *  The payload is used both for transmissions and for acknowledging a
 *  received packet with a payload.
 */
struct audio_payload {
    uint8_t dev_id; /**< Device ID, used to identify the device that sent the packet. */
	uint8_t length; /**< Length of the packet when not in DPL mode. */
	uint8_t data[MAX_PAYLOAD_SIZE]; /**< The payload data and devID. */
};


extern struct k_msgq m_msgq_tx_payloads;

#endif



