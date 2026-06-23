#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "dsp_pipeline.h"
#include "tensorflow/lite/c/c_api.h"
#include <stdio.h>
#include <string.h>

#define PCM_SAMPLES 16000
#define DSP_COEFS 1274
#define ESP_DATA_SIZE 33275
#define ESP_DATA_FILE      "../../storage/tools/s60.bin"
#define TFLITE_MODEL_PATH "../../dataset/d5_training/model_int8.tflite"

static float scaling_factor[DSP_MFCC_COEF_SIZE] = {1.0751028, 6.46053678, 9.90142454, 10.36176891, 13.33033384, 12.72560352, 13.14793948, 22.90498167, 20.47576543, 21.30183981, 20.23153021, 25.02760048, 20.26385331};
char data_buffer[2000000];

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
    if (TfLiteTensorCopyFromBuffer(input, mfcc, DSP_COEFS) != kTfLiteOk)
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

static int load_data(const char *path)
{
    FILE *fp=fopen(path,"rb");
    if(!fp) return -1;
    size_t n=fread(data_buffer, sizeof(int8_t), sizeof(data_buffer), fp);
    fclose(fp);
    return n;
}

void test_esp_output()
{
    int8_t dsp[DSP_COEFS];
    int dataSize = load_data(ESP_DATA_FILE);
    if ((dataSize % ESP_DATA_SIZE) != 0)
    {
        printf("Invalid Data Size : %d\n", dataSize);
    }

    TfLiteInterpreter *interpreter = tflite_model_init();
    dsp_pipeline_init(scaling_factor);

    int8_t *dptr = data_buffer;
    int dataFrame = dataSize / ESP_DATA_SIZE;
    for (int i = 0; i < dataFrame; i++)
    {
        int16_t *pcm = (int16_t *)dptr;
        int8_t *esp_coef = (int8_t *)(&pcm[PCM_SAMPLES]);
        int8_t esp_score = esp_coef[DSP_COEFS];
        dsp_pipe_mfcc_int8(pcm, PCM_SAMPLES, dsp);
        int ret = memcmp(esp_coef, dsp, DSP_COEFS);
        int8_t score = tf_lite_model_inference(interpreter, dsp);
        printf("Prediction %d : %d %d %d\n", i, ret, esp_score, score);
        dptr += ESP_DATA_SIZE;
    }
}

int main(int argc,char *argv[])
{
    test_esp_output();
}