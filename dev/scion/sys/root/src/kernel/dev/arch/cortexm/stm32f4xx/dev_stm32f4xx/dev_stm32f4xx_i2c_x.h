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


/*============================================
| Compiler Directive
==============================================*/
#ifndef __DEV_STM32F4XX_I2C_X_H__
#define __DEV_STM32F4XX_I2C_X_H__


/*============================================
| Includes
==============================================*/


/*============================================
| Declaration
==============================================*/

typedef struct board_stm32f4xx_i2c_info_st {
   uint32_t bus_speed;
   uint8_t own_address;
   //
   uint8_t current_slave_address;
   
   //
   desc_t desc_r;
   desc_t desc_w;
   
   //I2C
   I2C_HandleTypeDef i2c_handle;
   //
   
   //DMA
   //
   DMA_Stream_TypeDef * rx_dma_stream;
   IRQn_Type rx_dma_stream_irq_no;
   uint32_t rx_dma_channel;
  
   //
   DMA_Stream_TypeDef * tx_dma_stream;
   IRQn_Type tx_dma_stream_irq_no;
   uint32_t tx_dma_channel;
   //
   int i2c_rxtx_buffer_size;
   uint8_t* p_i2c_rxtx_buffer; 
   //
   DMA_HandleTypeDef hdma_tx;
   DMA_HandleTypeDef hdma_rx;
   
   //
   kernel_pthread_mutex_t mutex;
} board_stm32f4xx_i2c_info_t;


#endif //__DEV_STM32F4XX_UART_H__