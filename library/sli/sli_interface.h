#ifndef SLI_INTERFACE_H
#define SLI_INTERFACE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ====================================================================================================
 * SLI MESSAGE HEADER
 * ==================================================================================================== */
typedef struct
{
    uint32_t magic;          // 'SLIM'
    uint32_t version;        // protocol version
    uint32_t payload_size;   // payload bytes
    uint32_t sequence_id;    // message sequence
    uint32_t command;        // command
    uint32_t sub_command;    // sub-command
    uint64_t timestamp_ms;   // runtime timestamp
} sli_msg_hdr_t;

/* ====================================================================================================
 * SLI INTERFACE CONST
 * ==================================================================================================== */
#define SLI_MESSAGE_HEADER_SIZE   sizeof(sli_msg_hdr_t)
#define SLI_MESSAGE_MAGIC_CODE    0x534C494D
#define SLI_MESSAGE_VERSION       1

/* ====================================================================================================
 * SLI MESSAGE COMMANDS
 * ==================================================================================================== */
#define SLI_EVENT_REP             0
#define SLI_DATA_REQ              1
#define SLI_DATA_RES              2
#define SLI_CAPACITY_REQ          3
#define SLI_CAPACITY_RES          4
#define SLI_TELEMETRY_REQ         5
#define SLI_TELEMETRY_RES         6
#define SLI_MAX_MESSAGE_CMD       6

/* ====================================================================================================
 * SLI TELEMETRY SUB COMMAND
 * ==================================================================================================== */
#define SLI_TELEMETRY_COEF_STAT   1

/* ====================================================================================================
 * SLI SUB COMMANDS for DATA REQ/RES, CAPACITY REQ/RES : list of req/res data
 * ==================================================================================================== */
#define SLI_DATA_TYPE_PCM32       0x01
#define SLI_DATA_TYPE_PCM16       0x02
#define SLI_DATA_TYPE_COEF32      0x04
#define SLI_DATA_TYPE_COEF8       0x08
#define SLI_DATA_TYPE_INFERENCE   0x10

/* ====================================================================================================
 * SLI_DATA_REQ/RES MESSAGE HEADER
 *
 * Teacher -> SLI_DATA_REQ -> Student
 * Student -> SLI_DATA_RES (data) -> Teacher
 *                ...
 * Student -> SLI_DATA_RES (data) -> Teacher
 * ==================================================================================================== */

typedef struct
{
    sli_msg_hdr_t header;
    uint32_t request_id;     // request id;
    uint32_t sample_count;   // number of samples
} sli_data_req_hdr_t;

typedef struct
{
    sli_msg_hdr_t header;
    uint32_t request_id;     // request id as SLI_DATA_REQ message;
    uint32_t sample_count;   // number of samples
    uint32_t total_bytes;    // total_bytes
    uint16_t chunk_count;
    uint16_t chunk_id;
    uint8_t data[];
} sli_data_res_hdr_t;

/* ====================================================================================================
 * SLI_CAPACITY_REQ/RES MESSAGE HEADER
 *
 * Teacher -> SLI_CAPACITY_REQ (data) -> Student
 * Student -> SLI_CAPACITY_RES (data) -> Teacher
 * ==================================================================================================== */

typedef struct
{
    sli_msg_hdr_t header;
    uint32_t request_id;     // request id;
    uint32_t input_type;     // bitmap for input data
    uint32_t input_size;     // size of input data
    uint32_t input_count;    // count of input data
    uint8_t data[];
} sli_capacity_req_hdr_t;

typedef struct
{
    sli_msg_hdr_t header;
    uint32_t request_id;     // request id as SLI_CAPACITY_REQ;
    uint32_t input_type;     // bitmap for input data
    uint32_t input_size;     // size of input data
    uint32_t input_count;    // count of input data
    uint32_t status_code;    // status of output data, 0 when success
    uint32_t output_size;    // size of output data
    uint32_t output_count;   // count of output data
    uint32_t process_time;   // processing time in ms
    uint8_t data[];
} sli_capacity_res_hdr_t;

/* ====================================================================================================
 * SLI_TELEMETRY_REQ/RES MESSAGE HEADER
 *
 * Teacher -> SLI_TELEMETRY_REQ (data) -> Student
 * Student -> SLI_TELEMETRY_RES (data) -> Teacher
 * ==================================================================================================== */

#ifdef __cplusplus
}
#endif

#endif /* SLI_INTERFACE_H */