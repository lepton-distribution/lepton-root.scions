/*===========================================
Compiler Directive
=============================================*/
#ifndef _USER_KERNEL_MKCONF_H_
#define _USER_KERNEL_MKCONF_H_


//force definition of realtime kernel
//#define __KERNEL_UCORE_FREERTOS
#define __KERNEL_UCORE_EMBOS

//see kernel/core/kernelconf.h
#ifdef __KERNEL_UCORE_FREERTOS
   #include "kernel/core/ucore/freeRTOS_8-0-0/source/arch/cortex-m4/stm32f4/FreeRTOSConfig.h"
#endif 


//force definition of cpu device
#define __tauon_cpu_device__ __tauon_cpu_device_cortexM4_stm32f4__

//use pipe
#define __tauon_kernel_profile__ __tauon_kernel_profile_classic__
#define __KERNEL_PIPE_SIZE 1024
#define __KERNEL_RTFS_BLOCK_SIZE 16

//force EFFS for stm32f407 on olimex-stm32-p407 board
#define __file_system_profile__  __file_system_profile_classic__
#define __KERNEL_VFS_SUPPORT_EFFS   0

//ip stack
//#define USE_UIP 
#define USE_LWIP
#define USE_IF_ETHERNET
//specific target include for pinout definition
#include "kernel/dev/bsp/olimex_p407/dev_olimex_p407_board/dev_olimex_p407_board.h"

#define __USER_MONGOOSE_PTHREAD_STACK_SIZE   (8*1024)
#define __USER_MONGOOSE_CGI_ENVIRONMENT_SIZE (2*1024)
#define __USER_MONGOOSE_MAX_REQUEST_SIZE     (2*1024)

/*===========================================
Includes
=============================================*/


/*===========================================
Declaration
=============================================*/

#endif
