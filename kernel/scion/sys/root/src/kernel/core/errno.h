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
#ifndef __ERRNO_H__
#define __ERRNO_H__


/*===========================================
Includes
=============================================*/


/*===========================================
Declaration
=============================================*/

#ifdef E2BIG
   #undef E2BIG
#endif
#define E2BIG 1
//Argument list too long.

#ifdef EACCES
#undef EACCES
#endif
#define EACCES 2
//Permission denied.

#ifdef EADDRINUSE
#undef EADDRINUSE
#endif
#define EADDRINUSE 3
//Address in use.

#ifdef EADDRNOTAVAIL
#undef EADDRNOTAVAIL
#endif
#define EADDRNOTAVAIL 4
//Address not available.

#ifdef EAFNOSUPPORT
#undef EAFNOSUPPORT
#endif
#define EAFNOSUPPORT 5
//Address family not supported.

#ifdef EAGAIN
#undef EAGAIN
#endif
#define EAGAIN 6
//Resource unavailable, try again (may be the same value as EWOULDBLOCK).

#ifdef EALREADY
#undef EALREADY
#endif
#define EALREADY 7
//Connection already in progress.

#ifdef EBADF
#undef EBADF
#endif
#define EBADF 8
//Bad file descriptor.

#ifdef EBADMSG
#undef EBADMSG
#endif
#define EBADMSG 9
//Bad message.

#ifdef EBUSY
#undef EBUSY
#endif
#define EBUSY 10
//Device or resource busy.

#ifdef ECANCELED
#undef ECANCELED
#endif
#define ECANCELED 11
//Operation canceled.

#ifdef ECHILD
#undef ECHILD
#endif
#define ECHILD 12
//No child processes.

#ifdef ECONNABORTED
#undef ECONNABORTED
#endif
#define ECONNABORTED 13
//Connection aborted.

#ifdef ECONNREFUSED
#undef ECONNREFUSED
#endif
#define ECONNREFUSED 14
//Connection refused.

#ifdef ECONNRESET
#undef ECONNRESET
#endif
#define ECONNRESET 15
//Connection reset.

#ifdef EDEADLK
#undef EDEADLK
#endif
#define EDEADLK 16
//Resource deadlock would occur.

#ifdef EDESTADDRREQ
#undef EDESTADDRREQ
#endif
#define EDESTADDRREQ 17
//Destination address required.

#ifdef EDOM
#undef EDOM
#endif
#define EDOM 18
//Mathematics argument out of domain of function.

#ifdef EDQUOT
#undef EDQUOT
#endif
#define EDQUOT 19
//Reserved.

#ifdef EEXIST
#undef EEXIST
#endif
#define EEXIST 20
//File exists.

#ifdef EFAULT
#undef EFAULT
#endif
#define EFAULT 21
//Bad address.

#ifdef EFBIG
#undef EFBIG
#endif
#define EFBIG 22
//File too large.

#ifdef EHOSTUNREACH
#undef EHOSTUNREACH
#endif
#define EHOSTUNREACH 23
//Host is unreachable.

#ifdef EIDRM
#undef EIDRM
#endif
#define EIDRM 24
//Identifier removed.

#ifdef EILSEQ
#undef EILSEQ
#endif
#define EILSEQ 25
//Illegal byte sequence.

#ifdef EINPROGRESS
#undef EINPROGRESS
#endif
#define EINPROGRESS 26
//Operation in progress.

#ifdef EINTR
#undef EINTR
#endif
#define EINTR 27
//Interrupted function.

#ifdef EINVAL
#undef EINVAL
#endif
#define EINVAL 28
//Invalid argument.

#ifdef EIO
#undef EIO
#endif
#define EIO  29
//I/O error.

#ifdef EISCONN
#undef EISCONN
#endif
#define EISCONN 30
//Socket is connected.

#ifdef EISDIR
#undef EISDIR
#endif
#define EISDIR 31
//Is a directory.

#ifdef ELOOP
#undef ELOOP
#endif 
#define ELOOP 32
//Too many levels of symbolic links.

#ifdef EMFILE
#undef EMFILE
#endif
#define EMFILE 33
//Too many open files.

#ifdef EMLINK
#undef EMLINK
#endif
#define EMLINK 34
//Too many links.

#ifdef EMSGSIZE
#undef EMSGSIZE
#endif
#define EMSGSIZE 35
//Message too large.

#ifdef EMULTIHOP
#undef EMULTIHOP
#endif
#define EMULTIHOP 36
//Reserved.

#ifdef ENAMETOOLONG
#undef ENAMETOOLONG
#endif
#define ENAMETOOLONG 37
//Filename too long.

#ifdef ENETDOWN
#undef ENETDOWN
#endif
#define ENETDOWN 38
//Network is down.

#ifdef ENETUNREACH
#undef ENETUNREACH
#endif
#define ENETUNREACH 39
//Network unreachable.

#ifdef ENFILE
#undef ENFILE
#endif
#define ENFILE 40
//Too many files open in system.

#ifdef ENOBUFS
#undef ENOBUFS
#endif
#define ENOBUFS 41
//No buffer space available.

#ifdef ENODATA
#undef ENODATA
#endif
#define ENODATA 42
//No message is available on the STREAM head read queue.

#ifdef ENODEV
#undef ENODEV
#endif
#define ENODEV 43
//No such device.

#ifdef ENOENT
#undef ENOENT
#endif
#define ENOENT 44
//No such file or directory.

#ifdef ENOEXEC
#undef ENOEXEC
#endif
#define ENOEXEC 45
//Executable file format error.

#ifdef ENOLCK
#undef ENOLCK
#endif
#define ENOLCK 46
//No locks available.

#ifdef ENOLINK
#undef ENOLINK
#endif
#define ENOLINK 47
//Reserved.

#ifdef ENOMEM
#undef ENOMEM
#endif
#define ENOMEM 48
//Not enough space.

#ifdef ENOMSG
#undef ENOMSG
#endif
#define ENOMSG 49
//No message of the desired type.

#ifdef ENOPROTOOPT
#undef ENOPROTOOPT
#endif
#define ENOPROTOOPT 50
//Protocol not available.

#ifdef ENOSPC
#undef ENOSPC
#endif
#define ENOSPC 51
//No space left on device.

#ifdef ENOSR
#undef ENOSR
#endif
#define ENOSR 52
//No STREAM resources.

#ifdef ENOSTR
#undef ENOSTR
#endif
#define ENOSTR 53
//Not a STREAM.

#ifdef ENOSYS
#undef ENOSYS
#endif
#define ENOSYS 54
//Function not supported.

#ifdef ENOTCONN
#undef ENOTCONN
#endif
#define ENOTCONN 55
//The socket is not connected.

#ifdef ENOTDIR
#undef ENOTDIR
#endif
#define ENOTDIR 56
//Not a directory.

#ifdef ENOTEMPTY
#undef ENOTEMPTY
#endif
#define ENOTEMPTY 57
//Directory not empty.

#ifdef ENOTSOCK
#undef ENOTSOCK
#endif
#define ENOTSOCK 58
//Not a socket.

#ifdef ENOTSUP
#undef ENOTSUP
#endif
#define ENOTSUP 59
//Not supported.

#ifdef ENOTTY
#undef ENOTTY
#endif
#define ENOTTY 60
//Inappropriate I/O control operation.

#ifdef ENXIO
#undef ENXIO
#endif
#define ENXIO 61
//No such device or address.

#ifdef EOPNOTSUPP
#undef EOPNOTSUPP
#endif
#define EOPNOTSUPP 62
//Operation not supported on socket.

#ifdef EOVERFLOW
#undef EOVERFLOW
#endif
#define EOVERFLOW 63
//Value too large to be stored in data type.

#ifdef EPERM
#undef EPERM
#endif
#define EPERM 64
//Operation not permitted.

#ifdef EPIPE
#undef EPIPE
#endif
#define EPIPE 65
//Broken pipe.

#ifdef EPROTO
#undef EPROTO
#endif
#define EPROTO 66
//Protocol error.

#ifdef EPROTONOSUPPORT
#undef EPROTONOSUPPORT
#endif
#define EPROTONOSUPPORT 67
//Protocol not supported.

#ifdef EPROTOTYPE
#undef EPROTOTYPE
#endif
#define EPROTOTYPE 68
//Socket type not supported.

#ifdef ERANGE
#undef ERANGE
#endif
#define ERANGE 69
//Result too large.

#ifdef EROFS
#undef EROFS
#endif
#define EROFS 70
//Readonly file system.

#ifdef ESPIPE
#undef ESPIPE
#endif
#define ESPIPE 71
//Invalid seek.

#ifdef ESRCH
#undef ESRCH
#endif
#define ESRCH 72
//No such process.

#ifdef ESTALE
#undef ESTALE
#endif
#define ESTALE 73
//Reserved.

#ifdef ETIME
#undef ETIME
#endif
#define ETIME 74
//Stream ioctl() timeout.

#ifdef ETIMEDOUT
#undef ETIMEDOUT
#endif
#define ETIMEDOUT 75
//Connection timed out.

#ifdef ETXTBSY
#undef ETXTBSY
#endif
#define ETXTBSY 76
//Text file busy.

#ifdef EWOULDBLOCK
#undef EWOULDBLOCK
#endif
#define EWOULDBLOCK 77
//Operation would block (may be the same value as [EAGAIN]).


#ifdef EXDEV
#undef EXDEV
#endif
#define EXDEV 78
//Crossdevice link.


#endif
