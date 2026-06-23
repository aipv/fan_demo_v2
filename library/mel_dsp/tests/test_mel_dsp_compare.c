#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include "mel_dsp.h"
#include "mel_dsp_hann.h"

#define N_FFT 512
#define FFT_BINS 257

int main(int argc, char *argv[])
{
    float fft_data[N_FFT * 2];
    float mag[FFT_BINS];
    float power[FFT_BINS];

    /* Read PCM file */
    const char *input_file = "/home/test/workspace/fan_demo_v2/dataset/sample/normal/normal_0000.pcm";
    FILE *fp = fopen(input_file, "rb");
    if (!fp) {
        printf("Error: cannot open %s\n", input_file);
        return 1;
    }

    int16_t pcm[400];
    fread(pcm, sizeof(int16_t), 400, fp);
    fclose(fp);

    /* Apply window and zero-pad */
    memset(fft_data, 0, sizeof(fft_data));
    for (int i = 0; i < 400; i++) {
        fft_data[2 * i] = (float)pcm[i] * mel_dsp_hanning_window[i];
    }

    printf("FFT input (first 10 samples):\n");
    for (int i = 0; i < 10; i++) {
        printf("  [%d] = %f\n", i, fft_data[2 * i]);
    }

    /* Run FFT */
    dsps_fft2r_init_fc32(NULL, N_FFT);
    dsps_fft2r_fc32(fft_data, N_FFT);
    dsps_bit_rev_fc32(fft_data, N_FFT);

    printf("\nFFT output (first 10 complex values):\n");
    for (int i = 0; i < 10; i++) {
        printf("  [%d] = %f + %fj\n", i, fft_data[2 * i], fft_data[2 * i + 1]);
    }

    /* Compute power spectrum */
    for (int k = 0; k < FFT_BINS; k++) {
        float real = fft_data[2 * k];
        float imag = fft_data[2 * k + 1];
        mag[k] = sqrtf(real * real + imag * imag);
        power[k] = (real * real + imag * imag) / N_FFT;
    }

    printf("\nC Power spectrum (first 20 bins):\n");
    printf("%-10s %-20s\n", "Bin", "Power");
    printf("------------------------------\n");
    for (int i = 0; i < 20; i++) {
        printf("[%2d]    %.6f\n", i, power[i]);
    }

    /* Read Python power spectrum */
    float py_power[FFT_BINS];
    fp = fopen("/tmp/power_spec.bin", "rb");
    if (!fp) {
        printf("Error: cannot open /tmp/power_spec.bin\n");
        return 1;
    }
    fread(py_power, sizeof(float), FFT_BINS, fp);
    fclose(fp);

    printf("\nComparison (first 20 bins):\n");
    printf("%-10s %-20s %-20s %-15s\n", "Bin", "C Power", "Python Power", "Rel Diff %");
    printf("---------------------------------------------------------------\n");
    for (int i = 0; i < 20; i++) {
        float rel_diff = fabsf(power[i] - py_power[i]) / (py_power[i] + 1e-10) * 100.0f;
        printf("[%2d]    %.6f       %.6f       %.2f%%\n", i, power[i], py_power[i], rel_diff);
    }

    return 0;
}
