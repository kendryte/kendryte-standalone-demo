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

struct slave_info_t slave_device;

int on_irq_spi_slave(void *ctx);

void spi_slave_init(void)
{
    sysctl_reset(SYSCTL_RESET_SPI2);
    sysctl_clock_enable(SYSCTL_CLOCK_FPIOA);
    sysctl_clock_enable(SYSCTL_CLOCK_SPI2);
    sysctl_clock_set_threshold(SYSCTL_THRESHOLD_SPI2, 0);

    spi[2]->ssienr = 0x00;
    spi[2]->ctrlr0 = 0x001f0600;
    spi[2]->txftlr = 0x00000000;
    spi[2]->rxftlr = 0x00000000;
    spi[2]->imr = 0x00000010;
    spi[2]->ssienr = 0x01;

    plic_set_priority(IRQN_SPI_SLAVE_INTERRUPT, 1);
    plic_irq_enable(IRQN_SPI_SLAVE_INTERRUPT);
    plic_irq_register(IRQN_SPI_SLAVE_INTERRUPT, on_irq_spi_slave, NULL);
}

int on_irq_spi_slave(void *ctx)
{
    uint8_t isr = spi[2]->isr;
    uint32_t data;

    if (isr & 0x10)
    {
        data = spi[2]->dr[0];
        if (data & 0x80000000)
        { // cmd
            slave_device.acces_reg = (data & 0x3FFFFFFF);
            if (data & 0x40000000)
            { // read
                spi[2]->ssienr = 0x00;
                spi[2]->ctrlr0 = 0x001f0100;
                spi[2]->ssienr = 0x01;
                if (slave_device.acces_reg < SLAVE_MAX_ADDR)
                    data = slave_device.reg_data[slave_device.acces_reg];
                else
                    data = 0xFF;
                spi[2]->dr[0] = data;
                spi[2]->dr[0] = data;
                spi[2]->imr = 0x00000001;
                slave_device.acces_reg = SLAVE_MAX_ADDR;
            }
        }
        else
        { // data
            if (slave_device.acces_reg < SLAVE_MAX_ADDR)
                slave_device.reg_data[slave_device.acces_reg] = data;
            slave_device.acces_reg = SLAVE_MAX_ADDR;
        }
    }
    if (isr & 0x01)
    {
        spi[2]->ssienr = 0x00;
        spi[2]->ctrlr0 = 0x001f0600;
        spi[2]->imr = 0x00000010;
        spi[2]->ssienr = 0x01;
    }
    return 0;
}

void spi_master_init(void)
{
    spi_init(0, SPI_WORK_MODE_0, SPI_FF_STANDARD, 32, 0);
    spi_set_clk_rate(0, 23000000);
}

void spi_write_reg(uint32_t reg, uint32_t data)
{
    uint32_t reg_value = reg | 0x80000000;
    uint32_t data_value = data;
    spi_send_data_standard(0, 0, (const uint8_t *)&reg_value, 4, (const uint8_t *)&data_value, 4);
}

uint32_t spi_read_reg(uint32_t reg)
{
    uint32_t value = 0;

    uint32_t reg_value = reg | 0xc0000000;

    spi_send_data_standard(0, 0, (const uint8_t *)&reg_value, 4, NULL, 0);

    spi_receive_data_standard(0, 0,  NULL, 0, (uint8_t *)&value, 4);
	return value;
}
