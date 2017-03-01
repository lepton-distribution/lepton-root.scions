/*
The contents of this file are subject to the Mozilla Public License Version 1.1
(the "License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at http://www.mozilla.org/MPL/

Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for the
specific language governing rights and limitations under the License.

The Original Code is Lepton.

The Initial Developer of the Original Code is Philippe Le Boulanger.
Portions created by Philippe Le Boulanger are Copyright (C) 2017 <lepton.phlb@gmail.com>.
All Rights Reserved.

Contributor(s): Jean-Jacques Pitrolle <lepton.jjp@gmail.com>.

Alternatively, the contents of this file may be used under the terms of the eCos GPL license
(the  [eCos GPL] License), in which case the provisions of [eCos GPL] License are applicable
instead of those above. If you wish to allow use of your version of this file only under the
terms of the [eCos GPL] License and not to allow others to use your version of this file under
the MPL, indicate your decision by deleting  the provisions above and replace
them with the notice and other provisions required by the [eCos GPL] License.
If you do not delete the provisions above, a recipient may use your version of this file under
either the MPL or the [eCos GPL] License."
*/
#ifndef __FATFSCORE_H__
#define __FATFSCORE_H__



/*===========================================
Includes
=============================================*/


/*===========================================
Declaration
=============================================*/

typedef struct fatfs_dirent_st{
   char d_name[__KERNEL_FILENAME_MAX]; 
   inodenb_t inodenb;
}fatfs_dirent_t;


int fatfscore_readfs(mntdev_t* pmntdev);

//
int fatfscore_extern_create(desc_t desc, const char* path, mode_t mode);
int fatfscore_extern_open(desc_t desc, const char* path, int oflag, mode_t mode);
int fatfscore_extern_close(desc_t desc);
int fatfscore_extern_read(desc_t desc, char* buf, int size);
int fatfscore_extern_write(desc_t desc, const char* buf, int size);
int fatfscore_extern_seek(desc_t desc, off_t offset, int origin);
int fatfscore_extern_truncate(desc_t desc, off_t length);
int fatfscore_extern_sync(desc_t desc);
//
int fatfscore_extern_mkdir(const char* path, mode_t mode);			
int fatfscore_extern_opendir(desc_t desc, const char* path);
int fatfscore_extern_closedir(desc_t desc);
int fatfscore_extern_readdir(desc_t desc, fatfs_dirent_t* pdirent);
int fatfscore_extern_telldir(desc_t desc);
int fatfscore_extern_seekdir(desc_t desc, long loc);
//
int fatfscore_extern_unlink(const char* path);
int fatfscore_extern_rename(const char*  old_name, char* new_name);
int fatfscore_extern_stat(const char* path, struct stat * stat);

#endif


