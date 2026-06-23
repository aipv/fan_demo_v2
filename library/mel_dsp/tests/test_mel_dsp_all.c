#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <dirent.h>
#include "mel_dsp.h"

#define SAMPLE_RATE 16000
#define MEL_SIZE 40

typedef struct {
    const char *name;
    int frame_count;
} category_info_t;

static int process_file(const char *input_path, const char *ref_path, int category_idx);
static int compare_outputs(const float *c_output, const float *py_output, int n_frames, int mel_size);
static int count_files_in_dir(const char *dir_path);

int main(int argc, char *argv[])
{
    const char *base_path = "/home/test/workspace/fan_demo_v2/dataset";
    const char *sample_dir = "/sample";
    const char *dsp32_dir = "/dsp32";

    const char *categories[] = {"normal", "abnormal", "d1", "d2", "d3", "d4"};
    int num_categories = 6;

    printf("=== Mel DSP Full Test Suite ===\n\n");

    /* Initialize DSP */
    if (mel_dsp_init() != 0) {
        printf("ERROR: mel_dsp_init failed\n");
        return -1;
    }

    int total_files = 0;
    int passed_files = 0;
    float total_max_diff = 0.0f;
    float total_mean_diff = 0.0f;

    for (int c = 0; c < num_categories; c++) {
        const char *category = categories[c];

        /* Build paths */
        char sample_path[512];
        char dsp32_path[512];
        snprintf(sample_path, sizeof(sample_path), "%s%s/%s", base_path, sample_dir, category);
        snprintf(dsp32_path, sizeof(dsp32_path), "%s%s/%s", base_path, dsp32_dir, category);

        printf("--- Category: %s ---\n", category);

        /* Count files */
        int file_count = count_files_in_dir(sample_path);
        if (file_count < 0) {
            printf("  Skipping (cannot read directory)\n\n");
            continue;
        }
        printf("  Found %d files\n", file_count);

        /* Process each file */
        for (int i = 0; i < file_count; i++) {
            char input_file[512];
            char ref_file[512];
            char basename[64];
            snprintf(basename, sizeof(basename), "%s_%04d", category, i);
            snprintf(input_file, sizeof(input_file), "%s/%s.pcm", sample_path, basename);
            snprintf(ref_file, sizeof(ref_file), "%s/%s.bin", dsp32_path, basename);

            int result = process_file(input_file, ref_file, c);
            total_files++;
            if (result == 0) {
                passed_files++;
            } else {
                // Still count towards statistics
            }
        }
        printf("\n");
    }

    printf("=== Summary ===\n");
    printf("Total files: %d\n", total_files);
    printf("Passed: %d\n", passed_files);
    printf("Failed: %d\n", total_files - passed_files);

    return (passed_files == total_files) ? 0 : 1;
}

static int process_file(const char *input_path, const char *ref_path, int category_idx)
{
    FILE *fp;
    int16_t *pcm;
    float *mel_output;
    float *py_output;
    int n_frames;
    size_t read_count;

    /* Read PCM file */
    fp = fopen(input_path, "rb");
    if (!fp) {
        printf("  ERROR: cannot open %s\n", input_path);
        return -1;
    }

    pcm = (int16_t *)malloc(SAMPLE_RATE * sizeof(int16_t));
    if (!pcm) {
        fclose(fp);
        return -1;
    }

    read_count = fread(pcm, sizeof(int16_t), SAMPLE_RATE, fp);
    fclose(fp);

    if (read_count != SAMPLE_RATE) {
        printf("  WARNING: %s - expected %d samples, got %zu\n", input_path, SAMPLE_RATE, read_count);
    }

    /* Process with mel_dsp */
    n_frames = mel_dsp_get_frame_count(SAMPLE_RATE);
    mel_output = (float *)malloc(n_frames * MEL_SIZE * sizeof(float));
    if (!mel_output) {
        free(pcm);
        return -1;
    }

    mel_dsp_process(pcm, SAMPLE_RATE, mel_output);
    free(pcm);

    /* Read Python reference */
    fp = fopen(ref_path, "rb");
    if (!fp) {
        printf("  ERROR: cannot open reference %s\n", ref_path);
        free(mel_output);
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    long py_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    int py_frames = py_size / sizeof(float) / MEL_SIZE;
    py_output = (float *)malloc(py_frames * MEL_SIZE * sizeof(float));
    if (!py_output) {
        fclose(fp);
        free(mel_output);
        return -1;
    }

    fread(py_output, sizeof(float), py_frames * MEL_SIZE, fp);
    fclose(fp);

    /* Compare */
    int compare_frames = (n_frames < py_frames) ? n_frames : py_frames;
    int result = compare_outputs(mel_output, py_output, compare_frames, MEL_SIZE);

    free(mel_output);
    free(py_output);

    return result;
}

static int compare_outputs(const float *c_output, const float *py_output, int n_frames, int mel_size)
{
    float max_diff = 0.0f;
    float mean_diff = 0.0f;
    float max_rel_diff = 0.0f;
    int count = 0;

    for (int f = 0; f < n_frames; f++) {
        for (int i = 0; i < mel_size; i++) {
            float c_val = c_output[f * mel_size + i];
            float py_val = py_output[f * mel_size + i];
            float diff = fabsf(c_val - py_val);
            float rel_diff = diff / (fabsf(py_val) + 1e-10f);

            if (diff > max_diff) max_diff = diff;
            if (rel_diff > max_rel_diff) rel_diff = max_rel_diff;
            mean_diff += diff;
            count++;
        }
    }
    mean_diff /= count;

    int passed = (max_diff < 0.01f);  // Allow 0.01 absolute tolerance

    printf("  [%s] max_diff=%.6f mean_diff=%.6f max_rel=%.2f%% %s\n",
           passed ? "PASS" : "FAIL",
           max_diff, mean_diff, max_rel_diff * 100.0f,
           passed ? "" : "(ABORT)");

    if (!passed) {
        // Print first few diff values
        printf("       First 5 diffs:");
        for (int f = 0; f < 1 && f < n_frames; f++) {
            for (int i = 0; i < 5 && i < mel_size; i++) {
                float diff = fabsf(c_output[f * mel_size + i] - py_output[f * mel_size + i]);
                printf(" %.6f", diff);
            }
        }
        printf("\n");
    }

    return passed ? 0 : -1;
}

static int count_files_in_dir(const char *dir_path)
{
    DIR *dir;
    struct dirent *entry;
    int count = 0;

    dir = opendir(dir_path);
    if (!dir) {
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            const char *ext = strrchr(entry->d_name, '.');
            if (ext && strcmp(ext, ".pcm") == 0) {
                count++;
            }
        }
    }

    closedir(dir);
    return count;
}
