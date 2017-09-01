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
#ifndef __MODEM_CORE_API_H__
#define __MODEM_CORE_API_H__


/*============================================
| Includes
==============================================*/

/*============================================
| Declaration
==============================================*/
int modem_core_api_socket(desc_t desc,int domain, int type, int protocol);
int modem_core_api_bind(int modem_core_connexion_index, struct sockaddr *name, socklen_t namelen);
int modem_core_api_accept(int modem_core_connexion_index, struct sockaddr *addr, socklen_t *addrlen);
int modem_core_api_accepted(desc_t desc,int modem_core_connexion_index);
int modem_core_api_connect(int modem_core_connexion_index, struct sockaddr *name, socklen_t namelen);
int modem_core_api_listen(int modem_core_connexion_index, int backlog);
int modem_core_api_shutdown(int modem_core_connexion_index, int how);
int modem_core_api_close(int modem_core_connexion_index);
int modem_core_api_getpeername(int modem_core_connexion_index, struct sockaddr *name, socklen_t *namelen);
int modem_core_api_getsockname(int modem_core_connexion_index, struct sockaddr *name, socklen_t *namelen);
int modem_core_api_getsockopt(int modem_core_connexion_index, int level, int optname, void *optval, socklen_t *optlen);
int modem_core_api_setsockopt(int modem_core_connexion_index, int level, int optname, const void *optval, socklen_t optlen);
int modem_core_api_ioctl(int modem_core_connexion_index, long cmd, void *argp);
struct hostent* modem_core_api_gethostbyname(struct hostent* host, const char *name);

int modem_core_api_isset_read(int modem_core_connexion_index);
int modem_core_api_isset_write(int modem_core_connexion_index);
int modem_core_api_read(int modem_core_connexion_index,char* buf, int size);
int modem_core_api_write(int modem_core_connexion_index,char* buf, int len);




#endif
