#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "mel_dsp.h"

#define MAX_SAMPLES 384000  /* 24 seconds at 16kHz */

int main(int argc, char *argv[])
{
    const char *input_file = "../dataset/sample/normal/normal_0000.pcm";
    const char *output_file = "mel_output.bin";

    if (argc > 1) {
        input_file = argv[1];
    }
    if (argc > 2) {
        output_file = argv[2];
    }

    printf("Mel DSP Test\n");
    printf("============\n");
    printf("Input: %s\n", input_file);
    printf("Output: %s\n", output_file);

    /* Initialize DSP */
    mel_dsp_init();

    /* Read PCM file */
    FILE *fp = fopen(input_file, "rb");
    if (!fp) {
        printf("Error: cannot open %s\n", input_file);
        return -1;
    }

    int16_t pcm_data[MAX_SAMPLES];
    size_t read_count = fread(pcm_data, sizeof(int16_t), MAX_SAMPLES, fp);
    fclose(fp);

    printf("Read %zu samples\n", read_count);

    /* Process with DSP */
    float *output = malloc(read_count * MEL_DSP_MEL_SIZE * sizeof(float) / MEL_DSP_HOP_SIZE);
    if (!output) {
        printf("Error: memory allocation failed\n");
        return -1;
    }

    int n_frames = mel_dsp_process(pcm_data, (uint32_t)read_count, output);
    printf("Processed %d frames\n", n_frames);

    /* Write output */
    FILE *ofp = fopen(output_file, "wb");
    if (!ofp) {
        printf("Error: cannot create %s\n", output_file);
        free(output);
        return -1;
    }

    fwrite(output, sizeof(float), n_frames * MEL_DSP_MEL_SIZE, ofp);
    fclose(ofp);

    printf("Output written to %s\n", output_file);
    printf("Output shape: %d frames x %d mel features\n", n_frames, MEL_DSP_MEL_SIZE);

    /* Print first frame for verification */
    printf("\nFirst frame (first 10 mel features):\n");
    for (int i = 0; i < 10; i++) {
        printf("  mel[%d] = %f\n", i, output[i]);
    }

    free(output);
    return 0;
}
