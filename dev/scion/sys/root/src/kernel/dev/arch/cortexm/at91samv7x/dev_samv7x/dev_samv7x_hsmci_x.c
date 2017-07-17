/*
The contents of this file are subject to the Mozilla Public License Version 1.1
(the "License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at http://www.mozilla.org/MPL/

Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for the
specific language governing rights and limitations under the License.

The Original Code is Lepton.

The Initial Developer of the Original Code is Philippe Le Boulanger.
Portions created by Philippe Le Boulanger are Copyright (C) 2017 <lepton.phlb@gmail.com>.
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
#include "kernel/core/fcntl.h"
#include "kernel/core/cpu.h"
#include "kernel/fs/vfs/vfstypes.h"
#include "kernel/core/ioctl.h"
#include "kernel/core/ioctl_hd.h"

#include "board.h"
#include "libsdmmc.h"
#include "Media.h"
#include "MEDSdcard.h"

#include "kernel/dev/arch/cortexm/at91samv7x/dev_samv7x/dev_samv7x_cpu_x.h"
#include "kernel/dev/arch/cortexm/at91samv7x/dev_samv7x/dev_samv7x_hsmci_x.h"

/*===========================================
Global Declaration
=============================================*/

static samv71x_hsmci_info_t* g_list_samv71x_hsmci_info=(samv71x_hsmci_info_t*)0;


/*===========================================
Implementation
=============================================*/

/**
 * MCI interrupt handler. Forwards the event to the MCI driver handlers.
 */
void HSMCI_Handler(void)
{
   samv71x_hsmci_info_t* samv71x_hsmci_info=g_list_samv71x_hsmci_info;
   //
   __hw_enter_interrupt();
   //
   while(samv71x_hsmci_info!=(samv71x_hsmci_info_t*)0){
      MCID_Handler(&samv71x_hsmci_info->samv7x_mci_drv);
      samv71x_hsmci_info = samv71x_hsmci_info->next_samv71x_hsmci_info;
   }
   //
   __hw_leave_interrupt();
}


/*-------------------------------------------
| Name:dev_samv71x_hsmci_x_is_connected
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static uint8_t dev_samv71x_hsmci_x_is_connected(samv71x_hsmci_info_t* samv71x_hsmci_info)
{
	return PIO_Get(samv71x_hsmci_info->base_sdcd_pins) ? 0 : 1;
}


/*-------------------------------------------
| Name:dev_samv71x_hsmci_x_load
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_samv71x_hsmci_x_load(samv71x_hsmci_info_t* samv71x_hsmci_info){
   
   /* Initialize the HSMCI driver */
	MCID_Init(&samv71x_hsmci_info->samv7x_mci_drv, HSMCI, ID_HSMCI, BOARD_MCK, &g_samv7x_dmad, 0);
   
	/* Enable MCI interrupt and give it priority lower than DMA*/
	NVIC_ClearPendingIRQ(HSMCI_IRQn);
	NVIC_SetPriority(HSMCI_IRQn, (1u << __NVIC_PRIO_BITS) - 3u);
	NVIC_EnableIRQ(HSMCI_IRQn);
   
   /* Initialize SD driver */
   SDD_InitializeSdmmcMode(&samv71x_hsmci_info->samv7x_sd_drv, &samv71x_hsmci_info->samv7x_mci_drv, 0);
   
   //
   samv71x_hsmci_info->next_samv71x_hsmci_info=g_list_samv71x_hsmci_info;
   g_list_samv71x_hsmci_info=samv71x_hsmci_info;
   return 0;
}

/*-------------------------------------------
| Name:dev_samv71x_hsmci_x_open
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_samv71x_hsmci_x_open(desc_t desc, int o_flag, samv71x_hsmci_info_t* samv71x_hsmci_info){

   //
   if(samv71x_hsmci_info->desc_r<0 && samv71x_hsmci_info->desc_w<0) {
      //
      if(!dev_samv71x_hsmci_x_is_connected(samv71x_hsmci_info)){
         return -1;
      }
      //
      uint8_t error;
      uint8_t retry = 2;
      //
      while (retry--) {
         error = SD_Init(&samv71x_hsmci_info->samv7x_sd_drv);
         if (error == SDMMC_OK) break;
      }
      //
      if (error != SDMMC_OK) 
         return -1;
      //
      if(!(MEDSdcard_Initialize(&samv71x_hsmci_info->samv7x_sd_media, &samv71x_hsmci_info->samv7x_sd_drv))){
         return -1;
      }
      //
   }
   //
   if(o_flag & O_RDONLY) {
      if(samv71x_hsmci_info->desc_r<0) {
         samv71x_hsmci_info->desc_r = desc;
      }
      else
         return -1;                //already open
   }

   if(o_flag & O_WRONLY) {
      if(samv71x_hsmci_info->desc_w<0) {
         samv71x_hsmci_info->desc_w = desc;
      }
      else
         return -1;                //already open
   }

   if(!ofile_lst[desc].p)
      ofile_lst[desc].p=samv71x_hsmci_info;

   //unmask IRQ
   if(samv71x_hsmci_info->desc_r>=0 && samv71x_hsmci_info->desc_w>=0) {
     
   }
   //
   return 0;
}

/*-------------------------------------------
| Name:dev_samv71x_hsmci_x_close
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_samv71x_hsmci_x_close(desc_t desc){
   return 0;
}

/*-------------------------------------------
| Name:dev_samv71x_hsmci_x_isset_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_samv71x_hsmci_x_isset_read(desc_t desc){
   return -1;
}

/*-------------------------------------------
| Name:dev_samv71x_hsmci_x_isset_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_samv71x_hsmci_x_isset_write(desc_t desc){
   return -1;
}
/*-------------------------------------------
| Name:dev_samv71x_hsmci_x_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_samv71x_hsmci_x_read(desc_t desc, char* buf,int size){
   samv71x_hsmci_info_t* samv71x_hsmci_info =(samv71x_hsmci_info_t *)ofile_lst[desc].p;
   
   //
   if(samv71x_hsmci_info==(samv71x_hsmci_info_t*)0){
      return -1;
   }
   //
   if(size<samv71x_hsmci_info->rw_aligned_dma_buffer_size){
      return -1;
   }
   //must be multiple of sdio_info->BlockSize. see seek()
   if((ofile_lst[desc].offset%samv71x_hsmci_info->samv7x_sd_media.blockSize)!=0){
     return -1;
   }
   //must be multiple of sdio_info->BlockSize
   if((size%samv71x_hsmci_info->samv7x_sd_media.blockSize)!=0){
      return -1;
   }
   
   //
   samv71x_hsmci_info->result = MED_Read(&samv71x_hsmci_info->samv7x_sd_media, 
                                         ofile_lst[desc].offset/samv71x_hsmci_info->samv7x_sd_media.blockSize, 
                                         (void*)samv71x_hsmci_info->p_rw_aligned_dma_buffer, 
                                         size/samv71x_hsmci_info->samv7x_sd_media.blockSize, NULL, NULL);
   //
   if( samv71x_hsmci_info->result != MED_STATUS_SUCCESS ){
     return -1;
   }
   //
   memcpy(buf,(void*)samv71x_hsmci_info->p_rw_aligned_dma_buffer,size);
   
   //
   ofile_lst[desc].offset+=size;
   //
   return size;
}

/*-------------------------------------------
| Name:dev_samv71x_hsmci_x_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_samv71x_hsmci_x_write(desc_t desc, const char* buf,int size){
   samv71x_hsmci_info_t* samv71x_hsmci_info =(samv71x_hsmci_info_t *)ofile_lst[desc].p;
   
   //
   if(samv71x_hsmci_info==(samv71x_hsmci_info_t*)0){
      return -1;
   }
   //
   if(size>samv71x_hsmci_info->rw_aligned_dma_buffer_size){
      return -1;
   }
   //must be multiple of sdio_info->BlockSize. see seek()
   if((ofile_lst[desc].offset%samv71x_hsmci_info->samv7x_sd_media.blockSize)!=0){
     return -1;
   }
   //must be multiple of sdio_info->BlockSize
   if((size%samv71x_hsmci_info->samv7x_sd_media.blockSize)!=0){
      return -1;
   }
   
   //
   memcpy((void*)samv71x_hsmci_info->p_rw_aligned_dma_buffer,buf,size);
   //
   samv71x_hsmci_info->result = MED_Write(&samv71x_hsmci_info->samv7x_sd_media, 
                                          ofile_lst[desc].offset/samv71x_hsmci_info->samv7x_sd_media.blockSize, 
                                          (void*)samv71x_hsmci_info->p_rw_aligned_dma_buffer, 
                                          size/samv71x_hsmci_info->samv7x_sd_media.blockSize, NULL, NULL);
   //
   if( samv71x_hsmci_info->result != MED_STATUS_SUCCESS ){
     return -1;
   }
   
   //
   ofile_lst[desc].offset+=size;
   //
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
int dev_samv71x_hsmci_x_seek(desc_t desc,int offset,int origin){
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
| Name:dev_samv71x_hsmci_x_ioctl
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_samv71x_hsmci_x_ioctl(desc_t desc,int request,va_list ap){
   samv71x_hsmci_info_t* samv71x_hsmci_info =(samv71x_hsmci_info_t *)ofile_lst[desc].p;
   
   //
   if(samv71x_hsmci_info==(samv71x_hsmci_info_t*)0){
      return -1;
   }
   
   //
   switch(request) {
      case HDGETSZ: {
         uint64_t* hdsz_p= va_arg( ap, uint64_t*);
         if(!hdsz_p)
            return -1;
         
         //
         *hdsz_p= samv71x_hsmci_info->samv7x_sd_media.size;
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
         *sector_sz = samv71x_hsmci_info->samv7x_sd_media.blockSize;
      }
      break;
      
      //
      case HDCLRDSK: {
         unsigned long StartAddr= va_arg( ap, unsigned long);
         //
         uint32_t start_block_number=StartAddr/samv71x_hsmci_info->samv7x_sd_media.blockSize;
         uint32_t end_block_number=samv71x_hsmci_info->samv7x_sd_media.size/samv71x_hsmci_info->samv7x_sd_media.blockSize -1;
         //
         for(int i=start_block_number; i<end_block_number;i++){
            MEDSdcard_EraseBlock(&samv71x_hsmci_info->samv7x_sd_media, i);
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
| End of Source  : dev_samv71x_hsmci_x.c
==============================================*/