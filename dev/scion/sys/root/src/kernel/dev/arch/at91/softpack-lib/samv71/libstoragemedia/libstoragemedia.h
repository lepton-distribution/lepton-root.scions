/* ---------------------------------------------------------------------------- */
/*                  Atmel Microcontroller Software Support                      */
/*                       SAM Software Package License                           */
/* ---------------------------------------------------------------------------- */
/* Copyright (c) 2015, Atmel Corporation                                        */
/*                                                                              */
/* All rights reserved.                                                         */
/*                                                                              */
/* Redistribution and use in source and binary forms, with or without           */
/* modification, are permitted provided that the following condition is met:    */
/*                                                                              */
/* - Redistributions of source code must retain the above copyright notice,     */
/* this list of conditions and the disclaimer below.                            */
/*                                                                              */
/* Atmel's name may not be used to endorse or promote products derived from     */
/* this software without specific prior written permission.                     */
/*                                                                              */
/* DISCLAIMER:  THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR   */
/* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE   */
/* DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,      */
/* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT */
/* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,  */
/* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF    */
/* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING         */
/* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, */
/* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                           */
/* ---------------------------------------------------------------------------- */

#ifndef _LIB_STORAGEMEDIA_ 
#define _LIB_STORAGEMEDIA_ 


/* Define attribute */
#if defined   (__CC_ARM) /* Keil uvision 4 */
	#define WEAK __attribute__ ((weak))
#elif defined (__ICCARM__) /* IAR Ewarm 5.41+ */
	#define WEAK __weak
#elif defined (__GNUC__) /* GCC CS3 2009q3-68 */
	#define WEAK __attribute__ ((weak))
#endif

/* Define NO_INIT attribute */
#if defined   (__CC_ARM)
	#define NO_INIT
#elif defined (__ICCARM__)
	#define NO_INIT __no_init
#elif defined (__GNUC__)
	#define NO_INIT
#endif

/*
 * drivers
 */

#include "board.h"

#include "Media.h"
#include "MEDNandFlash.h"
#include "MEDRamDisk.h"
#include "MEDSdcard.h"
#include "MEDSdram.h"
#include "sdio.h"
#include "sdmmc.h"
#include "sdmmc_cmd.h"
#include "sdmmc_hal.h"
#include "sdmmc_trace.h"
#endif /* _LIB_STORAGEMEDIA_ */
