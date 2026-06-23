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
#define ESP_DATA_FILE      "../../storage/tools/default.pcm"
#define TFLITE_MODEL_PATH "../../dataset/d5_training/model_int8.tflite"

static float scaling_factor[DSP_MFCC_COEF_SIZE] = {1.0751028, 6.46053678, 9.90142454, 10.36176891, 13.33033384, 12.72560352, 13.14793948, 22.90498167, 20.47576543, 21.30183981, 20.23153021, 25.02760048, 20.26385331};

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

static int load_data(const char *path, int8_t *dsp_data)
{
    FILE *fp=fopen(path,"rb");
    if(!fp) return -1;
    size_t n=fread(dsp_data, sizeof(int8_t), ESP_DATA_SIZE,fp);
    fclose(fp);
    return n==ESP_DATA_SIZE?0:-1;
}

void test_esp_output()
{
    int8_t esp_data[ESP_DATA_SIZE];
    int8_t dsp[DSP_COEFS];
    int16_t *pcm = (int16_t *)esp_data;
    int8_t *esp_output = (int8_t *)(&esp_data[PCM_SAMPLES*sizeof(int16_t)]);

    load_data(ESP_DATA_FILE, esp_data);

    TfLiteInterpreter *interpreter = tflite_model_init();
    dsp_pipeline_init(scaling_factor);
    dsp_pipe_mfcc_int8(pcm, PCM_SAMPLES, dsp);
    int ret = memcmp(esp_output, dsp, DSP_COEFS);
    printf("compare_dsp_data (%d): %d\n", DSP_COEFS, ret);
    int8_t score = tf_lite_model_inference(interpreter, dsp);
    printf("Prediction : %d %d\n", esp_output[DSP_COEFS], score);
}

int main(int argc,char *argv[])
{
    test_esp_output();
}