/*
The contents of this file are subject to the Mozilla Public License Version 1.1
(the "License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at http://www.mozilla.org/MPL/

Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for the
specific language governing rights and limitations under the License.

The Original Code is Lepton.

The Initial Developer of the Original Code is Philippe Le Boulanger.
Portions created by Philippe Le Boulanger are Copyright (C) 2014 <lepton.phlb@gmail.com>.
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
#include "kernel/core/kernelconf.h"
#include "kernel/core/types.h"
#include "kernel/core/interrupt.h"
#include "kernel/core/kernel.h"
#include "kernel/core/system.h"
#include "kernel/core/fcntl.h"
#include "kernel/core/stat.h"
#include "kernel/core/cpu.h"
#include "kernel/fs/vfs/vfsdev.h"

#include "kernel/core/ucore/cmsis/Device/st/stm32f1xx/stm32f10x.h"
#include "kernel/dev/arch/cortexm/stm32f1xx/target.h"
#include "kernel/dev/arch/cortexm/stm32f1xx/gpio.h"
#include "kernel/dev/arch/cortexm/stm32f1xx/dma.h"
#include "kernel/dev/arch/cortexm/stm32f1xx/spi.h"

#include "dev_stm32f1xx_spi_x.h"

/*===========================================
Global Declaration
=============================================*/
const char dev_stm32f1xx_spi_1_name[]="spi0\0";

int dev_stm32f1xx_spi_1_load(void);
int dev_stm32f1xx_spi_1_open(desc_t desc, int o_flag);

extern int dev_stm32f1xx_spi_x_load(board_stm32f1xx_spi_info_t * uart_info);
extern int dev_stm32f1xx_spi_x_open(desc_t desc, int o_flag, board_stm32f1xx_spi_info_t * uart_info);
extern int dev_stm32f1xx_spi_x_close(desc_t desc);
extern int dev_stm32f1xx_spi_x_read(desc_t desc, char* buf,int cb);
extern int dev_stm32f1xx_spi_x_write(desc_t desc, const char* buf,int cb);
extern int dev_stm32f1xx_spi_x_ioctl(desc_t desc,int request,va_list ap);
extern int dev_stm32f1xx_spi_x_isset_read(desc_t desc);
extern int dev_stm32f1xx_spi_x_isset_write(desc_t desc);
extern int dev_stm32f1xx_spi_x_seek(desc_t desc,int offset,int origin);

dev_map_t dev_stm32f1xx_spi_1_map={
   dev_stm32f1xx_spi_1_name,
   S_IFCHR,
   dev_stm32f1xx_spi_1_load,
   dev_stm32f1xx_spi_1_open,
   dev_stm32f1xx_spi_x_close,
   dev_stm32f1xx_spi_x_isset_read,
   dev_stm32f1xx_spi_x_isset_write,
   dev_stm32f1xx_spi_x_read,
   dev_stm32f1xx_spi_x_write,
   dev_stm32f1xx_spi_x_seek,
   dev_stm32f1xx_spi_x_ioctl
};

#if (__tauon_compiler__==__compiler_keil_arm__)
   static const _Spi_Descriptor spi_descriptor[] = {
      {SPI1, RCC_APB2PeriphClockCmd, RCC_APB2Periph_SPI1, GPIO_MISO1, GPIO_MOSI1, GPIO_SCK1, 3, 20000000}   //SPI1
   };
   static board_stm32f1xx_spi_info_t stm32f1xx_spi_1;
#else

   static board_stm32f1xx_spi_info_t stm32f1xx_spi_1=
   {
      .spi_descriptor={SPI1, RCC_APB2PeriphClockCmd, RCC_APB2Periph_SPI1, GPIO_MISO1, GPIO_MOSI1, GPIO_SCK1, 3, 20000000}   // SPI1
   };
#endif


/*===========================================
Implementation
=============================================*/


/*-------------------------------------------
| Name:dev_stm32f1xx_spi_1_load
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_stm32f1xx_spi_1_load(void) {
   
   #if (__tauon_compiler__==__compiler_keil_arm__)
      memcpy(&stm32f1xx_spi_1.uart_descriptor,&spi_descriptor,sizeof(_Spi_Descriptor));
   #endif
   stm32f1xx_spi_1.spi_descriptor.board_spi_info=&stm32f1xx_spi_1;
   stm32f1xx_spi_1.desc_r=-1;
   stm32f1xx_spi_1.desc_w=-1;
   
   return dev_stm32f1xx_spi_x_load(&stm32f1xx_spi_1);
}
   
/*-------------------------------------------
| Name:dev_stm32f1xx_spi_1_open
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_stm32f1xx_spi_1_open(desc_t desc, int o_flag) {
   return dev_stm32f1xx_spi_x_open(desc, o_flag, &stm32f1xx_spi_1);
}

/*============================================
| End of Source  : dev_stm32f1xx_spi_1.c
==============================================*/