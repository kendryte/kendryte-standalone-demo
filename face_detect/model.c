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
#include "model.h"
#include "w25qxx.h"

static model_config_t model_cfg;

static void model_read(uint32_t addr, uint8_t *buff, uint32_t len)
{
        w25qxx_read_data(addr, buff, len, W25QXX_QUAD_FAST);
}

void* model_init(uint32_t addr)
{
        uint32_t layer_cfg_addr;
        layer_config_t layer_cfg;
        kpu_layer_argument_t *layer_arg_ptr;
        void *ptr;

        model_read(addr, (uint8_t *)&model_cfg, sizeof(model_config_t));
        if (model_cfg.magic_number != 0x12345678)
                return NULL;
        layer_arg_ptr = (kpu_layer_argument_t *)malloc(12 * 8 * model_cfg.layer_number);
        if (layer_arg_ptr == NULL)
                return NULL;
        layer_cfg_addr = addr + model_cfg.layer_cfg_addr_offset;
        for (uint32_t i = 0; i < model_cfg.layer_number; i++) {
                // read layer config
                model_read(layer_cfg_addr, (uint8_t *)&layer_cfg, sizeof(layer_config_t));
                // read reg arg
                model_read(addr + layer_cfg.reg_addr_offset, (uint8_t *)layer_arg_ptr, sizeof(kpu_layer_argument_t));
                // read act arg
                ptr = malloc(sizeof(kpu_activate_table_t));
                if (ptr == NULL)
                        return NULL;
                model_read(addr + layer_cfg.act_addr_offset, (uint8_t *)ptr, sizeof(kpu_activate_table_t));
                layer_arg_ptr->kernel_calc_type_cfg.data.active_addr = (uint32_t)ptr;
                // read bn arg
                ptr = malloc(layer_cfg.bn_len);
                if (ptr == NULL)
                        return NULL;
                model_read(addr + layer_cfg.bn_addr_offset, (uint8_t *)ptr, layer_cfg.bn_len);
                layer_arg_ptr->kernel_pool_type_cfg.data.bwsx_base_addr = (uint32_t)ptr;
                // read weights arg
                ptr = malloc(layer_cfg.weights_len);
                if (ptr == NULL)
                        return NULL;
                model_read(addr + layer_cfg.weights_addr_offset, (uint8_t *)ptr, layer_cfg.weights_len);
                layer_arg_ptr->kernel_load_cfg.data.para_start_addr = (uint32_t)ptr;
                // next layer
                layer_cfg_addr += sizeof(layer_config_t);
                layer_arg_ptr++;
        }
        layer_arg_ptr -= model_cfg.layer_number;
        return layer_arg_ptr;
}

int kpu_task_init(kpu_task_t* task)
{
        task->layers = model_init(0x00800000);
	if (task->layers == NULL)
		return 1;
        task->layers_length = model_cfg.layer_number;
        task->eight_bit_mode = 0;
        return 0;
}