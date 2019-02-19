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

#ifndef _SPI_SLAVE_H
#define _SPI_SLAVE_H

#include <stdint.h>

#define SPI_MASTER

#define SLAVE_MAX_ADDR 15

struct slave_info_t
{
    uint32_t acces_reg;
    uint32_t reg_data[SLAVE_MAX_ADDR];
};

void spi_master_init(void);
void spi_slave_init(void);

void spi_write_reg(uint32_t reg, uint32_t data);
uint32_t spi_read_reg(uint32_t reg);

#endif
