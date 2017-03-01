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


/*===========================================
Includes
=============================================*/
#include <stdint.h>

#include "lib/libc/ctype/ctype.h"

/*===========================================
Global Declaration
=============================================*/


/*===========================================
Implementation
=============================================*/

#ifndef __KERNEL_UCORE_ECOS

int __lepton_libc_isdigit ( int ch ) {
   return (unsigned int)(ch - '0') < 10u;
}

//
int __lepton_libc_isalnum ( int ch ) {
   return (unsigned int)((ch | 0x20) - 'a') < 26u  ||
          (unsigned int)( ch         - '0') < 10u;
}

//
int __lepton_libc_isalpha ( int ch ) {
   return (unsigned int)((ch | 0x20) - 'a') < 26u;
}

//
int __lepton_libc_isblank ( int ch ) {
   return ch == ' '  ||  ch == '\t';
}

//
int __lepton_libc_iscntrl ( int ch ) {
   return (unsigned int)ch < 32u  ||  ch == 127;
}

//
int __lepton_libc_isgraph ( int ch ) {
   return (unsigned int)(ch - '!') < 127u - '!';
}

//
int __lepton_libc_islower ( int ch ) {
   return (unsigned int) (ch - 'a') < 26u;
}

//
int __lepton_libc_isprint ( int ch ) {
   return (unsigned int)(ch - ' ') < 127u - ' ';
}

//
int __lepton_libc_isspace ( int ch ) {
   return (unsigned int)(ch - 9) < 5u  ||  ch == ' ';
}

//
int __lepton_libc_ispunct ( int ch ) {
   return __lepton_libc_isprint (ch)  &&  !__lepton_libc_isalnum (ch)  &&  !__lepton_libc_isspace (ch);
}


//
int __lepton_libc_isupper ( int ch ) {
   return (unsigned int)(ch - 'A') < 26u;
}

//
int __lepton_libc_isxdigit ( int ch ) {
   return (unsigned int)( ch         - '0') < 10u  ||
          (unsigned int)((ch | 0x20) - 'a') <  6u;
}

//
int __lepton_libc_tolower (int ch) {
   if ( (unsigned int)(ch - 'A') < 26u )
      ch += 'a' - 'A';
   return ch;
}

//
int __lepton_libc_toupper (int ch) {
   if ( (unsigned int)(ch - 'a') < 26u )
      ch += 'A' - 'a';
   return ch;
}
#endif

//
int __lepton_libc_isascii ( int ch ) {
   return (unsigned int)ch < 128u;
}


/*===========================================
End of Source ctype.c
=============================================*/
