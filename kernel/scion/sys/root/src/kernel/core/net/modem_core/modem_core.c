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
#include "kernel/core/net/modem_core/modem_core.h"
#include "kernel/core/net/kernel_net_core_socket.h"

#include "kernel/core/net/bsd/netdb.h"

#include "lib/libc/termios/termios.h"
#include "lib/libc/misc/ltostr.h"

/*============================================
| Global Declaration
==============================================*/

//
static int dev_modem_core_load(void);
static int dev_modem_core_open(desc_t desc,int o_flag);
static int dev_modem_core_close(desc_t desc);
static int dev_modem_core_ioctl(desc_t desc,int request,va_list ap);

static const char dev_modem_core_name[]="net/modem";

dev_map_t dev_modem_core_map={
   dev_modem_core_name,
   S_IFCHR,
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
static modem_core_info_t g_modem_core_info={
   .modem_ttys_desc_r=INVALID_DESC,
   .modem_ttys_desc_w=INVALID_DESC
};

//
#define MODEM_CORE_KERNEL_THREAD_STACK_SIZE  4096 //1024//1024
#define MODEM_CORE_KERNEL_THREAD_PRIORITY    80 //10 //20//10 //140
// request: mqueue from user socket connection thread to kernel modem core thread (1 mqueue).
#define MODEM_CORE_KERNEL_MQUEUE_BUFFER_SIZE (32*(sizeof(kernel_mqueue_header_t)+sizeof(modem_core_message_t)))

//specific for network interface and ip stack
#if defined (__KERNEL_NET_IPSTACK) && defined(USE_MODEMIP)
static uint8_t modem_core_mqueue_request_buffer[MODEM_CORE_KERNEL_MQUEUE_BUFFER_SIZE];
static uint8_t modem_core_mqueue_response_buffer[MODEM_CORE_KERNEL_MQUEUE_BUFFER_SIZE];
static _macro_stack_addr char kernel_thread_modem_stack[MODEM_CORE_KERNEL_THREAD_STACK_SIZE];
#endif

// response: mqueue from modem core thread to user socket connection thread (1 mqueue for each connection).
#define MODEM_CONNECTION_KERNEL_MQUEUE_BUFFER_SIZE (4*(sizeof(kernel_mqueue_header_t)+sizeof(modem_core_message_t)))
//
static uint8_t modem_core_connection_mqueue_buffer_pool[MODEM_CONNECTION_MAX][MODEM_CONNECTION_KERNEL_MQUEUE_BUFFER_SIZE];
modem_core_connection_info_t g_modem_core_connection_info_list[MODEM_CONNECTION_MAX]={0};


//
uint8_t g_modem_core_buffer[MODEM_CORE_AT_COMMAND_PARSER_RCV_BUFFER_SIZE];
uint8_t g_at_send_buffer[MODEM_CORE_AT_COMMAND_PARSER_SND_BUFFER_SIZE];


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
static int modem_core_parser_init(modem_at_parser_context_t* modem_at_parser_context,desc_t desc_r, desc_t desc_w, char *buf, int size){
   //
   modem_at_parser_context->state = PARSER_STATE_WAIT_CR;
   //
   modem_at_parser_context->desc_r = desc_r;
   modem_at_parser_context->desc_w = desc_w;
 
   //
   if (buf) {
      modem_at_parser_context->response_buffer = buf;
   }else {
      modem_at_parser_context->response_buffer = _sys_malloc(size);
   }
   //
   if (!modem_at_parser_context->response_buffer) {
      return -1;
   }
   //
   modem_at_parser_context->response_buffer_size = size;
   //
   modem_at_parser_context->pbuf = buf;
   //
   modem_at_parser_context->at_response_callback_list_size=0;
   modem_at_parser_context->at_response_callback_list=(void*)0;
   //
   return 0;
}

/*--------------------------------------------
| Name:        modem_core_parser_reset
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int modem_core_parser_reset(modem_at_parser_context_t* modem_at_parser_context){
   modem_at_parser_context->state = PARSER_STATE_WAIT_CR;
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
int modem_core_parser_set_callback_list(struct modem_core_info_st* p_modem_core_info, modem_at_parser_response_callback_t* at_response_callback_list, int at_response_callback_list_size){
   modem_at_parser_context_t* modem_at_parser_context = &p_modem_core_info->modem_at_parser_context;

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
static char* modem_core_parser_get_last_at_response(struct modem_core_info_st* p_modem_core_info){
   modem_at_parser_context_t* modem_at_parser_context = &p_modem_core_info->modem_at_parser_context;
   //
   if(modem_at_parser_context->state!=PARSER_STATE_WAIT_CR){
      return (char*)0;
   }
   //
   return modem_at_parser_context->response_buffer;
}

/*--------------------------------------------
| Name:        modem_core_parser_recv_cme_error
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int modem_core_parser_recv_cme_error(struct modem_core_info_st* p_modem_core_info, char* at_response) {
   char* pbuf = at_response;
   int cme_error_code = 0;

   //get +CME ERROR:
   while (*pbuf != ':' && *pbuf != '\0') {
      pbuf++;
   }
   //
   if (*pbuf == '\0') {
      return -1;
   }


   //
   pbuf++;
   //get cme error code
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
   cme_error_code = atoi(at_response);
   //
   p_modem_core_info->last_cme_error_code = cme_error_code;
   //
   return 0;
}

/*--------------------------------------------
| Name:        modem_core_parser_recv_at_response_ex
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int modem_core_parser_recv_at_response_ex(struct modem_core_info_st* p_modem_core_info,const char* expected_response,int nonblock_mode,int silent_mode,int timeout){
   modem_at_parser_context_t* modem_at_parser_context = &p_modem_core_info->modem_at_parser_context;
   //
   char c;
   int cb;
   char flag_exit_jocker=0;
   int expected_reponse_len=0;
   //
   int oflag=ofile_lst[modem_at_parser_context->desc_r].oflag;
   pfn_parser_at_response_callback_t at_response_callback=(pfn_parser_at_response_callback_t)0;
   
   
   //switch from blocking mode to non bloking mode on read
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
   //arm read timeout
   if(timeout>0){
      __kernel_io_read_set_timeout(modem_at_parser_context->desc_r,timeout,0);
   }else{
      __kernel_io_read_unset_timeout(modem_at_parser_context->desc_r);
   }
   //
   while((cb=kernel_io_read(modem_at_parser_context->desc_r,&c,1))>0){
      //
      //rearm read timeout
      if(timeout>0){
         __kernel_io_read_set_timeout(modem_at_parser_context->desc_r,timeout,0);
      }else{
         __kernel_io_read_unset_timeout(modem_at_parser_context->desc_r);
      }
      //
      //kernel trace for debug
      if(silent_mode==0){
         kernel_io_write(__get_kernel_tty_desc(),&c,1);
      }
      // for ex:">"
      if(expected_reponse_len==1 && c==*expected_response){
         //restore blocking mode
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
         int r;
         //
         r=at_response_callback(p_modem_core_info, modem_at_parser_context->response_buffer);
         //callback at response == expected response?
         if (strstr(modem_at_parser_context->response_buffer, expected_response) != (char*)0) {
            return r;// exit 
         }
          
         //
         continue;
      }
            
      //exit on any response, return response length >0.
      if(flag_exit_jocker){
         //kernel trace for debug
         if(silent_mode==0){
            kernel_io_write(__get_kernel_tty_desc(),modem_at_parser_context->response_buffer,strlen(modem_at_parser_context->response_buffer));
         }
         //restore blocking mode
         ofile_lst[modem_at_parser_context->desc_r].oflag = oflag;
         //
         return strlen(modem_at_parser_context->response_buffer);
      }
      
      //if found expected_response  return 0
      if(expected_response!=(char*)0){
         if(strstr(modem_at_parser_context->response_buffer,expected_response)!=(char*)0){
            //is expected_response
            //kernel trace for debug
            if(silent_mode==0)
               kernel_printk("\r\n");
            //restore blocking mode
            ofile_lst[modem_at_parser_context->desc_r].oflag = oflag;
            //
            return 0;  
         }
      }

      //no expected_response but exit on OK and return 0 
      if (expected_response == (char*)0) {
         if (strstr(modem_at_parser_context->response_buffer, "OK") != (char*)0) {
            //kernel trace for debug
            if (silent_mode == 0)
               kernel_printk("\r\n");
            //restore blocking mode
            ofile_lst[modem_at_parser_context->desc_r].oflag = oflag;
            return 0;
         }
      }
      
      //is not expected_response but exit on error and return -1
      if (strstr(modem_at_parser_context->response_buffer, "+CME ERROR") != (char*)0) {
         //kernel trace for debug
         if (silent_mode == 0)
            kernel_printk("\r\n");
         //restore blocking mode
         ofile_lst[modem_at_parser_context->desc_r].oflag = oflag;
         //
         modem_core_parser_recv_cme_error(p_modem_core_info, modem_at_parser_context->response_buffer);
         //
         return -1;
      }

      //is not expected_response but exit on error and return -1
      if (strstr(modem_at_parser_context->response_buffer, "ERROR") != (char*)0) {
         //kernel trace for debug
         if (silent_mode == 0)
            kernel_printk("\r\n");
         //restore blocking mode
         ofile_lst[modem_at_parser_context->desc_r].oflag = oflag;
         return -1;
      }
      //
          
   }//while
   
   //restore blocking mode
   ofile_lst[modem_at_parser_context->desc_r].oflag = oflag;
   return -1;
}

/*--------------------------------------------
| Name:        modem_core_parser_recv_at_response
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int modem_core_parser_recv_at_response(struct modem_core_info_st* p_modem_core_info,const char* expected_response,int nonblock_mode,int silent_mode){
   return modem_core_parser_recv_at_response_ex(p_modem_core_info,expected_response,nonblock_mode,silent_mode,MODEM_CORE_DEV_READ_TIMEOUT);
}

/*--------------------------------------------
| Name:        modem_core_parser_send_recv_at_command_ex
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
extern char trap_lion_flag;
int modem_core_parser_send_recv_at_command_ex(struct modem_core_info_st* p_modem_core_info,const char* command,const char* expected_response,int silent_mode,int timeout){
   modem_at_parser_context_t* modem_at_parser_context = &p_modem_core_info->modem_at_parser_context;
   //ublox inter command delay
   __kernel_usleep(50000); //20ms
   //
   if(command!=(char*)0 && strlen(command)>0){
      trap_lion_flag+=50;
      kernel_io_write(modem_at_parser_context->desc_w,command,strlen(command));
       //ublox inter command delay
      __kernel_usleep(50000); //20ms
      //
      kernel_io_write(modem_at_parser_context->desc_w,"\r",1);
      //
      p_modem_core_info->last_cme_error_code = NO_CME_ERROR_CODE;
   }
   trap_lion_flag++;
   //
   return modem_core_parser_recv_at_response_ex(p_modem_core_info,expected_response,0,silent_mode,timeout);
}

/*--------------------------------------------
| Name:        modem_core_parser_send_recv_at_command
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int modem_core_parser_send_recv_at_command(struct modem_core_info_st* p_modem_core_info,const char* command,const char* expected_response,int silent_mode){
   return modem_core_parser_send_recv_at_command_ex(p_modem_core_info,command,expected_response,silent_mode,MODEM_CORE_DEV_READ_TIMEOUT);
}

/*--------------------------------------------
| Name:        modem_core_parser_send_at_command
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int modem_core_parser_send_at_command(struct modem_core_info_st* p_modem_core_info,const char* command,int silent_mode){
   modem_at_parser_context_t* modem_at_parser_context = &p_modem_core_info->modem_at_parser_context;
   //
   //ublox inter command delay
   __kernel_usleep(50000); //20ms
   //
   if(command!=(char*)0 && strlen(command)>0){
      kernel_io_write(modem_at_parser_context->desc_w,command,strlen(command));
       //ublox inter command delay
      __kernel_usleep(50000); //20ms
      //
      kernel_io_write(modem_at_parser_context->desc_w,"\r",1);
      //
      p_modem_core_info->last_cme_error_code = NO_CME_ERROR_CODE;
      //
      return strlen(command);
   }
   //
   return -1;
}

/*--------------------------------------------
| Name:        modem_core_parser_get_last_cme_error
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int modem_core_parser_get_last_cme_error(struct modem_core_info_st* p_modem_core_info) {
   return p_modem_core_info->last_cme_error_code;
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
   struct timespec read_modem_timeout={
      .tv_sec=1,
      .tv_nsec=0
   };
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
   p_modem_core_info->status=MODEM_CORE_STATUS_STARTED;
   //
   __atomic_out();
   
   
   //
   kernel_printk("\r\nmodem_core_routine...\r\n");
   //
   while(1){
      //wait on sem_io of this thread. Just one thread for connection command and receive data from modem / ttys 
      if(kernel_mqueue_isempty(&g_modem_core_info.kernel_mqueue_request) 
          && ofile_lst[g_modem_core_info.modem_ttys_desc_r].pfsop->fdev.fdev_isset_read(g_modem_core_info.modem_ttys_desc_r)!=0){
         __wait_io_int2(pthread_ptr,&read_modem_timeout); //wait incomming data
      }
      
      //check data available in kernel_mqueue and in uart driver
      
      // 1: data available in uart  with isset_read (unsolicited message from modem)
      if(!ofile_lst[g_modem_core_info.modem_ttys_desc_r].pfsop->fdev.fdev_isset_read(g_modem_core_info.modem_ttys_desc_r)){   
         
         //
         if(modem_core_parser_recv_at_response(&g_modem_core_info,(char*)0,1,__KERNEL_MODEM_CORE_SILENT_MODE)>0){
            char* at_response;
            at_response = modem_core_parser_get_last_at_response(&g_modem_core_info);
            if(at_response!=(char*)0){
            }
         }
      }
      
      //2: if data available in mqueue
      if(!kernel_mqueue_isempty(&g_modem_core_info.kernel_mqueue_request)){
         modem_core_connection_info_t* p_modem_core_connection_info;
         // decode and send command to modem.
         // wait data from modem
         // send result to socket mqueue
         if(kernel_mqueue_get(&g_modem_core_info.kernel_mqueue_request,&p_modem_core_info->modem_core_message,sizeof(modem_core_message_t))<0){
            continue;
         }
         
         //
         switch(p_modem_core_info->modem_core_message.operation_request_code){
            //
            case MODEM_CORE_OPERATION_SOCKET_CREATE_REQUEST:
               //
               if((p_modem_core_connection_info = (modem_core_connection_info_t*)p_modem_core_info->modem_core_message.p_modem_core_connection_info)==(void*)0){
                  break;
               }
               //
               if (g_modem_core_info.modem_at_command_op.at_command_socket_create == __pfn_at_command_not_implemented) {
                  p_modem_core_info->modem_core_message.operation_response_code = MODEM_CORE_OPERATION_NOT_IMPLEMENTED;
                  //
                  kernel_mqueue_put(&p_modem_core_connection_info->kernel_mqueue, &p_modem_core_info->modem_core_message, sizeof(p_modem_core_info->modem_core_message));
               }else if (g_modem_core_info.modem_at_command_op.at_command_socket_create(&g_modem_core_info, p_modem_core_connection_info)<0) {
                  p_modem_core_info->modem_core_message.operation_response_code = MODEM_CORE_OPERATION_FAILED;
                  //
                  kernel_mqueue_put(&p_modem_core_connection_info->kernel_mqueue, &p_modem_core_info->modem_core_message, sizeof(p_modem_core_info->modem_core_message));
               }
            break;

            //
            case MODEM_CORE_OPERATION_SOCKET_CONNECT_REQUEST:
               //
                if((p_modem_core_connection_info = (modem_core_connection_info_t*)p_modem_core_info->modem_core_message.p_modem_core_connection_info)==(void*)0){
                  break;
               }
               //
               if (g_modem_core_info.modem_at_command_op.at_command_socket_connect == __pfn_at_command_not_implemented) {
                  p_modem_core_info->modem_core_message.operation_response_code = MODEM_CORE_OPERATION_NOT_IMPLEMENTED;
                  //
                  kernel_mqueue_put(&p_modem_core_connection_info->kernel_mqueue, &p_modem_core_info->modem_core_message, sizeof(p_modem_core_info->modem_core_message));
               }else if(g_modem_core_info.modem_at_command_op.at_command_socket_connect(&g_modem_core_info,p_modem_core_connection_info)<0){
                  p_modem_core_info->modem_core_message.operation_response_code =  MODEM_CORE_OPERATION_FAILED;
                  //
                  kernel_mqueue_put(&p_modem_core_connection_info->kernel_mqueue,&p_modem_core_info->modem_core_message,sizeof(p_modem_core_info->modem_core_message));
               }
            break;
            
            //
            case MODEM_CORE_OPERATION_SOCKET_CLOSE_REQUEST:
               //
                if((p_modem_core_connection_info = (modem_core_connection_info_t*)p_modem_core_info->modem_core_message.p_modem_core_connection_info)==(void*)0){
                  break;
               }
               //
               if (g_modem_core_info.modem_at_command_op.at_command_socket_close == __pfn_at_command_not_implemented) {
                  p_modem_core_info->modem_core_message.operation_response_code = MODEM_CORE_OPERATION_NOT_IMPLEMENTED;
               }else {
                  if (g_modem_core_info.modem_at_command_op.at_command_socket_close(&g_modem_core_info, p_modem_core_connection_info) < 0) {
                     p_modem_core_info->modem_core_message.operation_response_code = MODEM_CORE_OPERATION_FAILED;
                  }
                  else {
                     p_modem_core_info->modem_core_message.operation_response_code = MODEM_CORE_OPERATION_DONE;
                  }
               }
               //
               kernel_mqueue_put(&p_modem_core_connection_info->kernel_mqueue,&p_modem_core_info->modem_core_message,sizeof(p_modem_core_info->modem_core_message));
            break;
            
            //
            case MODEM_CORE_OPERATION_SOCKET_SEND_REQUEST:
               //
               if((p_modem_core_connection_info = (modem_core_connection_info_t*)p_modem_core_info->modem_core_message.p_modem_core_connection_info)==(void*)0){
                  break;
               }
               //
               if (g_modem_core_info.modem_at_command_op.at_command_socket_send == __pfn_at_command_not_implemented) {
                  p_modem_core_info->modem_core_message.operation_response_code = MODEM_CORE_OPERATION_NOT_IMPLEMENTED;
                  //
                  kernel_mqueue_put(&p_modem_core_connection_info->kernel_mqueue, &p_modem_core_info->modem_core_message, sizeof(p_modem_core_info->modem_core_message));
               }else if (g_modem_core_info.modem_at_command_op.at_command_socket_send(&g_modem_core_info, p_modem_core_connection_info)<0){
                  p_modem_core_info->modem_core_message.operation_response_code = MODEM_CORE_OPERATION_FAILED;
                  //
                  kernel_mqueue_put(&p_modem_core_connection_info->kernel_mqueue,&p_modem_core_info->modem_core_message,sizeof(p_modem_core_info->modem_core_message));
               }
            break;

            //
            case MODEM_CORE_OPERATION_GETHOSTBYNAME_REQUEST:
               //for gethostbyname p_modem_core_connection_info is a pointer on server name
               if((p_modem_core_connection_info = (modem_core_connection_info_t*)p_modem_core_info->modem_core_message.p_modem_core_connection_info)==(void*)0){
                  break;
               }
               if (g_modem_core_info.modem_at_command_op.at_command_gethostbyname == __pfn_at_command_not_implemented) {
                  //
                  modem_core_mq_post_unconnected_response((void*)0, MODEM_CORE_OPERATION_GETHOSTBYNAME_REQUEST, MODEM_CORE_OPERATION_NOT_IMPLEMENTED);
               }else if (g_modem_core_info.modem_at_command_op.at_command_gethostbyname(&g_modem_core_info, p_modem_core_connection_info)<0) {
                  //
                  modem_core_mq_post_unconnected_response((void*)0, MODEM_CORE_OPERATION_GETHOSTBYNAME_REQUEST, MODEM_CORE_OPERATION_FAILED);
               }
            break;
               
            //
            case MODEM_CORE_OPERATION_IFUP_REQUEST:
               //modem power up
               //modem reset
               if(g_modem_core_info.modem_at_command_op.at_command_modem_reset(&g_modem_core_info, (void*)0)<0){
                  modem_core_mq_post_unconnected_response((void*)0, MODEM_CORE_OPERATION_IFUP_REQUEST , MODEM_CORE_OPERATION_FAILED);
                  break;
               }
               //modem init gsm/gprs context 
               if(g_modem_core_info.modem_at_command_op.at_command_modem_init(&g_modem_core_info, (void*)0)<0){
                  modem_core_mq_post_unconnected_response((void*)0, MODEM_CORE_OPERATION_IFUP_REQUEST, MODEM_CORE_OPERATION_FAILED);
                  break;
               }
               //
               g_modem_core_info.if_status=IFF_UP;
               //
               modem_core_mq_post_unconnected_response((void*)0, MODEM_CORE_OPERATION_IFUP_REQUEST, MODEM_CORE_OPERATION_DONE);
            break;
               
            //
            case MODEM_CORE_OPERATION_IFDOWN_REQUEST:
               //shutdown all connection
               for(int modem_core_connexion_index=0;
                  modem_core_connexion_index<MODEM_CONNECTION_MAX;
                  modem_core_connexion_index++){
                  //
                  if(g_modem_core_connection_info_list[modem_core_connexion_index].status==CONNECTION_STATUS_CLOSE)
                     continue;
                  //
                  desc_t desc_r = g_modem_core_connection_info_list[modem_core_connexion_index].socket_desc;
                  if(desc_r!=INVALID_DESC && ofile_lst[desc_r].owner_pthread_ptr_read)
                     __fire_io(ofile_lst[desc_r].owner_pthread_ptr_read);
                  //
                  kernel_mqueue_flush(&g_modem_core_connection_info_list[modem_core_connexion_index].kernel_mqueue);
               }
               //
               g_modem_core_info.if_status=IFF_DOWN;
               
               //modem stop gsm/gprs context 
               if(g_modem_core_info.modem_at_command_op.at_command_modem_stop!=__pfn_at_command_not_implemented){
                  g_modem_core_info.modem_at_command_op.at_command_modem_stop(&g_modem_core_info, (void*)0);
               }
              
               //
               modem_core_mq_post_unconnected_response((void*)0, MODEM_CORE_OPERATION_IFDOWN_REQUEST, MODEM_CORE_OPERATION_DONE);
            break;
               
            case MODEM_CORE_OPERATION_UNDEFINED_REQUEST:
               //simcom_at_response_callback_gethostbyname("+CDNSGIP: 1,\"collector.o10ee.com\",\"217.182.129.15\"");
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
   //
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
   if(g_modem_core_info.status!=MODEM_CORE_STATUS_STARTED){
      return -1;
   }
   //
   modem_core_message.operation_request_code = operation_request_code;
   modem_core_message.operation_response_code = MODEM_CORE_OPERATION_INPROCESS;
   modem_core_message.p_modem_core_connection_info = data;
   //
   if (kernel_mqueue_put(&g_modem_core_info.kernel_mqueue_request, &modem_core_message, sizeof(modem_core_message_t))<0) {
      return -1;
   }
   //force unblock read
   kernel_pthread_t* p_kernel_pthread = &g_modem_core_info.kernel_thread;
   __fire_io(p_kernel_pthread);
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
int modem_core_mq_post_unconnected_response(void* data, uint8_t operation_request_code,uint8_t operation_response_code) {
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
static int modem_core_mq_wait_unconnected_response(void** data, uint8_t* operation_response_code,const struct timespec* request_timeout) {
   modem_core_message_t modem_core_message;
   //
   if(g_modem_core_info.status!=MODEM_CORE_STATUS_STARTED){
      return -1;
   }
   //
   if (kernel_mqueue_get_timedwait(&g_modem_core_info.
                                   kernel_mqueue_response, 
                                   &modem_core_message, 
                                   sizeof(modem_core_message_t),
                                   request_timeout)<=0) { //==0 timeout
      return -1;
   }
   //
   if(data!=(void*)0){
      *data = modem_core_message.p_modem_core_connection_info;
   }
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
   if(g_modem_core_info.status!=MODEM_CORE_STATUS_STARTED){
      return -1;
   }
   //
   modem_core_message.operation_request_code = operation_request_code;
   modem_core_message.operation_response_code = MODEM_CORE_OPERATION_INPROCESS;
   modem_core_message.p_modem_core_connection_info=p_modem_core_connection_info;
   //
   if(kernel_mqueue_put(&g_modem_core_info.kernel_mqueue_request,
                        &modem_core_message,
                        sizeof(modem_core_message_t))<0){
      return -1;
   }
   //force unblock read
   kernel_pthread_t* p_kernel_pthread = &g_modem_core_info.kernel_thread;
   __fire_io(p_kernel_pthread);
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
   if(g_modem_core_info.status!=MODEM_CORE_STATUS_STARTED){
      return -1;
   }
   //
   if(kernel_mqueue_get_timedwait(&p_modem_core_connection_info->kernel_mqueue,
                                  &modem_core_message,
                                  sizeof(modem_core_message_t),
                                  &p_modem_core_connection_info->request_timeout)<=0){ //==0 timeout
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
static int modem_core_link(desc_t desc_r,desc_t desc_w, dev_modem_info_t* p_dev_modem_info){
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
 
   //init g_modem_core_info structure with dev_modem_info.
   modem_core_parser_init(&g_modem_core_info.modem_at_parser_context,desc_r,desc_w,g_modem_core_buffer, MODEM_CORE_AT_COMMAND_PARSER_RCV_BUFFER_SIZE);
   //
   modem_core_parser_set_callback_list(&g_modem_core_info,
      p_dev_modem_info->at_response_callback_list,
      p_dev_modem_info->at_response_callback_list_size);
   //
   memcpy(&g_modem_core_info.modem_at_command_op, &p_dev_modem_info->modem_at_command_op, sizeof(modem_at_command_op_t));
                          
   //
#if defined (__KERNEL_NET_IPSTACK) && defined(USE_MODEMIP)
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
   
   //request queue protect mutex
   pthread_mutexattr_t mutex_attr=0;
   kernel_pthread_mutex_init(&g_modem_core_info.kernel_mqueue_mutex,&mutex_attr);
   
   //request queue
   kernel_mqueue_attr.buffer = modem_core_mqueue_request_buffer;
   kernel_mqueue_attr.size = sizeof(modem_core_mqueue_request_buffer);
   kernel_mqueue_init(&g_modem_core_info.kernel_mqueue_request, &kernel_mqueue_attr);

   //response queue
   kernel_mqueue_attr.buffer = modem_core_mqueue_response_buffer;
   kernel_mqueue_attr.size = sizeof(modem_core_mqueue_response_buffer);
   kernel_mqueue_init(&g_modem_core_info.kernel_mqueue_response, &kernel_mqueue_attr);

   //kernel thread
   thread_attr.stacksize = MODEM_CORE_KERNEL_THREAD_STACK_SIZE;
   thread_attr.stackaddr = (void*)&kernel_thread_modem_stack;
   thread_attr.priority  = MODEM_CORE_KERNEL_THREAD_PRIORITY;
   thread_attr.timeslice = 1;
   thread_attr.name = "kernel_pthread_modem_core_serial";
   //
   kernel_pthread_create(&g_modem_core_info.kernel_thread,&thread_attr,modem_core_routine,&g_modem_core_info);
   //grouik code: yield to wait pthread modem core start
   __kernel_usleep(100000); //100ms
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
static int modem_core_unlink(void){
   return 0;
}

/*--------------------------------------------
| Name: modem_core_ifup
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int modem_core_ifup(void){
   uint8_t operation_request_code;
   struct timespec* p_request_timeout=(struct timespec*)0;
   
   //
   if(g_modem_core_info.status!=MODEM_CORE_STATUS_STARTED){
      return -1;
   }
   //already up
   if(g_modem_core_info.if_status==IFF_UP){
      return -1;
   }
   
   //to do check if all socket connection are closed
   
   //
   kernel_pthread_mutex_lock(&g_modem_core_info.kernel_mqueue_mutex);
   //
   operation_request_code = MODEM_CORE_OPERATION_IFUP_REQUEST;
   //
   if (modem_core_mq_post_unconnected_request((void*)0, operation_request_code)<0) {
      //
      kernel_pthread_mutex_unlock(&g_modem_core_info.kernel_mqueue_mutex); 
      //
      return -1;
   }
   //
   if (modem_core_mq_wait_unconnected_response((void*)0, &operation_request_code,p_request_timeout)<0) {
      //
      kernel_pthread_mutex_unlock(&g_modem_core_info.kernel_mqueue_mutex); 
      //
      return -1;
   }
   //
   kernel_pthread_mutex_unlock(&g_modem_core_info.kernel_mqueue_mutex); 
   //
   if (operation_request_code != MODEM_CORE_OPERATION_DONE) {
      return -1;
   }
   return 0;
}

/*--------------------------------------------
| Name: modem_core_ifdown
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int modem_core_ifdown(void){
   uint8_t operation_request_code;
   struct timespec* p_request_timeout=(struct timespec*)0;
   
   //
   if(g_modem_core_info.status!=MODEM_CORE_STATUS_STARTED){
      return -1;
   }
   //already down
   if(g_modem_core_info.if_status==IFF_DOWN){
      return -1;
   }
   //
   operation_request_code = MODEM_CORE_OPERATION_IFDOWN_REQUEST;
   //
   kernel_pthread_mutex_lock(&g_modem_core_info.kernel_mqueue_mutex);  
   //
   if (modem_core_mq_post_unconnected_request((void*)0, operation_request_code)<0) {
      //
      kernel_pthread_mutex_unlock(&g_modem_core_info.kernel_mqueue_mutex); 
      //
      return -1;
   }
   //
   if (modem_core_mq_wait_unconnected_response((void*)0, &operation_request_code,p_request_timeout)<0) {
      //
      kernel_pthread_mutex_unlock(&g_modem_core_info.kernel_mqueue_mutex); 
      //
      return -1;
   }
   //
   kernel_pthread_mutex_unlock(&g_modem_core_info.kernel_mqueue_mutex); 
   //
   if (operation_request_code != MODEM_CORE_OPERATION_DONE) {
      return -1;
   }
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
   
   g_modem_core_info.status=MODEM_CORE_STATUS_STOPPED;
   g_modem_core_info.if_status=IFF_DOWN;
   g_modem_core_info.modem_ttys_desc_r=INVALID_DESC;
   g_modem_core_info.modem_ttys_desc_w=INVALID_DESC;
   
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
            desc_t desc_link_r = ofile_lst[desc].desc_nxt[0];
            if (desc_link_r < 0) {
               return -1;
            }
            //
            if(modem_core_link(ofile_lst[desc].desc_nxt[0],ofile_lst[desc].desc_nxt[1], ofile_lst[desc_link_r].p)<0){
               return -1;
            }
            //
            //if(modem_core_ifup()<0){
            //   return -1;
            //}
            //
            ofile_lst[desc].p=&g_modem_core_info;
         }

      }
      break;
      //
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
      //
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
      //
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
      case IFUP:{
         int* p_status= va_arg( ap, int*);
         //
         if(modem_core_ifup()<0){
            *p_status=g_modem_core_info.if_status;
            return -1;
         }
         //
         *p_status=g_modem_core_info.if_status;
      }
      break;
      
      //
      case IFDOWN:{
         int* p_status= va_arg( ap, int*);
         //
         if(modem_core_ifdown()<0){
            *p_status=g_modem_core_info.if_status;
            return -1;
         }
         //
         *p_status=g_modem_core_info.if_status;
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
   
   //
   if(g_modem_core_info.status!=MODEM_CORE_STATUS_STARTED){
      return -1;
   }
   //
   if(g_modem_core_info.if_status!=IFF_UP){
      return -1;
   }
   //
   for(modem_core_connexion_index=0;
      modem_core_connexion_index<MODEM_CONNECTION_MAX;
      modem_core_connexion_index++){
      //
      if(g_modem_core_connection_info_list[modem_core_connexion_index].status==CONNECTION_STATUS_CLOSE){
         uint8_t operation_request_code;
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
         g_modem_core_connection_info_list[modem_core_connexion_index].request_timeout.tv_sec=MODEM_CORE_REQUEST_TIMEOUT;
         g_modem_core_connection_info_list[modem_core_connexion_index].request_timeout.tv_nsec=0;
         

         //
         if (g_modem_core_info.modem_at_command_op.at_command_socket_create) {
            operation_request_code = MODEM_CORE_OPERATION_SOCKET_CREATE_REQUEST;
            //
            if (modem_core_mq_post_request(&g_modem_core_connection_info_list[modem_core_connexion_index], operation_request_code) < 0) {
               return -1;
            }
            //
            if (modem_core_mq_wait_response(&g_modem_core_connection_info_list[modem_core_connexion_index], &operation_request_code) < 0) {
               return -1;
            }
            //
            if (operation_request_code != MODEM_CORE_OPERATION_DONE) {
               return -1;
            }
         }

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
   
   //
   if(g_modem_core_info.status!=MODEM_CORE_STATUS_STARTED){
      return -1;
   }
   //
   if(g_modem_core_info.if_status!=IFF_UP){
      return -1;
   }
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
   if(g_modem_core_info.status!=MODEM_CORE_STATUS_STARTED){
      return -1;
   }
   //
   if(g_modem_core_info.if_status!=IFF_UP){
      return -1;
   }
   //
   if(modem_core_connexion_index<0 || modem_core_connexion_index>(MODEM_CONNECTION_MAX-1)){
      return -1;
   }
   //open?
   if(g_modem_core_connection_info_list[modem_core_connexion_index].status==CONNECTION_STATUS_CLOSE){
      return -1;
   }
   //alreday connected?
   if (g_modem_core_connection_info_list[modem_core_connexion_index].status == CONNECTION_STATUS_CONNECTED) {
      return -1;
   }
   //
   if(namelen!=sizeof(struct sockaddr_in)){
      return -1;
   }
   
   //
   memcpy(&g_modem_core_connection_info_list[modem_core_connexion_index].sockaddr_in_connect,name,namelen); 
   
   //
   operation_request_code = MODEM_CORE_OPERATION_SOCKET_CONNECT_REQUEST;
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
   if(g_modem_core_info.status!=MODEM_CORE_STATUS_STARTED){
      return -1;
   }
   //
   if(modem_core_connexion_index<0 || modem_core_connexion_index>(MODEM_CONNECTION_MAX-1)){
      return -1;
   }
   //
   if(g_modem_core_connection_info_list[modem_core_connexion_index].status==CONNECTION_STATUS_CLOSE){
      return -1;
   }
   //
   operation_request_code = MODEM_CORE_OPERATION_SOCKET_CLOSE_REQUEST;
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
   if(g_modem_core_info.status!=MODEM_CORE_STATUS_STARTED){
      return -1;
   }
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
   if(g_modem_core_info.status!=MODEM_CORE_STATUS_STARTED){
      return 0;//return 0 to not stay blocked in __wait_io
   }
   //
   if(g_modem_core_info.if_status!=IFF_UP){
      return 0;//return 0 to not stay blocked in __wait_io
   }
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
   if(g_modem_core_info.status!=MODEM_CORE_STATUS_STARTED){
      return -1;
   }
   //
   if(g_modem_core_info.if_status!=IFF_UP){
      return -1;
   }
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
   memcpy(buf,p_rcv_packet_head->buf+ p_rcv_packet_head->r,size);
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
   if(g_modem_core_info.status!=MODEM_CORE_STATUS_STARTED){
      return -1;
   }
   //
   if(g_modem_core_info.if_status!=IFF_UP){
      return -1;
   }
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
   operation_request_code = MODEM_CORE_OPERATION_SOCKET_SEND_REQUEST;
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
   //
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
   return -1;
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
   return -1;
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
   //
   if(g_modem_core_info.status!=MODEM_CORE_STATUS_STARTED){
      return -1;
   }
   //
   if(modem_core_connexion_index<0 || modem_core_connexion_index>(MODEM_CONNECTION_MAX-1)){
      return -1;
   }
   //
   if(g_modem_core_connection_info_list[modem_core_connexion_index].status!=CONNECTION_STATUS_CONNECTED){
      return -1;
   }
   
   //
   if(level!=SOL_SOCKET){
      return -1;
   } 
   
   //SO_RCVTIMEO or SO_SNDTIMEO 
   if(optname==SO_RCVTIMEO && optval){
      struct timespec* p_rcv_timeout=(struct timespec*)optval;
      *optlen==sizeof(struct timespec);
      //
      __kernel_io_read_get_timeout(g_modem_core_connection_info_list[modem_core_connexion_index].socket_desc,p_rcv_timeout->tv_sec,p_rcv_timeout->tv_nsec);
      return 0;
   }
   
   //
   return -1;
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
   //
   if(g_modem_core_info.status!=MODEM_CORE_STATUS_STARTED){
      return -1;
   }
   //
   if(modem_core_connexion_index<0 || modem_core_connexion_index>(MODEM_CONNECTION_MAX-1)){
      return -1;
   }
   //
   if(g_modem_core_connection_info_list[modem_core_connexion_index].status!=CONNECTION_STATUS_CONNECTED){
      return -1;
   }
   
   //
   if(level!=SOL_SOCKET){
      return -1;
   } 
   
   //SO_RCVTIMEO or SO_SNDTIMEO 
   if(optname==SO_RCVTIMEO && optval && optlen==sizeof(struct timespec)){
      struct timespec* p_rcv_timeout=(struct timespec*)optval;
      __kernel_io_read_set_timeout(g_modem_core_connection_info_list[modem_core_connexion_index].socket_desc,p_rcv_timeout->tv_sec,p_rcv_timeout->tv_nsec);
      return 0;
   }
   
   //
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
   //
   struct hostent* p_hostent;
   struct timespec request_timeout;
   struct timespec* p_request_timeout=(struct timespec*)0;
   
   //
   if(g_modem_core_info.status!=MODEM_CORE_STATUS_STARTED){
      return (struct hostent*)0;
   }
   //
   if(g_modem_core_info.if_status!=IFF_UP){
      return (struct hostent*)0;
   }
   
#ifdef MODEM_CORE_OPERATION_GETHOSTBYNAME_REQUEST_TIMEOUT
   //
   request_timeout.tv_sec = MODEM_CORE_OPERATION_GETHOSTBYNAME_REQUEST_TIMEOUT;
   request_timeout.tv_nsec = 0;
   p_request_timeout = &request_timeout;
   
#endif
   //
   kernel_pthread_mutex_lock(&g_modem_core_info.kernel_mqueue_mutex);
   //
   operation_request_code = MODEM_CORE_OPERATION_GETHOSTBYNAME_REQUEST;
   //
   if (modem_core_mq_post_unconnected_request((char*)name, operation_request_code)<0) {
      //
      kernel_pthread_mutex_unlock(&g_modem_core_info.kernel_mqueue_mutex);
      //
      return (struct hostent*)0;
   }
   //
   if (modem_core_mq_wait_unconnected_response(&p_hostent, &operation_request_code,p_request_timeout)<0) {
      //
      kernel_pthread_mutex_unlock(&g_modem_core_info.kernel_mqueue_mutex);
      //
      return (struct hostent*)0;
   }
   //
   kernel_pthread_mutex_unlock(&g_modem_core_info.kernel_mqueue_mutex);
   //
   if (operation_request_code != MODEM_CORE_OPERATION_DONE) {
      return (struct hostent*)0;
   }
   //
   memcpy(host, p_hostent, sizeof(struct hostent));
 
   //
   return host;
}

