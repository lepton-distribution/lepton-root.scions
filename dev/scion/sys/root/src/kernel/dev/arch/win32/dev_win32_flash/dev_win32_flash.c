/*
The contents of this file are subject to the Mozilla Public License Version 1.1
(the "License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at http://www.mozilla.org/MPL/

Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for the
specific language governing rights and limitations under the License.

The Original Code is Lepton.

The Initial Developer of the Original Code is Chauvin-Arnoux.
Portions created by Chauvin-Arnoux are Copyright (C) 2011. All Rights Reserved.

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

#include "kernel/core/system.h"
#include "kernel/core/fcntl.h"
#include "kernel/core/stat.h"
#include "kernel/core/ioctl_hd.h"
#include "kernel/fs/vfs/vfsdev.h"
#include "kernel/fs/vfs/vfstypes.h"


#include "kernel/dev/arch/win32/dev_win32_flash/dev_win32_flash.h"
#include "kernel/dev/arch/win32/dev_win32_flash/flash_win32_isolation.h"


/*===========================================
Global Declaration
=============================================*/
static const char dev_win32_flash_name[]="hdc\0win32_flash\0";

static int dev_win32_flash_load(void);
static int dev_win32_flash_open(desc_t desc, int o_flag);
static int dev_win32_flash_close(desc_t desc);
static int dev_win32_flash_isset_read(desc_t desc);
static int dev_win32_flash_isset_write(desc_t desc);
static int dev_win32_flash_read(desc_t desc, char* buf,int size);
static int dev_win32_flash_write(desc_t desc, const char* buf,int size);
static int dev_win32_flash_seek(desc_t desc,int offset,int origin);
static int dev_win32_flash_ioctl(desc_t desc,int request,va_list ap);

dev_map_t dev_win32_flash_map={
   dev_win32_flash_name,
   S_IFBLK,
   dev_win32_flash_load,
   dev_win32_flash_open,
   dev_win32_flash_close,
   __fdev_not_implemented,
   __fdev_not_implemented,
   dev_win32_flash_read,
   dev_win32_flash_write,
   dev_win32_flash_seek,
   dev_win32_flash_ioctl //ioctl
};



/*===========================================
Implementation
=============================================*/

/*-------------------------------------------
| Name:dev_win32_flash_load
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_win32_flash_load(void){
   return flash_win32_isolation_load(DFLT_FLASH_FILE_PATH);
}

/*-------------------------------------------
| Name:dev_win32_flash_open
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_win32_flash_open(desc_t desc, int o_flag){
   //
   if(o_flag & O_RDONLY) {
   }
   //
   if(o_flag & O_WRONLY) {
   }
   //
   ofile_lst[desc].offset = 0;
   //
   ofile_lst[desc].p= flash_win32_isolation_open();
   //
   return 0;
}

/*-------------------------------------------
| Name:dev_win32_flash_close
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_win32_flash_close(desc_t desc){
   return flash_win32_isolation_close();
}

/*-------------------------------------------
| Name:dev_win32_flash_isset_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_win32_flash_isset_read(desc_t desc){
   return -1;
}

/*-------------------------------------------
| Name:dev_win32_flash_isset_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_win32_flash_isset_write(desc_t desc){
   return -1;
}
/*-------------------------------------------
| Name:dev_win32_flash_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_win32_flash_read(desc_t desc, char* buf,int size){
   return flash_win32_isolation_read(&ofile_lst[desc].offset, buf, size);
}

/*-------------------------------------------
| Name:dev_win32_flash_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_win32_flash_write(desc_t desc, const char* buf,int size){
   return flash_win32_isolation_write(&ofile_lst[desc].offset, buf, size);
}

/*-------------------------------------------
| Name:dev_win32_flash_seek
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_win32_flash_seek(desc_t desc,int offset,int origin){
   //
   ofile_lst[desc].offset = flash_win32_isolation_seek(&offset, origin);
   //
   return ofile_lst[desc].offset;
}

/*-------------------------------------------
| Name:dev_win32_flash_ioctl
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_win32_flash_ioctl(desc_t desc,int request,va_list ap){
   switch(request) {

      //
      case HDGETSZ: {
         long* hdsz_p = va_arg(ap, long*);
         if (!hdsz_p)
            return -1;
         //
         *hdsz_p = flash_win32_isolation_getsize();
      }
      break;

      //
      case HDSETSZ: {
         long hdsz = va_arg(ap, long);
         //
         if (!hdsz)
            return -1;
         //
         flash_win32_isolation_setsize(hdsz);
         //
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
         //
         if(!sector_sz)
            return -1;
         //
         *sector_sz = flash_win32_isolation_getsectorsize(sector_addr);
         //
         if (*sector_sz < 0)
            return -1;

      }
      break;

      //
      case HDCLRSCTR: {
         unsigned long sector_addr= va_arg( ap, unsigned long);
         int r = flash_win32_isolation_erase_sector(sector_addr);
         return r;
      }
      break;

      //
      case HDCLRDSK: {
         int r = flash_win32_isolation_erase_all();
         return r;
      }
      break;

      //
      default:
         return -1;

   }

   return 0;
}
/*============================================
| End of Source  : dev_win32_flash.c
==============================================*/
