#ifndef MEL_DSP_H
#define MEL_DSP_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ====================================================================================================
 * Constants - Aligned with Python signal_process.py
 * ==================================================================================================== */
#define MEL_DSP_SAMPLE_RATE     16000
#define MEL_DSP_FRAME_SIZE      400
#define MEL_DSP_FFT_SIZE        512
#define MEL_DSP_HOP_SIZE        320
#define MEL_DSP_FFT_BINS        257         /* N_FFT // 2 + 1 */
#define MEL_DSP_MEL_SIZE        40          /* Number of mel filters */
#define MEL_DSP_LOG_EPS         1e-12f      /* Logging epsilon */

/* ====================================================================================================
 * Platform abstraction
 * ==================================================================================================== */
#include "mel_dsp_platform.h"

/* ====================================================================================================
 * Mel filter definition
 * ==================================================================================================== */
typedef struct {
    uint16_t start_bin;
    uint16_t length;
    const float *weights;
} mel_dsp_filter_t;

extern const mel_dsp_filter_t mel_dsp_filters[MEL_DSP_MEL_SIZE];

/* ====================================================================================================
 * Hanning window
 * ==================================================================================================== */
extern const float mel_dsp_hanning_window[MEL_DSP_FRAME_SIZE];

/* ====================================================================================================
 * DSP init
 * ==================================================================================================== */
int mel_dsp_init(void);

/* ====================================================================================================
 * Process one frame: 400 samples -> 40 log mel energies
 * Input:  int16_t PCM samples
 * Output: float[40] log mel energies (aligned with Python output)
 * ==================================================================================================== */
int mel_dsp_frame_process(const int16_t *input_pcm, float *output_mel);

/* ====================================================================================================
 * Process multiple frames
 * Input:  int16_t PCM samples array
 *         uint32_t sample_count - number of samples
 * Output: float array of size (n_frames * MEL_DSP_MEL_SIZE)
 *         n_frames = 1 + (sample_count - FRAME_SIZE) / HOP_SIZE
 * Returns: number of frames processed
 * ==================================================================================================== */
int mel_dsp_process(const int16_t *input_pcm, uint32_t sample_count, float *output_mel);

/* ====================================================================================================
 * Get number of frames that would be processed
 * ==================================================================================================== */
uint32_t mel_dsp_get_frame_count(uint32_t sample_count);

#ifdef __cplusplus
}
#endif

#endif /* MEL_DSP_H */
