#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

/* Simple test to verify FFT implementation */
#define N 512

int mel_dsp_is_power_of_two(int x)
{
    return (x != 0) && ((x & (x - 1)) == 0);
}

void mel_dsp_bit_reverse(float *data, int N)
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

int main()
{
    float data[N * 2];
    float mag[N / 2 + 1];

    /* Initialize with a simple sine wave: 10 cycles in 512 samples */
    for (int i = 0; i < N; i++) {
        data[2 * i] = sinf(2.0f * M_PI * 10.0f * i / N);  /* real part */
        data[2 * i + 1] = 0.0f;  /* imaginary part */
    }

    printf("Input: sine wave with 10 cycles in 512 samples\n");
    printf("First 10 samples (real part):\n");
    for (int i = 0; i < 10; i++) {
        printf("  [%d] = %f\n", i, data[2 * i]);
    }

    /* Apply bit reversal */
    mel_dsp_bit_reverse(data, N);

    printf("\nAfter bit reversal (first 10 samples):\n");
    for (int i = 0; i < 10; i++) {
        printf("  [%d] = %f\n", i, data[2 * i]);
    }

    /* Simple DFT for verification */
    printf("\nSimple DFT magnitude at bin 10 (should be peak):\n");
    float real_sum = 0, imag_sum = 0;
    for (int i = 0; i < N; i++) {
        float angle = -2.0f * M_PI * 10.0f * i / N;
        real_sum += data[2 * i] * cosf(angle) - data[2 * i + 1] * sinf(angle);
        imag_sum += data[2 * i] * sinf(angle) + data[2 * i + 1] * cosf(angle);
    }
    float dft_mag = sqrtf(real_sum * real_sum + imag_sum * imag_sum) / N;
    printf("  DFT magnitude at bin 10 = %f\n", dft_mag);

    return 0;
}
