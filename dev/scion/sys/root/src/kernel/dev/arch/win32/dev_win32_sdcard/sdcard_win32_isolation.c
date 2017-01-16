/*
The contents of this file are subject to the Mozilla Public License Version 1.1
(the "License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at http://www.mozilla.org/MPL/

Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for the
specific language governing rights and limitations under the License.

The Original Code is Lepton.

The Initial Developer of the Original Code is Philippe Le Boulanger.
Portions created by Philippe Le Boulanger are Copyright (C) 2011. All Rights Reserved.

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

#include <windows.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "kernel/dev/arch/win32/dev_win32_sdcard/dev_win32_sdcard.h"
#include "kernel/dev/arch/win32/dev_win32_sdcard/sdcard_win32_isolation.h"

/*===========================================
Global Declaration
=============================================*/
static char* pmemory;
static long memory_size = DFLT_SDCARD_MEMORYSIZE;
static int fh = -1;

static const char memory[DFLT_SDCARD_MEMORYSIZE] = { 0 };
static int instance_counter = 0;

static char memory_file_path[MAX_PATH] = { 0 };

#define __sdcard_size() DFLT_SDCARD_MEMORYSIZE

/*===========================================
Implementation
=============================================*/

/*-------------------------------------------
| Name:sdcard_win32_isolation_open
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int sdcard_win32_isolation_open(const char* path) {
	//
	if (fh == -1) {

		if ((fh = _open(path, _O_RDWR | _O_CREAT | _O_EXCL | _O_BINARY, _S_IREAD | _S_IWRITE)) == -1) {
			DWORD dwError = GetLastError();

			if (dwError != ERROR_FILE_EXISTS)
				return -1;

			if ((fh = _open(path, _O_RDWR | _O_BINARY, _S_IREAD | _S_IWRITE)) == -1)
				return -1;

			_lseek(fh, 0, SEEK_SET);
         //
         strcpy(memory_file_path, path);
         //
		}
		else {
			int w = 0;

			_close(fh);
			if ((fh = _open(path, _O_RDWR | _O_TRUNC | _O_EXCL | _O_BINARY, _S_IREAD | _S_IWRITE)) == -1)
				return -1;

			_lseek(fh, 0, SEEK_SET);
			_lseek(fh, memory_size, SEEK_SET);
			_write(fh, memory, 4);
         //
         strcpy(memory_file_path, path);
		}
	}
	//
	instance_counter++;
	//
	return 0;
}

/*-------------------------------------------
| Name:sdcard_win32_isolation_close
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int sdcard_win32_isolation_close(void) {
	//
	if (fh == -1)
		return -1;
	//
	instance_counter--;
	//
	if (instance_counter<0) {
		instance_counter = 0;
		_close(fh);
		fh = -1;
	}
	//
	return 0;
}

/*------------------------------------------ -
| Name: sdcard_win32_isolation_write
| Description :
| Parameters :
| Return Type :
| Comments :
| See :
-------------------------------------------- - */
int sdcard_win32_isolation_write(int* offset, const char* buf, int size) {
	int w;
	//
	if (*offset >= memory_size)
		return -1;
	//
	_lseek(fh, *offset, SEEK_SET);
	//
	w = _write(fh, buf, size);
	//_commit(fh);
	*offset = _lseek(fh, 0, SEEK_CUR);
	//
	return w;
}

/*------------------------------------------ -
| Name: sdcard_win32_isolation_read
| Description :
| Parameters :
| Return Type :
| Comments :
| See :
-------------------------------------------- - */
int sdcard_win32_isolation_read(int* offset, char* buf, int size) {
	int r;
	//
	if (*offset >= memory_size)
		return -1;
	//
	_lseek(fh, *offset, SEEK_SET);
	//
	r = _read(fh, buf, size);
	//
	*offset = _lseek(fh, 0, SEEK_CUR);
	//to remove : test
	//printf("<- read offset=%d\n",ofile_lst[desc].offset);
	return r;
}

/*------------------------------------------ -
| Name: sdcard_win32_isolation_getsize
| Description :
| Parameters :
| Return Type :
| Comments :
| See :
-------------------------------------------- - */
int sdcard_win32_isolation_getsize(void) {
	return memory_size;
}

/*------------------------------------------ -
| Name: sdcard_win32_isolation_setsize
| Description :
| Parameters :
| Return Type :
| Comments :
| See :
-------------------------------------------- - */
int sdcard_win32_isolation_setsize(int size) {
	int w;
	//
	_lseek(fh, 0, SEEK_SET);
	//
	memory_size = size;
	//
	if ((w = _lseek(fh, memory_size, SEEK_SET)) == -1)
		printf("sdcard0 creation failed");
	else
		printf("sdcard0 creation size %u bytes ok.\n", w);
	//
	return w;
}

/*------------------------------------------ -
| Name: sdcard_win32_isolation_erase
| Description :
| Parameters :
| Return Type :
| Comments :
| See :
-------------------------------------------- - */
int sdcard_win32_isolation_erase(void) {
	_close(fh);
	//
	if ((fh = _open(memory_file_path, _O_RDWR | _O_TRUNC | _O_EXCL | _O_BINARY, _S_IREAD | _S_IWRITE)) == -1)
		return -1;
	//
	_lseek(fh, 0, SEEK_SET);
	_lseek(fh, memory_size, SEEK_SET);
	_write(fh, memory, 4);
	_lseek(fh, 0, SEEK_SET);
	//
	return 0;
}
/*============================================
| End of Source  : win32_isolation_file.c
==============================================*/