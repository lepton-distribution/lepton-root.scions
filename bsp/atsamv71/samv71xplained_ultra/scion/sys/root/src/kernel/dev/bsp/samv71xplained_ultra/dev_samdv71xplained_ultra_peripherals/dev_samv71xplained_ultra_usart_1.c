/*
The contents of this file are subject to the Mozilla Public License Version 1.1 
(the "License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at http://www.mozilla.org/MPL/

Software distributed under the License is distributed on an "AS IS" basis, 
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for the 
specific language governing rights and limitations under the License.

The Original Code is Lepton.

The Initial Developer of the Original Code is Philippe Le Boulanger.
Portions created by Philippe Le Boulanger are Copyright (C) 2011 <lepton.phlb@gmail.com>.
All Rights Reserved.

Contributor(s): Jean-Jacques Pitrolle <lepton.jjp@gmail.com>.

Alternatively, the contents of this file may be used under the terms of the eCos GPL license 
(the  [eCos GPL] License), in which case the provisions of [eCos GPL] License are applicable 
instead of those above. If you wish to allow use of your version of this file only under the
terms of the [eCos GPL] License and not to allow others to use your version of this file under 
the MPL, indicate your decision by deleting  the provisions above and replace 
them with the notice and other provisions required by the [eCos GPL] License. 
If you do not delete the provisions above, a recipient may use your version of this file under 
either the MPL or the [eCos GPL] License."
*/

/*===========================================
Includes
=============================================*/

#include <stdint.h>

#include "kernel/core/kernelconf.h"
#include "kernel/core/types.h"
#include "kernel/core/dirent.h"
#include "kernel/core/stat.h"
#include "kernel/core/interrupt.h"
#include "kernel/core/kernel.h"
#include "kernel/core/system.h"
#include "kernel/core/ioctl.h"
#include "kernel/core/fcntl.h"
#include "kernel/core/cpu.h"
#include "kernel/fs/vfs/vfstypes.h"
#include "kernel/core/kernel_ring_buffer.h"

#include "lib/libc/termios/termios.h"

#include "board.h"

#include "kernel/dev/arch/cortexm/at91samv7x/dev_samv7x/dev_samv7x_cpu_x.h"
#include "kernel/dev/arch/cortexm/at91samv7x/dev_samv7x/dev_samv7x_usart_x.h"

/*===========================================
Global Declaration
=============================================*/
static const char dev_samv71xplained_ultra_usart_1_name[]="ttys1\0";

static int dev_samv71xplained_ultra_usart_1_load(void);
static int dev_samv71xplained_ultra_usart_1_open(desc_t desc, int o_flag);


dev_map_t dev_samv71xplained_ultra_usart_1_map={
   dev_samv71xplained_ultra_usart_1_name,
   S_IFCHR,
   dev_samv71xplained_ultra_usart_1_load,
   dev_samv71xplained_ultra_usart_1_open,
   dev_samv71x_usart_x_close,
   dev_samv71x_usart_x_isset_read,
   dev_samv71x_usart_x_isset_write,
   dev_samv71x_usart_x_read,
   dev_samv71x_usart_x_write,
   dev_samv71x_usart_x_seek,
   dev_samv71x_usart_x_ioctl
};


// uart 1 debug configuration for samv71 xplained ultra dev kit.
/** USART1 pin RX */
#define PIN_USART1_RXD_DBG \
	{PIO_PA21A_RXD1, PIOA, ID_PIOA, PIO_PERIPH_A, PIO_DEFAULT}
/** USART1 pin TX */
#define PIN_USART1_TXD_DBG \
	{PIO_PB4D_TXD1, PIOB, ID_PIOB, PIO_PERIPH_D, PIO_DEFAULT}
#define PINS_USART1        PIN_USART1_TXD_DBG, PIN_USART1_RXD_DBG

   
static const Pin base_uart_pins[] = {PINS_USART1};
static samv71x_usart_info_t samv71xplained_ultra_usart_info_1; 

COMPILER_ALIGNED(32) uint8_t rx_dma_buffer_usart_1[64];
COMPILER_ALIGNED(32) uint8_t tx_dma_buffer_usart_1[64];

/*===========================================
Implementation
=============================================*/


/*-------------------------------------------
| Name:dev_samv71xplained_ultra_usart_1_load
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_samv71xplained_ultra_usart_1_load(void) {
   
   //
   samv71xplained_ultra_usart_info_1.dwBaseId=ID_USART1;
   samv71xplained_ultra_usart_info_1.BaseUsart=USART1;
   //
   samv71xplained_ultra_usart_info_1.base_usart_pins = &base_uart_pins[0];
   //
   samv71xplained_ultra_usart_info_1.desc_r=-1;
   samv71xplained_ultra_usart_info_1.desc_w=-1;
   samv71xplained_ultra_usart_info_1.baudrate=115200; 
   //RX dma buffer
   samv71xplained_ultra_usart_info_1.RxDmaBufferSize=sizeof(rx_dma_buffer_usart_1);
   samv71xplained_ultra_usart_info_1.pRxDmaBuffer=&rx_dma_buffer_usart_1[0];
   //TX dma buffer
   samv71xplained_ultra_usart_info_1.TxDmaBufferSize=sizeof(tx_dma_buffer_usart_1);
   samv71xplained_ultra_usart_info_1.pTxDmaBuffer=&tx_dma_buffer_usart_1[0];
   
   //
   // Disable the MATRIX registers write protection
	MATRIX->MATRIX_WPMR  = MATRIX_WPMR_WPKEY_PASSWD;
	MATRIX->CCFG_SYSIO |= CCFG_SYSIO_SYSIO4;
   //
   PIO_Configure(base_uart_pins, PIO_LISTSIZE(base_uart_pins));
   
   return dev_samv71x_usart_x_load(&samv71xplained_ultra_usart_info_1);
}
   
/*-------------------------------------------
| Name:dev_samv71xplained_ultra_usart_1_open
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_samv71xplained_ultra_usart_1_open(desc_t desc, int o_flag) {
   return dev_samv71x_usart_x_open(desc, o_flag, &samv71xplained_ultra_usart_info_1);
}

/*============================================
| End of Source  : dev_samv71xplained_ultra_usart_1.c
==============================================*/