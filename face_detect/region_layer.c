#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <kpu.h>
#include "region_layer.h"

typedef struct box_t {
    float x, y, w, h;
} box;

typedef struct{
    int index;
    int class;
    float **probs;
} sortable_bbox;

uint32_t region_layer_img_w;
uint32_t region_layer_img_h;
uint32_t region_layer_net_w;
uint32_t region_layer_net_h;
float region_layer_thresh;
float region_layer_nms;

// params parse_net_options
uint32_t region_layer_l_h;  // From 14 conv
uint32_t region_layer_l_w; // From 14 conv
uint32_t region_layer_boxes;
uint32_t region_layer_outputs;
// parse_region
uint32_t region_layer_l_classes;
uint32_t region_layer_l_coords = 4;
uint32_t region_layer_l_n = 5;

static float *output = NULL;
static box *boxes = NULL;
static float *probs_buf = NULL;
static float **probs = NULL;

static const float l_biases[] = {0.57273, 0.677385, 1.87446, 2.06253, 3.33843, 5.47434, 7.88282, 3.52778, 9.77052, 9.16828};
#ifndef DEBUG_FLOAT
#include "region_layer_array.include"
#endif

void set_coords_n(uint32_t v_coords, uint32_t v_anchor)
{
    region_layer_l_coords = v_coords;
    region_layer_l_n = v_anchor;
}

void region_layer_init(kpu_task_t *task,uint32_t display_width, uint32_t display_hight, float layer_thresh, float layer_nms)
{
    kpu_layer_argument_t* last_layer = &task->layers[task->layers_length-1];
    kpu_layer_argument_t* first_layer = &task->layers[0];

    region_layer_thresh = layer_thresh;
    region_layer_nms = layer_nms;

    region_layer_img_w = display_width;
    region_layer_img_h = display_hight;

    region_layer_l_classes = (last_layer->image_channel_num.data.o_ch_num + 1) / 5 - 5;

    region_layer_net_w = first_layer->image_size.data.i_row_wid + 1;
    region_layer_net_h = first_layer->image_size.data.i_col_high + 1;

    region_layer_l_w = last_layer->image_size.data.o_row_wid + 1;
    region_layer_l_h = last_layer->image_size.data.o_col_high + 1;

    printf("region_layer_l_classes is %d\n", region_layer_l_classes);
    printf("region_layer_net_w is %d\n", region_layer_net_w);
    printf("region_layer_net_h is %d\n", region_layer_net_h);
    printf("region_layer_l_w is %d\n", region_layer_l_w);
    printf("region_layer_l_h is %d\n", region_layer_l_h);

    region_layer_boxes = (region_layer_l_h * region_layer_l_w * region_layer_l_n); // l.w * l.h * l.n
    region_layer_outputs = (region_layer_boxes * (region_layer_l_classes + region_layer_l_coords + 1)); // l.h * l.w * l.n * (l.classes + l.coords + 1)

    output = malloc(region_layer_outputs * sizeof(float));

    boxes = malloc(region_layer_boxes * sizeof(box));

    probs_buf = malloc(region_layer_boxes * (region_layer_l_classes + 1) * sizeof(float));

    probs = malloc(region_layer_boxes * sizeof(float *));
}

void region_layer_deinit(kpu_task_t *task)
{
    if(output != NULL)
        free(output);
    if(boxes != NULL)
        free(boxes);
    if(probs_buf != NULL)
        free(probs_buf);
    if(probs != NULL)
        free(probs);
}

static  void activate_array(float *x, const int n, const INPUT_TYPE *input)
{
    for (int i = 0; i < n; ++i)
#ifdef DEBUG_FLOAT
        x[i] = 1.0f / (1.0f + expf(-input[i]));
#else
        x[i] = activate_array_acc[input[i]];
#endif
}

static  int entry_index(int location, int entry)
{
    int n   = location / (region_layer_l_w * region_layer_l_h);
    int loc = location % (region_layer_l_w * region_layer_l_h);

    return n * region_layer_l_w * region_layer_l_h *
        (region_layer_l_coords + region_layer_l_classes + 1) +
        entry * region_layer_l_w * region_layer_l_h + loc;
}

static  void softmax(const INPUT_TYPE *u8in, int n, int stride, float *output)
{
    int i;
    float e;
    float sum = 0;
    INPUT_TYPE largest_i = u8in[0];
#ifdef DEBUG_FLOAT
    float diff;
#else
    int diff;
#endif

    for (i = 0; i < n; ++i) {
        if (u8in[i * stride] > largest_i)
            largest_i = u8in[i * stride];
    }

    for (i = 0; i < n; ++i) {
        diff = u8in[i * stride] - largest_i;
#ifdef DEBUG_FLOAT
        e = expf(diff);
#else
        e = softmax_acc[diff + 255];
#endif
        sum += e;
        output[i * stride] = e;
    }
    for (i = 0; i < n; ++i)
        output[i * stride] /= sum;
}

static  void softmax_cpu(const INPUT_TYPE *input, int n, int batch, int batch_offset, int groups, int stride, float *output)
{
    int g, b;

    for (b = 0; b < batch; ++b) {
        for (g = 0; g < groups; ++g)
            softmax(input + b * batch_offset + g, n, stride, output + b * batch_offset + g);
    }
}

static  void forward_region_layer(const INPUT_TYPE *u8in, float *output)
{
    volatile int n, index;

    for (index = 0; index < 8750; index++)
        output[index] = u8in[index] * scale + bais;

    for (n = 0; n < region_layer_l_n; ++n) {
        index = entry_index(n * region_layer_l_w * region_layer_l_h, 0);
        activate_array(output + index, 2 * region_layer_l_w * region_layer_l_h, u8in + index);
        index = entry_index(n * region_layer_l_w * region_layer_l_h, 4);
        activate_array(output + index, region_layer_l_w * region_layer_l_h, u8in + index);
    }

    index = entry_index(0, 5);
    softmax_cpu(u8in + index, region_layer_l_classes, region_layer_l_n,
            region_layer_outputs / region_layer_l_n,
            region_layer_l_w * region_layer_l_h,
            region_layer_l_w * region_layer_l_h, output + index);
}

static  void correct_region_boxes(box *boxes)
{
    int new_w = 0;
    int new_h = 0;

    if (((float)region_layer_net_w / region_layer_img_w) <
        ((float)region_layer_net_h / region_layer_img_h)) {
        new_w = region_layer_net_w;
        new_h = (region_layer_img_h * region_layer_net_w) / region_layer_img_w;
    } else {
        new_h = region_layer_net_h;
        new_w = (region_layer_img_w * region_layer_net_h) / region_layer_img_h;
    }
    for (int i = 0; i < region_layer_boxes; ++i) {
        volatile box b = boxes[i];

        b.x = (b.x - (region_layer_net_w - new_w) / 2. / region_layer_net_w) /
              ((float)new_w / region_layer_net_w);
        b.y = (b.y - (region_layer_net_h - new_h) / 2. / region_layer_net_h) /
              ((float)new_h / region_layer_net_h);
        b.w *= (float)region_layer_net_w / new_w;
        b.h *= (float)region_layer_net_h / new_h;
        boxes[i] = b;
    }
}

static  box get_region_box(float *x, const float *biases, int n, int index, int i, int j, int w, int h, int stride)
{
    volatile box b;

    b.x = (i + x[index + 0 * stride]) / w;
    b.y = (j + x[index + 1 * stride]) / h;
    b.w = expf(x[index + 2 * stride]) * biases[2 * n] / w;
    b.h = expf(x[index + 3 * stride]) * biases[2 * n + 1] / h;
    return b;
}

static  void get_region_boxes(float *predictions, float **probs, box *boxes)
{
    for (int i = 0; i < region_layer_l_w * region_layer_l_h; ++i) {
        volatile int row = i / region_layer_l_w;
        volatile int col = i % region_layer_l_w;

        for (int n = 0; n < region_layer_l_n; ++n) {
            int index = n * region_layer_l_w * region_layer_l_h + i;

            for (int j = 0; j < region_layer_l_classes; ++j)
                probs[index][j] = 0;
            int obj_index = entry_index(n * region_layer_l_w * region_layer_l_h + i, 4);
            int box_index = entry_index(n * region_layer_l_w * region_layer_l_h + i, 0);
            float scale  = predictions[obj_index];

            boxes[index] = get_region_box(
                predictions, l_biases, n, box_index, col, row,
                region_layer_l_w, region_layer_l_h,
                region_layer_l_w * region_layer_l_h);

            float max = 0;

            for (int j = 0; j < region_layer_l_classes; ++j) {
                int class_index = entry_index(n * region_layer_l_w * region_layer_l_h + i, 5 + j);
                float prob = scale * predictions[class_index];

                probs[index][j] = (prob > region_layer_thresh) ? prob : 0;
                if (prob > max)
                    max = prob;
            }
            probs[index][region_layer_l_classes] = max;
        }
    }
    correct_region_boxes(boxes);
}

static  int nms_comparator(const void *pa, const void *pb)
{
    volatile sortable_bbox a = *(sortable_bbox *)pa;
    volatile sortable_bbox b = *(sortable_bbox *)pb;
    volatile float diff = a.probs[a.index][b.class] - b.probs[b.index][b.class];

    if (diff < 0)
        return 1;
    else if (diff > 0)
        return -1;
    return 0;
}

static  float overlap(float x1, float w1, float x2, float w2)
{
    float l1 = x1 - w1/2;
    float l2 = x2 - w2/2;
    float left = l1 > l2 ? l1 : l2;
    float r1 = x1 + w1/2;
    float r2 = x2 + w2/2;
    float right = r1 < r2 ? r1 : r2;

    return right - left;
}

static  float box_intersection(box a, box b)
{
    float w = overlap(a.x, a.w, b.x, b.w);
    float h = overlap(a.y, a.h, b.y, b.h);

    if (w < 0 || h < 0)
        return 0;
    return w * h;
}

static  float box_union(box a, box b)
{
    float i = box_intersection(a, b);
    float u = a.w * a.h + b.w * b.h - i;

    return u;
}

static  float box_iou(box a, box b)
{
    return box_intersection(a, b)/box_union(a, b);
}

static  void do_nms_sort(box *boxes, float **probs)
{
    int i, j, k;
    sortable_bbox s[region_layer_boxes];

    for (i = 0; i < region_layer_boxes; ++i) {
        s[i].index = i;
        s[i].class = 0;
        s[i].probs = probs;
    }

    for (k = 0; k < region_layer_l_classes; ++k) {
        for (i = 0; i < region_layer_boxes; ++i)
            s[i].class = k;
        qsort(s, region_layer_boxes, sizeof(sortable_bbox), nms_comparator);
        for (i = 0; i < region_layer_boxes; ++i) {
            if (probs[s[i].index][k] == 0)
                continue;
            volatile box a = boxes[s[i].index];

            for (j = i+1; j < region_layer_boxes; ++j) {
                volatile box b = boxes[s[j].index];

                if (box_iou(a, b) > region_layer_nms)
                    probs[s[j].index][k] = 0;
            }
        }
    }
}

static  int max_index(float *a, int n)
{
    int i, max_i = 0;
    float max = a[0];

    for (i = 1; i < n; ++i) {
        if (a[i] > max) {
            max   = a[i];
            max_i = i;
        }
    }
    return max_i;
}

void region_layer_cal(INPUT_TYPE *u8in)
{
    forward_region_layer(u8in, output);

    for (int i = 0; i < region_layer_boxes; i++)
    {
        probs[i] = &(probs_buf[i * (region_layer_l_classes + 1)]);
    }

    get_region_boxes(output, probs, boxes);
    do_nms_sort(boxes, probs);
}

void region_layer_draw_boxes(callback_draw_box callback)
{
    for (int i = 0; i < region_layer_boxes; ++i) {
        volatile int class  = max_index(probs[i], region_layer_l_classes);
        volatile float prob = probs[i][class];

        if (prob > region_layer_thresh) {
            volatile box *b      = boxes + i;
            uint32_t x1 = b->x * region_layer_img_w -
                      (b->w * region_layer_img_w / 2);
            uint32_t y1 = b->y * region_layer_img_h -
                      (b->h * region_layer_img_h / 2);
            uint32_t x2 = b->x * region_layer_img_w +
                      (b->w * region_layer_img_w / 2);
            uint32_t y2 = b->y * region_layer_img_h +
                      (b->h * region_layer_img_h / 2);

            callback(x1, y1, x2, y2, class, prob);
        }
    }
}
