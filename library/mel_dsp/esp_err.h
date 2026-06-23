// Stub esp_err.h for Ubuntu/PC builds
#ifndef _ESP_ERR_H_
#define _ESP_ERR_H_

#include <stdlib.h>

typedef int esp_err_t;

#define ESP_OK          0
#define ESP_FAIL        -1
#define ESP_ERR_INVALID_ARG    0x102
#define ESP_ERR_NO_MEM        0x101

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#endif // _ESP_ERR_H_
