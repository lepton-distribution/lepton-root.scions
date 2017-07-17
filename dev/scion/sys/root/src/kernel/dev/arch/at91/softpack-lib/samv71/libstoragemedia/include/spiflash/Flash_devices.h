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


#ifndef FLASH_DEVICES_H_
#define FLASH_DEVICES_H_

//------------------------------------------------------------------------------
//         Headers
//------------------------------------------------------------------------------

#include "boards.h"


/*----------------------------------------------------------------------------
 *        Local definitions
 *----------------------------------------------------------------------------*/

/** Number of recognized dataflash. */
#define NUMDATAFLASH    (sizeof(at25Devices) / sizeof(At25Desc))

/*----------------------------------------------------------------------------
 *        Local variables
 *----------------------------------------------------------------------------*/

/** Array of recognized serial firmware dataflash chips. */
static const At25Desc at25Devices[] = {
	/* name,        Jedec ID,       size,  page size, block size, block erase command */
	{"AT25DF041A" , 0x0001441F,      512 * 1024, 256,  4 * 1024, AT25_BLOCK_ERASE_4K},
	{"AT25DF161"  , 0x0002461F, 2 * 1024 * 1024, 256,  4 * 1024, AT25_BLOCK_ERASE_4K},
	{"AT26DF081A" , 0x0001451F, 1 * 1024 * 1024, 256,  4 * 1024, AT25_BLOCK_ERASE_4K},
	{"AT26DF0161" , 0x0000461F, 2 * 1024 * 1024, 256,  4 * 1024, AT25_BLOCK_ERASE_4K},
	{"AT26DF161A" , 0x0001461F, 2 * 1024 * 1024, 256,  4 * 1024, AT25_BLOCK_ERASE_4K},
	{"AT25DF321"  , 0x0000471F, 4 * 1024 * 1024, 256,  4 * 1024, AT25_BLOCK_ERASE_4K},
	{"AT25DF321A" , 0x0001471F, 4 * 1024 * 1024, 256,  4 * 1024, AT25_BLOCK_ERASE_4K},
	{"AT25DF512B" , 0x0001651F,       64 * 1024, 256,  4 * 1024, AT25_BLOCK_ERASE_4K},
	{"AT25DF512B" , 0x0000651F,       64 * 1024, 256,  4 * 1024, AT25_BLOCK_ERASE_4K},
	{"AT25DF021"  , 0x0000431F,      256 * 1024, 256,  4 * 1024, AT25_BLOCK_ERASE_4K},
	{"AT26DF641"  , 0x0000481F, 8 * 1024 * 1024, 256,  4 * 1024, AT25_BLOCK_ERASE_4K},
	/* Manufacturer: ST */
	{"M25P05"     , 0x00102020,       64 * 1024, 256, 32 * 1024, AT25_BLOCK_ERASE_64K},
	{"M25P10"     , 0x00112020,      128 * 1024, 256, 32 * 1024, AT25_BLOCK_ERASE_64K},
	{"M25P20"     , 0x00122020,      256 * 1024, 256, 64 * 1024, AT25_BLOCK_ERASE_64K},
	{"M25P40"     , 0x00132020,      512 * 1024, 256, 64 * 1024, AT25_BLOCK_ERASE_64K},
	{"M25P80"     , 0x00142020, 1 * 1024 * 1024, 256, 64 * 1024, AT25_BLOCK_ERASE_64K},
	{"M25P16"     , 0x00152020, 2 * 1024 * 1024, 256, 64 * 1024, AT25_BLOCK_ERASE_64K},
	{"M25P32"     , 0x00162020, 4 * 1024 * 1024, 256, 64 * 1024, AT25_BLOCK_ERASE_64K},
	{"M25P64"     , 0x00172020, 8 * 1024 * 1024, 256, 64 * 1024, AT25_BLOCK_ERASE_64K},
	/* Manufacturer: Windbond */
	{"W25X10"     , 0x001130EF,      128 * 1024, 256,  4 * 1024, AT25_BLOCK_ERASE_4K},
	{"W25X20"     , 0x001230EF,      256 * 1024, 256,  4 * 1024, AT25_BLOCK_ERASE_4K},
	{"W25X40"     , 0x001330EF,      512 * 1024, 256,  4 * 1024, AT25_BLOCK_ERASE_4K},
	{"W25X80"     , 0x001430EF, 1 * 1024 * 1024, 256,  4 * 1024, AT25_BLOCK_ERASE_4K},
	{"W25Q256"    , 0x001940EF, 32 * 1024 * 1024, 256,  4 * 1024, AT25_BLOCK_ERASE_4K},
	/* Manufacturer: Macronix */
	{"MX25L512"   , 0x001020C2,       64 * 1024, 256,  4 * 1024, AT25_BLOCK_ERASE_4K},
	{"MX25L3205"  , 0x001620C2, 4 * 1024 * 1024, 256, 64 * 1024, AT25_BLOCK_ERASE_64K},
	{"MX25L6405"  , 0x001720C2, 8 * 1024 * 1024, 256,  4 * 1024, AT25_BLOCK_ERASE_4K},
	{"MX25L8005"  , 0x001420C2,     1024 * 1024, 256,  4 * 1024, AT25_BLOCK_ERASE_4K},
	/* Other */
	{"SST25VF040" , 0x008D25BF,      512 * 1024, 256,  4 * 1024, AT25_BLOCK_ERASE_4K},
	{"SST25VF080" , 0x008E25BF, 1 * 1024 * 1024, 256,  4 * 1024, AT25_BLOCK_ERASE_4K},
	{"SST25VF032" , 0x004A25BF, 4 * 1024 * 1024, 256,  4 * 1024, AT25_BLOCK_ERASE_4K},
	{"SST25VF064" , 0x004B25BF, 8 * 1024 * 1024, 256,  4 * 1024, AT25_BLOCK_ERASE_4K},
	/* Manufacturer: Micron */
	{"N25Q256"    , 0x0019BA20, 32 * 1024 * 1024, 256,  4 * 1024, AT25_BLOCK_ERASE_4K}
};

#endif // #ifndef AT26D_H

