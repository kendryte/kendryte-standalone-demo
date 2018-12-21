#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "dvp.h"
#include "fpioa.h"
#include "lcd.h"
#include "board_config.h"
#if OV5640
#include "ov5640.h"
#endif
#if OV2640
#include "ov2640.h"
#endif
#include "plic.h"
#include "sysctl.h"
#include "uarths.h"
#include "nt35310.h"
#include "utils.h"
#include "kpu.h"
#include "region_layer.h"
#include "gencode_output.h"

#define PLL0_OUTPUT_FREQ 800000000UL
#define PLL1_OUTPUT_FREQ 300000000UL

#define CLASS_NUMBER 1

kpu_task_t task;
static region_layer_t detect_rl;

volatile uint8_t g_ai_done_flag;

static int ai_done(void *ctx)
{
    g_ai_done_flag = 1;
    return 0;
}

uint32_t g_lcd_gram0[38400] __attribute__((aligned(64)));

uint8_t g_ai_buf[320 * 240 *3] __attribute__((aligned(128)));

#define ANCHOR_NUM 5

float g_anchor[ANCHOR_NUM * 2] = {0.57273, 0.677385, 1.87446, 2.06253, 3.33843, 5.47434, 7.88282, 3.52778, 9.77052, 9.16828};

volatile uint8_t g_dvp_finish_flag = 0;
volatile uint8_t g_ram_mux = 0;

static int on_irq_dvp(void* ctx)
{
    if (dvp_get_interrupt(DVP_STS_FRAME_FINISH))
    {
        /* switch gram */
        dvp_clear_interrupt(DVP_STS_FRAME_FINISH);
        g_dvp_finish_flag = 1;
    }
    else
    {
        if(g_dvp_finish_flag == 0)
            dvp_start_convert();
        dvp_clear_interrupt(DVP_STS_FRAME_START);
    }

    return 0;
}

#if BOARD_LICHEEDAN
static void io_mux_init(void)
{
    /* Init DVP IO map and function settings */
    fpioa_set_function(42, FUNC_CMOS_RST);
    fpioa_set_function(44, FUNC_CMOS_PWDN);
    fpioa_set_function(46, FUNC_CMOS_XCLK);
    fpioa_set_function(43, FUNC_CMOS_VSYNC);
    fpioa_set_function(45, FUNC_CMOS_HREF);
    fpioa_set_function(47, FUNC_CMOS_PCLK);
    fpioa_set_function(41, FUNC_SCCB_SCLK);
    fpioa_set_function(40, FUNC_SCCB_SDA);

    /* Init SPI IO map and function settings */
    fpioa_set_function(38, FUNC_GPIOHS0 + DCX_GPIONUM);
    fpioa_set_function(36, FUNC_SPI0_SS3);
    fpioa_set_function(39, FUNC_SPI0_SCLK);

    sysctl_set_spi0_dvp_data(1);
}

static void io_set_power(void)
{
    /* Set dvp and spi pin to 1.8V */
    sysctl_set_power_mode(SYSCTL_POWER_BANK6, SYSCTL_POWER_V18);
    sysctl_set_power_mode(SYSCTL_POWER_BANK7, SYSCTL_POWER_V18);
}

#else
static void io_mux_init(void)
{
    /* Init DVP IO map and function settings */
    fpioa_set_function(11, FUNC_CMOS_RST);
    fpioa_set_function(13, FUNC_CMOS_PWDN);
    fpioa_set_function(14, FUNC_CMOS_XCLK);
    fpioa_set_function(12, FUNC_CMOS_VSYNC);
    fpioa_set_function(17, FUNC_CMOS_HREF);
    fpioa_set_function(15, FUNC_CMOS_PCLK);
    fpioa_set_function(10, FUNC_SCCB_SCLK);
    fpioa_set_function(9, FUNC_SCCB_SDA);

    /* Init SPI IO map and function settings */
    fpioa_set_function(8, FUNC_GPIOHS0 + DCX_GPIONUM);
    fpioa_set_function(6, FUNC_SPI0_SS3);
    fpioa_set_function(7, FUNC_SPI0_SCLK);

    sysctl_set_spi0_dvp_data(1);
}

static void io_set_power(void)
{
    /* Set dvp and spi pin to 1.8V */
    sysctl_set_power_mode(SYSCTL_POWER_BANK1, SYSCTL_POWER_V18);
    sysctl_set_power_mode(SYSCTL_POWER_BANK2, SYSCTL_POWER_V18);
}
#endif


static void drawboxes(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t class, float prob)
{
    if (x1 >= 320)
        x1 = 319;
    if (x2 >= 320)
        x2 = 319;
    if (y1 >= 240)
        y1 = 239;
    if (y2 >= 240)
        y2 = 239;

    lcd_draw_rectangle(x1, y1, x2, y2, 2, RED);
}

int main(void)
{
    /* Set CPU and dvp clk */
    sysctl_pll_set_freq(SYSCTL_PLL0, PLL0_OUTPUT_FREQ);
    sysctl_pll_set_freq(SYSCTL_PLL1, PLL1_OUTPUT_FREQ);
    sysctl_clock_enable(SYSCTL_CLOCK_AI);
    uarths_init();

    io_mux_init();
    io_set_power();
    plic_init();

//    lable_init();

    /* LCD init */
    printf("LCD init\n");
    lcd_init();
#if BOARD_LICHEEDAN
    lcd_set_direction(DIR_YX_RLDU);
#else
    lcd_set_direction(DIR_YX_RLUD);
#endif
    lcd_clear(BLACK);
    lcd_draw_string(136, 70, "DEMO 2", WHITE);
    lcd_draw_string(104, 150, "face detection", WHITE);
    /* DVP init */
    printf("DVP init\n");

    #if OV5640
    dvp_init(16);
    dvp_set_xclk_rate(50000000);
    dvp_enable_burst();
    dvp_set_output_enable(0, 1);
    dvp_set_output_enable(1, 1);
    dvp_set_image_format(DVP_CFG_RGB_FORMAT);
    dvp_set_image_size(320, 240);
    ov5640_init();
    #else
    dvp_init(8);
    dvp_set_xclk_rate(24000000);
    dvp_enable_burst();
    dvp_set_output_enable(0, 1);
    dvp_set_output_enable(1, 1);
    dvp_set_image_format(DVP_CFG_RGB_FORMAT);
s    dvp_set_image_size(320, 240);
    ov2640_init();
    #endif

    dvp_set_ai_addr((uint32_t)g_ai_buf, (uint32_t)(g_ai_buf + 320 * 240), (uint32_t)(g_ai_buf + 320 * 240 * 2));
    dvp_set_display_addr((uint32_t)g_lcd_gram0);
    dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 0);
    dvp_disable_auto();

    /* DVP interrupt config */
    printf("DVP interrupt config\n");
    plic_set_priority(IRQN_DVP_INTERRUPT, 1);
    plic_irq_register(IRQN_DVP_INTERRUPT, on_irq_dvp, NULL);
    plic_irq_enable(IRQN_DVP_INTERRUPT);

    /* enable global interrupt */
    sysctl_enable_irq();

    /* system start */
    printf("system start\n");
    g_ram_mux = 0;
    g_dvp_finish_flag = 0;
    dvp_clear_interrupt(DVP_STS_FRAME_START | DVP_STS_FRAME_FINISH);
    dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 1);

/* init face detect model */
    kpu_task_gencode_output_init(&task);
    task.src = g_ai_buf;
    task.dma_ch = 5;
    task.callback = ai_done;
    kpu_task_init(&task);

    detect_rl.anchor_number = ANCHOR_NUM;
    detect_rl.anchor = g_anchor;
    detect_rl.threshold = 0.75;
    detect_rl.nms_value = 0.3;
    region_layer_init(&detect_rl, &task);
    while (1)
    {
        /* dvp finish*/
        while (g_dvp_finish_flag == 0)
            ;

        /* start to calculate */
        kpu_run(&task);
        while(!g_ai_done_flag);
        g_ai_done_flag = 0;

        /* start region layer */
        region_layer_run(&detect_rl, NULL);

        /* display pic*/
        g_ram_mux ^= 0x01;
        lcd_draw_picture(0, 0, 320, 240, g_lcd_gram0);
        /* draw boxs */
        region_layer_draw_boxes(&detect_rl, drawboxes);
        g_dvp_finish_flag = 0;
    }



    return 0;
}

