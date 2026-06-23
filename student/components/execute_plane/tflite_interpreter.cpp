#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_dsp.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "model_data.h"

static const char *TAG = "TFLITE_INTERPRETER";

constexpr int kArenaSize = 20 * 1024;
static uint8_t tensor_arena[kArenaSize];

static tflite::MicroInterpreter* interpreter;
static TfLiteTensor* input_tensor;
static TfLiteTensor* output_tensor;
static float input_scale;
static float input_zero_point;

#define TFLITE_COEFS_SIZE          13
#define TFLITE_FRAME_SIZE          1274

extern "C" void tflite_interpreter_init()
{
    const tflite::Model* model = tflite::GetModel(model_int8_tflite);

    static tflite::MicroMutableOpResolver<5> resolver;
    resolver.AddConv2D();
    resolver.AddMaxPool2D();
    resolver.AddFullyConnected();
    resolver.AddLogistic();
    resolver.AddMean();

    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kArenaSize);

    interpreter = &static_interpreter;

    TfLiteStatus allocate_status = interpreter->AllocateTensors();    
    if (allocate_status != kTfLiteOk) {
        ESP_LOGE(TAG, "AllocateTensors() failed! Check your kArenaSize.");
        return;
    }

    size_t used_bytes = interpreter->arena_used_bytes();
    ESP_LOGI(TAG, "✅ Tensor Arena Allocate Success.");
    ESP_LOGI(TAG, "Acutual Arena Space: %u bytes (%u KB)", used_bytes, used_bytes / 1024);

    input_tensor = interpreter->input(0);
    output_tensor = interpreter->output(0);
    input_scale = input_tensor->params.scale;
    input_zero_point = input_tensor->params.zero_point;
    ESP_LOGI(TAG, "output type = %d", output_tensor->type);
    ESP_LOGI(TAG, "input type = %d", input_tensor->type);
    ESP_LOGI(TAG, "input para = %f, %f", input_scale, input_zero_point);
    ESP_LOGI(TAG, "input dims: [%d, %d, %d, %d]", input_tensor->dims->data[0], input_tensor->dims->data[1], input_tensor->dims->data[2], input_tensor->dims->data[3]);
}

extern "C" int8_t tflite_interpreter_predict(int8_t *data_input)
{
    memcpy(input_tensor->data.int8, data_input, TFLITE_FRAME_SIZE);
    TfLiteStatus status = interpreter->Invoke();
    if(status != kTfLiteOk)
    {
        ESP_LOGE(TAG, "Invoke failed");
        return -127;
    }

    int8_t y_int8 = output_tensor->data.int8[0];
    ESP_LOGI(TAG, "predict output : %d", y_int8);
    return y_int8;
}
