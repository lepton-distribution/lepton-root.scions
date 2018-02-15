/*********************************************************************
*                SEGGER Microcontroller GmbH & Co. KG                *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 1995 - 2016 SEGGER Microcontroller GmbH & Co. KG         *
*                                                                    *
*       Internet: segger.com  Support: support_embos@segger.com      *
*                                                                    *
**********************************************************************
*                                                                    *
*       embOS * Real time operating system for microcontrollers      *
*                                                                    *
*       Please note:                                                 *
*                                                                    *
*       Knowledge of this file may under no circumstances            *
*       be used to write a similar product or a real-time            *
*       operating system for in-house use.                           *
*                                                                    *
*       Thank you for your fairness !                                *
*                                                                    *
**********************************************************************
*                                                                    *
*       OS version: 4.30                                             *
*                                                                    *
**********************************************************************

----------------------------------------------------------------------
File    : RTOSInit_SAMV71_CMSIS.c for Atmel SAMV71

Purpose : Initializes and handles the hardware for embOS as far
          as required by embOS
          Feel free to modify this file acc. to your target system.
--------  END-OF-HEADER  ---------------------------------------------
*/

#include "RTOS.h"
#include "SEGGER_SYSVIEW.h"
#include "samv71.h"     // Device specific header file, contains CMSIS

/*********************************************************************
*
*       Configuration
*
**********************************************************************
*/

/*********************************************************************
*
*       Clock frequency settings (configuration)
*/
#ifndef   OS_FSYS                   /* CPU Main clock frequency     */
  #define OS_FSYS SystemCoreClock
#endif

#ifndef   OS_PCLK_TIMER             /* Peripheral clock for timer   */
  #define OS_PCLK_TIMER (OS_FSYS)   /* May vary from CPU clock      */
#endif                              /* depending on CPU             */

#ifndef   OS_PCLK_UART              /* Peripheral clock for UART    */
  #define OS_PCLK_UART (OS_FSYS)    /* May vary from CPU clock      */
#endif                              /* depending on CPU             */

#ifndef   OS_TICK_FREQ
  #define OS_TICK_FREQ (1000u)
#endif

/*********************************************************************
*
*       Configuration of communication to embOSView
*/
#ifndef   OS_VIEW_IFSELECT
  #define OS_VIEW_IFSELECT  OS_VIEW_IF_JLINK
#endif

/****** End of configurable options *********************************/

/*********************************************************************
*
*       UART settings for embOSView
*       If you do not want (or can not due to hardware limitations)
*       to dedicate a UART to OSView, please define it to be -1
*       Currently the standard code enables USART0 per default.
*
*       Function | I/O line | Board location
*       ====================================
*          RX    |   PB0    |   EXT1_P13
*          TX    |   PB1    |   EXT1_P14
*          GND   |    -     |   SPI_GND
*          Vin   |    -     |   SPI_5V0
*/
#if (OS_VIEW_IFSELECT == OS_VIEW_IF_UART)
  #ifndef   OS_UART
    #define OS_UART (0)
  #endif

  #ifndef   OS_BAUDRATE
    #define OS_BAUDRATE (38400)
  #endif
#endif

/*********************************************************************
*
*       Vector table
*/
#if (defined __SES_ARM)           // SEGGER Embedded Studio
  extern int _vectors;
  #define __Vectors    _vectors
#elif (defined __CROSSWORKS_ARM)  // Rowley CrossStudio
  extern int _vectors;
  #define __Vectors    _vectors
#elif (defined __ICCARM__)        // IAR
  #define __Vectors    __vector_table
#elif (defined __GNUC__)          // GCC
  extern unsigned char __Vectors;
#elif (defined __CC_ARM)          // KEIL
  extern unsigned char __Vectors;
#endif

/*********************************************************************
*
*       Local defines (sfrs and addresses used in RTOSInit.c)
*
**********************************************************************
*/
#define NVIC_VTOR         (*(volatile OS_U32*) (0xE000ED08uL))

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

#if (OS_VIEW_IFSELECT == OS_VIEW_IF_JLINK)
  #include "JLINKMEM.h"
  const OS_U32 OS_JLINKMEM_BufferSize = 32u;  // Size of the communication buffer for JLINKMEM
#else
  const OS_U32 OS_JLINKMEM_BufferSize = 0;    // Communication not used
#endif

/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _OS_GetHWTimerCycles()
*
* Function description
*   Returns the current hardware timer count value
*
* Return value
*   Current timer count value
*/
static unsigned int _OS_GetHWTimerCycles(void) {
  return SysTick->VAL;
}

/*********************************************************************
*
*       _OS_GetHWTimer_IntPending()
*
* Function description
*   Returns if the hardware timer interrupt pending flag is set
*
* Return value
*   == 0; Interrupt pending flag not set
*   != 0: Interrupt pending flag set
*/
static unsigned int _OS_GetHWTimer_IntPending(void) {
  return SCB->ICSR & SCB_ICSR_PENDSTSET_Msk;
}

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*       SysTick_Handler()
*
* Function description
*   This is the code that gets called when the processor receives a
*   _SysTick exception. SysTick is used as OS timer tick.
*/
#ifdef __cplusplus
extern "C" {
#endif
void SysTick_Handler(void);      // Avoid warning, Systick_Handler is not prototyped in any CMSIS header
#ifdef __cplusplus
}
#endif
void SysTick_Handler(void) {
  OS_EnterNestableInterrupt();
  OS_TICK_Handle();
  #if (OS_VIEW_IFSELECT == OS_VIEW_IF_JLINK)
    JLINKMEM_Process();
  #endif
  OS_LeaveNestableInterrupt();
}

/*********************************************************************
*
*       OS_InitHW()
*
*       Initialize the hardware (timer) required for embOS to run.
*       May be modified, if an other timer should be used
*/
void OS_InitHW(void) {
  //
  // Structure with information about timer frequency, tick frequency, etc.
  // for micro second precise system time.
  // SysTimerConfig.TimerFreq will be set later, thus it is initialized with zero.
  //
  OS_SYSTIMER_CONFIG SysTimerConfig = {0, OS_TICK_FREQ, 0, _OS_GetHWTimerCycles, _OS_GetHWTimer_IntPending};

  OS_IncDI();
  //
  // We assume, the PLL and core clock was already set by the SystemInit() function
  // which was called from the startup code
  // Therefore, we don't have to initialize any hardware here,
  // we just ensure that the system clock variable is updated and then
  // set the periodic system timer tick for embOS.
  //
  SystemCoreClockUpdate();                             // Update the system clock variable (might not have been set before)
  if (SysTick_Config (OS_PCLK_TIMER / OS_TICK_FREQ)) { // Setup SysTick Timer for 1 msec interrupts
    while (1);                                         // Handle Error
  }
  //
  // Initialize NVIC vector base address. Might be necessary for RAM targets or application not running from 0
  //
  NVIC_VTOR = (OS_U32)&__Vectors;
  //
  // Set the interrupt priority for the system timer to 2nd lowest level to ensure the timer can preempt PendSV handler
  //
  NVIC_SetPriority(SysTick_IRQn, (1u << __NVIC_PRIO_BITS) - 2u);
  //
  // Setup values for usec precise system time functions
  //
  SysTimerConfig.TimerFreq = SystemCoreClock;
  OS_Config_SysTimer(&SysTimerConfig);
  //
  // Enable Cortex M7 cache
  //
  SCB_EnableICache();
  SCB_EnableDCache();
  //
  // Configure and initialize SEGGER SystemView
  //
#if OS_PROFILE
  SEGGER_SYSVIEW_Conf();
#endif
  //
  // Initialize the optional communication for embOSView
  //
#if (OS_VIEW_IFSELECT != OS_VIEW_DISABLED)
  OS_COM_Init();
#endif
  OS_DecRI();
}

/*********************************************************************
*
*       OS_Idle()
*
*       Please note:
*       This is basically the "core" of the idle loop.
*       This core loop can be changed, but:
*       The idle loop does not have a stack of its own, therefore no
*       functionality should be implemented that relies on the stack
*       to be preserved. However, a simple program loop can be programmed
*       (like toggling an output or incrementing a counter)
*/
extern uint8_t dev_samv7x_cpu_x_low_wfi_enable; //see dev_samv7x_cpu_x.c
void OS_Idle(void) {     // Idle loop: No task is ready to execute
  while (1) {
    //
    #if ((OS_VIEW_IFSELECT != OS_VIEW_IF_JLINK) && (OS_DEBUG == 0))     // Enter CPU halt mode when not in DEBUG build and J-Link communication not used
      __WFI();
    #else
      //lepton
      if(dev_samv7x_cpu_x_low_wfi_enable){
         __DSB();
         __WFI();
         __ISB();
      }
    #endif
  }
}

/*********************************************************************
*
*       OS_GetTime_Cycles()
*
*       This routine is required for task-info via embOSView or high
*       resolution time measurement functions.
*       It returns the system time in timer clock cycles.
*/
OS_U32 OS_GetTime_Cycles(void) {
  OS_U32 Time;
  OS_U32 Cnt;

  Time = OS_GetTime32();
  Cnt  = (OS_PCLK_TIMER/1000u) - SysTick->VAL;
  //
  // Check if timer interrupt pending ...
  //
  if (SCB->ICSR & SCB_ICSR_PENDSTSET_Msk) {
    Cnt = (OS_PCLK_TIMER/1000u) - SysTick->VAL;     // Interrupt pending, re-read timer and adjust result
    Time++;
  }
  return ((OS_PCLK_TIMER/1000u) * Time) + Cnt;
}

/*********************************************************************
*
*       OS_ConvertCycles2us()
*
*       Convert Cycles into micro seconds.
*
*       If your clock frequency is not a multiple of 1 MHz,
*       you may have to modify this routine in order to get proper
*       diagnostics.
*
*       This routine is required for profiling or high resolution time
*       measurement only. It does not affect operation of the OS.
*/
OS_U32 OS_ConvertCycles2us(OS_U32 Cycles) {
  return Cycles/(OS_PCLK_TIMER/1000000u);
}

/*********************************************************************
*
*       Optional communication with embOSView
*
**********************************************************************
*/

#if (OS_VIEW_IFSELECT == OS_VIEW_IF_JLINK)                  // Communication via JTAG / SWD

static void _JLINKMEM_OnRx(OS_U8 Data);
static void _JLINKMEM_OnTx(void);
static OS_INT _JLINKMEM_GetNextChar(void);

/*********************************************************************
*
*       _JLINKMEM_OnRx()
*/
static void _JLINKMEM_OnRx(OS_U8 Data) {
  OS_OnRx(Data);
}

/*********************************************************************
*
*       _JLINKMEM_OnTx()
*/
static void _JLINKMEM_OnTx(void) {
  OS_OnTx();
}

/*********************************************************************
*
*       _JLINKMEM_GetNextChar()
*/
static OS_INT _JLINKMEM_GetNextChar(void) {
  return OS_COM_GetNextChar();
}

/*********************************************************************
*
*       OS_COM_Init()
*       Initialize memory access for embOSView
*/
void OS_COM_Init(void) {
  JLINKMEM_SetpfOnRx(_JLINKMEM_OnRx);
  JLINKMEM_SetpfOnTx(_JLINKMEM_OnTx);
  JLINKMEM_SetpfGetNextChar(_JLINKMEM_GetNextChar);
}

/*********************************************************************
*
*       OS_COM_Send1()
*       Send one character via memory
*/
void OS_COM_Send1(OS_U8 c) {
  JLINKMEM_SendChar(c);
}
#elif (OS_VIEW_IFSELECT ==  OS_VIEW_IF_UART)  // Communication via UART, can not be implemented generic


#define OS_BAUDDIVIDE        (OS_PCLK_UART / (OS_BAUDRATE * 16L * 2L))

#define USART0_BASE_ADDR     (0x40024000uL) /* USART0 base address */
#define USART0_ID            (13u)
#define OS_UART_RX_PIN       (0u)
#define OS_UART_TX_PIN       (1u)

#define PIO_PDR_USART        ((1uL << OS_UART_RX_PIN) | (1uL << OS_UART_TX_PIN))

//
// Power management controller
//
#define PMC_BASE_ADDR        (0x400E0600uL)
#define PMC_PCER             (*(volatile OS_U32*) (PMC_BASE_ADDR + 0x10))  /* Peripheral clock enable register */
#define PMC_WPMR             (*(volatile OS_U32*) (PMC_BASE_ADDR + 0xE4))  /* PMC status protect register      */
//
// Parallel ports (PIO)
//
#define PIOB_BASE_ADDR       (0x400E1000uL)
#define PIOB_PDR             (*(volatile OS_U32*) (PIOB_BASE_ADDR + 0x04)) /* PIOB disable register           */
#define PIOB_ABCDSR1         (*(volatile OS_U32*) (PIOB_BASE_ADDR + 0x70)) /* PIOB peripheral select register */
#define PIOB_ABCDSR2         (*(volatile OS_U32*) (PIOB_BASE_ADDR + 0x74)) /* PIOB peripheral select register */
#define PIO_WPMR             (*(volatile OS_U32*) (PIOB_BASE_ADDR + 0xE4)) /* PIOB write protect              */
//
// The sfrs used for USART or UART
//
#define US_CR                (*(volatile OS_U32*)(USART0_BASE_ADDR + 0x00))  /* USART0 control register            */
#define US_MR                (*(volatile OS_U32*)(USART0_BASE_ADDR + 0x04))  /* USART0 mode register               */
#define US_IER               (*(volatile OS_U32*)(USART0_BASE_ADDR + 0x08))  /* USART0 interrupt enable register   */
#define US_IDR               (*(volatile OS_U32*)(USART0_BASE_ADDR + 0x0c))  /* USART0 interrupt disable           */
#define US_IMR               (*(volatile OS_U32*)(USART0_BASE_ADDR + 0x10))  /* USART0 interrupt mask register     */
#define US_CSR               (*(volatile OS_U32*)(USART0_BASE_ADDR + 0x14))  /* USART0 channel status register     */
#define US_RHR               (*(volatile OS_U32*)(USART0_BASE_ADDR + 0x18))  /* USART0 receive holding register    */
#define US_THR               (*(volatile OS_U32*)(USART0_BASE_ADDR + 0x1c))  /* USART0 transmit holding register   */
#define US_BRGR              (*(volatile OS_U32*)(USART0_BASE_ADDR + 0x20))  /* USART0 baudrate generator register */

#define US_RXRDY             (1uL << 0u)         /* Rx status flag                  */
#define US_TXRDY             (1uL << 1u)         /* Tx RDY Status flag              */
#define US_TXEMPTY           (1uL << 9u)         /* Tx EMPTY Status flag            */
#define USART_RX_ERROR_FLAGS (0xE0u)    /* Parity, framing, overrun error  */

#define US_TX_INT_FLAG       (US_TXRDY)

/*********************************************************************
*
*       USART0_Handler() OS USART interrupt handler
*       handles both, Rx and Tx interrupt
*/
void USART0_Handler(void);
void USART0_Handler(void) {
  volatile OS_U32 Dummy;
  int UsartStatus;
  OS_EnterNestableInterrupt();

  UsartStatus = US_CSR;                    /* Examine status register */
  do {
    if (UsartStatus & US_RXRDY) {          /* Data received?          */
      if (UsartStatus & USART_RX_ERROR_FLAGS) {  /* Any error ?       */
        Dummy = US_RHR;                    /* => Discard data         */
        US_CR = (1uL << 8);                /* RSTSTA: Reset Status Bits PARE, FRAME, OVRE and RXBRK */
        } else {
        OS_OnRx(US_RHR);                   /* Process actual byte     */
      }
    }
    if (UsartStatus & US_IMR & US_TX_INT_FLAG) { /* Check Tx status => Send next character */
      if (OS_OnTx()) {                     /* No more characters to send ?  */
        US_IDR = US_TX_INT_FLAG;           /* Disable further Tx interrupts */
      }
    }
    UsartStatus = US_CSR;                  /* Examine current status  */
  } while (UsartStatus & US_IMR & (US_TX_INT_FLAG | US_RXRDY));
  OS_LeaveNestableInterrupt();
}

/*********************************************************************
*
*       OS_COM_Send1()
*       Never call this function directly from your application
*/
void OS_COM_Send1(OS_U8 c) {
  while((US_CSR & US_TX_INT_FLAG) == 0) { // Wait until THR becomes available
  }
  US_THR  = c;
  US_IER  = US_TX_INT_FLAG; /* enable Tx interrupt */
}

/*********************************************************************
*
*       OS_COM_Init()
*       Initialize UART for OSView
*/
void OS_COM_Init(void) {                 /* Initialize UART, enable UART interrupts */
  OS_IncDI();
  PMC_WPMR      = 0x504D4300uL;          /* Disable write protect */
  PMC_PCER      = (1uL << USART0_ID);    /* Enable peripheral clock for selected USART */
  PMC_WPMR      = 0x504D4301uL;          /* Enable write protect */
  PIO_WPMR      = 0x50494F00;            /* Disable write protect */
  PIOB_PDR      = PIO_PDR_USART;         /* Enable peripheral output signals (disable PIO Port B) */
  PIOB_ABCDSR1 &= ~PIO_PDR_USART;        /* Write 0 to PIOB_P0 and PIOB_P1 in peripheral select register 1 */
  PIOB_ABCDSR2 |= PIO_PDR_USART;         /* and   1 to PIOB_P0 and PIOB_P1 in peripheral select register 2 to select function C*/
  PIO_WPMR      = 0x50494F01;            /* Enable write protect */
  US_CR         = (1uL <<  2) |          /* RSTRX: Reset Receiver: 1 = The receiver logic is reset. */
                  (1uL <<  3);           /* RSTTX: Reset Transmitter: 1 = The transmitter logic is reset. */
  US_CR         = (0uL <<  2) |          /* RSTRX: Reset Receiver: 1 = The receiver logic is reset. */
                  (0uL <<  3) |          /* RSTTX: Reset Transmitter: 1 = The transmitter logic is reset. */
                  (1uL <<  4) |          /* RXEN: Receiver Enable: 1 = The receiver is enabled if RXDIS is 0. */
                  (0uL <<  5) |          /* RXDIS: Receiver Disable: 0 = No effect. */
                  (1uL <<  6) |          /* TXEN: Transmitter Enable: 1 = The transmitter is enabled if TXDIS is 0. */
                  (0uL <<  7) |          /* TXDIS: Transmitter Disable: 0 = No effect. */
                  (1uL <<  8) |          /* RSTSTA: Reset Status Bits: 1 = Resets the status bits PARE, FRAME, OVRE and RXBRK in the US_CSR. */
                  (0uL <<  9) |          /* STTBRK: Start Break: 0 = No effect. */
                  (0uL << 10) |          /* STPBRK: Stop Break: 0 = No effect. */
                  (0uL << 11) |          /* STTTO: Start Time-out: 0 = No effect. */
                  (0uL << 12);           /* SENDA: Send Address: 0 = No effect. */
  US_MR         = (0uL <<  4) |          /* USCLKS: Clock Selection: 0 = MCK */
                  (3uL <<  6) |          /* CHRL: Character Length: 3 = Eight bits */
                  (0uL <<  8) |          /* SYNC: Synchronous Mode Select: 0 = USART operates in Asynchronous Mode. */
                  (0x4uL <<  9) |        /* PAR: Parity Type: 0x4 = No parity */
                  (0uL << 12) |          /* NBSTOP: Number of Stop Bits: 0 = 1 stop bit */
                  (0uL << 14) |          /* CHMODE: Channel Mode: 0 = Normal mode */
                  (0uL << 17) |          /* MODE9: 9-bit Character Length: 0 = CHRL defines character length. */
                  (0uL << 18);           /* CKLO: Clock Output Select: 0 = The USART does not drive the SCK pin. */
  US_BRGR       = (OS_BAUDDIVIDE);
  US_IDR        = 0xFFFFFFFFuL;          /* Disable all interrupts     */
  US_IER        = (1uL << 0)             /* Enable Rx Interrupt        */
                | (0uL << 1);            /* Do not Enable Tx Interrupt */
  //
  // Install USART Handler with preemtion level one above lowest level to ensure communication during PendSV
  //
  NVIC_SetPriority(USART0_IRQn, (1u << __NVIC_PRIO_BITS) - 2u);
  NVIC_EnableIRQ(USART0_IRQn);
  OS_DecRI();
}

#elif (OS_VIEW_IFSELECT == OS_VIEW_DISABLED)

void OS_COM_Send1(OS_U8 c) {
  OS_USEPARA(c);           /* Avoid compiler warning */
  OS_COM_ClearTxActive();  /* Let the OS know that Tx is not busy */
}
#else
  #error "Selected embOSView interface is currently not supported."
#endif

/****** End Of File *************************************************/
