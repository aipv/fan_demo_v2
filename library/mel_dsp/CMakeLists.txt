cmake_minimum_required(VERSION 3.10)
project(mel_dsp C)

set(CMAKE_C_STANDARD 99)

# ESP-IDF DSP library path
set(ESP32_DSP_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../student/managed_components/espressif__esp-dsp/modules/)

# ================================ mel_dsp library ================================
add_library(mel_dsp STATIC
    mel_dsp.c
    mel_dsp_mel.c
    mel_dsp_hann.c
)

target_include_directories(mel_dsp PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

# When building for ESP-IDF, add FFT from ESP-DSP library
if(ESP_PLATFORM)
    target_sources(mel_dsp PRIVATE
        ${ESP32_DSP_ROOT}/fft/float/dsps_fft2r_fc32_ansi.c
        ${ESP32_DSP_ROOT}/fft/float/dsps_fft2r_bitrev_tables_fc32.c
    )
    target_include_directories(mel_dsp PRIVATE
        ${ESP32_DSP_ROOT}/fft/include
        ${ESP32_DSP_ROOT}/common/include
    )
else()
    # On non-ESP platforms, use standalone FFT
    target_sources(mel_dsp PRIVATE mel_dsp_fft.c)
endif()

# ================================ test ================================
add_executable(test_mel_dsp tests/test_mel_dsp.c)
target_link_libraries(test_mel_dsp mel_dsp -lm)
