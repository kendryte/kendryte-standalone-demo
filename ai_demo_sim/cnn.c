#include "cnn.h"
#include <platform.h>
#include <sysctl.h>
#include <stdio.h>
#include <stdlib.h>
#include "printf.h"
#include "dmac.h"

//#define _DEBUG

volatile cnn_config_t *const cnn = (volatile cnn_config_t *)AI_BASE_ADDR;

#ifdef _DEBUG
#define LOG(fmt, args...) printk(fmt, ## args)
#else
#define LOG(fmt, args...)
#endif

int cnn_print_layer_argument(cnn_layer_argument_t layer_arg, print_cb pout)
{
    pout("layer:\n");

    pout(" interrupt_enabe:\n"
         "  int_en: %d, ram_flag: %d, full_add: %d, depth_wise_layer: %d\n",
         layer_arg.interrupt_enabe.data.int_en,
         layer_arg.interrupt_enabe.data.ram_flag,
         layer_arg.interrupt_enabe.data.full_add,
         layer_arg.interrupt_enabe.data.depth_wise_layer
        );

    pout(" image_addr:\n"
         "  image_src_addr: %p, image_dst_addr: %p\n",
         layer_arg.image_addr.data.image_src_addr*64,
         layer_arg.image_addr.data.image_dst_addr*64
        );

    pout(" image_channel_num:\n"
         "  i_ch_num: %d, o_ch_num: %d, o_ch_num_coef: %d\n",
         layer_arg.image_channel_num.data.i_ch_num,
         layer_arg.image_channel_num.data.o_ch_num,
         layer_arg.image_channel_num.data.o_ch_num_coef
        );

    pout(" image_size:\n"
         "  i_row_wid: %d, i_col_high: %d, o_row_wid: %d, o_col_high: %d\n",
         layer_arg.image_size.data.i_row_wid,
         layer_arg.image_size.data.i_col_high,
         layer_arg.image_size.data.o_row_wid,
         layer_arg.image_size.data.o_col_high
        );

    pout(" kernel_pool_type_cfg:\n"
         "  kernel_type: %d, pad_type: %d, pool_type: %d, first_stride: %d, "
         "bypass_conv: %d, load_para: %d, dma_burst_size: %d, pad_value: %d, bwsx_base_addr: %d\n",
         layer_arg.kernel_pool_type_cfg.data.kernel_type,
         layer_arg.kernel_pool_type_cfg.data.pad_type,
         layer_arg.kernel_pool_type_cfg.data.pool_type,
         layer_arg.kernel_pool_type_cfg.data.first_stride,
         layer_arg.kernel_pool_type_cfg.data.bypass_conv,
         layer_arg.kernel_pool_type_cfg.data.load_para,
         layer_arg.kernel_pool_type_cfg.data.dma_burst_size,
         layer_arg.kernel_pool_type_cfg.data.pad_value,
         layer_arg.kernel_pool_type_cfg.data.bwsx_base_addr
        );

    pout(" kernel_load_cfg:\n"
         "  load_coor: %d, load_time: %d, para_size: %d, para_start_addr: %d\n",
         layer_arg.kernel_load_cfg.data.load_coor,
         layer_arg.kernel_load_cfg.data.load_time,
         layer_arg.kernel_load_cfg.data.para_size,
         layer_arg.kernel_load_cfg.data.para_start_addr
        );

    pout(" kernel_offset:\n"
         "  coef_column_offset: %d, coef_row_offset: %d\n",
         layer_arg.kernel_offset.data.coef_column_offset,
         layer_arg.kernel_offset.data.coef_row_offset
        );

    pout(" kernel_calc_type_cfg:\n"
         "  channel_switch_addr: %d, row_switch_addr: %d, coef_size: %d, coef_group: %d, load_act: %d, active_addr: %d\n",
         layer_arg.kernel_calc_type_cfg.data.channel_switch_addr,
         layer_arg.kernel_calc_type_cfg.data.row_switch_addr,
         layer_arg.kernel_calc_type_cfg.data.coef_size,
         layer_arg.kernel_calc_type_cfg.data.coef_group,
         layer_arg.kernel_calc_type_cfg.data.load_act,
         layer_arg.kernel_calc_type_cfg.data.active_addr
        );

    pout(" write_back_cfg:\n"
         "  channel_switch_addr: %d, row_switch_addr: %d, coef_size: %d\n",
         layer_arg.write_back_cfg.data.wb_channel_switch_addr,
         layer_arg.write_back_cfg.data.wb_row_switch_addr,
         layer_arg.write_back_cfg.data.wb_group
        );

    pout(" conv_value:\n"
         "  shr_w: %d, shr_x: %d, arg_w: %d, arg_x: %d\n",
         layer_arg.conv_value.data.shr_w,
         layer_arg.conv_value.data.shr_x,
         layer_arg.conv_value.data.arg_w,
         layer_arg.conv_value.data.arg_x
        );

    pout(" conv_value2:\n"
         "  arg_add: %ld\n",
         layer_arg.conv_value2.data.arg_add
        );

    pout(" dma_parameter:\n"
         "  send_data_out: %d, channel_byte_num: %d, dma_total_byte: %d\n",
         layer_arg.dma_parameter.data.send_data_out,
         layer_arg.dma_parameter.data.channel_byte_num,
         layer_arg.dma_parameter.data.dma_total_byte
        );

    return 0;
}


int cnn_print_active_table(cnn_activate_table_t act_tab, print_cb pout)
{
    pout("activate_table:\n");
    for(uint32_t i=0; i<16; i++) {
        uint8_t bias;

        if(i<8) {
            bias = act_tab.activate_para_bias0.data.result_bias[i];
        } else {
            bias = act_tab.activate_para_bias1.data.result_bias[i-8];
        }

        pout("  [%02d]: shift: %d, y_mul: %d, x_start: %d, bian: %d\n",
             i,
             act_tab.activate_para[i].data.shift_number,
             act_tab.activate_para[i].data.y_mul,
             act_tab.activate_para[i].data.x_start,
             bias
            );
    }
    return 0;
}


int cnn_print_batchnorm_argument(cnn_batchnorm_argument_t bn_arg, print_cb pout)
{
    pout("batchnorm_argument:\n"
         "  norm_mul: %d, norm_add: %d, norm_shift: %d\n",
         bn_arg.batchnorm.data.norm_mul,
         bn_arg.batchnorm.data.norm_add,
         bn_arg.batchnorm.data.norm_shift
        );

    return 0;
}


int cnn_print_config(cnn_config_t conf, print_cb pout)
{
    pout("layer:\n");

    pout(" interrupt_mask:\n"
         "  calc_done_int: %d, layer_cfg_almost_empty_int: %d, layer_cfg_almost_full_int: %d, ram_flag: %d\n",
         conf.interrupt_mask.data.calc_done_int,
         conf.interrupt_mask.data.layer_cfg_almost_empty_int,
         conf.interrupt_mask.data.layer_cfg_almost_full_int
        );

    pout(" fifo_threshold:\n"
         "  fifo_full_threshold: %d, fifo_empty_threshold: %d\n",
         conf.fifo_threshold.data.fifo_full_threshold,
         conf.fifo_threshold.data.fifo_empty_threshold
        );

    pout(" eight_bit_mode: %d\n",
         conf.eight_bit_mode.data.eight_bit_mode
        );

    return 0;
}

int cnn_run_all_done(void* _task)
{
    LOG("_start %s [cnn] all done\n", __func__);
    cnn_task_t* task = (cnn_task_t*)_task;

    task->cb(task);
    LOG("_end %s\n", __func__);
    return 0;
}

int cnn_continue(void* _task)
{
    LOG("_start %s\n", __func__);
    cnn_task_t* task = (cnn_task_t*)_task;
    int layer_burst_size = 12;
    uint64_t int_status = cnn->interrupt_status.reg;
    LOG("[cnn] continued %d layers more, status:%ld\n", task->length, int_status);
    cnn->interrupt_clear.data = (cnn_config_interrupt_t) {
        .calc_done_int=1,
        .layer_cfg_almost_empty_int=1,
        .layer_cfg_almost_full_int=1
    };

    if(task->length == 0){
        return 0;
    }
    if(task->length <= layer_burst_size) {
        LOG("[cnn] last push\n");
        for(uint32_t i=0; i<task->length; i++) {
            cnn->layer_argument_fifo = task->layers[i].interrupt_enabe.reg;
            cnn->layer_argument_fifo = task->layers[i].image_addr.reg;
            cnn->layer_argument_fifo = task->layers[i].image_channel_num.reg;
            cnn->layer_argument_fifo = task->layers[i].image_size.reg;
            cnn->layer_argument_fifo = task->layers[i].kernel_pool_type_cfg.reg;
            cnn->layer_argument_fifo = task->layers[i].kernel_load_cfg.reg;
            cnn->layer_argument_fifo = task->layers[i].kernel_offset.reg;
            cnn->layer_argument_fifo = task->layers[i].kernel_calc_type_cfg.reg;
            cnn->layer_argument_fifo = task->layers[i].write_back_cfg.reg;
            cnn->layer_argument_fifo = task->layers[i].conv_value.reg;
            cnn->layer_argument_fifo = task->layers[i].conv_value2.reg;
            cnn->layer_argument_fifo = task->layers[i].dma_parameter.reg;
        }
        task->length = 0;
        LOG("[cnn] all layers pushed\n");
    } else {
        LOG("[cnn] mid push\n");

        for(uint32_t i=0; i<layer_burst_size; i++) {
            cnn->layer_argument_fifo = task->layers[i].interrupt_enabe.reg;
            cnn->layer_argument_fifo = task->layers[i].image_addr.reg;
            cnn->layer_argument_fifo = task->layers[i].image_channel_num.reg;
            cnn->layer_argument_fifo = task->layers[i].image_size.reg;
            cnn->layer_argument_fifo = task->layers[i].kernel_pool_type_cfg.reg;
            cnn->layer_argument_fifo = task->layers[i].kernel_load_cfg.reg;
            cnn->layer_argument_fifo = task->layers[i].kernel_offset.reg;
            cnn->layer_argument_fifo = task->layers[i].kernel_calc_type_cfg.reg;
            cnn->layer_argument_fifo = task->layers[i].write_back_cfg.reg;
            cnn->layer_argument_fifo = task->layers[i].conv_value.reg;
            cnn->layer_argument_fifo = task->layers[i].conv_value2.reg;
            cnn->layer_argument_fifo = task->layers[i].dma_parameter.reg;
        }
        task->layers += layer_burst_size;
        task->length -= layer_burst_size;
    }
    LOG("_end %s [cnn] exit with %d layers more\n", __func__, task->length);
    return 0;
}

int cnn_run_dma_input_done_push_layers(void* _task)
{
    LOG("_start %s\n", __func__);
    LOG("[cnn] dma in done\n");
    cnn_task_t* task = (cnn_task_t*)_task;
    cnn->interrupt_clear.reg = 7;
    dmac->channel[task->dma_ch].intclear = 0xFFFFFFFF;
    cnn->fifo_threshold.data = (cnn_config_fifo_threshold_t) {
        .fifo_full_threshold = 10, .fifo_empty_threshold=1
    };
    cnn->eight_bit_mode.data = (cnn_config_eight_bit_mode_t) {
        .eight_bit_mode=0
    };

    cnn_layer_argument_t* last_layer = &task->layers[task->length-1];

    cnn_run_dma_output(task->dma_ch, task->dst, last_layer->dma_parameter.data.dma_total_byte+1, cnn_run_all_done, task);

    cnn->interrupt_mask.reg = 7;
    LOG("%s task length is %d\n", __func__, task->length);
    cnn_continue(task);
    LOG("D cnn_continue flowing\n");
    cnn->interrupt_mask.data = (cnn_config_interrupt_t) {
        .calc_done_int=0,
        .layer_cfg_almost_empty_int=0,
        .layer_cfg_almost_full_int=1
    };
    LOG("_end %s\n", __func__);
    return 0;
}

void cnn_run_dma_input(uint32_t dma_ch, void* src, plic_irq_callback_t cb, void* _task)
{
    cnn_task_t* task = _task;
    cnn_layer_argument_t* first_layer = &task->layers[0];
    uint64_t input_size = first_layer->kernel_calc_type_cfg.data.channel_switch_addr * 64 * (first_layer->image_channel_num.data.i_ch_num+1);
    dmac_set_irq(dma_ch, cb, _task, 1);
    dmac_set_single_mode(dma_ch, (void *)src, (void *)(AI_IO_BASE_ADDR), DMAC_ADDR_INCREMENT, DMAC_ADDR_INCREMENT,
        DMAC_MSIZE_16, DMAC_TRANS_WIDTH_64, input_size / 8);
}

int cnn_run_dma_output(uint32_t dma_ch, void* dst, uint32_t length, plic_irq_callback_t cb, void* _task)
{
    sysctl_dma_select(dma_ch, SYSCTL_DMA_SELECT_AI_RX_REQ);
    dmac_set_irq(dma_ch, cb, _task, 1);
    dmac_set_single_mode(dma_ch, (void *)(&cnn->fifo_data_out), (void *)(dst), DMAC_ADDR_NOCHANGE, DMAC_ADDR_INCREMENT,
        DMAC_MSIZE_8, DMAC_TRANS_WIDTH_64, (length+7)/8);
    return 0;
}

volatile uint8_t g_ai_mem_copy_done_flag = 0;

static int cnn_input_done(void *ctx)
{
    LOG("%s\n",__func__);
    g_ai_mem_copy_done_flag = 1;
    return 0;
}

int cnn_run(cnn_task_t* task, int dma_ch, void *src, void* dst, plic_irq_callback_t cb)
{
    LOG("_start %s [cnn] start run\n", __func__);
    cnn_layer_argument_t* last_layer = &task->layers[task->length-1];
    cnn_layer_argument_t* first_layer = &task->layers[0];

    uint64_t input_size = first_layer->kernel_calc_type_cfg.data.channel_switch_addr * 64 * (first_layer->image_channel_num.data.i_ch_num+1);
    uint64_t output_size = last_layer->dma_parameter.data.dma_total_byte+1;
    LOG("[dma] input: %lu, output: %lu\n", input_size, output_size);

    last_layer->dma_parameter.data.send_data_out = 1;
    last_layer->interrupt_enabe.data.int_en = 1;

    task->dma_ch = dma_ch;
    task->dst = dst;
    task->dst_length = output_size;
    task->cb = cb;

    plic_irq_enable(IRQN_AI_INTERRUPT);
    plic_set_priority(IRQN_AI_INTERRUPT, 4);
    plic_irq_register(IRQN_AI_INTERRUPT, cnn_continue, task);

    cnn_run_dma_input(dma_ch, src, cnn_input_done, task);
    while(!g_ai_mem_copy_done_flag);
    g_ai_mem_copy_done_flag = 0;

    LOG("task length is %d\n", task->length);
    cnn_run_dma_input_done_push_layers(task);

    LOG("_end %s\n", __func__);
    return 0;
}

void cnn_set_irq(plic_irq_callback_t cnn_irq, void *ctx)
{
    plic_irq_enable(IRQN_AI_INTERRUPT);
    plic_set_priority(IRQN_AI_INTERRUPT, 4);
    plic_irq_register(IRQN_AI_INTERRUPT, cnn_irq, ctx);
}

