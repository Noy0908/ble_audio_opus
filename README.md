# BLE Audio OPUS Demo

A Bluetooth Low Energy audio streaming sample based on **NCS 3.1.0**, featuring OPUS audio codec integration. This demo supports multiple BLE connections and transmits real-time audio streams through BLE to a dongle device.

## Overview

This project consists of two main components:

- **ble_microphone**: Runs on nRF54L15 DK, captures audio via PDM microphone, encodes with OPUS, and transmits over BLE
- **dongle**: Runs on nRF52840 DK (USB audio output) or nRF54L15 DK (IIS speaker output), receives and decodes audio streams

## Hardware Requirements

- **nRF54L15 DK** - Microphone device (audio source)
- **nRF52840 DK** - Dongle device (USB audio output recommended)
- **USB cables** - For power and audio output
- **nRF Connect SDK 3.1.0+** - Development environment

## Build Instructions

### 1. Build Microphone Firmware

```bash
cd ble_microphone
west build -b nrf54l15dk_nrf54l15_cpuapp
```

### 2. Build Dongle Firmware

```bash
cd ../dongle

# For USB audio output (nRF52840, recommended)
west build -b nrf52840dk_nrf52840

# Or for IIS speaker output (nRF54L15, doesn't support yet)
# west build -b nrf54l15dk_nrf54l15_cpuapp
```

### 5. Flash to Devices

```bash
# Flash microphone
cd ../ble_microphone/build
west flash

# Flash dongle (from another terminal)
cd ../../dongle/build
west flash
```

## Usage

### Device Connection

1. **Power on the microphone device** (nRF54L15 DK)
   - Device will advertise as `ble_microphone_1`
   - LED0 will blink (running state)

2. **Power on the dongle device** (nRF52840 DK)
   - Device automatically scans and connects to the microphone
   - Once connected, LED0 on both devices turns solid

### Audio Capture

1. **Press Button 0 on the microphone device**
   - First press: Start audio capture
   - Second press: Stop audio capture
   - LED0 indicates connection status

2. Audio is encoded with OPUS and transmitted over BLE (247-byte packets)

### Testing and Audio Recording

#### Using Audacity (Recommended)

1. **Download and install Audacity**
   - https://www.audacityteam.org/

2. **Configure audio device**
   - Open Audacity → Audio Setup → Audio Settings
   - Recording Device: Select **USB Audio Device**
   - Sample Rate: 16 kHz
   - Default Sample Rate: 16000 HZ
   - Default Sample Format: 16-bit
   - Channels: Stereo

3. **Record audio**
   - Click red **Record** button in Audacity
   - Press Button 1 on nRF54L15DK to start audio capture
   - Press Button 1 again to stop capture
   - Click **Stop** button in Audacity

4. **Verify and save**
   - Check waveform in Audacity
   - Listen to the recording to verify audio quality

#### Using Command Line (Linux/Mac)

```bash
# Record audio from USB device for 10 seconds
```

#### Using Windows Tools

- **Windows 10/11**: Settings → Sound → Volume mixer
  - Select USB Audio as input device
  - Use built-in Sound Recorder or other recording software

## Features

- ✅ OPUS audio codec integration (16-128 kbps bitrate)
- ✅ Multiple BLE connections support
- ✅ 2M PHY and MTU optimization (247-byte packets)
- ✅ USB audio output via dongle (nRF52840DK)
- ✅ Real-time audio streaming with < 100ms latency
- ✅ Button-controlled audio capture

## Connection Parameters

- **Connection Interval**: 7.5 ms
- **PHY**: 2M (automatically updated)
- **MTU**: 247 bytes (max)
- **Max Connections**: 1 (configurable)

## Debugging

Check logs via UART console:


Key log messages to look for:

- `"Advertising successfully started"` - Microphone ready
- `"Connected"` - BLE connection established
- `"PHY updated. New PHY: 2M"` - PHY negotiation complete

## Troubleshooting

| Issue | Solution |
|-------|----------|
| Devices won't connect | Reboot both devices, check firmware is flashed correctly |
| No audio output | Verify USB device is recognized in system audio settings |
| Poor audio quality | Ensure 2M PHY is enabled, check connection interval in logs |
| Frequent disconnections | Increase timeout: `CONFIG_BT_PERIPHERAL_PREF_TIMEOUT=4000` |

## Project Structure

```
ble_audio_opus/
├── ble_microphone/
│   ├── src/
│   │   ├── main.c              # Application entry
│   │   ├── ble_app.c           # BLE connection logic
│   │   ├── sound_service.c     # Audio service
│   │   ├── drivers/drv_mic.c   # PDM microphone driver
│   │   └── events/             # Event system
│   ├── prj.conf                # Zephyr configuration
│   └── CMakeLists.txt
│
├── dongle/
│   ├── src/
│   │   ├── main.c              # Application entry
│   │   ├── ble_app.c           # BLE scanning and connection
│   │   ├── usb_audio_handle.c  # USB audio (nRF52840)
│   │   └── audio_handle.c      # IIS audio (nRF54L15)
│   ├── prj.conf
│   └── CMakeLists.txt
│
└── lib/opus/                   # OPUS library files
```

## License

Licensed under Nordic 5-Clause License (LicenseRef-Nordic-5-Clause)

## References

- [nRF Connect SDK Documentation](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/)
- [Zephyr RTOS](https://docs.zephyrproject.org/)
- [OPUS Codec](https://www.opus-codec.org/)
