/*
 * mytime.h
 *
 *  Created on: 2023年11月8日
 *      Author: 86183
 */

#ifndef USER_MYTIME_H_
#define USER_MYTIME_H_

#include "ch32v20x.h"
#include "Arduino.h"
#include "debug.h"

#ifdef __cplusplus
extern "C" {
#endif

void time_init();
u32 time_get();
u32 time_get_ms();
void delay_us(u32 us);

#ifdef __cplusplus
}
#endif

#endif /* USER_MYTIME_H_ */
