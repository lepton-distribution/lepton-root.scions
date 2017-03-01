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

#include "kernel/core/ioctl_hd.h"
#include "kernel/core/system.h"
#include "kernel/core/fcntl.h"
#include "kernel/core/stat.h"
#include "kernel/fs/vfs/vfsdev.h"
#include "kernel/fs/vfs/vfstypes.h"

#include "kernel/dev/arch/win32/dev_win32_fileflash/dev_win32_fileflash.h"
#include "kernel/dev/arch/win32/dev_win32_fileflash/fileflash_win32_isolation.h"

/*===========================================
Global Declaration
=============================================*/
const char dev_win32_fileflash_name[]="hdc\0";

int dev_win32_fileflash_load(void);
int dev_win32_fileflash_open(desc_t desc, int o_flag);
int dev_win32_fileflash_close(desc_t desc);
int dev_win32_fileflash_isset_read(desc_t desc);
int dev_win32_fileflash_isset_write(desc_t desc);
int dev_win32_fileflash_read(desc_t desc, char* buf,int size);
int dev_win32_fileflash_write(desc_t desc, const char* buf,int size);
int dev_win32_fileflash_seek(desc_t desc,int offset,int origin);
int dev_win32_fileflash_ioctl(desc_t desc,int request,va_list ap);

dev_map_t dev_win32_fileflash_map={
   dev_win32_fileflash_name,
   S_IFBLK,
   dev_win32_fileflash_load,
   dev_win32_fileflash_open,
   dev_win32_fileflash_close,
   __fdev_not_implemented,
   __fdev_not_implemented,
   dev_win32_fileflash_read,
   dev_win32_fileflash_write,
   dev_win32_fileflash_seek,
   dev_win32_fileflash_ioctl //ioctl
};



/*===========================================
Implementation
=============================================*/

/*-------------------------------------------
| Name:dev_win32_fileflash_load
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_win32_fileflash_load(void) {
   return 0;
}

/*-------------------------------------------
| Name:dev_win32_fileflash_open
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_win32_fileflash_open(desc_t desc, int o_flag) {
   //
   if (fileflash_win32_isolation_open(DFLT_FLILEFLASH_FILE_PATH) < 0)
      return -1;
   //
   if (o_flag & O_RDONLY) {
   }

   if (o_flag & O_WRONLY) {
   }

   return 0;
}

/*-------------------------------------------
| Name:dev_win32_fileflash_close
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_win32_fileflash_close(desc_t desc) {
   return fileflash_win32_isolation_close();
}

/*-------------------------------------------
| Name:dev_win32_fileflash_isset_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_win32_fileflash_isset_read(desc_t desc) {
   return -1;
}

/*-------------------------------------------
| Name:dev_win32_fileflash_isset_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_win32_fileflash_isset_write(desc_t desc) {
   return -1;
}
/*-------------------------------------------
| Name:dev_win32_fileflash_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_win32_fileflash_read(desc_t desc, char* buf, int size) {
   return fileflash_win32_isolation_read(&ofile_lst[desc].offset, buf, size);
}

/*-------------------------------------------
| Name:dev_win32_fileflash_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_win32_fileflash_write(desc_t desc, const char* buf, int size) {
   return fileflash_win32_isolation_write(&ofile_lst[desc].offset, buf, size);
}

/*-------------------------------------------
| Name:dev_win32_fileflash_seek
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_win32_fileflash_seek(desc_t desc, off_t offset, int origin) {
   unsigned long dev_current_addr = (unsigned long)ofile_lst[desc].offset;

   switch (origin) {
   case SEEK_SET:
      dev_current_addr = (unsigned long)(offset);
      break;

   case SEEK_CUR:
      dev_current_addr += (unsigned long)(offset);
      break;

   case SEEK_END:
      dev_current_addr = (unsigned long)fileflash_win32_isolation_getsize();
      break;
   }
   //
   ofile_lst[desc].offset = dev_current_addr;
   //
   return ofile_lst[desc].offset;
}

/*-------------------------------------------
| Name:dev_win32_fileflash_ioctl
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_win32_fileflash_ioctl(desc_t desc, int request, va_list ap) {
   switch (request) {
      //
      case HDGETSZ: {
         long* hdsz_p = va_arg(ap, long*);
         if (!hdsz_p)
            return -1;
         //
         *hdsz_p = fileflash_win32_isolation_getsize();
      }
      break;

      //
      case HDSETSZ: {
         long hdsz = va_arg(ap, long);
         //
         if (!hdsz)
            return -1;
         //
         fileflash_win32_isolation_setsize(hdsz);
         //
      }
      break;

      //
      case HDCLRDSK: {
         fileflash_win32_isolation_erase();
      }
      break;
      //
      default:
         return -1;

   }

   return 0;
}

/*===========================================
End of Sourcedrv_fileflash.c
=============================================*/
