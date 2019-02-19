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

#include <stdio.h>
#include "spi_slave.h"
#include "spi.h"
#include "sysctl.h"
#include "fpioa.h"

struct slave_info_t spi_slave_device;
spi_slave_handler_t spi_slave_handler;

void spi_slave_receive(uint32_t data)
{
    if(data & 0x40000000)
    {
        if (spi_slave_device.acces_reg < SLAVE_MAX_ADDR)
            spi_slave_device.reg_data[spi_slave_device.acces_reg] = data & 0x3FFFFFFF;
        spi_slave_device.acces_reg = SLAVE_MAX_ADDR;
    }
    else
    {
        spi_slave_device.acces_reg = (data & 0x3FFFFFFF);
    }
}

uint32_t spi_slave_transmit(uint32_t data)
{
    uint32_t ret = 0;
    spi_slave_device.acces_reg = (data & 0x3FFFFFFF);
    if (spi_slave_device.acces_reg < SLAVE_MAX_ADDR)
        ret = spi_slave_device.reg_data[spi_slave_device.acces_reg];
    else
        ret = 0xFF;
    spi_slave_device.acces_reg = SLAVE_MAX_ADDR;
    return ret;
}

spi_slave_event_t spi_slave_event(uint32_t data)
{
    if(data & 0x80000000)
        return SPI_EV_RECV;
    else
        return SPI_EV_TRANS;
}

void spi_slave_init(void)
{
    spi_slave_handler.on_event = spi_slave_event,
    spi_slave_handler.on_receive = spi_slave_receive,
    spi_slave_handler.on_transmit = spi_slave_transmit,

    spi_slave_config(32, &spi_slave_handler);
}

void spi_master_init(void)
{
    spi_init(0, SPI_WORK_MODE_0, SPI_FF_STANDARD, 32, 0);
    spi_set_clk_rate(0, 23000000);
}

void spi_write_reg(uint32_t reg, uint32_t data)
{
    uint32_t reg_value = reg | 0x80000000;
    uint32_t data_value = data | 0xc0000000;
    spi_send_data_standard(0, 0, (const uint8_t *)&reg_value, 4, (const uint8_t *)&data_value, 4);
}

uint32_t spi_read_reg(uint32_t reg)
{
    uint32_t value = 0;

    uint32_t reg_value = reg & 0x7FFFFFFF;

    spi_send_data_standard(0, 0, (const uint8_t *)&reg_value, 4, NULL, 0);

    fpioa_set_function(36, FUNC_SPI0_D1);
    fpioa_set_function(38, FUNC_SPI0_D0);
    spi_receive_data_standard(0, 0,  NULL, 0, (uint8_t *)&value, 4);
    fpioa_set_function(36, FUNC_SPI0_D0);
    fpioa_set_function(38, FUNC_SPI0_D1);
	return value;
}
