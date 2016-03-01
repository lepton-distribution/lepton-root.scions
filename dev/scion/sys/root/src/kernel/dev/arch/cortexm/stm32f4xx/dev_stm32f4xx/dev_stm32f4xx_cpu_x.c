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
#include "kernel/core/kernelconf.h"
#include "kernel/core/types.h"
#include "kernel/core/interrupt.h"
#include "kernel/core/kernel.h"
#include "kernel/core/system.h"
#include "kernel/core/fcntl.h"
#include "kernel/core/stat.h"
#include "kernel/core/ioctl.h"
#include "kernel/core/ioctl_board.h"

#include "kernel/fs/vfs/vfsdev.h"

#include "stm32f4xx_hal.h"

/*============================================
| Global Declaration 
==============================================*/


/*============================================
| Implementation 
==============================================*/
const char dev_stm32f4xx_cpu_x_name[]="cpu0\0";

int dev_stm32f4xx_cpu_x_load(void);
int dev_stm32f4xx_cpu_x_open(desc_t desc, int o_flag);
int dev_stm32f4xx_cpu_x_close(desc_t desc);
int dev_stm32f4xx_cpu_x_isset_read(desc_t desc);
int dev_stm32f4xx_cpu_x_isset_write(desc_t desc);
int dev_stm32f4xx_cpu_x_read(desc_t desc, char* buf,int size);
int dev_stm32f4xx_cpu_x_write(desc_t desc, const char* buf,int size);
int dev_stm32f4xx_cpu_x_seek(desc_t desc,int offset,int origin);
int dev_stm32f4xx_cpu_x_ioctl(desc_t desc,int request,va_list ap);


dev_map_t dev_stm32f4xx_cpu_x_map={
   dev_stm32f4xx_cpu_x_name,
   S_IFBLK,
   dev_stm32f4xx_cpu_x_load,
   dev_stm32f4xx_cpu_x_open,
   dev_stm32f4xx_cpu_x_close,
   dev_stm32f4xx_cpu_x_isset_read,
   dev_stm32f4xx_cpu_x_isset_write,
   dev_stm32f4xx_cpu_x_read,
   dev_stm32f4xx_cpu_x_write,
   dev_stm32f4xx_cpu_x_seek,
   dev_stm32f4xx_cpu_x_ioctl //ioctl
};

// Watchdog features (with value = see application layer)
#define        WATCHDOG_KEY            0xA5   

/*===========================================
Implementation
=============================================*/

//
void HAL_Delay(__IO uint32_t Delay)
{
   __kernel_usleep(Delay);
}

//
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;

  __PWR_CLK_ENABLE();

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1
                              |RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);

  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2S;
  PeriphClkInitStruct.PLLI2S.PLLI2SN = 192;
  PeriphClkInitStruct.PLLI2S.PLLI2SR = 2;
  HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

}


/*-------------------------------------------
| Name:dev_stm32f4xx_cpu_x_load
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_stm32f4xx_cpu_x_load(void){
   /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
   HAL_Init();
   /* Configure the system clock */
   SystemClock_Config();
   //
   return 0;
}

/*-------------------------------------------
| Name:dev_stm32f4xx_cpu_x_open
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_stm32f4xx_cpu_x_open(desc_t desc, int o_flag){

   //
   if(o_flag & O_RDONLY){
   }

   if(o_flag & O_WRONLY){
   }

   ofile_lst[desc].offset=0;

   return 0;
}

/*-------------------------------------------
| Name:dev_stm32f4xx_cpu_x_close
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_stm32f4xx_cpu_x_close(desc_t desc){
   return 0;
}

/*-------------------------------------------
| Name:dev_stm32f4xx_cpu_x_isset_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_stm32f4xx_cpu_x_isset_read(desc_t desc){
  return -1;
}

/*-------------------------------------------
| Name:dev_stm32f4xx_cpu_x_isset_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_stm32f4xx_cpu_x_isset_write(desc_t desc){
      return -1;
}
/*-------------------------------------------
| Name:dev_stm32f4xx_cpu_x_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_stm32f4xx_cpu_x_read(desc_t desc, char* buf,int size){
   return -1;
}

/*-------------------------------------------
| Name:dev_stm32f4xx_cpu_x_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_stm32f4xx_cpu_x_write(desc_t desc, const char* buf,int size){
   //read only mode
   return -1;
}

/*-------------------------------------------
| Name:dev_stm32f4xx_cpu_x_seek
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_stm32f4xx_cpu_x_seek(desc_t desc,int offset,int origin){

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
| Name:dev_stm32f4xx_cpu_x_ioctl
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_stm32f4xx_cpu_x_ioctl(desc_t desc,int request,va_list ap)
{
   int wdgvalue=0;
   
    switch(request)
    {
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
| End of Source  : dev_stm32f4xx_cpu_x.c
==============================================*/
