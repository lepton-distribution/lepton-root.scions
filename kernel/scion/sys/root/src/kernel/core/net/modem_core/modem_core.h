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
| Compiler Directive
==============================================*/
#ifndef __MODEM_CORE_H__
#define __MODEM_CORE_H__


/*============================================
| Includes
==============================================*/

/*============================================
| Declaration
==============================================*/
//
#ifndef MODEM_CONNECTION_MAX
   #define MODEM_CONNECTION_MAX 1
#endif

//
#define MODEM_CORE_OPERATION_UNDEFINED_REQUEST           ((uint8_t)(0))
#define MODEM_CORE_OPERATION_SOCKET_CREATE_REQUEST       ((uint8_t)(1))
#define MODEM_CORE_OPERATION_SOCKET_CONNECT_REQUEST      ((uint8_t)(2))
#define MODEM_CORE_OPERATION_SOCKET_SEND_REQUEST         ((uint8_t)(3))
#define MODEM_CORE_OPERATION_SOCKET_CLOSE_REQUEST        ((uint8_t)(4))

#define MODEM_CORE_OPERATION_EVENT_ACCEPT             ((uint8_t)(5))
#define MODEM_CORE_OPERATION_EVENT_PACKET             ((uint8_t)(6))

#define MODEM_CORE_OPERATION_GETHOSTBYNAME_REQUEST    ((uint8_t)(7))

#define MODEM_CORE_OPERATION_INPROCESS                ((uint8_t)(0x0F))
#define MODEM_CORE_OPERATION_DONE                     ((uint8_t)(0x1F))
#define MODEM_CORE_OPERATION_NOT_IMPLEMENTED            ((uint8_t)(0xFE))
#define MODEM_CORE_OPERATION_FAILED                   ((uint8_t)(0xFF))

//
#define CONNECTION_STATUS_CLOSE        ((int)(0))
#define CONNECTION_STATUS_OPEN         ((int)(1))
#define CONNECTION_STATUS_CONNECTED    ((int)(2))
#define CONNECTION_STATUS_SHUTDOWN     ((int)(3))

#define NO_CME_ERROR_CODE (-1)


//
typedef struct modem_core_message_st {
   uint8_t operation_request_code; //connect, send, recv, close.
   uint8_t operation_response_code;
   void* p_modem_core_connection_info;
}modem_core_message_t;

//
typedef struct modem_core_ip_packet_st {
   uint8_t* p_packet;
   struct modem_core_ip_packet_st* p_packet_next;
}modem_core_ip_packet_t;

//
typedef struct modem_core_recv_packet_parameters_st {
   uint16_t   len;
   uint16_t   r; //amount of data has been read by user level
   uint8_t* buf;
   struct modem_core_recv_packet_parameters_st* next;
}modem_core_recv_packet_parameters_t;

//
typedef struct modem_core_send_packet_parameters_st {
   uint16_t   len;
   uint16_t   w; //amount of data has been sent by modem
   uint8_t* buf;
}modem_core_send_packet_parameters_t;

//modem socket connection
typedef struct modem_core_connection_info_st {
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

extern modem_core_connection_info_t g_modem_core_connection_info_list[MODEM_CONNECTION_MAX];


//for socket asynchronous at response message from modem
typedef int(*pfn_parser_at_response_callback_t)(void* pv_modem_core_info,char* at_response);
typedef struct modem_at_parser_response_callback_st {
   const char* at_response;
   pfn_parser_at_response_callback_t at_response_callback;
}modem_at_parser_response_callback_t;


//for socket at command message to modem
typedef int(*pfn_at_command_connection_t)(void* pv_modem_core_info, struct modem_core_connection_info_st* p_modem_core_connection_info);
typedef int(*pfn_at_command_noconnection_t)(void* pv_modem_core_info, void* data);

#define __pfn_at_command_not_implemented  (void*)0

typedef  pfn_at_command_noconnection_t pfn_at_command_modem_reset_t;
typedef  pfn_at_command_noconnection_t pfn_at_command_modem_init_t;
typedef  pfn_at_command_connection_t   pfn_at_command_socket_create_t;
typedef  pfn_at_command_connection_t   pfn_at_command_socket_connect_t;
typedef  pfn_at_command_connection_t   pfn_at_command_socket_close_t;
typedef  pfn_at_command_connection_t   pfn_at_command_socket_send_t;
typedef  pfn_at_command_connection_t   pfn_at_command_socket_receive_t;
typedef  pfn_at_command_noconnection_t pfn_at_command_gethostbyname_t;



//
#define PARSER_STATE_WAIT_CR   1  // '\r'
#define PARSER_STATE_WAIT_LF   2  // '\n'
#define PARSER_STATE_PARSE     3  //

//
typedef struct modem_at_parser_context_st {
   uint8_t state;
   //
   int   response_buffer_size;
   char* response_buffer;
   char* pbuf;

   // asychronous message from modem
   int at_response_callback_list_size;
   modem_at_parser_response_callback_t * at_response_callback_list;

   desc_t desc_r;
   desc_t desc_w;
}modem_at_parser_context_t;

//
typedef struct modem_at_command_op_st {
   pfn_at_command_modem_reset_t     at_command_modem_reset;
   pfn_at_command_modem_init_t      at_command_modem_init;
   pfn_at_command_socket_create_t   at_command_socket_create;
   pfn_at_command_socket_connect_t  at_command_socket_connect;
   pfn_at_command_socket_close_t    at_command_socket_close;
   pfn_at_command_socket_send_t     at_command_socket_send;
   pfn_at_command_socket_receive_t  at_command_socket_receive;
   pfn_at_command_gethostbyname_t   at_command_gethostbyname;
}modem_at_command_op_t;

//
typedef struct modem_at_command_st {
   const char* command;
   const char* response;
}modem_at_command_t;

//
typedef struct modem_core_info_st {
   desc_t  modem_ttys_desc_r;
   desc_t  modem_ttys_desc_w;
   //
   modem_at_command_op_t      modem_at_command_op;
   modem_at_parser_context_t  modem_at_parser_context;

   //
   modem_core_message_t modem_core_message;

   //
   kernel_mqueue_t kernel_mqueue_request;
   kernel_mqueue_t kernel_mqueue_response;
   //
   kernel_pthread_t kernel_thread;
   //
   int last_cme_error_code;

}modem_core_info_t;

//
typedef struct dev_modem_info_st{
   desc_t  desc_r;
   desc_t  desc_w;
   //
   int at_response_callback_list_size;
   modem_at_parser_response_callback_t * at_response_callback_list;
   //
   modem_at_command_op_t modem_at_command_op;

}dev_modem_info_t;


//
#define MODEM_CORE_AT_COMMAND_PARSER_RCV_BUFFER_SIZE  (512+128)
#define MODEM_CORE_AT_COMMAND_PARSER_SND_BUFFER_SIZE  (256)
//
extern uint8_t g_modem_core_buffer[MODEM_CORE_AT_COMMAND_PARSER_RCV_BUFFER_SIZE];
extern uint8_t g_at_send_buffer[MODEM_CORE_AT_COMMAND_PARSER_SND_BUFFER_SIZE];

//
int modem_core_mq_post_unconnected_request(void* data, uint8_t operation_request_code);
int modem_core_mq_post_unconnected_response(void* data, uint8_t operation_request_code, uint8_t operation_response_code);
int modem_core_mq_wait_unconnected_response(void** data, uint8_t* operation_response_code);


int modem_core_parser_set_callback_list(struct modem_core_info_st* p_modem_core_info, modem_at_parser_response_callback_t* at_response_callback_list, int at_response_callback_list_size);
int modem_core_parser_send_at_command(struct modem_core_info_st* p_modem_core_info, const char* command, int silent_mode);
int modem_core_parser_recv_at_response(struct modem_core_info_st* p_modem_core_info, const char* expected_response, int nonblock_mode, int silent_mode);
int modem_core_parser_send_recv_at_command(struct modem_core_info_st* p_modem_core_info, const char* command, const char* expected_response, int silent_mode);

int modem_core_parser_get_last_cme_error(struct modem_core_info_st* p_modem_core_info);

#endif