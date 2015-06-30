/*
The contents of this file are subject to the Mozilla Public License Version 1.1
(the "License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at http://www.mozilla.org/MPL/

Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for the
specific language governing rights and limitations under the License.

The Original Code is Lepton.

The Initial Developer of the Original Code is Philippe Le Boulanger.
Portions created by Philippe Le Boulanger are Copyright (C) 2011 <lepton.phlb@gmail.com>.
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




/*===========================================
Includes
=============================================*/

#include "kernel/core/kernelconf.h"
#include "kernel/core/types.h"
#include "kernel/core/interrupt.h"
#include "kernel/core/kernel.h"
#include "kernel/core/system.h"
#include "kernel/core/fcntl.h"
#include "kernel/core/cpu.h"
#include "kernel/fs/vfs/vfsdev.h"
#include "kernel/core/ioctl_board.h"

#include "kernel/dev/arch/at91/asf/sam0/drivers/system/system.h" 


/*===========================================
Global Declaration
=============================================*/
const char dev_at91samd20_xplained_pro_board_name[]="board\0";

static int dev_at91samd20_xplained_pro_board_load(void);
static int dev_at91samd20_xplained_pro_board_open(desc_t desc, int o_flag);
static int dev_at91samd20_xplained_pro_board_close(desc_t desc);
static int dev_at91samd20_xplained_pro_board_isset_read(desc_t desc);
static int dev_at91samd20_xplained_pro_board_isset_write(desc_t desc);
static int dev_at91samd20_xplained_pro_board_read(desc_t desc, char* buf,int size);
static int dev_at91samd20_xplained_pro_board_write(desc_t desc, const char* buf,int size);
static int dev_at91samd20_xplained_pro_board_seek(desc_t desc,int offset,int origin);
static int dev_at91samd20_xplained_pro_board_ioctl(desc_t desc,int request,va_list ap);
 

dev_map_t dev_at91samd20_xplained_pro_board_map={
   dev_at91samd20_xplained_pro_board_name,
   S_IFBLK,
   dev_at91samd20_xplained_pro_board_load,
   dev_at91samd20_xplained_pro_board_open,
   dev_at91samd20_xplained_pro_board_close,
   dev_at91samd20_xplained_pro_board_isset_read,
   dev_at91samd20_xplained_pro_board_isset_write,
   dev_at91samd20_xplained_pro_board_read,
   dev_at91samd20_xplained_pro_board_write,
   dev_at91samd20_xplained_pro_board_seek,
   dev_at91samd20_xplained_pro_board_ioctl //ioctl
};


/*===========================================
Implementation
=============================================*/
unsigned int dev_at91samd20_xplained_pro_board_clock_source_get_hz(void){
   return system_clock_source_get_hz(SYSTEM_CLOCK_SOURCE_DFLL);
}

/*-------------------------------------------
| Name:dev_at91samd20_xplained_pro_board_load
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_at91samd20_xplained_pro_board_load(void){
   /* Initialisation is performed by the Atmel board support package. */
	system_init();
   return 0;
}

/*-------------------------------------------
| Name:dev_at91samd20_xplained_pro_board_open
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_at91samd20_xplained_pro_board_open(desc_t desc, int o_flag){

   //
   if(o_flag & O_RDONLY) {
   }

   if(o_flag & O_WRONLY) {
   }

   ofile_lst[desc].offset=0;

   return 0;
}

/*-------------------------------------------
| Name:dev_at91samd20_xplained_pro_board_close
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_at91samd20_xplained_pro_board_close(desc_t desc){
   return 0;
}

/*-------------------------------------------
| Name:dev_at91samd20_xplained_pro_board_isset_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_at91samd20_xplained_pro_board_isset_read(desc_t desc){
   return -1;
}

/*-------------------------------------------
| Name:dev_at91samd20_xplained_pro_board_isset_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_at91samd20_xplained_pro_board_isset_write(desc_t desc){
   return -1;
}
/*-------------------------------------------
| Name:dev_at91samd20_xplained_pro_board_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_at91samd20_xplained_pro_board_read(desc_t desc, char* buf,int size){
   return -1;
}

/*-------------------------------------------
| Name:dev_at91samd20_xplained_pro_board_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_at91samd20_xplained_pro_board_write(desc_t desc, const char* buf,int size){
   //read only mode
   return -1;
}

/*-------------------------------------------
| Name:dev_a0350_board_seek
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_at91samd20_xplained_pro_board_seek(desc_t desc,int offset,int origin){

   switch(origin) {

   case SEEK_SET:
      ofile_lst[desc].offset=offset;
      break;

   case SEEK_CUR:
      ofile_lst[desc].offset+=offset;
      break;

   case SEEK_END:
      //to do: warning in SEEK_END (+ or -)????
      ofile_lst[desc].offset-=offset;
      break;
   }

   return ofile_lst[desc].offset;
}

/*-------------------------------------------
| Name:dev_at91samd20_xplained_pro_board_ioctl
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_at91samd20_xplained_pro_board_ioctl(desc_t desc,int request,va_list ap){

   switch(request) {
   case BRDPWRDOWN:   //power down
      break;
   //
   case BRDRESET:   //reset
      break;

   case BRDWATCHDOG:   //start watchdog
      break;
   //
   case BRDCFGPORT: {
   }
   break;

   case BRDBEEP: {
   }
   break;

   //
   default:
      return -1;

   }

   return 0;
}

/*============================================
| End of Source  : dev_at91samd20_xplained_pro_board.c
==============================================*/
