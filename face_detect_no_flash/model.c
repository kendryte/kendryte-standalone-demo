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
#include "model.h"
#include "w25qxx.h"

typedef struct {
    uint32_t magic_number;
    uint32_t layer_number;
    uint32_t layer_cfg_addr_offset;
    uint32_t eight_bit_mode;
    float scale;
    float bias;
} model_config_t;

typedef struct {
	uint32_t reg_addr_offset;
	uint32_t act_addr_offset;
	uint32_t bn_addr_offset;
	uint32_t bn_len;
	uint32_t weights_addr_offset;
    uint32_t weights_len;
} layer_config_t;

static model_config_t model_cfg;

static void model_read(uint32_t addr, uint8_t *buff, uint32_t len)
{
    w25qxx_read_data(addr, buff, len, W25QXX_QUAD_FAST);
}

int model_init(kpu_task_t *task, uint32_t addr)
{
    uint32_t layer_cfg_addr;
    model_config_t model_cfg;
    layer_config_t layer_cfg;
    kpu_layer_argument_t *layer_arg_ptr;
    void *ptr;

    memset(task, 0, sizeof(kpu_task_t));
    model_read(addr, (uint8_t *)&model_cfg, sizeof(model_config_t));
    if (model_cfg.magic_number != 0x12345678)
        return -1;
    layer_arg_ptr = (kpu_layer_argument_t *)malloc(12 * 8 * model_cfg.layer_number);
    if (layer_arg_ptr == NULL)
        return -2;

    memset(layer_arg_ptr, 0, 12 * 8 * model_cfg.layer_number);
    task->layers = layer_arg_ptr;
    task->layers_length = model_cfg.layer_number;
    task->eight_bit_mode = model_cfg.eight_bit_mode;
    task->output_scale = model_cfg.scale;
    task->output_bias = model_cfg.bias;

    layer_cfg_addr = addr + model_cfg.layer_cfg_addr_offset;
    for (uint32_t i = 0; i < model_cfg.layer_number; i++)
    {
        // read layer config
        model_read(layer_cfg_addr, (uint8_t *)&layer_cfg, sizeof(layer_config_t));
        // read reg arg
        model_read(addr + layer_cfg.reg_addr_offset, (uint8_t *)layer_arg_ptr, sizeof(kpu_layer_argument_t));
        // read act arg
        ptr = malloc(sizeof(kpu_activate_table_t));
        if (ptr == NULL)
            return -2;
        model_read(addr + layer_cfg.act_addr_offset, (uint8_t *)ptr, sizeof(kpu_activate_table_t));
        layer_arg_ptr->kernel_calc_type_cfg.data.active_addr = (uint32_t)ptr;
        // read bn arg
        ptr = malloc(layer_cfg.bn_len);
        if (ptr == NULL)
            return -2;
        model_read(addr + layer_cfg.bn_addr_offset, (uint8_t *)ptr, layer_cfg.bn_len);
        layer_arg_ptr->kernel_pool_type_cfg.data.bwsx_base_addr = (uint32_t)ptr;
        // read weights arg
        ptr = malloc(layer_cfg.weights_len);
        if (ptr == NULL)
            return -2;
        model_read(addr + layer_cfg.weights_addr_offset, (uint8_t *)ptr, layer_cfg.weights_len);
        layer_arg_ptr->kernel_load_cfg.data.para_start_addr = (uint32_t)ptr;
        // next layer
        layer_cfg_addr += sizeof(layer_config_t);
        layer_arg_ptr++;
    }
    return 0;
}

int model_deinit(kpu_task_t *task)
{
    for (uint32_t i = 0; i < task->layers_length; i++)
    {
        free(task->layers[i].kernel_calc_type_cfg.data.active_addr);
        free(task->layers[i].kernel_pool_type_cfg.data.bwsx_base_addr);
        free(task->layers[i].kernel_load_cfg.data.para_start_addr);
    }
    free(task->layers);
    return 0;
}
