cmake_minimum_required(VERSION 3.10)

project(tflite_test C)

add_executable(
    test_inference
    tests/test_inference.c
)

target_include_directories(
    test_inference
    PRIVATE
    .
)

target_link_directories(
    test_inference
    PRIVATE
    ${CMAKE_SOURCE_DIR}/lib
)

target_link_libraries(
    test_inference
    PRIVATE
    tensorflowlite_c
)