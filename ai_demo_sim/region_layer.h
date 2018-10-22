#ifndef _REGION_LAYER
#define _REGION_LAYER

#include <stdint.h>

// #define CLASS_NUMBER 1
#define CLASS_NUMBER 20
// #define DEBUG_FLOAT

#ifdef DEBUG_FLOAT
#define INPUT_TYPE float
#else
#define INPUT_TYPE uint8_t
#endif

typedef void (*callback_draw_box)(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t class, float prob);
void region_layer_cal(INPUT_TYPE *u8in);
void region_layer_draw_boxes(callback_draw_box callback);

#endif // _REGION_LAYER
