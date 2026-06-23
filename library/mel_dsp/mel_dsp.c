#include <math.h>
#include <string.h>
#include "mel_dsp.h"
#include "mel_dsp_hann.h"
#include "mel_dsp_platform.h"

static const char *TAG = "MEL_DSP";

/*
 * FFT work buffers - allocated statically to avoid malloc overhead on ESP32
 */
__attribute__((aligned(16))) static float fft_data[MEL_DSP_FFT_SIZE * 2];
static float mag_output[MEL_DSP_FFT_BINS];
static float mel_output[MEL_DSP_MEL_SIZE];

/* ====================================================================================================
 * DSP init - initialize FFT
 * ==================================================================================================== */
int mel_dsp_init(void)
{
    int ret = dsps_fft2r_init_fc32(NULL, MEL_DSP_FFT_SIZE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "FFT initialization failed: %d", ret);
        return ret;
    }

    ESP_LOGI(TAG, "Mel DSP initialized");
    ESP_LOGI(TAG, "  Sample rate: %d Hz", MEL_DSP_SAMPLE_RATE);
    ESP_LOGI(TAG, "  Frame size: %d", MEL_DSP_FRAME_SIZE);
    ESP_LOGI(TAG, "  FFT size: %d", MEL_DSP_FFT_SIZE);
    ESP_LOGI(TAG, "  Hop size: %d", MEL_DSP_HOP_SIZE);
    ESP_LOGI(TAG, "  Mel filters: %d", MEL_DSP_MEL_SIZE);
    return ESP_OK;
}

/* ====================================================================================================
 * Process one frame: 400 samples -> 40 log mel energies
 * Aligned with Python signal_process.py dsp32_get_mfcc_feature()
 * ==================================================================================================== */
int mel_dsp_frame_process(const int16_t *input_pcm, float *output_mel)
{
    // Step 1: Zero-pad and apply Hanning window to FFT size
    // Python: padded_array = np.pad(pcm16, pad_width=(0, 112), mode='constant', constant_values=0)
    //         float_samples = padded_array.astype(np.float32)
    //         window = np.hanning(N_FFT)
    //         windowed_samples = float_samples * window
    memset(fft_data, 0, sizeof(fft_data));
    for (int i = 0; i < MEL_DSP_FFT_SIZE; i++) {
        float windowed_input = (i < MEL_DSP_FRAME_SIZE) ? (float)input_pcm[i] * mel_dsp_hanning_window[i] : 0.0f;
        fft_data[2 * i] = windowed_input;
    }

    // Step 2: Perform FFT
    // Python: fft_result = scipy_fft(windowed_samples, N_FFT)
    dsps_fft2r_fc32(fft_data, MEL_DSP_FFT_SIZE);
    dsps_bit_rev_fc32(fft_data, MEL_DSP_FFT_SIZE);

    // Step 3: Compute power spectrum
    // Python: fft_magnitude = np.abs(fft_result[:N_FFT // 2 + 1])
    //         power_spectrum = (fft_magnitude ** 2) / N_FFT
    for (int k = 0; k < MEL_DSP_FFT_BINS; k++) {
        float real = fft_data[2 * k];
        float imag = fft_data[2 * k + 1];
        mag_output[k] = (real * real + imag * imag) / MEL_DSP_FFT_SIZE;
    }

    // Step 4: Apply mel filterbank and compute log energies
    // Python: mel_energies = np.dot(power_spectrum, mel_basis.T)
    //         log_mel_energies = np.log(mel_energies.clip(min=1e-12))
    for (int m = 0; m < MEL_DSP_MEL_SIZE; m++) {
        const mel_dsp_filter_t *f = &mel_dsp_filters[m];
        float sum = 0.0f;
        for (int i = 0; i < f->length; i++) {
            sum += mag_output[f->start_bin + i] * f->weights[i];
        }
        mel_output[m] = logf(sum + MEL_DSP_LOG_EPS);
    }

    // Step 5: Copy output (40 log mel energies, NO DCT - aligned with Python)
    memcpy(output_mel, mel_output, MEL_DSP_MEL_SIZE * sizeof(float));

    return 0;
}

/* ====================================================================================================
 * Get number of frames that would be processed
 * Python: n_mfcc = 1 + (pcm16.size - bin_size) // hop_size
 * ==================================================================================================== */
uint32_t mel_dsp_get_frame_count(uint32_t sample_count)
{
    if (sample_count < MEL_DSP_FRAME_SIZE) {
        return 0;
    }
    return 1 + (sample_count - MEL_DSP_FRAME_SIZE) / MEL_DSP_HOP_SIZE;
}

/* ====================================================================================================
 * Process multiple frames
 * ==================================================================================================== */
int mel_dsp_process(const int16_t *input_pcm, uint32_t sample_count, float *output_mel)
{
    uint32_t n_frame = mel_dsp_get_frame_count(sample_count);
    const int16_t *iptr = input_pcm;
    float *optr = output_mel;

    for (uint32_t i = 0; i < n_frame; i++) {
        mel_dsp_frame_process(iptr, optr);
        iptr += MEL_DSP_HOP_SIZE;
        optr += MEL_DSP_MEL_SIZE;
    }

    return (int)n_frame;
}
