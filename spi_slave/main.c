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

#include <unistd.h>
#include <stdio.h>
#include "fpioa.h"
#include "plic.h"
#include "sysctl.h"
#include "spi_slave.h"

void io_mux_init(void)
{
    fpioa_set_function(32, FUNC_SPI0_SCLK);
    fpioa_set_function(34, FUNC_SPI0_SS0);
    fpioa_set_function(36, FUNC_SPI0_D0);   //MOSI
    fpioa_set_function(38, FUNC_SPI0_D1);   //MISO

    fpioa_set_function(33, FUNC_SPI_SLAVE_SCLK);
    fpioa_set_function(35, FUNC_SPI_SLAVE_SS);
    fpioa_set_function(37, FUNC_SPI_SLAVE_D0);
}

int main(void)
{
    uint8_t index;
    io_mux_init();
    plic_init();
    sysctl_enable_irq();
    spi_master_init();
    spi_slave_init();

    printf("spi slave test\n");

    printf("spi write\n");
    for (index = 0; index < SLAVE_MAX_ADDR; index++)
        spi_write_reg(index, index+0x10000);

    printf("spi read\n");
    for (index = 0; index < SLAVE_MAX_ADDR; index++)
    {
        if (index+0x10000 != spi_read_reg(index))
        {
            printf("_TEST_FAIL_\n");
            return 0;
        }
    }
    printf("spi slave test ok\n");
    printf("_TEST_PASS_\n");
    while(1)
        ;
    return 0;
}
