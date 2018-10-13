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
#include "rtc.h"
#include <unistd.h>

void get_date_time(void)
{
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    rtc_timer_get(&year, &month, &day, &hour, &minute, &second);
    printf("%4d-%d-%d %d:%d:%d\n", year, month, day, hour, minute, second);
}

int main(void)
{
    rtc_init();
    rtc_timer_set(2018, 9, 12, 23, 30, 29);
    while(1)
    {
        sleep(1);
        get_date_time();
    }
    return 0;
}
