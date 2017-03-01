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


#ifndef __TYPE_H__
#define __TYPE_H__


/*===========================================
Includes
=============================================*/
#include "kernel/core/etypes.h"
#include "kernel/core/kernelconf.h"

/*===========================================
Declaration
=============================================*/
#ifdef CPU_WIN32
   #pragma warning ( disable : 4005 )
   #pragma warning ( disable : 4142 )
#endif

//process types
typedef int16_t pid_t;

//ino
#if __KERNEL_CPU_ARCH == CPU_ARCH_32
typedef int32_t __ino_t;    //file serial number
#else
typedef int16_t __ino_t;    //file serial number
#endif

#ifdef _INO_T_DEFINED
   #pragma message ("warning: ino_t must be redefined!")
   #define ino_t __ino_t
#else
typedef __ino_t ino_t;
#endif

typedef __ino_t inodenb_t; //inode type

//filesystem types
typedef int16_t blocknb_t;
typedef int16_t desc_t;
typedef int16_t mode_t;
typedef int16_t fstype_t;

typedef int dev_t;

typedef uint32_t fsblkcnt_t;
typedef uint32_t fsfilcnt_t;

//ugly patch for compatiblity IAR ARM7 Compiler :-(
#if __IAR_SYSTEMS_ICC__> 1
   #if !defined(__SIZE_T_TYPE__) || !defined(_SIZE_T) || !defined(_SIZET)
   typedef unsigned int size_t;
//#define __SIZE_T_TYPE__
      #define _SIZE_T
      #define _SIZET
      #define _STD_USING_SIZE_T
   #endif

#else
   #if !defined(_SIZE_T_DEFINED_) && !defined(__size_t)
      typedef unsigned int size_t;
      #define _SIZE_T_DEFINED_
      #define __size_t 1
   #endif
#endif


typedef int32_t ssize_t;

#ifndef _OFF_T_DEFINED
//typedef int16_t __off_t;
   #if (__KERNEL_COMPILER_SUPPORT_TYPE>__KERNEL_COMPILER_SUPPORT_32_BITS_TYPE)
      typedef int64_t off_t;
   #else
      typedef int32_t off_t;
   #endif
   #define _OFF_T_DEFINED
#endif

#ifndef _TIME_T_DEFINED
#ifdef USE_COMPILER_TRACE_DEBUG
   #pragma message ("note: use lepton time_t definition")
#endif
   #if (__KERNEL_COMPILER_SUPPORT_TYPE>__KERNEL_COMPILER_SUPPORT_32_BITS_TYPE)
       //to do time64 bits
      //typedef uint64_t __time_t;
      typedef uint32_t __time_t;
   #else
      typedef uint32_t __time_t;
   #endif

   #define time_t __time_t
   #define _TIME_T_DEFINED
#else 
   #ifdef _USE_32BIT_TIME_T
	 #ifdef USE_COMPILER_TRACE_DEBUG
      #pragma message ("note: use windows time_t definition")
	 #endif
   #else 
	    #ifdef USE_COMPILER_TRACE_DEBUG
      #pragma message ("warning! use windows time_t definition with 64 bits format")
			#endif
   #endif
#endif

typedef uint32_t jiff_t;

typedef uint64_t fd_set;

typedef uint8_t uid_t;
typedef uint8_t gid_t;
typedef uint8_t nlink_t;

typedef uint32_t useconds_t;

//pthread
typedef unsigned long pthread_t;

#endif
