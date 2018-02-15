/*
The contents of this file are subject to the Mozilla Public License Version 1.1
(the "License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at http://www.mozilla.org/MPL/

Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for the
specific language governing rights and limitations under the License.

The Original Code is Lepton.

The Initial Developer of the Original Code is Philippe Le Boulanger.
Portions created by Philippe Le Boulanger are Copyright (C) 2017. All Rights Reserved.

Alternatively, the contents of this file may be used under the terms of the eCos GPL license
(the  [eCos GPL] License), in which case the provisions of [eCos GPL] License are applicable
instead of those above. If you wish to allow use of your version of this file only under the
terms of the [eCos GPL] License and not to allow others to use your version of this file under
the MPL, indicate your decision by deleting  the provisions above and replace
them with the notice and other provisions required by the [eCos GPL] License.
If you do not delete the provisions above, a recipient may use your version of this file under
either the MPL or the [eCos GPL] License."
*/

/* Asysnchrone message from modem
lepton#2$ modem socket

AT+CIFSR
10.139.142.126
AT+CIPSTART="TCP","91.121.101.217","15555"
OK

CONNECT OK

AT+CIPCLOSE=0
CLOSE OK

+CGREG: 5

+CGREG: 5

+CGREG: 5

+CGREG: 5

+PDP: DEACT
*/

/*============================================
| Includes
==============================================*/
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "kernel/core/kernelconf.h"
#include "kernel/core/limits.h"
#include "kernel/core/dirent.h"
#include "kernel/core/errno.h"
#include "kernel/core/system.h"
#include "kernel/core/process.h"
#include "kernel/core/kernel.h"
#include "kernel/core/fcntl.h"
#include "kernel/core/stat.h"
#include "kernel/core/kernel_io.h"
#include "kernel/core/malloc.h"

#include "kernel/core/kernel_pthread_mutex.h"
#include "kernel/core/kernel_sem.h"
#include "kernel/core/kernel_ring_buffer.h"
#include "kernel/core/kernel_mqueue.h"

#include "kernel/core/ioctl.h"
#include "kernel/core/ioctl_board.h"
#include "kernel/core/ioctl_modem.h"

#include "kernel/fs/vfs/vfstypes.h"
#include "kernel/fs/vfs/vfskernel.h"
#include "kernel/fs/vfs/vfs.h"

#include "kernel/core/kernel_device.h"
#include "kernel/core/kernel_printk.h"

#include "kernel/core/net/modem_core/modem_socket.h"
#include "kernel/core/net/modem_core/modem_core.h"
#include "kernel/core/net/kernel_net_core_socket.h"

#include "kernel/core/net/bsd/netdb.h"

#include "lib/libc/termios/termios.h"
#include "lib/libc/misc/ltostr.h"

#include "kernel/dev/arch/all/modem/ublox/dev_modem_ublox_sarag3.h"

/*============================================
| Global Declaration
==============================================*/

// to do:  see +UNFM for test.
//
static modem_at_command_t const modem_at_command_list[] = {
   { .command = "ATV1",.response = (const char*)0 },
   { .command = "ATE0",.response = (const char*)0 },
   { .command = "AT+CMEE=1",.response = (const char*)0 },
   { .command = "AT+CGREG?",.response = (const char*)0 },
   { .command = "AT+COPS?",.response = (const char*)0 },
   { .command = "AT+CGATT=1",.response = (const char*)0 },
   { .command = "AT+CGREG=1",.response = (const char*)0 },
   { .command = "AT+UPSD=0,0,0",.response = (const char*)0 }, // set IPv4
   { .command = "AT+UPSD=0,1,\"soracom.io\"",.response = (const char*)0 }, //set APN
   { .command = "AT+UPSD=0,2,\"sora\"",.response = (const char*)0 }, //set username
   { .command = "AT+UPSD=0,3,\"sora\"",.response = (const char*)0 }, //set password
   { .command = "AT+UPSDA=0,3",.response = (const char*)0 }, //set Internal PDP context activation
   { .command = "AT+UPSND=0,0",.response = (const char*)0 },  //get IP address
   { .command = "AT+UDCONF=1,1",.response = (const char*)0 },  //set HEX mode for socket read and write operation
   { .command = "AT+ULOC=2,2,2,240,1,1",.response = (const char*)0 }  //GSM localisation
};


static modem_at_command_t const stop_modem_at_command_list[] = {
   { .command = "AT+CPWROFF",.response = (const char*)0 }
};

//
static const char at_cmd1[] = "AT\r\n";

//
static int dev_modem_ublox_sarag3_load(void);
static int dev_modem_ublox_sarag3_open(desc_t desc, int o_flag);
static int dev_modem_ublox_sarag3_close(desc_t desc);
static int dev_modem_ublox_sarag3_isset_read(desc_t desc);
static int dev_modem_ublox_sarag3_isset_write(desc_t desc);
static int dev_modem_ublox_sarag3_read(desc_t desc, char* buffer, int nbyte);
static int dev_modem_ublox_sarag3_write(desc_t desc, const char* buffer, int nbyte);
static int dev_modem_ublox_sarag3_read_args(desc_t desc, char* buffer, int len, va_list ap);
static int dev_modem_ublox_sarag3_write_args(desc_t desc, const char* buffer, int len, va_list ap);
static int dev_modem_ublox_sarag3_seek(desc_t desc, int offset, int origin);
static int dev_modem_ublox_sarag3_ioctl(desc_t desc, int request, va_list ap);

static const char dev_modem_ublox_sarag3_name[] = "sarag3";

dev_map_t  dev_modem_ublox_sarag3_map = {
   .dev_name = dev_modem_ublox_sarag3_name,
   .dev_attr = S_IFCHR,
   .fdev_load = dev_modem_ublox_sarag3_load,
   .fdev_open = dev_modem_ublox_sarag3_open,
   .fdev_close = dev_modem_ublox_sarag3_close,
   .fdev_isset_read = dev_modem_ublox_sarag3_isset_read,
   .fdev_isset_write = dev_modem_ublox_sarag3_isset_write,
   .fdev_read = dev_modem_ublox_sarag3_read,
   .fdev_write = dev_modem_ublox_sarag3_write,
   .fdev_read_args = __fdev_not_implemented,
   .fdev_write_args = __fdev_not_implemented,
   .fdev_seek = __fdev_not_implemented,
   .fdev_ioctl = dev_modem_ublox_sarag3_ioctl
};


static int ublox_sarag3_at_response_callback_create(void* pv_modem_core_info, char* at_response);
static int ublox_sarag3_at_response_callback_close(void* pv_modem_core_info, char* at_response);
static int ublox_sarag3_at_response_callback_send(void* pv_modem_core_info, char* at_response);

static int ublox_sarag3_at_response_callback_async_receive(void* pv_modem_core_info, char* at_response);
static int ublox_sarag3_at_response_callback_receive(void* pv_modem_core_info, char* at_response);

static int ublox_sarag3_at_response_callback_gethostbyname(void* pv_modem_core_info, char* at_response);

static int ublox_sarag3_at_response_callback_localisation(void* pv_modem_core_info, char* at_response);

//
static const modem_at_parser_response_callback_t modem_ublox_sarag3_at_parser_response_callback_list[] = {
   { .at_response = "+USOCR:",.at_response_callback = ublox_sarag3_at_response_callback_create },
   { .at_response = "+UUSOCL:",.at_response_callback = ublox_sarag3_at_response_callback_close },
   //{ .at_response = "+UUSOCO:",.at_response_callback = ublox_sarag3_at_response_callback_connect },// not supportred for SARA-G3
   { .at_response = "+USOWR:",.at_response_callback = ublox_sarag3_at_response_callback_send },
   { .at_response = "+UUSORD:",.at_response_callback = ublox_sarag3_at_response_callback_async_receive },
   { .at_response = "+USORD:",.at_response_callback = ublox_sarag3_at_response_callback_receive },
   
   { .at_response = "+UDNSRN:",.at_response_callback = ublox_sarag3_at_response_callback_gethostbyname },
   { .at_response = "+UULOC:",.at_response_callback = ublox_sarag3_at_response_callback_localisation }
   
   //to "+UUPSDD: 0" close all socket connection
};

//
static int ublox_sarag3_at_command_modem_reset(void* pv_modem_core_info, void * data);
static int ublox_sarag3_at_command_modem_init(void* pv_modem_core_info, void * data);
static int ublox_sarag3_at_command_modem_stop(void* pv_modem_core_info, void * data);
static int ublox_sarag3_at_command_socket_connect(void* pv_modem_core_info, modem_core_connection_info_t* p_modem_core_connection_info);
static int ublox_sarag3_at_command_socket_close(void* pv_modem_core_info, modem_core_connection_info_t* p_modem_core_connection_info);
static int ublox_sarag3_at_command_socket_send(void* pv_modem_core_info, modem_core_connection_info_t* p_modem_core_connection_info);
static int ublox_sarag3_at_command_gethostbyname(void* pv_modem_core_info, void* data);

//
static dev_modem_info_t dev_modem_ublox_sarag3_info = {
   .desc_r = INVALID_DESC,
   .desc_w = INVALID_DESC,
   .at_response_callback_list_size= sizeof(modem_ublox_sarag3_at_parser_response_callback_list)/sizeof(modem_at_parser_response_callback_t),
   .at_response_callback_list =( modem_at_parser_response_callback_t*) modem_ublox_sarag3_at_parser_response_callback_list,
   .modem_at_command_op.at_command_modem_reset     = ublox_sarag3_at_command_modem_reset,
   .modem_at_command_op.at_command_modem_init      = ublox_sarag3_at_command_modem_init,
   .modem_at_command_op.at_command_modem_stop      = ublox_sarag3_at_command_modem_stop,
   .modem_at_command_op.at_command_socket_create   = __pfn_at_command_not_implemented,
   .modem_at_command_op.at_command_socket_connect  = ublox_sarag3_at_command_socket_connect,
   .modem_at_command_op.at_command_socket_close    = ublox_sarag3_at_command_socket_close,
   .modem_at_command_op.at_command_socket_send     = ublox_sarag3_at_command_socket_send,
   .modem_at_command_op.at_command_socket_receive  = __pfn_at_command_not_implemented,
   .modem_at_command_op.at_command_gethostbyname   = ublox_sarag3_at_command_gethostbyname
};

//
#define UBLOX_SOCKET_DATA_LENGTH_MAX 250 //64 //250
#define UBLOX_SOCKET_DATA_HEXASCII_LENGTH_MAX (UBLOX_SOCKET_DATA_LENGTH_MAX*2)
#ifndef UBLOX_SOCKET_MAX
   #define UBLOX_SOCKET_MAX 6
#endif

typedef struct ublox_socket_info_st{
   int  ublox_socket_connexion_index;
   modem_core_connection_info_t* p_modem_core_connection_info;
}ublox_socket_info_t;

static ublox_socket_info_t g_ublox_sarag3_socket_list[UBLOX_SOCKET_MAX] = { 0 };

//
static uint8_t ublox_hexascii_buffer_send[UBLOX_SOCKET_DATA_HEXASCII_LENGTH_MAX];
static uint8_t ublox_hexascii_buffer_recv[UBLOX_SOCKET_DATA_HEXASCII_LENGTH_MAX];
//
static const uint8_t * hex_conversion_array = "0123456789ABCDEF";

//
static modem_ublox_sarag3_info_t g_modem_ublox_sarag3_info;

/*============================================
| Implementation
==============================================*/

/*--------------------------------------------
| Name:        ublox_sarag3_get_ublox_socket_connexion_index
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int ublox_sarag3_get_ublox_socket_connexion_init(void){
   for(int ublox_socket_connexion_index=0;
       ublox_socket_connexion_index<UBLOX_SOCKET_MAX;
       ublox_socket_connexion_index++){
      g_ublox_sarag3_socket_list[ublox_socket_connexion_index].ublox_socket_connexion_index=ublox_socket_connexion_index;
      g_ublox_sarag3_socket_list[ublox_socket_connexion_index].p_modem_core_connection_info= (modem_core_connection_info_t*)0;
   }
   //
   return 0;
}

/*--------------------------------------------
| Name:        ublox_sarag3_get_ublox_socket_connexion_index
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int ublox_sarag3_get_ublox_socket_connexion_index(modem_core_connection_info_t* p_modem_core_connection_info) {
   ublox_socket_info_t* p_ublox_socket_info = (ublox_socket_info_t*)p_modem_core_connection_info->pv_modem_specific_info;
   if(p_ublox_socket_info==(ublox_socket_info_t*)0){
      return -1;
   }
   //
   return p_ublox_socket_info->ublox_socket_connexion_index;
}

/*--------------------------------------------
| Name:        ublox_sarag3_at_response_callback_create
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int ublox_sarag3_at_response_callback_create(void* pv_modem_core_info, char* at_response) {
   struct modem_core_info_st* p_modem_core_info = (struct modem_core_info_st*)pv_modem_core_info;
   modem_core_connection_info_t* p_modem_core_connection_info;
   char* pbuf = at_response;
   int ublox_socket_connexion_index;
   //
   if(p_modem_core_info->modem_core_message.operation_request_code!= MODEM_CORE_OPERATION_SOCKET_CONNECT_REQUEST
      && p_modem_core_info->modem_core_message.operation_request_code!= MODEM_CORE_OPERATION_INPROCESS){
      return -1;
   }
   //
   p_modem_core_connection_info = (modem_core_connection_info_t*)p_modem_core_info->modem_core_message.p_modem_core_connection_info;
   
   //get ublox sara g3 internal socket number
   //get +USOCR:
   while (*pbuf != ':' && *pbuf != '\0') {
      pbuf++;
   }
   //
   if (*pbuf == '\0') {
      return -1;
   }
   //
   pbuf++;

   //get connection number
   at_response = pbuf;
   //
   while (*pbuf != ',' && *pbuf != '\0') {
      pbuf++;
   }
   //
   *pbuf = '\0';
   //
   ublox_socket_connexion_index = atoi(at_response);
   
   //
   g_ublox_sarag3_socket_list[ublox_socket_connexion_index].p_modem_core_connection_info = p_modem_core_connection_info;
   //attach modem core connection with ublox connection info
   p_modem_core_connection_info->pv_modem_specific_info=&g_ublox_sarag3_socket_list[ublox_socket_connexion_index];
   
   return 0;
}

/*--------------------------------------------
| Name:        ublox_sarag3_at_response_callback_close_ok
| Description:
| Parameters:  none
| Return Type: none
| Comments: UUSOCL: <socket>
| See:
----------------------------------------------*/
static int ublox_sarag3_at_response_callback_close(void* pv_modem_core_info, char* at_response) {
   struct modem_core_info_st* p_modem_core_info = (struct modem_core_info_st*)pv_modem_core_info;
   modem_core_connection_info_t* p_modem_core_connection_info;
   char* pbuf = at_response;
   int ublox_socket_connexion_index;

   //
   //get ublox sara g3 internal socket number
   //UUSOCL: <socket>
   while (*pbuf != ':' && *pbuf != '\0') {
      pbuf++;
   }
   //
   if (*pbuf == '\0') {
      return -1;
   }
   //
   pbuf++;

   //get connection number
   at_response = pbuf;
   //
   while (*pbuf != '\0') {
      pbuf++;
   }
   //
   ublox_socket_connexion_index = atoi(at_response);
   //
   p_modem_core_connection_info = g_ublox_sarag3_socket_list[ublox_socket_connexion_index].p_modem_core_connection_info;
   //
   if (p_modem_core_connection_info == (void*)0) {
      return -1;
   }
   //check coherence: zombie connection
   if(ublox_socket_connexion_index != ublox_sarag3_get_ublox_socket_connexion_index(p_modem_core_connection_info)){
      return -1; //zombie connection socket close, but ublox was still alive, now is realy close in the both side: modem_core side and ublox side.
   }
   
   //
   if(p_modem_core_connection_info->status == CONNECTION_STATUS_SHUTDOWN || p_modem_core_connection_info->status == CONNECTION_STATUS_CLOSE){
      return -1;
   }
   
   // set status to shutdown.
   p_modem_core_connection_info->status = CONNECTION_STATUS_SHUTDOWN;
   //

   //to do: fire io to user level thread if it's blocked on read operation (__wait_io). 
   //See if this modification must do in kerner_io_read and in vfs_close if pthread_ower_read and pthread_ower_write 
   //are different of pthread that make syscall 
   desc_t desc_r = p_modem_core_connection_info->socket_desc;
   __fire_io(ofile_lst[desc_r].owner_pthread_ptr_read);

   //
   return 0;
}


/*--------------------------------------------
| Name:        ublox_sarag3_at_response_callback_send
| Description:
| Parameters:  none
| Return Type: none
| Comments: 
|        +USOWR: <socket>,<length>
|        OK
| See:
----------------------------------------------*/
static int ublox_sarag3_at_response_callback_send(void* pv_modem_core_info,char* at_response) {
   struct modem_core_info_st* p_modem_core_info = (struct modem_core_info_st*)pv_modem_core_info;
   modem_core_connection_info_t* p_modem_core_connection_info;
   char* pbuf = at_response;
   int ublox_socket_connexion_index;
   int data_sent_length;

 
   //get +USOWR:
   while (*pbuf != ':' && *pbuf != '\0') {
      pbuf++;
   }
   //
   if (*pbuf == '\0') {
      return -1;
   }
   //
   pbuf++;

   //get socket connection number
   at_response = pbuf;
   //
   while (*pbuf != ',' && *pbuf != '\0') {
      pbuf++;
   }
   //
   if (*pbuf == '\0') {
      return -1;
   }
   *pbuf = '\0';
   //
   pbuf++;
   //
   ublox_socket_connexion_index = atoi(at_response);
  

   //get data sent length
   at_response = pbuf;
   //
   while (*pbuf != ',' && *pbuf != '\0') {
      pbuf++;
   }
   //
   *pbuf = '\0';
   //
   data_sent_length= atoi(at_response);
   
   //
   p_modem_core_connection_info = g_ublox_sarag3_socket_list[ublox_socket_connexion_index].p_modem_core_connection_info;
   //
   if (p_modem_core_connection_info == (void*)0) {
      return -1;
   }
   //check coherence: zombie connection
   if(ublox_socket_connexion_index != ublox_sarag3_get_ublox_socket_connexion_index(p_modem_core_connection_info)){
      return -1; //zombie connection socket close, but ublox was still alive
   }

   //
   if (p_modem_core_connection_info->status != CONNECTION_STATUS_CONNECTED) {
      return -1;
   }
   //
   p_modem_core_connection_info->snd_packet.len = data_sent_length;
   //
   return data_sent_length;
}


/*--------------------------------------------
| Name:        ublox_sarag3_at_response_callback_async_receive
| Description:
| Parameters:  none
| Return Type: none
| Comments: +UUSORD: <socket>,<length>
| See:
----------------------------------------------*/
static int ublox_sarag3_at_response_callback_async_receive(void* pv_modem_core_info,char* at_response) {
   struct modem_core_info_st* p_modem_core_info = (struct modem_core_info_st*)pv_modem_core_info;
   modem_core_connection_info_t* p_modem_core_connection_info;
   char* pbuf = at_response;
   char ltostr_buffer[4];
   int ublox_socket_connexion_index;
   int data_received_length;
  

   //get +UUSORD:
   while (*pbuf != ':' && *pbuf != '\0') {
      pbuf++;
   }
   //
   if (*pbuf == '\0') {
      return -1;
   }
   //
   pbuf++;

   //get socket connection number
   at_response = pbuf;
   //
   while (*pbuf != ',' && *pbuf != '\0') {
      pbuf++;
   }
   //
   if (*pbuf == '\0') {
      return -1;
   }
   *pbuf = '\0';
   //
   pbuf++;
   //
   ublox_socket_connexion_index = atoi(at_response);


   //get data receive length
   at_response = pbuf;
   //
   while (*pbuf != ',' && *pbuf != '\0') {
      pbuf++;
   }
   //
   *pbuf = '\0';
   //
   data_received_length = atoi(at_response);
   

   //
   p_modem_core_connection_info = g_ublox_sarag3_socket_list[ublox_socket_connexion_index].p_modem_core_connection_info;
   //
   if (p_modem_core_connection_info == (void*)0) {
      return -1;
   }
   //check coherence: zombie connection
   if(ublox_socket_connexion_index != ublox_sarag3_get_ublox_socket_connexion_index(p_modem_core_connection_info)){
      return -1; //zombie connection socket close, but ublox was still alive.
   }

   //
   if (p_modem_core_connection_info->status != CONNECTION_STATUS_CONNECTED) {
      return -1;
   }
   //
   if (data_received_length <= 0) {
      return 0;
   }
   //
   if (data_received_length > (p_modem_core_info->modem_at_parser_context.response_buffer_size)/2) {
      data_received_length = (p_modem_core_info->modem_at_parser_context.response_buffer_size) / 2;
   }


   //AT+USORD=<socket>,<length>
   //
   memset(g_at_send_buffer, 0, sizeof(g_at_send_buffer));
   //at command
   strcpy(g_at_send_buffer, "AT+USORD=");
   //connection number
   strcat(g_at_send_buffer, ltostr(ltostr_buffer, sizeof(ltostr_buffer), ublox_socket_connexion_index, 10, 0));
   //
   strcat(g_at_send_buffer, ",");
   //
   strcat(g_at_send_buffer, ltostr(ltostr_buffer, sizeof(ltostr_buffer), data_received_length, 10, 0));
   //
   modem_core_parser_send_at_command(p_modem_core_info, g_at_send_buffer, __KERNEL_MODEM_CORE_SILENT_MODE);
   
   //
   return 0;
}

/*--------------------------------------------
| Name:        ublox_sarag3_at_response_callback_receive
| Description:
| Parameters:  none
| Return Type: none
| Comments: +USORD: <socket>,<length>,"<data>"
| See:
----------------------------------------------*/
static int ublox_sarag3_at_response_callback_receive(void* pv_modem_core_info, char* at_response) {
   struct modem_core_info_st* p_modem_core_info = (struct modem_core_info_st*)pv_modem_core_info;
   modem_core_connection_info_t* p_modem_core_connection_info;
   char* pbuf = at_response;
   int ublox_socket_connexion_index;
   int data_received_length;
   uint8_t flag_signal = 0;
   modem_core_recv_packet_parameters_t* p_recv_packet;

   //get +USORD:
   while (*pbuf != ':' && *pbuf != '\0') {
      pbuf++;
   }
   //
   if (*pbuf == '\0') {
      return -1;
   }
   //
   pbuf++;

   //get socket connection number
   at_response = pbuf;
   //
   while (*pbuf != ',' && *pbuf != '\0') {
      pbuf++;
   }
   //
   if (*pbuf == '\0') {
      return -1;
   }
   *pbuf = '\0';
   //
   pbuf++;
   //
   ublox_socket_connexion_index = atoi(at_response);


   //get data receive length
   at_response = pbuf;
   //
   while (*pbuf != ',' && *pbuf != '\0') {
      pbuf++;
   }
   //
   if (*pbuf == '\0') {
      return -1;
   }
   //
   *pbuf = '\0';
   //
   pbuf++;
   //
   data_received_length = atoi(at_response);
   
   //get first quote '"' begin data delimiter
   while (*pbuf != '\"' && *pbuf != '\0') {
      pbuf++;
   }
   //
   if (*pbuf == '\0') {
      return -1;
   }
   //
   pbuf++;

   //
   p_modem_core_connection_info = g_ublox_sarag3_socket_list[ublox_socket_connexion_index].p_modem_core_connection_info;
   //
   if (p_modem_core_connection_info == (void*)0) {
      return -1;
   }
   //check coherence: zombie connection
   if(ublox_socket_connexion_index != ublox_sarag3_get_ublox_socket_connexion_index(p_modem_core_connection_info)){
      return -1; //zombie connection socket close, but ublox was still alive
   }

   //
   if (p_modem_core_connection_info->status != CONNECTION_STATUS_CONNECTED) {
      return -1;
   }
   //
   if (data_received_length <= 0) {
      return 0;
   }
   //
   if (data_received_length > (p_modem_core_info->modem_at_parser_context.response_buffer_size/2)) {
      return -1;
   }


   //
   p_recv_packet = (modem_core_recv_packet_parameters_t*)_sys_malloc(sizeof(modem_core_recv_packet_parameters_t));
   if (!p_recv_packet) {
      return -1;
   }
   //
   p_recv_packet->len = data_received_length;
   p_recv_packet->r = 0;
   p_recv_packet->buf = (uint8_t*)_sys_malloc(data_received_length);
   if (!p_recv_packet->buf) {
      _sys_free(p_recv_packet);
      return -1;
   }

   //get data in hex asciii format
   at_response = pbuf;
   uint8_t* p_recv_packet_buf= p_recv_packet->buf;
   //
   while (*pbuf != '\"' && *pbuf != '\0') {
      uint8_t ha = *pbuf++;
      uint8_t msb= ha<58 ? ha - 48 : ha - 55;
      ha = *pbuf++;
      uint8_t lsb= ha<58 ? ha - 48 : ha - 55;

      //convert hex ascii in byte
      *p_recv_packet_buf++ = (uint8_t)((msb << 4) | lsb);
   }

   //lock 
   kernel_pthread_mutex_lock(&p_modem_core_connection_info->kernel_mutex);
   //
   p_recv_packet->next = (modem_core_recv_packet_parameters_t*)0;
   //
   if (!p_modem_core_connection_info->rcv_packet_head) {
      flag_signal = 1;
      p_modem_core_connection_info->rcv_packet_head = p_recv_packet;
      p_modem_core_connection_info->rcv_packet_last = p_modem_core_connection_info->rcv_packet_head;
   }
   else {
      p_modem_core_connection_info->rcv_packet_last->next = p_recv_packet;
      p_modem_core_connection_info->rcv_packet_last = p_recv_packet;
   }

   //unlock 
   kernel_pthread_mutex_unlock(&p_modem_core_connection_info->kernel_mutex);


   //signal 
   if (flag_signal) {
      desc_t desc_r = p_modem_core_connection_info->socket_desc;
      __fire_io(ofile_lst[desc_r].owner_pthread_ptr_read);
   }
   //
   return 0;
}

/*--------------------------------------------
| Name:        ublox_sarag3_at_response_callback_gethostbyname
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| if successful, return:
| AT+UDNSRN=0,"www.google.com" 
| +UDNSRN: "216.239.59.147"
| See:
----------------------------------------------*/
static int ublox_sarag3_at_response_callback_gethostbyname(void* pv_modem_core_info,char* at_response) {
   struct modem_core_info_st* p_modem_core_info = (struct modem_core_info_st*)pv_modem_core_info;
   char* pbuf = at_response;
   char* str_ip_address;


   //get +UDNSRN:
   while (*pbuf != ':' && *pbuf != '\0') {
      pbuf++;
   }
   //
   if (*pbuf == '\0') {
      return -1;
   }
   //
   pbuf++;

   //get ip address
   while (*pbuf != '\"'  && *pbuf != '\0') pbuf++;
   //
   if (*pbuf == '\0') {
      return -1;
   }
   //
   pbuf++;
   //
   at_response = pbuf;
   //
   while (*pbuf != '\"' && *pbuf != '\0') {
      pbuf++;
   }
   //
   *pbuf = '\0';
   //
   str_ip_address = at_response;

   
   /* fill hostent */
   static struct hostent hostent;
   static char *s_aliases;
   static unsigned long s_hostent_addr = (unsigned long)0L;
   static unsigned long *s_phostent_addr[2];

   //
   s_hostent_addr = inet_addr(str_ip_address);
   s_phostent_addr[0] = &s_hostent_addr;
   s_phostent_addr[1] = NULL;

   //
   s_aliases = NULL;
   hostent.h_aliases = &s_aliases;

   hostent.h_addrtype = AF_INET;
   hostent.h_length = sizeof(s_hostent_addr);
   hostent.h_addr_list = (char**)&s_phostent_addr;
   //
   modem_core_mq_post_unconnected_response(&hostent, MODEM_CORE_OPERATION_GETHOSTBYNAME_REQUEST, MODEM_CORE_OPERATION_DONE);
   //
   return 0;
}

/*--------------------------------------------
| Name:        ublox_sarag3_at_response_callback_localisation
| Description:
| Parameters:  none
| Return Type: none
| Comments:  
|     GSM localisation
|     AT+ULOC=2,2,2,240,1,1
|     attention: URC: +UULOC: 1,1,0,21/11/2017,17:04:16.000,46.0000000,2.0000000,0,1084000
|     If <response_type>=2, <sensor_used> = 2 and <num_hypothesis>=N:
|     +UULOC: <sol>,<num>,<sensor_ used>,<date>,<time>,<lat>,<long>, <alt>,<lat50>,<long50>,<major50 >,<minor50>,<orientation50>, <confidence50>[,<lat95>,<long95>, <major95>,<minor95>,<orientation95>, <confidence95>]
|
| See:
----------------------------------------------*/
static int ublox_sarag3_at_response_callback_localisation(void* pv_modem_core_info, char* at_response){
   char* pbuf = at_response;
   char* s_sol;
   char* s_num;
   char* s_sensor_used;
   char* s_date;
   char* s_time;
   char* s_lat;
   char* s_long;
  
   //get sol
   s_sol = pbuf;
   while (*pbuf != ',' && *pbuf != '\0') {
      pbuf++;
   }
   //
   if (*pbuf == '\0') {
      return -1;
   }
   *pbuf = '\0';
   //
   pbuf++;

   //get num
   s_num = pbuf;
   while (*pbuf != ',' && *pbuf != '\0') {
      pbuf++;
   }
   //
   if (*pbuf == '\0') {
      return -1;
   }
   *pbuf = '\0';
   //
   pbuf++;

   //get sensor_used
   s_sensor_used = pbuf;
   while (*pbuf != ',' && *pbuf != '\0') {
      pbuf++;
   }
   //
   if (*pbuf == '\0') {
      return -1;
   }
   *pbuf = '\0';
   //
   pbuf++;

   //get date
   s_date = pbuf;
   while (*pbuf != ',' && *pbuf != '\0') {
      pbuf++;
   }
   //
   if (*pbuf == '\0') {
      return -1;
   }
   *pbuf = '\0';
   //
   pbuf++;

   //get time
   s_time = pbuf;
   while (*pbuf != ',' && *pbuf != '\0') {
      pbuf++;
   }
   //
   if (*pbuf == '\0') {
      return -1;
   }
   *pbuf = '\0';
   //
   pbuf++;

   //get lat
   s_lat = pbuf;
   while (*pbuf != ',' && *pbuf != '\0') {
      pbuf++;
   }
   //
   if (*pbuf == '\0') {
      return -1;
   }
   *pbuf = '\0';
   //
   pbuf++;

   //get long
   s_long = pbuf;
   while (*pbuf != ',' && *pbuf != '\0') {
      pbuf++;
   }
   //
   if (*pbuf == '\0') {
      return -1;
   }
   *pbuf = '\0';
   //
   pbuf++;
   //
   if(strlen(s_lat)>0 && strlen(s_long)){
      g_modem_ublox_sarag3_info.loc_validity = LOC_VALID;
      g_modem_ublox_sarag3_info.loc_lat=atof(s_lat);
      g_modem_ublox_sarag3_info.loc_lng=atof(s_long);
   }
   //
   return 0;
}

/*--------------------------------------------
| Name: ublox_sarag3_at_command_modem_reset
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int ublox_sarag3_at_command_modem_reset(void* pv_modem_core_info, void * data) {
   struct modem_core_info_st* p_modem_core_info = (struct modem_core_info_st*)pv_modem_core_info;
   desc_t desc_r = p_modem_core_info->modem_ttys_desc_r;
   desc_t desc_w = p_modem_core_info->modem_ttys_desc_w;
   int cb;
   //
   unsigned long oflag = 0;
   int retry_counter = 10;
   //
   uint8_t* buf = g_modem_core_buffer;

   //
   desc_t desc_board = _vfs_open("/dev/board", O_RDWR, 0);
   //
   if (desc_board == INVALID_DESC)
      return -1;
   // to do : reimplement _sys_fcntl(fd,cmd,...)
   oflag = ofile_lst[desc_r].oflag;
   ofile_lst[desc_r].oflag = oflag | O_NONBLOCK;
   //  
   kernel_printk("modem reset...\r\n");
   //
   _vfs_ioctl(desc_board, BRDRESET, (void*)0);
   //
   kernel_printk("modem reset ok\r\n");
   //
   kernel_printk("modem setup communication...\r\n");
   //
   kernel_io_write(desc_w, at_cmd1, sizeof(at_cmd1) - 1);
   //
   while (retry_counter-->0) {
      __kernel_usleep(100000);
      //
      kernel_printk("modem send AT\r\n");
      //
      kernel_io_write(desc_w, at_cmd1, sizeof(at_cmd1) - 1);
      //
      __kernel_usleep(1000000);
      //
      cb = kernel_io_read(desc_r, buf, sizeof(buf));
      if (cb <= 0)
         continue;
      //
      kernel_printk("modem rcv:");
      //
      while (cb>0) {
         kernel_io_write(__get_kernel_tty_desc(), buf, cb);
         cb = kernel_io_read(desc_r, buf, sizeof(buf));
      }
      //
      break;
   }
   //
   if (retry_counter <= 0) {
      return -1;
   }
   //
   kernel_printk("\r\nmodem wait some seconds...\r\n");
   //
   __kernel_usleep(10000000);
   //
   kernel_printk("\r\nmodem ready\r\n");
   //
   ofile_lst[desc_r].oflag = oflag;
   //
   return 0;
   //
}

/*--------------------------------------------
| Name: ublox_sarag3_at_command_modem_init
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int ublox_sarag3_at_command_modem_init(void* pv_modem_core_info,void * data) {
   struct modem_core_info_st* p_modem_core_info = (struct modem_core_info_st*)pv_modem_core_info;
   desc_t desc_r = p_modem_core_info->modem_ttys_desc_r;
   desc_t desc_w = p_modem_core_info->modem_ttys_desc_w;
   //
   ublox_sarag3_get_ublox_socket_connexion_init();
   //
   for (int i = 0; i<(sizeof(modem_at_command_list) / sizeof(modem_at_command_t)); i++) {
      //
      kernel_io_ll_ioctl(desc_r, TCFLSH, TCIFLUSH);
      //
      kernel_printk("trace: %s\r\n", modem_at_command_list[i].command);
      //
      if (modem_core_parser_send_recv_at_command(p_modem_core_info, modem_at_command_list[i].command, modem_at_command_list[i].response, 0)<0)
         return -1;
      //
      __kernel_usleep(1000000);
      //
   }
   //
   return 0;
}

/*--------------------------------------------
| Name: ublox_sarag3_at_command_modem_stop
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int ublox_sarag3_at_command_modem_stop(void* pv_modem_core_info,void * data) {
   struct modem_core_info_st* p_modem_core_info = (struct modem_core_info_st*)pv_modem_core_info;
   desc_t desc_r = p_modem_core_info->modem_ttys_desc_r;
   desc_t desc_w = p_modem_core_info->modem_ttys_desc_w;
   //
   ublox_sarag3_get_ublox_socket_connexion_init();
   //
   for (int i = 0; i<(sizeof(stop_modem_at_command_list) / sizeof(stop_modem_at_command_list)); i++) {
      //
      kernel_io_ll_ioctl(desc_r, TCFLSH, TCIFLUSH);
      //
      kernel_printk("trace: %s\r\n", stop_modem_at_command_list[i].command);
      //
      if (modem_core_parser_send_recv_at_command(p_modem_core_info, stop_modem_at_command_list[i].command, stop_modem_at_command_list[i].response, 0)<0)
         return -1;
      //
      __kernel_usleep(1000000);
      //
   }
   //
   return 0;
}

/*--------------------------------------------
| Name:ublox_sarag3_at_command_socket_create
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int ublox_sarag3_at_command_socket_create(void* pv_modem_core_info, modem_core_connection_info_t* p_modem_core_connection_info) {
   struct modem_core_info_st* p_modem_core_info = (struct modem_core_info_st*)pv_modem_core_info;

   //
   if (p_modem_core_connection_info == (void*)0) {
      return -1;
   }
   //
   if (p_modem_core_connection_info->status != CONNECTION_STATUS_CLOSE) {
      return -1;
   }
   //
   memset(g_at_send_buffer, 0, sizeof(g_at_send_buffer));
   //TO DO send command with mqueue and signal thread p_modem_core_info->kernel_thread_connection2modem.sem_io 
   // wait result in socket mqueue

   //at command
   strcpy(g_at_send_buffer, "AT+USOCR=");
  //protocol
   if (p_modem_core_connection_info->type == SOCK_DGRAM && p_modem_core_connection_info->protocol == IPPROTO_UDP) {
      strcat(g_at_send_buffer, "17");
   }
   else if (p_modem_core_connection_info->type == SOCK_STREAM && p_modem_core_connection_info->protocol == IPPROTO_TCP) {
      strcat(g_at_send_buffer, "6");
   }
   else {
      return -1;
   }

   //
   if (modem_core_parser_send_at_command(p_modem_core_info, g_at_send_buffer, __KERNEL_MODEM_CORE_SILENT_MODE)<0)
      return -1;
   //
   return 0;
   //
}

/*--------------------------------------------
| Name:ublox_sarag3_at_command_socket_connect
| Description:
| Parameters:  none
| Return Type: none
| Comments:  AT+USOCO=<socket>,<remote_addr>,<remote_port>[,<async_connect>]
| Notes: asynchronous argument not supported in SARA-G3 model (see notes for this command in the documentation).
| See:
----------------------------------------------*/
char trap_lion_flag=0;
static int ublox_sarag3_at_command_socket_connect(void* pv_modem_core_info,modem_core_connection_info_t* p_modem_core_connection_info) {
   struct modem_core_info_st* p_modem_core_info = (struct modem_core_info_st*)pv_modem_core_info;
   char ltostr_buffer[4];
   
   trap_lion_flag= 11;
   //
   if (p_modem_core_connection_info == (void*)0) {
      return -1;
   }
   //
   if (p_modem_core_connection_info->status == CONNECTION_STATUS_CLOSE) {
      return -1;
   }
   //
   if (p_modem_core_connection_info->status == CONNECTION_STATUS_CONNECTED) {
      return -1;
   }
   
   trap_lion_flag= 12;
   
   //TO DO send command with mqueue and signal thread p_modem_core_info->kernel_thread_connection2modem.sem_io 
   // wait result in socket mqueue
   
   //
   memset(g_at_send_buffer, 0, sizeof(g_at_send_buffer));
   //at command
   strcpy(g_at_send_buffer, "AT+USOCR=");
   //protocol
   if (p_modem_core_connection_info->type == SOCK_DGRAM && p_modem_core_connection_info->protocol == IPPROTO_UDP) {
      strcat(g_at_send_buffer, "17");
   }
   else if (p_modem_core_connection_info->type == SOCK_STREAM && p_modem_core_connection_info->protocol == IPPROTO_TCP) {
      strcat(g_at_send_buffer, "6");
   }
   else {
      return -1;
   }

   //
   trap_lion_flag= 13;
   //
   if (modem_core_parser_send_recv_at_command(p_modem_core_info, g_at_send_buffer, "+USOCR:", __KERNEL_MODEM_CORE_SILENT_MODE)<0)
      return -1;

   //wait OK after +USOCR:
   if (modem_core_parser_recv_at_response(p_modem_core_info, "OK", 0, __KERNEL_MODEM_CORE_SILENT_MODE)<0)
      return -1;
   //
   trap_lion_flag= 20;
   
   //
   int ublox_socket_connexion_index = ublox_sarag3_get_ublox_socket_connexion_index(p_modem_core_connection_info);
   if (ublox_socket_connexion_index < 0)
      return -1;
   
   trap_lion_flag= 30;

   //only for TCP socket
   //
   memset(g_at_send_buffer, 0, sizeof(g_at_send_buffer));
   //at command
   strcpy(g_at_send_buffer, "AT+USOCO=");
   //connection number
   strcat(g_at_send_buffer, ltostr(ltostr_buffer, sizeof(ltostr_buffer), ublox_socket_connexion_index, 10, 0));
   strcat(g_at_send_buffer, ",\"");
   //ip address
   uint8_t* ip_addr = (uint8_t*)&p_modem_core_connection_info->sockaddr_in_connect.sin_addr.s_addr;
   //src[0], src[1], src[2], src[3]
   strcat(g_at_send_buffer, ltostr(ltostr_buffer, sizeof(ltostr_buffer), ip_addr[0], 10, 0));
   strcat(g_at_send_buffer, ".");
   strcat(g_at_send_buffer, ltostr(ltostr_buffer, sizeof(ltostr_buffer), ip_addr[1], 10, 0));
   strcat(g_at_send_buffer, ".");
   strcat(g_at_send_buffer, ltostr(ltostr_buffer, sizeof(ltostr_buffer), ip_addr[2], 10, 0));
   strcat(g_at_send_buffer, ".");
   strcat(g_at_send_buffer, ltostr(ltostr_buffer, sizeof(ltostr_buffer), ip_addr[3], 10, 0));
   strcat(g_at_send_buffer, "\",");
   // port number
   in_port_t port_no = ntohs(p_modem_core_connection_info->sockaddr_in_connect.sin_port);
   strcat(g_at_send_buffer, ltostr(ltostr_buffer, sizeof(ltostr_buffer), port_no, 10, 0));
   //asynchronous argument not supported in SARA-G3 model (see notes for this command in the documentation).
   //strcat(g_at_send_buffer, ",1");

   trap_lion_flag= 31;
   //OK or ERROR follow with UCR +UUSOCL response: 
   if (modem_core_parser_send_recv_at_command(p_modem_core_info, g_at_send_buffer, "OK", __KERNEL_MODEM_CORE_SILENT_MODE) < 0) {
      //wait +UUSOCL: after ERROR
      modem_core_parser_recv_at_response(p_modem_core_info, "+UUSOCL:", 0, __KERNEL_MODEM_CORE_SILENT_MODE);
      return -1;
   }
   //
   trap_lion_flag= 32;
   //
   p_modem_core_connection_info->status = CONNECTION_STATUS_CONNECTED;

   //
   modem_core_message_t modem_core_message;
   //
   modem_core_message.operation_request_code = MODEM_CORE_OPERATION_SOCKET_CONNECT_REQUEST;
   modem_core_message.operation_response_code = MODEM_CORE_OPERATION_DONE;
   modem_core_message.p_modem_core_connection_info = p_modem_core_connection_info;
   //
   kernel_mqueue_put(&p_modem_core_connection_info->kernel_mqueue, &modem_core_message, sizeof(modem_core_message));
   //
   trap_lion_flag= 0;
   //
   return 0;
}

/*--------------------------------------------
| Name:ublox_sarag3_at_command_socket_close
| Description:
| Parameters:  none
| Return Type: none
| Comments: AT+USOCL=<socket>
| See:
----------------------------------------------*/
static int ublox_sarag3_at_command_socket_close(void* pv_modem_core_info,modem_core_connection_info_t* p_modem_core_connection_info) {
   struct modem_core_info_st* p_modem_core_info = (struct modem_core_info_st*)pv_modem_core_info;
   //
   char ltostr_buffer[4];
   //
   if (p_modem_core_connection_info == (void*)0) {
      return -1;
   }
   //
   if (p_modem_core_connection_info->status == CONNECTION_STATUS_CLOSE) {
      return -1;
   }
   
   //
   int ublox_socket_connexion_index = ublox_sarag3_get_ublox_socket_connexion_index(p_modem_core_connection_info);
   if (ublox_socket_connexion_index < 0)
      return -1;
   
   //
   memset(g_at_send_buffer, 0, sizeof(g_at_send_buffer));
   //at command
   strcpy(g_at_send_buffer, "AT+USOCL=");
   //connection number
   strcat(g_at_send_buffer, ltostr(ltostr_buffer, sizeof(ltostr_buffer), ublox_socket_connexion_index, 10, 0));
   //
   //strcat(g_at_send_buffer, ",0");
   //
   if (modem_core_parser_send_recv_at_command(p_modem_core_info, g_at_send_buffer, "OK", __KERNEL_MODEM_CORE_SILENT_MODE)<0)
      return -1;
   //
   p_modem_core_connection_info->status = CONNECTION_STATUS_SHUTDOWN;
   //
   return 0;
}

/*--------------------------------------------
| Name:ublox_sarag3_at_command_socket_send
| Description:
| Parameters:  none
| Return Type: none
| Comments: AT+USOWR=<socket>,<length>,"<data>" HEX modem
|     AT+USOWR=3,12,"Hello world!" +USOWR: 3,12
|     OK
| Notes
|  SARA-G3 / LEON-G1
|  - The range of <length> parameter is
|     -. Base syntax normal mode: 0-512 
|     -. Base syntax HEX mode: 0-250
| Use only send with send lehgth up to UBLOX_SOCKET_DATA_LENGTH_MAX for each ublox_sarag3_at_command_socket_send() operation.
| not with value superior at UBLOX_SOCKET_DATA_LENGTH_MAX to preserve atomicity on send operation with at command. 
| To avoid URC message within AT+USOWR command. 
| See:
----------------------------------------------*/
static int ublox_sarag3_at_command_socket_send(void* pv_modem_core_info,modem_core_connection_info_t* p_modem_core_connection_info) {
   struct modem_core_info_st* p_modem_core_info = (struct modem_core_info_st*)pv_modem_core_info;
   char ltostr_buffer[4];
   int cb = 0;
   int len = 0;
   int snd_packet_len;
   int hexascii_snd_packet_len;
   uint8_t* snd_packet_buf = p_modem_core_connection_info->snd_packet.buf;
   uint8_t* p_ublox_hexascii_buffer_send = ublox_hexascii_buffer_send;

   //
   if (p_modem_core_connection_info->status != CONNECTION_STATUS_CONNECTED) {
      return -1;
   }
   //if length of packet is too large, trunc at UBLOX_SOCKET_DATA_LENGTH_MAX
   if (p_modem_core_connection_info->snd_packet.len > UBLOX_SOCKET_DATA_LENGTH_MAX) {
      p_modem_core_connection_info->snd_packet.len = UBLOX_SOCKET_DATA_LENGTH_MAX;
   }

   //
   int ublox_socket_connexion_index = ublox_sarag3_get_ublox_socket_connexion_index(p_modem_core_connection_info);
   if (ublox_socket_connexion_index < 0)
      return -1;
   
    //__kernel_usleep(100000);
#if 0
   //
   __kernel_usleep(500000);
   //at command
   strcpy(g_at_send_buffer, "AT+USOCTL=");
   //connection number
   strcat(g_at_send_buffer, ltostr(ltostr_buffer, sizeof(ltostr_buffer), ublox_socket_connexion_index, 10, 0));
   //
   strcat(g_at_send_buffer, ",11");
   
   if (modem_core_parser_send_recv_at_command(p_modem_core_info, g_at_send_buffer, "+USOCTL:", __KERNEL_MODEM_CORE_SILENT_MODE)<0){
      return -1;
   }
   //wait OK after +USOCTL:
   modem_core_parser_recv_at_response(p_modem_core_info, "OK", 0, __KERNEL_MODEM_CORE_SILENT_MODE);
   //
   __kernel_usleep(500000);
#endif  

   //prepare hex ascii data
   snd_packet_len = p_modem_core_connection_info->snd_packet.len;
   //
   for (int i = 0; i < snd_packet_len; i++) {
      *p_ublox_hexascii_buffer_send++ = hex_conversion_array[((snd_packet_buf[i] >> 4) & 0x0F)];
      *p_ublox_hexascii_buffer_send++ = hex_conversion_array[(snd_packet_buf[i] & 0x0F)];
   }
   //
   hexascii_snd_packet_len = 2 * snd_packet_len;

   //
   memset(g_at_send_buffer, 0, sizeof(g_at_send_buffer));
   //at command
   strcpy(g_at_send_buffer, "AT+USOWR=");
   //connection number
   strcat(g_at_send_buffer, ltostr(ltostr_buffer, sizeof(ltostr_buffer), ublox_socket_connexion_index, 10, 0));
   //
   strcat(g_at_send_buffer, ",");
   //
   snd_packet_len = p_modem_core_connection_info->snd_packet.len;
   //
   strcat(g_at_send_buffer, ltostr(ltostr_buffer, sizeof(ltostr_buffer), snd_packet_len, 10, 0));
   //
   strcat(g_at_send_buffer, ",\"");
   
   //send AT+USOWR=<socket>,<length>, . character '\r' not sent to no valid command (cannot use modem_core_parser_send_at_command()). 
   //Data must be sent before. 
   if ((cb = kernel_io_write(p_modem_core_info->modem_ttys_desc_w, g_at_send_buffer, strlen(g_at_send_buffer))) < 0) {
      return -1;
   }
   //send data
   len = 0;
   cb = 0;
   while (len<hexascii_snd_packet_len) {
      if ((cb = kernel_io_write(p_modem_core_info->modem_ttys_desc_w, ublox_hexascii_buffer_send + len, hexascii_snd_packet_len - len))<0)
         return -1;
      len += cb;
   }
   //
   if (modem_core_parser_send_at_command(p_modem_core_info, "\"", __KERNEL_MODEM_CORE_SILENT_MODE) < 0) {
      return -1;
   }
  
   //wait +USOWR:
   if (modem_core_parser_recv_at_response(p_modem_core_info, "+USOWR:",0, __KERNEL_MODEM_CORE_SILENT_MODE) < 0) {
      return -1;
   }
   //wait OK after +USOWR:
   modem_core_parser_recv_at_response(p_modem_core_info, "OK", 0, __KERNEL_MODEM_CORE_SILENT_MODE);

#if 0
   //
   __kernel_usleep(500000);
   //
   //at command
   strcpy(g_at_send_buffer, "AT+USOCTL=");
   //connection number
   strcat(g_at_send_buffer, ltostr(ltostr_buffer, sizeof(ltostr_buffer), ublox_socket_connexion_index, 10, 0));
   //
   strcat(g_at_send_buffer, ",11");
   
   if (modem_core_parser_send_recv_at_command(p_modem_core_info, g_at_send_buffer, "+USOCTL:", __KERNEL_MODEM_CORE_SILENT_MODE)<0){
      return -1;
   }
   //wait OK after +USOCTL:
   modem_core_parser_recv_at_response(p_modem_core_info, "OK", 0, __KERNEL_MODEM_CORE_SILENT_MODE);
#endif   

   //
   modem_core_message_t modem_core_message;
   //
   modem_core_message.operation_request_code = MODEM_CORE_OPERATION_SOCKET_SEND_REQUEST;
   modem_core_message.operation_response_code = MODEM_CORE_OPERATION_DONE;
   modem_core_message.p_modem_core_connection_info = p_modem_core_connection_info;
   //
   kernel_mqueue_put(&p_modem_core_connection_info->kernel_mqueue, &modem_core_message, sizeof(modem_core_message));

   //
   return 0;
}

/*--------------------------------------------
| Name:ublox_sarag3_at_command_gethostbyname
| Description:
| Parameters:  none
| Return Type: none
| Comments: AT+UDNSRN=<resolution_type>,<domain_ip_string>
|     AT+UDNSRN=0,"www.google.fru"
|     ERROR
|
|     AT+UDNSRN=0,"www.google.fr"
|     +UDNSRN: "172.217.21.227"
|
|     OK
|
| See:
----------------------------------------------*/
static int ublox_sarag3_at_command_gethostbyname(void* pv_modem_core_info, void* data) {
   struct modem_core_info_st* p_modem_core_info = (struct modem_core_info_st*)pv_modem_core_info;
   char* hostname = (char*)data;

   //
   if (!hostname) {
      return -1;
   }
   //
   if (!strlen(hostname)) {
      return -1;
   }
   //to add control on length of hostname too long.

   //
   memset(g_at_send_buffer, 0, sizeof(g_at_send_buffer));
   //at command
   strcpy(g_at_send_buffer, "AT+UDNSRN=0,\"");
   //add hostname
   strcat(g_at_send_buffer, hostname);
   //
   strcat(g_at_send_buffer, "\"");

   //
   if (modem_core_parser_send_recv_at_command_ex(p_modem_core_info, g_at_send_buffer, "+UDNSRN:", __KERNEL_MODEM_CORE_SILENT_MODE,MODEM_CORE_REQUEST_TIMEOUT_INFINITE) < 0) {
      return -1;
   }
   //wait OK after +UDNSRN:
   modem_core_parser_recv_at_response(p_modem_core_info, "OK", 0, __KERNEL_MODEM_CORE_SILENT_MODE);
   //
   return 0;
}

/*-------------------------------------------
| Name:dev_modem_ublox_sarag3_load
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_modem_ublox_sarag3_load(void) {
   //
   memset(&g_modem_ublox_sarag3_info,0,sizeof(g_modem_ublox_sarag3_info));
   g_modem_ublox_sarag3_info.loc_validity = LOC_INVALID;
   //
   return 0;
}

/*-------------------------------------------
| Name:dev_modem_ublox_sarag3_open
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_modem_ublox_sarag3_open(desc_t desc, int o_flag) {
   return 0;
}

/*-------------------------------------------
| Name:dev_modem_ublox_sarag3_close
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_modem_ublox_sarag3_close(desc_t desc) {
   return 0;
}

/*-------------------------------------------
| Name:dev_modem_ublox_sarag3_isset_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_modem_ublox_sarag3_isset_read(desc_t desc) {
   desc_t link_desc = ofile_lst[desc].desc_nxt[0];
   if (link_desc < 0) {
      return -1;
   }
   //
   return ofile_lst[link_desc].pfsop->fdev.fdev_isset_read(link_desc);
}

/*-------------------------------------------
| Name:dev_modem_ublox_sarag3_isset_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_modem_ublox_sarag3_isset_write(desc_t desc) {
   desc_t link_desc = ofile_lst[desc].desc_nxt[1];
   if (link_desc < 0) {
      return -1;
   }
   //
   return ofile_lst[link_desc].pfsop->fdev.fdev_isset_write(link_desc);
}

/*-------------------------------------------
| Name:dev_modem_ublox_sarag3_read
| Description:
| Parameters:
| Return Type:
| Comments:    lwip 2.0
| See:
---------------------------------------------*/
static int dev_modem_ublox_sarag3_read(desc_t desc, char* buffer, int len) {
   desc_t link_desc = ofile_lst[desc].desc_nxt[0];
   if (link_desc < 0) {
      return -1;
   }
   //
   return ofile_lst[link_desc].pfsop->fdev.fdev_read(link_desc,buffer,len);
}

/*-------------------------------------------
| Name:dev_modem_ublox_sarag3_write
| Description:
| Parameters:
| Return Type:
| Comments:    lwip 2.0
| See:
---------------------------------------------*/
static int dev_modem_ublox_sarag3_write(desc_t desc, const char *buffer, int size) {
   desc_t link_desc = ofile_lst[desc].desc_nxt[1];
   if (link_desc < 0) {
      return -1;
   }
   //
   return ofile_lst[link_desc].pfsop->fdev.fdev_write(link_desc, buffer, size);
}

/*-------------------------------------------
| Name:dev_modem_ublox_sarag3_read_args
| Description:
| Parameters:
| Return Type:
| Comments:  
| See:
---------------------------------------------*/
static int dev_modem_ublox_sarag3_read_args(desc_t desc, char* buffer, int len, va_list ap) {
   return -1;
}

/*-------------------------------------------
| Name:dev_modem_ublox_sarag3_write_args
| Description:
| Parameters:
| Return Type:
| Comments:   
| See:
---------------------------------------------*/
static int dev_modem_ublox_sarag3_write_args(desc_t desc, const char *buffer, int size, va_list ap) {
   return -1;
}

/*-------------------------------------------
| Name:dev_modem_ublox_sarag3_seek
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_modem_ublox_sarag3_seek(desc_t desc, int offset, int origin) {
   return -1;
}

/*--------------------------------------------
| Name:dev_modem_ublox_sarag3_ioctl
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static  int dev_modem_ublox_sarag3_ioctl(desc_t desc, int request, va_list ap) {
   switch (request) {

      case I_LINK: {
         //must be open in O_RDWR mode
         if ((ofile_lst[desc].oflag&O_RDWR) != O_RDWR)
            return -1;

         //link modem ip stack with serial peripheral.
         if (!(ofile_lst[desc].p)) {
            dev_modem_ublox_sarag3_info.desc_r = ofile_lst[desc].desc_nxt[0];
            dev_modem_ublox_sarag3_info.desc_w = ofile_lst[desc].desc_nxt[1];
            //
            ofile_lst[desc].p = &dev_modem_ublox_sarag3_info;
            //
            return 0;
         }

      }
      break;

      case I_UNLINK: {
         if (!(ofile_lst[desc].p)) {
            return -1;
         }
         //nothing to do
      }
      break;
      
      case IOCTL_UBLOX_SARAG3_GET_INFO: {
         modem_ublox_sarag3_info_t* p_modem_ublox_sarag3_info=va_arg(ap, modem_ublox_sarag3_info_t*);
         if(!p_modem_ublox_sarag3_info)
            return -1;
         //
         memcpy(p_modem_ublox_sarag3_info,&g_modem_ublox_sarag3_info,sizeof(g_modem_ublox_sarag3_info));
         //
         return 0;
      }
      break;

      default:
         return -1;
   }

   return -1;
}