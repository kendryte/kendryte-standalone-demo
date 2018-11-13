#ifndef _REGION_LAYER
#define _REGION_LAYER

#include <stdint.h>
#include "kpu.h"

#ifdef DEBUG_FLOAT
#define INPUT_TYPE float
#else
#define INPUT_TYPE uint8_t
#endif

typedef void (*callback_draw_box)(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t class, float prob);
void region_layer_cal(INPUT_TYPE *u8in);
void region_layer_draw_boxes(callback_draw_box callback);
int region_layer_init(kpu_task_t *task,uint32_t display_width, uint32_t display_hight, float layer_thresh, float layer_nms, uint32_t anchor_num, float *anchor_ptr);
void set_coords_n(uint32_t v_coords, uint32_t v_anchor);

#endif // _REGION_LAYER
