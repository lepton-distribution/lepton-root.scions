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
#include "kernel/core/dirent.h"
#include "kernel/core/fcntl.h"
#include "kernel/core/stat.h"
#include "kernel/core/ioctl.h"
#include "kernel/core/ioctl_hd.h"

#include "kernel/fs/vfs/vfstypes.h"

#include "stm32f4xx_hal.h"

#include "kernel/dev/arch/cortexm/stm32f4xx/flash_if.h"

/*============================================
| Global Declaration 
==============================================*/
#define APPLICATION_ADDRESS   (uint32_t)0x08100000 

const char dev_stm32f4xx_flash_name[]="flash0\0";

static int dev_stm32f4xx_flash_load(void);
static int dev_stm32f4xx_flash_open(desc_t desc, int o_flag);
static int dev_stm32f4xx_flash_close(desc_t desc);
static int dev_stm32f4xx_flash_isset_read(desc_t desc);
static int dev_stm32f4xx_flash_isset_write(desc_t desc);
static int dev_stm32f4xx_flash_read(desc_t desc, char* buf,int size);
static int dev_stm32f4xx_flash_write(desc_t desc, const char* buf,int size);
static int dev_stm32f4xx_flash_seek(desc_t desc,int offset,int origin);
static int dev_stm32f4xx_flash_ioctl(desc_t desc,int request,va_list ap);

//
dev_map_t dev_stm32f4xx_flash_map={
   dev_stm32f4xx_flash_name,
   S_IFBLK,
   dev_stm32f4xx_flash_load,
   dev_stm32f4xx_flash_open,
   dev_stm32f4xx_flash_close,
   dev_stm32f4xx_flash_isset_read,
   dev_stm32f4xx_flash_isset_write,
   dev_stm32f4xx_flash_read,
   dev_stm32f4xx_flash_write,
   dev_stm32f4xx_flash_seek,
   dev_stm32f4xx_flash_ioctl //ioctl
};

//
typedef struct stm32f4xx_flash_info_st{
   desc_t desc_r;
   desc_t desc_w;
   
   uint32_t flash_base_address;
   
}stm32f4xx_flash_info_t;

//
static stm32f4xx_flash_info_t g_stm32f4xx_flash_info={0};

/*===========================================
Implementation
=============================================*/



/*-------------------------------------------
| Name:dev_stm32f4xx_flash_load
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_stm32f4xx_flash_load(void){
   g_stm32f4xx_flash_info.desc_r=INVALID_DESC;
   g_stm32f4xx_flash_info.desc_w=INVALID_DESC;
   g_stm32f4xx_flash_info.flash_base_address=APPLICATION_ADDRESS;
   return 0;
}

/*-------------------------------------------
| Name:dev_stm32f4xx_flash_open
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_stm32f4xx_flash_open(desc_t desc, int o_flag){
      
   //
   if(o_flag & O_RDONLY){
      if(g_stm32f4xx_flash_info.desc_r<0) {
         g_stm32f4xx_flash_info.desc_r = desc;
      }
      else
         return -1;                //already open
   }
   //
   if(o_flag & O_WRONLY){
      if(g_stm32f4xx_flash_info.desc_w<0) {
         g_stm32f4xx_flash_info.desc_w = desc;
      }
      else
         return -1;                //already open
   }

   //
   ofile_lst[desc].offset=0;
   //

   return 0;

}

/*-------------------------------------------
| Name:dev_stm32f4xx_flash_close
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_stm32f4xx_flash_close(desc_t desc){
   return 0;
}

/*-------------------------------------------
| Name:dev_stm32f4xx_flash_isset_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_stm32f4xx_flash_isset_read(desc_t desc){
  return -1;
}

/*-------------------------------------------
| Name:dev_stm32f4xx_flash_isset_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_stm32f4xx_flash_isset_write(desc_t desc){
      return -1;
}
/*-------------------------------------------
| Name:dev_stm32f4xx_flash_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_stm32f4xx_flash_read(desc_t desc, char* buf,int size){
   static volatile uint32_t flash_base_address;
   
   //
   flash_base_address= g_stm32f4xx_flash_info.flash_base_address + ofile_lst[desc].offset;
   //
   if(flash_base_address<g_stm32f4xx_flash_info.flash_base_address){
      return -1;
   }
   //
   memcpy(buf,(void*)flash_base_address,size);
   //
   ofile_lst[desc].offset = ofile_lst[desc].offset+size;
   //
   return size;
}

/*-------------------------------------------
| Name:dev_stm32f4xx_flash_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_stm32f4xx_flash_write(desc_t desc, const char* buf,int size){
   volatile uint32_t flash_base_address;
   
   //
   flash_base_address= g_stm32f4xx_flash_info.flash_base_address + ofile_lst[desc].offset;
   //
   if(flash_base_address<g_stm32f4xx_flash_info.flash_base_address){
      return -1;
   }
   //
   if (FLASH_If_Write(flash_base_address, (uint32_t*) buf, size/4) != FLASHIF_OK){
      return -1;
   }
   //
   ofile_lst[desc].offset = ofile_lst[desc].offset+size;
   //
   return size;
}

/*-------------------------------------------
| Name:dev_stm32f4xx_flash_seek
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_stm32f4xx_flash_seek(desc_t desc,int offset,int origin){

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
| Name:dev_stm32f4xx_flash_ioctl
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_stm32f4xx_flash_ioctl(desc_t desc,int request,va_list ap){
   
   //
   switch(request) {

      //
      case HDGETSZ: {
         unsigned long* hdsz_p= va_arg( ap, unsigned long*);

         if(!ofile_lst[desc].p)
            return -1;

         if(!hdsz_p)
            return -1;
        // *hdsz_p = ((dev_flash_t*)ofile_lst[desc].p)->p_flash_type->size
        //           *((dev_flash_t*)ofile_lst[desc].p)->p_flash_type->width;
      }
      break;

      //
      case HDSETSZ: {
         return -1;
      }
      break;

      //
      case HDCHK: {
      }
      break;

      //
      case HDGETSCTRSZ: {
         unsigned long sector_addr= va_arg( ap, unsigned long);
         unsigned long* sector_sz= va_arg( ap, unsigned long*);
         if(!sector_sz)
            return -1;
         //
         return -1;
      }
      break;

      //
      case HDCLRSCTR: {
         return -1;
      }
      break;

      //
      case HDCLRDSK: {
         volatile uint32_t flash_base_address= g_stm32f4xx_flash_info.flash_base_address;
         if(FLASH_If_Erase(flash_base_address)!=FLASHIF_OK){
            return -1;
         }
      }
      break;

      //
      case HDIO: {
         //hdio_t* hdio= va_arg( ap, hdio_t*);
         //hdio->addr = (hdio_addr_t)((dev_flash_t*)ofile_lst[desc].p)->flash_base_pt;
      }
      break;

      //
      default:
         return -1;
   }

   return 0;
}

/*============================================
| End of Source  : dev_stm32f4xx_flash.c
==============================================*/
