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
#include <stdarg.h>
#include <string.h>

#include "kernel/core/types.h"
#include "kernel/core/kal.h"
#include "kernel/core/kernel.h"
#include "kernel/core/kernel_io.h"
#include "kernel/core/fcntl.h"
#include "kernel/core/system.h"
#include "kernel/core/syscall.h"
#include "kernel/core/process.h"
#include "kernel/core/ioctl.h"
#include "kernel/core/dirent.h"

#include "kernel/fs/vfs/vfs.h"
#include "kernel/fs/vfs/vfskernel.h"
#include "kernel/core/stat.h"




/*===========================================
Global Declaration
=============================================*/



/*===========================================
Implementation
=============================================*/

/*-------------------------------------------
| Name:mount
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int mount(int fstype,const char* dev_path, const char* mount_path){
   mount_t mount_dt;

   mount_dt.fstype = fstype;
   mount_dt.dev_path = dev_path;
   mount_dt.mount_path = mount_path;

   __mk_syscall(_SYSCALL_MOUNT,mount_dt)

   return mount_dt.ret;
}

/*-------------------------------------------
| Name:umount
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int umount(const char* mount_path){
   umount_t umount_dt;

   umount_dt.mount_path = mount_path;

   __mk_syscall(_SYSCALL_UMOUNT,umount_dt)

   return umount_dt.ret;
}

/*-------------------------------------------
| Name:sync
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
void sync(void){
   sync_t sync_dt;
   __mk_syscall(_SYSCALL_SYNC,sync_dt)
}

/*-------------------------------------------
| Name:open
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
#if defined(__GNUC__)
int __attribute__ ((visibility("hidden")))
open(const char *path, int oflag, mode_t mode){
#else
int open(const char *path, int oflag, mode_t mode){
#endif
   pid_t pid= _sys_getpid();
   open_t open_dt;

   if(!path)
      return -1;

   open_dt.path   = path;
   open_dt.oflag  = oflag;
   open_dt.mode   = mode;

   __mk_syscall(_SYSCALL_OPEN,open_dt)

   if(open_dt.fildes<0)
      return -1;


   if((open_dt.attr&S_IFIFO) && !(oflag&O_NONBLOCK)) {
      desc_t desc = process_lst[pid]->desc_tbl[open_dt.fildes];
      pipe_desc_t pipe_desc = ofile_lst[desc].ext.pipe_desc;
      desc_t desc_fifo_r = opipe_lst[pipe_desc].desc_r;
      desc_t desc_fifo_w = opipe_lst[pipe_desc].desc_w;

      if(oflag&O_RDONLY && !ofile_lst[desc_fifo_w].nb_writer) {
         __wait_io_int(kernel_pthread_self());
      }else if(oflag&O_WRONLY && !ofile_lst[desc_fifo_r].nb_reader) {
         __wait_io_int(kernel_pthread_self());
      }
   }

   return open_dt.fildes;
}

/*-------------------------------------------
| Name:creat
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int creat(const char *path, mode_t mode){
   return open(path,O_WRONLY|O_CREAT,mode);
}

/*-------------------------------------------
| Name:close
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
#if defined(__GNUC__)
int __attribute__ ((visibility("hidden")))
close(int fildes) {
#else
int close(int fildes){
#endif
   close_t close_dt;
   pid_t pid= _sys_getpid();

   if(fildes<0)
      return -1;

   if(process_lst[pid]->desc_tbl[fildes]<0)
      return -1;

   close_dt.fildes   = fildes;

   __mk_syscall(_SYSCALL_CLOSE,close_dt)

   return close_dt.ret;
}

/*-------------------------------------------
| Name:lseek
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
#if defined(__GNUC__)
off_t __attribute__ ((visibility("hidden")))
lseek(int fildes, off_t offset, int whence){
#else
off_t lseek(int fildes, off_t offset, int whence){
#endif
   kernel_pthread_t* pthread_ptr;
   pid_t pid=0;
   desc_t desc = -1;
   lseek_t lseek_dt;

   if(fildes<0)
      return -1;

   if(!(pthread_ptr = kernel_pthread_self()))
      return -1;

   if((pid= pthread_ptr->pid)<1)
      return -1;

   desc = process_lst[pid]->desc_tbl[fildes];
   if(desc<0)
      return -1;

   if(!(ofile_lst[desc].attr&(S_IFBLK|S_IFREG|S_IFDIR)))
      return -1;

   if(ofile_lst[desc].attr&S_IFBLK && ofile_lst[desc].pfsop->fdev.fdev_seek) {
      int r;
      __lock_io(pthread_ptr,ofile_lst[desc].desc,O_RDONLY);
      __lock_io(pthread_ptr,ofile_lst[desc].desc,O_WRONLY);
      r =ofile_lst[desc].pfsop->fdev.fdev_seek(desc,offset,whence);
      __unlock_io(pthread_ptr,ofile_lst[desc].desc,O_WRONLY);
      __unlock_io(pthread_ptr,ofile_lst[desc].desc,O_RDONLY);
      return r;
   }

   lseek_dt.fildes = fildes;
   lseek_dt.offset = offset;
   lseek_dt.whence = whence;

   __mk_syscall(_SYSCALL_SEEK,lseek_dt);

   return lseek_dt.offset;
}

/*-------------------------------------------
| Name:ioctl
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int ioctl(int fildes, int request, ... ){
   va_list ap;
   kernel_pthread_t* pthread_ptr;
   pid_t pid=0;
   desc_t desc = -1;
   int ret;

   if(fildes<0)
      return -1;

   if(!(pthread_ptr = kernel_pthread_self()))
      return -1;

   if((pid= pthread_ptr->pid)<1)
      return -1;

   desc = process_lst[pid]->desc_tbl[fildes];
   if(desc<0)
      return -1;

   if( !(ofile_lst[desc].attr&(S_IFCHR|S_IFBLK)))
      return -1;

   if(!ofile_lst[desc].pfsop->fdev.fdev_ioctl)
      return -1;  //not iplemented for this device

   va_start(ap, request);

   switch(request) {
   //not direct access, use system call to the kernel
   case I_LINK:
   case I_UNLINK: {
      ioctl_t ioctl_dt;

      ioctl_dt.fd = fildes;
      ioctl_dt.request = request;
      __va_list_copy(ioctl_dt.ap,ap);
      __mk_syscall(_SYSCALL_IOCTL,ioctl_dt);
      ret = ioctl_dt.ret;
   }
   break;

   //
   case FIONBIO: {
      int* p_on = va_arg( ap, int*);
      if (p_on && *p_on)
         ofile_lst[desc].oflag |= O_NONBLOCK;
      else
         ofile_lst[desc].oflag &= ~O_NONBLOCK;
      //
      va_start(ap, request);
      //
      ret = ofile_lst[desc].pfsop->fdev.fdev_ioctl(desc,request,ap);
   }
   break;

   //direct acces
   default:
      //__lock_io(pthread_ptr,desc);
      ret = ofile_lst[desc].pfsop->fdev.fdev_ioctl(desc,request,ap);
      //__unlock_io(pthread_ptr,desc);
      break;
   }

   va_end(ap);

   return ret;
}

/*-------------------------------------------
| Name:remove
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int remove(const char *path){
   remove_t remove_dt;
   remove_dt.ref = path;
   __mk_syscall(_SYSCALL_REMOVE,remove_dt);
   return remove_dt.ret;
}

/*-------------------------------------------
| Name:rename
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int rename(const char *old_name, const char *new_name){
   rename_t rename_dt;
   rename_dt.old_name = old_name;
   rename_dt.new_name = new_name;
   __mk_syscall(_SYSCALL_RENAME,rename_dt);
   return rename_dt.ret;
}

/*--------------------------------------------
| Name:        fattach
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int fattach(int fildes,const char *path){
   fattach_t fattach_dt;

   if(fildes<0)
      return -1;

   if(path==(const char*)0)
      return -1;

   fattach_dt.fd   = fildes;
   fattach_dt.path = path;

   __mk_syscall(_SYSCALL_FATTACH,fattach_dt);

   return fattach_dt.ret;
}

/*--------------------------------------------
| Name:        fdetach
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int fdetach(const char *path){
   fdetach_t fdetach_dt;


   if(path==(const char*)0)
      return -1;

   fdetach_dt.path = path;

   __mk_syscall(_SYSCALL_FDETACH,fdetach_dt);

   return fdetach_dt.ret;
}

/*-------------------------------------------
| Name:read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
#if defined(__GNUC__)
ssize_t __attribute__ ((visibility("hidden")))
read(int fildes, void *buf, size_t nbyte){
#else
ssize_t read(int fildes, void *buf, size_t nbyte){
#endif
   pid_t pid;
   kernel_pthread_t* pthread_ptr;
   desc_t desc;

   if(fildes<0)
      return -1;

   if(nbyte<=0)
      return -1;

   if(!(pthread_ptr = kernel_pthread_self()))
      return -1;

   if((pid= pthread_ptr->pid)<1)
      return -1;

   desc = process_lst[pid]->desc_tbl[fildes];

   if(desc<0)
      return -1;

   if(!(ofile_lst[desc].oflag&O_RDONLY))
      return -1;

   if(ofile_lst[desc].attr&(S_IFREG|S_IFDIR)) {
      read_t read_dt;
      read_dt.fildes= fildes;
      read_dt.buf    = buf;
      read_dt.nbyte  = nbyte;
      __mk_syscall(_SYSCALL_READ,read_dt);
      return read_dt.nbyte;
   }
   
   //direct access to device driver
   return kernel_io_read(desc,buf,nbyte);
}

/*-------------------------------------------
| Name:write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_ioflag;
#if defined(__GNUC__)
ssize_t __attribute__ ((visibility("hidden")))
write(int fildes, const void *buf, size_t nbyte){
#else
ssize_t write(int fildes, const void *buf, size_t nbyte){
#endif
   pid_t pid;
   kernel_pthread_t* pthread_ptr;
   desc_t desc;

   if(fildes<0)
      return -1;

   if(nbyte<=0)
      return -1;

   if(!(pthread_ptr = kernel_pthread_self()))
      return -1;

   if((pid= pthread_ptr->pid)<1)
      return -1;

   desc = process_lst[pid]->desc_tbl[fildes];

   if(desc<0)
      return -1;

   if(!(ofile_lst[desc].oflag&O_WRONLY))
      return -1;

   if(ofile_lst[desc].attr&(S_IFREG|S_IFDIR)) {
      write_t write_dt;
      write_dt.fildes = fildes;
      write_dt.buf    = (void*)buf;
      write_dt.nbyte  = nbyte;
      __mk_syscall(_SYSCALL_WRITE,write_dt);
      return write_dt.nbyte;
   }

  //direct access to device driver
   return kernel_io_write(desc,buf,nbyte);
}




/*===========================================
End of Source io.c
=============================================*/
