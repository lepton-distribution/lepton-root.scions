/*
    FreeRTOS V9.0.0 - Copyright (C) 2016 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>>> AND MODIFIED BY <<<< the FreeRTOS exception.

    ***************************************************************************
    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<
    ***************************************************************************

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available on the following
    link: http://www.freertos.org/a00114.html

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that is more than just the market leader, it     *
     *    is the industry's de facto standard.                               *
     *                                                                       *
     *    Help yourself get started quickly while simultaneously helping     *
     *    to support the FreeRTOS project by purchasing a FreeRTOS           *
     *    tutorial book, reference manual, or both:                          *
     *    http://www.FreeRTOS.org/Documentation                              *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org/FAQHelp.html - Having a problem?  Start by reading
    the FAQ page "My application does not run, what could be wrong?".  Have you
    defined configASSERT()?

    http://www.FreeRTOS.org/support - In return for receiving this top quality
    embedded software for free we request you assist our global community by
    participating in the support forum.

    http://www.FreeRTOS.org/training - Investing in training allows your team to
    be as productive as possible as early as possible.  Now you can receive
    FreeRTOS training directly from Richard Barry, CEO of Real Time Engineers
    Ltd, and the world's leading authority on the world's leading RTOS.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
    Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.

    http://www.OpenRTOS.com - Real Time Engineers ltd. license FreeRTOS to High
    Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and commercial middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

/******************************************************************************
 * This project provides two demo applications.  A simple blinky style project,
 * and a more comprehensive test and demo application.  The
 * mainCREATE_SIMPLE_BLINKY_DEMO_ONLY setting (defined in this file) is used to
 * select between the two.  The simply blinky demo is implemented and described
 * in main_blinky.c.  The more comprehensive test and demo application is
 * implemented and described in main_full.c.
 *
 * This file implements the code that is not demo specific, including the
 * hardware setup and standard FreeRTOS hook functions.
 *
 * ENSURE TO READ THE DOCUMENTATION PAGE FOR THIS PORT AND DEMO APPLICATION ON
 * THE http://www.FreeRTOS.org WEB SITE FOR FULL INFORMATION ON USING THIS DEMO
 * APPLICATION, AND ITS ASSOCIATE FreeRTOS ARCHITECTURE PORT!
 *
 */

/* Scheduler include files. */
/* FreeRTOS includes. */
//lepton
/* Library includes. */
#include <stdint.h>
#include "kernel/core/kernel.h"
#include "board.h"


/*-----------------------------------------------------------*/

/*
 * Configure the hardware as necessary to run this demo.
 */
static void prvSetupHardware( void );


/* Prototypes for the standard FreeRTOS callback/hook functions implemented
within this file. */
void vApplicationMallocFailedHook( void );
void vApplicationIdleHook( void );
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName );
void vApplicationTickHook( void );

/*-----------------------------------------------------------*/
int main( void ){
	/* Configure the hardware ready to run the demo. */
	prvSetupHardware();
   
   //lepton kernel start 
   _start_kernel(0);
   //
   /* Start the scheduler. */
	vTaskStartScheduler();
   
	
	/* If all is well, the scheduler will now be running, and the following line
	will never be reached.  If the following line does execute, then there was
	insufficient FreeRTOS heap memory available for the idle and/or timer tasks
	to be created.  See the memory management section on the FreeRTOS web site
	for more details. */
	for( ;; );	

}
/*-----------------------------------------------------------*/

static void prvSetupHardware( void ){
	/* Disable watchdog. */
	WDT_Disable( WDT );

	SCB_EnableICache();
	SCB_EnableDCache();

}
/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook( void ){
	/* Called if a call to pvPortMalloc() fails because there is insufficient
	free memory available in the FreeRTOS heap.  pvPortMalloc() is called
	internally by FreeRTOS API functions that create tasks, queues, software
	timers, and semaphores.  The size of the FreeRTOS heap is set by the
	configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */

	/* Force an assert. */
	configASSERT( ( volatile void * ) NULL );
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName ){
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected. */

	/* Force an assert. */
	configASSERT( ( volatile void * ) NULL );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void ){
}
/*-----------------------------------------------------------*/

void vApplicationTickHook( void ){
}
/*-----------------------------------------------------------*/
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint16_t *pusIdleTaskStackSize )
{
/* The buffers used by the idle task must be static so they are persistent, and
so exist after this function returns. */
static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

	/* configUSE_STATIC_ALLOCATION is set to 1, so the application has the
	opportunity to supply the buffers that will be used by the Idle task as its
	stack and to hold its TCB.  If these are set to NULL then the buffers will
	be allocated dynamically, just as if xTaskCreate() had been called. */
	*ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
	*ppxIdleTaskStackBuffer = uxIdleTaskStack;
	*pusIdleTaskStackSize = configMINIMAL_STACK_SIZE; /* In words.  NOT in bytes! */
}
/*-----------------------------------------------------------*/

void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint16_t *pusTimerTaskStackSize )
{
/* The buffers used by the Timer/Daemon task must be static so they are
persistent, and so exist after this function returns. */
static StaticTask_t xTimerTaskTCB;
static StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];

	/* configUSE_STATIC_ALLOCATION is set to 1, so the application has the
	opportunity to supply the buffers that will be used by the Timer/RTOS daemon
	task as its	stack and to hold its TCB.  If these are set to NULL then the
	buffers will be allocated dynamically, just as if xTaskCreate() had been
	called. */
	*ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
	*ppxTimerTaskStackBuffer = uxTimerTaskStack;
	*pusTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH; /* In words.  NOT in bytes! */
}

/* Just to keep the linker happy. */
int __write( int x );
int __write( int x )
{
	return x;
}
