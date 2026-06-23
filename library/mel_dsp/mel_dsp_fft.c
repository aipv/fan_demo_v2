/*
 * Standalone FFT implementation for Ubuntu/PC testing
 * This is a clean-room implementation of radix-2 decimation-in-frequency FFT
 * that matches the behavior of ESP-DSP's dsps_fft2r_fc32_ansi.c
 */

#include "mel_dsp_platform.h"

#ifndef ESP_PLATFORM

static float *fft_w_table = NULL;
static int fft_w_table_size = 0;
static int fft_initialized = 0;

static int is_power_of_two(int x)
{
    return (x != 0) && ((x & (x - 1)) == 0);
}

static int power_of_two(int x)
{
    for (int i = 0; i < 32; i++) {
        if ((1 << i) == x) return i;
    }
    return 0;
}

static void bit_reverse(float *data, int N)
{
    int j = 0;
    for (int i = 0; i < N - 1; i++) {
        if (i < j) {
            float temp_r = data[2 * i];
            float temp_i = data[2 * i + 1];
            data[2 * i] = data[2 * j];
            data[2 * i + 1] = data[2 * j + 1];
            data[2 * j] = temp_r;
            data[2 * j + 1] = temp_i;
        }
        int k = N / 2;
        while (k <= j) {
            j -= k;
            k /= 2;
        }
        j += k;
    }
}

static void gen_twiddle(float *w, int N)
{
    float e = 2.0f * (float)M_PI / N;
    for (int i = 0; i < N / 2; i++) {
        w[2 * i] = cosf(i * e);
        w[2 * i + 1] = sinf(i * e);
    }
}

esp_err_t dsps_fft2r_init_fc32(float *fft_table_buff, int table_size)
{
    if (fft_initialized && fft_w_table_size == table_size) {
        return ESP_OK;
    }

    if (!is_power_of_two(table_size)) {
        return ESP_FAIL;
    }

    if (fft_table_buff != NULL) {
        fft_w_table = fft_table_buff;
    } else {
        if (fft_w_table != NULL && fft_w_table_size != table_size) {
            free(fft_w_table);
        }
        if (fft_w_table == NULL) {
            fft_w_table = (float *)malloc(table_size * sizeof(float));
        }
        if (fft_w_table == NULL) {
            return ESP_FAIL;
        }
        gen_twiddle(fft_w_table, table_size);
    }

    fft_w_table_size = table_size;
    fft_initialized = 1;
    return ESP_OK;
}

void dsps_fft2r_fc32(float *data, int N)
{
    if (!fft_initialized || fft_w_table_size != N) {
        dsps_fft2r_init_fc32(NULL, N);
    }

    bit_reverse(data, N);

    int n2 = N / 2;
    for (int len = n2; len >= 1; len /= 2) {
        int half = len;
        int stride = 2 * half;
        for (int i = 0; i < N; i += stride) {
            for (int j = 0; j < half; j++) {
                int idx1 = i + j;
                int idx2 = idx1 + half;
                int tw_idx = j;

                float tr = fft_w_table[2 * tw_idx] * data[2 * idx2] - fft_w_table[2 * tw_idx + 1] * data[2 * idx2 + 1];
                float ti = fft_w_table[2 * tw_idx] * data[2 * idx2 + 1] + fft_w_table[2 * tw_idx + 1] * data[2 * idx2];

                float xr = data[2 * idx1];
                float xi = data[2 * idx1 + 1];

                data[2 * idx1] = xr + tr;
                data[2 * idx1 + 1] = xi + ti;
                data[2 * idx2] = xr - tr;
                data[2 * idx2 + 1] = xi - ti;
            }
        }
        n2 /= 2;
    }
}

void dsps_bit_rev_fc32(float *data, int N)
{
    // For the ANSI implementation, bit reversal is done inside dsps_fft2r_fc32
    // This function is kept for API compatibility
    (void)data;
    (void)N;
}

#endif /* !ESP_PLATFORM */
