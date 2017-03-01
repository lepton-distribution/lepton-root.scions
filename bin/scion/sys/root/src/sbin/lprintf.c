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
| Includes
==============================================*/
/*
#include <sys/types.h>

#include <err.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
*/


#include <stdlib.h>
#include <stdint.h>

#include "kernel/core/errno.h"
#include "kernel/core/libstd.h"
#include "kernel/core/sys/utsname.h"

#include "lib/libc/unistd/getopt.h"
#include "lib/libc/stdint.h"
#include "lib/libc/stdio/stdio.h"


/*============================================
| Global Declaration
==============================================*/
#if !defined(BUILTIN) && !defined(SHELL)
   #ifndef lint
static char const copyright[] =
   "@(#) Copyright (c) 1989, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
   #endif /* not lint */
#endif

#ifndef lint
   #if 0
static char const sccsid[] = "@(#)printf.c	8.1 (Berkeley) 7/20/93";
   #endif
static const char rcsid[] =
   "$FreeBSD: src/usr.bin/printf/printf.c,v 1.37 2005/08/05 08:18:00 stefanf Exp $";
#endif /* not lint */

#define warnx1(a, b, c)         warnx(a)
#define warnx2(a, b, c)         warnx(a, b)
#define warnx3(a, b, c)         warnx(a, b, c)


#define PF(f, func) do { \
      char b [256]; \
      if (havewidth) \
         if (haveprec) \
            (void)sprintf(b, f, fieldwidth, precision, func); \
         else \
            (void)sprintf(b, f, fieldwidth, func); \
      else if (haveprec) \
         (void)sprintf(b, f, precision, func); \
      else \
         (void)sprintf(b, f, func); \
      if (b) { \
         (void)fputs(b, stdout); \
      } \
} while (0)

static int       asciicode(void);
static char     *doformat(char *, int *);
static int       escape(char *, int, size_t *);
static int       getchr(void);
static int       getfloating(long double *, int);
static int       getint(int *);
static int       getnum(intmax_t *, uintmax_t *, int);
static const char
*getstr(void);
static char     *mknum(char *, int);
static void      usage(void);

static char **gargv;

/*============================================
| Implementation
==============================================*/


int printf_main(int argc, char *argv[])
{
   size_t len;
   int ch, chopped, end, rval;
   char *format, *fmt, *start;
   getopt_state_t getopt_state;

   //
   getopt_init(&getopt_state);

   while ((ch = getopt(&getopt_state,argc, argv, "")) != -1)
      switch (ch) {
      case '?':
      default:
         usage();
         return (1);
      }
   argc -= getopt_state.optind;
   argv += getopt_state.optind;

   if (argc < 1) {
      usage();
      return (1);
   }

   /*
    * Basic algorithm is to scan the format string for conversion
    * specifications -- once one is found, find out if the field
    * width or precision is a '*'; if it is, gather up value.  Note,
    * format strings are reused as necessary to use up the provided
    * arguments, arguments of zero/null string are provided to use
    * up the format string.
    */
   fmt = format = *argv;
   chopped = escape(fmt, 1, &len);              /* backslash interpretation */
   rval = end = 0;
   gargv = ++argv;
   for (;; ) {
      start = fmt;
      while (fmt < format + len) {
         if (fmt[0] == '%') {
            fwrite(start, 1, fmt - start, stdout);
            if (fmt[1] == '%') {
               /* %% prints a % */
               putchar('%');
               fmt += 2;
            } else {
               fmt = doformat(fmt, &rval);
               if (fmt == NULL)
                  return (1);
               end = 0;
            }
            start = fmt;
         } else
            fmt++;
      }

      if (end == 1) {
         ////warnx1("missing format character", NULL, NULL);
         return (1);
      }
      fwrite(start, 1, fmt - start, stdout);
      if (chopped || !*gargv)
         return (rval);
      /* Restart at the beginning of the format string. */
      fmt = format;
      end = 1;
   }
   /* NOTREACHED */
}


static char *
doformat(char *start, int *rval)
{
   static const char skip1[] = "#'-+ 0";
   static const char skip2[] = "0123456789";
   char *fmt;
   int fieldwidth, haveprec, havewidth, mod_ldbl, precision;
   char convch, nextch;

   fmt = start + 1;
   /* skip to field width */
   fmt += strspn(fmt, skip1);
   if (*fmt == '*') {
      if (getint(&fieldwidth))
         return (NULL);
      havewidth = 1;
      ++fmt;
   } else {
      havewidth = 0;

      /* skip to possible '.', get following precision */
      fmt += strspn(fmt, skip2);
   }
   if (*fmt == '.') {
      /* precision present? */
      ++fmt;
      if (*fmt == '*') {
         if (getint(&precision))
            return (NULL);
         haveprec = 1;
         ++fmt;
      } else {
         haveprec = 0;

         /* skip to conversion char */
         fmt += strspn(fmt, skip2);
      }
   } else
      haveprec = 0;
   if (!*fmt) {
      //warnx1("missing format character", NULL, NULL);
      return (NULL);
   }

   /*
    * Look for a length modifier.  POSIX doesn't have these, so
    * we only support them for floating-point conversions, which
    * are extensions.  This is useful because the L modifier can
    * be used to gain extra range and precision, while omitting
    * it is more likely to produce consistent results on different
    * architectures.  This is not so important for integers
    * because overflow is the only bad thing that can happen to
    * them, but consider the command  printf %a 1.1
    */
   if (*fmt == 'L') {
      mod_ldbl = 1;
      fmt++;
      if (!strchr("aAeEfFgG", *fmt)) {
         //warnx2("bad modifier L for %%%c", *fmt, NULL);
         return (NULL);
      }
   } else {
      mod_ldbl = 0;
   }

   convch = *fmt;
   nextch = *++fmt;
   *fmt = '\0';
   switch (convch) {
   case 'b': {
      size_t len;
      char *p=(char*)0;
      int getout;
      char* s;

      //p = (char*)strdup(getstr());
      s = (char*)getstr();
      if(s) {
         p= malloc( strlen(s)+1 );
         strcpy(p,s);
      }

      if (p == NULL) {
         //warnx2("%s", strerror(ENOMEM), NULL);
         return (NULL);
      }
      getout = escape(p, 0, &len);
      *(fmt - 1) = 's';
      PF(start, p);
      *(fmt - 1) = 'b';

      free(p);

      if (getout)
         return (fmt);
      break;
   }
   case 'c': {
      char p;

      p = getchr();
      PF(start, p);
      break;
   }
   case 's': {
      const char *p;

      p = getstr();
      PF(start, p);
      break;
   }
   case 'd': case 'i': case 'o': case 'u': case 'x': case 'X': {
      char *f;
      intmax_t val;
      uintmax_t uval;
      int signedconv;

      signedconv = (convch == 'd' || convch == 'i');
      if ((f = mknum(start, convch)) == NULL)
         return (NULL);
      if (getnum(&val, &uval, signedconv))
         *rval = 1;
      if (signedconv)
         PF(f, val);
      else
         PF(f, uval);
      break;
   }
   case 'e': case 'E':
   case 'f': case 'F':
   case 'g': case 'G':
   case 'a': case 'A': {
      long double p;

      if (getfloating(&p, mod_ldbl))
         *rval = 1;
      if (mod_ldbl)
         PF(start, p);
      else
         PF(start, (double)p);
      break;
   }
   default:
      //warnx2("illegal format character %c", convch, NULL);
      return (NULL);
   }
   *fmt = nextch;
   return (fmt);
}

static char *
mknum(char *str, int ch)
{
   static char *copy;
   static size_t copy_size;
   char *newcopy;
   size_t len, newlen;

   len = strlen(str) + 2;
   if (len > copy_size) {
      newlen = ((len + 1023) >> 10) << 10;
#ifdef SHELL
      if ((newcopy = ckrealloc(copy, newlen)) == NULL)
#else
      if ((newcopy = realloc(copy, newlen)) == NULL)
#endif
      {
         //warnx2("%s", strerror(ENOMEM), NULL);
         return (NULL);
      }
      copy = newcopy;
      copy_size = newlen;
   }

   memmove(copy, str, len - 3);
   copy[len - 3] = 'j';
   copy[len - 2] = ch;
   copy[len - 1] = '\0';
   return (copy);
}

static int
escape(char *fmt, int percent, size_t *len)
{
   char *save, *store;
   int value, c;

   for (save = store = fmt; (c = *fmt); ++fmt, ++store) {
      if (c != '\\') {
         *store = c;
         continue;
      }
      switch (*++fmt) {
      case '\0':                        /* EOS, user error */
         *store = '\\';
         *++store = '\0';
         *len = store - save;
         return (0);
      case '\\':                        /* backslash */
      case '\'':                        /* single quote */
         *store = *fmt;
         break;
      case 'a':                         /* bell/alert */
         *store = '\a';
         break;
      case 'b':                         /* backspace */
         *store = '\b';
         break;
      case 'c':
         *store = '\0';
         *len = store - save;
         return (1);
      case 'f':                         /* form-feed */
         *store = '\f';
         break;
      case 'n':                         /* newline */
         *store = '\n';
         break;
      case 'r':                         /* carriage-return */
         *store = '\r';
         break;
      case 't':                         /* horizontal tab */
         *store = '\t';
         break;
      case 'v':                         /* vertical tab */
         *store = '\v';
         break;
      /* octal constant */
      case '0': case '1': case '2': case '3':
      case '4': case '5': case '6': case '7':
         for (c = *fmt == '0' ? 4 : 3, value = 0;
              c-- && *fmt >= '0' && *fmt <= '7'; ++fmt) {
            value <<= 3;
            value += *fmt - '0';
         }
         --fmt;
         if (percent && value == '%') {
            *store++ = '%';
            *store = '%';
         } else
            *store = value;
         break;
      default:
         *store = *fmt;
         break;
      }
   }
   *store = '\0';
   *len = store - save;
   return (0);
}

static int
getchr(void)
{
   if (!*gargv)
      return ('\0');
   return ((int)**gargv++);
}

static const char *
getstr(void)
{
   if (!*gargv)
      return ("");
   return (*gargv++);
}

static int
getint(int *ip)
{
   intmax_t val;
   uintmax_t uval;
   int rval;

   if (getnum(&val, &uval, 1))
      return (1);
   rval = 0;
   if (val < INT_MIN || val > INT_MAX) {
      //warnx3("%s: %s", *gargv, strerror(ERANGE));
      rval = 1;
   }
   *ip = (int)val;
   return (rval);
}

static int
getnum(intmax_t *ip, uintmax_t *uip, int signedconv)
{
   char *ep=(char*)0;
   int rval;

   if (!*gargv) {
      *ip = 0;
      return (0);
   }
   if (**gargv == '"' || **gargv == '\'') {
      if (signedconv)
         *ip = asciicode();
      else
         *uip = asciicode();
      return (0);
   }
   rval = 0;
   //errno = 0;
   /*
        if (signedconv)
                *ip = strtoimax(*gargv, &ep, 0);
        else
                *uip = strtoumax(*gargv, &ep, 0);
        if (ep == *gargv) {
                //warnx2("%s: expected numeric value", *gargv, NULL);
                rval = 1;
        }
        else if (*ep != '\0') {
                //warnx2("%s: not completely converted", *gargv, NULL);
                rval = 1;
        }
        if (errno == ERANGE) {
                //warnx3("%s: %s", *gargv, strerror(ERANGE));
                rval = 1;
        }
   */
   ++gargv;
   return (rval);
}

static int
getfloating(long double *dp, int mod_ldbl)
{
   char *ep;
   int rval;

   if (!*gargv) {
      *dp = 0.0;
      return (0);
   }
   if (**gargv == '"' || **gargv == '\'') {
      *dp = asciicode();
      return (0);
   }
   rval = 0;
   //errno = 0;
   if (mod_ldbl)
      *dp = strtod(*gargv, &ep);
   else
      *dp = strtod(*gargv, &ep);
   if (ep == *gargv) {
      //warnx2("%s: expected numeric value", *gargv, NULL);
      rval = 1;
   } else if (*ep != '\0') {
      //warnx2("%s: not completely converted", *gargv, NULL);
      rval = 1;
   }
   /*
        if (errno == ERANGE) {
                //warnx3("%s: %s", *gargv, strerror(ERANGE));
                rval = 1;
        }
   */
   ++gargv;
   return (rval);
}

static int
asciicode(void)
{
   int ch;

   ch = **gargv;
   if (ch == '\'' || ch == '"')
      ch = (*gargv)[1];
   ++gargv;
   return (ch);
}

static void
usage(void)
{
   (void)fprintf(stderr, "usage: printf format [arguments ...]\n");
}


/*============================================
| End of Source  : printf.c
==============================================*/
