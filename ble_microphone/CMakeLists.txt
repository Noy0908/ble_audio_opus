#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ble_opus)

set(OPUS_LIBRARY_ROOT ${CMAKE_CURRENT_LIST_DIR}/../lib CACHE PATH "Opus library root directory")
set(ENV{OPUS_LIBRARY_ROOT} $OPUS_LIBRARY_ROOT)

zephyr_link_libraries(-mfloat-abi=hard)
zephyr_link_libraries(-mfpu=fpv4-sp-d16 -ffunction-sections -fdata-sections -fno-strict-aliasing -mthumb -mabi=aapcs)

# NORDIC SDK APP START
target_sources(app PRIVATE
  src/main.c
  src/sound_service.c
  src/drivers/drv_mic.c
  src/events/mic_work_event.c
)

# NORDIC SDK APP END
zephyr_library_include_directories(src/drivers/)
zephyr_library_include_directories(src/events/)
# zephyr_library_include_directories(../lib/adpcm_lib)
zephyr_include_directories(../lib/opus/opus_include/opus-1.2.1)
zephyr_include_directories(../lib/opus/opus_configuration)
target_link_libraries(app PRIVATE ${OPUS_LIBRARY_ROOT}/opus/opus_library/opus_lib.a)

# NORDIC SDK APP END
