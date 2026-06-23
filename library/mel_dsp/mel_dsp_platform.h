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
 */

esp_err_t dsps_fft2r_init_fc32(float *fft_table_buff, int table_size);
void dsps_fft2r_fc32(float *data, int N);
void dsps_bit_rev_fc32(float *data, int N);

#endif

#endif /* MEL_DSP_PLATFORM_H */
