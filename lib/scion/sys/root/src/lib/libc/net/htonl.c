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
#include <string.h>

#include "kernel/core/kernelconf.h"
#include "kernel/core/kernel_compiler.h"
#include "kernel/core/limits.h"
#include "kernel/core/dirent.h"
#include "kernel/core/errno.h"
#include "kernel/core/system.h"
#include "kernel/core/process.h"
#include "kernel/core/kernel.h"
#include "kernel/core/fcntl.h"
#include "kernel/core/stat.h"


/*============================================
| Global Declaration
==============================================*/

// Make sure we can recognize the endianness.
#if (!defined(__BYTE_ORDER__) || !defined(__ORDER_LITTLE_ENDIAN__) || !defined(__ORDER_BIG_ENDIAN__))
   #error "Unknown platform endianness!"
#endif



/*============================================
| Implementation
==============================================*/
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

/*--------------------------------------------
| Name:        libc_htons
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
uint16_t libc_htons(uint16_t v) {
  return (v >> 8) | (v << 8);
}

/*--------------------------------------------
| Name:        libc_htonl
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
uint32_t libc_htonl(uint32_t v) {
  return libc_htons(v >> 16) | (libc_htons((uint16_t) v) << 16);
}

#else

/*--------------------------------------------
| Name:        libc_htons
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
uint16_t libc_htons(uint16_t v) {
  return v;
}

/*--------------------------------------------
| Name:        libc_htonl
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
uint32_t libc_htonl(uint32_t v) {
  return v;
}

#endif

/*--------------------------------------------
| Name:        libc_ntohs
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
uint16_t libc_ntohs(uint16_t v) {
  return libc_htons(v);
}

/*--------------------------------------------
| Name:        libc_ntohl
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
uint32_t libc_ntohl(uint32_t v) {
  return libc_htonl(v);
}

/*============================================
| End of Source  : htonl.c
==============================================*/