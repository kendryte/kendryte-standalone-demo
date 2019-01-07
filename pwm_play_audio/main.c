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
#include <syslog.h>
#include <timer.h>
#include <pwm.h>
#include <plic.h>
#include <sysctl.h>
#include <fpioa.h>
#include "pwm_play_audio.h"
#include <unistd.h>
#include <stdlib.h>

#include "minimp3.h"

#include "test_mono_mp3.h"
#include "test_stereo_mp3.h"

#include "test_wav.h"
#include "test_8bit_wav.h"
#include "test_16bit_wav.h"
#include "test_24bit_wav.h"
#include "test_16bit_mono_wav.h"
#include "test_welcome.h"

#define TIMER_NOR   0
#define TIMER_CHN   1
#define TIMER_PWM   0
#define TIMER_PWM_CHN 0

int main(void)
{
    printf("PWM wav test\n");
    /* Init FPIOA pin mapping for PWM*/
    fpioa_set_function(24, FUNC_TIMER0_TOGGLE1);
    /* Init Platform-Level Interrupt Controller(PLIC) */
    plic_init();
    /* Enable global interrupt for machine mode of RISC-V */
    sysctl_enable_irq();

    pwm_play_init(TIMER_NOR, TIMER_PWM);

    uint8_t *wav_mono = malloc(10 * sizeof(test_mono_mp3));
    mp3_to_wav(test_mono_mp3, sizeof(test_mono_mp3), wav_mono, 10 * sizeof(test_mono_mp3));

    uint8_t *wav_stereo = malloc(20 * sizeof(test_stereo_mp3));
    mp3_to_wav(test_stereo_mp3, sizeof(test_stereo_mp3), wav_stereo, 20 * sizeof(test_stereo_mp3));

    while(1)
    {
        printf("Play mp3 mono\n");
        pwm_play_wav(TIMER_NOR, TIMER_CHN, TIMER_PWM, TIMER_PWM_CHN, wav_mono, 0);
        printf("Play mp3 stereo\n");
        pwm_play_wav(TIMER_NOR, TIMER_CHN, TIMER_PWM, TIMER_PWM_CHN, wav_stereo, 0);
//        pwm_play_wav(TIMER_NOR, TIMER_CHN, TIMER_PWM, TIMER_PWM_CHN, test_wav, 0);
//        pwm_play_wav(TIMER_NOR, TIMER_CHN, TIMER_PWM, TIMER_PWM_CHN, test_8bit_wav, 0);
//        pwm_play_wav(TIMER_NOR, TIMER_CHN, TIMER_PWM, TIMER_PWM_CHN, test_16bit_mono_wav, 0);
//        pwm_play_wav(TIMER_NOR, TIMER_CHN, TIMER_PWM, TIMER_PWM_CHN, test_16bit_wav, 0);
//        pwm_play_wav(TIMER_NOR, TIMER_CHN, TIMER_PWM, TIMER_PWM_CHN, test_24bit_wav, 0);
//        sleep(2);
        pwm_play_wav(TIMER_NOR, TIMER_CHN, TIMER_PWM, TIMER_PWM_CHN, test_welcome_wav, 0);
    }
}
