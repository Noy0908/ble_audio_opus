#include <zephyr/kernel.h>
#include <zephyr/audio/dmic.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>
#include "sound_service.h"
#include "mic_work_event.h"
#include "main.h"


LOG_MODULE_REGISTER(sound_service, LOG_LEVEL_INF);

#define SOUND_STACK_SIZE        30720
#define SOUND_PRIORITY          5

/* Milliseconds to wait for a block to be read. */
#define READ_TIMEOUT            1000

/** opus variables and functions */
#define OPUS_ENCODER_SIZE   11924


K_MSGQ_DEFINE(m_msgq_tx_payloads, sizeof(struct audio_payload), 100, 4);


__ALIGN(4) static uint8_t m_opus_encoder[OPUS_ENCODER_SIZE];
static OpusEncoder * const m_opus_encoder_state = (OpusEncoder *)m_opus_encoder;

static uint8_t m_opus_complexity = CONFIG_OPUS_COMPLEXITY;
static int32_t m_opus_bitrate    = ((CONFIG_OPUS_BITRATE != 0) ? CONFIG_OPUS_BITRATE : OPUS_AUTO);
static bool m_opus_vbr        = ((CONFIG_OPUS_BITRATE == 0) || (CONFIG_OPUS_VBR_ENABLED != 0));
static uint8_t m_opus_channels   = CONFIG_OPUS_CHANNELS;
static uint32_t packID = 0;

static void opus_encoder_configure(void)
{
	printk("opus_encoder_get_size() = %d\n", opus_encoder_get_size(m_opus_channels));
	__ASSERT_NO_MSG(opus_encoder_get_size(m_opus_channels) <= sizeof(m_opus_encoder));
	__ASSERT_NO_MSG(opus_encoder_init(m_opus_encoder_state, CONFIG_AUDIO_SAMPLING_FREQUENCY, m_opus_channels, OPUS_APPLICATION_RESTRICTED_LOWDELAY) == OPUS_OK);

	__ASSERT_NO_MSG(opus_encoder_ctl(m_opus_encoder_state, OPUS_SET_BITRATE(m_opus_bitrate))                      == OPUS_OK);
	__ASSERT_NO_MSG(opus_encoder_ctl(m_opus_encoder_state, OPUS_SET_VBR(m_opus_vbr))                              == OPUS_OK);
	__ASSERT_NO_MSG(opus_encoder_ctl(m_opus_encoder_state, OPUS_SET_VBR_CONSTRAINT((m_opus_bitrate != OPUS_AUTO)))== OPUS_OK);

	__ASSERT_NO_MSG(opus_encoder_ctl(m_opus_encoder_state, OPUS_SET_COMPLEXITY(m_opus_complexity))                == OPUS_OK);

	__ASSERT_NO_MSG(opus_encoder_ctl(m_opus_encoder_state, OPUS_SET_SIGNAL(OPUS_AUTO))                            == OPUS_OK);
	__ASSERT_NO_MSG(opus_encoder_ctl(m_opus_encoder_state, OPUS_SET_LSB_DEPTH(8))                                == OPUS_OK);
	__ASSERT_NO_MSG(opus_encoder_ctl(m_opus_encoder_state, OPUS_SET_DTX(0))                                       == OPUS_OK);
	__ASSERT_NO_MSG(opus_encoder_ctl(m_opus_encoder_state, OPUS_SET_INBAND_FEC(0))                                == OPUS_OK);
	__ASSERT_NO_MSG(opus_encoder_ctl(m_opus_encoder_state, OPUS_SET_PACKET_LOSS_PERC(0))                          == OPUS_OK);
}


static int ble_opus_package_enqueue(uint32_t idx, uint8_t *buf, uint32_t length)
{
	int ret = 0;
	static struct audio_payload tx_payload;
	if (length > MAX_PAYLOAD_SIZE) {
		LOG_ERR("Payload length %d exceeds maximum %d", length, MAX_PAYLOAD_SIZE);
		return -EMSGSIZE;
	}

	tx_payload.data[0] = idx & 0xFF;  // Set the first byte as the index
	tx_payload.data[1] = (idx >> 8) & 0xFF; // Set the second byte as the index high byte
	tx_payload.data[2] = (idx >> 16) & 0xFF; // Set the third byte as the index high byte
	tx_payload.data[3] = (idx >> 24) & 0xFF; // Set the fourth byte as the index high byte

	memcpy(&tx_payload.data[4], buf, length);
	tx_payload.length = length + 4;
	ret = k_msgq_put(&m_msgq_tx_payloads, &tx_payload, K_NO_WAIT);
	if (ret)  {
		LOG_INF("Audio message queue is full");
		return -ENOMEM;
	}
	return ret;
}


static void mic_data_handle(void *, void *, void *)
{
    void *buffer;
	uint32_t size;

	opus_encoder_configure();

    LOG_INF("Sound service start, wait for PCM data......");

	/** suspend the thread until we received a start event*/
	k_thread_suspend(k_current_get());

    while(1)
    {
        int frame_size;
		uint8_t frame_buf[CONFIG_AUDIO_FRAME_SIZE_BYTES] = {0};

        size = read_audio_data(&buffer, READ_TIMEOUT);
        if(size == CONFIG_AUDIO_FRAME_SIZE_SAMPLES * BYTES_PER_SAMPLE * CONFIG_OPUS_CHANNELS)
        {	
			frame_size = opus_encode(
									m_opus_encoder_state,
									buffer,
									CONFIG_AUDIO_FRAME_SIZE_SAMPLES,
									frame_buf,
									CONFIG_AUDIO_FRAME_SIZE_BYTES
									);
			if(frame_size != CONFIG_AUDIO_FRAME_SIZE_BYTES)
			{
				LOG_ERR("%d", frame_size);
				return;
			}
				
			// LOG_INF("Packet send[%d-%d], 0x%02x, 0x%02x, 0x%02x, 0x%02x  ", size,frame_size,			
			// 	 frame_buf[0],frame_buf[1], frame_buf[2],frame_buf[3]);		

			ble_opus_package_enqueue(packID, frame_buf, frame_size);		//send data to dongle through ble
			packID++;
	
            free_audio_memory(buffer);
		}
		else
		{
			LOG_ERR("Read audio data failed, size = %d", size);
			free_audio_memory(buffer);
		}
    }
}



K_THREAD_DEFINE(sound_service, SOUND_STACK_SIZE,
                mic_data_handle, NULL, NULL, NULL,
                K_PRIO_PREEMPT(3), 0, 0);


// extern void turn_on_off_led(uint8_t idx, bool onOff);
static bool mic_work_event_handler(const struct app_event_header *aeh)
{
	if (is_mic_work_event(aeh)) 
	{
		struct mic_work_event *event = cast_mic_work_event(aeh);
		if (event->type == MIC_STATUS_START) 
		{
            LOG_INF("Micphone start to work!");
			packID = 0;
			drv_mic_start();

			k_thread_resume(sound_service);
			
			dk_set_led_on(MIC_STATUS_LED);
		}
		else if(event->type == MIC_STATUS_STOP)
		{
            LOG_INF("Micphone stop to work!");
			drv_mic_stop();

			k_thread_suspend(sound_service);

			dk_set_led_off(MIC_STATUS_LED);
		}

		return true;
	}
	else
	{
		return false;
	}
}


APP_EVENT_LISTENER(mic_work, mic_work_event_handler);
APP_EVENT_SUBSCRIBE(mic_work, mic_work_event);