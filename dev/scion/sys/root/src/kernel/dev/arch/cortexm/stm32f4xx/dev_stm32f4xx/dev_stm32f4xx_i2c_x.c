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
#include <stdarg.h>

#include "kernel/core/kernelconf.h"
#include "kernel/core/types.h"
#include "kernel/core/dirent.h"
#include "kernel/core/interrupt.h"
#include "kernel/core/kernel.h"
#include "kernel/core/system.h"
#include "kernel/core/fcntl.h"
#include "kernel/core/stat.h"
#include "kernel/core/ioctl.h"
#include "kernel/core/ioctl_i2c.h"

#include "kernel/fs/vfs/vfstypes.h"

#include "stm32f4xx_hal.h"


#include "dev_stm32f4xx_i2c_x.h"


/*===========================================
Global Declaration
=============================================*/
int dev_stm32f4xx_i2c_x_load(board_stm32f4xx_i2c_info_t * i2c_info);
int dev_stm32f4xx_i2c_x_open(desc_t desc, int o_flag, board_stm32f4xx_i2c_info_t * i2c_info);

//
int dev_stm32f4xx_i2c_x_isset_read(desc_t desc);
int dev_stm32f4xx_i2c_x_isset_write(desc_t desc);
int dev_stm32f4xx_i2c_x_close(desc_t desc);
int dev_stm32f4xx_i2c_x_seek(desc_t desc,int offset,int origin);
int dev_stm32f4xx_i2c_x_read(desc_t desc, char* buf,int cb);
int dev_stm32f4xx_i2c_x_write(desc_t desc, const char* buf,int cb);
int dev_stm32f4xx_i2c_x_ioctl(desc_t desc,int request,va_list ap);
int dev_stm32f4xx_i2c_x_seek(desc_t desc,int offset,int origin);

#define STM32_I2C_DEVICE_MAX 2

typedef enum
{
   __stm32f4xx_i2c_1__ = 0,
   __stm32f4xx_i2c_2__ = 1
}stm32f4xx_i2c_no_t;
static board_stm32f4xx_i2c_info_t * board_stm32f4xx_i2c_info_list[STM32_I2C_DEVICE_MAX]={0};

/*===========================================
Implementation
=============================================*/


/**
  * @brief  This function handles I2C event interrupt request.  
  * @param  None
  * @retval None
  * @Note   This function is redefined in "main.h" and related to I2C data transmission     
  */
void I2C1_EV_IRQHandler(void)
{
  HAL_I2C_EV_IRQHandler(&board_stm32f4xx_i2c_info_list[__stm32f4xx_i2c_1__]->i2c_handle);
}

/**
  * @brief  This function handles I2C error interrupt request.
  * @param  None
  * @retval None
  * @Note   This function is redefined in "main.h" and related to I2C error
  */
void I2C1_ER_IRQHandler(void)
{
  HAL_I2C_ER_IRQHandler(&board_stm32f4xx_i2c_info_list[__stm32f4xx_i2c_1__]->i2c_handle);
  
}


/**
  * @brief  This function handles I2C event interrupt request.  
  * @param  None
  * @retval None
  * @Note   This function is redefined in "main.h" and related to I2C data transmission     
  */
void I2C2_EV_IRQHandler(void)
{
  HAL_I2C_EV_IRQHandler(&board_stm32f4xx_i2c_info_list[__stm32f4xx_i2c_2__]->i2c_handle);
}

/**
  * @brief  This function handles I2C error interrupt request.
  * @param  None
  * @retval None
  * @Note   This function is redefined in "main.h" and related to I2C error
  */
void I2C2_ER_IRQHandler(void)
{
  HAL_I2C_ER_IRQHandler(&board_stm32f4xx_i2c_info_list[__stm32f4xx_i2c_2__]->i2c_handle);
  
}

  
/**
  * @brief  Tx Transfer completed callback.
  * @param  I2cHandle: I2C handle
  * @note   This example shows a simple way to report end of DMA Tx transfer, and 
  *         you can add your own implementation. 
  * @retval None
  */
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *I2cHandle){
  /*Transfer in transmission process is correct */
}


/**
  * @brief  Rx Transfer completed callback.
  * @param  I2cHandle: I2C handle
  * @note   This example shows a simple way to report end of DMA Rx transfer, and 
  *         you can add your own implementation.
  * @retval None
  */
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *I2cHandle){
  /*Transfer in reception process is correct */
}

/**
  * @brief  I2C error callbacks.
  * @param  I2cHandle: I2C handle
  * @note   This example shows a simple way to report transfer error, and you can
  *         add your own implementation.
  * @retval None
  */
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *I2cHandle){
  /* Transfer error in reception/transmission process */

}


/*-------------------------------------------
| Name:dev_stm32f4xx_i2c_x_load
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_stm32f4xx_i2c_x_load(board_stm32f4xx_i2c_info_t * i2c_info){
  
   pthread_mutexattr_t mutex_attr=0;
   kernel_pthread_mutex_init(&i2c_info->mutex,&mutex_attr);

   return 0;
}

/*-------------------------------------------
| Name:dev_stm32f4xx_i2c_x_open
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_stm32f4xx_i2c_x_open(desc_t desc, int o_flag,
                            board_stm32f4xx_i2c_info_t * i2c_info){
                          
   //
   if(i2c_info->desc_r<0 && i2c_info->desc_w<0) {
      if(HAL_I2C_GetState(&i2c_info->i2c_handle) == HAL_I2C_STATE_RESET){
         /* DISCOVERY_I2Cx peripheral configuration */
         i2c_info->i2c_handle.Init.ClockSpeed = i2c_info->bus_speed;
         i2c_info->i2c_handle.Init.DutyCycle = I2C_DUTYCYCLE_2;
         i2c_info->i2c_handle.Init.OwnAddress1 = i2c_info->own_address;//0x33;
         i2c_info->i2c_handle.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
          
         //
         if(i2c_info->i2c_handle.Instance==I2C1){
            //
            board_stm32f4xx_i2c_info_list[__stm32f4xx_i2c_1__]=i2c_info;
            
            /* Enable the DISCOVERY_I2Cx peripheral clock */
            __HAL_RCC_I2C1_CLK_ENABLE();

            /* Force the I2C peripheral clock reset */
            __HAL_RCC_I2C1_FORCE_RESET();

            /* Release the I2C peripheral clock reset */
            __HAL_RCC_I2C1_RELEASE_RESET();
         }else if(i2c_info->i2c_handle.Instance==I2C2){
             //
            board_stm32f4xx_i2c_info_list[__stm32f4xx_i2c_2__]=i2c_info;
            
            /* Enable the DISCOVERY_I2Cx peripheral clock */
            __HAL_RCC_I2C2_CLK_ENABLE();

            /* Force the I2C peripheral clock reset */
            __HAL_RCC_I2C2_FORCE_RESET();

            /* Release the I2C peripheral clock reset */
            __HAL_RCC_I2C2_RELEASE_RESET();
            
         }else{
            return -1;
         }
      
         /*Enable DMA peripheral Clock ########################################*/   
         /* Enable DMA1clock */
         __HAL_RCC_DMA1_CLK_ENABLE();

         /*##-5- Configure the DMA streams ##########################################*/
         /* Configure the DMA handler for Transmission process */
         i2c_info->hdma_tx.Instance                 = i2c_info->tx_dma_stream;
         i2c_info->hdma_tx.Init.Channel             = i2c_info->tx_dma_channel;
         i2c_info->hdma_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
         i2c_info->hdma_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
         i2c_info->hdma_tx.Init.MemInc              = DMA_MINC_ENABLE;
         i2c_info->hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
         i2c_info->hdma_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
         i2c_info->hdma_tx.Init.Mode                = DMA_NORMAL;
         i2c_info->hdma_tx.Init.Priority            = DMA_PRIORITY_LOW;
         i2c_info->hdma_tx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;         
         i2c_info->hdma_tx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
         i2c_info->hdma_tx.Init.MemBurst            = DMA_MBURST_INC4;
         i2c_info->hdma_tx.Init.PeriphBurst         = DMA_PBURST_INC4;

         HAL_DMA_Init(&i2c_info->hdma_tx);   

         /* Associate the initialized DMA handle to the the I2C handle */
         __HAL_LINKDMA(&i2c_info->i2c_handle, hdmatx, i2c_info->hdma_tx);

         /* Configure the DMA handler for Transmission process */
         i2c_info->hdma_rx.Instance                 = i2c_info->rx_dma_stream;
         i2c_info->hdma_rx.Init.Channel             = i2c_info->rx_dma_channel;
         i2c_info->hdma_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
         i2c_info->hdma_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
         i2c_info->hdma_rx.Init.MemInc              = DMA_MINC_ENABLE;
         i2c_info->hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
         i2c_info->hdma_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
         i2c_info->hdma_rx.Init.Mode                = DMA_NORMAL;
         i2c_info->hdma_rx.Init.Priority            = DMA_PRIORITY_HIGH;
         i2c_info->hdma_rx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;         
         i2c_info->hdma_rx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
         i2c_info->hdma_rx.Init.MemBurst            = DMA_MBURST_INC4;
         i2c_info->hdma_rx.Init.PeriphBurst         = DMA_PBURST_INC4; 

         HAL_DMA_Init(&i2c_info->hdma_rx);

         /* Associate the initialized DMA handle to the the I2C handle */
         __HAL_LINKDMA(&i2c_info->i2c_handle, hdmarx, i2c_info->hdma_rx);

         /*##-6- Configure the NVIC for DMA #########################################*/
         /* NVIC configuration for DMA transfer complete interrupt (I2C1_TX) */
         NVIC_SetPriority(i2c_info->tx_dma_stream_irq_no, (1 << __NVIC_PRIO_BITS) -3);  
         //HAL_NVIC_SetPriority(i2c_info->tx_dma_stream_irq_no, 0, 1);
         HAL_NVIC_EnableIRQ(i2c_info->tx_dma_stream_irq_no);
        
         /* NVIC configuration for DMA transfer complete interrupt (I2C1_RX) */
         NVIC_SetPriority(i2c_info->rx_dma_stream_irq_no, (1 << __NVIC_PRIO_BITS) -3); 
         //HAL_NVIC_SetPriority(i2c_info->rx_dma_stream_irq_no, 0, 0);    
         HAL_NVIC_EnableIRQ(i2c_info->rx_dma_stream_irq_no);
         
           
         /* Configure the NVIC for I2C #########################################*/   
         if(i2c_info->i2c_handle.Instance==I2C1){
            //
            NVIC_SetPriority(I2C1_ER_IRQn, (1 << __NVIC_PRIO_BITS) -3); 
            //HAL_NVIC_SetPriority(I2C1_ER_IRQn, 0, 1);
            HAL_NVIC_EnableIRQ(I2C1_ER_IRQn);
            //
            NVIC_SetPriority(I2C1_EV_IRQn, (1 << __NVIC_PRIO_BITS) -4); 
            //HAL_NVIC_SetPriority(I2C1_EV_IRQn, 0, 2);
            HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);
         }else if(i2c_info->i2c_handle.Instance==I2C2){
            //
            NVIC_SetPriority(I2C2_ER_IRQn, (1 << __NVIC_PRIO_BITS) -3); 
            //HAL_NVIC_SetPriority(I2C2_ER_IRQn, 0, 1);
            HAL_NVIC_EnableIRQ(I2C2_ER_IRQn);
            //
            NVIC_SetPriority(I2C2_EV_IRQn, (1 << __NVIC_PRIO_BITS) -4); 
            //HAL_NVIC_SetPriority(I2C2_EV_IRQn, 0, 2);
            HAL_NVIC_EnableIRQ(I2C2_EV_IRQn);
         }
         
         //
         HAL_I2C_Init(&i2c_info->i2c_handle);
      }
   }
   //
   if(o_flag & O_RDONLY) {
      if(i2c_info->desc_r<0) {
         i2c_info->desc_r = desc;
      }
      else
         return 0;                //already open
   }

   if(o_flag & O_WRONLY) {
      if(i2c_info->desc_w<0) {
         i2c_info->desc_w = desc;
      }
      else
         return 0;                //already open
   }

   if(!ofile_lst[desc].p)
      ofile_lst[desc].p=i2c_info;

   //unmask IRQ
   if(i2c_info->desc_r>=0 && i2c_info->desc_w>=0) {
     
   }
   return 0;
}

/*-------------------------------------------
| Name:dev_stm32f4xx_i2c_x_close
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_stm32f4xx_i2c_x_close(desc_t desc){
  board_stm32f4xx_i2c_info_t * p_i2c_info = (board_stm32f4xx_i2c_info_t*)ofile_lst[desc].p;
  //
  if(!p_i2c_info)
   return -1;
  // 
  if(ofile_lst[desc].oflag & O_RDONLY) {
      if(!ofile_lst[desc].nb_reader) {
         p_i2c_info->desc_r = -1;
      }
   }
   //
   if(ofile_lst[desc].oflag & O_WRONLY) {
      if(!ofile_lst[desc].nb_writer) {
         p_i2c_info->desc_w = -1;
      }
   }
  
   //
   if(p_i2c_info->desc_r<0 && p_i2c_info->desc_w<0) {
      //
      if(p_i2c_info->i2c_handle.Instance==I2C1){
         //
         board_stm32f4xx_i2c_info_list[__stm32f4xx_i2c_1__]=(void*)0;

         /* Force the I2C peripheral clock reset */
         __HAL_RCC_I2C1_FORCE_RESET();

         /* Release the I2C peripheral clock reset */
         __HAL_RCC_I2C1_RELEASE_RESET();
      }else if(p_i2c_info->i2c_handle.Instance==I2C2){
          //
         board_stm32f4xx_i2c_info_list[__stm32f4xx_i2c_2__]=(void*)0;;
         /* Force the I2C peripheral clock reset */
         __HAL_RCC_I2C2_FORCE_RESET();

         /* Release the I2C peripheral clock reset */
         __HAL_RCC_I2C2_RELEASE_RESET();
      }


      /* Disable the DMA Streams ############################################*/
      /* De-Initialize the DMA Stream associate to transmission process */
      HAL_DMA_DeInit(&p_i2c_info->hdma_tx); 
      /* De-Initialize the DMA Stream associate to reception process */
      HAL_DMA_DeInit(&p_i2c_info->hdma_rx);

      /* Disable the NVIC for DMA ###########################################*/
      HAL_NVIC_DisableIRQ(p_i2c_info->tx_dma_stream_irq_no);
      HAL_NVIC_DisableIRQ(p_i2c_info->rx_dma_stream_irq_no);

      /* Disable the NVIC for I2C ###########################################*/
      if(p_i2c_info->i2c_handle.Instance==I2C1){
         HAL_NVIC_DisableIRQ(I2C1_ER_IRQn);
         HAL_NVIC_DisableIRQ(I2C1_EV_IRQn);
      }else if(p_i2c_info->i2c_handle.Instance==I2C2){
         HAL_NVIC_EnableIRQ(I2C2_ER_IRQn);
         HAL_NVIC_EnableIRQ(I2C2_EV_IRQn);
      }
   }

   return 0;
}

/*-------------------------------------------
| Name:dev_stm32f4xx_i2c_x_isset_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_stm32f4xx_i2c_x_isset_read(desc_t desc){
   return 0;
}

/*-------------------------------------------
| Name:dev_stm32f4xx_i2c_x_isset_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_stm32f4xx_i2c_x_isset_write(desc_t desc){
   return 0;
}

/*-------------------------------------------
| Name:dev_stm32f4xx_i2c_x_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_stm32f4xx_i2c_x_read(desc_t desc, char* buf,int size){
   board_stm32f4xx_i2c_info_t * p_i2c_info = (board_stm32f4xx_i2c_info_t*)ofile_lst[desc].p;
 
   uint8_t  i2c_slave_address;

   //
   if(!p_i2c_info){
      return -1;
   }
   //
   if(size>p_i2c_info->i2c_rxtx_buffer_size)
      size=p_i2c_info->i2c_rxtx_buffer_size;
   
   //
   while (HAL_I2C_GetState(&p_i2c_info->i2c_handle) != HAL_I2C_STATE_READY); 
   
   //
   kernel_pthread_mutex_lock(&p_i2c_info->mutex);
  
   //
   i2c_slave_address =  p_i2c_info->current_slave_address;;
  
   //
   while(HAL_I2C_Master_Receive_DMA(&p_i2c_info->i2c_handle, i2c_slave_address,  p_i2c_info->p_i2c_rxtx_buffer, size) != HAL_OK){
      /* Error_Handler() function is called when Timeout error occurs.
      When Acknowledge failure occurs (Slave don't acknowledge it's address)
      Master restarts communication */
      if (HAL_I2C_GetError(&p_i2c_info->i2c_handle) != HAL_I2C_ERROR_AF){
         kernel_pthread_mutex_unlock(&p_i2c_info->mutex);
         return -1;
      }   
   }
   
   /*##-3- Wait for the end of the transfer ###################################*/ 
   while (HAL_I2C_GetState(&p_i2c_info->i2c_handle) != HAL_I2C_STATE_READY){
   } 
   
   //
   memcpy(buf,p_i2c_info->p_i2c_rxtx_buffer,size);
   
   //
   kernel_pthread_mutex_unlock(&p_i2c_info->mutex);
  
   //
   return size;
}

/*-------------------------------------------
| Name:dev_stm32f4xx_i2c_x_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_stm32f4xx_i2c_x_write(desc_t desc, const char* buf,int size){
   board_stm32f4xx_i2c_info_t * p_i2c_info = (board_stm32f4xx_i2c_info_t*)ofile_lst[desc].p;
 
   uint8_t  i2c_slave_address;
 

   //
   if(!p_i2c_info){
      return -1;
   }
   
   //
   if(size>p_i2c_info->i2c_rxtx_buffer_size)
      size=p_i2c_info->i2c_rxtx_buffer_size;
   
   //
   while (HAL_I2C_GetState(&p_i2c_info->i2c_handle) != HAL_I2C_STATE_READY); 
   //
   kernel_pthread_mutex_lock(&p_i2c_info->mutex);
   //
   i2c_slave_address = p_i2c_info->current_slave_address;
   //
   memcpy(p_i2c_info->p_i2c_rxtx_buffer,buf,size);
   //
   while(HAL_I2C_Master_Transmit_DMA(&p_i2c_info->i2c_handle, i2c_slave_address,  p_i2c_info->p_i2c_rxtx_buffer, size) != HAL_OK){
      /* Error_Handler() function is called when Timeout error occurs.
      When Acknowledge failure occurs (Slave don't acknowledge it's address)
      Master restarts communication */
      if (HAL_I2C_GetError(&p_i2c_info->i2c_handle) != HAL_I2C_ERROR_AF){
         kernel_pthread_mutex_unlock(&p_i2c_info->mutex);
         return -1;
      }   
   }
   //
   kernel_pthread_mutex_unlock(&p_i2c_info->mutex);
   //
   return size;
}

/*-------------------------------------------
| Name:dev_stm32f4xx_i2c_x_seek
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_stm32f4xx_i2c_x_seek(desc_t desc,int offset,int origin){
   return -1;
}

/*-------------------------------------------
| Name:dev_stm32f4xx_i2c_x_ioctl
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_stm32f4xx_i2c_x_ioctl(desc_t desc,int request,va_list ap) {
   
   board_stm32f4xx_i2c_info_t * p_i2c_info = (board_stm32f4xx_i2c_info_t*)ofile_lst[desc].p;
   
   switch (request){
      //
      case I2C_SLAVEADDR:{
         uint8_t slave_addr;
         
         kernel_pthread_mutex_lock(&p_i2c_info->mutex);
         //
         slave_addr= va_arg( ap, uint8_t);
         p_i2c_info->current_slave_address=slave_addr;
         //
         kernel_pthread_mutex_unlock(&p_i2c_info->mutex);
      }
      break;
      // 
      case I2C_LOCK: 
         kernel_pthread_mutex_lock(&p_i2c_info->mutex);
      break;
      //
      case I2C_UNLOCK:
         kernel_pthread_mutex_unlock(&p_i2c_info->mutex);
      break;
      //
      default:
      return -1;
   }
   //
   return 0;
}

/*============================================
| End of Source  : dev_stm32f4xx_i2c_x.c
==============================================*/