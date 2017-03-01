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


/*============================================
| Compiler Directive
==============================================*/
#ifndef __STDINT_H__
#define __STDINT_H__


/*============================================
| Includes
==============================================*/
#include "kernel/core/types.h"
#include "kernel/core/kernelconf.h"

/*============================================
| Declaration
==============================================*/
#ifndef CHAR_BIT
   #define CHAR_BIT      8         /* number of bits in a char */
#endif

#ifndef SCHAR_MIN
   #define SCHAR_MIN   (-128)      /* minimum signed char value */
#endif

#ifndef SCHAR_MAX
   #define SCHAR_MAX     127       /* maximum signed char value */
#endif

#ifndef UCHAR_MAX
   #define UCHAR_MAX     0xff      /* maximum unsigned char value */
#endif

#ifndef _CHAR_UNSIGNED
   #ifndef CHAR_MIN
      #define CHAR_MIN    SCHAR_MIN /* mimimum char value */
   #endif
   #ifndef CHAR_MAX
      #define CHAR_MAX    SCHAR_MAX /* maximum char value */
   #endif
#else
   #ifndef CHAR_MIN
      #define CHAR_MIN      0
   #endif
   #ifndef CHAR_MAX
      #define CHAR_MAX    UCHAR_MAX
   #endif
#endif  /* _CHAR_UNSIGNED */

#if __KERNEL_CPU_ARCH == CPU_ARCH_16
   #ifndef SHRT_MIN
      #define SHRT_MIN    (-32768)           /* minimum (signed) short value */
   #endif
   #ifndef SHRT_MAX
      #define SHRT_MAX      (32767)          /* maximum (signed) short value */
   #endif
   #ifndef USHRT_MAX
      #define USHRT_MAX     (0xffff)         /* maximum unsigned short value */
   #endif
   #ifndef INT_MIN
      #define INT_MIN     (-32767 - 1)       /* minimum (signed) int value */
   #endif
   #ifndef INT_MAX
      #define INT_MAX       (32767)          /* maximum (signed) int value */
   #endif
   #ifndef UINT_MAX
      #define UINT_MAX      0xffffffff       /* maximum unsigned int value */
   #endif
   #ifndef LONG_MIN
      #define LONG_MIN    (-2147483647L - 1) /* minimum (signed) long value */
   #endif
   #ifndef LONG_MAX
      #define LONG_MAX      (2147483647L)    /* maximum (signed) long value */
   #endif
   #ifndef ULONG_MAX
      #define ULONG_MAX     (0xffffffffUL)   /* maximum unsigned long value */
   #endif
#else
   #ifndef SHRT_MIN
      #define SHRT_MIN    (-32768)           /* minimum (signed) short value */
   #endif
   #ifndef SHRT_MAX
      #define SHRT_MAX      (32767)          /* maximum (signed) short value */
   #endif
   #ifndef USHRT_MAX
      #define USHRT_MAX     (0xffff)         /* maximum unsigned short value */
   #endif
   #ifndef INT_MIN
      #define INT_MIN     (-2147483647 - 1)  /* minimum (signed) int value */
   #endif
   #ifndef INT_MAX
      #define INT_MAX       (2147483647)     /* maximum (signed) int value */
   #endif
   #ifndef UINT_MAX
      #define UINT_MAX      (0xffffffff)     /* maximum unsigned int value */
   #endif
   #ifndef LONG_MIN
      #define LONG_MIN    (-2147483647L - 1) /* minimum (signed) long value */
   #endif
   #ifndef LONG_MAX
      #define LONG_MAX      (2147483647L)    /* maximum (signed) long value */
   #endif
   #ifndef ULONG_MAX
      #define ULONG_MAX     (0xffffffffUL)   /* maximum unsigned long value */
   #endif
#endif

#ifndef __KERNEL_COMPILER_STDINT_INCLUDED__

   typedef short int int_least8_t;

   typedef int int_least16_t;
   typedef long int int_least32_t;

   typedef unsigned short int uint_least8_t;
   typedef unsigned int uint_least16_t;
   typedef unsigned long int uint_least32_t;

   typedef short int int_fast8_t;
   typedef int int_fast16_t;
   typedef long int int_fast32_t;

   typedef unsigned short int uint_fast8_t;
   typedef unsigned int uint_fast16_t;
   typedef unsigned long int uint_fast32_t;


   #if (__KERNEL_COMPILER_SUPPORT_TYPE>__KERNEL_COMPILER_SUPPORT_32_BITS_TYPE)

   typedef long long int int_least64_t;
   typedef long long int int64_t;
   typedef unsigned long long int uint64_t;
   typedef unsigned long long int uint_least64_t;
   typedef long long int int_fast64_t;
   typedef unsigned long long int uint_fast64_t;

   typedef int64_t intptr_t;
   typedef uint64_t uintptr_t;
   typedef int64_t intmax_t;
   typedef uint64_t uintmax_t;

   #else

   typedef int32_t intptr_t;
   typedef uint32_t uintptr_t;
   typedef int32_t intmax_t;
   typedef uint32_t uintmax_t;

   #endif

   //from $(TOOLCHAIN)/lib/gcc/arm-elf/4.3.3/include/stddef.h
   #if defined (__PTRDIFF_TYPE__)
      #undef __PTRDIFF_TYPE__
      #define __PTRDIFF_TYPE__   int
   typedef __PTRDIFF_TYPE__ ptrdiff_t;
   #endif
   
#endif //__KERNEL_COMPILER_STDINT_INCLUDED__

#endif
