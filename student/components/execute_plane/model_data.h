#ifndef TFLITE_MODEL_DATA_H
#define TFLITE_MODEL_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

extern const unsigned char model_int8_tflite[];
extern const unsigned int model_int8_tflite_len;

#define SCALING_FACTORS    {1.0751028, 6.46053678, 9.90142454, 10.36176891, 13.33033384, 12.72560352, 13.14793948, 22.90498167, 20.47576543, 21.30183981, 20.23153021, 25.02760048}
#define MODEL_THRESHOLD    21

#ifdef __cplusplus
}
#endif

#endif // TFLITE_MODEL_DATA_H

