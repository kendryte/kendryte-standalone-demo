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
#include <string.h>
#include "uart.h"
#include "gpiohs.h"
#include "sysctl.h"
#include <unistd.h>

#define CMD_LENTH  4

#define CLOSLIGHT   0x55555555
#define OPENLIGHT   0xAAAAAAAA

#define UART_NUM    UART_DEVICE_1

uint8_t recv_buf[48];
#define RECV_DMA_LENTH  6

volatile uint32_t recv_flag = 0;
char g_cmd[4];
volatile uint8_t g_cmd_cnt = 0;

int release_cmd(char *cmd)
{
    switch(*((int *)cmd)){
        case CLOSLIGHT:
        gpiohs_set_pin(3, GPIO_PV_LOW);
        break;
        case OPENLIGHT:
        gpiohs_set_pin(3, GPIO_PV_HIGH);
        break;
    }
    return 0;
}

volatile uint32_t g_uart_send_flag = 0;

int uart_send_done(void *ctx)
{
    g_uart_send_flag = 1;
    return 0;
}

int uart_recv_done(void *ctx)
{
    uint8_t *v_dest = ctx + RECV_DMA_LENTH;
    if(v_dest >= recv_buf + 48)
        v_dest = recv_buf;
    uart_receive_data_dma_irq(UART_NUM, DMAC_CHANNEL1, v_dest, RECV_DMA_LENTH, uart_recv_done, v_dest, 2);
    uint8_t *v_buf = (uint8_t *)ctx;
    for(uint32_t i = 0; i < RECV_DMA_LENTH; i++)
    {
        if(v_buf[i] == 0x55 && (recv_flag == 0 || recv_flag == 1))
        {
            recv_flag = 1;
            continue;
        }
        else if(v_buf[i] == 0xAA && recv_flag == 1)
        {
            recv_flag = 2;
            g_cmd_cnt = 0;
            continue;
        }
        else if(recv_flag == 2 && g_cmd_cnt < CMD_LENTH)
        {
            g_cmd[g_cmd_cnt++] = v_buf[i];
            if(g_cmd_cnt >= CMD_LENTH)
            {
                release_cmd(g_cmd);
                recv_flag = 0;
            }
            continue;
        }
        else
        {
            recv_flag = 0;
        }
    }

    return 0;
}

void io_mux_init(void)
{
    fpioa_set_function(4, FUNC_UART1_RX + UART_NUM * 2);
    fpioa_set_function(5, FUNC_UART1_TX + UART_NUM * 2);
    fpioa_set_function(24, FUNC_GPIOHS3);
}

int main(void)
{
    io_mux_init();
    dmac_init();
    plic_init();
    sysctl_enable_irq();

    gpiohs_set_drive_mode(3, GPIO_DM_OUTPUT);
    gpio_pin_value_t value = GPIO_PV_HIGH;
    gpiohs_set_pin(3, value);

    uart_init(UART_NUM);
    uart_config(UART_NUM, 115200, 8, UART_STOP_1, UART_PARITY_NONE);

    char *hel = {"hello!\n"};
    uart_send_data_dma_irq(UART_NUM, DMAC_CHANNEL0, (uint8_t *)hel, strlen(hel), uart_send_done, NULL, 1);

    uart_receive_data_dma_irq(UART_NUM, DMAC_CHANNEL1, recv_buf, RECV_DMA_LENTH, uart_recv_done, recv_buf, 2);

    while(1)
    {
        if(g_uart_send_flag)
        {
            sleep(1);
            uart_send_data_dma_irq(UART_NUM, DMAC_CHANNEL0, (uint8_t *)hel, strlen(hel), uart_send_done, NULL, 1);
            g_uart_send_flag = 0;
        }
    }
}

