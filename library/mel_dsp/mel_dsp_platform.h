#ifndef MEL_DSP_PLATFORM_H
#define MEL_DSP_PLATFORM_H

#ifdef ESP_PLATFORM

/*
 * Real ESP-IDF Environment
 */

#include "esp_err.h"
#include "esp_log.h"
#include "dsps_fft2r.h"
#include "dsp_common.h"

#else

/*
 * Host / PC Simulation Environment
 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

typedef int esp_err_t;

#define ESP_OK          0
#define ESP_FAIL       -1

#define ESP_LOGI(tag, fmt, ...) \
    printf("[I] %s: " fmt "\n", tag, ##__VA_ARGS__)

#define ESP_LOGW(tag, fmt, ...) \
    printf("[W] %s: " fmt "\n", tag, ##__VA_ARGS__)

#define ESP_LOGE(tag, fmt, ...) \
    printf("[E] %s: " fmt "\n", tag, ##__VA_ARGS__)

#define ESP_ERROR_CHECK(x)                     \
    do {                                       \
        esp_err_t err_rc_ = (x);               \
        if (err_rc_ != ESP_OK) {               \
            printf("ESP_ERROR_CHECK failed: %d\n", err_rc_); \
        }                                      \
    } while(0)

/*
 * Standalone FFT implementation for Ubuntu/PC testing
 * Uses ESP-DSP FFT via dsps_fft2r.h macro
 */

/*
 * NOTE: dsps_fft2r_fc32 and dsps_bit_rev_fc32 are defined as macros
 * in dsps_fft2r.h which expands to the appropriate implementation.
 * Include dsps_fft2r.h below to get the macro definitions.
 */
#include "dsps_fft2r.h"

esp_err_t dsps_fft2r_init_fc32(float *fft_table_buff, int table_size);

#endif

#endif /* MEL_DSP_PLATFORM_H */
