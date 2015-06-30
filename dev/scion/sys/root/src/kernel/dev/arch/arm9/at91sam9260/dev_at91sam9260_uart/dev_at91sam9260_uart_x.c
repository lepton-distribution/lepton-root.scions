/*
The contents of this file are subject to the Mozilla Public License Version 1.1
(the "License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at http://www.mozilla.org/MPL/

Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for the
specific language governing rights and limitations under the License.

The Original Code is Lepton.

The Initial Developer of the Original Code is Chauvin-Arnoux.
Portions created by Chauvin-Arnoux are Copyright (C) 2011. All Rights Reserved.

Alternatively, the contents of this file may be used under the terms of the eCos GPL license
(the  [eCos GPL] License), in which case the provisions of [eCos GPL] License are applicable
instead of those above. If you wish to allow use of your version of this file only under the
terms of the [eCos GPL] License and not to allow others to use your version of this file under
the MPL, indicate your decision by deleting  the provisions above and replace
them with the notice and other provisions required by the [eCos GPL] License.
If you do not delete the provisions above, a recipient may use your version of this file under
either the MPL or the [eCos GPL] License."
*/

/**
 * \addtogroup lepton_dev
 * @{
 *
 */

/**
 * \addtogroup hard_dev_at91sam9260
 * @{
 *
 */

/**
 * \defgroup dev_uart_arm9_c Les uarts de l'arm9 at91sam9260
 * @{
 *
 * Les pilotes de priphriques pour les uarts de l'arm9 at91sam9260
 * et supporte le timeout (VTIME) pour le mode bloquant.
 *
 */


/**
 * \file
 * implementation du pilote de priphrique Uart generique de l'arm9 at91sam9260
 * a partir de l'uart_s0 d'origine (auteur phillipe le boulanger)
 * \author bruno mollo
 */

/*===========================================
Includes
=============================================*/
#include "kernel/core/types.h"
#include "kernel/core/interrupt.h"
#include "kernel/core/kernelconf.h"
#include "kernel/core/kernel.h"
#include "kernel/core/system.h"
#include "kernel/core/fcntl.h"
#include "kernel/core/cpu.h"
#include "kernel/core/core_rttimer.h"
#include "lib/libc/termios/termios.h"
#include "kernel/fs/vfs/vfsdev.h"

// drivers
#include "dev_at91sam9260_uart_x.h"
#include "kernel/dev/arch/arm9/at91sam9260/common/dev_at91sam9260_common_uart.h"

// eCos
#include "cyg/hal/at91sam9260.h"
#include <string.h>
#include <stdlib.h>


/*===========================================
Global Declaration
=============================================*/
static int termios2ttys(desc_t desc, struct termios* termios_p);
static s2s_t const s2s[] = {
   { B0,                0 },
   { B50,              50 },
   { B75,              75 },
   { B110,            110 },
   { B134,            134 },
   { B150,            150 },
   { B200,            200 },
   { B300,            300 },
   { B600,            600 },
   { B1200,          1200 },
   { B1800,          1800 },
   { B2400,          2400 },
   { B4800,          4800 },
   { B9600,          9600 },
   { B19200,        19200 },
   { B38400,        38400 },
   { B57600,        57600 },
   { B115200,      115200 },
   { B230400,      230400 },
   { B460800,      460800 }
};


/*===========================================
Implementation
=============================================*/

/*--------------------------------------------
| Name       : dev_at91sam9260_uart_x_fifo_rcv
| Description: Receive fifo
| Parameters : desc : descriptor
| Return Type: none
| Comments   : -
| See        : -
----------------------------------------------*/
void dev_at91sam9260_uart_x_fifo_rcv(desc_t desc)
{
   board_inf_uart_t *p_inf_uart;
   AT91S_USART *p_adr;
   int data=0;
   unsigned char rcv_data;

   p_inf_uart = (board_inf_uart_t *)ofile_lst[desc].p;
   p_adr      = (AT91S_USART *)p_inf_uart->base_adr;

   data       = p_adr->US_RHR;
   rcv_data   = (unsigned char) data;

   p_inf_uart->fifo_input_buffer[ p_inf_uart->input_w] = rcv_data;

   if ( (p_inf_uart->desc_rd >=0) &&
        (p_inf_uart->input_r == p_inf_uart->input_w))
   {
      //empty to not empty
      p_inf_uart->rcv_flag = 1;
   }

   if(++p_inf_uart->input_w == UART_FIFO_INPUT_BUFFER_SZ)
      p_inf_uart->input_w = 0;

   p_inf_uart->inter_char_timer = p_inf_uart->ttys_termios.c_cc[VTIME];
}


/*-------------------------------------------
| Name       : dev_at91sam9260_uart_x_snd
| Description: Send Data
| Parameters : desc : descriptor
| Return Type: None
| Comments   : -
| See        : -
---------------------------------------------*/
void dev_at91sam9260_uart_x_snd (desc_t desc)
{
   unsigned char snd_data;
   board_inf_uart_t *p_inf_uart;
   AT91S_USART         *p_adr;

   p_inf_uart = (board_inf_uart_t *)ofile_lst[desc].p;
   p_adr      = (AT91S_USART *)p_inf_uart->base_adr;

   if (p_inf_uart->output_r < 0)
      return;

   p_inf_uart->output_r++;

   // Byte to send?
   if (p_inf_uart->output_r < p_inf_uart->output_w) {
      snd_data = p_inf_uart->output_buffer[p_inf_uart->output_r];
      p_adr->US_THR = snd_data;
      p_adr->US_IER = IER_MASK_SND;

      // Not empty --> empty
   } else if ( (p_inf_uart->desc_wr  >= 0) &&
               (p_inf_uart->output_r == p_inf_uart->output_w) ) {
      //while (!((p_adr->US_CSR) & (0x00000200)));  // Wait until TX shift register empty

      // There are not characters anymore to be sent -> Prevent the calling thread
      p_adr->US_IDR = IER_MASK_SND;
      p_inf_uart->XMIT  = 0;
   }
}

/*-------------------------------------------
| Name       : dev_at91sam9260_uart_x_timer_callback
| Description: Generic timer callback function
| Parameters : board_inf_uart_t pointer
| Return Type: -
| Comments   : -
| See        : -
---------------------------------------------*/
void dev_at91sam9260_uart_x_timer_callback(alrm_hdl_t alarm_handle, cyg_addrword_t data)
{
   board_inf_uart_t *p_board_inf_uart = (board_inf_uart_t *)data;

   if( (p_board_inf_uart->desc_rd >= 0)               &&
       (p_board_inf_uart->ttys_termios.c_cc[VTIME])   &&
       (p_board_inf_uart->inter_char_timer)           &&
       !(--p_board_inf_uart->inter_char_timer) )
   {
      if (p_board_inf_uart->input_w == p_board_inf_uart->input_r) {
         __fire_io_int(ofile_lst[p_board_inf_uart->desc_rd].owner_pthread_ptr_read);
      }
   }

   rttmr_restart(&p_board_inf_uart->timer);
}


/*-------------------------------------------
| Name       :dev_at91sam9260_uart_x_load
| Description: Load device
| Parameters : p_board_inf_uart : info uart pointer
| Return Type: integer -> devive loaded (=0)
| Comments   : -
| See        : -
---------------------------------------------*/
int dev_at91sam9260_uart_x_load(board_inf_uart_t *p_board_inf_uart)
{
   pthread_mutexattr_t mutex_attr=0;

   //already loaded?
   if (p_board_inf_uart->loaded)
      return 0;  //yes

   p_board_inf_uart->desc_rd = -1;
   p_board_inf_uart->desc_wr = -1;

   cfmakeraw   (&p_board_inf_uart->ttys_termios);
   cfsetispeed (&p_board_inf_uart->ttys_termios,B9600);
   cfsetospeed (&p_board_inf_uart->ttys_termios,B9600);

   p_board_inf_uart->ttys_termios.c_iflag &= ~(IXOFF|IXON); // xon/xoff disable
   p_board_inf_uart->ttys_termios.c_cc[VTIME]   = 0;        // no timeout, blocking call

   //VTIME timer in units of 0.1 seconds (posix specification)
   p_board_inf_uart->timer_attr.tm_msec = 100;
   p_board_inf_uart->timer_attr.func = dev_at91sam9260_uart_x_timer_callback;
   p_board_inf_uart->timer_attr.data = (cyg_addrword_t)p_board_inf_uart;

   rttmr_create(&p_board_inf_uart->timer,&p_board_inf_uart->timer_attr);

   // Device loaded flag
   p_board_inf_uart->loaded = 1;
   p_board_inf_uart->o_flag = 0;

   // initializations
   p_board_inf_uart->XMIT             = 0;  // transmission flag
   p_board_inf_uart->inter_char_timer = 0;
   p_board_inf_uart->rcv_flag = 0;

   kernel_pthread_mutex_init(&p_board_inf_uart->mutex, &mutex_attr);
   return 0;
}


/*-------------------------------------------
| Name       : dev_at91sam9260_uart_x_open
| Description: Open device
| Parameters : desc   : descriptor
|              o_flag : integer (O_RDONLY or/and O_WRONLY device type)
| Return Type: integer (-1) already open
|                      (0) OK
| Comments   : -
| See        : -
---------------------------------------------*/
int dev_at91sam9260_uart_x_open(desc_t desc, int o_flag)
{
   board_inf_uart_t *p_inf_uart;
   AT91S_USART         *p_adr;
   p_inf_uart = (board_inf_uart_t *)ofile_lst[desc].p;
   p_adr      = (AT91S_USART *)p_inf_uart->base_adr;
   char _rcv_buf;

   // if device not opened (read/write)
   if ( (p_inf_uart->desc_rd < 0) && (p_inf_uart->desc_wr < 0) ) {
      // Disable all USART interrupts.
      p_adr->US_IDR   = 0xffffffff;

      // Enable peripheral clock for selected USART
      *AT91C_PMC_PCER = 1 << (p_inf_uart->periph_id);

      // Assign the I/O line to the Uart function
      *AT91C_PIOB_ASR = p_inf_uart->ind_pio_rxd | p_inf_uart->ind_pio_txd;

      // Disable PIO control of PIOA_/TXDx and PIOA_/RXDy.
      *AT91C_PIOB_PDR = p_inf_uart->ind_pio_rxd | p_inf_uart->ind_pio_txd;

      //
      p_adr->US_CR  =   AT91C_US_RSTRX |         /* Reset Receiver      */
                      AT91C_US_RSTTX | /* Reset Transmitter   */
                      AT91C_US_RXDIS | /* Receiver Disable    */
                      AT91C_US_TXDIS; /* Transmitter Disable */
      //
      p_adr->US_MR   =  AT91C_US_USMODE_NORMAL |  /* Normal Mode */
                       AT91C_US_CLKS_CLOCK    | /* Clock = MCK */
                       AT91C_US_CHRL_8_BITS   | /* 8-bit Data  */
                       AT91C_US_PAR_NONE      | /* No Parity   */
                       AT91C_US_NBSTOP_1_BIT; /* 1 Stop Bit  */

      // Set baud rate.
      p_adr->US_BRGR = AT91_US_BAUD(OS_RS232_BAUDRATE); //see at91sam9260_misc.c

      // Reset status bits, reset rx/tx.
      p_adr->US_CR  = AT91C_US_RSTSTA;
      p_adr->US_CR  = AT91C_US_RXEN  |          /* Receiver Enable     */
                      AT91C_US_TXEN; /* Transmitter Enable  */

      p_adr->US_IER = IER_MASK_RCV;
   }
   // Read only Device opening
   if (o_flag & O_RDONLY) {

      if (p_inf_uart->desc_rd >= 0)
         return -1;  //already open: exclusive resource.

      p_inf_uart->input_r = 0; // used for Application layer
      p_inf_uart->input_w = 0; // used in interrupt
      p_inf_uart->desc_rd = desc;
      p_inf_uart->ttys_termios.c_cc[VTIME] = 0; // no timeout, blocking call
      p_inf_uart->inter_char_timer = 0;
   }

   // Write only Device opening
   if (o_flag & O_WRONLY) {
      if (p_inf_uart->desc_wr >= 0)
         return -1;  //already open: exclusive resource.

      p_inf_uart->output_r = -1;
      p_inf_uart->output_w = 0;
      p_inf_uart->desc_wr  = desc;
   }
   p_inf_uart->o_flag |= o_flag; // set o_flag
   return 0;
}


/*-------------------------------------------
| Name       : dev_at91sam9260_uart_x_close
| Description: Called by the kernel
| Parameters : desc : descriptor
| Return Type: integer
| Comments   : -
| See        : -
---------------------------------------------*/
int dev_at91sam9260_uart_x_close(desc_t desc)
{
   board_inf_uart_t *p_inf_uart;
   AT91S_USART *p_adr;

   p_inf_uart = (board_inf_uart_t *)ofile_lst[desc].p;
   p_adr      = (AT91S_USART *)p_inf_uart->base_adr;

   if(ofile_lst[desc].oflag & O_RDONLY) {
      // If several process are created by the kernel
      if(!ofile_lst[desc].nb_reader) {
         // Disable Interrupt on RXRDY
         p_adr->US_IDR = IER_MASK_RCV;
         //stop inter char timer
         rttmr_stop(&p_inf_uart->timer);
         p_inf_uart->ttys_termios.c_cc[VTIME] = 0; // no timeout, blocking call
         p_inf_uart->inter_char_timer = 0;
         p_inf_uart->desc_rd = -1;
      }
   }

   if(ofile_lst[desc].oflag & O_WRONLY) {
      // If several process are created by the kernel
      if(!ofile_lst[desc].nb_writer) {
         // Disable Interrupt on TXRDY
         p_adr->US_IDR = IER_MASK_SND;
         p_inf_uart->desc_wr = -1;
      }
   }

   //close all
   if ( (p_inf_uart->desc_wr < 0) && (p_inf_uart->desc_rd < 0) ) {
      p_adr->US_CR = AT91C_US_TXDIS | AT91C_US_RXDIS; // Disable receiver, disable transmitter
      p_adr->US_CR = AT91C_US_RSTRX | AT91C_US_RSTTX | AT91C_US_RSTSTA; // Reset status bits, reset rx/tx

      // disable peripheral clock for selected USART0
      *AT91C_PMC_PCDR =  1 << p_inf_uart->periph_id;
   }
   return 0;
}

/*-------------------------------------------
| Name       :dev_at91sam9260_uart_x_isset_read
| Description: Called by Read Posix interface
| Parameters : desc  : descriptor
| Return Type: integer (0)  : wait incomming data
|                      (-1) : end of waiting
| Comments   : -
| See        : -
---------------------------------------------*/
int dev_at91sam9260_uart_x_isset_read (desc_t desc)
{
   board_inf_uart_t *p_inf_uart = (board_inf_uart_t *)ofile_lst[desc].p;

   // if existing data
   if ( (p_inf_uart->input_w != p_inf_uart->input_r) ||
        (p_inf_uart->ttys_termios.c_cc[VTIME] && !p_inf_uart->inter_char_timer) ) {
      p_inf_uart->inter_char_timer = p_inf_uart->ttys_termios.c_cc[VTIME]; // load value
      return 0;  //wait incomming data
   } else {
      return -1;
   }
}

/*-------------------------------------------
| Name       : dev_at91sam9260_uart_x_isset_write
| Description: Called by write function
| Parameters : desc  : descriptor
| Return Type: int
| Comments   :
| See        :
---------------------------------------------*/
int dev_at91sam9260_uart_x_isset_write (desc_t desc)
{
   board_inf_uart_t *p_inf_uart = (board_inf_uart_t *)ofile_lst[desc].p;

   if (p_inf_uart->output_w == p_inf_uart->output_r) {
      p_inf_uart->output_r = -1;
      return 0;
   } else {
      return -1;
   }
}

/*-------------------------------------------
| Name       : dev_at91sam9260_uart_x_read
| Description: Called by Read Posix interface
| Parameters : desc : descriptor
|              buf  : pointer on read buffer
|              size : size
| Return Type: integer cb: number of bytes readed
| Comments   : -
| See        : -
---------------------------------------------*/
int dev_at91sam9260_uart_x_read(desc_t desc, char* buf,int size)
{
   board_inf_uart_t *p_inf_uart = (board_inf_uart_t *)ofile_lst[desc].p;
   int r   = p_inf_uart->input_r;
   int w   = p_inf_uart->input_w;
   int cb  = 0;

   kernel_pthread_mutex_lock (&p_inf_uart->mutex);

   for(cb=0; ((r!=w) && (cb<size)); cb++) {
      buf[cb] = p_inf_uart->fifo_input_buffer[r];

      if(++r == UART_FIFO_INPUT_BUFFER_SZ)
         r = 0;
   }
   p_inf_uart->input_r = r;

   kernel_pthread_mutex_unlock (&p_inf_uart->mutex);
   return cb;
}

/*-------------------------------------------
| Name       :dev_at91sam9260_uart_x_write
| Description: Write buffer
|              Called by Write Posix Interface
| Parameters : descriptor (desc_t)
|              buffer adress (char *)
|              buffer size
| Return Type: Integer
| Comments   : Send (byte per byte)
| See        : -
---------------------------------------------*/
int dev_at91sam9260_uart_x_write(desc_t desc, const char* buf,int size)
{
   unsigned char snd_data;
   board_inf_uart_t *p_inf_uart;
   AT91S_USART *p_adr;

   p_inf_uart = (board_inf_uart_t *)ofile_lst[desc].p;
   p_adr      = (AT91S_USART *)p_inf_uart->base_adr;

   kernel_pthread_mutex_lock  (&p_inf_uart->mutex);

   // truncated size
   if (size >= UART_OUTPUT_BUFFER_SIZE)
      size = UART_OUTPUT_BUFFER_SIZE-1;

   memcpy(p_inf_uart->output_buffer, buf, size);

   //buffer ready
   snd_data = p_inf_uart->output_buffer[0];

   while (!((p_adr->US_CSR) & (0x00000002))) ;   // Wait until THR empty

   p_inf_uart->output_r = 0;
   p_inf_uart->output_w = size;

   __clr_irq();

   p_inf_uart->XMIT = 1;

   // Byte per byte
   p_adr->US_THR = snd_data;
   p_adr->US_IER = IER_MASK_SND;

   __set_irq();

   kernel_pthread_mutex_unlock(&p_inf_uart->mutex);

   return size;
}

/*-------------------------------------------
| Name       :dev_at91sam9260_uart_x_seek
| Description: -
| Parameters : -
| Return Type: -
| Comments   : N/A
| See        : -
---------------------------------------------*/
int dev_at91sam9260_uart_x_seek(desc_t desc,int offset,int origin)
{
   return 0;
}

/*-------------------------------------------
| Name       :dev_at91sam9260_uart_x_ioctl
| Description: Called by ioctl
| Parameters :
|             descriptor desc_t  desc
|             int     request // $BM
|             va_list ap
|
| Return Type: int (-1 :)
| Comments   :
| See        :
---------------------------------------------*/
int dev_at91sam9260_uart_x_ioctl(desc_t desc, int request, va_list ap)
{
   struct termios* termios_p = (struct termios*)0;
   board_inf_uart_t *p_inf_uart;
   AT91S_USART *p_adr;

   p_inf_uart = (board_inf_uart_t *)ofile_lst[desc].p;
   p_adr      = (AT91S_USART *)p_inf_uart->base_adr;

   switch (request)
   {
   case TIOCSSERIAL: {
      unsigned long speed = va_arg( ap, unsigned long);
      if (speed==0) return -1;
      // Set baud rate.
      p_adr->US_BRGR = AT91_US_BAUD(speed);
   }
   break;

   /* If optional_actions is TCSANOW, the change will occur immediately.   */
   /* If optional_actions is TCSADRAIN, the change will occur after all
         output written to fildes is transmitted.
  This function should be used when changing parameters that affect output.*/
   /* If optional_actions is TCSAFLUSH, the change will occur after all
      output written to fildes is transmitted, and all input so far received
      but not read will be discarded before the change is made.
      return 0;*/
   case TCSETS:
   case TCSAFLUSH:
   case TCSADRAIN:
   case TCSANOW:
      termios_p = va_arg( ap, struct termios*);
      if (!termios_p) return -1;
      termios2ttys(desc,termios_p);
      break;
   case TCGETS:
      termios_p = va_arg( ap, struct termios*);
      if(!termios_p) return -1;
      memcpy(termios_p,&p_inf_uart->ttys_termios,sizeof(struct termios));
      break;
   //
   default:
      return -1;
   }

   return 0;
}


/*-------------------------------------------
| Name       :termios2ttys
| Description: called by dev_at91sam9260_uart_x_ioctl
| Parameters : descriptor
|              struct termios *
| Return Type: integer
| Comments   : -
| See        : -
---------------------------------------------*/
static int termios2ttys(desc_t desc, struct termios* termios_p)
{
   const struct s2s *sp;
   long n_speed;
   speed_t speed;
   board_inf_uart_t *p_inf_uart;
   AT91S_USART    *p_adr;

   p_inf_uart = (board_inf_uart_t *)ofile_lst[desc].p;
   p_adr      = (AT91S_USART *)p_inf_uart->base_adr;

   //speed
   speed = cfgetospeed(termios_p);
   for (sp = s2s; sp < s2s + (sizeof(s2s) / sizeof(s2s[0])); sp++) {
      if (sp->ts == speed) {
         n_speed = sp->ns;
         // Set baud rate
         p_adr->US_BRGR = AT91_US_BAUD(n_speed);
         break;
      }
   }

   //timeout read in blocking call
   if(termios_p->c_cc[VTIME]) {
      //OS_StopTimer (&p_inf_uart->timer);
      rttmr_stop(&p_inf_uart->timer);
      p_inf_uart->inter_char_timer = termios_p->c_cc[VTIME];
      //OS_RetriggerTimer (&p_inf_uart->timer);
      rttmr_restart(&p_inf_uart->timer);
   } else {
      //OS_StopTimer (&p_inf_uart->timer);
      rttmr_stop(&p_inf_uart->timer);
   }

   //
   memcpy (&p_inf_uart->ttys_termios,termios_p, sizeof(struct termios));
   return 0;
}


/** @} */
/** @} */
/** @} */

/*============================================
| End of Source  : dev_at91sam9260_uart_x.c
==============================================*/

