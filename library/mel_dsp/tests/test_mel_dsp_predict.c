#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "mel_dsp.h"

/*
 * Prediction model - copied from dataset/model/model.h
 * Logistic Regression: prob = 1 / (1 + exp(-(dot(feature, WEIGHT) + BIAS)))
 */
static const float g_bias = -31.848043442f;
static const float g_weight[40] = {
    -0.866885245f, -0.443949252f,  0.795156956f,  0.936486661f, -0.841350436f, -1.050930619f,  0.351005018f, -0.056457724f,
     1.208839774f,  0.583916664f, -0.442378700f, -1.057223797f,  0.273779005f,  0.617460847f,  0.781183720f,  0.285840422f,
     0.136659577f, -0.166905433f,  0.786048710f,  0.090710007f,  0.724035144f,  0.250064403f,  0.613965750f, -0.098670818f,
     1.583002090f,  1.324972034f,  0.878500044f, -2.952565193f, -2.080192327f, -2.122669220f,  0.018330466f,  0.330480725f,
    -0.723881245f, -1.083147407f,  0.990058184f,  0.328958690f, -0.143889606f, -0.105444655f,  1.555006504f,  2.324244499f,
};

#define SAMPLE_RATE 16000
#define BYTES_PER_SECOND (SAMPLE_RATE * sizeof(int16_t))  /* 32000 bytes */
#define FRAMES_PER_SECOND 49  /* 1 + (16000 - 400) / 320 = 49 */
#define MEL_SIZE 40

/*
 * Predict score from 40 averaged mel features
 */
static float predict_score(const float *feature)
{
    float score = g_bias;
    for (int i = 0; i < 40; i++) {
        score += feature[i] * g_weight[i];
    }
    return score;
}

/*
 * Predict probability from score
 */
static float predict_prob(float score)
{
    return 1.0f / (1.0f + expf(-score));
}

/*
 * Process one second of PCM data through mel_dsp
 * Returns: pointer to allocated buffer with 40 averaged mel features (caller must free)
 */
static float *process_one_second(const int16_t *pcm_samples)
{
    float *mel_features = (float *)malloc(FRAMES_PER_SECOND * MEL_SIZE * sizeof(float));
    if (!mel_features) {
        return NULL;
    }

    /* Run mel_dsp on the 16000 samples */
    int n_frames = mel_dsp_process(pcm_samples, SAMPLE_RATE, mel_features);
    if (n_frames != FRAMES_PER_SECOND) {
        printf("Warning: expected %d frames, got %d\n", FRAMES_PER_SECOND, n_frames);
    }

    /* Mean pool over frames to get 40 averaged features */
    float *averaged = (float *)malloc(MEL_SIZE * sizeof(float));
    if (!averaged) {
        free(mel_features);
        return NULL;
    }

    for (int j = 0; j < MEL_SIZE; j++) {
        float sum = 0.0f;
        for (int i = 0; i < n_frames; i++) {
            sum += mel_features[i * MEL_SIZE + j];
        }
        averaged[j] = sum / n_frames;
    }

    free(mel_features);
    return averaged;
}

/*
 * Read raw PCM file and process all 1-second samples
 * Returns: allocated array of probabilities (caller must free)
 *          *count returns the number of samples
 */
static float *process_raw_file(const char *raw_path, int *count)
{
    FILE *fp = fopen(raw_path, "rb");
    if (!fp) {
        printf("Error: cannot open %s\n", raw_path);
        return NULL;
    }

    /* Get file size */
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    int n_samples = file_size / BYTES_PER_SECOND;
    printf("  File size: %ld bytes, %d 1-second samples\n", file_size, n_samples);

    float *probabilities = (float *)malloc(n_samples * sizeof(float));
    if (!probabilities) {
        fclose(fp);
        return NULL;
    }

    int16_t *pcm_buffer = (int16_t *)malloc(BYTES_PER_SECOND);
    if (!pcm_buffer) {
        free(probabilities);
        fclose(fp);
        return NULL;
    }

    for (int i = 0; i < n_samples; i++) {
        size_t read = fread(pcm_buffer, 1, BYTES_PER_SECOND, fp);
        if (read != BYTES_PER_SECOND) {
            printf("Warning: short read at sample %d\n", i);
            break;
        }

        float *features = process_one_second(pcm_buffer);
        if (!features) {
            printf("Error processing sample %d\n", i);
            break;
        }

        float score = predict_score(features);
        probabilities[i] = predict_prob(score);

        free(features);

        if ((i + 1) % 100 == 0) {
            printf("  Processed %d / %d samples\n", i + 1, n_samples);
        }
    }

    free(pcm_buffer);
    fclose(fp);
    *count = n_samples;
    return probabilities;
}

/*
 * Compare C predictions with Python predictions
 */
static void compare_predictions(const char *group, float *c_probs, int c_count,
                               const char *python_path)
{
    /* Read Python predictions */
    FILE *fp = fopen(python_path, "rb");
    if (!fp) {
        printf("Error: cannot open Python predictions at %s\n", python_path);
        return;
    }

    fseek(fp, 0, SEEK_END);
    long py_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    int py_count = py_size / sizeof(float);
    printf("  Python predictions: %d, C predictions: %d\n", py_count, c_count);

    float *py_probs = (float *)malloc(py_count * sizeof(float));
    fread(py_probs, sizeof(float), py_count, fp);
    fclose(fp);

    /* Compare */
    float max_diff = 0.0f;
    float mean_diff = 0.0f;
    int compare_count = (c_count < py_count) ? c_count : py_count;

    for (int i = 0; i < compare_count; i++) {
        float diff = fabsf(c_probs[i] - py_probs[i]);
        if (diff > max_diff) max_diff = diff;
        mean_diff += diff;
    }
    mean_diff /= compare_count;

    float c_mean = 0.0f, py_mean = 0.0f;
    for (int i = 0; i < compare_count; i++) {
        c_mean += c_probs[i];
        py_mean += py_probs[i];
    }
    c_mean /= compare_count;
    py_mean /= compare_count;

    printf("\n=== %s ===\n", group);
    printf("  %-15s %-15s %-15s %-15s\n", "", "C Mean", "Python Mean", "Max Diff");
    printf("  %-15s %-15.9f %-15.9f %-15.9f\n", "", c_mean, py_mean, max_diff);
    printf("  Mean diff: %.9f\n", mean_diff);

    free(py_probs);
}

/*
 * Write C predictions to file
 */
static void write_predictions(const char *output_path, float *probabilities, int count)
{
    FILE *fp = fopen(output_path, "wb");
    if (!fp) {
        printf("Error: cannot create %s\n", output_path);
        return;
    }
    fwrite(probabilities, sizeof(float), count, fp);
    fclose(fp);
    printf("  Wrote %d predictions to %s\n", count, output_path);
}

int main(int argc, char *argv[])
{
    const char *raw_dir = "../../../dataset/raw";
    const char *python_dir = "../../../dataset/predict";
    const char *output_dir = "./output";

    if (argc > 1) raw_dir = argv[1];
    if (argc > 2) python_dir = argv[2];
    if (argc > 3) output_dir = argv[3];

    printf("=== Mel DSP Prediction Test ===\n\n");
    printf("Raw data dir: %s\n", raw_dir);
    printf("Python predict dir: %s\n", python_dir);
    printf("Output dir: %s\n\n", output_dir);

    /* Initialize DSP */
    printf("Initializing mel_dsp...\n");
    if (mel_dsp_init() != 0) {
        printf("mel_dsp_init failed\n");
        return -1;
    }

    /* Create output directory */
    char mkdir_cmd[256];
    snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p %s", output_dir);
    system(mkdir_cmd);

    /* Process each group */
    const char *groups[] = {"normal", "abnormal", "d1", "d2", "d3", "d4"};
    int n_groups = 6;

    printf("\n");

    for (int g = 0; g < n_groups; g++) {
        const char *group = groups[g];
        printf("=== Processing group: %s ===\n", group);

        /* Build paths */
        char raw_path[256], python_path[256], output_path[256];
        snprintf(raw_path, sizeof(raw_path), "%s/%s.pcm", raw_dir, group);
        snprintf(python_path, sizeof(python_path), "%s/%s.bin", python_dir, group);
        snprintf(output_path, sizeof(output_path), "%s/%s.bin", output_dir, group);

        /* Process raw file */
        int count = 0;
        float *c_probs = process_raw_file(raw_path, &count);
        if (!c_probs) {
            printf("Failed to process %s\n", raw_path);
            continue;
        }

        /* Write C predictions */
        write_predictions(output_path, c_probs, count);

        /* Compare with Python */
        compare_predictions(group, c_probs, count, python_path);

        printf("\n");

        free(c_probs);
    }

    printf("=== Done ===\n");
    return 0;
}
