#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "mel_dsp.h"

#define SAMPLE_RATE 16000
#define FRAMES_PER_SECOND 49  /* 1 + (16000 - 400) / 320 = 49 */
#define MEL_SIZE 40

int main(int argc, char *argv[])
{
    const char *input_path = "/home/test/workspace/fan_demo_v2/dataset/sample/normal/normal_0000.pcm";
    const char *output_path = "./mel_output.bin";
    const char *python_path = "/tmp/python_dsp32.bin";

    if (argc > 1) input_path = argv[1];
    if (argc > 2) output_path = argv[2];
    if (argc > 3) python_path = argv[3];

    printf("=== DSP32 Comparison Test ===\n\n");
    printf("Input: %s\n", input_path);
    printf("C Output: %s\n", output_path);
    printf("Python Reference: %s\n\n", python_path);

    /* Initialize DSP */
    if (mel_dsp_init() != 0) {
        printf("mel_dsp_init failed\n");
        return -1;
    }

    /* Read PCM file (1 second = 16000 samples) */
    FILE *fp = fopen(input_path, "rb");
    if (!fp) {
        printf("Error: cannot open %s\n", input_path);
        return -1;
    }

    int16_t *pcm = (int16_t *)malloc(SAMPLE_RATE * sizeof(int16_t));
    if (!pcm) {
        printf("Memory allocation failed\n");
        fclose(fp);
        return -1;
    }

    size_t read_count = fread(pcm, sizeof(int16_t), SAMPLE_RATE, fp);
    fclose(fp);
    printf("Read %zu samples\n", read_count);

    if (read_count != SAMPLE_RATE) {
        printf("Warning: expected %d samples, got %zu\n", SAMPLE_RATE, read_count);
    }

    /* Process with mel_dsp */
    float *mel_output = (float *)malloc(FRAMES_PER_SECOND * MEL_SIZE * sizeof(float));
    if (!mel_output) {
        printf("Memory allocation failed\n");
        free(pcm);
        return -1;
    }

    int n_frames = mel_dsp_process(pcm, SAMPLE_RATE, mel_output);
    printf("Processed %d frames\n", n_frames);

    /* Write C output */
    fp = fopen(output_path, "wb");
    if (!fp) {
        printf("Error: cannot create %s\n", output_path);
        free(pcm);
        free(mel_output);
        return -1;
    }
    fwrite(mel_output, sizeof(float), n_frames * MEL_SIZE, fp);
    fclose(fp);
    printf("Wrote C output to %s\n", output_path);

    /* Read Python reference */
    fp = fopen(python_path, "rb");
    if (!fp) {
        printf("Error: cannot open Python reference %s\n", python_path);
        free(pcm);
        free(mel_output);
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    long py_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    int py_frames = py_size / sizeof(float) / MEL_SIZE;
    printf("Python reference: %d frames\n", py_frames);

    float *py_output = (float *)malloc(py_frames * MEL_SIZE * sizeof(float));
    fread(py_output, sizeof(float), py_frames * MEL_SIZE, fp);
    fclose(fp);

    /* Compare */
    int compare_frames = (n_frames < py_frames) ? n_frames : py_frames;

    printf("\n=== DSP32 Comparison (First 10 features of frame 0) ===\n");
    printf("%-10s %-20s %-20s %-15s\n", "Feature", "C Value", "Python Value", "Diff");
    printf("---------------------------------------------------------------\n");
    for (int i = 0; i < 10; i++) {
        float c_val = mel_output[i];
        float py_val = py_output[i];
        float diff = fabsf(c_val - py_val);
        printf("mel[%2d]     %-20.6f %-20.6f %-15.6f\n", i, c_val, py_val, diff);
    }

    /* Overall statistics */
    float max_diff = 0.0f;
    float mean_diff = 0.0f;
    float max_rel_diff = 0.0f;
    int diff_count = 0;

    for (int f = 0; f < compare_frames; f++) {
        for (int i = 0; i < MEL_SIZE; i++) {
            float c_val = mel_output[f * MEL_SIZE + i];
            float py_val = py_output[f * MEL_SIZE + i];
            float diff = fabsf(c_val - py_val);
            float rel_diff = diff / (fabsf(py_val) + 1e-10f);

            if (diff > max_diff) max_diff = diff;
            if (rel_diff > max_rel_diff) max_rel_diff = rel_diff;
            mean_diff += diff;
            diff_count++;
        }
    }
    mean_diff /= diff_count;

    printf("\n=== Overall Statistics ===\n");
    printf("Frames compared: %d\n", compare_frames);
    printf("Max absolute diff: %.6f\n", max_diff);
    printf("Mean absolute diff: %.6f\n", mean_diff);
    printf("Max relative diff: %.2f%%\n", max_rel_diff * 100.0f);

    /* Per-frame mean comparison */
    printf("\n=== Per-Frame Mean Comparison (first 5 frames) ===\n");
    printf("%-10s %-15s %-15s %-15s\n", "Frame", "C Mean", "Python Mean", "Diff");
    printf("------------------------------------------------\n");
    for (int f = 0; f < 5 && f < compare_frames; f++) {
        float c_mean = 0.0f, py_mean = 0.0f;
        for (int i = 0; i < MEL_SIZE; i++) {
            c_mean += mel_output[f * MEL_SIZE + i];
            py_mean += py_output[f * MEL_SIZE + i];
        }
        c_mean /= MEL_SIZE;
        py_mean /= MEL_SIZE;
        printf("%-10d %-15.6f %-15.6f %-15.6f\n", f, c_mean, py_mean, fabsf(c_mean - py_mean));
    }

    free(pcm);
    free(mel_output);
    free(py_output);

    return 0;
}
