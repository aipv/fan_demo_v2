#ifndef DSP_PIPELINE_H
#define DSP_PIPELINE_H

#include <stdint.h>
#include <stddef.h>
#include "dsp_mfcc_feature.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ====================================================================================================
 * CONST Parameter
 * ==================================================================================================== */
#define DSP_PIPE_COEF_FRAME  98
#define DSP_PIPE_HOP_SIZE    160

/* ====================================================================================================
 * Statistic Counter
 * ==================================================================================================== */
typedef struct {
    float      min;
    float      max;
    float      sum;
    float      sum2;
} dsp_coef_counter_t;

typedef struct {
	uint32_t            n_frame;
	dsp_coef_counter_t  counter[DSP_MFCC_COEF_SIZE];
} dsp_mfcc_counter_t;

/* ====================================================================================================
 * Init statistic of coef distribution
 * ==================================================================================================== */
int dsp_pipe_init_statistic(int statistic_flag);

/* ====================================================================================================
 * Get statistic of coef distribution
 * ==================================================================================================== */
int dsp_pipe_get_statistic(dsp_mfcc_counter_t **statistic);

/* ====================================================================================================
 * Process multile Frame : N bin -> 13 * n coef;
 * ==================================================================================================== */
int dsp_pipe_mfcc_float(const int16_t *input_pcm, uint32_t sample_count, float *output_coef);

/* ====================================================================================================
 * DSP pipeline init
 * ==================================================================================================== */
int dsp_pipeline_init(float *scaling);

/* ====================================================================================================
 * Process multile Frame : N bin -> 13 * n coef;
 * ==================================================================================================== */
int dsp_pipe_mfcc_int8(const int16_t *input_pcm, uint32_t sample_count, int8_t *output_coef);



#ifdef __cplusplus
}
#endif

#endif /* DSP_PIPELINE_H */