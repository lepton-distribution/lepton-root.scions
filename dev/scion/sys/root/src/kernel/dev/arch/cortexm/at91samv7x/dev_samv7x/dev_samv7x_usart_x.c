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




/*===========================================
Implementation
=============================================*/
/*-------------------------------------------
| Name:samv71x_usart_x_rx_callback
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static void samv71x_usart_x_rx_callback(uint32_t channel, samv71x_usart_info_t *usart_info)
{
   UsartDma *pUsartd = &usart_info->Usartd;
	UsartChannel *pUsartdCh = pUsartd->pRxChannel;

   //
	if (channel != pUsartdCh->ChNum)
		return;
   
   //from empty state to not empty
   if(usart_info->rx_dma_buf_r==usart_info->rx_dma_buf_w){
      __fire_io_int(ofile_lst[usart_info->desc_r].owner_pthread_ptr_read);
   }
   //
   uint32_t current_dest_addr=XDMAC_GetChDestinationAddr(pUsartd->pXdmad->pXdmacs, pUsartdCh->ChNum);
   usart_info->rx_dma_buf_w= (current_dest_addr-(uint32_t)usart_info->pRxDmaBuffer);
           
   //
	SCB_InvalidateDCache_by_Addr((uint32_t *)pUsartdCh->pBuff, usart_info->RxDmaBufferSize);
}

/*-------------------------------------------
| Name:samv71x_usart_x_tx_callback
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static void samv71x_usart_x_tx_callback(uint32_t channel, samv71x_usart_info_t *usart_info)
{
   UsartDma *pUsartd = &usart_info->Usartd;
	UsartChannel *pUsartdCh = pUsartd->pTxChannel;

   //
	if (channel != pUsartdCh->ChNum)
		return;
   //
   __fire_io_int(ofile_lst[usart_info->desc_w].owner_pthread_ptr_write);
}


/*-------------------------------------------
| Name:dev_samv71x_usart_x_load
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_samv71x_usart_x_load(samv71x_usart_info_t * usart_info){
   return 0;
}

/*-------------------------------------------
| Name:dev_samv71x_usart_x_open
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_samv71x_usart_x_open(desc_t desc, int o_flag, samv71x_usart_info_t* usart_info){

   //
   if(usart_info->desc_r<0 && usart_info->desc_w<0) {
      if(usart_info->baudrate==0){
         usart_info->baudrate=115200;
      }
      //
      uint32_t mode = 0
            | US_MR_USART_MODE_NORMAL
				| US_MR_CHRL_8_BIT
				| US_MR_PAR_NO
				| US_MR_NBSTOP_1_BIT
				| US_MR_CHMODE_NORMAL;
		//| UART_MR_CHMODE_LOCAL_LOOPBACK;
      //
      //dmad.pXdmacs = XDMAC; //lepton: do in /dev/cpu0 
      //
      memset(&usart_info->UsartRx, 0, sizeof(UsartChannel));
      memset(&usart_info->UsartTx, 0, sizeof(UsartChannel));
      //rx dma ring buffer 
      usart_info->UsartRx.dmaProgrammingMode = XDMAD_LLI;
      usart_info->UsartRx.dmaBlockSize = usart_info->RxDmaBufferSize;
      usart_info->UsartRx.dmaRingBuffer = 1;
      usart_info->rx_dma_buf_w=0;
      usart_info->rx_dma_buf_r=0;
      //
      usart_info->UsartRx.BuffSize= 1; //use ring buffer  
      usart_info->UsartRx.pBuff = usart_info->pRxDmaBuffer;
      //
      usart_info->UsartTx.dmaProgrammingMode = XDMAD_SINGLE;
      usart_info->UsartTx.BuffSize = usart_info->TxDmaBufferSize;
      usart_info->UsartTx.pBuff = usart_info->pTxDmaBuffer;
      
      //not used
      usart_info->UsartTx.dmaProgress = 1;
      usart_info->UsartRx.dmaProgress = 1;
      //
      usart_info->Usartd.pRxChannel = &usart_info->UsartRx;
      usart_info->Usartd.pTxChannel = &usart_info->UsartTx;
      usart_info->Usartd.pXdmad = &g_samv7x_dmad;
      PMC_EnablePeripheral(usart_info->dwBaseId);
      //
      usart_info->UsartRx.callback    = (UsartdCallback)samv71x_usart_x_rx_callback;
      usart_info->UsartRx.pArgument   = usart_info;
      //
      usart_info->UsartTx.callback    = (UsartdCallback)samv71x_usart_x_tx_callback;
      usart_info->UsartTx.pArgument   = usart_info;
      //
#if USE_SAMV7_UART_USER_KERNEL_RING_BUFFER
      kernel_ring_buffer_init(&usart_info->kernel_ring_buffer_rx,usart_info->pRxRingBuffer,usart_info->RxRingBufferSize);
      kernel_ring_buffer_init(&usart_info->kernel_ring_buffer_tx,usart_info->pTxRingBuffer,usart_info->TxRingBufferSize);
#endif
      //
      USARTD_Configure(&usart_info->Usartd, usart_info->dwBaseId, mode, usart_info->baudrate, BOARD_MCK);
   }
   //
   if(o_flag & O_RDONLY) {
      if(usart_info->desc_r<0) {
         usart_info->desc_r = desc;
         //
         USARTD_EnableRxChannels(&usart_info->Usartd, &usart_info->UsartRx);
         //
         USARTD_RcvData(&usart_info->Usartd);	
      }
      else
         return -1;                //already open
   }

   if(o_flag & O_WRONLY) {
      if(usart_info->desc_w<0) {
         usart_info->desc_w = desc;
         //
         USARTD_EnableTxChannels(&usart_info->Usartd, &usart_info->UsartTx);
      }
      else
         return -1;                //already open
   }

   if(!ofile_lst[desc].p)
      ofile_lst[desc].p=usart_info;

   //unmask IRQ
   if(usart_info->desc_r>=0 && usart_info->desc_w>=0) {
     
   }
   //
   return 0;
}

/*-------------------------------------------
| Name:dev_samv71x_usart_x_close
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_samv71x_usart_x_close(desc_t desc){
   return 0;
}

/*-------------------------------------------
| Name:dev_samv71x_usart_x_isset_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_samv71x_usart_x_isset_read(desc_t desc){
   samv71x_usart_info_t* usart_info = (samv71x_usart_info_t*)ofile_lst[desc].p;
   if(usart_info->rx_dma_buf_r!=usart_info->rx_dma_buf_w){
      return 0;
   }
   return -1;
}

/*-------------------------------------------
| Name:dev_samv71x_usart_x_isset_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_samv71x_usart_x_isset_write(desc_t desc){
   samv71x_usart_info_t* usart_info = (samv71x_usart_info_t*)ofile_lst[desc].p;
   //
   UsartDma *pUsartd = &usart_info->Usartd;
	UsartChannel *pUsartdCh = pUsartd->pTxChannel;
   
   //
   if(XDMAD_IsTransferDone(pUsartd->pXdmad, pUsartdCh->ChNum)==XDMAD_OK){
      return 0;
   }
   //
   return -1;
}
/*-------------------------------------------
| Name:dev_samv71x_usart_x_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_samv71x_usart_x_read(desc_t desc, char* buf,int size){
   samv71x_usart_info_t* usart_info = (samv71x_usart_info_t*)ofile_lst[desc].p;
   //
   int16_t r;
   int16_t w;
   int16_t sz;
   int16_t available_data_sz;
   uint8_t* p_ring_buffer;
   
   //
   r=  usart_info->rx_dma_buf_r;
   w=  usart_info->rx_dma_buf_w;
   sz =  usart_info->RxDmaBufferSize;
   
   p_ring_buffer =  usart_info->pRxDmaBuffer;

   //available data
   if(r<w)
      available_data_sz=(w-r);
   else
      available_data_sz=(sz-r)+w;

   //
   if (available_data_sz == 0)
      return 0;
   
   //
   if (available_data_sz<size) {
      //copy all available data in user buffer
      size = available_data_sz; 
   }
   
   //
   if( (r+size) <= sz ){
      memcpy(buf,p_ring_buffer+r,size);
      r = ((r+size)==sz)? 0 : r+size;
   }else{
      memcpy(buf,p_ring_buffer+r,sz-r);
      memcpy(buf+(sz-r),p_ring_buffer+(sz-r),size-(sz-r));
      r=size-(sz-r);
   }
   //
   usart_info->rx_dma_buf_r=r;
   //
   return size;
}

/*-------------------------------------------
| Name:dev_samv71x_usart_x_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_samv71x_usart_x_write(desc_t desc, const char* buf,int size){
   samv71x_usart_info_t* usart_info = (samv71x_usart_info_t*)ofile_lst[desc].p;
   //
   UsartDma *pUsartd = &usart_info->Usartd;
	UsartChannel *pUsartdCh = pUsartd->pTxChannel;
   
   //
   if(size>usart_info->TxDmaBufferSize){
      size = usart_info->TxDmaBufferSize;
   }
   //
   memcpy(usart_info->pTxDmaBuffer,buf,size);
   //
   XDMAC_SetSourceAddr(pUsartd->pXdmad->pXdmacs, pUsartdCh->ChNum,(uint32_t)usart_info->pTxDmaBuffer);
   XDMAC_SetMicroblockControl(pUsartd->pXdmad->pXdmacs, pUsartdCh->ChNum,size);
   //
   USARTD_SendData(&usart_info->Usartd);
   return size;
}

/*-------------------------------------------
| Name:dev_a0350_board_seek
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_samv71x_usart_x_seek(desc_t desc,int offset,int origin){
   return -1;
}

/*-------------------------------------------
| Name:dev_samv71x_usart_x_ioctl
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_samv71x_usart_x_ioctl(desc_t desc,int request,va_list ap){
   return 0;
}

/*============================================
| End of Source  : dev_samv71x_usart_x.c
==============================================*/