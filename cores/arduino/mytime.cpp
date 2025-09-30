/*
 * mytime.c
 *
 *  Created on: 2023年11月8日
 *      Author: 86183
 */

#include <mytime.h>

uint32_t tick_per_us = 1;
uint32_t tick_per_ms = 1000*tick_per_us;

void time_init(void)
{
    /*配置定时器*/
    SysTick->CTLR= 0;
    SysTick->CNT = 0;  //64位  当前计数器的值
    SysTick->CTLR= 0x00000001; // 8 分频
	tick_per_us = SystemCoreClock/(1000000*8);
    tick_per_ms = 1000*tick_per_us;
}

u32 time_get()
{
    return SysTick->CNT/tick_per_us;
}

u32 time_get_ms()
{
    return SysTick->CNT/tick_per_ms;
}

void delay_us(u32 us){
	u32 time_enter = time_get();
	while(time_get() - time_enter < us);
}


