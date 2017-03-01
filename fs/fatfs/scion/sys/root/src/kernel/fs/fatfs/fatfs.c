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
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>


#include "kernel/core/errno.h"
#include "kernel/core/kernel.h"
#include "kernel/core/dirent.h"
#include "kernel/core/system.h"
#include "kernel/core/systime.h"
#include "kernel/core/stat.h"

#include "kernel/fs/vfs/vfstypes.h"
#include "kernel/fs/vfs/vfsdev.h"
#include "kernel/fs/fatfs/fatfs.h"
#include "kernel/fs/fatfs/fatfscore.h"







/*===========================================
Global Declaration
=============================================*/
//
static int _fatfs_statfs(mntdev_t* pmntdev,struct statvfs *statvfs);
static int _fatfs_readfs(mntdev_t* pmntdev);
static int _fatfs_writefs(mntdev_t* pmntdev);
static int _fatfs_checkfs(mntdev_t* pmntdev);
static int _fatfs_makefs(desc_t dev_desc,struct vfs_formatopt_t* vfs_formatopt);
static int _fatfs_loadfs(void);

static int _fatfs_mountdir(desc_t desc,inodenb_t original_root_node,inodenb_t target_root_node);
static int _fatfs_extern_readdir(desc_t desc, struct dirent* pdirent);

fsop_t const fatfs_op={
   .fs.vfstype          = VFS_TYPE_EXTERNAL_FS,
   .fs.loadfs        = _fatfs_loadfs,
   .fs.checkfs       = _fatfs_checkfs,
   .fs.makefs        = _fatfs_makefs,
   .fs.readfs        = _fatfs_readfs,
   .fs.writefs       = _fatfs_writefs,
   .fs.statfs        = _fatfs_statfs,
   .fs.mountdir      = _fatfs_mountdir,
   
   //
   .fs.extern_create    = fatfscore_extern_create,
   .fs.extern_open      = fatfscore_extern_open,
   .fs.extern_close     = fatfscore_extern_close,
   .fs.extern_read      = fatfscore_extern_read,
   .fs.extern_write     = fatfscore_extern_write,
   .fs.extern_seek      = fatfscore_extern_seek,
   .fs.extern_truncate  = fatfscore_extern_truncate,

   .fs.extern_mkdir     = fatfscore_extern_mkdir,
   .fs.extern_opendir   = fatfscore_extern_opendir,
   .fs.extern_closedir  = fatfscore_extern_closedir,
   .fs.extern_readdir   = _fatfs_extern_readdir,
   .fs.extern_telldir   = fatfscore_extern_telldir,
   .fs.extern_seekdir   = fatfscore_extern_seekdir,

   .fs.extern_unlink    = fatfscore_extern_unlink,
   .fs.extern_rename    = fatfscore_extern_rename,
   .fs.extern_stat      = fatfscore_extern_stat
};


/*===========================================
Implementation
=============================================*/

/*-------------------------------------------
| Name:_fatfs_statfs
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int _fatfs_statfs(mntdev_t* pmntdev,struct statvfs *statvfs){
   return 0;
}

/*-------------------------------------------
| Name:_fatfs_loadfs
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int _fatfs_loadfs(void){ 
   return 0;
}

/*-------------------------------------------
| Name:_fatfs_makefs
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int _fatfs_makefs(desc_t dev_desc,struct vfs_formatopt_t* vfs_formatopt){

   return 0;
}

/*-------------------------------------------
| Name:_fatfs_readfs
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int _fatfs_readfs(mntdev_t* pmntdev){
   return fatfscore_readfs(pmntdev);
}

/*-------------------------------------------
| Name:_fatfs_writefs
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int _fatfs_writefs(mntdev_t* pmntdev){
   return 0;
}

/*-------------------------------------------
| Name:_fatfs_checkfs
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static  int _fatfs_checkfs(mntdev_t* pmntdev){
   return 0;
}

/*-------------------------------------------
| Name:_fatfs_mountdir
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int _fatfs_mountdir(desc_t desc,inodenb_t original_root_node,inodenb_t target_root_node){
   return -1;
}

/*-------------------------------------------
| Name:_fatfs_extern_readdir
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int _fatfs_extern_readdir(desc_t desc, struct dirent* pdirent){
   static fatfs_dirent_t fatfs_dirent;
   //
   if(fatfscore_extern_readdir(desc,&fatfs_dirent)<0)
      return -1;
   //
   pdirent->inodenb = fatfs_dirent.inodenb;
   memset(pdirent->d_name,0,sizeof(pdirent->d_name));
   memcpy(pdirent->d_name,fatfs_dirent.d_name,sizeof(pdirent->d_name)-1);
   //
   return 0;
}

/*===========================================
End of Source fatfs.c
=============================================*/
