/*
The contents of this file are subject to the Mozilla Public License Version 1.1
(the "License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at http://www.mozilla.org/MPL/

Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for the
specific language governing rights and limitations under the License.

The Original Code is Lepton.

The Initial Developer of the Original Code is Philippe Le Boulanger.
Portions created by Philippe Le Boulanger are Copyright (C) 2015 <lepton.phlb@gmail.com>.
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


/*============================================
| Includes    
==============================================*/
#include <stdint.h>

#include "kernel/core/kernelconf.h"
#include "kernel/core/types.h"
#include "kernel/core/dirent.h"
#include "kernel/core/stat.h"
#include "kernel/core/interrupt.h"
#include "kernel/core/kernel.h"
#include "kernel/core/system.h"
#include "kernel/core/fcntl.h"
#include "kernel/core/ioctl.h"
#include "kernel/core/ioctl_board.h"

#include "kernel/core/kernel_printk.h"

#include "kernel/fs/vfs/vfstypes.h"

#include "board.h"

#include "kernel/dev/arch/cortexm/at91samv7x/dev_samv7x/dev_samv7x_cpu_x.h"

/*============================================
| Global Declaration 
==============================================*/

//IO
//inserted
#define CHAUDRICONCEPT_BOARD_IO_BOX_INSERT  {PIO_PA19, PIOA, ID_PIOA, PIO_INPUT, PIO_DEFAULT}
static const Pin pins_io_box_insert[] = {CHAUDRICONCEPT_BOARD_IO_BOX_INSERT};

//wakeup spare
#define CHAUDRICONCEPT_BOARD_IO_WAKEUP_SPARE  {PIO_PA14, PIOA, ID_PIOA, PIO_INPUT, PIO_DEFAULT}
static const Pin pins_io_wakeup_spare[] = {CHAUDRICONCEPT_BOARD_IO_WAKEUP_SPARE};


/* (SCR) Sleep deep bit */
#define SCR_SLEEPDEEP   (0x1 <<  2)
/* Wakeup PIN Index definition */
#define WKUP_IDX_EXTERNAL_RTC_PIN               1 
#define WKUP_IDX_EXTERNAL_INT_BAT_PIN           2 
#define WKUP_IDX_EXTERNAL_WAKEUP_SPARE_PIN      8
#define WKUP_IDX_EXTERNAL_INSERT_PIN            9
const uint32_t gWakeUpPinId = (1 << WKUP_IDX_EXTERNAL_RTC_PIN) | (1 << WKUP_IDX_EXTERNAL_WAKEUP_SPARE_PIN) | (1 << WKUP_IDX_EXTERNAL_INSERT_PIN);

const char dev_samv7x_cpu_x_name[]="cpu0\0";

static int dev_samv7x_cpu_x_load(void);
static int dev_samv7x_cpu_x_open(desc_t desc, int o_flag);
static int dev_samv7x_cpu_x_close(desc_t desc);
static int dev_samv7x_cpu_x_isset_read(desc_t desc);
static int dev_samv7x_cpu_x_isset_write(desc_t desc);
static int dev_samv7x_cpu_x_read(desc_t desc, char* buf,int size);
static int dev_samv7x_cpu_x_write(desc_t desc, const char* buf,int size);
static int dev_samv7x_cpu_x_seek(desc_t desc,int offset,int origin);
static int dev_samv7x_cpu_x_ioctl(desc_t desc,int request,va_list ap);


dev_map_t dev_samv7x_cpu_x_map={
   dev_samv7x_cpu_x_name,
   S_IFBLK,
   dev_samv7x_cpu_x_load,
   dev_samv7x_cpu_x_open,
   dev_samv7x_cpu_x_close,
   dev_samv7x_cpu_x_isset_read,
   dev_samv7x_cpu_x_isset_write,
   dev_samv7x_cpu_x_read,
   dev_samv7x_cpu_x_write,
   dev_samv7x_cpu_x_seek,
   dev_samv7x_cpu_x_ioctl //ioctl
};

//
sXdmad   g_samv7x_dmad;

//
uint8_t dev_samv7x_cpu_x_low_wfi_enable=0;

/*===========================================
Implementation
=============================================*/



/**
 *  \brief Handler for XDMAC.
 *
 *  Process XDMA interrupts
 */
void XDMAC_Handler(void)
{
   //
   __hw_enter_interrupt();
   //
	XDMAD_Handler(&g_samv7x_dmad);
   //
   __hw_leave_interrupt();
}

/**
 * \brief Get Delayed number of tick
 * \param startTick Start tick point.
 * \param endTick   End tick point.
 */
uint32_t GetDelayInTicks(uint32_t startTick, uint32_t endTick)
{
	assert(SysTickConfigured);

	if (endTick >= startTick) return (endTick - startTick);

	return (endTick + (0xFFFFFFFF - startTick) + 1);

}

/**
 * \brief Get Delayed number of tick
 * \param startTick Start tick point.
 * \param endTick   End tick point.
 */
uint32_t GetTicks(void)
{
	return __kernel_get_timer_ticks();
}

/**
 * \brief Test Backup Mode.
 */
static void EnterBackupMode(void)
{
	kernel_printk( "kernel: cpu enter in backup mode...\n\r" );
   
   /* Enable the PIO for wake-up */
   uint32_t msk_wuir = gWakeUpPinId;
	//
   if(PIO_Get(&pins_io_box_insert[0])) //==1 wait falling edge on insert: 1 to 0
      msk_wuir =  msk_wuir;
   else //==0 wait rising edge on  insert: 0 to 1
      msk_wuir = ( (1 << WKUP_IDX_EXTERNAL_INSERT_PIN) << 16 ) | msk_wuir;
   //
   if(PIO_Get(&pins_io_wakeup_spare[0])) //==1 wait falling edge on insert: 1 to 0
      msk_wuir =  msk_wuir;
   else //==0 wait rising edge on  insert: 0 to 1
      msk_wuir = ( (1 << WKUP_IDX_EXTERNAL_WAKEUP_SPARE_PIN) << 16 ) | msk_wuir;
   //
   SUPC->SUPC_WUIR = msk_wuir;
      
   //
	PMC->PMC_FSMR   = PMC_FSMR_FLPM_FLASH_DEEP_POWERDOWN | gWakeUpPinId;
   
	/* Set the SLEEPDEEP bit of Cortex-M processor. */
	SCB->SCR |= SCR_SLEEPDEEP;
	/* Set the VROFF bit of SUPC_CR. */
	SUPC->SUPC_CR = SUPC_CR_KEY_PASSWD | SUPC_CR_VROFF_STOP_VREG;
	SUPC->SUPC_WUMR = (1 << 12);

	/* Wake Up Input 2 and 4 (L_CLICK and R_CLICK) Enable + (0) high to low
		level Transition. Exit from Backup mode occurs as a result of one of
		the following enabled wake-up events:
		WKUPEN0-13 pins (level transition, configurable denouncing)
		Supply Monitor alarm
		RTC alarm
		RTT alarm */
   __DSB();
   __ISB();
   while (1)asm("nop");
   asm("nop");
   asm("nop");
   asm("nop");
   //
	kernel_printk("kernel: cpu exit backup Mode\n\r");
   //
	while (1);
   // to do ??? reset cpu and restart???
}


/**
 * \brief Test Sleep Mode
 */
static void EnterSleepMode(void)
{
	kernel_printk("kernel: cpu sleep mode\n\r");
	/* The purpose of sleep mode is to optimize power consumption of the
		device versus response time.
	   In this mode, only the core clock is stopped. The peripheral clocks can
	   be enabled.
	   The current consumption in this mode is application-dependent.*/
	PMC->PMC_FSMR &= (uint32_t)~PMC_FSMR_LPM;
	SCB->SCR &= (uint32_t)~SCR_SLEEPDEEP;
	/* Processor wake-up is triggered by an interrupt if the WFI instruction

	/* This mode is entered using the instruction Wait for Interrupt (WFI).*/
	// see in RTOS idle task
	
}
/*-------------------------------------------
| Name:dev_samv7x_cpu_x_load
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_samv7x_cpu_x_load(void){
   //
   WDT_Disable(WDT);
   //
   //check reset type, if return from backup mode force new reset
   // workaround to fix problem with sdio issue and small reset length
   if((RSTC_GetStatus()&RSTC_SR_RSTTYP_Msk)==RSTC_SR_RSTTYP_BACKUP_RST){
      RSTC_SetExtResetLength(0x1f);
      RSTC_ExtReset();
      while(1);
   }
   //
   RSTC_SetExtResetLength(0);
   //
   //
   XDMAD_Initialize(&g_samv7x_dmad,0);
   NVIC_ClearPendingIRQ(XDMAC_IRQn);
	NVIC_SetPriority(XDMAC_IRQn, (1u << __NVIC_PRIO_BITS) - 4u); //-2u
	NVIC_EnableIRQ(XDMAC_IRQn);
   //
  
   //NVIC_SetPriority(XDMAC_IRQn, 1);
   //
   g_samv7x_dmad.pXdmacs = XDMAC;
   //
   
   //
   
   return 0;
}

/*-------------------------------------------
| Name:dev_samv7x_cpu_x_open
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_samv7x_cpu_x_open(desc_t desc, int o_flag){

   //
   if(o_flag & O_RDONLY){
   }

   if(o_flag & O_WRONLY){
   }

   ofile_lst[desc].offset=0;

   return 0;
}

/*-------------------------------------------
| Name:dev_samv7x_cpu_x_close
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_samv7x_cpu_x_close(desc_t desc){
   return 0;
}

/*-------------------------------------------
| Name:dev_samv7x_cpu_x_isset_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_samv7x_cpu_x_isset_read(desc_t desc){
  return -1;
}

/*-------------------------------------------
| Name:dev_samv7x_cpu_x_isset_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_samv7x_cpu_x_isset_write(desc_t desc){
      return -1;
}
/*-------------------------------------------
| Name:dev_samv7x_cpu_x_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_samv7x_cpu_x_read(desc_t desc, char* buf,int size){
   return -1;
}

/*-------------------------------------------
| Name:dev_samv7x_cpu_x_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_samv7x_cpu_x_write(desc_t desc, const char* buf,int size){
   //read only mode
   return -1;
}

/*-------------------------------------------
| Name:dev_samv7x_cpu_x_seek
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_samv7x_cpu_x_seek(desc_t desc,int offset,int origin){

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
| Name:dev_samv7x_cpu_x_ioctl
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_samv7x_cpu_x_ioctl(desc_t desc,int request,va_list ap){  
    switch(request)
    {
      case CPUSRST:
         //RSTC_ProcessorReset();
         RSTC_SetExtResetLength(0x1f);
         RSTC_ExtReset();
         while(1);
      break;
      
      case CPULOWPWR:{
         uint32_t  mode= va_arg( ap, uint32_t);
         //
         switch(mode){
            case CPUSLEEPMODE:
               EnterSleepMode();
               dev_samv7x_cpu_x_low_wfi_enable=1;
            break;
            
            case CPUBCKUPMODE:
               EnterBackupMode();
            break;
            //
            default:
            return -1;
         }
            
      }
      break; 
      
      
      case CPUWDGDISABLE :
      {
      }      
      
      case CPUWDGREFRESH:
      {
      }
      break;

      case CPUWDGINIT:
      {
      }
      break;
      
      //
      default:
         return -1;

   }
   return 0;     
}

/*============================================
| End of Source  : dev_samv7x_cpu_x.c
==============================================*/
