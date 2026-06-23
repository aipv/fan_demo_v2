cmake_minimum_required(VERSION 3.10)
project(sbc C)

set(CMAKE_C_STANDARD 99)

set(ESP32_DSP_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../student/managed_components/espressif__esp-dsp/modules/)

# ================================ library ================================
add_library(esp_dsp_pipe STATIC 
    ${ESP32_DSP_ROOT}/fft/float/dsps_fft2r_fc32_ansi.c
    ${ESP32_DSP_ROOT}/fft/float/dsps_fft2r_bitrev_tables_fc32.c
    ${CMAKE_CURRENT_LIST_DIR}/esp32/dsps_pwroftwo.c
    mfcc/dsp_mfcc_feature.c
    mfcc/dsp_mfcc_mel.c
    pipe/dsp_pipeline.c
)

target_include_directories(esp_dsp_pipe PUBLIC 
    ${ESP32_DSP_ROOT}/fft/include
    ${ESP32_DSP_ROOT}/common/include
    ${ESP32_DSP_ROOT}/common/include_sim
    ${CMAKE_CURRENT_LIST_DIR}/mfcc
    ${CMAKE_CURRENT_LIST_DIR}/pipe
    /home/qing/esp/esp-idf/components/esp_common/include/
)

# ================================ test ================================
add_executable(test_dsp_data tests/test_dsp_data.c)
target_link_libraries(test_dsp_data esp_dsp_pipe -lm)

add_executable(test_mfcc tests/test_dsp_mfcc_feature.c)
target_link_libraries(test_mfcc esp_dsp_pipe -lm)

add_executable(test_pipe tests/test_dsp_pipeline.c)
target_link_libraries(test_pipe esp_dsp_pipe -lm)