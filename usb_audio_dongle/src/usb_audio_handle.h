#ifndef __USB_AUDIO_APP_H__
#define __USB_AUDIO_APP_H__

#include <stdint.h>
#include "opus_application_config.h"
#include "opus.h"

#define MAX_SAMPLE_RATE             16000
#define SAMPLE_BIT_WIDTH            16
#define BYTES_PER_SAMPLE            sizeof(int16_t)

// #define CONFIG_AUDIO_SAMPLE_RATE_HZ         16000
// #define CONFIG_AUDIO_BIT_DEPTH_OCTETS       2

#define USB_FRAME_SIZE_STEREO               (((MAX_SAMPLE_RATE * BYTES_PER_SAMPLE) / 1000) *2)

#define CONFIG_FIFO_FRAME_SPLIT_NUM         100




/* Size of a block for 1 ms of audio data. */
#define BLOCK_SIZE(_sample_rate, _number_of_ms) \
	(BYTES_PER_SAMPLE * (_sample_rate / 1000) * _number_of_ms)

// #define MAX_BLOCK_SIZE              (BLOCK_SIZE(MAX_SAMPLE_RATE, 1) * 5)	// 10 ms

#define PCM_BLOCK_SIZE				BLOCK_SIZE(MAX_SAMPLE_RATE, 2) 			// 1 ms
#define FRAME_SIZE                  ((MAX_SAMPLE_RATE / 1000) * 2)			// 1 ms

#define PCM_FRAME_BYTES 			(CONFIG_AUDIO_FRAME_SIZE_SAMPLES * CONFIG_OPUS_CHANNELS)


#define PCM_BLOCK_COUNT             50

#define MAX_PAYLOAD_SIZE			(160 + 4) // 80 bytes for Opus payload + 4 bytes for packet ID



/** @brief Enhanced ShockBurst payload.
 *
 *  The payload is used both for transmissions and for acknowledging a
 *  received packet with a payload.
 */
struct audio_payload {
    uint8_t dev_id; /**< Device ID, used to identify the device that sent the packet. */
	uint8_t length; /**< Length of the packet when not in DPL mode. */
	uint8_t data[MAX_PAYLOAD_SIZE]; /**< The payload data and devID. */
};


extern struct k_msgq m_msgq_rx_payloads;

#endif






