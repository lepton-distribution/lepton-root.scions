/*
The contents of this file are subject to the Mozilla Public License Version 1.1 
(the "License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at http://www.mozilla.org/MPL/

Software distributed under the License is distributed on an "AS IS" basis, 
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for the 
specific language governing rights and limitations under the License.

The Original Code is ______________________________________.

The Initial Developer of the Original Code is ________________________.
Portions created by ______________________ are Copyright (C) ______ _______________________.
All Rights Reserved.

Contributor(s): ______________________________________.

Alternatively, the contents of this file may be used under the terms of the eCos GPL license 
(the  [eCos GPL] License), in which case the provisions of [eCos GPL] License are applicable 
instead of those above. If you wish to allow use of your version of this file only under the
terms of the [eCos GPL] License and not to allow others to use your version of this file under 
the MPL, indicate your decision by deleting  the provisions above and replace 
them with the notice and other provisions required by the [eCos GPL] License. 
If you do not delete the provisions above, a recipient may use your version of this file under 
either the MPL or the [eCos GPL] License."
*/

//Based on dev_at91sam9261_cpu.c
/*============================================
| Includes    
==============================================*/
#include "kernel/core/types.h"
#include "kernel/core/interrupt.h"
#include "kernel/core/kernel.h"
#include "kernel/core/system.h"
#include "kernel/core/fcntl.h"
#include "kernel/core/ioctl_lcd.h"
#include "kernel/core/ioctl_board.h"

#include "kernel/fs/vfs/vfsdev.h"

#if defined(__KERNEL_UCORE_EMBOS)
//#include "kernel/core/ucore/embOSARM7_332/arch/cpu_cmsis/cmsis_init.h"
//#include <ioat91sam9261.h>
//#include <intrinsic.h>
#elif defined(__KERNEL_UCORE_ECOS)
#include "dev_cmsis_cpu.h"//GD-TODO ?
#include "cyg/hal/cmsis.h"
#endif

/*============================================
| Global Declaration 
==============================================*/

#if defined(__KERNEL_UCORE_EMBOS)
extern void (*g_p_fct_dbg_interrupt)(void);
#endif

/*============================================
| Implementation 
==============================================*/
const char dev_cmsis_cpu_name[]="cpu\0";

int dev_cmsis_cpu_load(void);
int dev_cmsis_cpu_open(desc_t desc, int o_flag);
int dev_cmsis_cpu_close(desc_t desc);
int dev_cmsis_cpu_isset_read(desc_t desc);
int dev_cmsis_cpu_isset_write(desc_t desc);
int dev_cmsis_cpu_read(desc_t desc, char* buf,int size);
int dev_cmsis_cpu_write(desc_t desc, const char* buf,int size);
int dev_cmsis_cpu_seek(desc_t desc,int offset,int origin);
int dev_cmsis_cpu_ioctl(desc_t desc,int request,va_list ap);


dev_map_t dev_cmsis_cpu_map={
   dev_cmsis_cpu_name,
   S_IFBLK,
   dev_cmsis_cpu_load,
   dev_cmsis_cpu_open,
   dev_cmsis_cpu_close,
   dev_cmsis_cpu_isset_read,
   dev_cmsis_cpu_isset_write,
   dev_cmsis_cpu_read,
   dev_cmsis_cpu_write,
   dev_cmsis_cpu_seek,
   dev_cmsis_cpu_ioctl //ioctl
};

// Watchdog features (with value = see application layer)
#define        WATCHDOG_KEY            0xA5   

/*===========================================
Implementation
=============================================*/
#if defined(__KERNEL_UCORE_EMBOS)
/*--------------------------------------------
| Name:        AT91F_UndefHandler
| Description: 
| Parameters:  none
| Return Type: none
| Comments:    
| See:         
----------------------------------------------*/
void CMSIS_UndefHandler(void)
{
	while (1);
}

/*--------------------------------------------
| Name:        AT91F_SpuriousHandler
| Description: 
| Parameters:  none
| Return Type: none
| Comments:    
| See:         
----------------------------------------------*/
void CMSIS_SpuriousHandler(void)
{
	while (1);
}
#endif
/*-------------------------------------------
| Name:dev_cmsis_cpu_load
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_cmsis_cpu_load(void){
   //unprotected mode (very important for irq management)
   //AT91C_BASE_AIC->AIC_DCR = 0;
   //
#if defined(__KERNEL_UCORE_EMBOS)
   // Pointer initialization
//   g_p_fct_dbg_interrupt =  NULL;
#elif defined(__KERNEL_UCORE_ECOS)
#endif
         
   return 0;
}

/*-------------------------------------------
| Name:dev_cmsis_cpu_open
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_cmsis_cpu_open(desc_t desc, int o_flag){

   //
   if(o_flag & O_RDONLY){
   }

   if(o_flag & O_WRONLY){
   }

   ofile_lst[desc].offset=0;

   return 0;
}

/*-------------------------------------------
| Name:dev_cmsis_cpu_close
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_cmsis_cpu_close(desc_t desc){
   return 0;
}

/*-------------------------------------------
| Name:dev_cmsis_cpu_isset_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_cmsis_cpu_isset_read(desc_t desc){
  return -1;
}

/*-------------------------------------------
| Name:dev_cmsis_cpu_isset_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_cmsis_cpu_isset_write(desc_t desc){
      return -1;
}
/*-------------------------------------------
| Name:dev_cmsis_cpu_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_cmsis_cpu_read(desc_t desc, char* buf,int size){
   return -1;
}

/*-------------------------------------------
| Name:dev_cmsis_cpu_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_cmsis_cpu_write(desc_t desc, const char* buf,int size){
   //read only mode
   return -1;
}

/*-------------------------------------------
| Name:dev_cmsis_cpu_seek
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_cmsis_cpu_seek(desc_t desc,int offset,int origin){

   switch(origin){

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
| Name:dev_cmsis_cpu_ioctl
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_cmsis_cpu_ioctl(desc_t desc,int request,va_list ap)
{
   int wdgvalue=0;
   
    switch(request)
    {
      case CPUWDGDISABLE :
      {
          // Arm watchdog Timer   
         //  *AT91C_WDTC_WDMR =  AT91C_WDTC_WDDIS; // disbale Wtachdog
      }      
      
      case CPUWDGREFRESH:
      {
          // Arm watchdog Timer   
          //*AT91C_WDTC_WDCR =   (WATCHDOG_KEY << 24)   |
          //                      AT91C_WDTC_WDRSTT;   // Watchdog Restart 
      }
      break;

      case CPUWDGINIT:
      {
         //wdgvalue = va_arg( ap, int);         
         // Initialize WatchDog Value
         //*AT91C_WDTC_WDMR =  ( wdgvalue           |     
         //                      AT91C_WDTC_WDRSTEN |                              
         //                      (((wdgvalue) & 0xFFF) <<  16) );            
          //  *AT91C_WDTC_WDMR =  AT91C_WDTC_WDDIS;         // not used here
      }
      break;
      
      //
      default:
         return -1;

   }
   return 0;     
}

/*============================================
| End of Source  : dev_cmsis_cpu.c
==============================================*/
