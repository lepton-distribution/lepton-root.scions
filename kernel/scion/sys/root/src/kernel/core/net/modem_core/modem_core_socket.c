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
#include "kernel/core/errno.h"
#include "kernel/core/system.h"
#include "kernel/core/process.h"
#include "kernel/core/kernel.h"
#include "kernel/core/fcntl.h"
#include "kernel/core/stat.h"

#include "kernel/fs/vfs/vfstypes.h"
#include "kernel/fs/vfs/vfskernel.h"
#include "kernel/fs/vfs/vfs.h"

#include "kernel/core/net/modem_core/modem_socket.h"

#include "kernel/core/net/kernel_net_core_socket.h"
#include "kernel/core/net/modem_core/modem_core_api.h"

#include "kernel/core/net/bsd/netdb.h"

/*============================================
| Global Declaration
==============================================*/


//
static int dev_modem_core_socket_load(void);
static int dev_modem_core_socket_open(desc_t desc, int o_flag);
static int dev_modem_core_socket_close(desc_t desc);
static int dev_modem_core_socket_isset_read(desc_t desc);
static int dev_modem_core_socket_isset_write(desc_t desc);
static int dev_modem_core_socket_read(desc_t desc, char* buffer, int nbyte);
static int dev_modem_core_socket_write(desc_t desc, const char* buffer, int nbyte);
static int dev_modem_core_socket_read_args(desc_t desc, char* buffer, int len, va_list ap);
static int dev_modem_core_socket_write_args(desc_t desc, const char* buffer, int len, va_list ap);
static int dev_modem_core_socket_seek(desc_t desc, int offset, int origin);
static int dev_modem_core_socket_ioctl(desc_t desc, int request, va_list ap);

static const char dev_modem_core_socket_name[] = "net/socket";

dev_map_t  dev_modem_core_socket_map = {
   .dev_name = dev_modem_core_socket_name,
   .dev_attr = S_IFCHR,
   .fdev_load = dev_modem_core_socket_load,
   .fdev_open = dev_modem_core_socket_open,
   .fdev_close = dev_modem_core_socket_close,
   .fdev_isset_read = dev_modem_core_socket_isset_read,
   .fdev_isset_write = dev_modem_core_socket_isset_write,
   .fdev_read = dev_modem_core_socket_read,
   .fdev_write = dev_modem_core_socket_write,
   .fdev_read_args = dev_modem_core_socket_read_args,
   .fdev_write_args = dev_modem_core_socket_write_args,
   .fdev_seek = dev_modem_core_socket_seek,
   .fdev_ioctl = dev_modem_core_socket_ioctl
};


//
int modem_core_socket_socket(desc_t desc, int domain, int type, int protocol);
int modem_core_socket_bind(desc_t desc, struct sockaddr *name, socklen_t namelen);
int modem_core_socket_accept(desc_t desc, struct sockaddr *addr, socklen_t *addrlen);
int modem_core_socket_accepted(desc_t desc, int native_socket_fd);
int modem_core_socket_connect(desc_t desc, struct sockaddr *name, socklen_t namelen);
int modem_core_socket_listen(desc_t desc, int backlog);
int modem_core_socket_shutdown(desc_t desc, int how);
int modem_core_socket_close(desc_t desc);
//
int modem_core_socket_getpeername(desc_t desc, struct sockaddr *name, socklen_t *namelen);
int modem_core_socket_getsockname(desc_t desc, struct sockaddr *name, socklen_t *namelen);
int modem_core_socket_getsockopt(desc_t desc, int level, int optname, void *optval, socklen_t *optlen);
int modem_core_socket_setsockopt(desc_t desc, int level, int optname, const void *optval, socklen_t optlen);
struct hostent* modem_core_socket_gethostbyname(desc_t desc,struct hostent* host,const char *name);

static  const kernel_net_core_socket_op_t kernel_net_core_socket_op = {
   .kernel_net_core_socket = modem_core_socket_socket,
   .kernel_net_core_bind = modem_core_socket_bind,
   .kernel_net_core_accept = modem_core_socket_accept,
   .kernel_net_core_accepted = modem_core_socket_accepted,
   .kernel_net_core_connect = modem_core_socket_connect,
   .kernel_net_core_listen = modem_core_socket_listen,
   .kernel_net_core_shutdown = modem_core_socket_shutdown,
   .kernel_net_core_close = modem_core_socket_close,
   //
   .kernel_net_core_getpeername = modem_core_socket_getpeername,
   .kernel_net_core_getsockname = modem_core_socket_getsockname,
   .kernel_net_core_getsockopt = modem_core_socket_getsockopt,
   .kernel_net_core_setsockopt = modem_core_socket_setsockopt,
   .kernel_net_core_gethostbyname = modem_core_socket_gethostbyname
};

//
typedef struct modem_core_socket_info_st {
   int native_modem_connexion_index;
   desc_t desc;
}modem_core_socket_info_t;
//
static modem_core_socket_info_t modem_core_socket_info_list[MAX_OPEN_FILE];
static const int modem_core_socket_info_list_size = sizeof(modem_core_socket_info_list) / sizeof(modem_core_socket_info_t);

/*============================================
| Implementation
==============================================*/

/*--------------------------------------------
| Name: modem_core_socket_socket
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int modem_core_socket_socket(desc_t desc, int domain, int type, int protocol) {

   modem_core_socket_info_t* p_modem_core_socket_info = kernel_net_core_get_specific_info(desc);

   if (p_modem_core_socket_info == (modem_core_socket_info_t*)0)
      return -1;

   if ((p_modem_core_socket_info->native_modem_connexion_index = modem_core_api_socket(desc,domain, type, protocol)) < 0) {
      return -1;
   }
   //
   p_modem_core_socket_info->desc = desc;
   //
   return 0;
}

/*--------------------------------------------
| Name: modem_core_socket_bind
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int modem_core_socket_bind(desc_t desc, struct sockaddr *name, socklen_t namelen) {
   if (modem_core_socket_info_list[desc].native_modem_connexion_index < 0)
      return -1;
   //
   return modem_core_api_bind(modem_core_socket_info_list[desc].native_modem_connexion_index, name, namelen);
}

/*--------------------------------------------
| Name: modem_core_socket_accept
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int modem_core_socket_accept(desc_t desc, struct sockaddr *addr, socklen_t *addrlen) {
   int native_modem_socket_accepted_fd;
   //
   if (modem_core_socket_info_list[desc].native_modem_connexion_index < 0)
      return -1;
   //
   native_modem_socket_accepted_fd = modem_core_api_accept(modem_core_socket_info_list[desc].native_modem_connexion_index, addr, addrlen);
   //
   return native_modem_socket_accepted_fd;
}

/*--------------------------------------------
| Name: modem_core_socket_accepted
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int modem_core_socket_accepted(desc_t desc, int native_socket_fd) {
   //
   modem_core_socket_info_list[desc].native_modem_connexion_index = native_socket_fd;
   modem_core_socket_info_list[desc].desc = desc;
   //
   kernel_net_core_set_specific_info(desc, &modem_core_socket_info_list[desc]);
   //
   return 0;
}

/*--------------------------------------------
| Name: modem_core_socket_connect
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int modem_core_socket_connect(desc_t desc, struct sockaddr *name, socklen_t namelen) {
   if (modem_core_socket_info_list[desc].native_modem_connexion_index < 0)
      return -1;
   //
   return modem_core_api_connect(modem_core_socket_info_list[desc].native_modem_connexion_index, name, namelen);
}

/*--------------------------------------------
| Name: modem_core_socket_listen
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int modem_core_socket_listen(desc_t desc, int backlog) {
   if (modem_core_socket_info_list[desc].native_modem_connexion_index < 0)
      return -1;
   //
   return modem_core_api_listen(modem_core_socket_info_list[desc].native_modem_connexion_index,backlog);
}

/*--------------------------------------------
| Name: modem_core_socket_shutdown
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int modem_core_socket_shutdown(desc_t desc, int how) {
   if (modem_core_socket_info_list[desc].native_modem_connexion_index < 0)
      return -1;
   //
   return modem_core_api_shutdown(modem_core_socket_info_list[desc].native_modem_connexion_index,how);
}

/*--------------------------------------------
| Name: modem_core_socket_close
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int modem_core_socket_close(desc_t desc) {
   int r;
   if (modem_core_socket_info_list[desc].native_modem_connexion_index < 0)
      return -1;
   //
   r=modem_core_api_close(modem_core_socket_info_list[desc].native_modem_connexion_index);
   if (r < 0){
      return r;
   }

   //
   modem_core_socket_info_list[desc].native_modem_connexion_index = -1;
   modem_core_socket_info_list[desc].desc = INVALID_DESC;
   //
   return r;
   //
}

/*--------------------------------------------
| Name: modem_core_socket_getpeername
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int modem_core_socket_getpeername(desc_t desc, struct sockaddr *name, socklen_t *namelen) {
   if (modem_core_socket_info_list[desc].native_modem_connexion_index < 0)
      return -1;
   //
   return modem_core_api_getpeername(modem_core_socket_info_list[desc].native_modem_connexion_index, name, namelen);
}

/*--------------------------------------------
| Name: modem_core_socket_getsockname
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int modem_core_socket_getsockname(desc_t desc, struct sockaddr *name, socklen_t *namelen) {
   if (modem_core_socket_info_list[desc].native_modem_connexion_index < 0)
      return -1;
   //
   return modem_core_api_getsockname(modem_core_socket_info_list[desc].native_modem_connexion_index, name, namelen);
}

/*--------------------------------------------
| Name: modem_core_socket_getsockopt
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int modem_core_socket_getsockopt(desc_t desc, int level, int optname, void *optval, socklen_t *optlen) {
   if (modem_core_socket_info_list[desc].native_modem_connexion_index < 0)
      return -1;
   //
   return modem_core_api_getsockopt(modem_core_socket_info_list[desc].native_modem_connexion_index, level, optname, optval, optlen);
}

/*--------------------------------------------
| Name: modem_core_socket_setsockopt
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int modem_core_socket_setsockopt(desc_t desc, int level, int optname, const void *optval, socklen_t optlen) {
   if (modem_core_socket_info_list[desc].native_modem_connexion_index < 0)
      return -1;
   //
   return modem_core_api_setsockopt(modem_core_socket_info_list[desc].native_modem_connexion_index, level, optname, optval, optlen);
}

/*--------------------------------------------
| Name: modem_core_socket_ioctl
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int modem_core_socket_ioctl(desc_t desc, long cmd, void *argp) {
   if (modem_core_socket_info_list[desc].native_modem_connexion_index < 0)
      return -1;
   //
   return modem_core_api_ioctl(modem_core_socket_info_list[desc].native_modem_connexion_index, cmd,argp);
}

/*--------------------------------------------
| Name: modem_core_socket_gethostbyname
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
struct hostent* modem_core_socket_gethostbyname(desc_t desc,struct hostent* host,const char *name) {
   return modem_core_api_gethostbyname(host,name);
}

/*-------------------------------------------
| Name:dev_modem_core_socket_load
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_modem_core_socket_load(void) {
   int i;
   //
   for (i = 0; i < modem_core_socket_info_list_size; i++) {
      modem_core_socket_info_list[i].desc = INVALID_DESC;
      modem_core_socket_info_list[i].native_modem_connexion_index = -1;
   }
   //
   return 0;
}

/*-------------------------------------------
| Name:dev_modem_core_socket_open
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_modem_core_socket_open(desc_t desc, int o_flag) {
   //
   kernel_net_core_set_socket_op(desc,&kernel_net_core_socket_op);
   kernel_net_core_set_specific_info(desc, &modem_core_socket_info_list[desc]);
   //

   return 0;
}

/*-------------------------------------------
| Name:dev_modem_core_socket_close
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_modem_core_socket_close(desc_t desc) {
   //hsock_t hsock = ofile_lst[desc].ext.hsock;
   if ((ofile_lst[desc].oflag & O_RDONLY)
      && ofile_lst[desc].oflag & O_WRONLY) {

      if (!ofile_lst[desc].nb_reader && !ofile_lst[desc].nb_writer) {
         modem_core_socket_shutdown(desc, SHUT_RDWR);
         modem_core_socket_close(desc);
         //check id pthread owner has been set to null (event call back) 
      }

   }
   //
   return 0;
}

/*-------------------------------------------
| Name:dev_modem_core_socket_isset_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_modem_core_socket_isset_read(desc_t desc) {
   modem_core_socket_info_t* p_modem_core_socket_info = kernel_net_core_get_specific_info(desc);
   if (!p_modem_core_socket_info) {
      return -1;
   }
   if (p_modem_core_socket_info->native_modem_connexion_index < 0)
      return -1;
   //
   return modem_core_api_isset_read(p_modem_core_socket_info->native_modem_connexion_index);
}

/*-------------------------------------------
| Name:dev_modem_core_socket_isset_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_modem_core_socket_isset_write(desc_t desc) {
   modem_core_socket_info_t* p_modem_core_socket_info = kernel_net_core_get_specific_info(desc);
   if (!p_modem_core_socket_info) {
      return -1;
   }
   if (p_modem_core_socket_info->native_modem_connexion_index < 0)
      return -1;
   //
   return modem_core_api_isset_write(p_modem_core_socket_info->native_modem_connexion_index);
}

/*-------------------------------------------
| Name:dev_modem_core_socket_read
| Description:
| Parameters:
| Return Type:
| Comments:    lwip 2.0
| See:
---------------------------------------------*/
static int dev_modem_core_socket_read(desc_t desc, char* buffer, int len) {
   modem_core_socket_info_t* p_modem_core_socket_info = kernel_net_core_get_specific_info(desc);
   if (!p_modem_core_socket_info) {
      return -1;
   }
   if (p_modem_core_socket_info->native_modem_connexion_index < 0) {
      return -1;
   }
   //
   return modem_core_api_read(p_modem_core_socket_info->native_modem_connexion_index, buffer, len);
}

/*-------------------------------------------
| Name:dev_modem_core_socket_write
| Description:
| Parameters:
| Return Type:
| Comments:    lwip 2.0
| See:
---------------------------------------------*/
static int dev_modem_core_socket_write(desc_t desc, const char *buffer, int size) {
   modem_core_socket_info_t* p_modem_core_socket_info = kernel_net_core_get_specific_info(desc);
   if (!p_modem_core_socket_info) {
      return -1;
   }
   if (p_modem_core_socket_info->native_modem_connexion_index < 0) {
      return -1;
   }
   //
   return modem_core_api_write(p_modem_core_socket_info->native_modem_connexion_index,buffer,size);
}

/*-------------------------------------------
| Name:dev_modem_core_socket_read_args
| Description:
| Parameters:
| Return Type:
| Comments:    lwip 2.0
| See:
---------------------------------------------*/
static int dev_modem_core_socket_read_args(desc_t desc, char* buffer, int len, va_list ap) {
   modem_core_socket_info_t* p_modem_core_socket_info = kernel_net_core_get_specific_info(desc);
   if (!p_modem_core_socket_info) {
      return -1;
   }
   if (p_modem_core_socket_info->native_modem_connexion_index < 0) {
      return -1;
   }
   //
   unsigned int flags = va_arg(ap, unsigned int);
   struct sockaddr *from = va_arg(ap, struct sockaddr*);
   socklen_t *fromlen = va_arg(ap, socklen_t *);
   
   //
   //return modem_core_api_recvfrom(p_modem_core_socket_info->native_modem_connexion_index, buffer, len, flags, from, fromlen);
   
   //
   return modem_core_api_read(p_modem_core_socket_info->native_modem_connexion_index, buffer, len);
}

/*-------------------------------------------
| Name:dev_modem_core_socket_write_args
| Description:
| Parameters:
| Return Type:
| Comments:    lwip 2.0
| See:
---------------------------------------------*/
static int dev_modem_core_socket_write_args(desc_t desc, const char *buffer, int size, va_list ap) {
   modem_core_socket_info_t* p_modem_core_socket_info = kernel_net_core_get_specific_info(desc);
   if (!p_modem_core_socket_info) {
      return -1;
   }
   if (p_modem_core_socket_info->native_modem_connexion_index < 0) {
      return -1;
   }
   //
   unsigned int flags = va_arg(ap, unsigned int);
   struct sockaddr *to = va_arg(ap, struct sockaddr*);
   socklen_t tolen = va_arg(ap, socklen_t);
   //
   //if (to) {
      //return  modem_core_api_sendto(p_modem_core_socket_info->native_modem_connexion_index, buffer, size, flags, to, tolen);
      //
      //return -1;
   //}
   //
   //return modem_core_api_send(p_modem_core_socket_info->native_modem_connexion_index, buffer, size, flags);
   //
   return modem_core_api_write(p_modem_core_socket_info->native_modem_connexion_index,buffer,size);
}

/*-------------------------------------------
| Name:dev_modem_core_socket_seek
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_modem_core_socket_seek(desc_t desc, int offset, int origin) {
   return -1;
}

/*--------------------------------------------
| Name:dev_modem_core_socket_ioctl
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static  int dev_modem_core_socket_ioctl(desc_t desc, int request, va_list ap) {
   
   modem_core_socket_info_t* p_modem_core_socket_info = kernel_net_core_get_specific_info(desc);
   if (!p_modem_core_socket_info) {
      return -1;
   }
   if (p_modem_core_socket_info->native_modem_connexion_index < 0) {
      return -1;
   }
   //
   void *argp = va_arg(ap, void*);
   //
   return modem_core_api_ioctl(p_modem_core_socket_info->native_modem_connexion_index,request, argp);
}

/*============================================
| End of Source  : modem_core_socket.c
==============================================*/
