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
#ifndef __LIB_LIBC_NET_SOCKET_H__
#define __LIB_LIBC_NET_SOCKET_H__


/*============================================
| Includes
==============================================*/

#include "kernel/core/net/socket.h"
#include "kernel/core/net/kernel_net_core_socket.h"
/*============================================
| Declaration
==============================================*/


int libc_socket(int domain, int type, int protocol);
int libc_bind(int fd, struct sockaddr *name, socklen_t namelen);
int libc_accept(int fd, void *addr, uint32_t *addrlen);
int libc_connect(int fd, struct sockaddr *name, socklen_t namelen);
int libc_listen(int fd, int backlog);
int libc_recv(int fd, void *mem, int len, unsigned int flags);
int libc_recvfrom(int fd, void *mem, int len, unsigned int flags, struct sockaddr *from, socklen_t *fromlen);
int libc_send(int fd, const void *dataptr, int size, unsigned int flags);
int libc_sendto(int fd, void *dataptr, int size, unsigned int flags, struct sockaddr *to, socklen_t tolen);
int libc_shutdown(int fd, int how);
int libc_getpeername(int fd, struct sockaddr *name, socklen_t *namelen);
int libc_getsockname(int fd, struct sockaddr *name, socklen_t *namelen);
int libc_getsockopt(int fd, int level, int optname, void *optval, socklen_t *optlen);
int libc_setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen);

#define socket(__domain__,__type__,__protocol__) libc_socket(__domain__,__type__,__protocol__)
#define bind(__fd__,__name__,__namelen__) libc_bind(__fd__,__name__,__namelen__)
#define accept(__fd__,__addr__,__addrlen__) libc_accept(__fd__,__addr__,__addrlen__)
#define connect(__fd__,__name__,__namelen__) libc_connect(__fd__,__name__,__namelen__)
#define listen(__fd__,__backlog__) libc_listen(__fd__,__backlog__)
#define recv(__fd__,__mem__,__len__,__flags__) libc_recv(__fd__,__mem__,__len__,__flags__)
#define recvfrom(__fd__,__mem__,__len__,__flags__,__from__,__fromlen__) libc_recvfrom(__fd__,__mem__,__len__,__flags__,__from__,__fromlen__)
#define send(__fd__,__dataptr__,__size__,__flags__) libc_send(__fd__,__dataptr__,__size__,__flags__)
#define sendto(__fd__,__dataptr__,__size__,__flags__,__to__,__tolen__) libc_sendto(__fd__,__dataptr__,__size__,__flags__,__to__,__tolen__)
#define shutdown(__fd__,__how__) libc_shutdown(__fd__,__how__)
#define getpeername(__fd__,__name__,__namelen__) libc_getpeername(__fd__,__name__,__namelen__)
#define getsockname(__fd__,__name__,__namelen__) libc_getsockname(__fd__,__name__,__namelen__)
#define getsockopt(__fd__,__level__,__optname__,__optval__,__optlen__) libc_getsockopt(__fd__,__level__,__optname__,__optval__,__optlen__)
#define setsockopt(__fd__,__level__,__optname__,__optval__,__optlen__) libc_setsockopt(__fd__,__level__,__optname__,__optval__,__optlen__)


#endif