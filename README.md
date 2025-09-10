# ble_audio_opus
This sample is based on NCS3.1.0, it integrates OPUS encode/decode, can support multiple connection,  audio stream will be transmitted through BLE to dongle.

As nRF54l15 doesn't support usb ,so in this sample, it will be the microphone to sample audio stream then transmit it to dongle(nRF52840DK).
Dongle will send the received audio stream to usb audio class driver, then you can record the audio with "Audacity" app.

Of course you can choose nRF54L15 as dongle then paly it with IIS speaker, but this sample didn't implement IIS output feature, so you can only print log to check if packet lost.
