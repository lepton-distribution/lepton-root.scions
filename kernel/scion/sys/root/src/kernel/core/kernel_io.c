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
#include "kernel/core/types.h"
#include "kernel/core/kernel.h"
#include "kernel/core/kernel_io.h"
#include "kernel/core/fcntl.h"
#include "kernel/core/system.h"
#include "kernel/core/syscall.h"
#include "kernel/core/process.h"
#include "kernel/core/ioctl.h"
#include "kernel/fs/vfs/vfs.h"
#include "kernel/fs/vfs/vfskernel.h"
#include "kernel/core/stat.h"

#include "kernel/core/kal.h"



/*===========================================
Global Declaration
=============================================*/
const int STDIN_FILENO  = 0;
const int STDOUT_FILENO = 1;
const int STDERR_FILENO = 2;

/*===========================================
Implementation
=============================================*/


/*-------------------------------------------
| Name: kernel_io_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
ssize_t kernel_io_read(desc_t desc, void *buf, size_t nbyte){
   pid_t pid=-1;
   kernel_pthread_t* pthread_ptr=(void*)0;

   if(desc<0)
      return -1;
   
   if(nbyte<=0)
      return -1;

   if((pthread_ptr = kernel_pthread_self())){
      pid = pthread_ptr->pid;
   }

   if(!(ofile_lst[desc].oflag&O_RDONLY))
      return -1;

   //is not implemented for this device dev?
   if(!ofile_lst[desc].pfsop->fdev.fdev_read)
     return -1;

   //
   __lock_io(pthread_ptr,ofile_lst[desc].desc,O_RDONLY);
   //check thread owner
   __atomic_in();
   {
      desc_t _desc=ofile_lst[desc].desc;
      //
      if(ofile_lst[_desc].owner_pthread_ptr_read!=pthread_ptr) {
         do {
            //check
            if(ofile_lst[_desc].used<=0) {
               __atomic_out();
               __unlock_io(pthread_ptr,ofile_lst[desc].desc,O_RDONLY);
               return -1; //error, stream not coherent :(
            }
            //
            //begin of section: protection from io interrupt
            __disable_interrupt_section_in();
            //
            ofile_lst[_desc].owner_pthread_ptr_read=pthread_ptr;
            ofile_lst[_desc].owner_pid=pid;
            //end of section: protection from io interrupt
            __disable_interrupt_section_out();
            //
            //aware: continue operation on original desc (see fattach() and _vfs_open() note 1)
         } while((_desc=ofile_lst[_desc].desc_nxt[0])>=0);
      }
   }
   __atomic_out();

   //
   if((ofile_lst[desc].attr&S_IFCHR) && !(ofile_lst[desc].oflag&O_NONBLOCK)) {
      while(ofile_lst[desc].pfsop->fdev.fdev_isset_read
            && ofile_lst[desc].pfsop->fdev.fdev_isset_read(desc)) {
         __wait_io_int(pthread_ptr); //wait incomming data
      }
      nbyte=ofile_lst[desc].pfsop->fdev.fdev_read(desc,buf,nbyte);
      //profiler
      __io_profiler_stop(desc);
      __io_profiler_add_result(desc,O_RDONLY,nbyte,__io_profiler_get_counter(desc));
      //
      __unlock_io(pthread_ptr,ofile_lst[desc].desc,O_RDONLY);
      return nbyte;
   }else if((ofile_lst[desc].attr&S_IFCHR) && (ofile_lst[desc].oflag&O_NONBLOCK)) {
      //profiler
      __io_profiler_start(desc);
      nbyte=ofile_lst[desc].pfsop->fdev.fdev_read(desc,buf,nbyte);
      __unlock_io(pthread_ptr,ofile_lst[desc].desc,O_RDONLY);
      //profiler
      __io_profiler_stop(desc);
      __io_profiler_add_result(desc,O_RDONLY,nbyte,__io_profiler_get_counter(desc));
      //
      return nbyte;
   }else if(ofile_lst[desc].attr&S_IFBLK) {
      //no specific action.
      //profiler
      __io_profiler_start(desc);
      nbyte =  ofile_lst[desc].pfsop->fdev.fdev_read(desc,buf,nbyte);
      __unlock_io(pthread_ptr,ofile_lst[desc].desc,O_RDONLY);
      //profiler
      __io_profiler_stop(desc);
      __io_profiler_add_result(desc,O_RDONLY,nbyte,__io_profiler_get_counter(desc));
      //
      return nbyte;
   }
   __unlock_io(pthread_ptr,ofile_lst[desc].desc,O_RDONLY);
   return -1;
}

/*-------------------------------------------
| Name: kernel_io_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
ssize_t kernel_io_write(desc_t desc, const void *buf, size_t nbyte){
   pid_t pid=-1;
   kernel_pthread_t* pthread_ptr=(void*)0;
   int cb=-1;

   if(desc<0)
      return -1;
    
   if(nbyte<=0)
      return -1;

   if((pthread_ptr = kernel_pthread_self())){
      pid = pthread_ptr->pid;
   }

   if(!(ofile_lst[desc].oflag&O_WRONLY))
      return -1;

   //is not implemented for this device dev?
   if(!ofile_lst[desc].pfsop->fdev.fdev_write)
      return -1;

   //
   __lock_io(pthread_ptr,ofile_lst[desc].desc,O_WRONLY);
   //check thread owner
   __atomic_in();
   {
      desc_t _desc=ofile_lst[desc].desc;
      //
      if(ofile_lst[_desc].owner_pthread_ptr_write!=pthread_ptr) {
         do {
            //check
            if(ofile_lst[_desc].used<=0) {
               __atomic_out();
               __unlock_io(pthread_ptr,ofile_lst[desc].desc,O_WRONLY);
               return -1; //error, stream not coherent :(
            }
            //
            //begin of section: protection from io interrupt
            __disable_interrupt_section_in();
            //
            ofile_lst[_desc].owner_pthread_ptr_write=pthread_ptr;
            ofile_lst[_desc].owner_pid=pid;
            //end of section: protection from io interrupt
            __disable_interrupt_section_out();
            //
            //aware: continue operation on original desc (see fattach() and _vfs_open() note 1)
         } while((_desc=ofile_lst[_desc].desc_nxt[1])>=0);
      }
   }
   __atomic_out();
   //
   if((ofile_lst[desc].attr&S_IFCHR) && !(ofile_lst[desc].oflag&O_NONBLOCK) && !(ofile_lst[desc].oflag&O_NSYNC) ) {
      //profiler
      __io_profiler_start(desc);
      //
      if((cb = ofile_lst[desc].pfsop->fdev.fdev_write(desc,(void*)buf,nbyte))<0) {
         //profiler
         __io_profiler_stop(desc);
         __io_profiler_add_result(desc,O_WRONLY,0,0);
         //
         __unlock_io(pthread_ptr,ofile_lst[desc].desc,O_WRONLY);
         return -1;
      }
      //
      if(pthread_ptr){
         do {
            __wait_io_int(pthread_ptr); //wait all data are transmitted  
         } while(ofile_lst[desc].pfsop->fdev.fdev_isset_write
                 && ofile_lst[desc].pfsop->fdev.fdev_isset_write(desc));
      }
      //profiler
      __io_profiler_stop(desc);
      __io_profiler_add_result(desc,O_WRONLY,nbyte,__io_profiler_get_counter(desc));
      //
      //printf("__wait_io_int ok\n");
   }else if((ofile_lst[desc].attr&S_IFCHR) && !(ofile_lst[desc].oflag&O_NONBLOCK) && (ofile_lst[desc].oflag&O_NSYNC) ) {
      //profiler
      __io_profiler_start(desc);
      //write operation is possible?
      if(pthread_ptr){
         while(ofile_lst[desc].pfsop->fdev.fdev_isset_write
                 && (ofile_lst[desc].pfsop->fdev.fdev_isset_write(desc)<0)){
            __wait_io_int(pthread_ptr); //wait write operation is possible  
         }
      }
      //
      if((cb = ofile_lst[desc].pfsop->fdev.fdev_write(desc,(void*)buf,nbyte))<0) {
         //profiler
         __io_profiler_stop(desc);
         __io_profiler_add_result(desc,O_WRONLY,0,0);
         //
         __unlock_io(pthread_ptr,ofile_lst[desc].desc,O_WRONLY);
         return -1;
      }
      
      //profiler
      __io_profiler_stop(desc);
      __io_profiler_add_result(desc,O_WRONLY,nbyte,__io_profiler_get_counter(desc));
      //
      //printf("__wait_io_int ok\n");
   }else if((ofile_lst[desc].attr&S_IFCHR) && (ofile_lst[desc].oflag&O_NONBLOCK)) {
      //profiler
      __io_profiler_start(desc);
      //
      nbyte = ofile_lst[desc].pfsop->fdev.fdev_write(desc,(void*)buf,nbyte);
      //profiler
      __io_profiler_stop(desc);
      __io_profiler_add_result(desc,O_WRONLY,nbyte,__io_profiler_get_counter(desc));
   }else if(ofile_lst[desc].attr&S_IFBLK) {
      //no specific action.
      //profiler
      __io_profiler_start(desc);
      //
      cb =  ofile_lst[desc].pfsop->fdev.fdev_write(desc,(void*)buf,nbyte);
      //profiler
      __io_profiler_stop(desc);
      __io_profiler_add_result(desc,O_WRONLY,nbyte,__io_profiler_get_counter(desc));
   }
   //
   __unlock_io(pthread_ptr,ofile_lst[desc].desc,O_WRONLY);
   //
   return cb;
}




/*===========================================
End of Source kernel_io.c
=============================================*/
