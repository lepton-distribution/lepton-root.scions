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
#ifndef __LIB_LIBC_NET_NETDB_H__
#define __LIB_LIBC_NET_NETDB_H__


/*============================================
| Includes
==============================================*/

#if defined (__KERNEL_NET_IPSTACK) && defined(USE_LWIP)
   #include "kernel/net/lwip/include/lwip/netdb.h"
#elif defined (__KERNEL_NET_IPSTACK) && defined(USE_UIP)

#elif defined (__KERNEL_NET_IPSTACK) && defined(USE_MODEMIP)
   #include "kernel/core/net/bsd/netdb.h"
#else
    #include "kernel/core/net/bsd/netdb.h"
#endif

/*============================================
| Declaration
==============================================*/


#if defined (__KERNEL_NET_IPSTACK) && defined(USE_LWIP)
   #define libc_hostent hostent
   struct libc_hostent* libc_gethostbyname(const char *name);
#elif defined(__KERNEL_NET_IPSTACK) && defined(USE_MODEMIP)
   #define libc_hostent hostent
   struct libc_hostent* libc_gethostbyname(const char *name);
#else
   #define libc_hostent hostent
   struct libc_hostent* libc_gethostbyname(const char *name);
#endif

#define gethostbyname(__name__) libc_gethostbyname(__name__)


#endif
