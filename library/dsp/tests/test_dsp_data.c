#include <stdio.h>
#include <string.h>
#include "dsp_pipeline.h"

#define PCM_SAMPLES 16000
#define DSP_COEFS 1274
#define DSP_DATA_SIZE 33274

static float scaling_factor[DSP_MFCC_COEF_SIZE] = {1.0751028, 6.46053678, 9.90142454, 10.36176891, 13.33033384, 12.72560352, 13.14793948, 22.90498167, 20.47576543, 21.30183981, 20.23153021, 25.02760048, 20.26385331};
#define DSP_DATA_FILE      "../../../storage/tools/default.pcm"

static int load_data(const char *path, int8_t *dsp_data)
{
    FILE *fp=fopen(path,"rb");
    if(!fp) return -1;
    size_t n=fread(dsp_data, sizeof(int8_t), DSP_DATA_SIZE,fp);
    fclose(fp);
    return n==DSP_DATA_SIZE?0:-1;
}

void print_dsp(int8_t *dsp, int count)
{
    for (int i = 0; i < count; i++)
    {
        printf("%04d, ", dsp[i]);
        if (((i + 1) % 16) == 0)
            printf("\n");
    }
    printf("\n");
}

void test_dsp_data()
{
    int8_t dsp_data[DSP_DATA_SIZE];
    int8_t dsp[DSP_COEFS];
    int16_t *pcm = (int16_t *)dsp_data;
    int8_t *expect = (int8_t *)(&dsp_data[PCM_SAMPLES*sizeof(int16_t)]);

    load_data(DSP_DATA_FILE, dsp_data);

    dsp_pipeline_init(scaling_factor);
    dsp_pipe_mfcc_int8(pcm, PCM_SAMPLES, dsp);
    int ret = memcmp(expect, dsp, DSP_COEFS);
    printf("test_dsp_data : %d\n", ret);
}

int main()
{
    test_dsp_data();
}

