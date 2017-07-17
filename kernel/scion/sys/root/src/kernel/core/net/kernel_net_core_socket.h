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
| Compiler Directive
==============================================*/
#ifndef __NET_CORE_SOCKET_H__
#define __NET_CORE_SOCKET_H__


/*============================================
| Includes
==============================================*/




/*============================================
| Declaration
==============================================*/

//
typedef int (*pfn_kernel_net_core_socket_t)(desc_t desc, int domain, int type, int protocol);
typedef int (*pfn_kernel_net_core_bind_t)(desc_t desc, struct sockaddr *name, socklen_t namelen);
typedef int (*pfn_kernel_net_core_accept_t)(desc_t desc, struct sockaddr *addr, socklen_t *addrlen);
typedef int (*pfn_kernel_net_core_accepted_t)(desc_t desc, int native_socket_fd);
typedef int (*pfn_kernel_net_core_connect_t)(desc_t desc, struct sockaddr *name, socklen_t namelen);
typedef int (*pfn_kernel_net_core_listen_t)(desc_t desc, int backlog);
typedef int (*pfn_kernel_net_core_shutdown_t)(desc_t desc, int how);
typedef int (*pfn_kernel_net_core_close_t)(desc_t desc);
//
typedef int (*pfn_kernel_net_core_getpeername_t)(desc_t desc, struct sockaddr *name, socklen_t *namelen);
typedef int (*pfn_kernel_net_core_getsockname_t)(desc_t desc, struct sockaddr *name, socklen_t *namelen);
typedef int (*pfn_kernel_net_core_getsockopt_t)(desc_t desc, int level, int optname, void *optval, socklen_t *optlen);
typedef int (*pfn_kernel_net_core_setsockopt_t)(desc_t desc, int level, int optname, const void *optval, socklen_t optlen);
//
typedef struct hostent* (*pfn_kernel_net_core_gethostbyname_t)(desc_t desc,struct hostent* host,const char *name);

//
typedef struct kernel_net_core_socket_op_st {
   pfn_kernel_net_core_socket_t kernel_net_core_socket;
   pfn_kernel_net_core_bind_t   kernel_net_core_bind;
   pfn_kernel_net_core_accept_t kernel_net_core_accept;
   pfn_kernel_net_core_accepted_t kernel_net_core_accepted;
   pfn_kernel_net_core_connect_t kernel_net_core_connect;
   pfn_kernel_net_core_listen_t kernel_net_core_listen;
   pfn_kernel_net_core_shutdown_t kernel_net_core_shutdown;
   pfn_kernel_net_core_close_t kernel_net_core_close;
   //
   pfn_kernel_net_core_getpeername_t kernel_net_core_getpeername;
   pfn_kernel_net_core_getsockname_t kernel_net_core_getsockname;
   pfn_kernel_net_core_getsockopt_t kernel_net_core_getsockopt;
   pfn_kernel_net_core_setsockopt_t kernel_net_core_setsockopt;
   pfn_kernel_net_core_gethostbyname_t kernel_net_core_gethostbyname;
}kernel_net_core_socket_op_t;

//
typedef struct kernel_net_core_socket_st {
   const kernel_net_core_socket_op_t* p_kernel_net_core_socket_op;
   void* p_kernel_net_core_socket_specific_info;
}kernel_net_core_socket_t;

//
void kernel_net_core_set_socket_op(desc_t desc, const kernel_net_core_socket_op_t* p_kernel_net_core_socket_op);
//
void kernel_net_core_set_specific_info(desc_t desc, void* p_kernel_net_core_socket_specific_info);
void* kernel_net_core_get_specific_info(desc_t desc);

//
int kernel_net_core_socket(desc_t desc, int domain, int type, int protocol);
int kernel_net_core_bind(desc_t desc, struct sockaddr *name, socklen_t namelen);
int kernel_net_core_accept(desc_t desc, struct sockaddr *addr, socklen_t *addrlen);
int kernel_net_core_accepted(desc_t desc, int native_socket_fd);
int kernel_net_core_connect(desc_t desc, struct sockaddr *name, socklen_t namelen);
int kernel_net_core_listen(desc_t desc, int backlog);
int kernel_net_core_shutdown(desc_t desc, int how);
int kernel_net_core_close(desc_t desc);
int kernel_net_core_getpeername(desc_t desc, struct sockaddr *name, socklen_t *namelen);
int kernel_net_core_getsockname(desc_t desc, struct sockaddr *name, socklen_t *namelen);
int kernel_net_core_getsockopt(desc_t desc, int level, int optname, void *optval, socklen_t *optlen);
int kernel_net_core_setsockopt(desc_t desc, int level, int optname, const void *optval, socklen_t optlen);
struct hostent* kernel_net_core_gethostbyname(desc_t desc,struct hostent* host, const char *name);

#endif


