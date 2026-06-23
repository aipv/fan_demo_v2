#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "sli_interface.h"
#include "student_learning.h"

#define SLI_SERVER_IP      "192.168.0.242"
#define SLI_SERVER_PORT    8889

static const char *TAG = "SLI";
static int g_sli_socket = -1;

/* ====================================================================================================
 * Print Message Header
 * ==================================================================================================== */
static void sli_print_header(const sli_msg_hdr_t *hdr)
{
    ESP_LOGI(TAG, "----------------------------------------");
    ESP_LOGI(TAG, "magic        : 0x%08lx", hdr->magic);
    ESP_LOGI(TAG, "version      : %lu", hdr->version);
    ESP_LOGI(TAG, "payload_size : %lu", hdr->payload_size);
    ESP_LOGI(TAG, "sequence_id  : %lu", hdr->sequence_id);
    ESP_LOGI(TAG, "command      : %lu", hdr->command);
    ESP_LOGI(TAG, "sub_command  : %lu", hdr->sub_command);
    ESP_LOGI(TAG, "timestamp_ms : %llu", hdr->timestamp_ms);
    ESP_LOGI(TAG, "----------------------------------------");
}

/* ====================================================================================================
 * Handle Message
 * ==================================================================================================== */
static int sli_handle_message(int sock, const sli_msg_hdr_t *hdr)
{
    sli_print_header(hdr);
    const char *test_data = "HHHHHHH";

    switch (hdr->command)
    {
        case SLI_DATA_REQ:
            ESP_LOGI(TAG, "SLI_DATA_REQ");
            sli_send_message(hdr->command, hdr->sub_command, test_data, strlen(test_data));
            break;

        case SLI_CAPACITY_REQ:
            ESP_LOGI(TAG, "SLI_CAPACITY_REQ");
            sli_send_message(hdr->command, hdr->sub_command, test_data, strlen(test_data));
            break;

        case SLI_TELEMETRY_REQ:
            ESP_LOGI(TAG, "SLI_TELEMETRY_REQ");
            sli_send_message(hdr->command, hdr->sub_command, test_data, strlen(test_data));
            break;

        default:
            ESP_LOGW(TAG, "Unknown Command %lu", hdr->command);
            break;
    }

    return 0;
}

/* ====================================================================================================
 * SLI Task
 * ==================================================================================================== */
static void sli_task(void *arg)
{
    while (1)
    {
        struct sockaddr_in server_addr;
        g_sli_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
        if (g_sli_socket < 0)
        {
            ESP_LOGE(TAG, "socket failed");
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port   = htons(SLI_SERVER_PORT);
        inet_pton(AF_INET, SLI_SERVER_IP, &server_addr.sin_addr);

        ESP_LOGI(TAG, "Connecting %s:%d", SLI_SERVER_IP, SLI_SERVER_PORT);
        if (connect(g_sli_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0)
        {
            ESP_LOGE(TAG, "connect failed");
            close(g_sli_socket);
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }
        ESP_LOGI(TAG, "Connected");

        while (1)
        {
            sli_msg_hdr_t hdr;
            int ret = recv(g_sli_socket, &hdr, sizeof(hdr), MSG_WAITALL);
            if (ret <= 0)
            {
                ESP_LOGW(TAG, "Disconnected");
                break;
            }

            if (hdr.magic != SLI_MESSAGE_MAGIC_CODE)
            {
                ESP_LOGE(TAG, "Invalid magic 0x%08lx", hdr.magic);
                continue;
            }

            sli_handle_message(g_sli_socket, &hdr);
        }

        close(g_sli_socket);
        g_sli_socket = -1;
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

/* ====================================================================================================
 * Student Learning Task Start
 * ==================================================================================================== */
int student_learning_start(void)
{
    xTaskCreate(sli_task, "sli_task", 4096, NULL, 5, NULL);

    return 0;
}

/* ====================================================================================================
 * Send SLI Message
 * ==================================================================================================== */
int sli_send_message(uint32_t command, uint32_t sub_command, const void *payload, uint32_t payload_size)
{
    sli_msg_hdr_t hdr;

    memset(&hdr, 0, sizeof(hdr));
    hdr.magic        = SLI_MESSAGE_MAGIC_CODE;
    hdr.version      = SLI_MESSAGE_VERSION;
    hdr.payload_size = payload_size;
    hdr.sequence_id++;
    hdr.command      = command;
    hdr.sub_command  = sub_command;
    hdr.timestamp_ms = esp_timer_get_time() / 1000;

    int ret = send(g_sli_socket, &hdr, sizeof(hdr), 0);
    if (ret != sizeof(hdr))
    {
        ESP_LOGE(TAG, "send header failed");
        return -1;
    }

    if ((payload != NULL) && (payload_size > 0))
    {
        ret = send(g_sli_socket, payload, payload_size, 0);
        if (ret != payload_size)
        {
            ESP_LOGE(TAG, "send payload failed");
            return -1;
        }
    }

    return 0;
}