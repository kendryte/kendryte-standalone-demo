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
#include "timer.h"
#include "pwm.h"
#include "plic.h"
#include "sysctl.h"
#include "fpioa.h"

#define TIMER_NOR   0
#define TIMER_CHN   0
#define TIMER_PWM   1
#define TIMER_PWM_CHN 0

void irq_time(void)
{
    static double cnt = 0.01;
    static int flag = 0;

    pwm_set_frequency(TIMER_PWM, TIMER_PWM_CHN, 200000, cnt);

    flag ? (cnt -= 0.01): (cnt += 0.01);
    if(cnt > 1.0)
    {
        cnt = 1.0;
        flag = 1;
    }
    else if (cnt < 0.0)
    {
        cnt = 0.0;
        flag = 0;
    }
}

int main(void)
{
    fpioa_set_function(24, FUNC_TIMER1_TOGGLE1);                /*PWM1*/

    plic_init();
    sysctl_enable_irq();
    timer_init(TIMER_NOR);
    timer_set_interval(TIMER_NOR, TIMER_CHN, 1e7);
    timer_set_irq(TIMER_NOR, TIMER_CHN, irq_time, 1);
    timer_set_enable(TIMER_NOR, TIMER_CHN, 1);

    pwm_init(TIMER_PWM);
    pwm_set_frequency(TIMER_PWM, TIMER_PWM_CHN, 200000, 0.5);
    pwm_set_enable(TIMER_PWM, TIMER_PWM_CHN, 1);

    while(1);
}
