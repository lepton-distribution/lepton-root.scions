/*-
 * Copyright (c) 2009 Fredrik Lindberg
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
 * Sample AVR system clock, runs at 32 Hz (31.25 ms interval)
 *
 * Calculated prescaler and counter values are adjusted for
 * an external 32kHz watch crystal connected to the TOSC-pins. 
 * Values MUST be recalculated if another frequency is used.
 * 
 * Requires AVR libc <http://www.nongnu.org/avr-libc/>
 * Registers/IRQ vectors are for AVR ATMega644/324, adjust as needed.
 *
 */

//#include <avr/io.h>
//#include <avr/interrupt.h>
//#include <util/delay.h>
//#include "itron_types.h"
//#include "trifecta_lib.h"
//#include "mem_regions.h"
//#include "BSP_ERROR.h"
//#include "BSP_RTC.h"
//#include "BSP_Common.h"

//#include "trifecta_map.h"
typedef struct
{
  unsigned long l; //u32
  unsigned long h;  //u32
} RTC_Counter48;

//extern TIM_TypeDef* TIMER_BASE_REG[TIMER_COUNT];
extern unsigned long tic_counter;
typedef unsigned long clock_time_t;
//static clock_time_t global_system_ticks = 0;

//ISR(TIMER2_OVF_vect)
//{
//
//	global_system_ticks++;
//}
//
//ISR(TIMER2_COMPA_vect)
//{
//
//	global_system_ticks++;
//}
//extern unsigned char RTC_GetValue(RTC_TypeDef* pRTC, RTC_Counter48 *pValue);

clock_time_t clock_time()
{
//        if( u16CSYNC_Timer2_Count >= TIMER2_ONE_SEC/TIMER2_PERIOD )
//        {
//        }
//            u16CSYNC_Timer2_Count = ( TIMER2_ONE_SEC / TIMER2_PERIOD * (delatRTC + u16CSYNC_Timer2_Count_RTCRemainder) ) / RTC_COUNTER_FREQ;
//        if(u16CSYNC_Timer2_Count >= TIMER2_ONE_SEC / TIMER2_PERIOD)
//        {
//            OutstandingTicks++;
//            u16CSYNC_Timer2_Count = u16CSYNC_Timer2_Count - TIMER2_ONE_SEC / TIMER2_PERIOD;
//        }
   // return (u16CSYNC_Timer2_Count);

//    uint32_t tmp;
// 
//    TIMSK2 &= ~(1 << OCIE2A) | (1 << TOIE2);
//    tmp = global_system_ticks;
//    TIMSK2 |= (1 << OCIE2A) | (1 << TOIE2);
//    return (tmp);

//phlb modif
    //RTC_Counter48 counter;
    //RTC_GetValue(RTC, &counter);
    //return(counter.l);
    return 0;
    

//    return (tic_counter);
//    return(    BSP_Common_getMsFreeRunningTime());
//    return( BSP_RTC_FreeRunCounter_Read() );
        
//    return ( GMT_Real_Time );//TIM_Get(TIMER_BASE_REG[TIMER2]));
}


unsigned long clock_seconds(void)
{

	return (clock_time() / 1000);//500);//32);
}

void
clock_init()
{

//	/* Enable external oscillator on TOSC{1,2} */
//	ASSR |= (1 << AS2); 
//
//	/* Reset timer */
//	TCNT2 = 0;
//
//	/* TS/8 prescaler, gives a 4096Hz clock */
//	TCCR2B |= (1 << CS21); 
//
//	/* Compare at half counter value */
//	OCR2A = 128; 
//
//	/*
//	 * Enable overflow, trigger each 1/32 secs 
//	 * Enable compare, trigger each 1/32 secs
//	 */
//	TIMSK2 |= (1 << OCIE2A) | (1 << TOIE2); 
}

