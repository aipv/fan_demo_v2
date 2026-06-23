#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <dirent.h>

#include "tensorflow/lite/c/c_api.h"

#define MFCC_H 98
#define MFCC_W 13
#define MFCC_SIZE (MFCC_H * MFCC_W)

#define TFLITE_MODEL_PATH "../../../dataset/d5_training//model_int8.tflite"
#define TEST_POS_PATH     "../../../dataset/d4_mfcc_feature/dsp08/test_pos"
#define TEST_NEG_PATH     "../../../dataset/d4_mfcc_feature/dsp08/test_neg"

static int compare_names(const void *a, const void *b)
{
    const char * const *sa = a;
    const char * const *sb = b;
    return strcmp(*sa, *sb);
}

int read_sorted_filenames(const char *path, char ***filenames)
{
    DIR *dir = opendir(path);
    if (!dir)
    {
        perror("opendir");
        return -1;
    }

    int capacity = 128;
    int count = 0;

    char **list = malloc(capacity * sizeof(char *));
    if (!list)
    {
        closedir(dir);
        return -1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        if (count >= capacity)
        {
            capacity *= 2;
            char **tmp = realloc(list, capacity * sizeof(char *));

            if (!tmp)
            {
                closedir(dir);
                return -1;
            }

            list = tmp;
        }

        size_t len = strlen(path) + 1 + strlen(entry->d_name) + 1;
        list[count] = malloc(len);
        snprintf(list[count], len, "%s/%s", path, entry->d_name);
        count++;
    }

    closedir(dir);
    qsort(list,
          count,
          sizeof(char *),
          compare_names);

    *filenames = list;

    return count;
}

TfLiteInterpreter *tflite_model_init()
{
    const char *model_path = TFLITE_MODEL_PATH;
    TfLiteModel *model = TfLiteModelCreateFromFile(model_path);

    if (!model)
    {
        printf("Load model failed\n");
        return NULL;
    }

    TfLiteInterpreterOptions *options = TfLiteInterpreterOptionsCreate();

    TfLiteInterpreter *interpreter = TfLiteInterpreterCreate(model, options);

    if (!interpreter)
    {
        printf("Create interpreter failed\n");
        return NULL;
    }

    if (TfLiteInterpreterAllocateTensors(interpreter) != kTfLiteOk)
    {
        printf("Allocate tensors failed\n");
        return NULL;
    }
    return interpreter;
}

int8_t tf_lite_model_inference(TfLiteInterpreter *interpreter, int8_t *mfcc)
{
    TfLiteTensor *input = TfLiteInterpreterGetInputTensor(interpreter, 0);
    if (TfLiteTensorCopyFromBuffer(input, mfcc, MFCC_SIZE) != kTfLiteOk)
    {
        printf("Copy input failed\n");
        return -1;
    }
    if (TfLiteInterpreterInvoke(interpreter) != kTfLiteOk)
    {
        printf("Invoke failed\n");
        return -1;
    }
    const TfLiteTensor *output = TfLiteInterpreterGetOutputTensor(interpreter, 0);
    int8_t score;
    TfLiteTensorCopyToBuffer(output, &score, sizeof(score));
    //printf("INT8 Score = %d\n", score);
    return score;
}

int read_mfcc_data(const char *mfcc_path, int8_t *mfcc)
{
    FILE *fp = fopen(mfcc_path, "rb");

    if (!fp)
    {
        printf("Open MFCC file failed\n");
        return -1;
    }

    size_t n = fread(mfcc, sizeof(int8_t), MFCC_H * MFCC_W, fp);
    fclose(fp);

    if (n != MFCC_H * MFCC_W)
    {
        printf("MFCC size mismatch\n");
        return -1;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    const char *mfcc_path  = argv[1];
    int8_t mfcc[MFCC_H * MFCC_W];

    TfLiteInterpreter *interpreter = tflite_model_init();

    char **files;

    int count = read_sorted_filenames(TEST_POS_PATH, &files);
    for (int i = 0; i < count; i++)
    {
        read_mfcc_data(files[i], mfcc);
        int8_t score = tf_lite_model_inference(interpreter, mfcc);
        printf("%s : %d\n", files[i], score);
    }

    count = read_sorted_filenames(TEST_NEG_PATH, &files);
    for (int i = 0; i < count; i++)
    {
        read_mfcc_data(files[i], mfcc);
        int8_t score = tf_lite_model_inference(interpreter, mfcc);
        printf("%s : %d\n", files[i], score);
    }

    return 0;
}