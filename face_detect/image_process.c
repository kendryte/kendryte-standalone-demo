/* Copyright 2018 Canaan Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "image_process.h"

int image_init(image_t *image)
{
    image->addr = malloc(image->width * image->height * image->pixel);
    if (image->addr == NULL)
        return -1;
    return 0;
}

void image_deinit(image_t *image)
{
    free(image->addr);
}

void image_crop(image_t *image_src, image_t *image_dst, uint16_t x_offset, uint16_t y_offset)
{
    uint8_t *src, *r_src, *g_src, *b_src, *dst, *r_dst, *g_dst, *b_dst;
    uint16_t w_src, h_src, w_dst, h_dst;

    src = image_src->addr;
    w_src = image_src->width;
    h_src = image_src->height;
    dst = image_dst->addr;
    w_dst = image_dst->width;
    h_dst = image_dst->height;

    r_src = src + y_offset * w_src + x_offset;
    g_src = r_src + w_src * h_src;
    b_src = g_src + w_src * h_src;
    r_dst = dst;
    g_dst = r_dst + w_dst * h_dst;
    b_dst = g_dst + w_dst * h_dst;

    for (uint16_t y = 0; y < h_dst; y++)
    {
        for (uint16_t x = 0; x < w_dst; x++)
        {
            *r_dst++ = r_src[x];
            *g_dst++ = g_src[x];
            *b_dst++ = b_src[x];
        }
        r_src += w_src;
        g_src += w_src;
        b_src += w_src;
    }
}

void image_resize(image_t *image_src, image_t *image_dst)
{
    uint16_t x1, x2, y1, y2;
    double w_scale, h_scale;
    double temp1, temp2;
    double x_src, y_src;

    uint8_t *r_src, *g_src, *b_src, *r_dst, *g_dst, *b_dst;
    uint16_t w_src, h_src, w_dst, h_dst;

    w_src = image_src->width;
    h_src = image_src->height;
    r_src = image_src->addr;
    g_src = r_src + w_src * h_src;
    b_src = g_src + w_src * h_src;
    w_dst = image_dst->width;
    h_dst = image_dst->height;
    r_dst = image_dst->addr;
    g_dst = r_dst + w_dst * h_dst;
    b_dst = g_dst + w_dst * h_dst;

    w_scale = (double)w_src / w_dst;
    h_scale = (double)h_src / h_dst;

    for (uint16_t y = 0; y < h_dst; y++)
    {
        for (uint16_t x = 0; x < w_dst; x++)
        {
            x_src = (x + 0.5) * w_scale - 0.5;
            x1 = (uint16_t)x_src;
            x2 = x1 + 1;
            y_src = (y + 0.5) * h_scale - 0.5;
            y1 = (uint16_t)y_src;
            y2 = y1 + 1;

            if (x2 >= w_src || y2 >= h_src)
            {   
                *(r_dst + x + y * w_dst) = *(r_src + x1 + y1 * w_src);
                *(g_dst + x + y * w_dst) = *(g_src + x1 + y1 * w_src);
                *(b_dst + x + y * w_dst) = *(b_src + x1 + y1 * w_src);
                continue;
            }

            temp1 = (x2 - x_src) * *(r_src + x1 + y1 * w_src) + (x_src - x1) * *(r_src + x2 + y1 * w_src);
            temp2 = (x2 - x_src) * *(r_src + x1 + y2 * w_src) + (x_src - x1) * *(r_src + x2 + y2 * w_src);
            *(r_dst + x + y * w_dst) = (uint8_t)((y2 - y_src) * temp1 + (y_src - y1) * temp2);
            temp1 = (x2 - x_src) * *(g_src + x1 + y1 * w_src) + (x_src - x1) * *(g_src + x2 + y1 * w_src);
            temp2 = (x2 - x_src) * *(g_src + x1 + y2 * w_src) + (x_src - x1) * *(g_src + x2 + y2 * w_src);
            *(g_dst + x + y * w_dst) = (uint8_t)((y2 - y_src) * temp1 + (y_src - y1) * temp2);
            temp1 = (x2 - x_src) * *(b_src + x1 + y1 * w_src) + (x_src - x1) * *(b_src + x2 + y1 * w_src);
            temp2 = (x2 - x_src) * *(b_src + x1 + y2 * w_src) + (x_src - x1) * *(b_src + x2 + y2 * w_src);
            *(b_dst + x + y * w_dst) = (uint8_t)((y2 - y_src) * temp1 + (y_src - y1) * temp2);
        }
    }
}

static void svd22(const double a[4], double u[4], double s[2], double v[4])
{
    s[0] = (sqrt(pow(a[0] - a[3], 2) + pow(a[1] + a[2], 2)) + sqrt(pow(a[0] + a[3], 2) + pow(a[1] - a[2], 2))) / 2;
    s[1] = fabs(s[0] - sqrt(pow(a[0] - a[3], 2) + pow(a[1] + a[2], 2)));
    v[2] = (s[0] > s[1]) ? sin((atan2(2 * (a[0] * a[1] + a[2] * a[3]), a[0] * a[0] - a[1] * a[1] + a[2] * a[2] - a[3] * a[3])) / 2) : 0;
    v[0] = sqrt(1 - v[2] * v[2]);
    v[1] = -v[2];
    v[3] = v[0];
    u[0] = (s[0] != 0) ? -(a[0] * v[0] + a[1] * v[2]) / s[0] : 1;
    u[2] = (s[0] != 0) ? -(a[2] * v[0] + a[3] * v[2]) / s[0] : 0;
    u[1] = (s[1] != 0) ? (a[0] * v[1] + a[1] * v[3]) / s[1] : -u[2];
    u[3] = (s[1] != 0) ? (a[2] * v[1] + a[3] * v[3]) / s[1] : u[0];
    v[0] = -v[0];
    v[2] = -v[2];
}

static double umeyama_args[] =  
{
#define PIC_SIZE 128
    38.2946*PIC_SIZE/112, 51.6963*PIC_SIZE/112,
    73.5318*PIC_SIZE/112, 51.5014*PIC_SIZE/112,
    56.0252*PIC_SIZE/112, 71.7366*PIC_SIZE/112,
    41.5493*PIC_SIZE/112, 92.3655*PIC_SIZE/112,
    70.7299*PIC_SIZE/112, 92.2041*PIC_SIZE/112
};

void image_umeyama(double *src, double *dst)
{
#define SRC_NUM 5
#define SRC_DIM 2
    int i, j, k;
    double src_mean[SRC_DIM] = { 0.0 };
    double dst_mean[SRC_DIM] = { 0.0 };
    for(i=0; i<SRC_NUM*2; i+=2)
    {
        src_mean[0] += umeyama_args[i];
        src_mean[1] += umeyama_args[i+1];
        dst_mean[0] += src[i];
        dst_mean[1] += src[i+1];
    }
    src_mean[0] /= SRC_NUM;
    src_mean[1] /= SRC_NUM;
    dst_mean[0] /= SRC_NUM;
    dst_mean[1] /= SRC_NUM;

    double src_demean[SRC_NUM][2] = {0.0};
    double dst_demean[SRC_NUM][2] = {0.0};

    for(i=0; i<SRC_NUM; i++)
    {
        src_demean[i][0] = umeyama_args[2*i] - src_mean[0];
        src_demean[i][1] = umeyama_args[2*i+1] - src_mean[1];
        dst_demean[i][0] = src[2*i] - dst_mean[0];
        dst_demean[i][1] = src[2*i+1] - dst_mean[1];
    }

    double A[SRC_DIM][SRC_DIM] = {0.0};
    for(i=0; i<SRC_DIM; i++)
    {
        for(k=0; k<SRC_DIM; k++)
        {
            for(j=0; j<SRC_NUM; j++)
            {
                A[i][k] += dst_demean[j][i]*src_demean[j][k];
            }
            A[i][k] /= SRC_NUM;
        }
    }

    double d[SRC_DIM] = {1, 1};
    double det_A = A[0][0]*A[1][1] - A[1][0]*A[1][0];
    if(det_A < 0)
        d[SRC_DIM-1] = -1;

    double (*T)[SRC_DIM+1] = (double (*)[SRC_DIM+1])dst;
    T[0][0] = 1;
    T[0][1] = 0;
    T[0][2] = 0;
    T[1][0] = 0;
    T[1][1] = 1;
    T[1][2] = 0;
    T[2][0] = 0;
    T[2][1] = 0;
    T[2][2] = 1;

    double U[SRC_DIM][SRC_DIM] = {0};
    double S[SRC_DIM] = {0};
    double V[SRC_DIM][SRC_DIM] = {0};
    svd22(&A[0][0], &U[0][0], S, &V[0][0]);

    double diag_d[SRC_DIM][SRC_DIM] = 
        {
            {1.0, 0.0},
            {0.0, 1.0}
        };
    T[0][0] = U[0][0]*V[0][0] + U[0][1]*V[1][0];
    T[0][1] = U[0][0]*V[0][1] + U[0][1]*V[1][1];
    T[1][0] = U[1][0]*V[0][0] + U[1][1]*V[1][0];
    T[1][1] = U[1][0]*V[0][1] + U[1][1]*V[1][1];

    double scale = 1.0;
    double src_demean_mean[SRC_DIM] = {0.0};
    double src_demean_var[SRC_DIM] = {0.0};
    for(i=0; i<SRC_NUM; i++)
    {
        src_demean_mean[0] += src_demean[i][0];
        src_demean_mean[1] += src_demean[i][1];
    }
    src_demean_mean[0] /= SRC_NUM;
    src_demean_mean[1] /= SRC_NUM;
    
    for(i=0; i<SRC_NUM; i++)
    {
        src_demean_var[0] += (src_demean_mean[0] - src_demean[i][0])*(src_demean_mean[0] - src_demean[i][0]);
        src_demean_var[1] += (src_demean_mean[1] - src_demean[i][1])*(src_demean_mean[1] - src_demean[i][1]);
    }
    src_demean_var[0] /= (SRC_NUM);
    src_demean_var[1] /= (SRC_NUM);
    scale = 1.0 / (src_demean_var[0] + src_demean_var[1]) *(S[0] + S[1]);
    T[0][2] = dst_mean[0] - scale*(T[0][0]*src_mean[0] + T[0][1]*src_mean[1]);
    T[1][2] = dst_mean[1] - scale*(T[1][0]*src_mean[0] + T[1][1]*src_mean[1]);
    T[0][0] *= scale;
    T[0][1] *= scale;
    T[1][0] *= scale;
    T[1][1] *= scale;
}

void image_similarity(image_t *image_src, image_t *image_dst, double *T)
{
    //相似变换
    int width = image_src->width;
    int height = image_src->height;
    int channels = image_src->pixel;
    int step = width;
	int color_step = width * height;
    int sim_step;
    int i, j, k;
	
    //初始化处理后图片的信息
    image_dst->pixel = channels;
    image_dst->width = 128;
    image_dst->height = 128;
	int sim_color_step = image_dst->width * image_dst->height;
    sim_step = image_dst->width;
    image_dst->addr = malloc(image_dst->width*image_dst->height*image_dst->pixel);

    //初始化图像
    memset(image_dst->addr, 0, image_dst->width*image_dst->height*image_dst->pixel);

    int pre_x, pre_y, after_x, after_y;//缩放前对应的像素点坐标
    int x, y;
    unsigned short color[2][2];
    double (*TT)[3] = (double (*)[3])T;
    for(i = 0;i < image_dst->height;i++)
    {
        for(j = 0;j < image_dst->width;j++)
        {
            pre_x = (int)(TT[0][0] * j + TT[0][1] * i + TT[0][2]);
            pre_y = (int)(TT[1][0] * j + TT[1][1] * i + TT[1][2]);

            pre_x <<= 8;
            pre_y <<= 8;
            y = pre_y & 0xFF;
            x = pre_x & 0xFF;
            pre_x >>= 8;
            pre_y >>= 8;
            if(pre_x<0 || pre_x > (width-1) || pre_y<0 || pre_y>(height-1))
				    continue;
            for(k=0; k<channels; k++)
            {
                color[0][0] = image_src->addr[pre_y*step + pre_x + k * color_step];
                color[1][0] = image_src->addr[pre_y*step + (pre_x + 1) + k * color_step];
                color[0][1] = image_src->addr[(pre_y + 1)*step + pre_x + k * color_step];
                color[1][1] = image_src->addr[(pre_y + 1)*step + (pre_x + 1) + k * color_step];
                int final = (0x100 - x)*(0x100 - y)*color[0][0] + x * (0x100 - y)*color[1][0] + (0x100 - x)*y*color[0][1] + x * y*color[1][1];
                final = final >> 16;
                image_dst->addr[i * sim_step + j + k*sim_color_step] = final;
            }
        }
    }
}
