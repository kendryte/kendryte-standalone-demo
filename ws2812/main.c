#include <stdint.h>
#include <fpioa.h>
#include <gpiohs.h>
#include <bsp.h>
#include <sysctl.h>
#include <uarths.h>
#include "ws2812.h"

#define WS_GPIOHS_TEST 1
#define WS_SPI_TEST 0

#if WS_GPIOHS_TEST + WS_SPI_TEST != 1
#error set WS_GPIOHS_TEST or WS_SPI_TEST
#endif

#define WS_PIN 40

#if WS_GPIOHS_TEST
#define WS_GPIOHS_NUM 4
#endif

ws2812_info * ws_info;

int main(void)
{
    sysctl_cpu_set_freq(500000000UL);
    uarths_init();

    #if WS_SPI_TEST
    ws2812_init_spi(WS_PIN);
    #endif

    #if WS_GPIOHS_TEST
    ws2812_init_gpiohs(WS_PIN, WS_GPIOHS_NUM);
    #endif

    ws_info = ws2812_get_buf(6);

    while(1)
    {
        #if WS_SPI_TEST
            ws2812_set_data(ws_info, 0, 0xFF, 0, 0);
            ws2812_send_data_spi(0, 1, ws_info);
            sleep(1);
            ws2812_set_data(ws_info, 0, 0, 0xFF, 0);
            ws2812_send_data_spi(0, 1, ws_info);
            sleep(1);
            ws2812_set_data(ws_info, 0, 0, 0, 0xFF);
            ws2812_send_data_spi(0, 1, ws_info);
            sleep(1);
            ws2812_set_data(ws_info, 0, 0xFF, 0xFF, 0);
            ws2812_send_data_spi(0, 1, ws_info);
            sleep(1);
        #endif
        #if WS_GPIOHS_TEST
            ws2812_set_data(ws_info, 0, 0xFF, 0, 0);
            ws2812_send_data_gpiohs(WS_GPIOHS_NUM, ws_info);
            sleep(1);
            ws2812_set_data(ws_info, 0, 0, 0xFF, 0);
            ws2812_send_data_gpiohs(WS_GPIOHS_NUM, ws_info);
            sleep(1);
            ws2812_set_data(ws_info, 0, 0, 0, 0xFF);
            ws2812_send_data_gpiohs(WS_GPIOHS_NUM, ws_info);
            sleep(1);
            ws2812_set_data(ws_info, 0, 0xFF, 0xFF, 0);
            ws2812_send_data_gpiohs(WS_GPIOHS_NUM, ws_info);
            sleep(1);
        #endif
    }
}

