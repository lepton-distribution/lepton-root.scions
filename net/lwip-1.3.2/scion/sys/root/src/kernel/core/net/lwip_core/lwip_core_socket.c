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
#include <stdarg.h>

#include "kernel/core/kernelconf.h"
#include "kernel/core/limits.h"
#include "kernel/core/dirent.h"
//#include "kernel/core/errno.h"
#include "kernel/core/system.h"
#include "kernel/core/process.h"
#include "kernel/core/kernel.h"
#include "kernel/core/fcntl.h"
#include "kernel/core/stat.h"

#include "kernel/fs/vfs/vfstypes.h"
#include "kernel/fs/vfs/vfskernel.h"
#include "kernel/fs/vfs/vfs.h"

#include "kernel/net/lwip/include/lwip/api.h"
#include "kernel/net/lwip/include/lwip/sockets.h"
#include "kernel/net/lwip/include/lwip/netdb.h"
#include "kernel/core/net/kernel_net_core_socket.h"


/*============================================
| Global Declaration
==============================================*/


//
static int dev_lwip_core_socket_load(void);
static int dev_lwip_core_socket_open(desc_t desc, int o_flag);
static int dev_lwip_core_socket_close(desc_t desc);
static int dev_lwip_core_socket_isset_read(desc_t desc);
static int dev_lwip_core_socket_isset_write(desc_t desc);
static int dev_lwip_core_socket_read(desc_t desc, char* buffer, int nbyte);
static int dev_lwip_core_socket_write(desc_t desc, const char* buffer, int nbyte);
static int dev_lwip_core_socket_read_args(desc_t desc, char* buffer, int len, va_list ap);
static int dev_lwip_core_socket_write_args(desc_t desc, const char* buffer, int len, va_list ap);
static int dev_lwip_core_socket_seek(desc_t desc, int offset, int origin);
static int dev_lwip_core_socket_ioctl(desc_t desc, int request, va_list ap);

static const char dev_lwip_core_socket_name[] = "net/socket";

 dev_map_t  dev_lwip_core_socket_map = {
   .dev_name = dev_lwip_core_socket_name,
   .dev_attr = S_IFCHR,
   .fdev_load = dev_lwip_core_socket_load,
   .fdev_open = dev_lwip_core_socket_open,
   .fdev_close = dev_lwip_core_socket_close,
   .fdev_isset_read = dev_lwip_core_socket_isset_read,
   .fdev_isset_write = dev_lwip_core_socket_isset_write,
   .fdev_read = dev_lwip_core_socket_read,
   .fdev_write = dev_lwip_core_socket_write,
   .fdev_read_args = dev_lwip_core_socket_read_args,
   .fdev_write_args = dev_lwip_core_socket_write_args,
   .fdev_seek = dev_lwip_core_socket_seek,
   .fdev_ioctl = dev_lwip_core_socket_ioctl
};


//
int lwip_core_socket_socket(desc_t desc, int domain, int type, int protocol);
int lwip_core_socket_bind(desc_t desc, struct sockaddr *name, socklen_t namelen);
int lwip_core_socket_accept(desc_t desc, struct sockaddr *addr, socklen_t *addrlen);
int lwip_core_socket_accepted(desc_t desc, int native_socket_fd);
int lwip_core_socket_connect(desc_t desc, struct sockaddr *name, socklen_t namelen);
int lwip_core_socket_listen(desc_t desc, int backlog);
int lwip_core_socket_shutdown(desc_t desc, int how);
int lwip_core_socket_close(desc_t desc);
//
int lwip_core_socket_getpeername(desc_t desc, struct sockaddr *name, socklen_t *namelen);
int lwip_core_socket_getsockname(desc_t desc, struct sockaddr *name, socklen_t *namelen);
int lwip_core_socket_getsockopt(desc_t desc, int level, int optname, void *optval, socklen_t *optlen);
int lwip_core_socket_setsockopt(desc_t desc, int level, int optname, const void *optval, socklen_t optlen);
struct hostent* lwip_core_socket_gethostbyname(desc_t desc, struct hostent* host, const char *name);

static  const kernel_net_core_socket_op_t kernel_net_core_socket_op = {
   .kernel_net_core_socket = lwip_core_socket_socket,
   .kernel_net_core_bind = lwip_core_socket_bind,
   .kernel_net_core_accept = lwip_core_socket_accept,
   .kernel_net_core_accepted = lwip_core_socket_accepted,
   .kernel_net_core_connect = lwip_core_socket_connect,
   .kernel_net_core_listen = lwip_core_socket_listen,
   .kernel_net_core_shutdown = lwip_core_socket_shutdown,
   .kernel_net_core_close = lwip_core_socket_close,
   //
   .kernel_net_core_getpeername = lwip_core_socket_getpeername,
   .kernel_net_core_getsockname = lwip_core_socket_getsockname,
   .kernel_net_core_getsockopt = lwip_core_socket_getsockopt,
   .kernel_net_core_setsockopt = lwip_core_socket_setsockopt,
   .kernel_net_core_gethostbyname = lwip_core_socket_gethostbyname
};

//
typedef struct lwip_core_socket_info_st {
   int native_lwip_socket_fd;
   desc_t desc;
}lwip_core_socket_info_t;
//
static lwip_core_socket_info_t lwip_core_socket_info_list[MAX_OPEN_FILE];

/*============================================
| Implementation
==============================================*/
/*--------------------------------------------
| Name: lwip_core_socket_event_callback
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static void lwip_core_socket_event_callback(struct netconn *conn, enum netconn_evt evt, u16_t len){
   int s;
   struct lwip_sock *sock;
   
   SYS_ARCH_DECL_PROTECT(lev);

   LWIP_UNUSED_ARG(len);

   /* Get socket */
   if (conn) {
      s = conn->socket;
      if (s < 0) {
         /* Data comes in right away after an accept, even though
         * the server task might not have created a new socket yet.
         * Just count down (or up) if that's the case and we
         * will use the data later. Note that only receive events
         * can happen before the new socket is set up. */
         SYS_ARCH_PROTECT(lev);
         if (conn->socket < 0) {
            if (evt == NETCONN_EVT_RCVPLUS) {
               conn->socket--;
            }
            SYS_ARCH_UNPROTECT(lev);
            return;
         }
         s = conn->socket;
         SYS_ARCH_UNPROTECT(lev);
      }

      sock = get_socket(s);
      if (!sock) {
         return;
      }
   }
   else {
      return;
   }

   SYS_ARCH_PROTECT(lev);
   //
   desc_t desc = INVALID_DESC;
   lwip_core_socket_info_t* p_lwip_core_socket_info = (lwip_core_socket_info_t*) sock->p_lwip_core_socket_info;
   //
   if (p_lwip_core_socket_info) {
      desc = p_lwip_core_socket_info->desc;
   }

   /* Set event as required */
   switch (evt) {
   case NETCONN_EVT_RCVPLUS:
      sock->rcvevent++;
      if(desc!=INVALID_DESC)
         __fire_io(ofile_lst[desc].owner_pthread_ptr_read);
      break;
   case NETCONN_EVT_RCVMINUS:
      sock->rcvevent--;
      break;
   case NETCONN_EVT_SENDPLUS:
      sock->sendevent = 1;
      if (desc != INVALID_DESC)
         __fire_io(ofile_lst[desc].owner_pthread_ptr_write);
      break;
   case NETCONN_EVT_SENDMINUS:
      sock->sendevent = 0;
      break;
   case NETCONN_EVT_ERROR:
      sock->errevent = 1;
      break;
   default:
      LWIP_ASSERT("unknown event", 0);
      break;
   }

   if (sock->select_waiting == 0) {
      /* noone is waiting for this socket, no need to check select_cb_list */
      SYS_ARCH_UNPROTECT(lev);
      return;
   }
   //
   SYS_ARCH_UNPROTECT(lev);
   //
}

/*--------------------------------------------
| Name: lwip_core_socket_socket
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int lwip_core_socket_socket(desc_t desc, int domain, int type, int protocol) {

   lwip_core_socket_info_t* p_lwip_core_socket_info = kernel_net_core_get_specific_info(desc);

   if (p_lwip_core_socket_info == (lwip_core_socket_info_t*)0)
      return -1;

   if ((p_lwip_core_socket_info->native_lwip_socket_fd = lwip_socket(domain, type, protocol)) < 0) {
      return -1;
   }
   //
   p_lwip_core_socket_info->desc = desc;
   //
   sockets[p_lwip_core_socket_info->native_lwip_socket_fd].conn->callback = lwip_core_socket_event_callback;
   //
   sockets[p_lwip_core_socket_info->native_lwip_socket_fd].p_lwip_core_socket_info = p_lwip_core_socket_info;
   //
   return 0;
}

/*--------------------------------------------
| Name: lwip_core_socket_bind
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int lwip_core_socket_bind(desc_t desc, struct sockaddr *name, socklen_t namelen) {
   return lwip_bind(lwip_core_socket_info_list[desc].native_lwip_socket_fd, name, namelen);
}

/*--------------------------------------------
| Name: lwip_core_socket_accept
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int lwip_core_socket_accept(desc_t desc, struct sockaddr *addr, socklen_t *addrlen) {
   int native_lwip_socket_accepted_fd;
   native_lwip_socket_accepted_fd = lwip_accept(lwip_core_socket_info_list[desc].native_lwip_socket_fd, addr, addrlen);
   //
   return native_lwip_socket_accepted_fd;
}

/*--------------------------------------------
| Name: lwip_core_socket_accepted
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int lwip_core_socket_accepted(desc_t desc, int native_socket_fd) {
   //
   lwip_core_socket_info_list[desc].native_lwip_socket_fd = native_socket_fd;
   lwip_core_socket_info_list[desc].desc = desc;
   //
   sockets[lwip_core_socket_info_list[desc].native_lwip_socket_fd].p_lwip_core_socket_info = &lwip_core_socket_info_list[desc];
   //
   kernel_net_core_set_specific_info(desc, &lwip_core_socket_info_list[desc]);
   
   // note:  event callback of the new connection is set in accept_function() (see api_msg.c) with event callback from listen connection. 
   // (already set in lwip_core_socket()) .

   //
   return 0;
}

/*--------------------------------------------
| Name: lwip_core_socket_connect
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int lwip_core_socket_connect(desc_t desc, struct sockaddr *name, socklen_t namelen) {
   return lwip_connect(lwip_core_socket_info_list[desc].native_lwip_socket_fd, name, namelen);
}

/*--------------------------------------------
| Name: lwip_core_socket_listen
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int lwip_core_socket_listen(desc_t desc, int backlog) {
   return lwip_listen(lwip_core_socket_info_list[desc].native_lwip_socket_fd,backlog);
}

/*--------------------------------------------
| Name: lwip_core_socket_shutdown
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int lwip_core_socket_shutdown(desc_t desc, int how) {
   return lwip_shutdown(lwip_core_socket_info_list[desc].native_lwip_socket_fd,how);
}

/*--------------------------------------------
| Name: lwip_core_socket_close
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int lwip_core_socket_close(desc_t desc) {
   return lwip_close(lwip_core_socket_info_list[desc].native_lwip_socket_fd);
}

/*--------------------------------------------
| Name: lwip_core_socket_getpeername
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int lwip_core_socket_getpeername(desc_t desc, struct sockaddr *name, socklen_t *namelen) {
   return lwip_getpeername(lwip_core_socket_info_list[desc].native_lwip_socket_fd, name, namelen);
}

/*--------------------------------------------
| Name: lwip_core_socket_getsockname
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int lwip_core_socket_getsockname(desc_t desc, struct sockaddr *name, socklen_t *namelen) {
   return lwip_getsockname(lwip_core_socket_info_list[desc].native_lwip_socket_fd, name, namelen);
}

/*--------------------------------------------
| Name: lwip_core_socket_getsockopt
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int lwip_core_socket_getsockopt(desc_t desc, int level, int optname, void *optval, socklen_t *optlen) {
   return lwip_getsockopt(lwip_core_socket_info_list[desc].native_lwip_socket_fd, level, optname, optval, optlen);
}

/*--------------------------------------------
| Name: lwip_core_socket_setsockopt
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int lwip_core_socket_setsockopt(desc_t desc, int level, int optname, const void *optval, socklen_t optlen) {
   return lwip_setsockopt(lwip_core_socket_info_list[desc].native_lwip_socket_fd, level, optname, optval, optlen);
}

/*--------------------------------------------
| Name: lwip_core_socket_ioctl
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int lwip_core_socket_ioctl(desc_t desc, long cmd, void *argp) {
   return lwip_ioctl(lwip_core_socket_info_list[desc].native_lwip_socket_fd, cmd,argp);
}

/*--------------------------------------------
| Name: lwip_core_socket_gethostbyname
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
struct hostent* lwip_core_socket_gethostbyname(desc_t desc,struct hostent* host,const char *name) {
   return lwip_gethostbyname(name);
}

/*-------------------------------------------
| Name:dev_lwip_core_socket_load
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_lwip_core_socket_load(void) {
   return 0;
}

/*-------------------------------------------
| Name:dev_lwip_core_socket_open
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_lwip_core_socket_open(desc_t desc, int o_flag) {
   //
   kernel_net_core_set_socket_op(desc,&kernel_net_core_socket_op);
   kernel_net_core_set_specific_info(desc, &lwip_core_socket_info_list[desc]);
   //

   return 0;
}

/*-------------------------------------------
| Name:dev_lwip_core_socket_close
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_lwip_core_socket_close(desc_t desc) {
   //hsock_t hsock = ofile_lst[desc].ext.hsock;
   if ((ofile_lst[desc].oflag & O_RDONLY)
      && ofile_lst[desc].oflag & O_WRONLY) {

      if (!ofile_lst[desc].nb_reader && !ofile_lst[desc].nb_writer) {
         lwip_core_socket_shutdown(desc, SHUT_RDWR);
         lwip_core_socket_close(desc);
         //check id pthread owner has been set to null (event call back) 
      }

   }

   return 0;
}

/*-------------------------------------------
| Name:dev_lwip_core_socket_isset_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_lwip_core_socket_isset_read(desc_t desc) {
   lwip_core_socket_info_t* p_lwip_core_socket_info = kernel_net_core_get_specific_info(desc);
   if (!p_lwip_core_socket_info) {
      return -1;
   }
   if (p_lwip_core_socket_info->native_lwip_socket_fd < 0)
      return -1;
   //
   if (sockets[p_lwip_core_socket_info->native_lwip_socket_fd].rcvevent > 0) {
      return 0;
   }
   //
   return -1;
}

/*-------------------------------------------
| Name:dev_lwip_core_socket_isset_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_lwip_core_socket_isset_write(desc_t desc) {
   lwip_core_socket_info_t* p_lwip_core_socket_info = kernel_net_core_get_specific_info(desc);
   if (!p_lwip_core_socket_info) {
      return -1;
   }
   if (p_lwip_core_socket_info->native_lwip_socket_fd < 0)
      return -1;
   //
   if (sockets[p_lwip_core_socket_info->native_lwip_socket_fd].sendevent > 0) {
      return 0;
   }
   //
   return -1;
}

/*-------------------------------------------
| Name:dev_lwip_core_socket_read
| Description:
| Parameters:
| Return Type:
| Comments:    lwip 2.0
| See:
---------------------------------------------*/
static int dev_lwip_core_socket_read(desc_t desc, char* buffer, int len) {
   lwip_core_socket_info_t* p_lwip_core_socket_info = kernel_net_core_get_specific_info(desc);
   if (!p_lwip_core_socket_info) {
      return -1;
   }
   if (p_lwip_core_socket_info->native_lwip_socket_fd < 0) {
      return -1;
   }
   //
   return lwip_recvfrom(p_lwip_core_socket_info->native_lwip_socket_fd, buffer, len, 0, NULL, NULL);
}

/*-------------------------------------------
| Name:dev_lwip_core_socket_write
| Description:
| Parameters:
| Return Type:
| Comments:    lwip 2.0
| See:
---------------------------------------------*/
static int dev_lwip_core_socket_write(desc_t desc, const char *buffer, int size) {
   lwip_core_socket_info_t* p_lwip_core_socket_info = kernel_net_core_get_specific_info(desc);
   if (!p_lwip_core_socket_info) {
      return -1;
   }
   if (p_lwip_core_socket_info->native_lwip_socket_fd < 0) {
      return -1;
   }
   //
   return lwip_send(p_lwip_core_socket_info->native_lwip_socket_fd, buffer, size, 0);
}

/*-------------------------------------------
| Name:dev_lwip_core_socket_read_args
| Description:
| Parameters:
| Return Type:
| Comments:    lwip 2.0
| See:
---------------------------------------------*/
static int dev_lwip_core_socket_read_args(desc_t desc, char* buffer, int len, va_list ap) {
   lwip_core_socket_info_t* p_lwip_core_socket_info = kernel_net_core_get_specific_info(desc);
   if (!p_lwip_core_socket_info) {
      return -1;
   }
   if (p_lwip_core_socket_info->native_lwip_socket_fd < 0) {
      return -1;
   }
   //
   unsigned int flags = va_arg(ap, unsigned int);
   struct sockaddr *from = va_arg(ap, struct sockaddr*);
   socklen_t *fromlen = va_arg(ap, socklen_t *);
   //
   return lwip_recvfrom(p_lwip_core_socket_info->native_lwip_socket_fd, buffer, len, flags, from, fromlen);
}

/*-------------------------------------------
| Name:dev_lwip_core_socket_write_args
| Description:
| Parameters:
| Return Type:
| Comments:    lwip 2.0
| See:
---------------------------------------------*/
static int dev_lwip_core_socket_write_args(desc_t desc, const char *buffer, int size, va_list ap) {
   lwip_core_socket_info_t* p_lwip_core_socket_info = kernel_net_core_get_specific_info(desc);
   if (!p_lwip_core_socket_info) {
      return -1;
   }
   if (p_lwip_core_socket_info->native_lwip_socket_fd < 0) {
      return -1;
   }
   //
   unsigned int flags = va_arg(ap, unsigned int);
   struct sockaddr *to = va_arg(ap, struct sockaddr*);
   socklen_t tolen = va_arg(ap, socklen_t);
   //
   if (to) {
      return  lwip_sendto(p_lwip_core_socket_info->native_lwip_socket_fd, buffer, size, flags, to, tolen);
   }
   //
   return lwip_send(p_lwip_core_socket_info->native_lwip_socket_fd, buffer, size, flags);
}

/*-------------------------------------------
| Name:dev_lwip_core_socket_seek
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_lwip_core_socket_seek(desc_t desc, int offset, int origin) {
   return -1;
}

/*--------------------------------------------
| Name:dev_lwip_core_socket_ioctl
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static  int dev_lwip_core_socket_ioctl(desc_t desc, int request, va_list ap) {
   
   lwip_core_socket_info_t* p_lwip_core_socket_info = kernel_net_core_get_specific_info(desc);
   if (!p_lwip_core_socket_info) {
      return -1;
   }
   if (p_lwip_core_socket_info->native_lwip_socket_fd < 0) {
      return -1;
   }
   //
   void *argp = va_arg(ap, void*);
   //
   return lwip_ioctl(p_lwip_core_socket_info->native_lwip_socket_fd,request, argp);
}

/*============================================
| End of Source  : lwip_core_socket.c
==============================================*/
