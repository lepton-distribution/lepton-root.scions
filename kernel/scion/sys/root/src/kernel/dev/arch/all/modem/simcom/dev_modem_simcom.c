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
#include "kernel/core/ioctl_eth.h"
#include "kernel/core/ioctl_if.h"

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

/*============================================
| Global Declaration
==============================================*/

//
static modem_at_command_t const modem_at_command_list[] = {
   { .command = "ATV1",.response = (const char*)0 },
   { .command = "ATE0",.response = (const char*)0 },
   { .command = "AT+CMEE=1",.response = (const char*)0 },
   { .command = "AT+CGREG?",.response = (const char*)0 },
   { .command = "AT+COPS?",.response = (const char*)0 },
   { .command = "AT+CIPMUX=1",.response = (const char*)0 },
   { .command = "AT+CGATT=1",.response = (const char*)0 },
   { .command = "AT+CGREG=1",.response = (const char*)0 },
   { .command = "AT+CSTT=\"soracom.io\",\"sora\",\"sora\"",.response = (const char*)0 },
   { .command = "AT+CIICR",.response = (const char*)0 },
   { .command = "AT+CIFSR",.response = "*" }
};

//
static const char at_cmd1[] = "AT\r\n";

//
static int dev_modem_simcom_load(void);
static int dev_modem_simcom_open(desc_t desc, int o_flag);
static int dev_modem_simcom_close(desc_t desc);
static int dev_modem_simcom_isset_read(desc_t desc);
static int dev_modem_simcom_isset_write(desc_t desc);
static int dev_modem_simcom_read(desc_t desc, char* buffer, int nbyte);
static int dev_modem_simcom_write(desc_t desc, const char* buffer, int nbyte);
static int dev_modem_simcom_read_args(desc_t desc, char* buffer, int len, va_list ap);
static int dev_modem_simcom_write_args(desc_t desc, const char* buffer, int len, va_list ap);
static int dev_modem_simcom_seek(desc_t desc, int offset, int origin);
static int dev_modem_simcom_ioctl(desc_t desc, int request, va_list ap);

static const char dev_modem_simcom_name[] = "sim800";

dev_map_t  dev_modem_simcom_map = {
   .dev_name = dev_modem_simcom_name,
   .dev_attr = S_IFCHR,
   .fdev_load = dev_modem_simcom_load,
   .fdev_open = dev_modem_simcom_open,
   .fdev_close = dev_modem_simcom_close,
   .fdev_isset_read = dev_modem_simcom_isset_read,
   .fdev_isset_write = dev_modem_simcom_isset_write,
   .fdev_read = dev_modem_simcom_read,
   .fdev_write = dev_modem_simcom_write,
   .fdev_read_args = __fdev_not_implemented,
   .fdev_write_args = __fdev_not_implemented,
   .fdev_seek = __fdev_not_implemented,
   .fdev_ioctl = dev_modem_simcom_ioctl
};

/*
AT+CIPSTART
[<n>,]CONNECT OK\r\n          TCP/ UDP connection is successful
[<n>,]CONNECT FAIL\r\         TCP/UDP connection fails
[<n>,]ALREADY CONNECT\r\      TCP/UDP connection exists

[<n>,]SEND OK\r\n         Data sending is successful

[<n>,]CLOSED\r\n          TCP/UDP connection is closed

+RECEIVE,<n>,<length>\r\n
[binary data]
*/
static int simcom_at_response_callback_connect_ok(void* pv_modem_core_info, char* at_response);
static int simcom_at_response_callback_connect_fail(void* pv_modem_core_info, char* at_response);
static int simcom_at_response_callback_already_connect(void* pv_modem_core_info, char* at_response);
static int simcom_at_response_callback_send_ok(void* pv_modem_core_info, char* at_response);
static int simcom_at_response_callback_send_fail(void* pv_modem_core_info, char* at_response);
static int simcom_at_response_callback_close_ok(void* pv_modem_core_info, char* at_response);
static int simcom_at_response_callback_receive(void* pv_modem_core_info, char* at_response);
static int simcom_at_response_callback_gethostbyname(void* pv_modem_core_info, char* at_response);

//
static const modem_at_parser_response_callback_t modem_simcom_at_parser_response_callback_list[] = {
   { .at_response = ", CONNECT OK",.at_response_callback = simcom_at_response_callback_connect_ok },
   { .at_response = ", CONNECT FAIL",.at_response_callback = simcom_at_response_callback_connect_fail },
   { .at_response = ", ALREADY CONNECT",.at_response_callback = simcom_at_response_callback_already_connect },
   { .at_response = ", SEND OK",.at_response_callback = simcom_at_response_callback_send_ok },
   { .at_response = ", SEND FAIL",.at_response_callback = simcom_at_response_callback_send_fail },
   { .at_response = ", CLOSED",.at_response_callback = simcom_at_response_callback_close_ok },
   { .at_response = "+RECEIVE",.at_response_callback = simcom_at_response_callback_receive },
   { .at_response = "+CDNSGIP:",.at_response_callback = simcom_at_response_callback_gethostbyname },
};

//
static int simcom_at_command_modem_reset(void* pv_modem_core_info, void * data);
static int simcom_at_command_modem_init(void* pv_modem_core_info, void * data);
static int simcom_at_command_socket_connect(void* pv_modem_core_info, modem_core_connection_info_t* p_modem_core_connection_info);
static int simcom_at_command_socket_close(void* pv_modem_core_info, modem_core_connection_info_t* p_modem_core_connection_info);
static int simcom_at_command_socket_send(void* pv_modem_core_info, modem_core_connection_info_t* p_modem_core_connection_info);
static int simcom_at_command_gethostbyname(void* pv_modem_core_info, void* data);

//
static dev_modem_info_t dev_modem_simcom_info = {
   .desc_r = INVALID_DESC,
   .desc_w = INVALID_DESC,
   .at_response_callback_list_size= sizeof(modem_simcom_at_parser_response_callback_list)/sizeof(modem_at_parser_response_callback_t),
   .at_response_callback_list =( modem_at_parser_response_callback_t*) modem_simcom_at_parser_response_callback_list,
   .modem_at_command_op.at_command_modem_reset     = simcom_at_command_modem_reset,
   .modem_at_command_op.at_command_modem_init      = simcom_at_command_modem_init,
   .modem_at_command_op.at_command_modem_stop      = __pfn_at_command_not_implemented,
   .modem_at_command_op.at_command_socket_create   = __pfn_at_command_not_implemented,
   .modem_at_command_op.at_command_socket_connect  = simcom_at_command_socket_connect,
   .modem_at_command_op.at_command_socket_close    = simcom_at_command_socket_close,
   .modem_at_command_op.at_command_socket_send     = simcom_at_command_socket_send,
   .modem_at_command_op.at_command_socket_receive  = __pfn_at_command_not_implemented,
   .modem_at_command_op.at_command_gethostbyname   = simcom_at_command_gethostbyname
};

/*============================================
| Implementation
==============================================*/


/*--------------------------------------------
| Name:        simcom_at_response_callback_connect_ok
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int simcom_at_response_callback_connect_ok(void* pv_modem_core_info,char* at_response) {
   struct modem_core_info_st* p_modem_core_info = (struct modem_core_info_st*)pv_modem_core_info;
   char* pbuf = at_response;
   int modem_core_connexion_index;
   modem_core_message_t modem_core_message;

   //get connection number
   while (*pbuf != ',' && *pbuf != '\0') {
      pbuf++;
   }
   *pbuf = '\0';
   //
   pbuf = at_response;
   //
   modem_core_connexion_index = atoi(pbuf);
   //
   g_modem_core_connection_info_list[modem_core_connexion_index].status = CONNECTION_STATUS_CONNECTED;
   //
   modem_core_message.operation_request_code = MODEM_CORE_OPERATION_SOCKET_CONNECT_REQUEST;
   modem_core_message.operation_response_code = MODEM_CORE_OPERATION_DONE;
   modem_core_message.p_modem_core_connection_info = &g_modem_core_connection_info_list[modem_core_connexion_index];
   //
   kernel_mqueue_put(&g_modem_core_connection_info_list[modem_core_connexion_index].kernel_mqueue, &modem_core_message, sizeof(modem_core_message));
   //
   return 0;
}

/*--------------------------------------------
| Name:        simcom_at_response_callback_connect_fail
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int simcom_at_response_callback_connect_fail(void* pv_modem_core_info,char* at_response) {
   struct modem_core_info_st* p_modem_core_info = (struct modem_core_info_st*)pv_modem_core_info;
   char* pbuf = at_response;
   int modem_core_connexion_index;
   modem_core_message_t modem_core_message;

   //get connection number
   while (*pbuf != ',' && *pbuf != '\0') {
      pbuf++;
   }
   *pbuf = '\0';
   //
   pbuf = at_response;
   //
   modem_core_connexion_index = atoi(pbuf);
   //
   modem_core_message.operation_request_code = MODEM_CORE_OPERATION_SOCKET_CONNECT_REQUEST;
   modem_core_message.operation_response_code = MODEM_CORE_OPERATION_FAILED;
   modem_core_message.p_modem_core_connection_info = &g_modem_core_connection_info_list[modem_core_connexion_index];
   //
   kernel_mqueue_put(&g_modem_core_connection_info_list[modem_core_connexion_index].kernel_mqueue, &modem_core_message, sizeof(modem_core_message));

   return 0;
}

/*--------------------------------------------
| Name:        simcom_at_response_callback_already_connect
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int simcom_at_response_callback_already_connect(void* pv_modem_core_info,char* at_response) {
   struct modem_core_info_st* p_modem_core_info = (struct modem_core_info_st*)pv_modem_core_info;
   char* pbuf = at_response;
   int modem_core_connexion_index;
   modem_core_message_t modem_core_message;

   //get connection number
   while (*pbuf != ',' && *pbuf != '\0') {
      pbuf++;
   }
   *pbuf = '\0';
   //
   pbuf = at_response;
   //
   modem_core_connexion_index = atoi(pbuf);
   //
   modem_core_message.operation_request_code = MODEM_CORE_OPERATION_SOCKET_CONNECT_REQUEST;
   modem_core_message.operation_response_code = MODEM_CORE_OPERATION_FAILED;
   modem_core_message.p_modem_core_connection_info = &g_modem_core_connection_info_list[modem_core_connexion_index];
   //
   kernel_mqueue_put(&g_modem_core_connection_info_list[modem_core_connexion_index].kernel_mqueue, &modem_core_message, sizeof(modem_core_message));

   return 0;
}

/*--------------------------------------------
| Name:        simcom_at_response_callback_send_ok
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int simcom_at_response_callback_send_ok(void* pv_modem_core_info,char* at_response) {
   struct modem_core_info_st* p_modem_core_info = (struct modem_core_info_st*)pv_modem_core_info;
   char* pbuf = at_response;
   int modem_core_connexion_index;
   modem_core_message_t modem_core_message;

   //get connection number
   while (*pbuf != ',' && *pbuf != '\0') {
      pbuf++;
   }
   *pbuf = '\0';
   //
   pbuf = at_response;
   //
   modem_core_connexion_index = atoi(pbuf);
   //
   modem_core_message.operation_request_code = MODEM_CORE_OPERATION_SOCKET_SEND_REQUEST;
   modem_core_message.operation_response_code = MODEM_CORE_OPERATION_DONE;
   modem_core_message.p_modem_core_connection_info = &g_modem_core_connection_info_list[modem_core_connexion_index];
   //
   kernel_mqueue_put(&g_modem_core_connection_info_list[modem_core_connexion_index].kernel_mqueue, &modem_core_message, sizeof(modem_core_message));

   return 0;
}

/*--------------------------------------------
| Name:        simcom_at_response_callback_send_fail
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int simcom_at_response_callback_send_fail(void* pv_modem_core_info,char* at_response) {
   struct modem_core_info_st* p_modem_core_info = (struct modem_core_info_st*)pv_modem_core_info;
   char* pbuf = at_response;
   int modem_core_connexion_index;
   modem_core_message_t modem_core_message;

   //get connection number
   while (*pbuf != ',' && *pbuf != '\0') {
      pbuf++;
   }
   *pbuf = '\0';
   //
   pbuf = at_response;
   //
   modem_core_connexion_index = atoi(pbuf);
   //
   modem_core_message.operation_request_code = MODEM_CORE_OPERATION_SOCKET_SEND_REQUEST;
   modem_core_message.operation_response_code = MODEM_CORE_OPERATION_FAILED;
   modem_core_message.p_modem_core_connection_info = &g_modem_core_connection_info_list[modem_core_connexion_index];
   //
   kernel_mqueue_put(&g_modem_core_connection_info_list[modem_core_connexion_index].kernel_mqueue, &modem_core_message, sizeof(modem_core_message));
   //
   return 0;
}

/*--------------------------------------------
| Name:        simcom_at_response_callback_close_ok
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int simcom_at_response_callback_close_ok(void* pv_modem_core_info,char* at_response) {
   struct modem_core_info_st* p_modem_core_info = (struct modem_core_info_st*)pv_modem_core_info;
   char* pbuf = at_response;
   int modem_core_connexion_index;
   desc_t desc_r;

   //get connection number
   while (*pbuf != ',' && *pbuf != '\0') {
      pbuf++;
   }
   *pbuf = '\0';
   //
   pbuf = at_response;
   //
   modem_core_connexion_index = atoi(pbuf);
   // set status to shutdown.
   g_modem_core_connection_info_list[modem_core_connexion_index].status = CONNECTION_STATUS_SHUTDOWN;
   //

   //to do: fire io to user level thread if it's blocked on read operation (__wait_io). 
   //See if this modification must do in kerner_io_read and in vfs_close if pthread_ower_read and pthread_ower_write 
   //are different of pthread that make syscall 
   desc_r = g_modem_core_connection_info_list[modem_core_connexion_index].socket_desc;
   __fire_io(ofile_lst[desc_r].owner_pthread_ptr_read);

   //
   return 0;
}

/*--------------------------------------------
| Name:        at_response_callback_receive
| Description:
| Parameters:  none
| Return Type: none
| Comments: +RECEIVE,0,11:
| See:
----------------------------------------------*/
static int simcom_at_response_callback_receive(void* pv_modem_core_info,char* at_response) {
   struct modem_core_info_st* p_modem_core_info = (struct modem_core_info_st*)pv_modem_core_info;
   char* pbuf = at_response;
   int modem_core_connexion_index;
   int recv_packet_len = 0;
   int len = 0;
   int cb;
   int oflag;
   desc_t desc_r;
   uint8_t flag_signal = 0;
   modem_core_recv_packet_parameters_t* p_recv_packet;

   //get +Receive
   while (*pbuf != ',' && *pbuf != '\0') {
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
   if (*pbuf == '\0') {
      return -1;
   }
   *pbuf = '\0';
   //
   modem_core_connexion_index = atoi(at_response);
   //
   pbuf++;
   //get receive data len
   at_response = pbuf;
   //
   while (*pbuf != ':' && *pbuf != '\0') {
      pbuf++;
   }
   //
   if (*pbuf == '\0') {
      return -1;
   }
   //
   *pbuf = '\0';
   //
   recv_packet_len = atoi(at_response);
   //
   if (recv_packet_len <= 0) {
      return 0;
   }

   //
   p_recv_packet = (modem_core_recv_packet_parameters_t*)_sys_malloc(sizeof(modem_core_recv_packet_parameters_t));
   if (!p_recv_packet) {
      return -1;
   }
   //
   p_recv_packet->len = recv_packet_len;
   p_recv_packet->r = 0;
   p_recv_packet->buf = (uint8_t*)_sys_malloc(recv_packet_len);
   if (!p_recv_packet->buf) {
      _sys_free(p_recv_packet);
      return -1;
   }

   //
   len = 0;
   //save and set oflag
   oflag = ofile_lst[p_modem_core_info->modem_ttys_desc_r].oflag;
   ofile_lst[p_modem_core_info->modem_ttys_desc_r].oflag &= ~(O_NONBLOCK);
   //
   while (len<recv_packet_len) {
      if ((cb = kernel_io_read(p_modem_core_info->modem_ttys_desc_r, p_recv_packet->buf + len, recv_packet_len - len))<0) {
         ofile_lst[p_modem_core_info->modem_ttys_desc_r].oflag = oflag;
         return -1;
      }
      len += cb;
   }
   //restore oflag 
   ofile_lst[p_modem_core_info->modem_ttys_desc_r].oflag = oflag;

   //lock 
   kernel_pthread_mutex_lock(&g_modem_core_connection_info_list[modem_core_connexion_index].kernel_mutex);
   //
   p_recv_packet->next = (modem_core_recv_packet_parameters_t*)0;
   //
   if (!g_modem_core_connection_info_list[modem_core_connexion_index].rcv_packet_head) {
      flag_signal = 1;
      g_modem_core_connection_info_list[modem_core_connexion_index].rcv_packet_head = p_recv_packet;
      g_modem_core_connection_info_list[modem_core_connexion_index].rcv_packet_last = g_modem_core_connection_info_list[modem_core_connexion_index].rcv_packet_head;
   }
   else {
      g_modem_core_connection_info_list[modem_core_connexion_index].rcv_packet_last->next = p_recv_packet;
      g_modem_core_connection_info_list[modem_core_connexion_index].rcv_packet_last = p_recv_packet;
   }

   //unlock 
   kernel_pthread_mutex_unlock(&g_modem_core_connection_info_list[modem_core_connexion_index].kernel_mutex);


   //signal 
   if (flag_signal) {
      desc_r = g_modem_core_connection_info_list[modem_core_connexion_index].socket_desc;
      __fire_io(ofile_lst[desc_r].owner_pthread_ptr_read);
   }
   //

   return 0;
}

/*--------------------------------------------
| Name:        simcom_at_response_callback_gethostbyname
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| if successful, return:
| +CDNSGIP: 1, <domain name>,<IP1>[,<IP2>]
| If fail, return:
| +CDNSGIP:0,<dns error code>
|
| +CDNSGIP: 1,"ks354041.kimsufi.com","91.121.101.217"
| See:
----------------------------------------------*/
static int simcom_at_response_callback_gethostbyname(void* pv_modem_core_info,char* at_response) {
   struct modem_core_info_st* p_modem_core_info = (struct modem_core_info_st*)pv_modem_core_info;
   char* pbuf = at_response;
   int dns_operation_status = 0;
   char* str_official_hostname;
   char* str_ip_address;


   //get +CDNSGIP:
   while (*pbuf != ':' && *pbuf != '\0') {
      pbuf++;
   }
   //
   if (*pbuf == '\0') {
      return -1;
   }


   //
   pbuf++;
   //get dns operation status
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
   dns_operation_status = atoi(at_response);


   //
   pbuf++;
   //get official hostname
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
   if (*pbuf == '\0') {
      return -1;
   }
   //
   *(pbuf) = '\0';
   //
   str_official_hostname = at_response;

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

   //
   if (!dns_operation_status) {
      modem_core_mq_post_unconnected_response((void*)0, MODEM_CORE_OPERATION_GETHOSTBYNAME_REQUEST, MODEM_CORE_OPERATION_FAILED);
   }
   else {
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
   }
   return 0;
}

/*--------------------------------------------
| Name: simcom_at_command_modem_reset
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int simcom_at_command_modem_reset(void* pv_modem_core_info, void * data) {
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
| Name: simcom_at_command_modem_init
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int simcom_at_command_modem_init(void* pv_modem_core_info,void * data) {
   struct modem_core_info_st* p_modem_core_info = (struct modem_core_info_st*)pv_modem_core_info;
   desc_t desc_r = p_modem_core_info->modem_ttys_desc_r;
   desc_t desc_w = p_modem_core_info->modem_ttys_desc_w;
   //
   for (int i = 0; i<(sizeof(modem_at_command_list) / sizeof(modem_at_command_t)); i++) {
      //
      kernel_io_ll_ioctl(desc_r, TCFLSH, TCIFLUSH);
      //
      kernel_printk("trace: %s\r\n", modem_at_command_list[i].command);
      //
      if (modem_core_parser_send_recv_at_command(p_modem_core_info, modem_at_command_list[i].command, modem_at_command_list[i].response, __KERNEL_MODEM_CORE_SILENT_MODE)<0)
         return -1;
      //
      __kernel_usleep(1000000);
      //
   }
   //
   return 0;
}

/*--------------------------------------------
| Name:simcom_at_command_socket_connect
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int simcom_at_command_socket_connect(void* pv_modem_core_info,modem_core_connection_info_t* p_modem_core_connection_info) {
   struct modem_core_info_st* p_modem_core_info = (struct modem_core_info_st*)pv_modem_core_info;
   char ltostr_buffer[4];
   //
   if (p_modem_core_connection_info == (void*)0) {
      return -1;
   }
   //
   if (p_modem_core_connection_info->status != CONNECTION_STATUS_OPEN) {
      return -1;
   }
   //
   memset(g_at_send_buffer, 0, sizeof(g_at_send_buffer));
   //TO DO send command with mqueue and signal thread p_modem_core_info->kernel_thread_connection2modem.sem_io 
   // wait result in socket mqueue

   //at command
   strcpy(g_at_send_buffer, "AT+CIPSTART=");
   //connection number
   strcat(g_at_send_buffer, ltostr(ltostr_buffer, sizeof(ltostr_buffer), p_modem_core_connection_info->modem_core_connexion_index, 10, 0));
   //protocol
   if (p_modem_core_connection_info->type == SOCK_DGRAM && p_modem_core_connection_info->protocol == IPPROTO_UDP) {
      strcat(g_at_send_buffer, ",\"UDP\",");
   }
   else if (p_modem_core_connection_info->type == SOCK_STREAM && p_modem_core_connection_info->protocol == IPPROTO_TCP) {
      strcat(g_at_send_buffer, ",\"TCP\",");
   }
   else {
      return -1;
   }
   // modem at "AT+CIPSTART=\"TCP\",\"hrc.o10ee.com\",80"
   //ip address
   uint8_t* ip_addr = (uint8_t*)&p_modem_core_connection_info->sockaddr_in_connect.sin_addr.s_addr;
   //src[0], src[1], src[2], src[3]
   strcat(g_at_send_buffer, "\"");
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
   strcat(g_at_send_buffer, "\"");
   strcat(g_at_send_buffer, ltostr(ltostr_buffer, sizeof(ltostr_buffer), port_no, 10, 0));
   strcat(g_at_send_buffer, "\"");

   //
   if (modem_core_parser_send_at_command(p_modem_core_info, g_at_send_buffer, __KERNEL_MODEM_CORE_SILENT_MODE)<0)
      return -1;
   //
   return 0;
}

/*--------------------------------------------
| Name:simcom_at_command_socket_close
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int simcom_at_command_socket_close(void* pv_modem_core_info,modem_core_connection_info_t* p_modem_core_connection_info) {
   struct modem_core_info_st* p_modem_core_info = (struct modem_core_info_st*)pv_modem_core_info;
   //
   char ltostr_buffer[4];
   //
   if (p_modem_core_connection_info == (void*)0) {
      return -1;
   }
   //
   if (p_modem_core_connection_info->status != CONNECTION_STATUS_CONNECTED) {
      return -1;
   }
   //
   memset(g_at_send_buffer, 0, sizeof(g_at_send_buffer));
   //at command
   strcpy(g_at_send_buffer, "AT+CIPCLOSE=");
   //connection number
   strcat(g_at_send_buffer, ltostr(ltostr_buffer, sizeof(ltostr_buffer), p_modem_core_connection_info->modem_core_connexion_index, 10, 0));
   //
   strcat(g_at_send_buffer, ",0");
   //
   if (modem_core_parser_send_recv_at_command(p_modem_core_info, g_at_send_buffer, "CLOSE OK", __KERNEL_MODEM_CORE_SILENT_MODE)<0)
      return -1;
   //
   p_modem_core_connection_info->status = CONNECTION_STATUS_SHUTDOWN;
   //
   return 0;
}

/*--------------------------------------------
| Name:simcom_at_command_socket_send
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int simcom_at_command_socket_send(void* pv_modem_core_info,modem_core_connection_info_t* p_modem_core_connection_info) {
   struct modem_core_info_st* p_modem_core_info = (struct modem_core_info_st*)pv_modem_core_info;
   char ltostr_buffer[4];
   int cb = 0;
   int len = 0;
   int snd_packet_len;
   uint8_t* snd_packet_buf = p_modem_core_connection_info->snd_packet.buf;

   //
   if (p_modem_core_connection_info->status != CONNECTION_STATUS_CONNECTED) {
      return -1;
   }
   //
   memset(g_at_send_buffer, 0, sizeof(g_at_send_buffer));

   //at command
   strcpy(g_at_send_buffer, "AT+CIPSEND=");
   //connection number
   strcat(g_at_send_buffer, ltostr(ltostr_buffer, sizeof(ltostr_buffer), p_modem_core_connection_info->modem_core_connexion_index, 10, 0));
   //
   strcat(g_at_send_buffer, ",");
   //
   snd_packet_len = p_modem_core_connection_info->snd_packet.len;
   //
   strcat(g_at_send_buffer, ltostr(ltostr_buffer, sizeof(ltostr_buffer), snd_packet_len, 10, 0));
   //   
   if (modem_core_parser_send_recv_at_command(p_modem_core_info, g_at_send_buffer, ">", __KERNEL_MODEM_CORE_SILENT_MODE)<0)
      return -1;
   //
   while (len<snd_packet_len) {
      if ((cb = kernel_io_write(p_modem_core_info->modem_ttys_desc_w, snd_packet_buf + len, snd_packet_len - len))<0)
         return -1;
      len += cb;
   }
   //
   return 0;
}

/*--------------------------------------------
| Name:simcom_at_command_gethostbyname
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int simcom_at_command_gethostbyname(void* pv_modem_core_info, void* data) {
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
   strcpy(g_at_send_buffer, " AT+CDNSGIP=");
   //add hostname
   strcat(g_at_send_buffer, hostname);

   //
   if (modem_core_parser_send_recv_at_command(p_modem_core_info, g_at_send_buffer, "OK", __KERNEL_MODEM_CORE_SILENT_MODE)<0)
      return -1;
   //
   return 0;
}

/*-------------------------------------------
| Name:dev_modem_simcom_load
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_modem_simcom_load(void) {
   //
   return 0;
}

/*-------------------------------------------
| Name:dev_modem_simcom_open
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_modem_simcom_open(desc_t desc, int o_flag) {
   return 0;
}

/*-------------------------------------------
| Name:dev_modem_simcom_close
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_modem_simcom_close(desc_t desc) {
   return 0;
}

/*-------------------------------------------
| Name:dev_modem_simcom_isset_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_modem_simcom_isset_read(desc_t desc) {
   desc_t link_desc = ofile_lst[desc].desc_nxt[0];
   if (link_desc < 0) {
      return -1;
   }
   //
   return ofile_lst[link_desc].pfsop->fdev.fdev_isset_read(link_desc);
}

/*-------------------------------------------
| Name:dev_modem_simcom_isset_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_modem_simcom_isset_write(desc_t desc) {
   desc_t link_desc = ofile_lst[desc].desc_nxt[1];
   if (link_desc < 0) {
      return -1;
   }
   //
   return ofile_lst[link_desc].pfsop->fdev.fdev_isset_write(link_desc);
}

/*-------------------------------------------
| Name:dev_modem_simcom_read
| Description:
| Parameters:
| Return Type:
| Comments:    lwip 2.0
| See:
---------------------------------------------*/
static int dev_modem_simcom_read(desc_t desc, char* buffer, int len) {
   desc_t link_desc = ofile_lst[desc].desc_nxt[0];
   if (link_desc < 0) {
      return -1;
   }
   //
   return ofile_lst[link_desc].pfsop->fdev.fdev_read(link_desc,buffer,len);
}

/*-------------------------------------------
| Name:dev_modem_simcom_write
| Description:
| Parameters:
| Return Type:
| Comments:    lwip 2.0
| See:
---------------------------------------------*/
static int dev_modem_simcom_write(desc_t desc, const char *buffer, int size) {
   desc_t link_desc = ofile_lst[desc].desc_nxt[1];
   if (link_desc < 0) {
      return -1;
   }
   //
   return ofile_lst[link_desc].pfsop->fdev.fdev_write(link_desc, buffer, size);
}

/*-------------------------------------------
| Name:dev_modem_simcom_read_args
| Description:
| Parameters:
| Return Type:
| Comments:  
| See:
---------------------------------------------*/
static int dev_modem_simcom_read_args(desc_t desc, char* buffer, int len, va_list ap) {
   return -1;
}

/*-------------------------------------------
| Name:dev_modem_simcom_write_args
| Description:
| Parameters:
| Return Type:
| Comments:   
| See:
---------------------------------------------*/
static int dev_modem_simcom_write_args(desc_t desc, const char *buffer, int size, va_list ap) {
   return -1;
}

/*-------------------------------------------
| Name:dev_modem_simcom_seek
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_modem_simcom_seek(desc_t desc, int offset, int origin) {
   return -1;
}

/*--------------------------------------------
| Name:dev_modem_simcom_ioctl
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static  int dev_modem_simcom_ioctl(desc_t desc, int request, va_list ap) {
   switch (request) {

      case I_LINK: {
         //must be open in O_RDWR mode
         if ((ofile_lst[desc].oflag&O_RDWR) != O_RDWR)
            return -1;

         //link modem ip stack with serial peripheral.
         if (!(ofile_lst[desc].p)) {
            dev_modem_simcom_info.desc_r = ofile_lst[desc].desc_nxt[0];
            dev_modem_simcom_info.desc_w = ofile_lst[desc].desc_nxt[1];
            //
            ofile_lst[desc].p = &dev_modem_simcom_info;
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

      default:
         return -1;
   }

   return -1;
}