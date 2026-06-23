#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "dsp_mfcc_feature.h"
#include "tensorflow/lite/c/c_api.h"

#define PCM_FILE_SIZE 32000
#define PCM_SAMPLES (PCM_FILE_SIZE / sizeof(int16_t))
#define TFLITE_MODEL_PATH "../../dataset/d5_training//model_int8.tflite"
#define MFCC_SIZE (DSP_MFCC_COEF_FRAME*DSP_MFCC_COEF_SIZE)


float mfcc_scaling_factor[DSP_MFCC_COEF_SIZE] = {1.0751028, 6.46053678, 9.90142454, 10.36176891, 13.33033384, 12.72560352, 13.14793948, 22.90498167, 20.47576543, 21.30183981, 20.23153021, 25.02760048, 20.26385331};

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


static int load_pcm(const char *path,int16_t *pcm)
{
    FILE *fp=fopen(path,"rb");
    if(!fp) return -1;
    size_t n=fread(pcm,sizeof(int16_t),PCM_SAMPLES,fp);
    fclose(fp);
    return n==PCM_SAMPLES?0:-1;
}

static int save_mfcc(const char *path,const float *mfcc)
{
    FILE *fp=fopen(path,"wb");
    if(!fp) return -1;
    size_t n=fwrite(mfcc,sizeof(float),DSP_MFCC_COEF_FRAME*DSP_MFCC_COEF_SIZE,fp);
    fclose(fp);
    return n==(DSP_MFCC_COEF_FRAME*DSP_MFCC_COEF_SIZE)?0:-1;
}

static int save_dsp8(const char *path,const int8_t *mfcc)
{
    FILE *fp=fopen(path,"wb");
    if(!fp) return -1;
    size_t n=fwrite(mfcc,sizeof(int8_t),DSP_MFCC_COEF_FRAME*DSP_MFCC_COEF_SIZE,fp);
    fclose(fp);
    return n==(DSP_MFCC_COEF_FRAME*DSP_MFCC_COEF_SIZE)?0:-1;
}

static int calc_mfcc(const int16_t *pcm,float *mfcc)
{
    int i;
    for(i=0;i<DSP_MFCC_COEF_FRAME;i++)
    {
        const int16_t *frame=&pcm[i*DSP_MFCC_HOP_SIZE];
        dsp_mfcc_frame_process(frame,&mfcc[i*DSP_MFCC_COEF_SIZE]);
    }
    return 0;
}

static int8_t scaling_convert(float mfcc, float factor)
{
    int value = round(mfcc * factor);
    int8_t clip;
    if (value > 127)
        clip = 127;
    else if (value < -127)
        clip = -127;
    else
        clip = (int8_t)value;
    return clip;
}

static int mfcc_scaling_convert(const float *mfcc, int8_t *dsp8)
{
    int i, j;
    for(i=0;i<DSP_MFCC_COEF_FRAME;i++)
    {
        for (j = 0; j < DSP_MFCC_COEF_SIZE; j++)
            dsp8[i * DSP_MFCC_COEF_SIZE + j] = scaling_convert(mfcc[i * DSP_MFCC_COEF_SIZE + j], mfcc_scaling_factor[j]);
    }
    return 0;
}

int main(int argc,char *argv[])
{
    int16_t pcm[PCM_SAMPLES];
    int8_t dsp8[DSP_MFCC_COEF_FRAME*DSP_MFCC_COEF_SIZE];
    float mfcc[DSP_MFCC_COEF_FRAME*DSP_MFCC_COEF_SIZE];

    if(argc!=3)
    {
        printf("usage: %s input.pcm output.bin\n",argv[0]);
        return -1;
    }

    TfLiteInterpreter *interpreter = tflite_model_init();

    if(dsp_mfcc_init())
        return -1;

    if(load_pcm(argv[1],pcm))
        return -1;

    calc_mfcc(pcm,mfcc);

    mfcc_scaling_convert(mfcc, dsp8);

    if(save_dsp8(argv[2],dsp8))
        return -1;

    int8_t score = tf_lite_model_inference(interpreter, dsp8);
    printf("score : %d\n", score);

    return 0;
}