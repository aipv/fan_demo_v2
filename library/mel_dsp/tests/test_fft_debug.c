#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "mel_dsp.h"

int main()
{
    // Initialize DSP
    if (mel_dsp_init() != 0) {
        printf("mel_dsp_init failed\n");
        return -1;
    }

    // Read PCM file
    FILE *fp = fopen("/home/test/workspace/fan_demo_v2/dataset/sample/normal/normal_0000.pcm", "rb");
    if (!fp) {
        printf("Error: cannot open PCM file\n");
        return -1;
    }

    int16_t pcm[400];
    size_t read = fread(pcm, sizeof(int16_t), 400, fp);
    fclose(fp);

    if (read != 400) {
        printf("Error: expected 400 samples, got %zu\n", read);
        return -1;
    }

    // Process one frame
    float output[40];
    mel_dsp_frame_process(pcm, output);

    // Print mel[0] and mel[7]
    printf("mel[0] = %.6f\n", output[0]);
    printf("mel[7] = %.6f\n", output[7]);

    // Expected from Python:
    // mel[0] = 15.666929
    // mel[7] = 12.043590

    printf("\nExpected from Python:\n");
    printf("mel[0] = 15.666929\n");
    printf("mel[7] = 12.043590\n");

    return 0;
}
