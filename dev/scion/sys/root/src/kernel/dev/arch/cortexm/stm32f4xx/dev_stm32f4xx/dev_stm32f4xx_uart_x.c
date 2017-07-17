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
#include "kernel/core/interrupt.h"
#include "kernel/core/kernel.h"
#include "kernel/core/system.h"
#include "kernel/core/ioctl.h"
#include "kernel/core/fcntl.h"
#include "kernel/core/cpu.h"
#include "kernel/core/core_rttimer.h"
#include "kernel/fs/vfs/vfstypes.h"


#include "kernel/dev/arch/cortexm/stm32f4xx/driverlib/stm32f4xx.h"
#include "kernel/dev/arch/cortexm/stm32f4xx/types.h"
#include "kernel/dev/arch/cortexm/stm32f4xx/gpio.h"
#include "kernel/dev/arch/cortexm/stm32f4xx/uart.h"

#include "dev_stm32f4xx_uart_x.h"

#include "lib/libc/termios/termios.h"

/*===========================================
Global Declaration
=============================================*/
int dev_stm32f4xx_uart_x_load(board_stm32f4xx_uart_info_t * uart_info);
int dev_stm32f4xx_uart_x_open(desc_t desc, int o_flag, board_stm32f4xx_uart_info_t * uart_info);

//
int dev_stm32f4xx_uart_x_isset_read(desc_t desc);
int dev_stm32f4xx_uart_x_isset_write(desc_t desc);
int dev_stm32f4xx_uart_x_close(desc_t desc);
int dev_stm32f4xx_uart_x_seek(desc_t desc,int offset,int origin);
int dev_stm32f4xx_uart_x_read(desc_t desc, char* buf,int cb);
int dev_stm32f4xx_uart_x_write(desc_t desc, const char* buf,int cb);
int dev_stm32f4xx_uart_x_ioctl(desc_t desc,int request,va_list ap);
int dev_stm32f4xx_uart_x_seek(desc_t desc,int offset,int origin);


/*===========================================
Implementation
=============================================*/
/*-------------------------------------------
| Name:dev_stm32f4xx_uart_x_load
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_stm32f4xx_uart_x_load(board_stm32f4xx_uart_info_t * uart_info){
  
   pthread_mutexattr_t mutex_attr=0;

   kernel_pthread_mutex_init(&uart_info->mutex,&mutex_attr);

   return 0;
}

/*-------------------------------------------
| Name:dev_stm32f4xx_uart_x_open
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_stm32f4xx_uart_x_open(desc_t desc, int o_flag,
                            board_stm32f4xx_uart_info_t * uart_info){
                          
   //
   if(uart_info->desc_r<0 && uart_info->desc_w<0) {
      if(uart_info->baudrate==0){
         uart_info->baudrate=115200;
      }
      //
      uart_open(&uart_info->uart_descriptor, uart_info->baudrate, 64, 256, 256, UART_HW_FLOW_CTRL_NONE, 0);
   }
   //
   if(o_flag & O_RDONLY) {
      if(uart_info->desc_r<0) {
         uart_info->desc_r = desc;
      }
      else
         return -1;                //already open
   }

   if(o_flag & O_WRONLY) {
      if(uart_info->desc_w<0) {
         uart_info->desc_w = desc;
      }
      else
         return -1;                //already open
   }

   if(!ofile_lst[desc].p)
      ofile_lst[desc].p=uart_info;

   //unmask IRQ
   if(uart_info->desc_r>=0 && uart_info->desc_w>=0) {
     
   }
   return 0;
}

/*-------------------------------------------
| Name:dev_stm32f4xx_uart_x_close
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_stm32f4xx_uart_x_close(desc_t desc){
  board_stm32f4xx_uart_info_t * p_uart_info = (board_stm32f4xx_uart_info_t*)ofile_lst[desc].p;
  //
  if(!p_uart_info)
   return -1;
  // 
  if(ofile_lst[desc].oflag & O_RDONLY) {
      if(!ofile_lst[desc].nb_reader) {
         p_uart_info->desc_r = -1;
      }
   }
   //
   if(ofile_lst[desc].oflag & O_WRONLY) {
      if(!ofile_lst[desc].nb_writer) {
         p_uart_info->desc_w = -1;
      }
   }
   //
   if(p_uart_info->desc_r<0 && p_uart_info->desc_w<0) {
      uart_close(&p_uart_info->uart_descriptor);
   }

   return 0;
}

/*-------------------------------------------
| Name:dev_stm32f4xx_uart_x_isset_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_stm32f4xx_uart_x_isset_read(desc_t desc){
   board_stm32f4xx_uart_info_t * p_uart_info = (board_stm32f4xx_uart_info_t*)ofile_lst[desc].p;
   _Uart_Descriptor *p_uart_descriptor;
   //
   if(!p_uart_info)
      return -1;
   
   p_uart_descriptor=&p_uart_info->uart_descriptor;
   
   if((*p_uart_descriptor->Ctrl)->RxCnt)
     return 0;

   return -1;
}

/*-------------------------------------------
| Name:dev_stm32f4xx_uart_x_isset_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_stm32f4xx_uart_x_isset_write(desc_t desc){
   board_stm32f4xx_uart_info_t * p_uart_info = (board_stm32f4xx_uart_info_t*)ofile_lst[desc].p;
   _Uart_Descriptor *p_uart_descriptor;
   //
   if(!p_uart_info)
      return -1;
   
   p_uart_descriptor=&p_uart_info->uart_descriptor;
   
   if(!(*p_uart_descriptor->Ctrl)->TxCnt)
     return 0;

   return -1;
}

/*-------------------------------------------
| Name:dev_stm32f4xx_uart_x_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_stm32f4xx_uart_x_read(desc_t desc, char* buf,int size){
   int cb;
   board_stm32f4xx_uart_info_t * p_uart_info = (board_stm32f4xx_uart_info_t*)ofile_lst[desc].p;
   _Uart_Descriptor *p_uart_descriptor;
   //
   if(!p_uart_info)
      return -1;
   // 
   p_uart_descriptor=&p_uart_info->uart_descriptor;
   //
   cb=uart_read(buf,size,p_uart_descriptor);
   //
   return cb;
}

/*-------------------------------------------
| Name:dev_stm32f4xx_uart_x_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_stm32f4xx_uart_x_write(desc_t desc, const char* buf,int size){
    int cb;
   board_stm32f4xx_uart_info_t * p_uart_info = (board_stm32f4xx_uart_info_t*)ofile_lst[desc].p;
   _Uart_Descriptor *p_uart_descriptor;
   //
   if(!p_uart_info)
      return -1;
   // 
   p_uart_descriptor=&p_uart_info->uart_descriptor;
   //
   cb=uart_write(buf,size,p_uart_descriptor);
   //
   return cb;
}

/*-------------------------------------------
| Name:dev_stm32f4xx_uart_x_seek
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_stm32f4xx_uart_x_seek(desc_t desc,int offset,int origin){
   return 0;
}

/*-------------------------------------------
| Name:dev_stm32f4xx_uart_x_ioctl
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_stm32f4xx_uart_x_ioctl(desc_t desc,int request,va_list ap) {
   struct termios* termios_p = (struct termios*)0;

   switch(request) {
      case TCFLSH: {
         //
         int flush_io= va_arg( ap, int);
         board_stm32f4xx_uart_info_t * p_uart_info = (board_stm32f4xx_uart_info_t*)ofile_lst[desc].p;
         _Uart_Descriptor *p_uart_descriptor;
         //
         if(!p_uart_info)
            return -1;
         // 
         p_uart_descriptor=&p_uart_info->uart_descriptor;
         //
         if(flush_io>2)
            return -1;
         //TCIFLUSH 0 -> 1:01
         //TCIFLUSH 1 -> 2:10
         //TCOFLUSH 2 -> 3:11
         flush_io+=1;
         //
         if( (flush_io&(TCIFLUSH+1)) ) {
            if( !(ofile_lst[desc].oflag & O_RDONLY) )
               return -1;   //not compatible open mode
            //uart_flush_rx(p_uart_descriptor);
            uint8_t c;
            int cb;
            while((cb=uart_read(&c,1,p_uart_descriptor))>0);
         }
         if( (flush_io&(TCOFLUSH+1)) ) {
            if( !(ofile_lst[desc].oflag & O_WRONLY) )
               return -1;   //not compatible open mode
            //
            //nothing to do
         }
      }
      break;
      
      //
      case I_LINK:
      case I_UNLINK:
        //nothing to do, see _vfs_ioctl2() 
        //ioctl of device driver is call with request I_LINK, I_UNLINK.
      return 0;

      //
      default:
         return -1;

   }

   return 0;
}


/*============================================
| End of Source  : dev_stm32f4xx_uart_x.c
==============================================*/