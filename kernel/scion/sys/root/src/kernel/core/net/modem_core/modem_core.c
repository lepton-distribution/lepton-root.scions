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
#include "kernel/core/net/kernel_net_core_socket.h"

#include "kernel/core/net/bsd/netdb.h"

#include "lib/libc/termios/termios.h"
#include "lib/libc/misc/ltostr.h"

/*============================================
| Global Declaration
==============================================*/
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

//
#define PARSER_STATE_WAIT_CR   1  // '\r'
#define PARSER_STATE_WAIT_LF   2  // '\n'
#define PARSER_STATE_PARSE     3  //

//for asynchronous message from modem
typedef int (*pfn_parser_at_response_callback_t)(char* at_response);
typedef struct modem_at_parser_response_callback_st{
   const char* at_response;
   pfn_parser_at_response_callback_t at_response_callback;
}modem_at_parser_response_callback_t;

//
typedef struct modem_at_parser_context_st{ 
   uint8_t state;
   char* response_buffer;
   char* pbuf;
   
   // asychronous message from odeme
   int at_response_callback_list_size;
   modem_at_parser_response_callback_t * at_response_callback_list;
   
   desc_t desc_r; 
   desc_t desc_w;
}modem_at_parser_context_t;

//
static const char at_cmd1[]="AT\r\n";
//
typedef struct modem_at_command_st{
   const char* command;
   const char* response;   
}modem_at_command_t;

static modem_at_command_t const modem_at_command_list[]={
   { .command = "ATV1", .response=(const char*)0},
   { .command = "ATE0", .response=(const char*)0},
   { .command = "AT+CMEE=1", .response=(const char*)0},
   { .command = "AT+CGREG?", .response=(const char*)0},
   { .command = "AT+COPS?", .response=(const char*)0},
   { .command = "AT+CIPMUX=1", .response=(const char*)0},
   { .command = "AT+CGATT=1", .response=(const char*)0},
   { .command = "AT+CGREG=1", .response=(const char*)0},
   { .command = "AT+CSTT=\"soracom.io\",\"sora\",\"sora\"", .response=(const char*)0},
   { .command = "AT+CIICR", .response=(const char*)0},
   { .command = "AT+CIFSR", .response="*"}
};

//
static int dev_modem_core_load(void);
static int dev_modem_core_open(desc_t desc,int o_flag);
static int dev_modem_core_close(desc_t desc);
static int dev_modem_core_ioctl(desc_t desc,int request,va_list ap);

static const char dev_modem_core_name[]="net/mdmcore";

dev_map_t dev_modem_core_map={
   dev_modem_core_name,
   S_IFBLK,
   dev_modem_core_load,
   dev_modem_core_open,
   dev_modem_core_close,
   __fdev_not_implemented,
   __fdev_not_implemented,
   __fdev_not_implemented,
   __fdev_not_implemented,
   __fdev_not_implemented,
   dev_modem_core_ioctl //ioctl
};

//
#define MODEM_CORE_OPERATION_UNDEFINED_REQUEST        ((uint8_t)(0))
#define MODEM_CORE_OPERATION_CONNECT_REQUEST          ((uint8_t)(1))
#define MODEM_CORE_OPERATION_SEND_REQUEST             ((uint8_t)(2))
#define MODEM_CORE_OPERATION_CLOSE_REQUEST            ((uint8_t)(3))

#define MODEM_CORE_OPERATION_EVENT_ACCEPT             ((uint8_t)(4))
#define MODEM_CORE_OPERATION_EVENT_PACKET             ((uint8_t)(5))

#define MODEM_CORE_OPERATION_GETHOSTBYNAME_REQUEST    ((uint8_t)(6))

#define MODEM_CORE_OPERATION_INPROCESS                ((uint8_t)(0x0F))
#define MODEM_CORE_OPERATION_DONE                     ((uint8_t)(0x1F))
#define MODEM_CORE_OPERATION_FAILED                   ((uint8_t)(0xFF))


//
typedef struct modem_core_message_st{
   uint8_t operation_request_code; //connect, send, recv, close.
   uint8_t operation_response_code;
   void* p_modem_core_connection_info;
}modem_core_message_t;


//
typedef struct modem_core_info_st{
   desc_t  modem_ttys_desc_r;
   desc_t  modem_ttys_desc_w;
   //
   modem_at_parser_context_t modem_at_parser_context;
   //
   kernel_mqueue_t kernel_mqueue_request;
   kernel_mqueue_t kernel_mqueue_response;
   //
   kernel_pthread_t kernel_thread;
}modem_core_info_t;
//
static modem_core_info_t g_modem_core_info={
   .modem_ttys_desc_r=INVALID_DESC,
   .modem_ttys_desc_w=INVALID_DESC
};

//
#define MODEM_CORE_KERNEL_THREAD_STACK_SIZE  2048 //1024//1024
#define MODEM_CORE_KERNEL_THREAD_PRIORITY    10 //10 //20//10 //140
// request: mqueue from user socket connection thread to kernel modem core thread (1 mqueue).
#define MODEM_CORE_KERNEL_MQUEUE_BUFFER_SIZE (32*(sizeof(kernel_mqueue_header_t)+sizeof(modem_core_message_t)))

//specific for network interface and ip stack
#if defined (__KERNEL_NET_IPSTACK) && defined(USE_MODEMIP)
static uint8_t modem_core_mqueue_request_buffer[MODEM_CORE_KERNEL_MQUEUE_BUFFER_SIZE];
static uint8_t modem_core_mqueue_response_buffer[MODEM_CORE_KERNEL_MQUEUE_BUFFER_SIZE];
static _macro_stack_addr char kernel_thread_modem_stack[MODEM_CORE_KERNEL_THREAD_STACK_SIZE];
#endif


//
#ifndef MODEM_CONNECTION_MAX
   #define MODEM_CONNECTION_MAX 1
#endif
// response: mqueue from modem core thread to user socket connection thread (1 mqueue for each connection).
#define MODEM_CONNECTION_KERNEL_MQUEUE_BUFFER_SIZE (4*(sizeof(kernel_mqueue_header_t)+sizeof(modem_core_message_t)))
//
#define CONNECTION_STATUS_CLOSE        ((int)(0))
#define CONNECTION_STATUS_OPEN         ((int)(1))
#define CONNECTION_STATUS_CONNECTED    ((int)(2))
#define CONNECTION_STATUS_SHUTDOWN     ((int)(3))


//
typedef struct modem_core_ip_packet_st{
   uint8_t* p_packet;
   struct modem_core_ip_packet_st* p_packet_next;
}modem_core_ip_packet_t;


//
typedef struct modem_core_recv_packet_parameters_st{
   uint16_t   len;
   uint16_t   r; //amount of data has been read by user level
   uint8_t* buf;
   struct modem_core_recv_packet_parameters_st* next;
}modem_core_recv_packet_parameters_t;

//
typedef struct modem_core_send_packet_parameters_st{
   uint16_t   len;
   uint16_t   w; //amount of data has been sent by modem
   uint8_t* buf;
}modem_core_send_packet_parameters_t;

//
typedef struct modem_core_connection_info_st{
   //
   int modem_core_connexion_index;
   //
   desc_t socket_desc; //lepton bsd socket descriptor 
   //
   int domain;
   int type; 
   int protocol;
   //
   struct sockaddr_in sockaddr_in_bind;
   struct sockaddr_in sockaddr_in_connect;
   //
   int status; //open,connected,shutdown, close;
   //
   kernel_mqueue_t kernel_mqueue;
   kernel_pthread_mutex_t kernel_mutex;
   
   //
   modem_core_send_packet_parameters_t  snd_packet;
   //
   modem_core_recv_packet_parameters_t* rcv_packet_head;
   modem_core_recv_packet_parameters_t* rcv_packet_last;
}modem_core_connection_info_t;

//
static uint8_t modem_core_connection_mqueue_buffer_pool[MODEM_CONNECTION_MAX][MODEM_CONNECTION_KERNEL_MQUEUE_BUFFER_SIZE];
static modem_core_connection_info_t g_modem_core_connection_info_list[MODEM_CONNECTION_MAX]={0};

static int modem_core_mq_post_unconnected_request(void* data, uint8_t operation_request_code);
static int modem_core_mq_post_unconnected_response(void* data, uint8_t operation_request_code, uint8_t operation_response_code);
static int modem_core_mq_wait_unconnected_response(void** data, uint8_t* operation_response_code);

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
static int simcom_at_response_callback_connect_ok(char* at_response);
static int simcom_at_response_callback_connect_fail(char* at_response);
static int simcom_at_response_callback_already_connect(char* at_response);
static int simcom_at_response_callback_send_ok(char* at_response);
static int simcom_at_response_callback_send_fail(char* at_response);
static int simcom_at_response_callback_close_ok(char* at_response);
static int simcom_at_response_callback_receive(char* at_response);
static int simcom_at_response_callback_gethostbyname(char* at_response);

//
static const modem_at_parser_response_callback_t g_modem_at_parser_response_callback_list[]={
   {.at_response = ", CONNECT OK",.at_response_callback=simcom_at_response_callback_connect_ok},
   {.at_response = ", CONNECT FAIL",.at_response_callback=simcom_at_response_callback_connect_fail},
   {.at_response = ", ALREADY CONNECT",.at_response_callback=simcom_at_response_callback_already_connect},
   {.at_response = ", SEND OK",.at_response_callback=simcom_at_response_callback_send_ok},
   {.at_response = ", SEND FAIL",.at_response_callback=simcom_at_response_callback_send_fail},
   {.at_response = ", CLOSED",.at_response_callback=simcom_at_response_callback_close_ok},
   {.at_response = "+RECEIVE",.at_response_callback=simcom_at_response_callback_receive}, 
   {.at_response = "+CDNSGIP:",.at_response_callback = simcom_at_response_callback_gethostbyname },
};

//
static uint8_t g_at_send_buffer[256];
static uint8_t g_modem_core_buffer[256];

/*============================================
| Implementation
==============================================*/


/*--------------------------------------------
| Name:        modem_core_parser_init
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int modem_core_parser_init(modem_at_parser_context_t* modem_at_parser_context,desc_t desc_r, desc_t desc_w, char *buf){
   //
   modem_at_parser_context->state = PARSER_STATE_WAIT_CR;
   //
   modem_at_parser_context->desc_r = desc_r;
   modem_at_parser_context->desc_w = desc_w;
   //
   modem_at_parser_context->response_buffer=buf;
   modem_at_parser_context->pbuf = buf;
   //
   modem_at_parser_context->at_response_callback_list_size=0;
   modem_at_parser_context->at_response_callback_list=(void*)0;
   //
   return 0;
}

/*--------------------------------------------
| Name:        modem_core_parser_set_callback_list
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int modem_core_parser_set_callback_list(modem_at_parser_context_t* modem_at_parser_context, modem_at_parser_response_callback_t* at_response_callback_list, int at_response_callback_list_size){
   modem_at_parser_context->at_response_callback_list=at_response_callback_list;
   modem_at_parser_context->at_response_callback_list_size=at_response_callback_list_size;
   return at_response_callback_list_size;
}

/*--------------------------------------------
| Name:        modem_core_parser_get_last_at_response
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static char* modem_core_parser_get_last_at_response(modem_at_parser_context_t* modem_at_parser_context){
   //
   if(modem_at_parser_context->state!=PARSER_STATE_WAIT_CR){
      return (char*)0;
   }
   //
   return modem_at_parser_context->response_buffer;
}

/*--------------------------------------------
| Name:        modem_core_parser_recv_at_command
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int modem_core_parser_recv_at_response(modem_at_parser_context_t* modem_at_parser_context,const char* expected_response,int nonblock_mode,int silent_mode){
   char c;
   int cb;
   char flag_exit_jocker=0;
   int expected_reponse_len=0;
   int oflag=ofile_lst[modem_at_parser_context->desc_r].oflag;
   pfn_parser_at_response_callback_t at_response_callback=(pfn_parser_at_response_callback_t)0;
   
   //blocking/non bloking mode on read
   if(nonblock_mode==1){
      ofile_lst[modem_at_parser_context->desc_r].oflag|=O_NONBLOCK;
   }
   
   //
   if(expected_response!=(char*)0){
      expected_reponse_len=strlen(expected_response);
   }
   //
   if(expected_response!=(char*)0 && strcmp(expected_response,"*")==0){
      flag_exit_jocker=1;
   }
   //
   while((cb=kernel_io_read(modem_at_parser_context->desc_r,&c,1))>0){
      //kernel trace for debug
      if(silent_mode==0){
         kernel_io_write(__get_kernel_tty_desc(),&c,1);
      }
      // for ex:">"
      if(expected_reponse_len==1 && c==*expected_response){
         ofile_lst[modem_at_parser_context->desc_r].oflag = oflag;
         return 0;
      }
      //
      if(modem_at_parser_context->state==PARSER_STATE_WAIT_CR && c=='\r'){
         modem_at_parser_context->state=PARSER_STATE_WAIT_LF;
         *modem_at_parser_context->pbuf++='\0';
      }else if(modem_at_parser_context->state==PARSER_STATE_WAIT_LF && c=='\n'){
         modem_at_parser_context->state=PARSER_STATE_PARSE;
         *modem_at_parser_context->pbuf++='\0';
      }else{
         *modem_at_parser_context->pbuf++=c;
         *modem_at_parser_context->pbuf='\0';
      }
      //
      if(modem_at_parser_context->state!=PARSER_STATE_PARSE){
         continue;
      }
      
      //parse now
      //restore initial state of parser
      modem_at_parser_context->state=PARSER_STATE_WAIT_CR;
      //
      modem_at_parser_context->pbuf=modem_at_parser_context->response_buffer;
      
      if(*modem_at_parser_context->pbuf=='\0'){
         continue;
      }
     
      //check asynchronous message from modem 
      at_response_callback=(pfn_parser_at_response_callback_t)0;
      //
      for(int i=0; i< modem_at_parser_context->at_response_callback_list_size; i++){
         if(strstr(modem_at_parser_context->response_buffer,modem_at_parser_context->at_response_callback_list[i].at_response)!=(char*)0){
           at_response_callback = modem_at_parser_context->at_response_callback_list[i].at_response_callback;
           break;
         }
      }
      //
      if(at_response_callback!=(pfn_parser_at_response_callback_t)0){
         at_response_callback(modem_at_parser_context->response_buffer);
         //
         continue;
      }
            
      //
      if(flag_exit_jocker){
         //kernel trace for debug
         if(silent_mode==0){
            kernel_io_write(__get_kernel_tty_desc(),modem_at_parser_context->response_buffer,strlen(modem_at_parser_context->response_buffer));
         }
         //
         ofile_lst[modem_at_parser_context->desc_r].oflag = oflag;
         //
         return strlen(modem_at_parser_context->response_buffer);
      }
      
      //
      if(expected_response!=(char*)0){
         if(strstr(modem_at_parser_context->response_buffer,expected_response)!=(char*)0){
            //is expected_response
            //kernel trace for debug
            if(silent_mode==0)
               kernel_printk("\r\n");
            //
            ofile_lst[modem_at_parser_context->desc_r].oflag = oflag;
            //
            return 0;  
         }
         
         //is not expected_response
         if(strstr(modem_at_parser_context->response_buffer,"ERROR")!=(char*)0){
            //kernel trace for debug
            if(silent_mode==0)
               kernel_printk("\r\n");
            //
            ofile_lst[modem_at_parser_context->desc_r].oflag = oflag;
            return -1;
         }
         // nothing to do
         //go to next response :(pbuf=buf;)
      }else if(strstr(modem_at_parser_context->response_buffer,"OK")!=(char*)0){
         //kernel trace for debug
         if(silent_mode==0)
            kernel_printk("\r\n");
         //
         ofile_lst[modem_at_parser_context->desc_r].oflag = oflag;
         return 0;
      }else  if(strstr(modem_at_parser_context->response_buffer,"ERROR")!=(char*)0){
         //kernel trace for debug
          if(silent_mode==0)
            kernel_printk("\r\n");
         //
         ofile_lst[modem_at_parser_context->desc_r].oflag = oflag;
         return -1;
      }
      //
          
   }//while
   
   //
   ofile_lst[modem_at_parser_context->desc_r].oflag = oflag;
   return -1;
}

/*--------------------------------------------
| Name:        modem_core_parser_send_recv_at_command
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int modem_core_parser_send_recv_at_command(modem_at_parser_context_t* modem_at_parser_context,const char* command,const char* expected_response,int silent_mode){
   //
   if(command!=(char*)0 && strlen(command)>0){
      kernel_io_write(modem_at_parser_context->desc_w,command,strlen(command));
      kernel_io_write(modem_at_parser_context->desc_w,"\r",1);
   }
   //
   return modem_core_parser_recv_at_response(modem_at_parser_context,expected_response,0,silent_mode);
}

/*--------------------------------------------
| Name:        modem_core_parser_send_at_command
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int modem_core_parser_send_at_command(modem_at_parser_context_t* modem_at_parser_context,const char* command,int silent_mode){
   //
   if(command!=(char*)0 && strlen(command)>0){
      kernel_io_write(modem_at_parser_context->desc_w,command,strlen(command));
      kernel_io_write(modem_at_parser_context->desc_w,"\r",1);
      return strlen(command);
   }
   //
   return -1;
}


/*--------------------------------------------
| Name:        simcom_at_response_callback_connect_ok
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int simcom_at_response_callback_connect_ok(char* at_response){
   char* pbuf=at_response;
   int modem_core_connexion_index;
   modem_core_message_t modem_core_message;
   
   //get connection number
   while(*pbuf!=',' && *pbuf!='\0'){
      pbuf++;
   }
   *pbuf='\0';
   //
   pbuf= at_response;
   //
   modem_core_connexion_index = atoi(pbuf);
   //
   g_modem_core_connection_info_list[modem_core_connexion_index].status=CONNECTION_STATUS_CONNECTED;
   //
   modem_core_message.operation_request_code  =  MODEM_CORE_OPERATION_CONNECT_REQUEST;
   modem_core_message.operation_response_code =  MODEM_CORE_OPERATION_DONE;
   modem_core_message.p_modem_core_connection_info = &g_modem_core_connection_info_list[modem_core_connexion_index];
   //
   kernel_mqueue_put(&g_modem_core_connection_info_list[modem_core_connexion_index].kernel_mqueue,&modem_core_message,sizeof(modem_core_message));
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
static int simcom_at_response_callback_connect_fail(char* at_response){
   char* pbuf=at_response;
   int modem_core_connexion_index;
   modem_core_message_t modem_core_message;
   
   //get connection number
   while(*pbuf!=',' && *pbuf!='\0'){
      pbuf++;
   }
   *pbuf='\0';
   //
   pbuf= at_response;
   //
   modem_core_connexion_index = atoi(pbuf);
   //
   modem_core_message.operation_request_code  =  MODEM_CORE_OPERATION_CONNECT_REQUEST;
   modem_core_message.operation_response_code =  MODEM_CORE_OPERATION_FAILED;
   modem_core_message.p_modem_core_connection_info = &g_modem_core_connection_info_list[modem_core_connexion_index];
   //
   kernel_mqueue_put(&g_modem_core_connection_info_list[modem_core_connexion_index].kernel_mqueue,&modem_core_message,sizeof(modem_core_message));
  
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
static int simcom_at_response_callback_already_connect(char* at_response){
   char* pbuf=at_response;
   int modem_core_connexion_index;
   modem_core_message_t modem_core_message;
   
   //get connection number
   while(*pbuf!=',' && *pbuf!='\0'){
      pbuf++;
   }
   *pbuf='\0';
   //
   pbuf= at_response;
   //
   modem_core_connexion_index = atoi(pbuf);
   //
   modem_core_message.operation_request_code  =  MODEM_CORE_OPERATION_CONNECT_REQUEST;
   modem_core_message.operation_response_code =  MODEM_CORE_OPERATION_FAILED;
   modem_core_message.p_modem_core_connection_info = &g_modem_core_connection_info_list[modem_core_connexion_index];
   //
   kernel_mqueue_put(&g_modem_core_connection_info_list[modem_core_connexion_index].kernel_mqueue,&modem_core_message,sizeof(modem_core_message));
  
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
static int simcom_at_response_callback_send_ok(char* at_response){
   char* pbuf=at_response;
   int modem_core_connexion_index;
   modem_core_message_t modem_core_message;
   
   //get connection number
   while(*pbuf!=',' && *pbuf!='\0'){
      pbuf++;
   }
   *pbuf='\0';
   //
   pbuf= at_response;
   //
   modem_core_connexion_index = atoi(pbuf);
   //
   modem_core_message.operation_request_code  =  MODEM_CORE_OPERATION_SEND_REQUEST;
   modem_core_message.operation_response_code =  MODEM_CORE_OPERATION_DONE;
   modem_core_message.p_modem_core_connection_info = &g_modem_core_connection_info_list[modem_core_connexion_index];
   //
   kernel_mqueue_put(&g_modem_core_connection_info_list[modem_core_connexion_index].kernel_mqueue,&modem_core_message,sizeof(modem_core_message));
  
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
static int simcom_at_response_callback_send_fail(char* at_response){
   char* pbuf=at_response;
   int modem_core_connexion_index;
   modem_core_message_t modem_core_message;
   
   //get connection number
   while(*pbuf!=',' && *pbuf!='\0'){
      pbuf++;
   }
   *pbuf='\0';
   //
   pbuf= at_response;
   //
   modem_core_connexion_index = atoi(pbuf);
   //
   modem_core_message.operation_request_code  =  MODEM_CORE_OPERATION_SEND_REQUEST;
   modem_core_message.operation_response_code =  MODEM_CORE_OPERATION_FAILED;
   modem_core_message.p_modem_core_connection_info = &g_modem_core_connection_info_list[modem_core_connexion_index];
   //
   kernel_mqueue_put(&g_modem_core_connection_info_list[modem_core_connexion_index].kernel_mqueue,&modem_core_message,sizeof(modem_core_message));
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
static int simcom_at_response_callback_close_ok(char* at_response){
   char* pbuf=at_response;
   int modem_core_connexion_index;
   desc_t desc_r;
   
   //get connection number
   while(*pbuf!=',' && *pbuf!='\0'){
      pbuf++;
   }
   *pbuf='\0';
   //
   pbuf= at_response;
   //
   modem_core_connexion_index = atoi(pbuf);
   // set status to shutdown.
   g_modem_core_connection_info_list[modem_core_connexion_index].status=CONNECTION_STATUS_SHUTDOWN;
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
static int simcom_at_response_callback_receive(char* at_response){
   char* pbuf=at_response;
   int modem_core_connexion_index;
   int recv_packet_len=0;
   int len=0;
   int cb;
   int oflag;
   desc_t desc_r;
   uint8_t flag_signal=0;
   modem_core_recv_packet_parameters_t* p_recv_packet;
   
   //get +Receive
   while(*pbuf!=',' && *pbuf!='\0'){
      pbuf++;
   }
   //
   if(*pbuf=='\0'){
      return -1;
   }
   //
   pbuf++;
   
   //get connection number
   at_response=pbuf;
   //
   while(*pbuf!=',' && *pbuf!='\0'){
      pbuf++;
   }
   //
   if(*pbuf=='\0'){
      return -1;
   }
   *pbuf='\0';
   //
   modem_core_connexion_index = atoi(at_response);
   //
   pbuf++;
   //get receive data len
   at_response=pbuf;
   //
   while(*pbuf!=':' && *pbuf!='\0'){
      pbuf++;
   }
   //
   if(*pbuf=='\0'){
      return -1;
   }
   //
   *pbuf='\0';
   //
   recv_packet_len = atoi(at_response);
   //
   if(recv_packet_len<=0){
      return 0;
   }
   
   //
   p_recv_packet=(modem_core_recv_packet_parameters_t*)_sys_malloc(sizeof(modem_core_recv_packet_parameters_t));
   if(!p_recv_packet){
      return -1;
   }
   //
   p_recv_packet->len=recv_packet_len;
   p_recv_packet->r=0;
   p_recv_packet->buf= (uint8_t*)_sys_malloc(recv_packet_len);
   if(!p_recv_packet->buf){
      _sys_free(p_recv_packet);
      return -1;
   }
   
   //
   len=0;
   //save and set oflag
   oflag = ofile_lst[g_modem_core_info.modem_ttys_desc_r].oflag;
   ofile_lst[g_modem_core_info.modem_ttys_desc_r].oflag &= ~(O_NONBLOCK);
   //
   while(len<recv_packet_len){
      if((cb=kernel_io_read(g_modem_core_info.modem_ttys_desc_r,p_recv_packet->buf+len,recv_packet_len-len))<0){
         ofile_lst[g_modem_core_info.modem_ttys_desc_r].oflag=oflag;
         return -1;
      }
      len+=cb;
   }
   //restore oflag 
   ofile_lst[g_modem_core_info.modem_ttys_desc_r].oflag=oflag;
   
   //lock 
   kernel_pthread_mutex_lock(&g_modem_core_connection_info_list[modem_core_connexion_index].kernel_mutex);
   //
   p_recv_packet->next=(modem_core_recv_packet_parameters_t*)0;
   //
   if(!g_modem_core_connection_info_list[modem_core_connexion_index].rcv_packet_head){
      flag_signal=1;
      g_modem_core_connection_info_list[modem_core_connexion_index].rcv_packet_head=p_recv_packet;
      g_modem_core_connection_info_list[modem_core_connexion_index].rcv_packet_last=g_modem_core_connection_info_list[modem_core_connexion_index].rcv_packet_head;
   }else{
       g_modem_core_connection_info_list[modem_core_connexion_index].rcv_packet_last->next=p_recv_packet;
       g_modem_core_connection_info_list[modem_core_connexion_index].rcv_packet_last=p_recv_packet;
   }
   
   //unlock 
   kernel_pthread_mutex_unlock(&g_modem_core_connection_info_list[modem_core_connexion_index].kernel_mutex);
   
   
   //signal 
   if(flag_signal){
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
static int simcom_at_response_callback_gethostbyname(char* at_response) {
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
| Name: modem_core_simcom_connect
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int modem_core_simcom_connect(modem_core_connection_info_t* p_modem_core_connection_info) {
   char ltostr_buffer[4];
   //
   if(p_modem_core_connection_info==(void*)0){
      return -1;
   }
   //
   if(p_modem_core_connection_info->status!=CONNECTION_STATUS_OPEN){
      return -1;
   }
   //
   memset(g_at_send_buffer,0,sizeof(g_at_send_buffer));
   //TO DO send command with mqueue and signal thread g_modem_core_info.kernel_thread_connection2modem.sem_io 
   // wait result in socket mqueue
   
   //at command
   strcpy(g_at_send_buffer,"AT+CIPSTART=");
   //connection number
   strcat(g_at_send_buffer, ltostr(ltostr_buffer,sizeof(ltostr_buffer),p_modem_core_connection_info->modem_core_connexion_index,10,0));
   //protocol
   if(p_modem_core_connection_info->type==SOCK_DGRAM && p_modem_core_connection_info->protocol==IPPROTO_UDP){
      strcat(g_at_send_buffer,",\"UDP\",");
   }else if(p_modem_core_connection_info->type==SOCK_STREAM && p_modem_core_connection_info->protocol==IPPROTO_TCP){
      strcat(g_at_send_buffer,",\"TCP\",");
   }else{
      return -1;
   }
   // modem at "AT+CIPSTART=\"TCP\",\"hrc.o10ee.com\",80"
   //ip address
   uint8_t* ip_addr = (uint8_t*)&p_modem_core_connection_info->sockaddr_in_connect.sin_addr.s_addr;
   //src[0], src[1], src[2], src[3]
   strcat(g_at_send_buffer,"\"");
   strcat(g_at_send_buffer, ltostr(ltostr_buffer,sizeof(ltostr_buffer),ip_addr[0],10,0));
   strcat(g_at_send_buffer,".");
   strcat(g_at_send_buffer, ltostr(ltostr_buffer,sizeof(ltostr_buffer),ip_addr[1],10,0));
   strcat(g_at_send_buffer,".");
   strcat(g_at_send_buffer, ltostr(ltostr_buffer,sizeof(ltostr_buffer),ip_addr[2],10,0));
   strcat(g_at_send_buffer,".");
   strcat(g_at_send_buffer, ltostr(ltostr_buffer,sizeof(ltostr_buffer),ip_addr[3],10,0));
   strcat(g_at_send_buffer,"\",");
   // port number
   in_port_t port_no = ntohs(p_modem_core_connection_info->sockaddr_in_connect.sin_port);
   strcat(g_at_send_buffer,"\"");
   strcat(g_at_send_buffer, ltostr(ltostr_buffer,sizeof(ltostr_buffer),port_no,10,0));
   strcat(g_at_send_buffer,"\"");
   
   //
   if(modem_core_parser_send_at_command(&g_modem_core_info.modem_at_parser_context,g_at_send_buffer,0)<0)
      return -1; 
   //
  return 0;
}

/*--------------------------------------------
| Name: modem_core_simcom_close
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int modem_core_simcom_close(modem_core_connection_info_t* p_modem_core_connection_info) { 
   //
   char ltostr_buffer[4];
   //
   if(p_modem_core_connection_info==(void*)0){
      return -1;
   }
   //
   if(p_modem_core_connection_info->status!=CONNECTION_STATUS_CONNECTED){
      return -1;
   }
   //
   memset(g_at_send_buffer,0,sizeof(g_at_send_buffer));
   //at command
   strcpy(g_at_send_buffer,"AT+CIPCLOSE=");
   //connection number
   strcat(g_at_send_buffer, ltostr(ltostr_buffer,sizeof(ltostr_buffer),p_modem_core_connection_info->modem_core_connexion_index,10,0));
   //
    strcat(g_at_send_buffer,",0");
   //
   if(modem_core_parser_send_recv_at_command(&g_modem_core_info.modem_at_parser_context,g_at_send_buffer,"CLOSE OK",0)<0)
      return -1;      
   //
   p_modem_core_connection_info->status=CONNECTION_STATUS_SHUTDOWN;
   //
   return 0;
}

/*--------------------------------------------
| Name: modem_core_simcom_send
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int modem_core_simcom_send(modem_core_connection_info_t* p_modem_core_connection_info) {
   char ltostr_buffer[4];
   int cb=0;
   int len = 0;
   int snd_packet_len;
   uint8_t* snd_packet_buf = p_modem_core_connection_info->snd_packet.buf;
   
   //
   if(p_modem_core_connection_info->status!=CONNECTION_STATUS_CONNECTED){
      return -1;
   }
   //
   memset(g_at_send_buffer,0,sizeof(g_at_send_buffer));
   
   //at command
   strcpy(g_at_send_buffer,"AT+CIPSEND=");
   //connection number
   strcat(g_at_send_buffer, ltostr(ltostr_buffer,sizeof(ltostr_buffer),p_modem_core_connection_info->modem_core_connexion_index,10,0));
   //
   strcat(g_at_send_buffer,",");
   //
   snd_packet_len = p_modem_core_connection_info->snd_packet.len;
   //
   strcat(g_at_send_buffer, ltostr(ltostr_buffer,sizeof(ltostr_buffer),snd_packet_len,10,0));
   //   
   if(modem_core_parser_send_recv_at_command(&g_modem_core_info.modem_at_parser_context,g_at_send_buffer,">",0)<0)
     return -1;
   //
   while(len<snd_packet_len){
      if((cb=kernel_io_write(g_modem_core_info.modem_ttys_desc_w,snd_packet_buf+len,snd_packet_len-len))<0)
         return -1;
      len+=cb;
   }
   //
   return 0;
}

/*--------------------------------------------
| Name: modem_core_simcom_gethostbyname
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int modem_core_simcom_gethostbyname(void* data) {
   char* hostname=(char*)data;

   //
   if(!hostname){
      return -1;
   }
   //
   if(!strlen(hostname)){
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
   if (modem_core_parser_send_recv_at_command(&g_modem_core_info.modem_at_parser_context, g_at_send_buffer, "OK", 0)<0)
      return -1;
   //
   return 0;
}

/*--------------------------------------------
| Name: modem_core_simcom_reset
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int modem_core_simcom_reset(desc_t desc_r,desc_t desc_w){
   int cb;
   //
   unsigned long oflag=0;
   int retry_counter=10;
   //
   uint8_t* buf = g_modem_core_buffer;
   
   //
   desc_t desc_board= _vfs_open("/dev/board",O_RDWR,0);
   //
   if(desc_board==INVALID_DESC)
      return -1;
   // to do : reimplement _sys_fcntl(fd,cmd,...)
   oflag = ofile_lst[desc_r].oflag;
   ofile_lst[desc_r].oflag = oflag | O_NONBLOCK;
   //  
   kernel_printk("modem reset...\r\n");
   //
   _vfs_ioctl(desc_board,BRDRESET,(void*)0);
   //
   kernel_printk("modem reset ok\r\n");
   //
   kernel_printk("modem setup communication...\r\n");
   //
   kernel_io_write(desc_w,at_cmd1,sizeof(at_cmd1)-1);
   //
   while(retry_counter-->0){
      __kernel_usleep(100000);
      //
      kernel_printk("modem send AT\r\n");
      //
      kernel_io_write(desc_w,at_cmd1,sizeof(at_cmd1)-1);
      //
      __kernel_usleep(1000000);
      //
      cb=kernel_io_read(desc_r,buf,sizeof(buf));
      if(cb<=0)
       continue;
      //
      kernel_printk("modem rcv:");
      //
      while(cb>0){
         kernel_io_write(__get_kernel_tty_desc(),buf,cb);
         cb=kernel_io_read(desc_r,buf,sizeof(buf));
      }
      //
      break;
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
| Name: modem_core_init
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int modem_core_simcom_init(desc_t desc_r,desc_t desc_w){
   //
   for(int i=0; i<(sizeof(modem_at_command_list)/sizeof(modem_at_command_t));i++){
      //
      kernel_io_ll_ioctl(desc_r,TCFLSH,TCIFLUSH);
      //
      if(modem_core_parser_send_recv_at_command(&g_modem_core_info.modem_at_parser_context,modem_at_command_list[i].command,modem_at_command_list[i].response,0)<0)
         return -1;
      //
      __kernel_usleep(1000000);
      //
   }
   //
   return 0;
}


/*--------------------------------------------
| Name: modem_core_routine
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static void* modem_core_routine(void* arg){
   modem_core_info_t* p_modem_core_info = (modem_core_info_t*)arg;
   //
   kernel_pthread_t* pthread_ptr = kernel_pthread_self();

   //
   __atomic_in();
   //begin of section: protection from io interrupt
   __disable_interrupt_section_in();
   //force thread owner
   ofile_lst[g_modem_core_info.modem_ttys_desc_r].owner_pthread_ptr_read = pthread_ptr;
   //end of section: protection from io interrupt
   __disable_interrupt_section_out();
   //
   __atomic_out();
   
   //
   kernel_printk("\r\nmodem_core_routine...\r\n");
   //
   while(1){
      //wait on sem_io of this thread. Just one thread for connection command and receive data from modem / ttys 
      if(kernel_mqueue_isempty(&g_modem_core_info.kernel_mqueue_request) 
          && ofile_lst[g_modem_core_info.modem_ttys_desc_r].pfsop->fdev.fdev_isset_read(g_modem_core_info.modem_ttys_desc_r)!=0){
         __wait_io_int(pthread_ptr); //wait incomming data
      }
      
      //check data available in kernel_mqueue and in uart driver
      
      // 1: data available in uart  with isset_read (unsolicited message from modem)
      if(!ofile_lst[g_modem_core_info.modem_ttys_desc_r].pfsop->fdev.fdev_isset_read(g_modem_core_info.modem_ttys_desc_r)){   
         
         //
         if(modem_core_parser_recv_at_response(&g_modem_core_info.modem_at_parser_context,(char*)0,1,0)>0){
            char* at_response;
            at_response = modem_core_parser_get_last_at_response(&g_modem_core_info.modem_at_parser_context);
            if(at_response!=(char*)0){
            }
         }
      }
      
      
      //2: if data available in mqueue
      if(!kernel_mqueue_isempty(&g_modem_core_info.kernel_mqueue_request)){
         modem_core_message_t modem_core_message;
         modem_core_connection_info_t* p_modem_core_connection_info;
         // decode and send command to modem.
         // wait data from modem
         // send result to socket mqueue
         if(kernel_mqueue_get(&g_modem_core_info.kernel_mqueue_request,&modem_core_message,sizeof(modem_core_message_t))<0){
            continue;
         }
         //
         p_modem_core_connection_info = (modem_core_connection_info_t*) modem_core_message.p_modem_core_connection_info;
         if(p_modem_core_connection_info==(void*)0){
            continue;
         }
         
         //
         switch(modem_core_message.operation_request_code){
            //
            case MODEM_CORE_OPERATION_CONNECT_REQUEST:
               if(modem_core_simcom_connect(p_modem_core_connection_info)<0){
                  modem_core_message.operation_response_code =  MODEM_CORE_OPERATION_FAILED;
                  //
                  kernel_mqueue_put(&p_modem_core_connection_info->kernel_mqueue,&modem_core_message,sizeof(modem_core_message));
               }
            break;
            
            //
            case MODEM_CORE_OPERATION_CLOSE_REQUEST:
               if(modem_core_simcom_close(p_modem_core_connection_info)<0){
                  modem_core_message.operation_response_code =  MODEM_CORE_OPERATION_FAILED;
               }else{
                  modem_core_message.operation_response_code =  MODEM_CORE_OPERATION_DONE;
               }
               //
               kernel_mqueue_put(&p_modem_core_connection_info->kernel_mqueue,&modem_core_message,sizeof(modem_core_message));
            break;
            
            //
            case MODEM_CORE_OPERATION_SEND_REQUEST:
               if(modem_core_simcom_send(p_modem_core_connection_info)<0){
                  //
                  kernel_mqueue_put(&p_modem_core_connection_info->kernel_mqueue,&modem_core_message,sizeof(modem_core_message));
               }
            break;

            //
            case MODEM_CORE_OPERATION_GETHOSTBYNAME_REQUEST:
               if (modem_core_simcom_gethostbyname(p_modem_core_connection_info)<0) {
                  //
                  modem_core_mq_post_unconnected_response((void*)0, MODEM_CORE_OPERATION_GETHOSTBYNAME_REQUEST, MODEM_CORE_OPERATION_FAILED);
               }
               break;
               
            case MODEM_CORE_OPERATION_UNDEFINED_REQUEST:
               simcom_at_response_callback_gethostbyname("+CDNSGIP: 1,\"collector.o10ee.com\",\"217.182.129.15\"");
               modem_core_mq_post_unconnected_response((void*)0, MODEM_CORE_OPERATION_UNDEFINED_REQUEST, MODEM_CORE_OPERATION_DONE);
            break;
            
            //
            default:
               continue;
            break;
            
         }
         //
        
      }
      
     
   }
   return(void*)0;
}


/*--------------------------------------------
| Name: modem_core_mq_post_unconnected_request
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int modem_core_mq_post_unconnected_request(void* data, uint8_t operation_request_code) {
   modem_core_message_t modem_core_message;
   //
   modem_core_message.operation_request_code = operation_request_code;
   modem_core_message.operation_response_code = MODEM_CORE_OPERATION_INPROCESS;
   modem_core_message.p_modem_core_connection_info = data;
   //
   if (kernel_mqueue_put(&g_modem_core_info.kernel_mqueue_request, &modem_core_message, sizeof(modem_core_message_t))<0) {
      return -1;
   }
   //force unblock read
   __fire_io(ofile_lst[g_modem_core_info.modem_ttys_desc_r].owner_pthread_ptr_read);
   //
   return 0;
}

/*--------------------------------------------
| Name: modem_core_mq_post_unconnected_response
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int modem_core_mq_post_unconnected_response(void* data, uint8_t operation_request_code,uint8_t operation_response_code) {
   modem_core_message_t modem_core_message;
   //
   modem_core_message.operation_request_code = operation_request_code;
   modem_core_message.operation_response_code = operation_response_code;
   modem_core_message.p_modem_core_connection_info = data;
   //
   if (kernel_mqueue_put(&g_modem_core_info.kernel_mqueue_response, &modem_core_message, sizeof(modem_core_message_t))<0) {
      return -1;
   }
   //
   return 0;
}

/*--------------------------------------------
| Name: modem_core_mq_wait_unconnected_response
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int modem_core_mq_wait_unconnected_response(void** data, uint8_t* operation_response_code) {
   modem_core_message_t modem_core_message;
   //
   if (kernel_mqueue_get(&g_modem_core_info.kernel_mqueue_response, &modem_core_message, sizeof(modem_core_message_t))<0) {
      return -1;
   }
   //
   *data = modem_core_message.p_modem_core_connection_info;
   //
   *operation_response_code = modem_core_message.operation_response_code;
   //
   return 0;
}

/*--------------------------------------------
| Name: modem_core_mq_post_request
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int modem_core_mq_post_request(modem_core_connection_info_t* p_modem_core_connection_info,uint8_t operation_request_code){
   modem_core_message_t modem_core_message;
   //
   modem_core_message.operation_request_code = operation_request_code;
   modem_core_message.operation_response_code = MODEM_CORE_OPERATION_INPROCESS;
   modem_core_message.p_modem_core_connection_info=p_modem_core_connection_info;
   //
   if(kernel_mqueue_put(&g_modem_core_info.kernel_mqueue_request,&modem_core_message,sizeof(modem_core_message_t))<0){
      return -1;
   }
   //force unblock read
   __fire_io(ofile_lst[g_modem_core_info.modem_ttys_desc_r].owner_pthread_ptr_read);
   //
   return 0;
}

/*--------------------------------------------
| Name: modem_core_mq_wait_response
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int modem_core_mq_wait_response(modem_core_connection_info_t* p_modem_core_connection_info,uint8_t* operation_response_code){
   modem_core_message_t modem_core_message;
   //
   if(kernel_mqueue_get(&p_modem_core_connection_info->kernel_mqueue,&modem_core_message,sizeof(modem_core_message_t))<0){
      return -1;
   }
   //
   *operation_response_code = modem_core_message.operation_response_code;
   //
   return 0;
}

/*--------------------------------------------
| Name: modem_core_link
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int modem_core_link(desc_t desc_r,desc_t desc_w){
   pthread_attr_t          thread_attr={0}; 
   kernel_mqueue_attr_t    kernel_mqueue_attr={0}; 
    
   //
   if(g_modem_core_info.modem_ttys_desc_r!=INVALID_DESC && g_modem_core_info.modem_ttys_desc_w!=INVALID_DESC){
      return -1;
   }
   
   //
   if(g_modem_core_info.modem_ttys_desc_r==INVALID_DESC){
      g_modem_core_info.modem_ttys_desc_r =desc_r;
   }
   
   if(g_modem_core_info.modem_ttys_desc_w==INVALID_DESC){
      g_modem_core_info.modem_ttys_desc_w =desc_w;
   }
   
   //
   modem_core_parser_init(&g_modem_core_info.modem_at_parser_context,desc_r,desc_w,g_modem_core_buffer);
   //
   modem_core_parser_set_callback_list(&g_modem_core_info.modem_at_parser_context,
                                       g_modem_at_parser_response_callback_list,
                                       sizeof(g_modem_at_parser_response_callback_list)/sizeof(modem_at_parser_response_callback_t));
                          
   //to do: modem power up
   //modem reset
   if(modem_core_simcom_reset(desc_r,desc_w)<0){
      return -1;
   }
   //
   if(modem_core_simcom_init(desc_r,desc_w)<0){
      return -1;
   }
   //
#if defined (__KERNEL_NET_IPSTACK) && defined(USE_MODEMIP)
   
   //request queue
   kernel_mqueue_attr.buffer = modem_core_mqueue_request_buffer;
   kernel_mqueue_attr.size = sizeof(modem_core_mqueue_request_buffer);
   kernel_mqueue_init(&g_modem_core_info.kernel_mqueue_request, &kernel_mqueue_attr);

   //response queue
   kernel_mqueue_attr.buffer = modem_core_mqueue_response_buffer;
   kernel_mqueue_attr.size = sizeof(modem_core_mqueue_response_buffer);
   kernel_mqueue_init(&g_modem_core_info.kernel_mqueue_response, &kernel_mqueue_attr);

   
   //
   thread_attr.stacksize = MODEM_CORE_KERNEL_THREAD_STACK_SIZE;
   thread_attr.stackaddr = (void*)&kernel_thread_modem_stack;
   thread_attr.priority  = MODEM_CORE_KERNEL_THREAD_PRIORITY;
   thread_attr.timeslice = 0;
   thread_attr.name = "kernel_pthread_modem_core_serial";
   //
   kernel_pthread_create(&g_modem_core_info.kernel_thread,&thread_attr,modem_core_routine,&g_modem_core_info);
   
   //init modem core connection
   int modem_core_connexion_index;
   for(modem_core_connexion_index=0;
      modem_core_connexion_index<MODEM_CONNECTION_MAX;
      modem_core_connexion_index++){
      
         kernel_mqueue_attr_t    kernel_mqueue_attr={0}; 
         pthread_mutexattr_t     mutex_attr=0;
         //
         g_modem_core_connection_info_list[modem_core_connexion_index].modem_core_connexion_index=modem_core_connexion_index;
         //
         g_modem_core_connection_info_list[modem_core_connexion_index].socket_desc=INVALID_DESC;
         //
         g_modem_core_connection_info_list[modem_core_connexion_index].status=CONNECTION_STATUS_CLOSE;
         //
         kernel_mqueue_attr.buffer = modem_core_connection_mqueue_buffer_pool[modem_core_connexion_index];
         kernel_mqueue_attr.size = MODEM_CONNECTION_KERNEL_MQUEUE_BUFFER_SIZE;
         kernel_mqueue_init(&g_modem_core_connection_info_list[modem_core_connexion_index].kernel_mqueue, &kernel_mqueue_attr);
         //
         if(kernel_pthread_mutex_init(&g_modem_core_connection_info_list[modem_core_connexion_index].kernel_mutex,&mutex_attr)<0)
            return -1;
         //
         g_modem_core_connection_info_list[modem_core_connexion_index].rcv_packet_last=(modem_core_recv_packet_parameters_t*)0;
         g_modem_core_connection_info_list[modem_core_connexion_index].rcv_packet_head=(modem_core_recv_packet_parameters_t*)0;
   }
#endif
   
   //
   return 0;
}

/*--------------------------------------------
| Name: modem_core_unlink
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static  int modem_core_unlink(void){
   //to do: shutdown the modem. shutdown all connections. 
   //to do: modem power down
   return 0;
}


/*--------------------------------------------
| Name:        dev_modem_core_load
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int dev_modem_core_load(void){
   return 0;
}

/*--------------------------------------------
| Name:        dev_modem_core_open
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static  int dev_modem_core_open(desc_t desc, int o_flag){

   if(ofile_lst[desc].oflag & O_RDONLY) {
   }

   if(ofile_lst[desc].oflag & O_WRONLY) {
   }

   if(!ofile_lst[desc].nb_writer
      &&!ofile_lst[desc].nb_reader) {
   }

   return 0;
}

/*--------------------------------------------
| Name:        dev_modem_core_close
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int dev_modem_core_close(desc_t desc){
   if(ofile_lst[desc].oflag & O_RDONLY) {
      if(!ofile_lst[desc].nb_reader) {

      }
   }

   if(ofile_lst[desc].oflag & O_WRONLY) {
      if(!ofile_lst[desc].nb_writer) {
      }
   }

   if(!ofile_lst[desc].nb_writer
      &&!ofile_lst[desc].nb_reader) {
   }
   return 0;
}

/*--------------------------------------------
| Name:        dev_modem_core_ioctl
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int dev_modem_core_ioctl(desc_t desc,int request,va_list ap){
   switch(request) {

   case I_LINK: {
      //must be open in O_RDWR mode
      if((ofile_lst[desc].oflag&O_RDWR)!=O_RDWR)
         return -1;

      //link modem ip stack with serial peripheral.
      if(!(ofile_lst[desc].p)){
         if(modem_core_link(ofile_lst[desc].desc_nxt[0],ofile_lst[desc].desc_nxt[1])<0){
            return -1;
         }
         //
         ofile_lst[desc].p=&g_modem_core_info;
      }

   }
   break;

   case I_UNLINK: {
      if(!(ofile_lst[desc].p)){
         return -1;
      }
      //
      if(modem_core_unlink()<0){
         return -1;
      }
   }
   break;


   case IFGETCFG: {
      if_config_t* p_if_config= va_arg( ap, if_config_t*);
      if(!p_if_config)
         return -1;
      if(!(ofile_lst[desc].p))
         return -1;
      //to do
      //memcpy(p_if_config,&((lwip_if_t*)ofile_lst[desc].p)->if_config,sizeof(if_config_t));
   }
   break;

   case IFSETCFG: {
      if_config_t* p_if_config= va_arg( ap, if_config_t*);
      if(!p_if_config)
         return -1;
      if(!(ofile_lst[desc].p))
         return -1;
      //to do
      //memcpy(&((lwip_if_t*)ofile_lst[desc].p)->if_config,p_if_config,sizeof(if_config_t));
      //config_lwip_if(((lwip_if_t*)ofile_lst[desc].p));
   }
   break;


   //
   default:
      return -1;
   }

   return 0;
}

/*--------------------------------------------
| Name: modem_core_api_socket
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int modem_core_api_socket(desc_t desc,int domain, int type, int protocol){
   int modem_core_connexion_index;
   for(modem_core_connexion_index=0;
      modem_core_connexion_index<MODEM_CONNECTION_MAX;
      modem_core_connexion_index++){
      //
      if(g_modem_core_connection_info_list[modem_core_connexion_index].status==CONNECTION_STATUS_CLOSE){
          //
          if(domain!=AF_INET){
             return -1;
          }
          //
          if(type==SOCK_DGRAM && protocol!=IPPROTO_UDP){
             return -1;
          }
          //
          if(type==SOCK_STREAM && protocol!=IPPROTO_TCP){
             return -1;
          }
          //
          g_modem_core_connection_info_list[modem_core_connexion_index].socket_desc = desc;
          //
          g_modem_core_connection_info_list[modem_core_connexion_index].domain     = domain;
          g_modem_core_connection_info_list[modem_core_connexion_index].type       = type;
          g_modem_core_connection_info_list[modem_core_connexion_index].protocol   = protocol;
          //
          g_modem_core_connection_info_list[modem_core_connexion_index].status=CONNECTION_STATUS_OPEN;
          //
          return modem_core_connexion_index;
      }
   }
   //
   return -1;
}

/*--------------------------------------------
| Name: modem_core_api_bind
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int modem_core_api_bind(int modem_core_connexion_index, struct sockaddr *name, socklen_t namelen) {
   if(modem_core_connexion_index<0 || modem_core_connexion_index>(MODEM_CONNECTION_MAX-1)){
      return -1;
   }
   //
   if(g_modem_core_connection_info_list[modem_core_connexion_index].status!=CONNECTION_STATUS_OPEN){
      return -1;
   }
   //
   if(namelen!=sizeof(struct sockaddr_in)){
      return -1;
   }
   //
   memcpy(&g_modem_core_connection_info_list[modem_core_connexion_index].sockaddr_in_bind,&name,namelen); 
   //
   return 0;
}

/*--------------------------------------------
| Name: modem_core_api_accept
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int modem_core_api_accept(int modem_core_connexion_index, struct sockaddr *addr, socklen_t *addrlen) {
   // not yet supported
   // return new_modem_core_connexion_index
   return -1;
}

/*--------------------------------------------
| Name: modem_core_api_accepted
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int modem_core_api_accepted(desc_t desc,int modem_core_connexion_index) {
   // to do= make association new desc socket and new_modem_core_connexion_index from modem_core_api_accept.
   return -1;
}

/*--------------------------------------------
| Name: modem_core_api_connect
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int modem_core_api_connect(int modem_core_connexion_index, struct sockaddr *name, socklen_t namelen) {
   uint8_t operation_request_code;
   //
   if(modem_core_connexion_index<0 || modem_core_connexion_index>(MODEM_CONNECTION_MAX-1)){
      return -1;
   }
   //
   if(g_modem_core_connection_info_list[modem_core_connexion_index].status!=CONNECTION_STATUS_OPEN){
      return -1;
   }
   //
   if(namelen!=sizeof(struct sockaddr_in)){
      return -1;
   }
   
   //
   memcpy(&g_modem_core_connection_info_list[modem_core_connexion_index].sockaddr_in_connect,name,namelen); 
   
   //
   operation_request_code = MODEM_CORE_OPERATION_CONNECT_REQUEST;
   //
   if(modem_core_mq_post_request(&g_modem_core_connection_info_list[modem_core_connexion_index],operation_request_code)<0){
      return -1;
   }
   //
   if(modem_core_mq_wait_response(&g_modem_core_connection_info_list[modem_core_connexion_index],&operation_request_code)<0){
      return -1;
   }
   //
   if(operation_request_code!=MODEM_CORE_OPERATION_DONE){
      return -1;
   }
   //
   return 0;
}

/*--------------------------------------------
| Name: modem_core_api_listen
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int modem_core_api_listen(int modem_core_connexion_index, int backlog) {
   // not yet supported
   return -1;
}

/*--------------------------------------------
| Name: modem_core_api_shutdown
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int modem_core_api_shutdown(int modem_core_connexion_index, int how) {
   uint8_t operation_request_code;
   //
   if(modem_core_connexion_index<0 || modem_core_connexion_index>(MODEM_CONNECTION_MAX-1)){
      return -1;
   }
   //
   if(g_modem_core_connection_info_list[modem_core_connexion_index].status!=CONNECTION_STATUS_CONNECTED){
      return -1;
   }
   //
   operation_request_code = MODEM_CORE_OPERATION_CLOSE_REQUEST;
   //
   if(modem_core_mq_post_request(&g_modem_core_connection_info_list[modem_core_connexion_index],operation_request_code)<0){
      return -1;
   }
   //
   if(modem_core_mq_wait_response(&g_modem_core_connection_info_list[modem_core_connexion_index],&operation_request_code)<0){
      return -1;
   }
   //
   if(operation_request_code!=MODEM_CORE_OPERATION_DONE){
      return -1;
   }
  
   return 0;
}

/*--------------------------------------------
| Name: modem_core_api_close
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int modem_core_api_close(int modem_core_connexion_index) { 
   modem_core_recv_packet_parameters_t* p_rcv_packet_head;
   
   //
   if(modem_core_connexion_index<0 || modem_core_connexion_index>(MODEM_CONNECTION_MAX-1)){
      return -1;
   }
   //
   if(g_modem_core_connection_info_list[modem_core_connexion_index].status==CONNECTION_STATUS_CLOSE){
      return -1;
   }
   
   //lock 
   kernel_pthread_mutex_lock(&g_modem_core_connection_info_list[modem_core_connexion_index].kernel_mutex);
   
   //free all packet has been received and flush connection queue.
   while((p_rcv_packet_head=g_modem_core_connection_info_list[modem_core_connexion_index].rcv_packet_head)){
      g_modem_core_connection_info_list[modem_core_connexion_index].rcv_packet_head=g_modem_core_connection_info_list[modem_core_connexion_index].rcv_packet_head->next;
      _sys_free(p_rcv_packet_head->buf);
      _sys_free(p_rcv_packet_head);
   }
   //
   g_modem_core_connection_info_list[modem_core_connexion_index].status=CONNECTION_STATUS_CLOSE;
      
   //unlock 
   kernel_pthread_mutex_unlock(&g_modem_core_connection_info_list[modem_core_connexion_index].kernel_mutex);
   
   //
   return 0;
}

/*--------------------------------------------
| Name: modem_core_api_isset_read
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int  modem_core_api_isset_read(int modem_core_connexion_index){
   //
   //
   if(modem_core_connexion_index<0 || modem_core_connexion_index>(MODEM_CONNECTION_MAX-1)){
      return -1;
   }
   //
   if(g_modem_core_connection_info_list[modem_core_connexion_index].status!=CONNECTION_STATUS_CONNECTED){
      if(g_modem_core_connection_info_list[modem_core_connexion_index].status==CONNECTION_STATUS_OPEN){
         return -1;
      }
      if(g_modem_core_connection_info_list[modem_core_connexion_index].status==CONNECTION_STATUS_CLOSE){
         return -1;
      }
   }
   //connected or shutdown
   if(g_modem_core_connection_info_list[modem_core_connexion_index].status==CONNECTION_STATUS_SHUTDOWN){
         return 0;
   }
   //packet available
   if(!g_modem_core_connection_info_list[modem_core_connexion_index].rcv_packet_head){
      return -1;
   }
   //
   return 0;
}

/*--------------------------------------------
| Name: modem_core_api_isset_write
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int  modem_core_api_isset_write(int modem_core_connexion_index){
   return 0;//synchronous
}

/*--------------------------------------------
| Name: modem_core_api_read
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int modem_core_api_read(int modem_core_connexion_index,char* buf, int size){
   modem_core_recv_packet_parameters_t* p_rcv_packet_head;
   int data_available=0;
   
    
   //
   if(modem_core_connexion_index<0 || modem_core_connexion_index>(MODEM_CONNECTION_MAX-1)){
      return -1;
   }
   //
   if(g_modem_core_connection_info_list[modem_core_connexion_index].status!=CONNECTION_STATUS_CONNECTED){
      if(g_modem_core_connection_info_list[modem_core_connexion_index].status==CONNECTION_STATUS_OPEN){
         return -1;
      }
      if(g_modem_core_connection_info_list[modem_core_connexion_index].status==CONNECTION_STATUS_CLOSE){
         return -1;
      }
      if(g_modem_core_connection_info_list[modem_core_connexion_index].status==CONNECTION_STATUS_SHUTDOWN && !g_modem_core_connection_info_list[modem_core_connexion_index].rcv_packet_head){
         return -1;
      }
   }
   
   //lock 
   kernel_pthread_mutex_lock(&g_modem_core_connection_info_list[modem_core_connexion_index].kernel_mutex);
   
   //
   p_rcv_packet_head=g_modem_core_connection_info_list[modem_core_connexion_index].rcv_packet_head;
   if(!p_rcv_packet_head){
      //unlock 
      kernel_pthread_mutex_unlock(&g_modem_core_connection_info_list[modem_core_connexion_index].kernel_mutex);   
      return 0;
   }
   
   //
   data_available=p_rcv_packet_head->len - p_rcv_packet_head->r;
   //
   if(data_available<=size){
      size=data_available;
   }
   //
   memcpy(buf,p_rcv_packet_head->buf,size); 
   //
   p_rcv_packet_head->r+=size;
   //
   data_available=p_rcv_packet_head->len - p_rcv_packet_head->r;
   //
   if(data_available==0){
      //buffer is empty remove and free rcv packet
      g_modem_core_connection_info_list[modem_core_connexion_index].rcv_packet_head=g_modem_core_connection_info_list[modem_core_connexion_index].rcv_packet_head->next;
      //
      _sys_free(p_rcv_packet_head->buf);
      _sys_free(p_rcv_packet_head);
   }
    
   //unlock 
   kernel_pthread_mutex_unlock(&g_modem_core_connection_info_list[modem_core_connexion_index].kernel_mutex);   
   
   //
   return size;
}

/*--------------------------------------------
| Name: modem_core_api_write
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int modem_core_api_write(int modem_core_connexion_index,char* buf, int len){
   uint8_t operation_request_code;
   desc_t  desc_w = INVALID_DESC;
    
   //
   if(modem_core_connexion_index<0 || modem_core_connexion_index>(MODEM_CONNECTION_MAX-1)){
      return -1;
   }
   //
   if(g_modem_core_connection_info_list[modem_core_connexion_index].status!=CONNECTION_STATUS_CONNECTED){
      return -1;
   }
   //
   //
   g_modem_core_connection_info_list[modem_core_connexion_index].snd_packet.buf=(uint8_t*)buf;
   g_modem_core_connection_info_list[modem_core_connexion_index].snd_packet.len=len;
   //
   operation_request_code = MODEM_CORE_OPERATION_SEND_REQUEST;
   //
   if(modem_core_mq_post_request(&g_modem_core_connection_info_list[modem_core_connexion_index],operation_request_code)<0){
      return -1;
   }
   //
   if(modem_core_mq_wait_response(&g_modem_core_connection_info_list[modem_core_connexion_index],&operation_request_code)<0){
      return -1;
   }
   //
   desc_w = g_modem_core_connection_info_list[modem_core_connexion_index].socket_desc;
   __fire_io(ofile_lst[desc_w].owner_pthread_ptr_write)
   //
   if(operation_request_code!=MODEM_CORE_OPERATION_DONE){
      return -1;
   }
   //
   return g_modem_core_connection_info_list[modem_core_connexion_index].snd_packet.len;
}

/*--------------------------------------------
| Name: modem_core_api_getpeername
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int modem_core_api_getpeername(int modem_core_connexion_index, struct sockaddr *name, socklen_t *namelen) {
   // not yet supported
   return -1;;
}

/*--------------------------------------------
| Name: modem_core_api_getsockname
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int modem_core_api_getsockname(int modem_core_connexion_index, struct sockaddr *name, socklen_t *namelen) {
   // not yet supported
   return -1;;
}

/*--------------------------------------------
| Name: modem_core_api_getsockopt
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int modem_core_api_getsockopt(int modem_core_connexion_index, int level, int optname, void *optval, socklen_t *optlen) {
   // not yet supported
   return -1;;
}

/*--------------------------------------------
| Name: modem_core_api_setsockopt
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int modem_core_api_setsockopt(int modem_core_connexion_index, int level, int optname, const void *optval, socklen_t optlen) {
  // not yet supported
  return -1;
}

/*--------------------------------------------
| Name: modem_core_api_ioctl
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int modem_core_api_ioctl(int modem_core_connexion_index, long cmd, void *argp) {
   // not yet supported
  return -1;
}

/*--------------------------------------------
| Name: modem_core_api_gethostbyname
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
struct hostent* modem_core_api_gethostbyname(struct hostent* host, const char *name) {
   // to do with: AT+CDNSGIP 
   uint8_t operation_request_code;

   struct hostent* p_hostent;
  
   //
   operation_request_code = MODEM_CORE_OPERATION_GETHOSTBYNAME_REQUEST;//MODEM_CORE_OPERATION_UNDEFINED_REQUEST;///MODEM_CORE_OPERATION_GETHOSTBYNAME_REQUEST;
   //
   if (modem_core_mq_post_unconnected_request(name, operation_request_code)<0) {
      return (struct hostent*)0;
   }
   //
   if (modem_core_mq_wait_unconnected_response(&p_hostent, &operation_request_code)<0) {
      return (struct hostent*)0;
   }
   //
   if (operation_request_code != MODEM_CORE_OPERATION_DONE) {
      return (struct hostent*)0;
   }
   //
   memcpy(host, p_hostent, sizeof(struct hostent));
 
   //
   return host;
}

