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
#ifndef _MODEL_H
#define _MODEL_H

#include <stdint.h>
#include <kpu.h>

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

void* model_init(uint32_t addr);
int kpu_task_init(kpu_task_t* task);

#endif /* _MODEL_H */

