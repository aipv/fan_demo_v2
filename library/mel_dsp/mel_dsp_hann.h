#ifndef MEL_DSP_HANN_H
#define MEL_DSP_HANN_H

#include "mel_dsp.h"

/*
 * Hanning window - 512 samples (FFT size)
 * Generated from: window = np.hanning(512)
 * Applied to zero-padded input (400 samples + 112 zeros)
 */

extern const float mel_dsp_hanning_window[MEL_DSP_FFT_SIZE];

#endif /* MEL_DSP_HANN_H */
