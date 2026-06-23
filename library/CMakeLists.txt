cmake_minimum_required(VERSION 3.10)
project(sbc C)

set(CMAKE_C_STANDARD 99)

set(ESP32_DSP_ROOT ${CMAKE_CURRENT_LIST_DIR}/../student/managed_components/espressif__esp-dsp/modules/)

# ================================ library ================================
add_library(esp_dsp_pipe STATIC 
    ${ESP32_DSP_ROOT}/fft/float/dsps_fft2r_fc32_ansi.c
    ${ESP32_DSP_ROOT}/fft/float/dsps_fft2r_bitrev_tables_fc32.c
    ${CMAKE_CURRENT_LIST_DIR}/dsp/esp32/dsps_pwroftwo.c
    dsp/mfcc/dsp_mfcc_feature.c
    dsp/mfcc/dsp_mfcc_mel.c
    dsp/pipe/dsp_pipeline.c
)

target_include_directories(esp_dsp_pipe PUBLIC 
    ${ESP32_DSP_ROOT}/fft/include
    ${ESP32_DSP_ROOT}/common/include
    ${ESP32_DSP_ROOT}/common/include_sim
    ${CMAKE_CURRENT_LIST_DIR}/dsp/mfcc
    ${CMAKE_CURRENT_LIST_DIR}/dsp/pipe
    /home/qing/esp/esp-idf/components/esp_common/include/
)

# ================================ test ================================
add_executable(test_file_process tests/test_file_process.c)
target_include_directories(test_file_process PRIVATE ${CMAKE_CURRENT_LIST_DIR}/tflite)
target_link_directories(test_file_process PRIVATE ${CMAKE_CURRENT_LIST_DIR}/tflite/lib)
target_link_libraries(test_file_process esp_dsp_pipe tensorflowlite_c -lm)

add_executable(test_esp32_predict tests/test_esp32_predict.c)
target_include_directories(test_esp32_predict PRIVATE ${CMAKE_CURRENT_LIST_DIR}/tflite)
target_link_directories(test_esp32_predict PRIVATE ${CMAKE_CURRENT_LIST_DIR}/tflite/lib)
target_link_libraries(test_esp32_predict esp_dsp_pipe tensorflowlite_c -lm)

add_executable(test_esp32_stream tests/test_esp32_stream.c)
target_include_directories(test_esp32_stream PRIVATE ${CMAKE_CURRENT_LIST_DIR}/tflite)
target_link_directories(test_esp32_stream PRIVATE ${CMAKE_CURRENT_LIST_DIR}/tflite/lib)
target_link_libraries(test_esp32_stream esp_dsp_pipe tensorflowlite_c -lm)
