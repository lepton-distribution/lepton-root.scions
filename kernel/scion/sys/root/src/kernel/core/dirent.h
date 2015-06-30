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
#ifndef _DIRENT_H
#define _DIRENT_H


/*===========================================
Includes
=============================================*/
#include "kernel/core/types.h"


/*===========================================
Declaration
=============================================*/
#define NAME_MAX           32 //16

typedef struct dirent {
   ino_t d_ino;        //file serial number
   char d_name[NAME_MAX];      //name of entry
   inodenb_t inodenb;
}dirent_t;


typedef struct ofile_s DIR;

#ifdef __cplusplus
extern "C" {
#endif
int            closedir(DIR *);
DIR           *opendir(char *);
struct dirent *readdir(DIR *);
struct dirent *readdir2(DIR *,struct dirent *);
int            readdir_r(DIR *, struct dirent *, struct dirent **);
void           rewinddir(DIR *);
void           seekdir(DIR *,int);
int            telldir(DIR *);

#ifdef __cplusplus
}
#endif


#endif
