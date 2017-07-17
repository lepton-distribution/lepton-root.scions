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
#ifndef __CTYPE_H__
#define __CTYPE_H__


/*===========================================
Includes
=============================================*/
#include "kernel/core/kernelconf.h"

/*===========================================
Declaration
=============================================*/
//win32 patch
#ifdef CPU_WIN32
   #define _INC_CTYPE
#endif

//
#ifndef __KERNEL_UCORE_ECOS
   //
   #ifndef toascii
      #define toascii(__c__) ((__c__) & 0x7F)
   #endif 
   
   //
   extern int __lepton_libc_isdigit   ( int ch );
   extern int __lepton_libc_isalnum   ( int ch );
   extern int __lepton_libc_isalpha   ( int ch );
   extern int __lepton_libc_isblank   ( int ch );
   extern int __lepton_libc_iscntrl   ( int ch );
   extern int __lepton_libc_isgraph   ( int ch );
   extern int __lepton_libc_islower   ( int ch );
   extern int __lepton_libc_isprint   ( int ch );
   extern int __lepton_libc_isspace   ( int ch );
   extern int __lepton_libc_ispunct   ( int ch );
   extern int __lepton_libc_isupper   ( int ch );
   extern int __lepton_libc_isxdigit  ( int ch );
   extern int __lepton_libc_tolower   ( int ch );
   extern int __lepton_libc_toupper   ( int ch );

   //
   #ifdef isdigit
      #define isdigit(__c__)   __lepton_libc_isdigit(__c__)
   #endif
   
   //
   #ifdef isalnum
      #define isalnum(__c__)   __lepton_libc_isalnum(__c__)
   #endif
   
   //
   #ifdef isalpha
      #define isalpha((__c__))   __lepton_libc_isalpha((__c__))
   #endif
   
   //
   #ifdef isblank
      #define isblank(__c__)   __lepton_libc_isblank(__c__)
   #endif
   
   //
   #ifdef iscntrl
      #define iscntrl(__c__)   __lepton_libc_iscntrl(__c__)
   #endif
   
   //
   #ifdef isgraph
      #define isgraph(__c__)   __lepton_libc_isgraph(__c__)
   #endif
   
   //
   #ifdef islower
      #define islower(__c__)   __lepton_libc_islower(__c__)
   #endif
   
   //
   #ifdef isprint
       #define isprint(__c__)   __lepton_libc_isprint(__c__)
   #endif
  
   //
   #ifdef isspace
      #define isspace(__c__)   __lepton_libc_isspace(__c__)
   #endif
   
   //
   #ifdef ispunct
      #define ispunct(__c__)   __lepton_libc_ispunct(__c__)
   #endif
   
   //
   #ifdef isupper
      #define isupper(__c__)   __lepton_libc_isupper(__c__)
   #endif
   
   //
   #ifdef isxdigit
      #define isxdigit(__c__)   __lepton_libc_isxdigit(__c__)
   #endif
   
   //
   #ifdef tolower
      #define tolower(__c__)   __lepton_libc_tolower(__c__)
   #endif
   
   //
   #ifdef toupper
      #define toupper(__c__)   __lepton_libc_toupper(__c__)
   #endif
   

#endif //ifndef __KERNEL_UCORE_ECOS


//
extern int __lepton_libc_isascii   ( int ch );

#ifndef isascii
   #define isascii(__c__)   __lepton_libc_isascii(__c__)
#endif


//
#endif
