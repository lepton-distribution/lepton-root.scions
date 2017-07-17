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
#ifndef __BSD_INET_IN_H__
#define __BSD_INET_IN_H__


/*============================================
| Includes
==============================================*/

/*============================================
| Declaration
==============================================*/

// The <netinet/in.h> header shall define the following types:
typedef uint16_t in_port_t; // Equivalent to the type uint16_t as defined in <inttypes.h> .
typedef uint32_t in_addr_t; // Equivalent to the type uint32_t as defined in <inttypes.h> .

// The sa_family_t type shall be defined as described in <sys/socket.h>.

// The uint8_t and uint32_t type shall be defined as described in <inttypes.h>. 
// Inclusion of the <netinet/in.h> header may also make visible all symbols from <inttypes.h> and <sys/socket.h>.

// The <netinet/in.h> header shall define the in_addr structure that includes at least the following member: in_addr_t  s_addr;
struct in_addr{
   in_addr_t  s_addr;
};

// The <netinet/in.h> header shall define the sockaddr_in structure that includes at least the following members:
struct sockaddr_in{ 
   sa_family_t     sin_family;   // AF_INET. 
   in_port_t       sin_port;     // Port number. 
   struct in_addr  sin_addr;     // IP address. 
};
// The sin_port and sin_addr members shall be in network byte order.
// The sockaddr_in structure is used to store addresses for the Internet address family. 
// Values of this type shall be cast by applications to struct sockaddr for use with socket functions.



#ifdef MODEM_SOCKET_IPV6

// [IP6] [Option Start] The <netinet/in.h> header shall define the in6_addr structure that contains at least the following member:
struct in6_addr{
   uint8_t s6_addr[16]; // This array is used to contain a 128-bit IPv6 address, stored in network byte order.
};

// The <netinet/in.h> header shall define the sockaddr_in6 structure that includes at least the following members:
struct sockaddr_in6 { 
   sa_family_t      sin6_family;    // AF_INET6. 
   in_port_t        sin6_port;      // Port number. 
   uint32_t         sin6_flowinfo;  // IPv6 traffic class and flow information. 
   struct in6_addr  sin6_addr;      // IPv6 address. 
   uint32_t         sin6_scope_id;  // Set of interfaces for a scope. 
};
// The sin6_port and sin6_addr members shall be in network byte order.
// The sockaddr_in6 structure shall be set to zero by an application prior to using it, since implementations are free to have additional, implementation-defined fields in sockaddr_in6.
// The sin6_scope_id field is a 32-bit integer that identifies a set of interfaces as appropriate for the scope of the address carried in the sin6_addr field. 
// For a link scope sin6_addr, the application shall ensure that sin6_scope_id is a link index. For a site scope sin6_addr, the application shall ensure that sin6_scope_id is a site index. 
// The mapping of sin6_scope_id to an interface or set of interfaces is implementation-defined.


// The <netinet/in.h> header shall declare the following external variable:
extern const struct in6_addr in6addr_any
// This variable is initialized by the system to contain the wildcard IPv6 address. 
// The <netinet/in.h> header also defines the IN6ADDR_ANY_INIT macro. 
// This macro must be constant at compile time and can be used to initialize a variable of type struct in6_addr to the IPv6 wildcard address.

// The <netinet/in.h> header shall declare the following external variable:
extern const struct in6_addr in6addr_loopback
// This variable is initialized by the system to contain the loopback IPv6 address. The <netinet/in.h> header also defines the IN6ADDR_LOOPBACK_INIT macro. 
// This macro must be constant at compile time and can be used to initialize a variable of type struct in6_addr to the IPv6 loopback address.


// The <netinet/in.h> header shall define the ipv6_mreq structure that includes at least the following members:
typedef struct in6_addr  ipv6mr_multiaddr;  // IPv6 multicast address. 
typedef unsigned          ipv6mr_interface;  // Interface index. 

#endif // MODEM_SOCKET_IPV6



// The <netinet/in.h> header shall define the following macros for use as values of the level argument of getsockopt() and setsockopt():
#define IPPROTO_IP     ((int)(01)) // Internet protocol.
#ifdef MODEM_SOCKET_IPV6
#define IPPROTO_IPV6   ((int)(02)) // [IP6] [Option Start] Internet Protocol Version 6. [Option End]
#endif
#define  IPPROTO_ICMP   ((int)(03)) // Control message protocol.
#define  IPPROTO_RAW    ((int)(04)) // [RS] [Option Start] Raw IP Packets Protocol. [Option End]
#define  IPPROTO_TCP    ((int)(05)) // Transmission control protocol.
#define  IPPROTO_UDP    ((int)(06)) // User datagram protocol.

// The <netinet/in.h> header shall define the following macros for use as destination addresses for connect(), sendmsg(), and sendto():
#define INADDR_ANY         ((unsigned long int)(0x00000000))   // IPv4 local host address. Address to accept any incoming messages. 
#define INADDR_BROADCAST   ((unsigned long int)(0xffffffff))   // IPv4 broadcast address. Address to send to all hosts.
#define INADDR_NONE        ((unsigned long int)(0xffffffff))   //Address indicating an error return.
#define IN_LOOPBACKNET		(127)                               // Network number for local host loopback.
#define INADDR_LOOPBACK	   ((unsigned long int)(0x7f000001))	// Address to loopback in software to local host: 127.0.0.1 
#define IN_LOOPBACK(__a__) ((((unsignedlong int) (__a__)) & ((unsigned long int)(0xff000000)) ) == ((unsigned long int)(0x7f000000))


// The <netinet/in.h> header shall define the following macro to help applications declare buffers of the proper size to store IPv4 addresses in string form:
#define  INET_ADDRSTRLEN 16  //Length of the string form for IP.
// The htonl(), htons(), ntohl(), and ntohs() functions shall be available as defined in <arpa/inet.h>. 
// Inclusion of the <netinet/in.h> header may also make visible all symbols from <arpa/inet.h>.

extern uint16_t libc_htons(uint16_t v);
extern uint32_t libc_htonl(uint32_t v);
extern uint16_t libc_ntohs(uint16_t v);
extern uint32_t libc_ntohl(uint32_t v);
   
#define  htons(__v__)  libc_htons(__v__)
#define  htonl(__v__)  libc_htonl(__v__)
#define  ntohs(__v__)  libc_ntohs(__v__)
#define  ntohl(__v__)  libc_ntohl(__v__)

extern in_addr_t libc_inet_addr(const char *cp);
#define  inet_addr(__v__) libc_inet_addr(__v__)

#ifdef MODEM_SOCKET_IPV6

// [IP6] [Option Start] The <netinet/in.h> header shall define the following macro to help applications declare buffers of the proper size to store IPv6 addresses in string form:
#define INET6_ADDRSTRLEN 46 // Length of the string form for IPv6.

// The <netinet/in.h> header shall define the following macros, with distinct integer values, for use in the option_name argument in the getsockopt() or setsockopt() functions at protocol level IPPROTO_IPV6:
#define IPV6_JOIN_GROUP            ((int)(01)) // Join a multicast group.
#define IPV6_LEAVE_GROUP           ((int)(02)) // Quit a multicast group.
#define IPV6_MULTICAST_HOPS        ((int)(03)) // Multicast hop limit.
#define IPV6_MULTICAST_IF          ((int)(04)) // Interface to use for outgoing multicast packets.
#define IPV6_MULTICAST_LOOP        ((int)(05)) // Multicast packets are delivered back to the local application.
#define IPV6_UNICAST_HOPS          ((int)(06)) // Unicast hop limit.
#define IPV6_V6ONLY                ((int)(07)) // Restrict AF_INET6 socket to IPv6 communications only.

// The <netinet/in.h> header shall define the following macros that test for special IPv6 addresses. Each macro is of type int and takes a single argument of type const struct in6_addr *:
#define IN6_IS_ADDR_UNSPECIFIED    // Unspecified address.
#define IN6_IS_ADDR_LOOPBACK       // Loopback address.
#define IN6_IS_ADDR_MULTICAST      // Multicast address.
#define IN6_IS_ADDR_LINKLOCAL      // Unicast link-local address.
#define IN6_IS_ADDR_SITELOCAL      // Unicast site-local address.
#define IN6_IS_ADDR_V4MAPPED       // IPv4 mapped address.
#define IN6_IS_ADDR_V4COMPAT       // IPv4-compatible address.
#define IN6_IS_ADDR_MC_NODELOCAL   // Multicast node-local address.
#define IN6_IS_ADDR_MC_LINKLOCAL   // Multicast link-local address.
#define IN6_IS_ADDR_MC_SITELOCAL   // Multicast site-local address.
#define IN6_IS_ADDR_MC_ORGLOCAL    // Multicast organization-local address.
#define IN6_IS_ADDR_MC_GLOBAL      // Multicast global address. 

#endif // MODEM_SOCKET_IPV6



#endif
