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


/*============================================
| Compiler Directive
==============================================*/
#ifndef __BSD_SYS_SOCKET_H__
#define __BSD_SYS_SOCKET_H__


/*============================================
| Includes
==============================================*/

/*============================================
| Declaration
==============================================*/

// The <sys/socket.h> header shall define the type socklen_t, which is an integer type of width of at least 32 bits; see APPLICATION USAGE.
typedef uint32_t socklen_t;

// The <sys/socket.h> header shall define the unsigned integer type sa_family_t.
typedef unsigned int sa_family_t;

// The <sys/socket.h> header shall define the sockaddr structure that includes at least the following members:
struct sockaddr{
   sa_family_t  sa_family;    // Address family. 
   char         sa_data[14];  // Socket address (variable-length data). actually longer address value.
};
// The sockaddr structure is used to define a socket address which is used in the bind(), connect(), getpeername(), getsockname(), recvfrom(), and sendto() functions.


// The <sys/socket.h> header shall define the following macros, with distinct integer values:
#define SOCK_DGRAM      ((int)(01)) // Datagram socket.
#define SOCK_RAW        ((int)(02)) // Raw Protocol Interface.
#define SOCK_SEQPACKET  ((int)(03)) // Sequenced-packet socket.
#define SOCK_STREAM     ((int)(04)) // Byte-stream socket.

// The <sys/socket.h> header shall define the following macro for use as the level argument of setsockopt() and getsockopt().
#define SOL_SOCKET

// Options to be accessed at socket level, not protocol level.
// The <sys/socket.h> header shall define the following macros, with distinct integer values, for use as the option_name argument in getsockopt() or setsockopt() calls:
#define SO_ACCEPTCONN        ((int)(01)) // Socket is accepting connections.
#define SO_BROADCAST         ((int)(02)) // Transmission of broadcast messages is supported.
#define SO_DEBUG Debugging   ((int)(03)) // information is being recorded.
#define SO_DONTROUTE         ((int)(04)) // Bypass normal routing.
#define SO_ERROR             ((int)(05)) // Socket error status.
#define SO_KEEPALIVE         ((int)(06)) // Connections are kept alive with periodic messages.
#define SO_LINGER            ((int)(07)) // Socket lingers on close.
#define SO_OOBINLINE         ((int)(08)) // Out-of-band data is transmitted in line.
#define SO_RCVBUF            ((int)(09)) // Receive buffer size.
#define SO_RCVLOWAT          ((int)(10)) // Receive ``low water mark''.
#define SO_RCVTIMEO          ((int)(11)) // Receive timeout.
#define SO_REUSEADDR         ((int)(12)) // Reuse of local addresses is supported.
#define SO_SNDBUF            ((int)(13)) // Send buffer size.
#define SO_SNDLOWAT          ((int)(14)) // Send ``low water mark''.
#define SO_SNDTIMEO          ((int)(15)) // Send timeout.
#define SO_TYPE              ((int)(16)) // Socket type.

// The <sys/socket.h> header shall define the following macro as the maximum backlog queue length which may be specified by the backlog field of the listen() function:
#define SOMAXCONN  ((int)(0)) //The maximum backlog queue length.


// The <sys/socket.h> header shall define the following macros, with distinct integer values, for use as the valid values for the msg_flags field in the msghdr structure, 
// or the flags parameter in recvfrom(), recvmsg(), sendmsg(), or sendto() calls:
#define MSG_CTRUNC        ((int)(01)) // Control data truncated.
#define MSG_DONTROUTE     ((int)(02)) // Send without using routing tables.
#define MSG_EOR           ((int)(04)) // Terminates a record (if supported by the protocol).
#define MSG_OOB           ((int)(05)) // Out-of-band data.
#define MSG_PEEK          ((int)(06)) // Leave received data in queue.
#define MSG_TRUNC         ((int)(07)) // Normal data truncated.
#define MSG_WAITALL       ((int)(08)) // Attempt to fill the read buffer.

// The <sys/socket.h> header shall define the following macros, with distinct integer values:
#define AF_INET     ((int)(01)) // Internet domain sockets for use with IPv4 addresses.
#define AF_INET6    ((int)(02)) // Internet domain sockets for use with IPv6 addresses. [Option End]
#define AF_UNIX     ((int)(03)) // UNIX domain sockets.
#define AF_UNSPEC   ((int)(04)) // Unspecified.


// The <sys/socket.h> header shall define the following macros, with distinct integer values:
#define SHUT_RD     ((int)(01)) // Disables further receive operations.
#define SHUT_RDWR   ((int)(02)) // Disables further send and receive operations.
#define SHUT_WR     ((int)(03)) // Disables further send operations.


#endif
