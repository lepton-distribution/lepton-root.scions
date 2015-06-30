/* termios type and macro definitions.  Linux version.
   Copyright (C) 1993, 94, 95, 96, 97, 98, 99 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#ifndef _TERMIOS_H
#define _TERMIOS_H


/*===========================================
Includes
=============================================*/


/*===========================================
Declaration
=============================================*/

/* 0x54 is just a magic number to make these relatively unique ('T') */
#define TCGETS          0x5401
#define TCSETS          0x5402
#define TCSETSW         0x5403
#define TCSETSF         0x5404
#define TCGETA          0x5405
#define TCSETA          0x5406
#define TCSETAW         0x5407
#define TCSETAF         0x5408
#define TCSBRK          0x5409
#define TCXONC          0x540A
#define TCFLSH          0x540B
#define TIOCEXCL        0x540C
#define TIOCNXCL        0x540D
#define TIOCSCTTY       0x540E
#define TIOCGPGRP       0x540F
#define TIOCSPGRP       0x5410
#define TIOCOUTQ        0x5411
#define TIOCSTI         0x5412
#define TIOCGWINSZ      0x5413
#define TIOCSWINSZ      0x5414
#define TIOCMGET        0x5415
#define TIOCMBIS        0x5416
#define TIOCMBIC        0x5417
#define TIOCMSET        0x5418
#define TIOCGSOFTCAR    0x5419
#define TIOCSSOFTCAR    0x541A
//#define FIONREAD	0x541B //see kernel/core/ioctl.h
#define TIOCINQ         FIONREAD
#define TIOCLINUX       0x541C
#define TIOCCONS        0x541D
#define TIOCGSERIAL     0x541E
#define TIOCSSERIAL     0x541F
#define TIOCPKT         0x5420
//#define FIONBIO		0x5421 //see kernel/core/ioctl.h
#define TIOCNOTTY       0x5422
#define TIOCSETD        0x5423
#define TIOCGETD        0x5424
#define TCSBRKP         0x5425  /* Needed for POSIX tcsendbreak() */
#define TIOCTTYGSTRUCT  0x5426  /* For debugging only */
#define TIOCSBRK        0x5427  /* BSD compatibility */
#define TIOCCBRK        0x5428  /* BSD compatibility */
#define TIOCGSID        0x5429  /* Return the session ID of FD */
#define TIOCGPTN        _IOR('T',0x30, unsigned int) /* Get Pty Number (of pty-mux device) */
#define TIOCSPTLCK      _IOW('T',0x31, int)  /* Lock/unlock Pty */

#define FIONCLEX        0x5450  /* these numbers need to be adjusted. */
#define FIOCLEX         0x5451
#define FIOASYNC        0x5452
#define TIOCSERCONFIG   0x5453
#define TIOCSERGWILD    0x5454
#define TIOCSERSWILD    0x5455
#define TIOCGLCKTRMIOS  0x5456
#define TIOCSLCKTRMIOS  0x5457
#define TIOCSERGSTRUCT  0x5458 /* For debugging only */
#define TIOCSERGETLSR   0x5459 /* Get line status register */
#define TIOCSERGETMULTI 0x545A /* Get multiport config  */
#define TIOCSERSETMULTI 0x545B /* Set multiport config */

#define TIOCMIWAIT      0x545C  /* wait for a change on serial input line(s) */
#define TIOCGICOUNT     0x545D  /* read serial port inline interrupt counts */
#define TIOCGHAYESESP   0x545E  /* Get Hayes ESP configuration */
#define TIOCSHAYESESP   0x545F  /* Set Hayes ESP configuration */
#define FIOQSIZE        0x5460

/* Used for packet mode */
#define TIOCPKT_DATA                   0
#define TIOCPKT_FLUSHREAD           1
#define TIOCPKT_FLUSHWRITE          2
#define TIOCPKT_STOP                   4
#define TIOCPKT_START               8
#define TIOCPKT_NOSTOP              16
#define TIOCPKT_DOSTOP              32

#define TIOCSER_TEMT    0x01    /* Transmitter physically empty */

/* Socket-level I/O control calls. */
//exclude for lepton: conflict with redefinition in ip stack.
/*
#define FIOSETOWN       0x8901
#define SIOCSPGRP	0x8902
#define FIOGETOWN	0x8903
#define SIOCGPGRP	0x8904
#define SIOCATMARK	0x8905
#define SIOCGSTAMP	0x8906		//Get stamp
*/

/* modem lines */

#define TIOCM_LE        0x001
#define TIOCM_DTR       0x002
#define TIOCM_RTS       0x004
#define TIOCM_ST        0x008
#define TIOCM_SR        0x010
#define TIOCM_CTS       0x020
#define TIOCM_CAR       0x040
#define TIOCM_RNG       0x080
#define TIOCM_DSR       0x100
#define TIOCM_CD        TIOCM_CAR
#define TIOCM_RI        TIOCM_RNG
#define TIOCM_OUT1      0x2000
#define TIOCM_OUT2      0x4000
#define TIOCM_LOOP      0x8000


/* The <termios.h> header is used for controlling tty modes. */
typedef unsigned short tcflag_t;
typedef unsigned char cc_t;
typedef unsigned long speed_t;

#define NCCS            20      /* size of cc_c array, some extra space
                               * for extensions. */

#define __USE_MISC

/* Primary terminal control structure. POSIX Table 7-1. */
struct termios {
   tcflag_t c_iflag;          /* input modes */
   tcflag_t c_oflag;          /* output modes */
   tcflag_t c_cflag;          /* control modes */
   tcflag_t c_lflag;          /* local modes */
   speed_t c_ispeed;          /* input speed */
   speed_t c_ospeed;          /* output speed */
   cc_t c_cc[NCCS];           /* control characters */
};

#define VINTR 0
#define VQUIT 1
#define VERASE 2
#define VKILL 3
#define VEOF 4
#define VTIME 5
#define VMIN 6
#define VSWTC 7
#define VSTART 8
#define VSTOP 9
#define VSUSP 10
#define VEOL 11
#define VREPRINT 12
#define VDISCARD 13
#define VWERASE 14
#define VLNEXT 15
#define VEOL2 16

/* c_iflag bits */
#define IGNBRK  0000001
#define BRKINT  0000002
#define IGNPAR  0000004
#define PARMRK  0000010
#define INPCK   0000020
#define ISTRIP  0000040
#define INLCR   0000100
#define IGNCR   0000200
#define ICRNL   0000400
#define IUCLC   0001000
#define IXON    0002000
#define IXANY   0004000
#define IXOFF   0010000
#define IMAXBEL 0020000

/* c_oflag bits */
#define OPOST   0000001
#define OLCUC   0000002
#define ONLCR   0000004
#define OCRNL   0000010
#define ONOCR   0000020
#define ONLRET  0000040
#define OFILL   0000100
#define OFDEL   0000200
#if defined __USE_MISC || defined __USE_XOPEN
   # define NLDLY  0000400
   # define   NL0  0000000
   # define   NL1  0000400
   # define CRDLY  0003000
   # define   CR0  0000000
   # define   CR1  0001000
   # define   CR2  0002000
   # define   CR3  0003000
   # define TABDLY 0014000
   # define   TAB0 0000000
   # define   TAB1 0004000
   # define   TAB2 0010000
   # define   TAB3 0014000
   # define BSDLY  0020000
   # define   BS0  0000000
   # define   BS1  0020000
   # define FFDLY  0100000
   # define   FF0  0000000
   # define   FF1  0100000
#endif

#define VTDLY   0040000
#define   VT0   0000000
#define   VT1   0040000

#ifdef __USE_MISC
   # define XTABS  0014000
#endif

/* c_cflag bit meaning */
#ifdef __USE_MISC
   # define CBAUD  0010017
#endif
#define  B0     0000000         /* hang up */
#define  B50    0000001
#define  B75    0000002
#define  B110   0000003
#define  B134   0000004
#define  B150   0000005
#define  B200   0000006
#define  B300   0000007
#define  B600   0000010
#define  B1200  0000011
#define  B1800  0000012
#define  B2400  0000013
#define  B4800  0000014
#define  B9600  0000015
#define  B19200 0000016
#define  B38400 0000017
#ifdef __USE_MISC
   # define EXTA B19200
   # define EXTB B38400
#endif
#define CSIZE   0000060
#define   CS5   0000000
#define   CS6   0000020
#define   CS7   0000040
#define   CS8   0000060
#define CSTOPB  0000100
#define CREAD   0000200
#define PARENB  0000400
#define PARODD  0001000
#define HUPCL   0002000
#define CLOCAL  0004000
#ifdef __USE_MISC
   # define CBAUDEX 0010000
#endif
#define  B57600   0010001
#define  B115200  0010002
#define  B230400  0010003
#define  B460800  0010004
#define  B500000  0010005
#define  B576000  0010006
#define  B921600  0010007
#define  B1000000 0010010
#define  B1152000 0010011
#define  B1500000 0010012
#define  B2000000 0010013
#define  B2500000 0010014
#define  B3000000 0010015
#define  B3500000 0010016
#define  B4000000 0010017
#define __MAX_BAUD B4000000
#ifdef __USE_MISC
   # define CIBAUD   002003600000       /* input baud rate (not used) */
   # define CRTSCTS  020000000000       /* flow control */
#endif

/* c_lflag bits */
#define ISIG    0000001
#define ICANON  0000002
#if defined __USE_MISC || defined __USE_XOPEN
   # define XCASE  0000004
#endif
#define ECHO    0000010
#define ECHOE   0000020
#define ECHOK   0000040
#define ECHONL  0000100
#define NOFLSH  0000200
#define TOSTOP  0000400
#ifdef __USE_MISC
   # define ECHOCTL 0001000
   # define ECHOPRT 0002000
   # define ECHOKE  0004000
   # define FLUSHO  0010000
   # define PENDIN  0040000
#endif
#define IEXTEN  0100000

/* tcflow() and TCXONC use these */
#define TCOOFF          0
#define TCOON           1
#define TCIOFF          2
#define TCION           3

/* tcflush() and TCFLSH use these */
#define TCIFLUSH        0
#define TCOFLUSH        1
#define TCIOFLUSH       2

/* tcsetattr uses these */
#define TCSANOW         0
#define TCSADRAIN       1
#define TCSAFLUSH       2


#define _IOT_termios /* Hurd ioctl type field.  */ \
   _IOT (_IOTS (cflag_t), 4, _IOTS (cc_t), NCCS, _IOTS (speed_t), 2)

/* Function Prototypes. */
int tcsendbreak   (int fd, int duration);
int tcdrain       (int fd);
int tcflush       (int fd, int queue_selector);
int tcflow        (int fd, int action);

speed_t cfgetispeed (const struct termios *termios_p);
speed_t cfgetospeed (const struct termios *termios_p);

int cfsetispeed   (struct termios *termios_p, speed_t speed);
int cfsetospeed   (struct termios *termios_p, speed_t speed);
int tcgetattr     (int fd, struct termios *termios_p);
int tcsetattr     (int fd, int opt_actions, const struct termios *termios_p);

int cfsetspeed    (struct termios *termios_p, speed_t speed);
void cfmakeraw    (struct termios *termios_p);

#endif

