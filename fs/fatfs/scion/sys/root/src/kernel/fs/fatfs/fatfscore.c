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
#include "kernel/core/kernel_compiler.h"
#include "kernel/core/system.h"
#include "kernel/core/systime.h"
#include "kernel/core/stat.h"
#include "kernel/core/fcntl.h"

#include "kernel/fs/vfs/vfstypes.h"
#include "kernel/fs/vfs/vfsdev.h"

#include "kernel/fs/fatfs/fatfs.h"
#include "kernel/fs/fatfs/fatfscore.h"

#include "kernel/fs/fatfs/core/integer.h"	/* Basic integer types */
#include "kernel/fs/fatfs/core/ffconf.h"		/* FatFs configuration options */
#include "kernel/fs/fatfs/core/diskio.h"		
#include "kernel/fs/fatfs/core/ff_gen_drv.h"
#include "kernel/fs/fatfs/core/ff.h"	
#include "kernel/fs/fatfs/core/drivers/sd_diskio.h" 



/*===========================================
Global Declaration
=============================================*/
#define COMPILER_PRAGMA(arg)            _Pragma(#arg)


#if defined   (__CC_ARM)
	#define COMPILER_SECTION(a)    __attribute__((__section__(a)))
#elif defined (__ICCARM__)
	#define COMPILER_SECTION(a)    COMPILER_PRAGMA(location = a)
#elif defined (__GNUC__)
	#define COMPILER_SECTION(a)    __attribute__((__section__(a)))
#endif

#if defined   (__CC_ARM)
	#define COMPILER_ALIGNED(a)    __attribute__((__aligned__(a)))
#elif defined (__ICCARM__)
	#define COMPILER_ALIGNED(a)    COMPILER_PRAGMA(data_alignment = a)
#elif defined (__GNUC__)
	#define COMPILER_ALIGNED(a)    __attribute__((__aligned__(a)))
#endif


static char g_sd_path[4];  /* SD logical drive path */

//
typedef union {
   uint8_t padding[16];
   FIL   fil;
   DIR   dir;
}fatfscore_u;

#define USE_ALIGNED_FATFS

#ifdef USE_ALIGNED_FATFS
   //
   typedef struct _ALIGN_FATFS {
      uint8_t padding[16];
      FATFS fs;
   } ALIGN_FATFS;
   //COMPILER_SECTION(".ram_nocache")
   COMPILER_ALIGNED(32) ALIGN_FATFS aligned_fs;
   //
   #define g_disk_fatfs aligned_fs.fs
   
   typedef struct _ALIGN_FATFS_LIST {
      uint8_t padding[16];
      fatfscore_u fatfs_list_aligned[MAX_OPEN_FILE];
   } ALIGN_FATFS_LIST;
   //COMPILER_SECTION(".ram_nocache")
   COMPILER_ALIGNED(32) ALIGN_FATFS_LIST aligned_fat_fs;
   //
   #define fatfs_list aligned_fat_fs.fatfs_list_aligned
#else
   static fatfscore_u fatfs_list[MAX_OPEN_FILE];
   static FATFS g_disk_fatfs;
#endif









/*===========================================
Implementation
=============================================*/

/*-------------------------------------------
| Name:get_fattime
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
DWORD get_fattime(void){
   return 0;
}

/*-------------------------------------------
| Name:ff_convert
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
WCHAR ff_convert (WCHAR wch, UINT dir) { 
   if (wch < 0x80) { 
           /* ASCII Char */ 
           return wch; 
   }  

   /* I don't support unicode it is too big! */ 
   return 0; 
}  

/*-------------------------------------------
| Name:ff_wtoupper
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
WCHAR ff_wtoupper (WCHAR wch) { 
   if (wch < 0x80) {      
      /* ASCII Char */ 
      if (wch >= 'a' && wch <= 'z') { 
               wch &= ~0x20; 
      } 
      return wch; 
   }  

   /* I don't support unicode it is too big! */ 
   return 0; 
} 

/*-------------------------------------------
| Name:fatfscore_statfs
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int fatfscore_statfs(mntdev_t* pmntdev,struct statvfs *statvfs){
   memset(statvfs,0,sizeof(struct statvfs));

   statvfs->f_bsize  = _MAX_SS; /* sector size in byte */
   statvfs->f_frsize = _MAX_SS; /* sector size in byte */

   statvfs->f_blocks = g_disk_fatfs.fsize; /* Sectors per FAT */
   statvfs->f_bfree  = g_disk_fatfs.free_clust*g_disk_fatfs.csize; /* (Number of free clusters) x Sectors per cluster (1,2,4...128) */
   statvfs->f_namemax = _MAX_LFN;

   return 0;
}

/*-------------------------------------------
| Name: fatfscore_readfs
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int fatfscore_readfs(mntdev_t* pmntdev){
   uint8_t retSD;
   
   //
	memset(&g_disk_fatfs, 0, sizeof(FATFS));

   //
   memset(g_sd_path,0,sizeof(g_sd_path));
   //
   /*## FatFS: Link the SD driver ###########################*/
   retSD = FATFS_LinkDriver(&SD_Driver, g_sd_path);
   if(retSD!=FR_OK){
      return -1;
   }

   /* USER CODE BEGIN Init */
   /* additional user code for init */     
   // mount immediatly option (1)
   if(f_mount(&g_disk_fatfs, (TCHAR const*)g_sd_path, 1) != FR_OK){
      return -1;
   }
   //
   pmntdev->inodetbl_size = g_disk_fatfs.n_fatent;
   //
   return 0;
}

/*-------------------------------------------
| Name:fatfscore_extern_create
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int fatfscore_extern_create(desc_t desc, const char* path, mode_t mode){
   FRESULT fr; 
   //
   fr = f_open(&fatfs_list[desc].fil, path, FA_WRITE | FA_CREATE_ALWAYS);
   if(fr!=FR_OK)
      return -1;
   //
   fr = f_close(&fatfs_list[desc].fil);
   if(fr!=FR_OK)
      return -1;
   //
   return 0;
}

/*-------------------------------------------
| Name:fatfscore_extern_create
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int fatfscore_extern_open(desc_t desc, const char* path, int oflag, mode_t mode){
   FRESULT fr; 
   BYTE b_oflag=0;
   
   //
   if(oflag & O_WRONLY){
      b_oflag |= FA_WRITE;
   }
   if(oflag & O_RDONLY){
      b_oflag |= FA_READ;
   }
   if( (oflag & O_CREAT) && (oflag & O_EXCL) ){
      b_oflag |= FA_CREATE_NEW;
      b_oflag |= FA_WRITE;
   }else if( (oflag & O_CREAT) && !(oflag & O_EXCL) && (oflag & O_TRUNC) ){
       b_oflag |= FA_CREATE_ALWAYS;
       b_oflag |= FA_WRITE;
   }else if( !(oflag & O_CREAT) && !(oflag & O_EXCL) && !(oflag & O_TRUNC) ){
       b_oflag |= FA_OPEN_ALWAYS;
       b_oflag |= FA_WRITE;
   }
#if 0
   if(oflag & O_APPEND){
      b_oflag |= FA_OPEN_APPEND;
   }
#endif
   
   // note:
   // FA_READ Specifies read access to the object. Data can be read from the file.
   // FA_WRITE	Specifies write access to the object. Data can be written to the file. Combine with FA_READ for read-write access.
   // FA_OPEN_EXISTING	Opens the file. The function fails if the file is not existing. (Default)
   // FA_CREATE_NEW:	Creates a new file. The function fails with FR_EXIST if the file is existing.
   // FA_CREATE_ALWAYS:	Creates a new file. If the file is existing, it will be truncated and overwritten.
   // FA_OPEN_ALWAYS:	Opens the file if it is existing. If not, a new file will be created.
   // FA_OPEN_APPEND:	Same as FA_OPEN_ALWAYS except the read/write pointer is set end of the file.
   
   //
   fr = f_open(&fatfs_list[desc].fil, path, b_oflag);
   if(fr!=FR_OK)
      return -1;
   //
   ofile_lst[desc].offset  =  f_tell(&fatfs_list[desc].fil);
   ofile_lst[desc].attr    =  S_IFREG;
   ofile_lst[desc].size    =  f_size(&fatfs_list[desc].fil);
   //
   return 0;
}

/*-------------------------------------------
| Name:fatfscore_extern_close
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int fatfscore_extern_close(desc_t desc){
   FRESULT fr; 
   //
   fr = f_close(&fatfs_list[desc].fil);
   if(fr!=FR_OK)
      return -1;
   //
   return 0;
}

/*-------------------------------------------
| Name:fatfscore_extern_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int fatfscore_extern_read(desc_t desc, char* buf, int size){
   FRESULT fr; 
   int cb; 
   //
   fr = f_read(&fatfs_list[desc].fil,buf,size,&cb);
   if(fr!=FR_OK)
      return -1;
   //
   return cb;
}

/*-------------------------------------------
| Name:fatfscore_extern_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int fatfscore_extern_write(desc_t desc, const char* buf, int size){
   FRESULT fr; 
   int cb; 
   //
   fr = f_write(&fatfs_list[desc].fil,buf,size,&cb);
   if(fr!=FR_OK)
      return -1;
   //
   return cb;
}

/*-------------------------------------------
| Name:fatfscore_extern_seek
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int fatfscore_extern_seek(desc_t desc, off_t offset, int origin){
   FRESULT fr; 
   //
   switch(origin){
      case SEEK_SET:
         //Begin of the File
         /* Move to offset of offset from top of the file */
         fr = f_lseek(&fatfs_list[desc].fil, offset);
      break;

      case SEEK_CUR:
         //Current position of the file
         /* Forward 3000 bytes */
         fr = f_lseek(&fatfs_list[desc].fil, f_tell(&fatfs_list[desc].fil) + offset);
      break;

      case SEEK_END:
         //End of the File
         /* Move to end of the file to append data */
         fr = f_lseek(&fatfs_list[desc].fil, f_size(&fatfs_list[desc].fil) + offset);
      break;

      default:
         return -1;
   }
   //
   if(fr!=FR_OK)
      return -1;
   //
   ofile_lst[desc].offset = f_tell(&fatfs_list[desc].fil);
   //
   return 0;
}

/*-------------------------------------------
| Name:fatfscore_extern_truncate
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int fatfscore_extern_truncate(desc_t desc, off_t length){
   FRESULT fr; 
   //
   fr = f_lseek(&fatfs_list[desc].fil, length);
   if(fr!=FR_OK)
      return -1;
   //
   fr = f_truncate(&fatfs_list[desc].fil);
   if(fr!=FR_OK)
     return -1;
   //
   return 0;
}

/*-------------------------------------------
| Name:fatfscore_extern_sync
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int fatfscore_extern_sync(desc_t desc){
   FRESULT fr; 
   //
   fr = f_sync(&fatfs_list[desc].fil);
   if(fr!=FR_OK)
      return -1;
   //
   return 0;
}

/*-------------------------------------------
| Name:fatfscore_extern_mkdir
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int fatfscore_extern_mkdir(const char* path, mode_t mode){
   FRESULT fr; 
   //
   fr = f_mkdir(path);
   if(fr!=FR_OK)
      return -1;
   //
   return 0;
}

/*-------------------------------------------
| Name:fatfscore_extern_opendir
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int fatfscore_extern_opendir(desc_t desc, const char* path){
   FRESULT fr; 
   //
   fr = f_opendir(&fatfs_list[desc].dir,path);
   if(fr!=FR_OK)
      return -1;
   //
   return 0;
}

/*-------------------------------------------
| Name:fatfscore_extern_closedir
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int fatfscore_extern_closedir(desc_t desc){
   FRESULT fr; 
   //
   fr = f_closedir(&fatfs_list[desc].dir);
   if(fr!=FR_OK)
      return -1;
   //
   return 0;
}

/*-------------------------------------------
| Name:fatfscore_extern_readdir
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int fatfscore_extern_readdir(desc_t desc, fatfs_dirent_t* pdirent){
   FRESULT fr; 
   static FILINFO fno;
   static TCHAR lfname[_MAX_LFN]; 
   //
   fno.lfname = lfname; 
   fno.lfsize = _MAX_LFN - 1; 
   //
   fr = f_readdir(&fatfs_list[desc].dir,&fno);
   if(fr!=FR_OK || fno.fname[0] == 0)
      return -1;
   
   //pdirent->inodenb = __cvt2logicnode(desc,p_kofs_node->ino);
   pdirent->inodenb = __cvt2logicnode(desc,1);
   memset(pdirent->d_name,0,sizeof(pdirent->d_name));
   if(fno.lfname[0]=='\0'){
      memcpy(pdirent->d_name,fno.fname,(sizeof(fno.fname)>sizeof(pdirent->d_name)?(sizeof(pdirent->d_name)-1):sizeof(fno.fname)));
   }else{
      memcpy(pdirent->d_name,fno.lfname,sizeof(pdirent->d_name)-1);
   }
   //
   return 0;
}

/*-------------------------------------------
| Name:fatfscore_extern_telldir
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int fatfscore_extern_telldir(desc_t desc){
   FRESULT fr;
   
   return fatfs_list[desc].dir.index;
}

/*-------------------------------------------
| Name:fatfscore_extern_seekdir
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int fatfscore_extern_seekdir(desc_t desc, long loc){
   FRESULT fr;
   //
   if(loc!=0)
      return -1;
   //
   fr = f_rewinddir(&fatfs_list[desc].dir);
    if(fr!=FR_OK)
      return -1;
   
   //
   return 0;
}

/*-------------------------------------------
| Name:fatfscore_extern_unlink
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int fatfscore_extern_unlink(const char* path){
   FRESULT fr;
   //
   fr=f_unlink(path);
   if(fr!=FR_OK)
      return -1;
   //
   return 0;
}

/*-------------------------------------------
| Name:fatfscore_extern_rename
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int fatfscore_extern_rename(const char*  old_name, char* new_name){
   FRESULT fr;
   //
   fr=f_rename(old_name,new_name);
   if(fr!=FR_OK)
      return -1;
   //
   return 0;
}

/*-------------------------------------------
| Name:fatfscore_extern_stat
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int fatfscore_extern_stat(const char* path, struct stat * stat){
   FRESULT fr;
   static FILINFO fno;
   static TCHAR lfname[_MAX_LFN];
   //
   fno.lfname = lfname; 
   fno.lfsize = _MAX_LFN - 1; 
   
   //roor directory
   if(strcmp(path,"/")==0){
      static DIR root_dir;
      fr=f_opendir(&root_dir,path);
      if(fr!=FR_OK)
         return -1;
      //
      fno.fattrib=AM_DIR;
      fno.fsize=0;
      //
      f_close(&root_dir);
   }else{
      fr=f_stat(path,&fno);	
      if(fr!=FR_OK)
         return -1;
   }
  
   //
   if(fno.fattrib&AM_DIR){
      stat->st_mode = S_IFDIR;      
   }else{
      stat->st_mode = S_IFREG;
   }
   
   //notes:
   //fno.fsize;			/* File size */
	//fno.fdate;			/* Last modified date */
	//fno.ftime;			/* Last modified time */
   //
   stat->st_size  = fno.fsize;             //file size in bytes (if file is a regular file)
   stat->st_atime = fno.ftime;           //time of last access
   stat->st_mtime = fno.ftime;           //time of last data modification
   stat->st_ctime = fno.ftime;           //time of last status change
   
   //
   return 0;
}




/*===========================================
End of Source fatfscore.c
=============================================*/