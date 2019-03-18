#ifndef _WS2812_H
#define _WS2812_H
#include "stddef.h"
#include "dmac.h"
#include "stdbool.h"

typedef struct _WS2812_DATA {
	uint32_t blue : 8;
	uint32_t red : 8;
	uint32_t green : 8;
	uint32_t reserved : 8;
} __attribute__((packed, aligned(4))) ws2812_data;

typedef struct _WS2812_INFO {
	size_t ws_num;
	ws2812_data *ws_buf;
} ws2812_info;

bool ws2812_send_data(uint32_t spi_num, dmac_channel_number_t DMAC_NUM, ws2812_info *ws);
ws2812_info *ws2812_get_buf(uint32_t num);
bool ws2812_release_buf(ws2812_info *ws);
bool ws2812_clear(ws2812_info *ws);
bool ws2812_set_data(ws2812_info *ws, uint32_t num, uint8_t r, uint8_t g, uint8_t b);

#endif

