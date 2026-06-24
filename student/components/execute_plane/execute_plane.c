#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <math.h>
#include "i2s_audio.h"
#include "gpio_button.h"
#include "network_socket.h"
#include "support_plane.h"
#include "observe_plane.h"
#include "execute_plane.h"
#include "dsp_mfcc_feature.h"
#include "dsp_pipeline.h"
#include "test_mfcc_feature.h"
#include "student_learning.h"
#include "mel_dsp.h"

static const char *TAG = "EXECUTE_PLANE";

static float scaling_factor[DSP_MFCC_COEF_SIZE] = {1.0751028, 6.46053678, 9.90142454, 10.36176891, 13.33033384, 12.72560352, 13.14793948, 22.90498167, 20.47576543, 21.30183981, 20.23153021, 25.02760048, 20.26385331};

/*
 * Prediction model - aligned with Python
 * Logistic Regression: prob = 1 / (1 + exp(-(dot(feature, WEIGHT) + BIAS)))
 */
static const float g_predict_bias = -31.848043442f;
static const float g_predict_weight[40] = {
    -0.866885245f, -0.443949252f,  0.795156956f,  0.936486661f, -0.841350436f, -1.050930619f,  0.351005018f, -0.056457724f,
     1.208839774f,  0.583916664f, -0.442378700f, -1.057223797f,  0.273779005f,  0.617460847f,  0.781183720f,  0.285840422f,
     0.136659577f, -0.166905433f,  0.786048710f,  0.090710007f,  0.724035144f,  0.250064403f,  0.613965750f, -0.098670818f,
     1.583002090f,  1.324972034f,  0.878500044f, -2.952565193f, -2.080192327f, -2.122669220f,  0.018330466f,  0.330480725f,
    -0.723881245f, -1.083147407f,  0.990058184f,  0.328958690f, -0.143889606f, -0.105444655f,  1.555006504f,  2.324244499f,
};

extern void tflite_interpreter_init();
extern int8_t tflite_interpreter_predict(int8_t *data_input);

static size_t count = 16000;
static char data_buffer[64000];
static char send_buffer[40000];  /* 32000 PCM + 7840 mel output + 160 margin */
static int32_t *pcm_data = (int32_t *)(data_buffer);
static int16_t *pcm16_data = (int16_t *)(send_buffer);

typedef struct {
    int32_t    count;
    float      min;
    float      max;
    float      sum;
    double     sum2;
} CoefStatistic;
CoefStatistic   Counter[13];

void counter_init()
{
    for (int i = 0; i < 13; i++)
    {
        Counter[i].count = 0;
        Counter[i].min = 0;
        Counter[i].max = 0;
        Counter[i].sum = 0;
        Counter[i].sum2 = 0;
    }
}

void counter_update(float *data)
{
    for (int i = 0; i < 98; i++)
    {
        for (int j = 0; j < 13; j++)
        {
            float coef = data[i * 13 + j];
            Counter[j].min = (coef < Counter[j].min) ? coef : Counter[j].min;
            Counter[j].max = (coef > Counter[j].max) ? coef : Counter[j].max;
            Counter[j].sum += coef;
            Counter[j].sum2 += coef * coef;
            Counter[j].count++;
        }
    }
}

void counter_print()
{
    for (int i = 0; i < 13; i++)
    {
        ESP_LOGI(TAG, "%d : %d, %f, %f, %f, %lf", i, Counter[i].count, Counter[i].min, Counter[i].max, Counter[i].sum, Counter[i].sum2);
    }
}

esp_err_t pcm16_mfcc_preprocess(int16_t *input_pcm, int count)
{
    int16_t *iptr = input_pcm;
    float *optr = (float *)(input_pcm + count);

    for (int i = 0; i < DSP_MFCC_COEF_FRAME; i++)
    {
        dsp_mfcc_frame_process(iptr, optr);
        iptr += DSP_MFCC_HOP_SIZE;
        optr += DSP_MFCC_COEF_SIZE;
    }
    return ESP_OK;
}

void application_button_boot_callback(uint8_t gpio_num)
{
    ESP_LOGW(TAG, ">>> Button Boot (GPIO %d) Pressed! - Executing action A.", gpio_num);
    i2s_audio_read_data(pcm_data, count);
    ESP_LOGI(TAG, "Success read %d samples!", count);
    i2s_audio_play_data(pcm_data, count);
    ESP_LOGI(TAG, "Success play %d samples!", count);
    i2s_audio_convert_data(pcm_data, pcm16_data, count);
    ESP_LOGI(TAG, "Success convert %d samples!", count);

    // DSP: use send_buffer space after pcm16_data for mel output (49*40*4 = 7840 bytes)
    float *mel_buf = (float *)(pcm16_data + 16000);
    int n_frames = mel_dsp_process(pcm16_data, 16000, mel_buf);
    ESP_LOGI(TAG, "DSP: %d frames processed", n_frames);

    // Mean pool over frames to get 40 averaged features
    float mel_features[40];
    for (int j = 0; j < 40; j++) {
        float sum = 0.0f;
        for (int i = 0; i < n_frames; i++) {
            sum += mel_buf[i * 40 + j];
        }
        mel_features[j] = sum / n_frames;
    }

    // Predict: logistic regression
    float score = g_predict_bias;
    for (int i = 0; i < 40; i++) {
        score += mel_features[i] * g_predict_weight[i];
    }
    float probability = 1.0f / (1.0f + expf(-score));
    ESP_LOGI(TAG, "Prediction: %.6f", probability);

    network_socket_data_publish(pcm16_data, 32000);
    ESP_LOGI(TAG, "Success Publish feature %d samples!", count);
}

void application_button_up_callback(uint8_t gpio_num)
{
    ESP_LOGW(TAG, ">>> Button Up (GPIO %d) Pressed! - Executing action B.", gpio_num);
    counter_init();
    counter_print();
    if (network_socket_init() < 0)
    {
        ESP_LOGE(TAG, "Failed to connect to host.");
        return;
    }
    for (int i = 0; i < 300; i++)
    {
        i2s_audio_read_data(pcm_data, count);
        i2s_audio_convert_data(pcm_data, pcm16_data, count);
        if (((i + 1) % 60) == 0)
            ESP_LOGE(TAG, "%d", i);
        //int8_t *dsp = (int8_t *)(&pcm16_data[count]);
        //dsp_pipe_mfcc_int8(pcm16_data, count, dsp);
        //ESP_LOGI(TAG, "%d %d %d %d %d %d", dsp[0], dsp[1], dsp[2], dsp[3], dsp[4], dsp[5]);
        //int8_t score = tflite_interpreter_predict(dsp);
        //ESP_LOGI(TAG, "score : %d", score);
        //dsp[1274] = score;
        int bytes_sent = network_socket_send(pcm16_data, 32000);
        ESP_LOGI(TAG, "Success Publish %d bytes!", bytes_sent);
    }
    network_socket_close();
    ESP_LOGI(TAG, "Success Stream 100 seconds data!");
}

void application_button_down_callback(uint8_t gpio_num)
{
    ESP_LOGW(TAG, ">>> Button Down (GPIO %d) Pressed! - Executing action C.", gpio_num);
    i2s_audio_read_data(pcm_data, count);
    ESP_LOGI(TAG, "Success read %d samples!", count);
    i2s_audio_play_data(pcm_data, count);
    ESP_LOGI(TAG, "Success play %d samples!", count);
    i2s_audio_convert_data(pcm_data, pcm16_data, count);
    ESP_LOGI(TAG, "Success convert %d samples!", count);
    int8_t *dsp = (int8_t *)(&pcm16_data[count]);
    dsp_pipe_mfcc_int8(pcm16_data, count, dsp);
    ESP_LOGI(TAG, "%d %d %d %d %d %d", dsp[0], dsp[1], dsp[2], dsp[3], dsp[4], dsp[5]);
    int8_t score = tflite_interpreter_predict(dsp);
    ESP_LOGI(TAG, "score : %d", score);
    dsp[1274] = score;
    network_socket_data_publish(pcm16_data, 32000 + 1275);
    ESP_LOGI(TAG, "Success Publish %d bytes!", 33275);
}

esp_err_t execute_plane_init(void)
{
    gpio_button_set_callback_func(0, application_button_boot_callback);
    gpio_button_set_callback_func(1, application_button_up_callback);
    gpio_button_set_callback_func(2, application_button_down_callback);

    mel_dsp_init();
    test_mfcc_process();
    tflite_interpreter_init();

    dsp_pipeline_init(scaling_factor);
    return ESP_OK;
}
