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
#include <stdlib.h>

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

#if USE_UIP_VER == 1000 
#pragma message ("uip 1.0")
#include "kernel/net/uip1.0/net/uip.h"
#include "kernel/net/uip1.0/net/uip_arch.h"
#endif

#if USE_UIP_VER == 2500 || USE_UIP_VER == 3000
#if USE_UIP_VER == 2500
#pragma message ("uip 2.5")
#include "kernel/net/uip2.5/contiki-conf.h"
#include "kernel/net/uip2.5/net/uip.h"
#include "kernel/net/uip2.5/net/uip_arch.h"
#endif

#if USE_UIP_VER == 3000
#pragma message ("uip 3.0")
#include "kernel/net/uip/core/contiki-conf.h"
#include "kernel/net/uip/core/net/ip/uip.h"
#include "kernel/net/uip/core/net/ip/uip_arch.h"
#endif
#endif

#include "kernel/core/net/uip_core/uip_sock.h"
#include "kernel/core/net/uip_core/uip_socket.h"
#include "kernel/core/net/uip_core/uip_core.h"

#if UIP_LOGGING!=1
#include "lib/libc/stdio/stdio.h"
#else
#pragma message ("warning uip 1.0 logging")
#endif

#define __set_kernel_pthread_errno(__errno__) (__kernel_pthread_errno=__errno__)


#include "kernel/core/net/kernel_net_core_socket.h"


/*============================================
| Global Declaration
==============================================*/


//
static int dev_uip_core_socket_load(void);
static int dev_uip_core_socket_open(desc_t desc, int o_flag);
static int dev_uip_core_socket_close(desc_t desc);
static int dev_uip_core_socket_isset_read(desc_t desc);
static int dev_uip_core_socket_isset_write(desc_t desc);
static int dev_uip_core_socket_read(desc_t desc, char* buffer, int nbyte);
static int dev_uip_core_socket_write(desc_t desc, const char* buffer, int nbyte);
static int dev_uip_core_socket_read_args(desc_t desc, char* buffer, int len, va_list ap);
static int dev_uip_core_socket_write_args(desc_t desc, const char* buffer, int len, va_list ap);
static int dev_uip_core_socket_seek(desc_t desc, int offset, int origin);
static int dev_uip_core_socket_ioctl(desc_t desc, int request, va_list ap);

static const char dev_uip_core_socket_name[] = "net/socket";

 dev_map_t  dev_uip_core_socket_map = {
   .dev_name = dev_uip_core_socket_name,
   .dev_attr = S_IFCHR,
   .fdev_load = dev_uip_core_socket_load,
   .fdev_open = dev_uip_core_socket_open,
   .fdev_close = dev_uip_core_socket_close,
   .fdev_isset_read = dev_uip_core_socket_isset_read,
   .fdev_isset_write = dev_uip_core_socket_isset_write,
   .fdev_read = dev_uip_core_socket_read,
   .fdev_write = dev_uip_core_socket_write,
   .fdev_read_args = dev_uip_core_socket_read_args,
   .fdev_write_args = dev_uip_core_socket_write_args,
   .fdev_seek = dev_uip_core_socket_seek,
   .fdev_ioctl = dev_uip_core_socket_ioctl
};


//
int uip_core_socket_socket(desc_t desc, int domain, int type, int protocol);
int uip_core_socket_bind(desc_t desc, struct sockaddr *name, socklen_t namelen);
int uip_core_socket_accept(desc_t desc, struct sockaddr *addr, socklen_t *addrlen);
int uip_core_socket_accepted(desc_t desc, int native_socket_fd);
int uip_core_socket_connect(desc_t desc, struct sockaddr *name, socklen_t namelen);
int uip_core_socket_listen(desc_t desc, int backlog);
int uip_core_socket_shutdown(desc_t desc, int how);
int uip_core_socket_close(desc_t desc);
//
int uip_core_socket_getpeername(desc_t desc, struct sockaddr *name, socklen_t *namelen);
int uip_core_socket_getsockname(desc_t desc, struct sockaddr *name, socklen_t *namelen);
int uip_core_socket_getsockopt(desc_t desc, int level, int optname, void *optval, socklen_t *optlen);
int uip_core_socket_setsockopt(desc_t desc, int level, int optname, const void *optval, socklen_t optlen);
struct hostent* uip_core_socket_gethostbyname(desc_t desc, struct hostent* host, const char *name);

static  const kernel_net_core_socket_op_t kernel_net_core_socket_op = {
   .kernel_net_core_socket = uip_core_socket_socket,
   .kernel_net_core_bind = uip_core_socket_bind,
   .kernel_net_core_accept = uip_core_socket_accept,
   .kernel_net_core_accepted = uip_core_socket_accepted,
   .kernel_net_core_connect = uip_core_socket_connect,
   .kernel_net_core_listen = uip_core_socket_listen,
   .kernel_net_core_shutdown = uip_core_socket_shutdown,
   .kernel_net_core_close = uip_core_socket_close,
   //
   .kernel_net_core_getpeername = uip_core_socket_getpeername,
   .kernel_net_core_getsockname = uip_core_socket_getsockname,
   .kernel_net_core_getsockopt = uip_core_socket_getsockopt,
   .kernel_net_core_setsockopt = uip_core_socket_setsockopt,
   .kernel_net_core_gethostbyname = uip_core_socket_gethostbyname
};

//
typedef struct uip_core_socket_info_st {
   int native_uip_socket_fd;
   desc_t desc;
}uip_core_socket_info_t;
//
static uip_core_socket_info_t uip_core_socket_info_list[MAX_OPEN_FILE];

//
extern desc_t uip_core_if_indextodesc(int ifindex, unsigned long oflag);

const struct in6_addr in6addr_any = {
   { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 }
};

//
#define __set_kernel_pthread_errno(__errno__) (__kernel_pthread_errno=__errno__)

/*============================================
| Implementation
==============================================*/

/**
* \brief Convert IPv4 address from dotted string to unsigned long. Standard BSD inet_addr() with internal name.
* \note IPv4 function only.
* \param[in] cp dotted string IPv4 address.
* \return returns the IPv4 address is stored in network byte order. On failure, return (unsigned long)-1.
* \details Example
\code
struct sockaddr_in addr;
addr.sin_addr.s_addr = inet_addr("192.168.0.10");
\endcode
* \callgraph
*/
unsigned long _uip_inet_addr(char *cp) {

   unsigned long laddr = 0;

   uip_ipaddr_t* paddr = (uip_ipaddr_t*)&laddr;

   char saddr[14] = { 0 };
   unsigned char baddr[4] = { 0 };
   char* token;
   int i = -1;

   if (!cp) return 0;

   strcpy(saddr, cp);

   token = strtok(saddr, ".");

   while (token && i++<4) {
      baddr[i] = atoi(token);
      token = strtok(NULL, ".");
   }

   uip_ipaddr(paddr, baddr[0], baddr[1], baddr[2], baddr[3]);

   return laddr;
} //end of _uip_inet_addr()

/**
* \brief Convert a IPv4 socket address to the IPv4 "h1.h2.h3.h4\0" format. Standard BSD inet_ntoa() with internal name.
* \note IPv4 function only.
* \param[out] cp string buffer to copy the resulting convertion.
* \param[in] in address structure containing the IPv4 address.
* \return \p cp, string with resulting convertion.
* \callgraph
*/
char* _uip_inet_ntoa(char*cp, struct _in_addr in) {

   sprintf(cp, "%d.%d.%d.%d", in.S_un.S_un_b.s_b1,
      in.S_un.S_un_b.s_b2,
      in.S_un.S_un_b.s_b3,
      in.S_un.S_un_b.s_b4);

   return cp;
} //end of _uip_inet_

/*--------------------------------------------
| Name: uip_core_socket_socket
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int uip_core_socket_socket(desc_t desc, int domain, int type, int protocol) {
   kernel_pthread_t* pthread_ptr;
   hsock_t hsock;

   //
   if (!(pthread_ptr = kernel_pthread_self())) {
      __set_kernel_pthread_errno(ESRCH);
      return -1;
   }
   //
   if (desc<0) {
      __set_kernel_pthread_errno(EBADF);
      return -1;
   }

   //
   uip_core_socket_info_t* p_uip_core_socket_info = kernel_net_core_get_specific_info(desc);
   if (p_uip_core_socket_info == (uip_core_socket_info_t*)0) {
      return -1;
   }

   //
   if ((hsock = sock_get(desc)) == (hsock_t)0) {
      return -1;
   }

   //
   if (protocol == IPPROTO_TCP) {
      ((socket_t*)hsock)->protocol = IPPROTO_TCP;
   }
   else if (protocol == IPPROTO_UDP) {
      ((socket_t*)hsock)->protocol = IPPROTO_UDP;
   }
   else if (!protocol && type == SOCK_STREAM) {
      ((socket_t*)hsock)->protocol = IPPROTO_TCP;
   }
   else if (!protocol && type == SOCK_DGRAM) {
      ((socket_t*)hsock)->protocol = IPPROTO_UDP;
   }else {
      //
      sock_put(hsock);
      //
      __set_kernel_pthread_errno(EPROTONOSUPPORT);
      return -1;
   }
  
   //
   p_uip_core_socket_info->native_uip_socket_fd = sock_hsock_to_fd(hsock);
   //
   p_uip_core_socket_info->desc = desc;
   //
   ((socket_t*)hsock)->p_uip_core_socket_info = p_uip_core_socket_info;

   //TCP
   if (((socket_t*)hsock)->protocol == IPPROTO_TCP) {
      return 0;
   }


   //UDP
   ((socket_t*)hsock)->r = 0;
   ((socket_t*)hsock)->w = 0;
   //
   ((socket_t*)hsock)->state = STATE_SOCKET_OPENED;
   // 
   ((socket_t*)hsock)->uip_udp_conn = uip_udp_new((void*)0, 0);
   //
   if (!((socket_t*)hsock)->uip_udp_conn) {
      sock_put(hsock);
      return -1;
   }

   //
   ((socket_t*)hsock)->socksconn = (struct socksconn_state *)(((socket_t*)hsock)->uip_udp_conn->appstate.state);
   //
   ((socket_t*)hsock)->socksconn->__r = 0;
   ((socket_t*)hsock)->socksconn->_r = 0;
   ((socket_t*)hsock)->socksconn->_w = 0;
   //
   ((socket_t*)hsock)->socksconn->hsocks = hsock;
   //
   ((socket_t*)hsock)->state = STATE_SOCKET_WAIT;
   //
   __set_kernel_pthread_errno(0);
   //
   return 0;
}

/*--------------------------------------------
| Name: uip_core_socket_bind
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int uip_core_socket_bind(desc_t desc, struct sockaddr *address, socklen_t namelen) {
   kernel_pthread_t* pthread_ptr;
   hsock_t hsock;
#if UIP_CONF_IPV6
   desc_t desc_if;
#endif
   //
   if (!(pthread_ptr = kernel_pthread_self())) {
      __set_kernel_pthread_errno(ESRCH);
      return -1;
   }
   //
   if (desc<0) {
      __set_kernel_pthread_errno(EBADF);
      return -1;
   }
   //
   uip_core_socket_info_t* p_uip_core_socket_info = kernel_net_core_get_specific_info(desc);
   if (p_uip_core_socket_info == (uip_core_socket_info_t*)0) {
      return -1;
   }
   //
   hsock = sock_fd_to_hsock(p_uip_core_socket_info->native_uip_socket_fd);
   if (hsock == (hsock_t)0) {
      return -1;
   }
   
   //lock
   __lock_io(pthread_ptr, desc, O_WRONLY);
   __lock_io(pthread_ptr, desc, O_RDONLY);
   //
#if UIP_CONF_IPV6
   ((socket_t*)hsock)->addr_in.sin6_port = ((struct _sockaddr_in6*)address)->sin6_port;
   //interface index in list (see uip_core interface list)
   if (((struct _sockaddr_in6*)address)->sin6_scope_id >= IF_LIST_MAX) {
      //unlock
      __set_kernel_pthread_errno(EINVAL);
      //unlock
      __unlock_io(pthread_ptr, desc, O_WRONLY);
      __unlock_io(pthread_ptr, desc, O_RDONLY);
      return -1;
   }
   //
   ((socket_t*)hsock)->addr_in.sin6_scope_id = ((struct _sockaddr_in6*)address)->sin6_scope_id;
   //
   desc_if = uip_core_if_indextodesc(((socket_t*)hsock)->addr_in.sin6_scope_id, O_RDONLY);
   if (desc_if<0) {
      __set_kernel_pthread_errno(EINVAL);
      //unlock
      __unlock_io(pthread_ptr, desc, O_WRONLY);
      __unlock_io(pthread_ptr, desc, O_RDONLY);
      return -1;
   }
   /* to do
   if(dev_core_ioctl(desc_if,NETUP)<0){
   __set_kernel_pthread_errno (ENETDOWN);
   //unlock
    __unlock_io(pthread_ptr, desc, O_WRONLY);
    __unlock_io(pthread_ptr, desc, O_RDONLY);
   return -1;
   }
   */
#else
   ((socket_t*)hsock)->addr_in.sin_port = ((struct _sockaddr_in*)address)->sin_port;
#endif
   //
   if (((socket_t*)hsock)->protocol == IPPROTO_UDP) {
      ((socket_t*)hsock)->uip_udp_conn->rport = 0;//to do: check that
      uip_udp_bind(((socket_t*)hsock)->uip_udp_conn, ((struct _sockaddr_in*)address)->sin_port);
   }
   //
   //unlock
   __unlock_io(pthread_ptr, desc, O_WRONLY);
   __unlock_io(pthread_ptr, desc, O_RDONLY);
   //
   __set_kernel_pthread_errno(0);
   //
   return 0;
}

/*--------------------------------------------
| Name: uip_core_socket_accept
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int uip_core_socket_accept(desc_t desc, struct sockaddr *address, socklen_t *len) {
   kernel_pthread_t* pthread_ptr;
   hsock_t hsock = 0;
   hsock_t hsock_accepted = 0;

   //
   if (!(pthread_ptr = kernel_pthread_self())) {
      __set_kernel_pthread_errno(ESRCH);
      return -1;
   }
   //
   if (desc<0) {
      __set_kernel_pthread_errno(EBADF);
      return -1;
   }
   //
   uip_core_socket_info_t* p_uip_core_socket_info = kernel_net_core_get_specific_info(desc);
   if (p_uip_core_socket_info == (uip_core_socket_info_t*)0) {
      return -1;
   }
   //
   hsock = sock_fd_to_hsock(p_uip_core_socket_info->native_uip_socket_fd);
   if (hsock == (hsock_t)0) {
      return -1;
   }

   if (((socket_t*)(hsock))->state == STATE_SOCKET_CLOSED) {
      __set_kernel_pthread_errno(ENOTCONN);
      return -1;
   }
   if (((socket_t*)(hsock))->state == STATE_SOCKET_ABORTED) {
      __set_kernel_pthread_errno(ECONNABORTED);
      return -1;
   }
   if (((socket_t*)hsock)->protocol != IPPROTO_TCP) {
      __set_kernel_pthread_errno(EPROTONOSUPPORT);
      return -1;
   }

   //set thread owner
   __atomic_in();
   {
      ofile_lst[desc].owner_pthread_ptr_read = pthread_ptr;
      ofile_lst[desc].owner_pthread_ptr_write = pthread_ptr;
      //Make ready to accept connection
      ((socket_t*)hsock)->state = STATE_SOCKET_LISTEN;
   }
   __atomic_out();
   //
   //wait connected operation from socketEngine
   __WAIT_SOCKET_EVENT(pthread_ptr, hsock);
   //
   if (((socket_t*)hsock)->state != STATE_SOCKET_CONNECTED) {
      __set_kernel_pthread_errno(ENOTCONN);
      return -1;
   }
   //((socket_t*)hsock)->state=STATE_SOCKET_ACCEPTED;
   //wait Accepted operation from socketEngine
   //__WAIT_SOCKET_EVENT(hsock);
   //
   hsock_accepted = ((socket_t*)hsock)->hsocks;
   if (hsock_accepted == (hsock_t)0) {
      __set_kernel_pthread_errno(ENOTCONN);
      return -1;
   }
   //
#if UIP_CONF_IPV6
   memcpy(address, &((socket_t*)hsock_accepted)->addr_in, sizeof(struct _sockaddr_in6));
   //
   *len = sizeof(struct _sockaddr_in6);
   //
#else
   ((struct _sockaddr_in*)address)->sin_port = ((socket_t*)hsock_accepted)->addr_in.sin_port;
   ((struct _sockaddr_in*)address)->sin_addr.s_addr = ((socket_t*)hsock_accepted)->addr_in.sin_addr.s_addr;
   //
   *len = sizeof(struct _sockaddr_in);
   //
#endif
   //
   __set_kernel_pthread_errno(0);
   //
   return ((socket_t*)hsock_accepted)->fd;
}

/*--------------------------------------------
| Name: uip_core_socket_accepted
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int uip_core_socket_accepted(desc_t desc, int native_socket_fd) {
   kernel_pthread_t* pthread_ptr;
   hsock_t hsock_accepted = 0;

   //
   if (!(pthread_ptr = kernel_pthread_self())) {
      __set_kernel_pthread_errno(ESRCH);
      return -1;
   }
   //
   if (desc<0) {
      __set_kernel_pthread_errno(EBADF);
      return -1;
   }
   //
   if ((hsock_accepted = sock_fd_to_hsock(native_socket_fd)) == (hsock_t)0) {
      __set_kernel_pthread_errno(EBADF);
      return -1;
   }
   //
   //set thread owner
   __atomic_in();
   {
      ((socket_t*)hsock_accepted)->desc = desc;
      ofile_lst[desc].owner_pthread_ptr_read = pthread_ptr;
      ofile_lst[desc].owner_pthread_ptr_write = pthread_ptr;
   }
   __atomic_out();
   
   //
   uip_core_socket_info_list[desc].native_uip_socket_fd = native_socket_fd;
   uip_core_socket_info_list[desc].desc = desc;
   //
   kernel_net_core_set_specific_info(desc, &uip_core_socket_info_list[desc]);
   //
   __set_kernel_pthread_errno(0);
   //
   return 0;
}

/*--------------------------------------------
| Name: uip_core_socket_connect
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int uip_core_socket_connect(desc_t desc, struct sockaddr *address, socklen_t len) {
   kernel_pthread_t* pthread_ptr;
   hsock_t hsock;

   //
   if (!(pthread_ptr = kernel_pthread_self())) {
      __set_kernel_pthread_errno(ESRCH);
      return -1;
   }
   //
   if (desc<0) {
      __set_kernel_pthread_errno(EBADF);
      return -1;
   }
   //
   uip_core_socket_info_t* p_uip_core_socket_info = kernel_net_core_get_specific_info(desc);
   if (p_uip_core_socket_info == (uip_core_socket_info_t*)0) {
      return -1;
   }
   //
   hsock = sock_fd_to_hsock(p_uip_core_socket_info->native_uip_socket_fd);
   if (hsock == (hsock_t)0) {
      return -1;
   }

   //
   if (((socket_t*)(hsock))->state == STATE_SOCKET_CLOSED) {
      __set_kernel_pthread_errno(ENOTCONN);
      return -1;
   }
   if (((socket_t*)(hsock))->state == STATE_SOCKET_ABORTED) {
      __set_kernel_pthread_errno(ECONNABORTED);
      return -1;
   }
   if (((socket_t*)hsock)->protocol != IPPROTO_TCP) {
      __set_kernel_pthread_errno(EPROTONOSUPPORT);
      return -1;
   }
   //lock
   __lock_io(ofile_lst[desc].owner_pthread_ptr_write, desc, O_WRONLY);
   __lock_io(ofile_lst[desc].owner_pthread_ptr_read, desc, O_RDONLY);
   //
   ((socket_t*)hsock)->state = STATE_SOCKET_CONNECT;
   //
#if UIP_CONF_IPV6
   memcpy(&((socket_t*)hsock)->addr_in, address, sizeof(struct _sockaddr_in6));
   //
   uip_conn = uip_connect((uip_ipaddr_t*)(&((socket_t*)hsock)->addr_in.sin6_addr.s6_addr),
      (u16_t)(((socket_t*)hsock)->addr_in.sin6_port));
#else
   ((socket_t*)hsock)->addr_in.sin_port = ((struct _sockaddr_in*)address)->sin_port;
   ((socket_t*)hsock)->addr_in.sin_addr.s_addr = ((struct _sockaddr_in*)address)->sin_addr.s_addr;
   //
   uip_conn = uip_connect((uip_ipaddr_t*)(&((socket_t*)hsock)->addr_in.sin_addr.s_addr),
      (u16_t)(((socket_t*)hsock)->addr_in.sin_port));
#endif

   //
   if (!uip_conn) {
      //unlock
      __unlock_io(pthread_ptr, desc, O_WRONLY);
      __unlock_io(pthread_ptr, desc, O_RDONLY);
      return -1;
   }
   //
   ((socket_t*)hsock)->r = 0;
   ((socket_t*)hsock)->w = 0;
   // 
   ((socket_t*)hsock)->socksconn = (struct socksconn_state *)(uip_conn->appstate.state);
   //
   ((socket_t*)hsock)->socksconn->__r = 0;
   ((socket_t*)hsock)->socksconn->_r = 0;
   ((socket_t*)hsock)->socksconn->_w = 0;
   //
   ((socket_t*)hsock)->socksconn->hsocks = hsock;
   //unlock
   __unlock_io(pthread_ptr, desc, O_WRONLY);
   __unlock_io(pthread_ptr, desc, O_RDONLY);

   //to do:lepton
   //Wake ip_stack????
   //to do:lepton: syscall kernel
   //OS_WakeTask(_OS_TASK_TCB(os_ipStack));
   //Wait uip_connected();
   uip_core_queue_put(UIP_POLL_REQUEST, desc, (void*)address, len);
   //
   __WAIT_SOCKET_EVENT(pthread_ptr, hsock);
   if (((socket_t*)hsock)->state != STATE_SOCKET_WAIT)
      return -1;
   //
   __set_kernel_pthread_errno(0);
   //
   return 0;
}

/*--------------------------------------------
| Name: uip_core_socket_listen
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int uip_core_socket_listen(desc_t desc, int backlog) {
   kernel_pthread_t* pthread_ptr;
   hsock_t hsock;

   //
   if (!(pthread_ptr = kernel_pthread_self())) {
      __set_kernel_pthread_errno(ESRCH);
      return -1;
   }
   //
   if (desc<0) {
      __set_kernel_pthread_errno(EBADF);
      return -1;
   }
   //
   uip_core_socket_info_t* p_uip_core_socket_info = kernel_net_core_get_specific_info(desc);
   if (p_uip_core_socket_info == (uip_core_socket_info_t*)0) {
      return -1;
   }
   //
   hsock = sock_fd_to_hsock(p_uip_core_socket_info->native_uip_socket_fd);
   if (hsock == (hsock_t)0) {
      return -1;
   }

   if (((socket_t*)hsock)->protocol != IPPROTO_TCP) {
      __set_kernel_pthread_errno(EPROTONOSUPPORT);
      return -1;
   }
   //lock
   __lock_io(pthread_ptr, desc, O_WRONLY);
   __lock_io(pthread_ptr, desc, O_RDONLY);
   //
   ((socket_t*)hsock)->state = STATE_SOCKET_LISTEN;
#if UIP_CONF_IPV6
   uip_listen((u16_t)(((socket_t*)hsock)->addr_in.sin6_port));
#else
   uip_listen((u16_t)(((socket_t*)hsock)->addr_in.sin_port));
#endif
   //unlock
   __unlock_io(pthread_ptr, desc, O_WRONLY);
   __unlock_io(pthread_ptr, desc, O_RDONLY);
   //
   __set_kernel_pthread_errno(0);
   //
   return 0;
}

/*--------------------------------------------
| Name: uip_core_socket_shutdown
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int uip_core_socket_shutdown(desc_t desc, int how) {
   kernel_pthread_t* pthread_ptr;
   hsock_t hsock;

   //
   if (!(pthread_ptr = kernel_pthread_self())) {
      __set_kernel_pthread_errno(ESRCH);
      return -1;
   }
   //
   if (desc<0) {
      __set_kernel_pthread_errno(EBADF);
      return -1;
   }
   //
   uip_core_socket_info_t* p_uip_core_socket_info = kernel_net_core_get_specific_info(desc);
   if (p_uip_core_socket_info == (uip_core_socket_info_t*)0) {
      return -1;
   }
   //
   hsock = sock_fd_to_hsock(p_uip_core_socket_info->native_uip_socket_fd);
   if (hsock == (hsock_t)0) {
      return -1;
   }

   //
   if (how != SHUT_RDWR) {
      __set_kernel_pthread_errno(EOPNOTSUPP);
      return -1; //only SHUT_RDWR is supported with current implementation
   }

   //
   if (((socket_t*)(hsock))->state == STATE_SOCKET_CLOSED) {
      __set_kernel_pthread_errno(ENOTCONN);
      return -1;
   }
   if (((socket_t*)(hsock))->state == STATE_SOCKET_ABORTED) {
      __set_kernel_pthread_errno(ECONNABORTED);
      return -1;
   }
   //
   if (((socket_t*)hsock)->socksconn != NULL) {
      /*
      while( ((socket_t*)hsock)->w != ((socket_t*)hsock)->socksconn->_r )
      __WAIT_SOCKET_EVENT(hsock);
      */
   }
   //
   if (((socket_t*)(hsock))->state == STATE_SOCKET_CLOSED
      || ((socket_t*)(hsock))->state == STATE_SOCKET_ABORTED
      || ((socket_t*)(hsock))->state == STATE_SOCKET_ACCEPTED) {
      __CLR_SOCKET_EVENT(hsock);

      sock_put(hsock);
      return -1;
   }
   //
   ((socket_t*)(hsock))->state = STATE_SOCKET_CLOSE;
   //
   while (((socket_t*)(hsock))->state == STATE_SOCKET_WAIT)
      __WAIT_SOCKET_EVENT(pthread_ptr, hsock);
   //
   __CLR_SOCKET_EVENT(hsock);
   //
   if (((socket_t*)hsock)->protocol == IPPROTO_UDP) {
      //free udp connection
      ((socket_t*)hsock)->uip_udp_conn->lport = 0;
   }
   //
   sock_put(hsock);
   //
   __set_kernel_pthread_errno(0);
   //
   return 0;
}

/*--------------------------------------------
| Name: uip_core_socket_close
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int uip_core_socket_close(desc_t desc) {
   kernel_pthread_t* pthread_ptr;
   hsock_t hsock;

   //
   if (!(pthread_ptr = kernel_pthread_self())) {
      __set_kernel_pthread_errno(ESRCH);
      return -1;
   }
   //
   if (desc<0) {
      __set_kernel_pthread_errno(EBADF);
      return -1;
   }
   //
   uip_core_socket_info_t* p_uip_core_socket_info = kernel_net_core_get_specific_info(desc);
   if (p_uip_core_socket_info == (uip_core_socket_info_t*)0) {
      return -1;
   }
   //
   hsock = sock_fd_to_hsock(p_uip_core_socket_info->native_uip_socket_fd);
   if (hsock == (hsock_t)0) {
      return -1;
   }
   //
   sock_put(hsock);
   return 0;
}

/*--------------------------------------------
| Name: uip_core_socket_getpeername
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int uip_core_socket_getpeername(desc_t desc, struct sockaddr *name, socklen_t *namelen) {
   return -1;
}

/*--------------------------------------------
| Name: uip_core_socket_getsockname
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int uip_core_socket_getsockname(desc_t desc, struct sockaddr *name, socklen_t *namelen) {
   return -1;
}

/*--------------------------------------------
| Name: uip_core_socket_getsockopt
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int uip_core_socket_getsockopt(desc_t desc, int level, int optname, void *optval, socklen_t *optlen) {
   return -1;
}

/*--------------------------------------------
| Name: uip_core_socket_setsockopt
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int uip_core_socket_setsockopt(desc_t desc, int level, int optname, const void *optval, socklen_t optlen) {
   return -1;
}

/*--------------------------------------------
| Name: uip_core_socket_ioctl
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int uip_core_socket_ioctl(desc_t desc, long cmd, void *argp) {
   return -1;
}

/*--------------------------------------------
| Name: uip_core_socket_gethostbyname
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
struct hostent* uip_core_socket_gethostbyname(desc_t desc, struct hostent* host, const char *name) {
   return (struct hostent*)(0);
}

/*-------------------------------------------
| Name:dev_uip_core_socket_load
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_uip_core_socket_load(void) {
   uip_sock_init();
   return 0;
}

/*-------------------------------------------
| Name:dev_uip_core_socket_open
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_uip_core_socket_open(desc_t desc, int o_flag) {
   //
   kernel_net_core_set_socket_op(desc,&kernel_net_core_socket_op);
   kernel_net_core_set_specific_info(desc, &uip_core_socket_info_list[desc]);
   //
   return 0;
}

/*-------------------------------------------
| Name:dev_uip_core_socket_close
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_uip_core_socket_close(desc_t desc) {
   //hsock_t hsock = ofile_lst[desc].ext.hsock;
   if ((ofile_lst[desc].oflag & O_RDONLY)
      && ofile_lst[desc].oflag & O_WRONLY) {

      if (!ofile_lst[desc].nb_reader && !ofile_lst[desc].nb_writer) {
         uip_core_socket_shutdown(desc, SHUT_RDWR);
         uip_core_socket_close(desc);
         //check id pthread owner has been set to null (event call back) 
      }

   }

   return 0;
}

/*-------------------------------------------
| Name:dev_uip_core_socket_isset_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_uip_core_socket_isset_read(desc_t desc) {
   uip_core_socket_info_t* p_uip_core_socket_info = kernel_net_core_get_specific_info(desc);
   if (!p_uip_core_socket_info) {
      return -1;
   }
   //
   if (p_uip_core_socket_info->native_uip_socket_fd < 0) {
      return -1;
   } 
   //
   hsock_t hsock = sock_fd_to_hsock(p_uip_core_socket_info->native_uip_socket_fd);
   if (hsock == (hsock_t)0) {
      return -1;
   }

   //
   int r = ((socket_t*)hsock)->r;
   int _w = ((socket_t*)hsock)->socksconn->_w;
   //
   if ((((socket_t*)(hsock))->state != STATE_SOCKET_WAIT))
      return 0;
   //
   if (r != _w)//recv data ok.
      return 0;

   return -1;
}

/*-------------------------------------------
| Name:dev_uip_core_socket_isset_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_uip_core_socket_isset_write(desc_t desc) {
   uip_core_socket_info_t* p_uip_core_socket_info = kernel_net_core_get_specific_info(desc);
   if (!p_uip_core_socket_info) {
      return -1;
   }
   //
   if (p_uip_core_socket_info->native_uip_socket_fd < 0) {
      return -1;
   }
   //
   hsock_t hsock = sock_fd_to_hsock(p_uip_core_socket_info->native_uip_socket_fd);
   if (hsock == (hsock_t)0) {
      return -1;
   }
  
   int w = ((socket_t*)hsock)->w;
   int _r = ((socket_t*)hsock)->socksconn->_r;
   int free_size;
   //
   if ((((socket_t*)(hsock))->state != STATE_SOCKET_WAIT)) {
      return 0;
   }
   //
   if ((free_size = (_r - w)) <= 0)
      free_size = SND_SOCKET_BUFFER_SIZE + free_size;
   //
   if (w == _r) {//send data ok.
      return 0;
   }
   
   //
   return -1;
}

/*-------------------------------------------
| Name:dev_uip_core_socket_read
| Description:
| Parameters:
| Return Type:
| Comments:    uip 3.0
| See:
---------------------------------------------*/
static int dev_uip_core_socket_read(desc_t desc, char* buffer, int nbyte) {
   int cb = 0;
   hsock_t hsock;
  
   int len = 0;
   int* plen;
   int r = 0;
   int _w = 0;

   //
   uip_core_socket_info_t* p_uip_core_socket_info = kernel_net_core_get_specific_info(desc);
   if (!p_uip_core_socket_info) {
      return -1;
   }
   //
   if (p_uip_core_socket_info->native_uip_socket_fd < 0) {
      return -1;
   }
   //
   hsock = sock_fd_to_hsock(p_uip_core_socket_info->native_uip_socket_fd);
   if (hsock == (hsock_t)0) {
      return -1;
   }

   //
   r = ((socket_t*)hsock)->r;
   _w = ((socket_t*)hsock)->socksconn->_w;
   //
   if (((socket_t*)(hsock))->state == STATE_SOCKET_CLOSED) {
      return -1;
   }
   if (((socket_t*)(hsock))->state == STATE_SOCKET_ABORTED) {
      return -1;
   }
   if (((socket_t*)(hsock))->state == STATE_SOCKET_NETDOWN) {
      return -1;
   }
   //
   if ((r == _w) && (((socket_t*)(hsock))->state != STATE_SOCKET_WAIT))
      return -1;
   //if MSG_DONTWAIT and socket buffer empty return 0
   if (r == _w)
      return 0;
   // For UDP datagram, get the packet header from the ring buffer
   if (((socket_t*)hsock)->protocol == IPPROTO_UDP) {
      //begin udp: get header
      cb = sizeof(socket_recvfrom_header_t);//32bytes for v6, 20bytes for v4
                                            //Case where both sock_addr and packet length are in sequence
      if ((r + cb)<RCV_SOCKET_BUFFER_SIZE) {
         //Start with source address strcuture
         memcpy(&((socket_t*)(hsock))->addr_in_from, &((socket_t*)hsock)->socksconn->rcv_buffer[r], sizeof(((socket_t*)(hsock))->addr_in_from));
         r = r + sizeof(((socket_t*)(hsock))->addr_in_from);
         //Then go with length
         plen = (int*)&((socket_t*)hsock)->socksconn->rcv_buffer[r];
         len = *plen;
         r = r + sizeof(int);
      }
      else {
         cb = sizeof(((socket_t*)(hsock))->addr_in_from); //_sockaddr_in6 is 28bytes, _sockaddr_in is 16bytes
                                                          //Then case where sock_addr fits, but not the length
         if ((r + cb)<RCV_SOCKET_BUFFER_SIZE) {
            //Start with source address strcuture
            memcpy(&((socket_t*)(hsock))->addr_in_from, &((socket_t*)hsock)->socksconn->rcv_buffer[r], cb);
            r = r + cb;
            //Then go with length
            memcpy(&len, &((socket_t*)hsock)->socksconn->rcv_buffer[r], RCV_SOCKET_BUFFER_SIZE - r);
            memcpy(((char*)&len) + (RCV_SOCKET_BUFFER_SIZE - r), ((socket_t*)hsock)->socksconn->rcv_buffer, sizeof(int) - (RCV_SOCKET_BUFFER_SIZE - r));
            r = sizeof(int) - (RCV_SOCKET_BUFFER_SIZE - r);
         }
         else {
            //Start with source address strcuture
            memcpy(&((socket_t*)(hsock))->addr_in_from, &((socket_t*)hsock)->socksconn->rcv_buffer[r], RCV_SOCKET_BUFFER_SIZE - r);
            memcpy(((char*)&((socket_t*)(hsock))->addr_in_from) + (RCV_SOCKET_BUFFER_SIZE - r), ((socket_t*)hsock)->socksconn->rcv_buffer, cb - (RCV_SOCKET_BUFFER_SIZE - r));
            r = cb - (RCV_SOCKET_BUFFER_SIZE - r);
            //Then go with length
            plen = (int*)&((socket_t*)hsock)->socksconn->rcv_buffer[r];
            len = *plen;
            r = r + sizeof(int);
         }
      }
      //get packet len
      cb = len;
      //end udp
   }
   else {
      if (r <= _w) {
         cb = _w - r;
      }
      else {
         cb = (RCV_SOCKET_BUFFER_SIZE - r) + _w;
      }
   }

   // Trap any internal issue from ring buffer 
   if (cb<0) {
      return -1;
   }

   if (nbyte <= cb)
      cb = nbyte;
   //get packet
   if ((r + cb)<RCV_SOCKET_BUFFER_SIZE) {
      memcpy(buffer, &((socket_t*)hsock)->socksconn->rcv_buffer[r], cb);
      r = r + cb;
   }
   else {
      memcpy(buffer, &((socket_t*)hsock)->socksconn->rcv_buffer[r], RCV_SOCKET_BUFFER_SIZE - r);
      memcpy(buffer + (RCV_SOCKET_BUFFER_SIZE - r), ((socket_t*)hsock)->socksconn->rcv_buffer, cb - (RCV_SOCKET_BUFFER_SIZE - r));
      r = cb - (RCV_SOCKET_BUFFER_SIZE - r);
   }
   //
   if ((((socket_t*)hsock)->flags&MSG_PEEK))
      return cb;
   //go to next packet
   ((socket_t*)hsock)->r = r;
   return cb;
}

/*-------------------------------------------
| Name:dev_uip_core_socket_write
| Description:
| Parameters:
| Return Type:
| Comments:    uip 3.0
| See:
---------------------------------------------*/
static int dev_uip_core_socket_write(desc_t desc, const char *buffer, int nbyte) {
   int cb = 0;
   hsock_t hsock;
   int w;

   //
   uip_core_socket_info_t* p_uip_core_socket_info = kernel_net_core_get_specific_info(desc);
   if (!p_uip_core_socket_info) {
      return -1;
   }
   //
   if (p_uip_core_socket_info->native_uip_socket_fd < 0) {
      return -1;
   }
   //
   hsock = sock_fd_to_hsock(p_uip_core_socket_info->native_uip_socket_fd);
   if (hsock == (hsock_t)0) {
      return -1;
   }


#if UIP_LOGGING==1
   ANNOTATE("0 ready to send\r\n");
#endif

   if (((socket_t*)hsock)->state != STATE_SOCKET_WAIT)
      return -1;

   if (nbyte >= SND_SOCKET_BUFFER_SIZE)
      cb = SND_SOCKET_BUFFER_SIZE - 1;
   else
      cb = nbyte;

   //lock, already lock in write().
   //__lock_io(ofile_lst[desc].owner_pthread_ptr_write,desc,O_WRONLY);
   //
   w = ((socket_t*)hsock)->w;
#if UIP_LOGGING==1
   ANNOTATE("1)desc=%d dev_sock_write w=%d _r=%d\r\n", desc, w, ((socket_t*)hsock)->socksconn->_r);
#endif

   if ((w + cb)<SND_SOCKET_BUFFER_SIZE) {
      memcpy(&((socket_t*)hsock)->socksconn->snd_buffer[w], buffer, cb);
      w = w + cb;
   }
   else {
      memcpy(&((socket_t*)hsock)->socksconn->snd_buffer[w], buffer, SND_SOCKET_BUFFER_SIZE - w);
      memcpy(((socket_t*)hsock)->socksconn->snd_buffer, (char*)&buffer[SND_SOCKET_BUFFER_SIZE - w], cb - (SND_SOCKET_BUFFER_SIZE - w));
      w = cb - (SND_SOCKET_BUFFER_SIZE - w);
   }

   ((socket_t*)hsock)->w = w;
#if UIP_LOGGING==1
   ANNOTATE("2)desc=%d dev_sock_write w=%d _r=%d\r\n", desc, w, ((socket_t*)hsock)->socksconn->_r);
#endif

   //unlock
   //__unlock_io(ofile_lst[desc].owner_pthread_ptr_write,desc,O_WRONLY);


   //Wake ip_stack
   //to do:lepton: syscall kernel
   //OS_WakeTask(_OS_TASK_TCB(os_ipStack));
   //   netsend.desc = desc;
   //__mk_uip_core_syscall(_SYSCALL_NET_SND,netsend);
   uip_core_queue_put(UIP_TIMER, desc, (void*)buffer, nbyte);
   //

   return cb;
}

/*-------------------------------------------
| Name:dev_uip_core_socket_read_args
| Description:
| Parameters:
| Return Type:
| Comments:    uip 3.0
| See:
---------------------------------------------*/
static int dev_uip_core_socket_read_args(desc_t desc, char* buffer, int len, va_list ap) {
   kernel_pthread_t* pthread_ptr;
   hsock_t hsock;
   int r;
   #if UIP_CONF_IPV6
      desc_t desc_if;
   #endif

   //
   unsigned int flags = va_arg(ap, unsigned int);
   struct sockaddr *from = va_arg(ap, struct sockaddr*);
   socklen_t *fromlen = va_arg(ap, socklen_t *);
   
   //
   if (!(pthread_ptr = kernel_pthread_self())) {
      __set_kernel_pthread_errno(ESRCH);
      return -1;
   }
   //
   uip_core_socket_info_t* p_uip_core_socket_info = kernel_net_core_get_specific_info(desc);
   if (!p_uip_core_socket_info) {
      return -1;
   }
   //
   if (p_uip_core_socket_info->native_uip_socket_fd < 0) {
      return -1;
   }
   //
   hsock = sock_fd_to_hsock(p_uip_core_socket_info->native_uip_socket_fd);
   if (hsock == (hsock_t)0) {
      return -1;
   }

   //
   if (((socket_t*)(hsock))->state == STATE_SOCKET_CLOSED) {
      __set_kernel_pthread_errno(ENOTCONN);
      return -1;
   }
   if (((socket_t*)(hsock))->state == STATE_SOCKET_ABORTED) {
      __set_kernel_pthread_errno(ECONNABORTED);
      return -1;
   }
   if (((socket_t*)(hsock))->state == STATE_SOCKET_NETDOWN) {
      __set_kernel_pthread_errno(ENETDOWN);
      return -1;
   }
   if (((socket_t*)hsock)->protocol != IPPROTO_UDP) {
      __set_kernel_pthread_errno(EPROTONOSUPPORT);
      return -1;
   }
   //
#if UIP_CONF_IPV6
   desc_if = uip_core_if_indextodesc(((socket_t*)hsock)->addr_in.sin6_scope_id, O_RDONLY);
   if (desc_if<0) {
      __set_kernel_pthread_errno(EINVAL);
      return -1;
   }
#endif
   /* to do
   if(dev_core_ioctl(desc_if,NETUP)<0){
   __set_kernel_pthread_errno (ENETDOWN);
   return -1;
   }
   */
   //lock
   __lock_io(ofile_lst[desc].owner_pthread_ptr_read, desc, O_RDONLY);
   //Prepare flags
   if (flags & MSG_DONTWAIT)
      ofile_lst[desc].oflag |= O_NONBLOCK; //GD 2011/05/24 //GD 2011/07/05: standard MSG_DONTWAIT flag compliancy (but still a workaround to pass the flag to dev_core_read())
   if (flags & MSG_PEEK)
      ((socket_t*)hsock)->flags |= MSG_PEEK;
   //unlock
   __unlock_io(ofile_lst[desc].owner_pthread_ptr_read, desc, O_RDONLY);
   //
   if (r = dev_uip_core_socket_read(desc, buffer, len))
   {
      //
#if UIP_CONF_IPV6
      if(from)
         memcpy(from, &((socket_t*)hsock)->addr_in_from, sizeof(struct _sockaddr_in6));
#else
      if(from)
         memcpy(from, &((socket_t*)(hsock))->addr_in_from, sizeof(struct _sockaddr_in));
#endif   
   }
   if (r<0) {
      if (((socket_t*)(hsock))->state == STATE_SOCKET_CLOSED) {
         __set_kernel_pthread_errno(ENOTCONN);
         return -1;
      }
      if (((socket_t*)(hsock))->state == STATE_SOCKET_ABORTED) {
         __set_kernel_pthread_errno(ECONNABORTED);
         return -1;
      }
      if (((socket_t*)(hsock))->state == STATE_SOCKET_NETDOWN) {
         __set_kernel_pthread_errno(ENETDOWN);
         return -1;
      }
      //
      /*UIP_EVENT(EVT_UIP_SOCK + EVT_LVL_ERR + EVT_UIP_SOCK_RECVFROM_FAIL,
      desc, &flags, sizeof(flags),
      "recvfrom() failure (return %d)", r);*/
   }
   else {
      __set_kernel_pthread_errno(0);
   }
   //Enter Semaphore
   //lock
   __lock_io(ofile_lst[desc].owner_pthread_ptr_read, desc, O_RDONLY);
   //Restore flags
   if (flags & MSG_DONTWAIT)
      ofile_lst[desc].oflag &= ~(O_NONBLOCK);
   if (flags & MSG_PEEK)
      ((socket_t*)hsock)->flags &= ~(MSG_PEEK);
   //unlock
   __unlock_io(ofile_lst[desc].owner_pthread_ptr_read, desc, O_RDONLY);
   //
   return r;
}

/*-------------------------------------------
| Name:dev_uip_core_socket_write_args
| Description:
| Parameters:
| Return Type:
| Comments:    uip 3.0
| See:
---------------------------------------------*/
static int dev_uip_core_socket_write_args(desc_t desc, const char *buffer, int size, va_list ap) {
   kernel_pthread_t* pthread_ptr;
   hsock_t hsock;
   int w;
#if UIP_CONF_IPV6
   desc_t desc_if;
#endif

   //
   unsigned int flags = va_arg(ap, unsigned int);
   struct sockaddr *to = va_arg(ap, struct sockaddr*);
   socklen_t tolen = va_arg(ap, socklen_t);

   //
   if (!(pthread_ptr = kernel_pthread_self())) {
      __set_kernel_pthread_errno(ESRCH);
      return -1;
   }
   //
   uip_core_socket_info_t* p_uip_core_socket_info = kernel_net_core_get_specific_info(desc);
   if (!p_uip_core_socket_info) {
      return -1;
   }
   //
   if (p_uip_core_socket_info->native_uip_socket_fd < 0) {
      return -1;
   }
   //
   hsock = sock_fd_to_hsock(p_uip_core_socket_info->native_uip_socket_fd);
   if (hsock == (hsock_t)0) {
      return -1;
   }

   //
   if (((socket_t*)(hsock))->state == STATE_SOCKET_CLOSED) {
      __set_kernel_pthread_errno(ENOTCONN);
      return -1;
   }
   if (((socket_t*)(hsock))->state == STATE_SOCKET_ABORTED) {
      __set_kernel_pthread_errno(ECONNABORTED);
      return -1;
   }
   if (((socket_t*)hsock)->protocol != IPPROTO_UDP) {
      __set_kernel_pthread_errno(EPROTONOSUPPORT);
      return -1;
   }
   //
#if UIP_CONF_IPV6
   desc_if = uip_core_if_indextodesc(((socket_t*)hsock)->addr_in.sin6_scope_id, O_RDONLY);
   if (desc_if<0) {
      __set_kernel_pthread_errno(EINVAL);
      return -1;
   }
#endif
   /*to do
   if(dev_core_ioctl(desc_if,NETUP)<0){
   __set_kernel_pthread_errno (ENETDOWN);
   return -1;
   }
   */
   //
#if UIP_CONF_IPV6
   if(to)
      memcpy(&((socket_t*)hsock)->addr_in_to, to, sizeof(struct _sockaddr_in6));
#else
   if(to)
      memcpy(&((socket_t*)(hsock))->addr_in_to, dest_addr, sizeof(struct _sockaddr_in));
#endif
   //
   w = dev_uip_core_socket_write(desc, buffer, size);
   if (w<0) {
      if (((socket_t*)(hsock))->state == STATE_SOCKET_CLOSED) {
         __set_kernel_pthread_errno(ENOTCONN);
         return -1;
      }
      if (((socket_t*)(hsock))->state == STATE_SOCKET_ABORTED) {
         __set_kernel_pthread_errno(ECONNABORTED);
         return -1;
      }
      if (((socket_t*)(hsock))->state == STATE_SOCKET_NETDOWN) {
         __set_kernel_pthread_errno(ENETDOWN);
         return -1;
      }
      //
      /*UIP_EVENT(EVT_UIP_SOCK + EVT_LVL_ERR + EVT_UIP_SOCK_SENDTO_FAIL,
      desc, &length, sizeof(length),
      "sendto() failure (return %d)(flags %d)", r, flags);*/
   }
   else {
      __set_kernel_pthread_errno(0);
   }
   //
   return w;
}

/*-------------------------------------------
| Name:dev_uip_core_socket_seek
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_uip_core_socket_seek(desc_t desc, int offset, int origin) {
   return -1;
}

/*--------------------------------------------
| Name:dev_uip_core_socket_ioctl
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static  int dev_uip_core_socket_ioctl(desc_t desc, int request, va_list ap) {
   kernel_pthread_t* pthread_ptr;
   hsock_t hsock;

   //
   if (!(pthread_ptr = kernel_pthread_self())) {
      __set_kernel_pthread_errno(ESRCH);
      return -1;
   }
   //
   uip_core_socket_info_t* p_uip_core_socket_info = kernel_net_core_get_specific_info(desc);
   if (!p_uip_core_socket_info) {
      return -1;
   }
   //
   if (p_uip_core_socket_info->native_uip_socket_fd < 0) {
      return -1;
   }
   //
   hsock = sock_fd_to_hsock(p_uip_core_socket_info->native_uip_socket_fd);
   if (hsock == (hsock_t)0) {
      return -1;
   }
   
   //
   switch (request) {

      case SETUIPSOCKOPT: {
         int v = va_arg(ap, int);
         switch (v) {
         case IPPROTO_UDP:
         case IPPROTO_TCP:
            ((socket_t*)hsock)->protocol = v;
            return 0;
         }//

      }//
   }

   //
   return -1;
}

/*============================================
| End of Source  : uip_core_socket.c
==============================================*/
