#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
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

static const char *TAG = "EXECUTE_PLANE";

static float scaling_factor[DSP_MFCC_COEF_SIZE] = {1.0751028, 6.46053678, 9.90142454, 10.36176891, 13.33033384, 12.72560352, 13.14793948, 22.90498167, 20.47576543, 21.30183981, 20.23153021, 25.02760048, 20.26385331};

extern void tflite_interpreter_init();
extern int8_t tflite_interpreter_predict(int8_t *data_input);

static size_t count = 16000;
static char data_buffer[64000];
static char send_buffer[37096];
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

    test_mfcc_process();
    tflite_interpreter_init();

    dsp_pipeline_init(scaling_factor);
    return ESP_OK;
}
