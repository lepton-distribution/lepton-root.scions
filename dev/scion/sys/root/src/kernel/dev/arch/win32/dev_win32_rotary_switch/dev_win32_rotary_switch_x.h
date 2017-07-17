/*
The contents of this file are subject to the Mozilla Public License Version 1.1
(the "License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at http://www.mozilla.org/MPL/

Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for the
specific language governing rights and limitations under the License.

The Original Code is Lepton.

The Initial Developer of the Original Code is Philippe Le Boulanger.
Portions created by Philippe Le Boulanger are Copyright (C) 2016. All Rights Reserved.

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
#ifndef __DEV_WIN32_ROTARY_SWITCH_X_H__
#define __DEV_WIN32_ROTARY_SWITCH_X_H__


/*============================================
| Includes
==============================================*/


/*============================================
| Declaration
==============================================*/
typedef struct rotary_switch_info_st {
   const char* rotary_switch_windows_form_name;
   HANDLE hRotarySwitchSemaphore;
   HANDLE hRotarySwitchMapFile;
   HANDLE hRotarySwitchNamedPipe;
   HANDLE hRotarySwitchThread;
   DWORD dwRotarySwitchThreadId;

   OVERLAPPED structOverlappedNamedPipe;

   desc_t desc_r;
   uint32_t interrupt_no;
   uint8_t input_r;
   uint8_t input_w;
   //
   int32_t counter;
   int32_t counter_limit_min;
   int32_t counter_limit_max;
   int32_t counter_step;
   //
#ifdef USE_ROTARY_ENCODER_KERNEL_RING_BUFFER
   kernel_ring_buffer_t krb_ring_buffer_info;
   uint8_t buffer_read[16];
#endif

}rotary_switch_info_t;

extern int dev_rotary_switch_x_interrupt(rotary_switch_info_t* rotary_switch_info);

extern int dev_win32_rotary_switch_x_load(rotary_switch_info_t* rotary_switch_info);
extern int dev_win32_rotary_switch_x_open(desc_t desc, int o_flag, rotary_switch_info_t* rotary_switch_info);
extern int dev_win32_rotary_switch_x_close(desc_t desc);
extern int dev_win32_rotary_switch_x_isset_read(desc_t desc);
extern int dev_win32_rotary_switch_x_isset_write(desc_t desc);
extern int dev_win32_rotary_switch_x_read(desc_t desc, char* buf, int size);
extern int dev_win32_rotary_switch_x_write(desc_t desc, const char* buf, int size);
extern int dev_win32_rotary_switch_x_ioctl(desc_t desc, int request, va_list ap);


#endif