#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
// #include <zephyr/usb/usb_device.h>
// #include <zephyr/usb/class/usb_audio.h>
#include <pcm_mix.h>

#include "audio_handle.h"
#include "main.h"


LOG_MODULE_REGISTER(smart_dongle, CONFIG_BLE_DONGLE_APP_LOG_LEVEL);

#ifdef CONFIG_SUPPORT_OPUS_DECODER

#define AUDIO_HANDLE_STACK_SIZE        	30720
#define AUDIO_HANDLE_PRIORITY          	3

#define OPUS_DECODER_SIZE   			17944


K_MSGQ_DEFINE(audio_queue1, PCM_BLOCK_SIZE, PCM_BLOCK_COUNT, 4);
K_MSGQ_DEFINE(audio_queue2, PCM_BLOCK_SIZE, PCM_BLOCK_COUNT, 4);

/******************************** opus decoder variables ******************************************/
static uint8_t m_opus_channels   = CONFIG_OPUS_CHANNELS;
__ALIGN(4) static uint8_t m_opus_decoder[OPUS_DECODER_SIZE];
static OpusDecoder * const m_opus_decoder_state = (OpusDecoder *)m_opus_decoder;


K_MSGQ_DEFINE(m_msgq_rx_payloads, sizeof(struct audio_payload), 60, 4);



/*********************************opus decoder*********************************************/
static void opus_decoder_configure(void)
{
        printk("opus_decoder_get_size() = %d\n", opus_decoder_get_size(m_opus_channels));        
        __ASSERT_NO_MSG(opus_decoder_get_size(m_opus_channels) <= sizeof(m_opus_decoder));
        __ASSERT_NO_MSG(opus_decoder_init(m_opus_decoder_state, CONFIG_AUDIO_SAMPLING_FREQUENCY, m_opus_channels) == OPUS_OK);
}

void audio_buffer_handle(void)
{
    // int err = 0;
    struct audio_payload rx_payload;
	int16_t block_ptr[PCM_FRAME_BYTES];

	if(k_msgq_get(&m_msgq_rx_payloads, &rx_payload, K_FOREVER) == 0)
    {
		uint16_t pcm_index = 0;
		int frame_size = 0;
		uint8_t devID = rx_payload.dev_id;
		uint32_t packet_id = rx_payload.data[0] | (rx_payload.data[1] << 8) | (rx_payload.data[2] << 16) | (rx_payload.data[3] << 24);
        // LOG_INF("Packet received[%d] from %d, 0x%02x, 0x%02x, 0x%02x, 0x%02x  ", rx_payload.length,			
		// 		devID, rx_payload.data[0],rx_payload.data[1], rx_payload.data[2],rx_payload.data[3]);
		frame_size = opus_decode(m_opus_decoder_state, 
								&rx_payload.data[4], 
								CONFIG_AUDIO_FRAME_SIZE_BYTES, 
								block_ptr, 
								CONFIG_AUDIO_FRAME_SIZE_SAMPLES, 0);
																			
		LOG_INF("%d--%d", packet_id, frame_size);
	#if 1		
		/** send the PCM data to USB audio driver*/
		if(devID == 1)
		{
			while(pcm_index + FRAME_SIZE <= frame_size * 2) )
			{
				err = k_msgq_put(&audio_queue1, &block_ptr[pcm_index], K_NO_WAIT);
				if(!err)
				{
					pcm_index += FRAME_SIZE;
				}
				else
				{
					// LOG_ERR("[%d] Message sent error: %d", pcm_index, err);
					break;
				}
			}
			// LOG_INF("audio_queue1: %d", pcm_index);
		}
		else if(devID == 2)
		{	
			while(pcm_index + FRAME_SIZE <= frame_size * 2)
			{
				err = k_msgq_put(&audio_queue2, &block_ptr[pcm_index], K_NO_WAIT);			
				if(!err)
				{
					pcm_index += FRAME_SIZE;
				}
				else
				{
					// LOG_ERR("[%d] Message sent error: %d", pcm_index, err);
					break;
				}
			}
			// LOG_INF("audio_queue2: %d", pcm_index);
		}
		else
		{
			err = -EINVAL;
		}
				
		// if (err) {
		// 	LOG_ERR("Message sent error: %d", err);
		// }
	#endif
    } 
    else 
    {
        LOG_ERR("Error while reading esb rx packet");
    }
}

#else

#define AUDIO_HANDLE_STACK_SIZE        	4096
#define AUDIO_HANDLE_PRIORITY          	3

K_MSGQ_DEFINE(m_msgq_rx_payloads, sizeof(struct audio_payload), 60, 4);


void audio_buffer_handle(void)
{
    struct audio_payload rx_payload;

	if(k_msgq_get(&m_msgq_rx_payloads, &rx_payload, K_FOREVER) == 0)
    {
		uint8_t devID = rx_payload.dev_id;
		uint32_t packet_id = rx_payload.data[0] | (rx_payload.data[1] << 8) | (rx_payload.data[2] << 16) | (rx_payload.data[3] << 24);
        LOG_INF("Packet[%d] received[%d] bytes from dev--%d \n", packet_id, rx_payload.length, devID);

		/** send the audio packet to ble central */
		//bt_send_to_central(rx_payload.data, rx_payload.length);
    } 
    else 
    {
        LOG_ERR("Error while reading esb rx packet");
    }
}

#endif

static void ble_audio_data_handle(void *, void *, void *)
{
#ifdef CONFIG_SUPPORT_OPUS_DECODER
	opus_decoder_configure();
	LOG_INF("opus enabled");
	// LOG_INF("mic_frame_size = %d\t ", usb_audio_get_in_frame_size(mic_dev));
#endif

    while(1)
    {
		audio_buffer_handle();
    }
}


K_THREAD_DEFINE(ble_audio_service, AUDIO_HANDLE_STACK_SIZE,
                ble_audio_data_handle, NULL, NULL, NULL,
                K_PRIO_PREEMPT(AUDIO_HANDLE_PRIORITY), 0, 0);