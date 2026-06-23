set(COMMON_LIBRARY_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../../library/)

idf_component_register(
    SRCS
        "${COMMON_LIBRARY_ROOT}/dsp/mfcc/dsp_mfcc_mel.c"
        "${COMMON_LIBRARY_ROOT}/dsp/mfcc/dsp_mfcc_feature.c"
        "${COMMON_LIBRARY_ROOT}/dsp/pipe/dsp_pipeline.c"
        "execute_plane.c"
        "student_learning.c"
        "test_mfcc_feature.c"
        "model_data.cc"
        "tflite_interpreter.cpp"
    INCLUDE_DIRS
        "${COMMON_LIBRARY_ROOT}/dsp/mfcc/"
        "${COMMON_LIBRARY_ROOT}/dsp/pipe/"
        "${COMMON_LIBRARY_ROOT}/sli/"
         "."
    PRIV_REQUIRES
        esp_timer
        support_plane
        observe_plane
)
