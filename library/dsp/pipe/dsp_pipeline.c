#include <math.h>
#include <string.h>
#include "dsp_mfcc_feature.h"
#include "dsp_pipeline.h"

#define DSP_PIPE_MFCC_FRAME_COUNT(sample_count) \
    (((sample_count) < DSP_MFCC_FRAME_SIZE) ? \
    0 : \
    (((sample_count) - DSP_MFCC_FRAME_SIZE) / DSP_PIPE_HOP_SIZE + 1))

static int                            dsp_statistic_flag = 0;
static dsp_mfcc_counter_t             dsp_mfcc_counter;
static float                          dsp_pipe_scaling[DSP_MFCC_COEF_SIZE];

static int8_t coef_scaling_convert(float coef, float factor)
{
    int value = round(coef * factor);
    int8_t clip;
    if (value > 127)
        clip = 127;
    else if (value < -127)
        clip = -127;
    else
        clip = (int8_t)value;
    return clip;
}

static void dsp_coef_counter_update(float *coef, uint32_t n_frame)
{
    float *dptr = coef;
    dsp_coef_counter_t *counter = &dsp_mfcc_counter.counter[0];
    for (int i = 0; i < n_frame; i++)
    {
        for (int j = 0; j < DSP_MFCC_COEF_SIZE; j++)
        {
            if ((dsp_mfcc_counter.n_frame == 0) && (i == 0) && (j == 0))
            {
                counter[j].min = *dptr;
                counter[j].max = *dptr;
            }
            else
            {
                counter[j].min = (*dptr < counter[j].min) ? *dptr : counter[j].min;
                counter[j].max = (*dptr > counter[j].max) ? *dptr : counter[j].max;
            }
            counter[j].sum += *dptr;
            counter[j].sum2 += *dptr * *dptr;
            dptr++;
        }
    }
    dsp_mfcc_counter.n_frame += n_frame;
}

int dsp_pipe_init_statistic(int enable_flag)
{
    dsp_statistic_flag = enable_flag;
    memset(&dsp_mfcc_counter, 0, sizeof(dsp_mfcc_counter_t));
    return 0;
}

int dsp_pipe_get_statistic(dsp_mfcc_counter_t **statistic)
{
    *statistic = &dsp_mfcc_counter;
    return 0;
}

int dsp_pipe_mfcc_float(const int16_t *input_pcm, uint32_t sample_count, float *output_coef)
{
    const int16_t *iptr = input_pcm;
    float *optr = output_coef;

    uint32_t n_frame = DSP_PIPE_MFCC_FRAME_COUNT(sample_count);
    for (int i = 0; i < n_frame; i++)
    {
        dsp_mfcc_frame_process(iptr, optr);
        iptr += DSP_MFCC_HOP_SIZE;
        optr += DSP_MFCC_COEF_SIZE;
    }
    if (dsp_statistic_flag)
    {
        dsp_coef_counter_update(output_coef, n_frame);
    }

    return 0;
}

/* ====================================================================================================
 * DSP pipe config
 * ==================================================================================================== */
int dsp_pipeline_init(float *scaling)
{
    memcpy(dsp_pipe_scaling, scaling, sizeof(dsp_pipe_scaling));
    dsp_pipe_init_statistic(1);
    dsp_mfcc_init();
    return 0;
}

/* ====================================================================================================
 * Process multile Frame : N bin -> 13 * n coef;
 * ==================================================================================================== */
int dsp_pipe_mfcc_int8(const int16_t *input_pcm, uint32_t sample_count, int8_t *output_coef)
{
    float f32_coef[DSP_MFCC_COEF_SIZE];
    const int16_t *iptr = input_pcm;
    int8_t *optr = output_coef;

    uint32_t n_frame = DSP_PIPE_MFCC_FRAME_COUNT(sample_count);
    for (int i = 0; i < n_frame; i++)
    {
        dsp_mfcc_frame_process(iptr, f32_coef);
        dsp_coef_counter_update(f32_coef, 1);
        for (int j = 0; j < DSP_MFCC_COEF_SIZE; j++)
        {
            optr[j] = coef_scaling_convert(f32_coef[j], dsp_pipe_scaling[j]);
        }
        iptr += DSP_MFCC_HOP_SIZE;
        optr += DSP_MFCC_COEF_SIZE;
    }
    return 0;
}
