#include <stdio.h>
#include <FreeRTOS.h>
#include <task.h>
#include <unistd.h>
#include <plic.h>
#include <sysctl.h>
#include <bsp.h>

void vTask1(void *ctx)
{
    uint64_t core = current_coreid();
    while (1)
    {
        printf("[%ld]Hello I am task1\n", core);
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}

void vTask2(void *ctx)
{
    uint64_t core = current_coreid();
    while (1)
    {
        printf("[%ld]Hello I am task2\n", core);
        vTaskDelay(2000 / portTICK_RATE_MS);
    }
}

int core1_func(void *ctx)
{
    usleep(500000);
    xTaskCreate(vTask2, "vTask2", 2048, NULL, 2, NULL);
    xTaskCreate(vTask1, "vTask1", 2048, NULL, 1, NULL);
    vTaskStartScheduler();
    while(1);
}

int main(void)
{
    register_core1(core1_func, NULL);
    xTaskCreate(vTask2, "vTask2", 2048, NULL, 1, NULL);
    xTaskCreate(vTask1, "vTask1", 2048, NULL, 2, NULL);
    vTaskStartScheduler();
    while (1)
        ;
}

