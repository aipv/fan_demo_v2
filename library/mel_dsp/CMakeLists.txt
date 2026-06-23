cmake_minimum_required(VERSION 3.10)
project(mel_dsp C)

set(CMAKE_C_STANDARD 99)

# ESP-IDF and ESP-DSP paths
# From library/mel_dsp/ -> ../../student/managed_components/
set(ESP_IDF_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../student/managed_components/espressif__esp-dsp/modules/)

# ================================ mel_dsp library ================================
add_library(mel_dsp STATIC
    mel_dsp.c
    mel_dsp_mel.c
    mel_dsp_hann.c
    dsp_power.c
)

target_include_directories(mel_dsp PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

# When building for ESP-IDF, add FFT from ESP-DSP library
if(ESP_PLATFORM)
    target_sources(mel_dsp PRIVATE
        ${ESP_IDF_ROOT}/fft/float/dsps_fft2r_fc32_ansi.c
        ${ESP_IDF_ROOT}/fft/float/dsps_fft2r_bitrev_tables_fc32.c
    )
    target_include_directories(mel_dsp PRIVATE
        ${ESP_IDF_ROOT}/fft/include
        ${ESP_IDF_ROOT}/common/include
    )
else()
    # On non-ESP platforms, use ESP-DSP sources + local stubs
    # Stub headers take priority over ESP-DSP headers
    target_sources(mel_dsp PRIVATE
        ${ESP_IDF_ROOT}/fft/float/dsps_fft2r_fc32_ansi.c
        ${ESP_IDF_ROOT}/fft/float/dsps_fft2r_bitrev_tables_fc32.c
    )
    target_include_directories(mel_dsp PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}  # Stubs (esp_idf_version.h, dsp_err.h, esp_err.h)
        ${ESP_IDF_ROOT}/fft/include
        ${ESP_IDF_ROOT}/common/include
        ${ESP_IDF_ROOT}/common/include_sim
    )
endif()

# ================================ tests ================================
add_executable(test_mel_dsp tests/test_mel_dsp.c)
target_include_directories(test_mel_dsp PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${ESP_IDF_ROOT}/fft/include
    ${ESP_IDF_ROOT}/common/include
    ${ESP_IDF_ROOT}/common/include_sim
)
target_link_libraries(test_mel_dsp mel_dsp -lm)

add_executable(test_mel_dsp_predict tests/test_mel_dsp_predict.c)
target_include_directories(test_mel_dsp_predict PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${ESP_IDF_ROOT}/fft/include
    ${ESP_IDF_ROOT}/common/include
    ${ESP_IDF_ROOT}/common/include_sim
)
target_link_libraries(test_mel_dsp_predict mel_dsp -lm)

add_executable(test_mel_dsp_dsp32 tests/test_mel_dsp_dsp32.c)
target_include_directories(test_mel_dsp_dsp32 PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${ESP_IDF_ROOT}/fft/include
    ${ESP_IDF_ROOT}/common/include
    ${ESP_IDF_ROOT}/common/include_sim
)
target_link_libraries(test_mel_dsp_dsp32 mel_dsp -lm)

add_executable(test_mel_dsp_all tests/test_mel_dsp_all.c)
target_include_directories(test_mel_dsp_all PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${ESP_IDF_ROOT}/fft/include
    ${ESP_IDF_ROOT}/common/include
    ${ESP_IDF_ROOT}/common/include_sim
)
target_link_libraries(test_mel_dsp_all mel_dsp -lm)
