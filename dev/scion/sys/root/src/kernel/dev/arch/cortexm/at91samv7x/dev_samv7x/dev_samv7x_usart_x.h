/*
The contents of this file are subject to the Mozilla Public License Version 1.1
(the "License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at http://www.mozilla.org/MPL/

Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for the
specific language governing rights and limitations under the License.

The Original Code is Lepton.

The Initial Developer of the Original Code is Philippe Le Boulanger.
Portions created by Philippe Le Boulanger are Copyright (C) 2015 <lepton.phlb@gmail.com>.
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


/*============================================
| Compiler Directive
==============================================*/
#ifndef __DEV_SAMV7X_USART_X_H__
#define __DEV_SAMV7X_USART_X_H__


/*============================================
| Includes
==============================================*/


/*============================================
| Declaration
==============================================*/

typedef struct samv71x_usart_info_st {
   uint32_t dwBaseId; //ex:ID_UART4
   Usart *BaseUsart;  // ex:UART4
   //
   const Pin* base_usart_pins;
   //
   int desc_r;
   int desc_w;
   //
   unsigned long baudrate;
   
   //Dma Buffer
   uint16_t RxDmaBufferSize;
   uint8_t* pRxDmaBuffer;
   //
   uint16_t TxDmaBufferSize;
   uint8_t* pTxDmaBuffer;
   
   //private
   /** Global DMA driver for all transfer */
   UsartDma Usartd;
   //
   UsartChannel UsartRx;
   UsartChannel UsartTx; 
   //
   uint8_t  rx_dma_buf_w;
   uint8_t  rx_dma_buf_r;
     //
#if USE_SAMV7_USART_USER_KERNEL_RING_BUFFER
   //
   kernel_ring_buffer_t kernel_ring_buffer_rx;
   kernel_ring_buffer_t kernel_ring_buffer_tx;
   
   //Ring Buffer
   uint16_t RxRingBufferSize;
   uint8_t* pRxRingBuffer;
   //
   uint16_t TxRingBufferSize;
   uint8_t* pTxRingBuffer;
#endif
} samv71x_usart_info_t;


int dev_samv71x_usart_x_load(samv71x_usart_info_t * uart_info);
int dev_samv71x_usart_x_open(desc_t desc, int o_flag, samv71x_usart_info_t* uart_info);

int dev_samv71x_usart_x_close(desc_t desc);
int dev_samv71x_usart_x_isset_read(desc_t desc);
int dev_samv71x_usart_x_isset_write(desc_t desc);
int dev_samv71x_usart_x_read(desc_t desc, char* buf,int size);
int dev_samv71x_usart_x_write(desc_t desc, const char* buf,int size);
int dev_samv71x_usart_x_seek(desc_t desc,int offset,int origin);
int dev_samv71x_usart_x_ioctl(desc_t desc,int request,va_list ap);
 


#endif //__DEV_SAMV7X_USART_X_H__
