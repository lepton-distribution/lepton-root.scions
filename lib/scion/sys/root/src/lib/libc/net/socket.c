/*
The contents of this file are subject to the Mozilla Public License Version 1.1
(the "License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at http://www.mozilla.org/MPL/

Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for the
specific language governing rights and limitations under the License.

The Original Code is Lepton.

The Initial Developer of the Original Code is Philippe Le Boulanger.
Portions created by Philippe Le Boulanger are Copyright (C) 2017. All Rights Reserved.

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
#include "kernel/core/limits.h"
#include "kernel/core/dirent.h"
#include "kernel/core/errno.h"
#include "kernel/core/kernel.h"
#include "kernel/core/kernel_io.h"
#include "kernel/core/system.h"
#include "kernel/core/process.h"
#include "kernel/core/fcntl.h"
#include "kernel/core/stat.h"

#include "kernel/fs/vfs/vfstypes.h"
#include "kernel/fs/vfs/vfskernel.h"
#include "kernel/fs/vfs/vfs.h"

#include "kernel/core/net/socket.h"
#include "kernel/core/net/kernel_net_core_socket.h"

#include "lib/libc/unistd.h"
#include "lib/libc/net/netdb.h"
/*============================================
| Global Declaration
==============================================*/




/*============================================
| Implementation
==============================================*/

/*--------------------------------------------
| Name: libc_socket
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int libc_socket( int domain, int type, int protocol) {
   int error;
   int _fd;
   pid_t pid;
   kernel_pthread_t* pthread_ptr;
   desc_t desc;
   //
   if ((_fd = open("/dev/net/socket", O_RDWR, 0)) < 0) {
      return -1;
   }
   //
   if (_fd<0)
      return -1;
   //
   if (!(pthread_ptr = kernel_pthread_self()))
      return -1;
   //
   if ((pid = pthread_ptr->pid)<1)
      return -1;
   //
   desc = process_lst[pid]->desc_tbl[_fd];
   //
   if (desc<0)
      return -1;
   //
   if ((error = kernel_net_core_socket(desc, domain, type, protocol)) < 0) {
      close(_fd);
      return -1;
   }
   //
   return _fd;
}

/*--------------------------------------------
| Name: libc_bind
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int libc_bind(int fd, struct sockaddr *name, socklen_t namelen) {
   int error;
   pid_t pid;
   kernel_pthread_t* pthread_ptr;
   desc_t desc;
   //
   if (fd<0)
      return -1;
   //
   if (!(pthread_ptr = kernel_pthread_self()))
      return -1;
   //
   if ((pid = pthread_ptr->pid)<1)
      return -1;
   //
   desc = process_lst[pid]->desc_tbl[fd];
   //
   if (desc<0)
      return -1;
   //
   error = kernel_net_core_bind(desc, name, namelen);
   //
   return error;
}

/*--------------------------------------------
| Name: libc_accept
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int libc_accept(int fd, void *addr, uint32_t *addrlen) {
   int _fd;
   int native_socket_fd;
   int error;
   pid_t pid;
   kernel_pthread_t* pthread_ptr;
   desc_t desc;
   desc_t desc_accepted;
   //
   if (fd<0)
      return -1;
   //
   if (!(pthread_ptr = kernel_pthread_self()))
      return -1;
   //
   if ((pid = pthread_ptr->pid)<1)
      return -1;
   //
   desc = process_lst[pid]->desc_tbl[fd];
   //
   if (desc<0)
      return -1;
  
   //
   if ((native_socket_fd = kernel_net_core_accept(desc, addr, addrlen)) < 0) {
      return -1;
   }
   //
   if ((_fd = open("/dev/net/socket", O_RDWR, 0)) < 0) {
      //todo disconnect native_socket_fd with kernel_net_core_close() or another primitive??.
      return -1;
   }
   //
   desc_accepted = process_lst[pid]->desc_tbl[_fd];
   //
   if ((error = kernel_net_core_accepted(desc_accepted, native_socket_fd)) < 0) {
      //todo disconnect native_socket_fd with kernel_net_core_close() or another primitive??.
      return -1;
   }
   //
   return _fd;
}

/*--------------------------------------------
| Name: libc_connect
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int libc_connect(int fd, struct sockaddr *name, socklen_t namelen) {
   int error;
   pid_t pid;
   kernel_pthread_t* pthread_ptr;
   desc_t desc;
   //
   if (fd<0)
      return -1;
   //
   if (!(pthread_ptr = kernel_pthread_self()))
      return -1;
   //
   if ((pid = pthread_ptr->pid)<1)
      return -1;
   //
   desc = process_lst[pid]->desc_tbl[fd];
   //
   if (desc<0)
      return -1;
   //
   error=kernel_net_core_connect(desc, name, namelen);
   //
   return error;
}

/*--------------------------------------------
| Name: libc_listen
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int libc_listen(int fd, int backlog) {
   int error;
   pid_t pid;
   kernel_pthread_t* pthread_ptr;
   desc_t desc;
   //
   if (fd<0)
      return -1;
   //
   if (!(pthread_ptr = kernel_pthread_self()))
      return -1;
   //
   if ((pid = pthread_ptr->pid)<1)
      return -1;
   //
   desc = process_lst[pid]->desc_tbl[fd];
   //
   if (desc<0)
      return -1;
   //
   error=kernel_net_core_listen(desc, backlog);
   //
   return error;
}

/*--------------------------------------------
| Name: libc_recv
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int libc_recv(int fd, void *mem, int len, unsigned int flags) {
   pid_t pid;
   kernel_pthread_t* pthread_ptr;
   desc_t desc;
   //
   if (fd<0)
      return -1;
   //
   if (len <= 0)
      return -1;
   //
   if (!(pthread_ptr = kernel_pthread_self()))
      return -1;
   //
   if ((pid = pthread_ptr->pid)<1)
      return -1;
   //
   desc = process_lst[pid]->desc_tbl[fd];
   //
   if (desc<0)
      return -1;
   //
   if (!(ofile_lst[desc].oflag&O_RDONLY))
      return -1;
   //
   if (!ofile_lst[desc].attr&(S_IFBLK | S_IFCHR)) {
      return -1;
   }

   //direct access to device driver
   return kernel_io_read_args(desc, mem, len, KERNEL_IO_READWRITE_ARGS, flags);
}

/*--------------------------------------------
| Name: libc_recvfrom
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int libc_recvfrom(int fd, void *mem, int len, unsigned int flags, struct sockaddr *from, socklen_t *fromlen) {
   pid_t pid;
   kernel_pthread_t* pthread_ptr;
   desc_t desc;
   //
   if (fd<0)
      return -1;
   //
   if (len <= 0)
      return -1;
   //
   if (!(pthread_ptr = kernel_pthread_self()))
      return -1;
   //
   if ((pid = pthread_ptr->pid)<1)
      return -1;
   //
   desc = process_lst[pid]->desc_tbl[fd];
   //
   if (desc<0)
      return -1;
   //
   if (!(ofile_lst[desc].oflag&O_RDONLY))
      return -1;
   //
   if (!ofile_lst[desc].attr&(S_IFBLK | S_IFCHR)) {
      return -1;
   }
   //direct access to device driver
   return kernel_io_read_args(desc, mem, len, KERNEL_IO_READWRITE_ARGS, flags, from, fromlen);
}

/*--------------------------------------------
| Name: libc_send
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int libc_send(int fd, const void *dataptr, int size, unsigned int flags) {
   pid_t pid;
   kernel_pthread_t* pthread_ptr;
   desc_t desc;
   //
   if (fd<0)
      return -1;
   //
   if (size <= 0)
      return -1;
   //
   if (!(pthread_ptr = kernel_pthread_self()))
      return -1;
   //
   if ((pid = pthread_ptr->pid)<1)
      return -1;
   //
   desc = process_lst[pid]->desc_tbl[fd];
   //
   if (desc<0)
      return -1;
   //
   if (!(ofile_lst[desc].oflag&O_WRONLY))
      return -1;
   //
   if (!ofile_lst[desc].attr&(S_IFBLK | S_IFCHR)) {
      return -1;
   }

   //direct access to device driver
   return kernel_io_write_args(desc, dataptr, size, KERNEL_IO_READWRITE_ARGS, flags);
}

/*--------------------------------------------
| Name: libc_sendto
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int libc_sendto(int fd, void *dataptr, int size, unsigned int flags, struct sockaddr *to, socklen_t tolen) {
   pid_t pid;
   kernel_pthread_t* pthread_ptr;
   desc_t desc;
   //
   if (fd<0)
      return -1;
   //
   if (size <= 0)
      return -1;
   //
   if (!(pthread_ptr = kernel_pthread_self()))
      return -1;
   //
   if ((pid = pthread_ptr->pid)<1)
      return -1;
   //
   desc = process_lst[pid]->desc_tbl[fd];
   //
   if (desc<0)
      return -1;
   //
   if (!(ofile_lst[desc].oflag&O_WRONLY))
      return -1;
   //
   if (!ofile_lst[desc].attr&(S_IFBLK | S_IFCHR)) {
      return -1;
   }

   //direct access to device driver
   return kernel_io_write_args(desc, dataptr, size, KERNEL_IO_READWRITE_ARGS, flags,to,tolen);
}

/*--------------------------------------------
| Name: libc_shutdown
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int libc_shutdown(int fd, int how) {
   int error;
   pid_t pid;
   kernel_pthread_t* pthread_ptr;
   desc_t desc;
   //
   if (fd<0)
      return -1;
   //
   if (!(pthread_ptr = kernel_pthread_self()))
      return -1;
   //
   if ((pid = pthread_ptr->pid)<1)
      return -1;
   //
   desc = process_lst[pid]->desc_tbl[fd];
   //
   if (desc<0)
      return -1;
   //
   error = kernel_net_core_shutdown(desc, how);
   //
   return error;
}

/*--------------------------------------------
| Name: libc_getpeername
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int libc_getpeername(int fd, struct sockaddr *name, socklen_t *namelen) {
   int error;
   pid_t pid;
   kernel_pthread_t* pthread_ptr;
   desc_t desc;
   //
   if (fd<0)
      return -1;
   //
   if (!(pthread_ptr = kernel_pthread_self()))
      return -1;
   //
   if ((pid = pthread_ptr->pid)<1)
      return -1;
   //
   desc = process_lst[pid]->desc_tbl[fd];
   //
   if (desc<0)
      return -1;
   //
   error = kernel_net_core_getpeername(desc, name, namelen);
   //
   return error;
}

/*--------------------------------------------
| Name: libc_getsockname
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int libc_getsockname(int fd, struct sockaddr *name, socklen_t *namelen) {
   int error;
   pid_t pid;
   kernel_pthread_t* pthread_ptr;
   desc_t desc;
   //
   if (fd<0)
      return -1;
   //
   if (!(pthread_ptr = kernel_pthread_self()))
      return -1;
   //
   if ((pid = pthread_ptr->pid)<1)
      return -1;
   //
   desc = process_lst[pid]->desc_tbl[fd];
   //
   if (desc<0)
      return -1;
   //
   error = kernel_net_core_getsockname(desc, name, namelen);
   //
   return error;
}

/*--------------------------------------------
| Name: libc_getsockopt
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int libc_getsockopt(int fd, int level, int optname, void *optval, socklen_t *optlen) {
   int error;
   pid_t pid;
   kernel_pthread_t* pthread_ptr;
   desc_t desc;
   //
   if (fd<0)
      return -1;
   //
   if (!(pthread_ptr = kernel_pthread_self()))
      return -1;
   //
   if ((pid = pthread_ptr->pid)<1)
      return -1;
   //
   desc = process_lst[pid]->desc_tbl[fd];
   //
   if (desc<0)
      return -1;
   //
   error = kernel_net_core_getsockopt(desc, level, optname, optval, optlen);
   //
   return error;
}

/*--------------------------------------------
| Name: libc_setsockopt
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int libc_setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen) {
   int error;
   pid_t pid;
   kernel_pthread_t* pthread_ptr;
   desc_t desc;
   //
   if (fd<0)
      return -1;
   //
   if (!(pthread_ptr = kernel_pthread_self()))
      return -1;
   //
   if ((pid = pthread_ptr->pid)<1)
      return -1;
   //
   desc = process_lst[pid]->desc_tbl[fd];
   //
   if (desc<0)
      return -1;
   //
   error = kernel_net_core_setsockopt(desc, level, optname, optval, optlen);
   //
   return error;
}

/*--------------------------------------------
| Name: libc_gethostbyname
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
struct libc_hostent* libc_gethostbyname(const char *name) {
   int _fd;
   pid_t pid;
   kernel_pthread_t* pthread_ptr;
   desc_t desc;
   //to do use alloca once
   volatile struct libc_hostent hostent;
   struct libc_hostent* p_hostent;
   //

   //
   if (name == (char*)0)
      return (struct libc_hostent*)0;
   //
   if ((_fd = open("/dev/net/socket", O_RDWR, 0)) < 0) {
      return (struct libc_hostent*)0;
   }
   //
   if (_fd<0)
      return (struct libc_hostent*)0;
   //
   if (!(pthread_ptr = kernel_pthread_self()))
      return (struct libc_hostent*)0;
   //
   if ((pid = pthread_ptr->pid)<1)
      return (struct libc_hostent*)0;
   //
   desc = process_lst[pid]->desc_tbl[_fd];
   //
   if (desc<0)
      return (struct libc_hostent*)0;
   
   //
   p_hostent = kernel_net_core_gethostbyname(desc, (struct libc_hostent*)&hostent, name);
   //
   close(_fd);
   //
   return p_hostent;
}

/*============================================
| End of Source  : libc_socket.c
==============================================*/