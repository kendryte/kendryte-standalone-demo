#include "ws2812.h"
#include <stdio.h>
#include <sysctl.h>
#include <bsp.h>
#include <uarths.h>
#include <fpioa.h>

ws2812_info * ws_info;

int main(void)
{
    sysctl_cpu_set_freq(400000000UL);
    uarths_init();
    fpioa_set_function(40, FUNC_SPI0_D0);

    ws_info = ws2812_get_buf(16);
    while(1)
    {
        ws2812_set_data(ws_info, 0, 0xFF, 0, 0);
        ws2812_send_data(0, 1, ws_info);
        sleep(1);
        ws2812_set_data(ws_info, 0, 0, 0xFF, 0);
        ws2812_send_data(0, 1, ws_info);
        sleep(1);
        ws2812_set_data(ws_info, 0, 0, 0, 0xFF);
        ws2812_send_data(0, 1, ws_info);
        sleep(1);
        ws2812_set_data(ws_info, 0, 0xFF, 0xFF, 0);
        ws2812_send_data(0, 1, ws_info);
        sleep(1);
    }
}
