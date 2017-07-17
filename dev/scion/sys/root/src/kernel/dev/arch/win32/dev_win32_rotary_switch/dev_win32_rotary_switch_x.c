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
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "kernel/core/types.h"
#include "kernel/core/interrupt.h"
#include "kernel/core/system.h"
#include "kernel/core/stat.h"
#include "kernel/core/fcntl.h"

#include "kernel/fs/vfs/vfsdev.h"
#include "kernel/fs/vfs/vfstypes.h"

#include "kernel/core/ioctl.h"
#include "kernel/core/ioctl_encoder.h"
#include "kernel/core/kernel_ring_buffer.h"

#include "kernel/dev/arch/win32/dev_win32_rotary_switch/dev_win32_rotary_switch_x.h"

/*===========================================
Global Declaration
=============================================*/


/*===========================================
Implementation
=============================================*/
#if defined (ROTARY_SWITCH_PIPE_OVERLAPPED)
VOID CALLBACK FileIOCompletionRoutine(
   __in  DWORD dwErrorCode,
   __in  DWORD dwNumberOfBytesTransfered,
   __in  LPOVERLAPPED lpOverlapped
);

VOID CALLBACK FileIOCompletionRoutine(
   __in  DWORD dwErrorCode,
   __in  DWORD dwNumberOfBytesTransfered,
   __in  LPOVERLAPPED lpOverlapped)
{
   printf(TEXT("Error code:\t%x\n"), dwErrorCode);
   printf(TEXT("Number of bytes:\t%x\n"), dwNumberOfBytesTransfered);
}
#endif
/*-------------------------------------------
| Name:dev_win32_rotary_x_thread
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static void dev_win32_rotary_x_thread(LPVOID lpvParam) {
   rotary_switch_info_t* rotary_switch_info = (rotary_switch_info_t*)lpvParam;
   
   for(;;){
      DWORD dwError;
      DWORD dwBytesAvaiblable;
      DWORD dwBytesRead = 0;
      BOOL bFireInterrupt = FALSE;
      char data[16];
      // to do: wait on semaphore
#if defined (ROTARY_SWITCH_PIPE_OVERLAPPED)
      if ((dwError = WaitForSingleObject(rotary_switch_info->hRotarySwitchNamedPipe, INFINITE))!= WAIT_OBJECT_0) {
         ExitThread(-1);
      }
      BOOL fSuccess = GetOverlappedResult(rotary_switch_info->hRotarySwitchNamedPipe,&rotary_switch_info->structOverlappedNamedPipe,&dwBytesAvaiblable,FALSE);    
#endif
      //
      PeekNamedPipe(rotary_switch_info->hRotarySwitchNamedPipe,NULL,0,NULL,&dwBytesAvaiblable,NULL);
      //
#if defined (ROTARY_SWITCH_PIPE_OVERLAPPED)
      //BOOL result = ReadFile(rotary_switch_info->hRotarySwitchNamedPipe,data,sizeof(data),&_numBytesRead,&rotary_switch_info->structOverlappedNamedPipe);
      BOOL result = ReadFileEx(rotary_switch_info->hRotarySwitchNamedPipe, data, sizeof(data), &rotary_switch_info->structOverlappedNamedPipe, FileIOCompletionRoutine);
#else
      BOOL result = ReadFile(rotary_switch_info->hRotarySwitchNamedPipe, data, sizeof(data), &dwBytesRead, NULL);
#endif
      if (!result) {
         dwError = GetLastError();
      }
     
      // interrupt
      if (rotary_switch_info->desc_r == INVALID_DESC) {
         continue;
      }
#ifdef USE_ROTARY_ENCODER_KERNEL_RING_BUFFER
      if (__kernel_ring_buffer_is_empty(rotary_switch_info->krb_ring_buffer_info)) {
         bFireInterrupt = TRUE;
      }
      //
      kernel_ring_buffer_write(&rotary_switch_info->krb_ring_buffer_info, (uint8_t*)data, dwBytesRead);
#else
      if (rotary_switch_info->input_r == rotary_switch_info->input_w) {
         rotary_switch_info->input_w++;
         bFireInterrupt = TRUE;
      }

      for (DWORD i = 0; i < dwBytesRead; i++) {
            if (rotary_switch_info->counter_limit_max != rotary_switch_info->counter_limit_min) {
               if (data[i] > 0 && rotary_switch_info->counter < rotary_switch_info->counter_limit_max) {
                  rotary_switch_info->counter+=rotary_switch_info->counter_step;
               }else if (data[i] < 0 && rotary_switch_info->counter > rotary_switch_info->counter_limit_min) {
                  rotary_switch_info->counter-=rotary_switch_info->counter_step;
               }
            }else {
               if (data[i] > 0) {
                  rotary_switch_info->counter+=rotary_switch_info->counter_step;
               }
               else if (data[i] < 0) {
                  rotary_switch_info->counter-=rotary_switch_info->counter_step;
               }
            }
            
      }//end for
#endif
      //
      if(bFireInterrupt)
         emuFireInterrupt(rotary_switch_info->interrupt_no);//to do: verify interrupt no
   }
   //
   ExitThread(0);
}

/*-------------------------------------------
| Name:dev_rotary_switch_x_interrupt
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_rotary_switch_x_interrupt(rotary_switch_info_t* rotary_switch_info) {
   __hw_enter_interrupt();
   //
#ifdef USE_ROTARY_ENCODER_KERNEL_RING_BUFFER
   if ( rotary_switch_info->desc_r != INVALID_DESC 
      && __kernel_ring_buffer_is_not_empty(rotary_switch_info->krb_ring_buffer_info)) {
      __fire_io_int(ofile_lst[rotary_switch_info->desc_r].owner_pthread_ptr_read);
   }
#else
   if (rotary_switch_info->desc_r != INVALID_DESC
      && rotary_switch_info->input_r != rotary_switch_info->input_w) {
      __fire_io_int(ofile_lst[rotary_switch_info->desc_r].owner_pthread_ptr_read);
   }
#endif
   //
   __hw_leave_interrupt();
   return 0;
}

/*-------------------------------------------
| Name:dev_win32_rotary_switch_x_load
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_win32_rotary_switch_x_load(rotary_switch_info_t* rotary_switch_info) {
   BOOL bSucceed;
   DWORD dwError;
   DWORD dwMode;
   char name[MAX_PATH] = { 0 };
   
   //
   memset(&rotary_switch_info->structOverlappedNamedPipe, 0, sizeof(rotary_switch_info->structOverlappedNamedPipe));
   //
   rotary_switch_info->desc_r = INVALID_DESC;
   //
#ifdef USE_ROTARY_ENCODER_KERNEL_RING_BUFFER
   kernel_ring_buffer_init(&rotary_switch_info->krb_ring_buffer_info, rotary_switch_info->buffer_read, sizeof(rotary_switch_info->buffer_read));
#endif
   //
   strcpy(name, rotary_switch_info->rotary_switch_windows_form_name);
   strcat(name, ".sem");
   //
   rotary_switch_info->hRotarySwitchSemaphore = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, TRUE, name);
   if (rotary_switch_info->hRotarySwitchSemaphore == INVALID_HANDLE_VALUE) {
      dwError = GetLastError();
      return -1;
   }
   //
   strcpy(name, rotary_switch_info->rotary_switch_windows_form_name);
   strcat(name, ".mmap");
   //
   rotary_switch_info->hRotarySwitchMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, name);
   if (rotary_switch_info->hRotarySwitchMapFile == INVALID_HANDLE_VALUE) {
      dwError = GetLastError();
      return -1;
   }
   //
   strcpy(name,TEXT("\\\\.\\pipe\\"));
   strcat(name, rotary_switch_info->rotary_switch_windows_form_name);
   strcat(name, ".pipe");
   //
#if defined (ROTARY_SWITCH_PIPE_OVERLAPPED)
   rotary_switch_info->hRotarySwitchNamedPipe = CreateFile(name, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
#else
   rotary_switch_info->hRotarySwitchNamedPipe = CreateFile(name,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING, 0,NULL);
#endif
   if (rotary_switch_info->hRotarySwitchNamedPipe == INVALID_HANDLE_VALUE) {
      dwError = GetLastError();
      return -1;
   }
#if 0
   // All pipe instances are busy, so wait for 20 seconds.
   if (!WaitNamedPipe(name, 20000)) {
      dwError = GetLastError();
      return -1;
   }
#endif
   // The pipe connected; change to message-read mode.
   dwMode = PIPE_READMODE_BYTE;
   bSucceed = SetNamedPipeHandleState(rotary_switch_info->hRotarySwitchNamedPipe, &dwMode,NULL,NULL);
   if (!bSucceed) {
      dwError = GetLastError();
      return -1;
   }
   //
   rotary_switch_info->hRotarySwitchThread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)dev_win32_rotary_x_thread,(LPVOID)rotary_switch_info,0,&rotary_switch_info->dwRotarySwitchThreadId);      
   if (rotary_switch_info->hRotarySwitchThread == INVALID_HANDLE_VALUE) {
      dwError = GetLastError();
      return -1;
   }
   //
   return 0;
}

/*-------------------------------------------
| Name:dev_win32_rotary_switch_x_open
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_win32_rotary_switch_x_open(desc_t desc, int o_flag, rotary_switch_info_t* rotary_switch_info) {

   //
   if (o_flag & O_RDONLY) {
      if (rotary_switch_info->desc_r != INVALID_DESC) {
         return -1; //already open
      }
      //
      rotary_switch_info->desc_r = desc;
      rotary_switch_info->input_r = 0;
      rotary_switch_info->input_w = 0;
      //
      rotary_switch_info->counter = 0;
      rotary_switch_info->counter_limit_min = 0;
      rotary_switch_info->counter_limit_max = 0;
      rotary_switch_info->counter_step = 1;

      ofile_lst[desc].p = rotary_switch_info;
   }

   //
   if (o_flag & O_WRONLY) {
      return -1;
   }

   ofile_lst[desc].offset = 0;

   return 0;
}

/*-------------------------------------------
| Name:dev_win32_rotary_switch_x_close
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_win32_rotary_switch_x_close(desc_t desc) {
   if (ofile_lst[desc].oflag & O_RDONLY) {
      if (!ofile_lst[desc].nb_writer) {
         rotary_switch_info_t* rotary_switch_info = (rotary_switch_info_t*)ofile_lst[desc].p;
         //
         if (rotary_switch_info->hRotarySwitchNamedPipe != INVALID_HANDLE_VALUE) {
            FlushFileBuffers(rotary_switch_info->hRotarySwitchNamedPipe);
            DisconnectNamedPipe(rotary_switch_info->hRotarySwitchNamedPipe);
            CloseHandle(rotary_switch_info->hRotarySwitchNamedPipe);
         }
         //
         rotary_switch_info->desc_r = INVALID_DESC;
      }
   }

   return 0;
}

/*-------------------------------------------
| Name:dev_win32_rotary_switch_x_isset_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_win32_rotary_switch_x_isset_read(desc_t desc) {
   rotary_switch_info_t* rotary_switch_info = (rotary_switch_info_t*)ofile_lst[desc].p;
   //
#ifdef USE_ROTARY_ENCODER_KERNEL_RING_BUFFER
   if (__kernel_ring_buffer_is_not_empty(rotary_switch_info->krb_ring_buffer_info)) {
      return 0;
   }
#else
   if (rotary_switch_info->input_r != rotary_switch_info->input_w) {
      return 0;
   }
#endif
   //
   return -1;
}

/*-------------------------------------------
| Name:dev_win32_rotary_switch_x_isset_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_win32_rotary_switch_x_isset_write(desc_t desc) {
   return -1;
}

/*-------------------------------------------
| Name:dev_win32_rotary_switch_x_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_win32_rotary_switch_x_read(desc_t desc, char* buf, int size) {
   int cb;
   rotary_switch_info_t* rotary_switch_info = (rotary_switch_info_t*)ofile_lst[desc].p;
   //
#ifdef USE_ROTARY_ENCODER_KERNEL_RING_BUFFER
   cb = kernel_ring_buffer_read(&rotary_switch_info->krb_ring_buffer_info, (uint8_t*)buf, size);
#else
   //
   int32_t counter = 0;
   //
   if (size<sizeof(rotary_switch_info->counter))
      return -1;
   //
   counter = rotary_switch_info->counter;
   //
   rotary_switch_info->input_r = rotary_switch_info->input_w;
   //
   memcpy(buf, &counter, sizeof(counter));
   //
   cb= sizeof(counter);
#endif
   //
   return cb;
}

/*-------------------------------------------
| Name:dev_win32_rotary_switch_x_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_win32_rotary_switch_x_write(desc_t desc, const char* buf, int size) {
   return -1;
}


/*--------------------------------------------
| Name:dev_win32_rotary_switch_x_ioctl
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int dev_win32_rotary_switch_x_ioctl(desc_t desc, int request, va_list ap) {
   rotary_switch_info_t* rotary_switch_info = (rotary_switch_info_t*)ofile_lst[desc].p;
   switch (request) {

      case I_LINK: {
      }
      break;

      //
      case I_UNLINK: {
      }
      break;

                     
      case ENCODERSETCOUNTER: {
         int32_t counter = va_arg(ap, int32_t);
         rotary_switch_info->counter = counter;
      }
      break;
      //
      case ENCODERGETCOUNTER: {
         int32_t* p_counter = va_arg(ap, int32_t*);
         *p_counter = rotary_switch_info->counter;
      }
      break;
      //
      case ENCODERSETCOUNTERLIMIT: {
         int32_t counter_limit_min = va_arg(ap, int32_t);
         int32_t counter_limit_max = va_arg(ap, int32_t);
         //
         rotary_switch_info->counter_limit_min = counter_limit_min;
         rotary_switch_info->counter_limit_max = counter_limit_max;
      }
      break;

      case ENCODERSETCOUNTERSTEP: {
         int32_t counter_step = va_arg(ap, int32_t);
         //
         rotary_switch_info->counter_step = counter_step;
      }
      break;
      //
      default:
      return -1;
   }
   //
   return 0;
}


/*============================================
| End of Source  : dev_win32_rotary_switch_x.c
==============================================*/
