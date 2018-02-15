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
#include "kernel/dev/arch/cortexm/at91samv7x/dev_samv7x/dev_samv7x_uart_x.h"

/*===========================================
Global Declaration
=============================================*/




/*===========================================
Implementation
=============================================*/
/*-------------------------------------------
| Name:samv71x_uart_x_rx_callback
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static void samv71x_uart_x_rx_callback(uint32_t channel, samv71x_uart_info_t *uart_info)
{
   UartDma *pUartd = &uart_info->Uartd;
	UartChannel *pUartdCh = pUartd->pRxChannel;

   //
	if (channel != pUartdCh->ChNum)
		return;
   
   //from empty state to not empty
   if(uart_info->rx_dma_buf_r==uart_info->rx_dma_buf_w){
      __fire_io_int(ofile_lst[uart_info->desc_r].owner_pthread_ptr_read);
   }
   //
   uint32_t current_dest_addr=XDMAC_GetChDestinationAddr(pUartd->pXdmad->pXdmacs, pUartdCh->ChNum);
   uart_info->rx_dma_buf_w= (current_dest_addr-(uint32_t)uart_info->pRxDmaBuffer);
           
   //
	SCB_InvalidateDCache_by_Addr((uint32_t *)pUartdCh->pBuff,  uart_info->RxDmaBufferSize);
}

/*-------------------------------------------
| Name:samv71x_uart_x_tx_callback
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static void samv71x_uart_x_tx_callback(uint32_t channel, samv71x_uart_info_t *uart_info)
{
   UartDma *pUartd = &uart_info->Uartd;
	UartChannel *pUartdCh = pUartd->pTxChannel;

   //
	if (channel != pUartdCh->ChNum)
		return;
   //
   __fire_io_int(ofile_lst[uart_info->desc_w].owner_pthread_ptr_write);
}


/*-------------------------------------------
| Name:dev_samv71x_uart_x_load
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_samv71x_uart_x_load(samv71x_uart_info_t * uart_info){
   return 0;
}

/*-------------------------------------------
| Name:dev_samv71x_uart_x_open
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_samv71x_uart_x_open(desc_t desc, int o_flag, samv71x_uart_info_t* uart_info){

   //
   if(uart_info->desc_r<0 && uart_info->desc_w<0) {
      if(uart_info->baudrate==0){
         uart_info->baudrate=115200;
      }
      //
      uint32_t mode = 0
		| UART_MR_PAR_NO
		| UART_MR_BRSRCCK_PERIPH_CLK
      //|UART_MR_CHMODE_REMOTE_LOOPBACK;
      | UART_MR_CHMODE_NORMAL;
		//| UART_MR_CHMODE_LOCAL_LOOPBACK;
      //
      //dmad.pXdmacs = XDMAC; //lepton: do in /dev/cpu0 
      //
      memset(&uart_info->UartRx, 0, sizeof(UartChannel));
      memset(&uart_info->UartTx, 0, sizeof(UartChannel));
      //rx dma ring buffer 
      uart_info->UartRx.dmaProgrammingMode = XDMAD_LLI;
      uart_info->UartRx.dmaBlockSize = uart_info->RxDmaBufferSize;
      uart_info->UartRx.dmaRingBuffer = 1;
      uart_info->rx_dma_buf_w=0;
      uart_info->rx_dma_buf_r=0;
      //
      uart_info->UartRx.BuffSize= 1; //use ring buffer  
      uart_info->UartRx.pBuff = uart_info->pRxDmaBuffer;
      //
      uart_info->UartTx.dmaProgrammingMode = XDMAD_SINGLE;
      uart_info->UartTx.BuffSize = uart_info->TxDmaBufferSize;
      uart_info->UartTx.pBuff = uart_info->pTxDmaBuffer;
      
      //
      uart_info->UartTx.sempaphore = 1;
      uart_info->UartRx.sempaphore = 1;
      //
      uart_info->Uartd.pRxChannel = &uart_info->UartRx;
      uart_info->Uartd.pTxChannel = &uart_info->UartTx;
      uart_info->Uartd.pXdmad = &g_samv7x_dmad;
      PMC_EnablePeripheral(uart_info->dwBaseId);
      //
      uart_info->UartRx.callback    = (UartdCallback)samv71x_uart_x_rx_callback;
      uart_info->UartRx.pArgument   = uart_info;
      //
      uart_info->UartTx.callback    = (UartdCallback)samv71x_uart_x_tx_callback;
      uart_info->UartTx.pArgument   = uart_info;
      //
#if USE_SAMV7_UART_USER_KERNEL_RING_BUFFER
      kernel_ring_buffer_init(&uart_info->kernel_ring_buffer_rx,uart_info->pRxRingBuffer,uart_info->RxRingBufferSize);
      kernel_ring_buffer_init(&uart_info->kernel_ring_buffer_tx,uart_info->pTxRingBuffer,uart_info->TxRingBufferSize);
#endif
      //
      UARTD_Configure(&uart_info->Uartd, uart_info->dwBaseId, mode, uart_info->baudrate, BOARD_MCK);
   }
   //
   if(o_flag & O_RDONLY) {
      if(uart_info->desc_r<0) {
         uart_info->desc_r = desc;
         //
         UARTD_EnableRxChannels(&uart_info->Uartd, &uart_info->UartRx);
         //
         UARTD_RcvData(&uart_info->Uartd);	
      }
      else
         return -1;                //already open
   }

   if(o_flag & O_WRONLY) {
      if(uart_info->desc_w<0) {
         uart_info->desc_w = desc;
         //
         UARTD_EnableTxChannels(&uart_info->Uartd, &uart_info->UartTx);
      }
      else
         return -1;                //already open
   }

   if(!ofile_lst[desc].p)
      ofile_lst[desc].p=uart_info;

   //unmask IRQ
   if(uart_info->desc_r>=0 && uart_info->desc_w>=0) {
     
   }
   //
   return 0;
}

/*-------------------------------------------
| Name:dev_samv71x_uart_x_close
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_samv71x_uart_x_close(desc_t desc){
   
   samv71x_uart_info_t* uart_info = (samv71x_uart_info_t*)ofile_lst[desc].p;
   
   //
   if(ofile_lst[desc].oflag& O_RDONLY){
      if(ofile_lst[desc].nb_reader==0){
         UARTD_DisableRxChannels(&uart_info->Uartd, &uart_info->UartRx);
         //
         //
         uart_info->desc_r = INVALID_DESC;
      }
   }
   
   //
   if(ofile_lst[desc].oflag& O_WRONLY){
      if(ofile_lst[desc].nb_writer==0){
         UARTD_DisableTxChannels(&uart_info->Uartd, &uart_info->UartTx);
         //
         uart_info->desc_w = INVALID_DESC;
      }
   }
   
   return 0;
}

/*-------------------------------------------
| Name:dev_samv71x_uart_x_isset_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_samv71x_uart_x_isset_read(desc_t desc){
   samv71x_uart_info_t* uart_info = (samv71x_uart_info_t*)ofile_lst[desc].p;
   if(uart_info->rx_dma_buf_r!=uart_info->rx_dma_buf_w){
      return 0;
   }
   return -1;
}

/*-------------------------------------------
| Name:dev_samv71x_uart_x_isset_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_samv71x_uart_x_isset_write(desc_t desc){
   samv71x_uart_info_t* uart_info = (samv71x_uart_info_t*)ofile_lst[desc].p;
   //
   UartDma *pUartd = &uart_info->Uartd;
	UartChannel *pUartdCh = pUartd->pTxChannel;
   
   //
   if(XDMAD_IsTransferDone(pUartd->pXdmad, pUartdCh->ChNum)==XDMAD_OK){
      return 0;
   }
   //
   return -1;
}
/*-------------------------------------------
| Name:dev_samv71x_uart_x_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_samv71x_uart_x_read(desc_t desc, char* buf,int size){
   samv71x_uart_info_t* uart_info = (samv71x_uart_info_t*)ofile_lst[desc].p;
   //
   int16_t r;
   int16_t w;
   int16_t sz;
   int16_t available_data_sz;
   uint8_t* p_ring_buffer;
   
   //
   r=  uart_info->rx_dma_buf_r;
   w=  uart_info->rx_dma_buf_w;
   sz =  uart_info->RxDmaBufferSize;
   
   p_ring_buffer =  uart_info->pRxDmaBuffer;
   
   //empty or overrun
   if(r==w){
      return 0;
   }

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
   uart_info->rx_dma_buf_r=r;
   //
   return size;
}

/*-------------------------------------------
| Name:dev_samv71x_uart_x_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_samv71x_uart_x_write(desc_t desc, const char* buf,int size){
   samv71x_uart_info_t* uart_info = (samv71x_uart_info_t*)ofile_lst[desc].p;
   //
   UartDma *pUartd = &uart_info->Uartd;
	UartChannel *pUartdCh = pUartd->pTxChannel;
   
   //
   if(size>uart_info->TxDmaBufferSize){
      size = uart_info->TxDmaBufferSize;
   }
   //
   SCB_CleanInvalidateDCache();
   //
   memcpy(uart_info->pTxDmaBuffer,buf,size);
   //
   SCB_InvalidateDCache_by_Addr((uint32_t*)uart_info->pTxDmaBuffer, size);
   //
   XDMAC_SetSourceAddr(pUartd->pXdmad->pXdmacs, pUartdCh->ChNum,(uint32_t)uart_info->pTxDmaBuffer);
   XDMAC_SetMicroblockControl(pUartd->pXdmad->pXdmacs, pUartdCh->ChNum,size);
   //
   UARTD_SendData(&uart_info->Uartd);
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
int dev_samv71x_uart_x_seek(desc_t desc,int offset,int origin){
   return -1;
}

/*-------------------------------------------
| Name:dev_samv71x_uart_x_ioctl
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_samv71x_uart_x_ioctl(desc_t desc,int request,va_list ap){
   return 0;
}

/*============================================
| End of Source  : dev_samv71x_uart_x.c
==============================================*/