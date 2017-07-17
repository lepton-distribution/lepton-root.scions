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
#include "kernel/core/system.h"
#include "kernel/core/process.h"
#include "kernel/core/kernel.h"
#include "kernel/core/fcntl.h"
#include "kernel/core/stat.h"

#include "kernel/fs/vfs/vfstypes.h"
#include "kernel/fs/vfs/vfskernel.h"
#include "kernel/fs/vfs/vfs.h"


#if defined (__KERNEL_NET_IPSTACK) && defined(USE_LWIP)
   #include "kernel/net/lwip/include/lwip/sockets.h"
   #include "kernel/net/lwip/include/lwip/netdb.h"
#elif defined (__KERNEL_NET_IPSTACK) && defined(USE_UIP)
   #include "kernel/core/net/uip_core/uip_socket.h"
#elif defined (__KERNEL_NET_IPSTACK) && defined(USE_MODEMIP)
   #include "kernel/core/net/modem_core/modem_socket.h"
   #include "kernel/core/net/bsd/netdb.h"
#else
   #include "kernel/core/net/bsd/sys/socket.h"
   #include "kernel/core/net/bsd/inet/in.h" 
   #include "kernel/core/net/bsd/netdb.h"
#endif

#include "kernel/core/net/kernel_net_core_socket.h"
/*============================================
| Global Declaration
==============================================*/

//
static kernel_net_core_socket_t kernel_net_core_socket_list[MAX_OPEN_FILE];

/*============================================
| Implementation
==============================================*/
/*--------------------------------------------
| Name: kernel_net_core_set_socket_op
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
void kernel_net_core_set_socket_op(desc_t desc, const kernel_net_core_socket_op_t* p_kernel_net_core_socket_op) {
   kernel_net_core_socket_list[desc].p_kernel_net_core_socket_op = p_kernel_net_core_socket_op;
   ofile_lst[desc].p=&kernel_net_core_socket_list[desc];
}
/*--------------------------------------------
| Name: kernel_net_core_set_specific_info
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
void kernel_net_core_set_specific_info(desc_t desc, void* p_kernel_net_core_socket_specific_info) {
   kernel_net_core_socket_list[desc].p_kernel_net_core_socket_specific_info = p_kernel_net_core_socket_specific_info;
   ofile_lst[desc].p=&kernel_net_core_socket_list[desc];
}

/*--------------------------------------------
| Name: kernel_net_core_get_specific_info
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
void* kernel_net_core_get_specific_info(desc_t desc) {
   return ((kernel_net_core_socket_t*)ofile_lst[desc].p)->p_kernel_net_core_socket_specific_info;
}

/*--------------------------------------------
| Name: kernel_net_core_socket
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int kernel_net_core_socket(desc_t desc, int domain, int type, int protocol) {
   int error;
   const kernel_net_core_socket_op_t* p_kernel_net_core_socket_op;
   //
   if (desc < 0) {
      return -1;
   }
   //
   if(!ofile_lst[desc].p){
      return -1;
   }
   //
   p_kernel_net_core_socket_op=((kernel_net_core_socket_t*)ofile_lst[desc].p)->p_kernel_net_core_socket_op;
   //
   if(!p_kernel_net_core_socket_op){
      return -1;
   }
   //
   if (!p_kernel_net_core_socket_op->kernel_net_core_socket) {
      return -1;
   }
   //
   if ((error=p_kernel_net_core_socket_op->kernel_net_core_socket(desc, domain, type, protocol)) < 0) {
      return error;
   }
   //
   return error;
}

/*--------------------------------------------
| Name: kernel_net_core_bind
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int kernel_net_core_bind(desc_t desc, struct sockaddr *name, socklen_t namelen) {
   int error;
   const kernel_net_core_socket_op_t* p_kernel_net_core_socket_op;
   //
   if (desc < 0) {
      return -1;
   }
   //
   if(!ofile_lst[desc].p){
      return -1;
   }
   //
   p_kernel_net_core_socket_op=((kernel_net_core_socket_t*)ofile_lst[desc].p)->p_kernel_net_core_socket_op;
   //
   if(!p_kernel_net_core_socket_op){
      return -1;
   }
   //
   if (!p_kernel_net_core_socket_op->kernel_net_core_bind) {
      return -1;
   }
   //
   if ((error=p_kernel_net_core_socket_op->kernel_net_core_bind(desc,name,namelen)) < 0) {
      return -1;
   }
   //
   return error;
}

/*--------------------------------------------
| Name: kernel_net_core_accept
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int kernel_net_core_accept(desc_t desc, struct sockaddr *addr, socklen_t *addrlen) {
   int native_socket_fd;
   const kernel_net_core_socket_op_t* p_kernel_net_core_socket_op;
   //
   if (desc < 0) {
      return -1;
   }
   //
   if(!ofile_lst[desc].p){
      return -1;
   }
   //
   p_kernel_net_core_socket_op=((kernel_net_core_socket_t*)ofile_lst[desc].p)->p_kernel_net_core_socket_op;
   //
   if(!p_kernel_net_core_socket_op){
      return -1;
   }
   //
   if (!p_kernel_net_core_socket_op->kernel_net_core_accept) {
      return -1;
   }
   //
   if ((native_socket_fd = p_kernel_net_core_socket_op->kernel_net_core_accept(desc, addr, addrlen)) < 0) {
      return -1;
   }
   //
   return native_socket_fd;
}

/*--------------------------------------------
| Name: kernel_net_core_accepted
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int kernel_net_core_accepted(desc_t desc, int native_socket_fd) {
   int error;
   const kernel_net_core_socket_op_t* p_kernel_net_core_socket_op;
   //
   if (desc < 0) {
      return -1;
   }
   //
   if(!ofile_lst[desc].p){
      return -1;
   }
   //
   p_kernel_net_core_socket_op=((kernel_net_core_socket_t*)ofile_lst[desc].p)->p_kernel_net_core_socket_op;
   //
   if(!p_kernel_net_core_socket_op){
      return -1;
   }
   //
   if (!p_kernel_net_core_socket_op->kernel_net_core_accepted) {
      return -1;
   }
   //
   if ((error = p_kernel_net_core_socket_op->kernel_net_core_accepted(desc, native_socket_fd)) < 0) {
      return -1;
   }
   //
   return error;
}

/*--------------------------------------------
| Name: kernel_net_core_connect
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int kernel_net_core_connect(desc_t desc, struct sockaddr *name, socklen_t namelen) {
   int error;
   const kernel_net_core_socket_op_t* p_kernel_net_core_socket_op;
   //
   if (desc < 0) {
      return -1;
   }
   //
   if(!ofile_lst[desc].p){
      return -1;
   }
   //
   p_kernel_net_core_socket_op=((kernel_net_core_socket_t*)ofile_lst[desc].p)->p_kernel_net_core_socket_op;
   //
   if(!p_kernel_net_core_socket_op){
      return -1;
   }
   //
   if (!p_kernel_net_core_socket_op->kernel_net_core_connect) {
      return -1;
   }
   //
   if ((error = p_kernel_net_core_socket_op->kernel_net_core_connect(desc,name,namelen)) < 0) {
      return error;
   }
   //
   return error;
}

/*--------------------------------------------
| Name: kernel_net_core_listen
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int kernel_net_core_listen(desc_t desc, int backlog) {
   int error;
   const kernel_net_core_socket_op_t* p_kernel_net_core_socket_op;
   //
   if (desc < 0) {
      return -1;
   }
   //
   if(!ofile_lst[desc].p){
      return -1;
   }
   //
   p_kernel_net_core_socket_op=((kernel_net_core_socket_t*)ofile_lst[desc].p)->p_kernel_net_core_socket_op;
   //
   if(!p_kernel_net_core_socket_op){
      return -1;
   }
   //
   if (!p_kernel_net_core_socket_op->kernel_net_core_listen) {
      return -1;
   }
   //
   if ((error = p_kernel_net_core_socket_op->kernel_net_core_listen(desc,backlog)) < 0) {
      return error;
   }
   //
   return error;
}

/*--------------------------------------------
| Name: kernel_net_core_recv
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int kernel_net_core_recv(desc_t desc, void *mem, int len, unsigned int flags) {
   return 0;
}

/*--------------------------------------------
| Name: kernel_net_core_recvfrom
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int kernel_net_core_recvfrom(desc_t desc, void *mem, int len, unsigned int flags, struct sockaddr *from, socklen_t *fromlen) {
   return 0;
}

/*--------------------------------------------
| Name: kernel_net_core_send
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int kernel_net_core_send(desc_t desc, const void *dataptr, int size, unsigned int flags) {
   return 0;
}

/*--------------------------------------------
| Name: kernel_net_core_sendto
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int kernel_net_core_sendto(desc_t desc, void *dataptr, int size, unsigned int flags, struct sockaddr *to, socklen_t tolen) {
   return 0;
}

/*--------------------------------------------
| Name: kernel_net_core_shutdown
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int kernel_net_core_shutdown(desc_t desc, int how) {
   int error;
   const kernel_net_core_socket_op_t* p_kernel_net_core_socket_op;
   //
   if (desc < 0) {
      return -1;
   }
   //
   if(!ofile_lst[desc].p){
      return -1;
   }
   //
   p_kernel_net_core_socket_op=((kernel_net_core_socket_t*)ofile_lst[desc].p)->p_kernel_net_core_socket_op;
   //
   if(!p_kernel_net_core_socket_op){
      return -1;
   }
   //
   if (!p_kernel_net_core_socket_op->kernel_net_core_shutdown) {
      return -1;
   }
   //
   if ((error = p_kernel_net_core_socket_op->kernel_net_core_shutdown(desc, how)) < 0) {
      return error;
   }
   //
   return error;
}

/*--------------------------------------------
| Name: kernel_net_core_shutdown
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int kernel_net_core_close(desc_t desc) {
   int error;
   const kernel_net_core_socket_op_t* p_kernel_net_core_socket_op;
   //
   if (desc < 0) {
      return -1;
   }
   //
   if(!ofile_lst[desc].p){
      return -1;
   }
   //
   p_kernel_net_core_socket_op=((kernel_net_core_socket_t*)ofile_lst[desc].p)->p_kernel_net_core_socket_op;
   //
   if(!p_kernel_net_core_socket_op){
      return -1;
   }
   //
   if (!p_kernel_net_core_socket_op->kernel_net_core_close) {
      return -1;
   }
   //
   if ((error = p_kernel_net_core_socket_op->kernel_net_core_close(desc)) < 0) {
      return error;
   }
   //
   return error;
}

/*--------------------------------------------
| Name: kernel_net_core_getpeername
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int kernel_net_core_getpeername(desc_t desc, struct sockaddr *name, socklen_t *namelen) {
   int error;
   const kernel_net_core_socket_op_t* p_kernel_net_core_socket_op;
   //
   if (desc < 0) {
      return -1;
   }
   //
   if(!ofile_lst[desc].p){
      return -1;
   }
   //
   p_kernel_net_core_socket_op=((kernel_net_core_socket_t*)ofile_lst[desc].p)->p_kernel_net_core_socket_op;
   //
   if(!p_kernel_net_core_socket_op){
      return -1;
   }
   //
   if (!p_kernel_net_core_socket_op->kernel_net_core_getpeername) {
      return -1;
   }
   //
   if ((error = p_kernel_net_core_socket_op->kernel_net_core_getpeername(desc,name,namelen)) < 0) {
      return error;
   }
   //
   return error;
}

/*--------------------------------------------
| Name: kernel_net_core_getsockname
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int kernel_net_core_getsockname(desc_t desc, struct sockaddr *name, socklen_t *namelen) {
   int error;
   const kernel_net_core_socket_op_t* p_kernel_net_core_socket_op;
   //
   if (desc < 0) {
      return -1;
   }
   //
   if(!ofile_lst[desc].p){
      return -1;
   }
   //
   p_kernel_net_core_socket_op=((kernel_net_core_socket_t*)ofile_lst[desc].p)->p_kernel_net_core_socket_op;
   //
   if(!p_kernel_net_core_socket_op){
      return -1;
   }
   //
   if (!p_kernel_net_core_socket_op->kernel_net_core_getsockname) {
      return -1;
   }
   //
   if ((error = p_kernel_net_core_socket_op->kernel_net_core_getsockname(desc, name, namelen)) < 0) {
      return error;
   }
   //
   return error;
}

/*--------------------------------------------
| Name: kernel_net_core_getsockopt
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int kernel_net_core_getsockopt(desc_t desc, int level, int optname, void *optval, socklen_t *optlen) {
   int error;
   const kernel_net_core_socket_op_t* p_kernel_net_core_socket_op;
   //
   if (desc < 0) {
      return -1;
   }
   //
   if(!ofile_lst[desc].p){
      return -1;
   }
   //
   p_kernel_net_core_socket_op=((kernel_net_core_socket_t*)ofile_lst[desc].p)->p_kernel_net_core_socket_op;
   //
   if(!p_kernel_net_core_socket_op){
      return -1;
   }
   //
   if (!p_kernel_net_core_socket_op->kernel_net_core_getsockopt) {
      return -1;
   }
   //
   if ((error = p_kernel_net_core_socket_op->kernel_net_core_getsockopt(desc, level,optname,optval,optlen)) < 0) {
      return error;
   }
   //
   return error;
}

/*--------------------------------------------
| Name: kernel_net_core_setsockopt
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int kernel_net_core_setsockopt(desc_t desc, int level, int optname, const void *optval, socklen_t optlen) {
   int error;
   const kernel_net_core_socket_op_t* p_kernel_net_core_socket_op;
   //
   if (desc < 0) {
      return -1;
   }
   //
   if(!ofile_lst[desc].p){
      return -1;
   }
   //
   p_kernel_net_core_socket_op=((kernel_net_core_socket_t*)ofile_lst[desc].p)->p_kernel_net_core_socket_op;
   //
   if(!p_kernel_net_core_socket_op){
      return -1;
   }
   //
   if (!p_kernel_net_core_socket_op->kernel_net_core_setsockopt) {
      return -1;
   }
   //
   if ((error = p_kernel_net_core_socket_op->kernel_net_core_setsockopt(desc, level, optname, optval, optlen)) < 0) {
      return error;
   }
   //
   return error;
}

/*--------------------------------------------
| Name: kernel_net_core_gethostbyname
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
struct hostent* kernel_net_core_gethostbyname(desc_t desc,struct hostent* host,const char *name) {
   const kernel_net_core_socket_op_t* p_kernel_net_core_socket_op;
   //
   if (desc < 0) {
      return (struct hostent*)0;
   }
   //
   if (!ofile_lst[desc].p) {
      return (struct hostent*)0;
   }
   //
   p_kernel_net_core_socket_op = ((kernel_net_core_socket_t*)ofile_lst[desc].p)->p_kernel_net_core_socket_op;
   //
   if (!p_kernel_net_core_socket_op) {
      return (struct hostent*)0;
   }
   //
   if (!p_kernel_net_core_socket_op->kernel_net_core_gethostbyname) {
      return (struct hostent*)0;
   }
   //
   return p_kernel_net_core_socket_op->kernel_net_core_gethostbyname(desc, host, name);
}

/*============================================
| End of Source  : kernel_net_core_socket.c
==============================================*/