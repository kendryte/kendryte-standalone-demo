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
#include "fpioa.h"
#include "sysctl.h"
#include "w25qxx.h"

#define TEST_NUMBER (256)
#define DATA_ADDRESS 0x400000
uint8_t data_buf[TEST_NUMBER];

void io_mux_init(uint8_t index)
{

    if (index == 0)
    {
        fpioa_set_function(30, FUNC_SPI0_SS0);
        fpioa_set_function(32, FUNC_SPI0_SCLK);
        fpioa_set_function(34, FUNC_SPI0_D0);
        fpioa_set_function(13, FUNC_SPI0_D1);
        fpioa_set_function(15, FUNC_SPI0_D2);
        fpioa_set_function(17, FUNC_SPI0_D3);
    }
    else
    {
        fpioa_set_function(30, FUNC_SPI1_SS0);
        fpioa_set_function(32, FUNC_SPI1_SCLK);
        fpioa_set_function(34, FUNC_SPI1_D0);
        fpioa_set_function(13, FUNC_SPI1_D1);
        fpioa_set_function(15, FUNC_SPI1_D2);
        fpioa_set_function(17, FUNC_SPI1_D3);
    }
}

int main(void)
{
    uint8_t manuf_id, device_id;
    uint32_t index, spi_index;
    spi_index = 3;
    printf("spi%d master test\n", spi_index);
    io_mux_init(spi_index);

    w25qxx_init_dma(spi_index, 0);

    w25qxx_enable_quad_mode_dma();

    w25qxx_read_id_dma(&manuf_id, &device_id);
    printf("manuf_id:0x%02x, device_id:0x%02x\n", manuf_id, device_id);
    if ((manuf_id != 0xEF && manuf_id != 0xC8) || (device_id != 0x17 && device_id != 0x16))
    {
        printf("manuf_id:0x%02x, device_id:0x%02x\n", manuf_id, device_id);
        return 0;
    }
    printf("write data\n");

    for (index = 0; index < TEST_NUMBER; index++)
        data_buf[index] = (uint8_t)(index);

    /*write data*/
    w25qxx_write_data_dma(DATA_ADDRESS, data_buf, TEST_NUMBER);

    uint8_t v_recv_buf[TEST_NUMBER];

    for(index = 0; index < TEST_NUMBER; index++)
        v_recv_buf[index] = 0;

    w25qxx_read_data_dma(DATA_ADDRESS, v_recv_buf, TEST_NUMBER, W25QXX_QUAD_FAST);
    for (index = 0; index < TEST_NUMBER; index++)
    {
        if (v_recv_buf[index] != (uint8_t)(index))
        {
            printf("quad fast read test error\n");
            return 0;
        }
    }

    w25qxx_sector_erase_dma(DATA_ADDRESS);
    while (w25qxx_is_busy_dma() == W25QXX_BUSY)
        ;

    w25qxx_write_data_direct_dma(DATA_ADDRESS, data_buf, TEST_NUMBER);

    for(index = 0; index < TEST_NUMBER; index++)
        v_recv_buf[index] = 0;

    w25qxx_read_data_dma(DATA_ADDRESS, v_recv_buf, TEST_NUMBER, W25QXX_QUAD_FAST);
    for (index = 0; index < TEST_NUMBER; index++)
    {
        if (v_recv_buf[index] != (uint8_t)(index))
        {
            printf("w25qxx_read_data_dma test error\n");
            return 0;
        }
    }

    printf("spi%d master test ok\n", spi_index);
    while (1)
        ;
    return 0;
}
