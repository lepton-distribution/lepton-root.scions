/*
The contents of this file are subject to the Mozilla Public License Version 1.1
(the "License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at http://www.mozilla.org/MPL/

Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for the
specific language governing rights and limitations under the License.

The Original Code is Lepton.

The Initial Developer of the Original Code is Philippe Le Boulanger.
Portions created by Philippe Le Boulanger are Copyright (C) 2017 <lepton.phlb@gmail.com>.
All Rights Reserved.

Contributor(s): Jean-Jacques Pitrolle <lepton.jjp@gmail.com>.

Alternatively, the contents of this file may be used under the terms of the eCos GPL license
(the  [eCos GPL] License), in which case the provisions of [eCos GPL] License are applicable
instead of those above. If you wish to allow use of your version of this file only under the
terms of the [eCos GPL] License and not to allow others to use your version of this file under
the MPL, indicate your decision by deleting  the provisions above and replace
them with the notice and other provisions required by the [eCos GPL] License.
If you do not delete the provisions above, a recipient may use your version of this file under
either the MPL or the [eCos GPL] License."
*/


// from https://stackoverflow.com/questions/22077802/simple-c-example-of-doing-an-http-post-and-consuming-the-response
// url encode/decode from http://www.geekhideout.com/urlcode.shtml
// json embedded parser http://zserge.com/jsmn.html

/*===========================================
Includes
=============================================*/
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "kernel/core/kernelconf.h"
#include "kernel/core/dirent.h"
#include "kernel/core/kernel.h"
#include "kernel/core/system.h"
#include "kernel/core/signal.h"
#include "kernel/core/wait.h"
#include "kernel/core/stat.h"
#include "kernel/core/statvfs.h"
#include "kernel/core/libstd.h"
#include "kernel/core/time.h"
#include "kernel/core/select.h"
#include "kernel/core/fcntl.h"

#include "lib/libc/unistd.h"
#include "lib/libc/stdio/stdio.h"
#include "lib/libc/ctype/ctype.h"
#include "lib/pthread/pthread.h"
#include "lib/libc/net/socket.h"
#include "lib/libc/net/netdb.h"



/*===========================================
Global Declaration
=============================================*/
#define TANK_ID  "22beb489-2ba9-44c8-b189-5855e1d4-1-1"
#define CASE_ID  "22baf683-2cd1-28f4-a593-5391e1d4-1-1"

/* ---- Base64 Encoding/Decoding Table --- */
const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

//
static const char format_http_request_get[] = "GET /api/v1/parameters HTTP/1.1\r\n\
Host:%s:%d\r\n\
X-Case-Id:%s\r\n\
Cache-Control:no-cache\r\n\
Accept: */*\r\n\
\r\n";

static const char format_http_request_header_post[] = "POST /api/v1/store HTTP/1.1\r\n\
Host: %s:%d\r\n\
X-Case-Id: %s\r\n\
Content-Type: application/x-www-form-urlencoded\r\n\
Content-Length: %d\r\n\
Cache-Control: no-cache\r\n\
\r\n";

static const char format_senml_begin[] = "d=[";
static const char format_senml_end[] = "]";


static const char format_senml_identification[] = "\
{\
\"bn\": \"%s/%s/%d/\"\
}";

static const char format_senml_temperature_external[] = "\
{\
\"t\": %d,\
\"n\": \"temp\",\
\"v\": %f\
}";

static const char format_senml_temperature_internal[] = "\
{\
\"t\": %d,\
\"n\": \"tint\",\
\"v\": %f\
}";

static const char format_senml_pressure[] = "\
{\
\"t\": %d,\
\"n\": \"pres\",\
\"v\": %f\
}";

static const char format_senml_battery_capacity[] = "\
{\
\"t\": %d,\
\"n\": \"batt\",\
\"v\": %f\
}";

static const char format_senml_gnss_location[] = "\
{\
\"t\": %d,\
\"n\": \"geo\",\
\"vs\": \"%f/%f\"\
}";


static const char format_senml_insertion_switch[] = "\
{\
\"t\": %d,\
\"n\": \"fdci\",\
\"vs\": \"%s\"\
}";


static const char format_senml_high_switch[] = "\
{\
\"t\": %d,\
\"n\": \"fdch\",\
\"vs\": \"%s\"\
}";

static const char format_senml_low_switch[] = "\
{\
\"t\": %d,\
\"n\": \"fdcl\",\
\"vs\": \"%s\"\
}";

static char g_http_request_buffer[512];
static char g_http_request_content_buffer[2048];

static char g_http_request_urlencoded_content_buffer[2048];
static char g_http_request_base64encoded_content_buffer[2048];

static char* g_ptr_http_content_buffer = g_http_request_urlencoded_content_buffer;


/*===========================================
Implementation
=============================================*/

void error(const char *msg) { 
   perror(msg); exit(0); 
}

/* ------------------------------------------------------------------------ *
* file:        base64_stringencode.c v1.0                                  *
* purpose:     tests encoding/decoding strings with base64                 *
* author:      02/23/2009 Frank4DD                                         *
*                                                                          *
* source:      http://base64.sourceforge.net/b64.c for encoding            *
*              http://en.literateprograms.org/Base64_(C) for decoding      *
* ------------------------------------------------------------------------ */


/* decodeblock - decode 4 '6-bit' characters into 3 8-bit binary bytes */
void decodeblock(unsigned char in[], char *clrstr) {
   unsigned char out[4];
   out[0] = in[0] << 2 | in[1] >> 4;
   out[1] = in[1] << 4 | in[2] >> 2;
   out[2] = in[2] << 6 | in[3] >> 0;
   out[3] = '\0';
   strncat(clrstr, out, sizeof(out));
}

void b64_decode(char *clrdst, char *b64src) {
   int c, phase, i;
   unsigned char in[4];
   char *p;

   clrdst[0] = '\0';
   phase = 0; i = 0;
   while (b64src[i]) {
      c = (int)b64src[i];
      if (c == '=') {
         decodeblock(in, clrdst);
         break;
      }
      p = strchr(b64, c);
      if (p) {
         in[phase] = p - b64;
         phase = (phase + 1) % 4;
         if (phase == 0) {
            decodeblock(in, clrdst);
            in[0] = in[1] = in[2] = in[3] = 0;
         }
      }
      i++;
   }
}

/* encodeblock - encode 3 8-bit binary bytes as 4 '6-bit' characters */
void encodeblock(unsigned char in[], char b64str[], int len) {
   unsigned char out[5];
   out[0] = b64[in[0] >> 2];
   out[1] = b64[((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4)];
   out[2] = (unsigned char)(len > 1 ? b64[((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6)] : '=');
   out[3] = (unsigned char)(len > 2 ? b64[in[2] & 0x3f] : '=');
   out[4] = '\0';
   strncat(b64str, out, sizeof(out));
}

/* encode - base64 encode a stream, adding padding if needed */
int b64_encode(char *b64dst, char *clrstr) {
   unsigned char in[3];
   int i, len = 0;
   int j = 0;
   int cb = 0;

   //
   b64dst[0] = '\0';
   while (clrstr[j]) {
      len = 0;
      for (i = 0; i<3; i++) {
         in[i] = (unsigned char)clrstr[j];
         if (clrstr[j]) {
            len++; j++;
         }
         else in[i] = 0;
      }
      if (len) {
         encodeblock(in, b64dst, len);
      }
      //
      cb += len;
   }
   //
   return cb;
}

/* Converts a hex character to its integer value */
char from_hex(char ch) {
   return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

/* Converts an integer value to its hex character*/
char to_hex(char code) {
   static char hex[] = "0123456789abcdef";
   return hex[code & 15];
}

/* Returns a url-encoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
int url_encode(char*buf, int size, char *str) {
   char *pstr = str, *pbuf = buf;

   //
   if (size < (strlen(buf) * 3 + 1))
      return -1;
   //
   while (*pstr) {
      if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~')
         *pbuf++ = *pstr;
      else if (*pstr == ' ')
         *pbuf++ = '+';
      else
         *pbuf++ = '%', *pbuf++ = to_hex(*pstr >> 4), *pbuf++ = to_hex(*pstr & 15);
      pstr++;
   }
   *pbuf = '\0';
   //
   return strlen(buf);
}

/* Returns a url-decoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
int url_decode(char*buf, int size, char *str) {
   char *pstr = str, *pbuf = buf;

   //
   if (size < (strlen(buf) + 1))
      return -1;
   //
   while (*pstr) {
      if (*pstr == '%') {
         if (pstr[1] && pstr[2]) {
            *pbuf++ = from_hex(pstr[1]) << 4 | from_hex(pstr[2]);
            pstr += 2;
         }
      }
      else if (*pstr == '+') {
         *pbuf++ = ' ';
      }
      else {
         *pbuf++ = *pstr;
      }
      pstr++;
   }
   *pbuf = '\0';
   //
   return strlen(buf);
}


int construct_http_request_post_add(char* http_request_buffer, int size, const char* format, ...) {
   int r;
   char* p_start_buffer;
   int len;

   va_list ap;

   //
   len = strlen(http_request_buffer);
   p_start_buffer = &http_request_buffer[len];
   if (len > 0 && http_request_buffer[len - 1] == '}') {
      if (format[0] == '{' && (len + 1)<size) {
         http_request_buffer[len] = ',';
         http_request_buffer[len + 1] = '\0';
         len = len + 1;
         p_start_buffer = &http_request_buffer[len];
      }
   }
   //
   va_start(ap, format);
   //
   r = vsnprintf(p_start_buffer, size - len, format, ap);
   //
   va_end(ap);
   //
   return r;
}

/*-------------------------------------------
| Name:httpc_main
| Description:
| Parameters:
| Return Type:
| Comments:  http://collector.o10ee.com:36194
| See:
---------------------------------------------*/
int httpc_main(int argc, char *argv[]){
   int socket_fd;
   int cb = 0;

   struct hostent *server;
   struct sockaddr_in serv_addr;
   char  response[4096];
   int portno = 36194; 
   char *host;
   char* request_type = argc > 3 ? argv[3] : "POST";

   if (argc > 1) {
      host = strlen(argv[1]) > 0 ? argv[1] : "collector.o10ee.com";
   }else {
      host = "collector.o10ee.com";
   }
   //
   if (argc > 2) {
      portno = atoi(argv[2]) > 0 ? atoi(argv[2]) : 80;
   }
  

   //
   time_t senml_time = time(NULL);
   //
   int  header_length = 0;
   int  content_length = 0;

   if (!strcmp(request_type, "GET")) {
      //
      memset(g_http_request_buffer, 0, sizeof(g_http_request_buffer));
      construct_http_request_post_add(g_http_request_buffer, sizeof(g_http_request_buffer), format_http_request_get, host, portno, CASE_ID);
      header_length = strlen(g_http_request_buffer);
      //
   }
   else if (!strcmp(request_type, "POST")) {
      memset(g_http_request_content_buffer, 0, sizeof(g_http_request_content_buffer));
      construct_http_request_post_add(g_http_request_content_buffer, sizeof(g_http_request_content_buffer), format_senml_begin);
      construct_http_request_post_add(g_http_request_content_buffer, sizeof(g_http_request_content_buffer), format_senml_identification, TANK_ID, CASE_ID, 1);
      construct_http_request_post_add(g_http_request_content_buffer, sizeof(g_http_request_content_buffer), format_senml_temperature_external, (int)senml_time, (float)18.1);
      construct_http_request_post_add(g_http_request_content_buffer, sizeof(g_http_request_content_buffer), format_senml_end);
      content_length = strlen(g_http_request_content_buffer);

      content_length = url_encode(g_http_request_urlencoded_content_buffer + 2, sizeof(g_http_request_urlencoded_content_buffer) - 2, g_http_request_content_buffer + 2);
      g_http_request_urlencoded_content_buffer[0] = 'd';
      g_http_request_urlencoded_content_buffer[1] = '=';
      content_length += 2;

      //content_length=b64_encode(g_http_request_base64encoded_content_buffer, g_http_request_urlencoded_content_buffer);

      //
      memset(g_http_request_buffer, 0, sizeof(g_http_request_buffer));
      construct_http_request_post_add(g_http_request_buffer, sizeof(g_http_request_buffer), format_http_request_header_post, host, portno, CASE_ID, content_length);
      header_length = strlen(g_http_request_buffer);
   }

   /* create the socket */

   //Create a socket
   if ((socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) <0)
   {
      printf("--> error: could not create socket\r\n");
      return -1;
   }
   //
   printf("--> socket created\n");

   //
   server = gethostbyname(host);
   //
   if(server==(struct hostent *)0){
      printf("--> error: unresolved host name:%s\r\n",host);
      return -1;
   }
   //
   memset(&serv_addr, 0, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_port = htons(portno);
   memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
   //Connect to remote server
   if (connect(socket_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
   {
      printf("--> error: connect failed with error code\r\n");
      return 1;
   }

   //
   puts("--> socket connected");
   //
   printf("--> send  post header: %s\r\n", g_http_request_buffer);
   if (send(socket_fd, g_http_request_buffer, strlen(g_http_request_buffer), 0) < 0)
   {
      printf("--> error: send failed with error code\r\n");
      return 1;
   }
   printf("--> header sent %d\r\n\r\n", strlen(g_http_request_buffer));

   if (content_length > 0) {
      //
      printf("--> send content: %s\r\n", g_ptr_http_content_buffer);
      if (send(socket_fd, g_ptr_http_content_buffer, strlen(g_ptr_http_content_buffer), 0) < 0)
      {
         printf("--> error: send failed\r\n");
         return 1;
      }
      printf("--> content sent %d\r\n", strlen(g_ptr_http_content_buffer));
   }


   //
   puts("--> reply received:\r\n");
   while ((cb = recv(socket_fd, response, sizeof(response), 0)) > 0) {
      //Add a NULL terminating character to make it a proper string before printing
      //response[cb] = '\0';
      for(int i=0; i<cb;i++){
         write(1,&response[i],1);
      }
      //puts(response);
   }

   //
   close(socket_fd);

   return 0;
}
/*============================================
| End of Source  : httpc.c
==============================================*/