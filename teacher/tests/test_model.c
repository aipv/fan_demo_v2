#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "../../dataset/model/model.h"

#define N_MFCC 49
#define N_COEF 40

typedef struct {
    float* data;
    int count;
} Dsp32Data;

typedef struct {
    float* probs;
    int count;
} PredictData;

Dsp32Data load_dsp32(const char* path) {
    Dsp32Data result = {0};
    FILE* fp = fopen(path, "rb");
    if (!fp) {
        printf("Error: cannot open %s\n", path);
        return result;
    }
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    result.count = size / sizeof(float) / N_MFCC / N_COEF;
    result.data = (float*)malloc(size);
    fread(result.data, sizeof(float), size / sizeof(float), fp);
    fclose(fp);
    return result;
}

PredictData load_predict(const char* path) {
    PredictData result = {0};
    FILE* fp = fopen(path, "rb");
    if (!fp) {
        printf("Error: cannot open %s\n", path);
        return result;
    }
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    result.count = size / sizeof(float);
    result.probs = (float*)malloc(size);
    fread(result.probs, sizeof(float), result.count, fp);
    fclose(fp);
    return result;
}

float* mean_pool_40(float* data, int count) {
    float* result = (float*)malloc(count * N_COEF * sizeof(float));
    for (int i = 0; i < count; i++) {
        for (int j = 0; j < N_COEF; j++) {
            float sum = 0.0f;
            for (int k = 0; k < N_MFCC; k++) {
                sum += data[i * N_MFCC * N_COEF + k * N_COEF + j];
            }
            result[i * N_COEF + j] = sum / N_MFCC;
        }
    }
    return result;
}

void compare_results(const char* group, float* c_probs, int c_count, float* py_probs, int py_count) {
    printf("%-10s ", group);

    if (c_count != py_count) {
        printf("COUNT MISMATCH: C=%d, Python=%d\n", c_count, py_count);
        return;
    }

    float max_diff = 0.0f;
    float mean_diff = 0.0f;
    for (int i = 0; i < c_count; i++) {
        float diff = fabsf(c_probs[i] - py_probs[i]);
        if (diff > max_diff) max_diff = diff;
        mean_diff += diff;
    }
    mean_diff /= c_count;

    float c_mean = 0.0f, py_mean = 0.0f;
    for (int i = 0; i < c_count; i++) {
        c_mean += c_probs[i];
        py_mean += py_probs[i];
    }
    c_mean /= c_count;
    py_mean /= py_count;

    printf("C_mean=%.9f Py_mean=%.9f Max_diff=%.9f Mean_diff=%.9f\n",
           c_mean, py_mean, max_diff, mean_diff);
}

int main() {
    const char* groups[] = {"normal", "abnormal", "d1", "d2", "d3", "d4"};
    int num_groups = 6;

    printf("%-10s %-20s %-20s %-15s %-15s\n", "Group", "C Mean", "Python Mean", "Max Diff", "Mean Diff");
    printf("--------------------------------------------------------------------------------\n");

    for (int i = 0; i < num_groups; i++) {
        const char* group = groups[i];
        char dsp32_path[256];
        char predict_path[256];

        sprintf(dsp32_path, "../../dataset/dsp32/%s.bin", group);
        sprintf(predict_path, "../../dataset/predict/%s.bin", group);

        Dsp32Data dsp32 = load_dsp32(dsp32_path);
        PredictData py_predict = load_predict(predict_path);

        if (!dsp32.data || !py_predict.probs) {
            printf("%-10s ERROR loading files\n", group);
            continue;
        }

        float* features = mean_pool_40(dsp32.data, dsp32.count);
        float* c_probs = (float*)malloc(dsp32.count * sizeof(float));

        for (int j = 0; j < dsp32.count; j++) {
            c_probs[j] = predict_prob(&features[j * N_COEF]);
        }

        compare_results(group, c_probs, dsp32.count, py_predict.probs, py_predict.count);

        free(dsp32.data);
        free(py_predict.probs);
        free(features);
        free(c_probs);
    }

    return 0;
}
