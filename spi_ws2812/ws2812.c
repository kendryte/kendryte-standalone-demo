#include "spi.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "sysctl.h"
#include <string.h>
#include <stdbool.h>
#include "ws2812.h"

ws2812_info *ws2812_get_buf(uint32_t num)
{
    ws2812_info *ws = malloc(sizeof(ws2812_info));

    if (ws == NULL)
        return NULL;
    ws->ws_num = num;
    ws->ws_buf = malloc(num * sizeof(ws2812_data));
    if (ws->ws_buf == NULL) {
        free(ws);
        return NULL;
    }
    memset(ws->ws_buf, 0, num * sizeof(ws2812_data));
    return ws;
}

bool ws2812_release_buf(ws2812_info *ws)
{
    if (ws == NULL)
        return false;

    if (ws->ws_buf == NULL)
        return false;

    free(ws->ws_buf);
    free(ws);
    return true;
}

bool ws2812_clear(ws2812_info *ws)
{
    if (ws == NULL)
        return false;

    if (ws->ws_buf == NULL)
        return false;

    memset(ws->ws_buf, 0, ws->ws_num * sizeof(ws2812_data));

    return true;
}

bool ws2812_set_data(ws2812_info *ws, uint32_t num, uint8_t r, uint8_t g, uint8_t b)
{
    if (ws == NULL)
        return false;

    if (ws->ws_buf == NULL)
        return false;
    if (num >= ws->ws_num)
        return false;

    (ws->ws_buf + num)->red = r;
    (ws->ws_buf + num)->green = g;
    (ws->ws_buf + num)->blue = b;

    return true;
}

uint32_t spi_get_rate(uint8_t spi_bus)
{
    volatile spi_t *spi_adapter = spi[spi_bus];
    uint32_t freq_spi_src = sysctl_clock_get_freq(SYSCTL_CLOCK_SPI0 + spi_bus);

    return freq_spi_src / spi_adapter->baudr;
}

bool ws2812_send_data(uint32_t spi_num, dmac_channel_number_t DMAC_NUM, ws2812_info *ws)
{

    uint32_t longbit;
    uint32_t shortbit;
    uint32_t resbit;

    if (ws == NULL)
        return false;

    if (ws->ws_buf == NULL)
        return false;

    size_t ws_cnt = ws->ws_num;
    uint32_t *ws_data = (uint32_t *)ws->ws_buf;

    spi_init(spi_num, SPI_WORK_MODE_0, SPI_FF_STANDARD, 32, 0);

    uint32_t freq_spi = spi_get_rate(spi_num);
    double clk_time = 1e9 / freq_spi;   // ns per clk

    uint32_t longtime = 850 / clk_time * clk_time;

    if (longtime < 700)
        longbit = 850 / clk_time + 1;
    else
        longbit = 850 / clk_time;

    uint32_t shortime = 400 / clk_time * clk_time;

    if (shortime < 250)
        shortbit = 400 / clk_time + 1;
    else
        shortbit = 400 / clk_time;

    resbit = (400000 / clk_time);   // > 300us

    uint32_t spi_send_cnt = (((ws_cnt * 24 * (longbit + shortbit) + resbit) / 8) + 7) / 4; //加最后的reset
    uint32_t reset_cnt = (resbit + 7) / 8 / 4;      //先reset
    uint32_t *tmp_spi_data = malloc((spi_send_cnt + reset_cnt) * 4);

    if (tmp_spi_data == NULL)
        return false;

    memset(tmp_spi_data, 0, (spi_send_cnt + reset_cnt) * 4);
    uint32_t *spi_data = tmp_spi_data;

    spi_data += reset_cnt;
    int pos = 31;
    uint32_t long_cnt = longbit;
    uint32_t short_cnt = shortbit;

    for (int i = 0; i < ws_cnt; i++) {

        for (uint32_t mask = 0x800000; mask > 0; mask >>= 1) {

            if (ws_data[i] & mask) {

                while (long_cnt--) {
                    *(spi_data) |= (1 << (pos--));
                    if (pos < 0) {
                        spi_data++;
                        pos = 31;
                    }
                }
                long_cnt = longbit;

                while (short_cnt--) {
                    *(spi_data) &= ~(1 << (pos--));
                    if (pos < 0) {
                        spi_data++;
                        pos = 31;
                    }
                }
                short_cnt = shortbit;

            } else{
                while (short_cnt--) {
                    *(spi_data) |= (1 << (pos--));
                    if (pos < 0) {
                        spi_data++;
                        pos = 31;
                    }
                }
                short_cnt = shortbit;

                while (long_cnt--) {
                    *(spi_data) &= ~(1 << (pos--));
                    if (pos < 0) {
                        spi_data++;
                        pos = 31;
                    }
                }
                long_cnt = longbit;


            }
        }
    }
    spi_send_data_normal_dma(DMAC_NUM, spi_num, 0, tmp_spi_data, spi_send_cnt + reset_cnt, SPI_TRANS_INT);
    free(tmp_spi_data);
    return true;
}

