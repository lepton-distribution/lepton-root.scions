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
| Includes    
==============================================*/
#include <stdint.h>

#include "kernel/core/kernelconf.h"
#include "kernel/core/types.h"
#include "kernel/core/dirent.h"
#include "kernel/core/interrupt.h"
#include "kernel/core/kernel.h"
#include "kernel/core/system.h"
#include "kernel/core/fcntl.h"
#include "kernel/core/stat.h"
#include "kernel/core/ioctl.h"
#include "kernel/core/ioctl_hd.h"

#include "kernel/fs/vfs/vfstypes.h"

#include "stm32f4xx_hal.h"

#include "dev_stm32f4xx_sdio.h"

/*============================================
| Global Declaration 
==============================================*/

#define BUS_4BITS 1

//for compatibility with old version of hal cube ms
#ifndef SD_OK 
#define SD_OK  HAL_SD_ERROR_NONE
#endif


int dev_stm32f4xx_sdio_load(board_stm32f4xx_sdio_info_t * spi_info);
int dev_stm32f4xx_sdio_open(desc_t desc, int o_flag,board_stm32f4xx_sdio_info_t * sdio_info);
int dev_stm32f4xx_sdio_close(desc_t desc);
int dev_stm32f4xx_sdio_isset_read(desc_t desc);
int dev_stm32f4xx_sdio_isset_write(desc_t desc);
int dev_stm32f4xx_sdio_read(desc_t desc, char* buf,int size);
int dev_stm32f4xx_sdio_write(desc_t desc, const char* buf,int size);
int dev_stm32f4xx_sdio_seek(desc_t desc,int offset,int origin);
int dev_stm32f4xx_sdio_ioctl(desc_t desc,int request,va_list ap);



/*===========================================
Implementation
=============================================*/



/*-------------------------------------------
| Name:dev_stm32f4xx_sdio_load
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_stm32f4xx_sdio_load(board_stm32f4xx_sdio_info_t * sdio_info){
   
   //
   sdio_info->BlockSize = MSD_DEFAULT_BLOCK_SIZE;
   //
   sdio_info->hsd.Instance = SDIO;
   sdio_info->hsd.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
   sdio_info->hsd.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
   sdio_info->hsd.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
   sdio_info->hsd.Init.BusWide = SDIO_BUS_WIDE_1B;
   sdio_info->hsd.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
   sdio_info->hsd.Init.ClockDiv = 0;

   /* Peripheral clock enable */
   __HAL_RCC_SDIO_CLK_ENABLE();

   //
   return 0;
}

/*-------------------------------------------
| Name:dev_stm32f4xx_sdio_open
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_stm32f4xx_sdio_open(desc_t desc, int o_flag, board_stm32f4xx_sdio_info_t * sdio_info){
  
   if(sdio_info->desc_r<0 && sdio_info->desc_w<0) {
      //
      sdio_info->sd_state = MSD_OK;
      sdio_info->BlockSize = MSD_DEFAULT_BLOCK_SIZE;
      //
#if 0
      /* Check if the SD card is plugged in the slot */
      if (BSP_SD_IsDetected() != SD_PRESENT){
         return MSD_ERROR;
      }
#endif

#if 0 //to do: migration in progress to hal cube mx 1.16
      /* HAL SD initialization */
      __disable_interrupt_section_in();
      sdio_info->sd_state = HAL_SD_Init(&sdio_info->hsd, &sdio_info->SDCardInfo);
      __disable_interrupt_section_out();
      #ifdef BUS_4BITS
         /* Configure SD Bus width */
         if (sdio_info->sd_state == MSD_OK){
         /* Enable wide operation */
            if (HAL_SD_WideBusOperation_Config(&sdio_info->hsd, SDIO_BUS_WIDE_4B) != SD_OK){
               sdio_info->sd_state = MSD_ERROR;
               return -1;
            }
            else{
               sdio_info->sd_state = MSD_OK;
            }
         }else{
             sdio_info->sd_state = MSD_ERROR;
             return -1;
         }
      #endif
         
#endif //migration in progress
   }

   //
   if(o_flag & O_RDONLY){
      if(sdio_info->desc_r<0) {
         sdio_info->desc_r = desc;
      }
      else
         return -1;                //already open
   }
   //
   if(o_flag & O_WRONLY){
      if(sdio_info->desc_w<0) {
         sdio_info->desc_w = desc;
      }
      else
         return -1;                //already open
   }
   
   //
   if(ofile_lst[desc].p==(void*)0){
      ofile_lst[desc].p = sdio_info;
      //
      HAL_SD_GetCardInfo(&sdio_info->hsd, &sdio_info->SDCardInfo);
   }

   //
   ofile_lst[desc].offset=0;

   return 0;
}

/*-------------------------------------------
| Name:dev_stm32f4xx_sdio_close
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_stm32f4xx_sdio_close(desc_t desc){
   return 0;
}

/*-------------------------------------------
| Name:dev_stm32f4xx_sdio_isset_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_stm32f4xx_sdio_isset_read(desc_t desc){
  return -1;
}

/*-------------------------------------------
| Name:dev_stm32f4xx_sdio_isset_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_stm32f4xx_sdio_isset_write(desc_t desc){
      return -1;
}
/*-------------------------------------------
| Name:dev_stm32f4xx_sdio_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_stm32f4xx_sdio_read(desc_t desc, char* buf,int size){
    
   board_stm32f4xx_sdio_info_t* sdio_info =(board_stm32f4xx_sdio_info_t *)ofile_lst[desc].p;
   
   //
   if(sdio_info==(board_stm32f4xx_sdio_info_t*)0){
      return -1;
   }
   
   //must be multiple of sdio_info->BlockSize. see seek()
   if((ofile_lst[desc].offset%sdio_info->BlockSize)!=0){
     return -1;
   }
   
   //must be multiple of sdio_info->BlockSize
   if((size%sdio_info->BlockSize)!=0){
      return -1;
   }
   
   //
   if(HAL_SD_ReadBlocks(&sdio_info->hsd, (uint8_t*)buf, ofile_lst[desc].offset, sdio_info->BlockSize, size/sdio_info->BlockSize) != SD_OK){
      sdio_info->sd_state = MSD_ERROR;
      return -1;
   }  
   //
   sdio_info->sd_state = MSD_OK;
   //
   ofile_lst[desc].offset+=size;
   //
   return size;
}

/*-------------------------------------------
| Name:dev_stm32f4xx_sdio_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_stm32f4xx_sdio_write(desc_t desc, const char* buf,int size){
   board_stm32f4xx_sdio_info_t* sdio_info =(board_stm32f4xx_sdio_info_t *)ofile_lst[desc].p;
   
   //
   if(sdio_info==(board_stm32f4xx_sdio_info_t*)0){
      return -1;
   }
   
   //must be multiple of sdio_info->BlockSize. see seek()
   if((ofile_lst[desc].offset%sdio_info->BlockSize)!=0){
     return -1;
   }
   
   //must be multiple of sdio_info->BlockSize
   if((size%sdio_info->BlockSize)!=0){
      return -1;
   }
   
   //
   if(HAL_SD_WriteBlocks(&sdio_info->hsd, (uint8_t*)buf, ofile_lst[desc].offset, sdio_info->BlockSize, size/sdio_info->BlockSize) != SD_OK){
      sdio_info->sd_state = MSD_ERROR;
      return -1;
   }
   
   //
   sdio_info->sd_state = MSD_OK;
   //
   ofile_lst[desc].offset+=size;
   //
   return size;
}

/*-------------------------------------------
| Name:dev_stm32f4xx_sdio_seek
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_stm32f4xx_sdio_seek(desc_t desc,int offset,int origin){

   switch(origin){

      case SEEK_SET:
         ofile_lst[desc].offset=offset;
      break;

      case SEEK_CUR:
         ofile_lst[desc].offset+=offset;
      break;

      case SEEK_END:
         //to do: warning in SEEK_END (+ or -)????
         ofile_lst[desc].offset-=offset;
      break;
   }

   return ofile_lst[desc].offset;
}

/*-------------------------------------------
| Name:dev_stm32f4xx_sdio_ioctl
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_stm32f4xx_sdio_ioctl(desc_t desc,int request,va_list ap){
  
   board_stm32f4xx_sdio_info_t* sdio_info =(board_stm32f4xx_sdio_info_t *)ofile_lst[desc].p;
   
   //
   if(sdio_info==(board_stm32f4xx_sdio_info_t*)0){
      return -1;
   }
   
   //
   switch(request) {
      case HDGETSZ: {
         uint64_t* hdsz_p= va_arg( ap, uint64_t*);
         if(!hdsz_p)
            return -1;
         /* Get SD card Information */
         //HAL_SD_Get_CardInfo(&sdio_info->hsd, &sdio_info->SDCardInfo);
         HAL_SD_GetCardInfo(&sdio_info->hsd, &sdio_info->SDCardInfo);
         //
         //*hdsz_p= sdio_info->SDCardInfo.CardCapacity;
         *hdsz_p= sdio_info->SDCardInfo.BlockNbr*sdio_info->SDCardInfo.BlockSize;
      }
      break;
      
      //
      case HDGETSCTRSZ: {
         unsigned long sector_addr= va_arg( ap, unsigned long);
         unsigned long* sector_sz= va_arg( ap, unsigned long*);
         //
         if(!sector_sz)
            return -1;
         //
         *sector_sz = sdio_info->BlockSize;
      }
      break;
      
      //
      case HDCLRDSK: {
         unsigned long StartAddr= va_arg( ap, unsigned long);
         unsigned long EndAddr =  ( ( (sdio_info->SDCardInfo.BlockNbr*sdio_info->SDCardInfo.BlockSize)/sdio_info->BlockSize) - 1 )*sdio_info->BlockSize;
         
         if(HAL_SD_Erase(&sdio_info->hsd, StartAddr, EndAddr) != SD_OK){
            sdio_info->sd_state = MSD_ERROR;
         }else{
            sdio_info->sd_state = MSD_OK;
         }
      }
      
      break;
      //
      default:
      return -1;
   }
  
   //
   return 0;     
}

/*============================================
| End of Source  : dev_stm32f4xx_sdio.c
==============================================*/
