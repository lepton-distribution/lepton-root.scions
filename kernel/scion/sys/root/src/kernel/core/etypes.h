/*
The contents of this file are subject to the Mozilla Public License Version 1.1
(the "License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at http://www.mozilla.org/MPL/

Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for the
specific language governing rights and limitations under the License.

The Original Code is Lepton.

The Initial Developer of the Original Code is Chauvin-Arnoux.
Portions created by Chauvin-Arnoux are Copyright (C) 2011. All Rights Reserved.

Alternatively, the contents of this file may be used under the terms of the eCos GPL license
(the  [eCos GPL] License), in which case the provisions of [eCos GPL] License are applicable
instead of those above. If you wish to allow use of your version of this file only under the
terms of the [eCos GPL] License and not to allow others to use your version of this file under
the MPL, indicate your decision by deleting  the provisions above and replace
them with the notice and other provisions required by the [eCos GPL] License.
If you do not delete the provisions above, a recipient may use your version of this file under
either the MPL or the [eCos GPL] License."
*/

#ifndef __ETYPES_H__
#define __ETYPES_H__

/*===========================================
Includes
=============================================*/
#include "kernel/core/kernelconf.h"

/*===========================================
Declaration
=============================================*/
//embedded types
#ifndef __KERNEL_COMPILER_STDINT_INCLUDED__

   typedef signed char schar8_t;
   typedef unsigned char uint8_t;
   typedef uint8_t char8_t;

   typedef signed char int8_t;
   typedef unsigned char uint8_t;


   typedef short int int16_t;

   typedef unsigned short int uint16_t;
   #if (__KERNEL_CPU_ARCH == CPU_ARCH_16)
   typedef long  int          int32_t;
   typedef unsigned long int uint32_t;
   #elif (__KERNEL_CPU_ARCH == CPU_ARCH_32)
   typedef int                int32_t;
   typedef unsigned int       uint32_t;
   #endif

   #if (__KERNEL_COMPILER_SUPPORT_TYPE>__KERNEL_COMPILER_SUPPORT_32_BITS_TYPE)
   typedef signed long long int64_t;
   typedef unsigned long long uint64_t;
   #else 
   typedef signed long int64_t;
   typedef unsigned long uint64_t;
   #endif
   
#endif  //__KERNEL_COMPILER_STDINT_INCLUDED__
   
//  
typedef float float32_t;
   
#endif
