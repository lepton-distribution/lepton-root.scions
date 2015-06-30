/*
 * Copyright (c) 2004-2009 Sergey Lyubka
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */
/*
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
*/

#include "mongoose.h"

#if 0
//_WIN32
   #include <windows.h>
   #include <winsvc.h>
   #define DIRSEP                       '\\'
   #define      snprintf                _snprintf
   #if !defined(__LCC__)
      #define   strdup(x)               _strdup(x)
   #endif /* !MINGW */
   #define      sleep(x)                Sleep((x) * 1000)
#else
//#include <sys/wait.h>
//#include <unistd.h>		/* For pause() */

   #include <stdlib.h>
   #include <string.h>
   #include "lib/libc/ctype/ctype.h"
   #include "kernel/core/system.h"
   #include "kernel/core/signal.h"
   #include "kernel/core/wait.h"
   #include "kernel/core/devio.h"
   #include "kernel/core/stat.h"
   #include "kernel/core/statvfs.h"
   #include "kernel/core/dirent.h"
   #include "kernel/core/time.h"
   #include "kernel/core/fcntl.h"

   #include "kernel/core/select.h"
   #include "kernel/core/net/socket.h"
   #include "lib/libc/stdio/stdio.h"
   #include "lib/pthread/pthread.h"

   #include "kernel/core/libstd.h"

   #undef _WIN32


   #define   O_BINARY                        0
   #define  FILENAME_MAX 512    //8
   #define   DIRSEP                          '/'
   #define   IS_DIRSEP_CHAR(c)               ((c) == '/')
   #define   closesocket(a)                  close(a)
//#define	ERRNO	errno

   #define __func__ "not-avail"

   #define   SHUT_WR                 1

   #define   flockfile(x)
   #define   funlockfile(x)
   #define  fork           vfork
   #define  sleep(__t__) usleep((__t__*1000000L))
   #define  assert(__expr__) ((void)0)

//#define	snprintf		_snprintf

   #define EMBEDDED
   #define NO_GUI
   #define NO_SSL
   #define NO_SSI
   #define NO_AUTH
   #define NO_THREADS
   #define NO_SOCKLEN_T

   #define DIRSEP '/'
#endif /* _WIN32 */

static int exit_flag;                   /* Program termination flag	*/
static struct mg_context *ctx;          /* Mongoose context		*/

#if !defined(CONFIG_FILE)
   #define CONFIG_FILE             "mongoose.conf"
#endif /* !CONFIG_FILE */



int _snprintf(char *buf, size_t buflen, const char *fmt, ...)
{
   va_list ap;
   int n;

   if (buflen == 0)
      return (0);

   va_start(ap, fmt);
   n = vsnprintf(buf, buflen, fmt, ap);
   va_end(ap);

   if (n < 0 || (size_t) n >= buflen)
      n = buflen - 1;
   buf[n] = '\0';

   return (n);
}

static void
signal_handler(int sig_num)
{
#if !defined(_WIN32)
   if (sig_num == SIGCHLD) {
      do {
      } while (waitpid(-1, &sig_num, WNOHANG) > 0);
   } else
#endif /* !_WIN32 */
   {
      exit_flag = sig_num;
   }
}

/*
 * Show usage string and exit.
 */
static void
show_usage_and_exit(const char *prog)
{
   const struct mg_option  *o;

   (void) fprintf(stderr,
                  "Mongoose version %s (c) Sergey Lyubka\n"
                  "usage: %s [options] [config_file]\n", mg_version(), prog);

#if !defined(NO_AUTH)
   fprintf(stderr, "  -A <htpasswd_file> <realm> <user> <passwd>\n");
#endif /* NO_AUTH */

   o = mg_get_option_list();
   for (; o->name != NULL; o++) {
      (void) fprintf(stderr, "  -%s\t%s", o->name, o->description);
      if (o->default_value != NULL)
         fprintf(stderr, " (default: \"%s\")", o->default_value);
      fputc('\n', stderr);
   }
   exit(EXIT_FAILURE);
}

#if !defined(NO_AUTH)
/*
 * Edit the passwords file.
 */
static int
mg_edit_passwords(const char *fname, const char *domain,
                  const char *user, const char *pass)
{
   int ret = EXIT_SUCCESS, found = 0;
   char line[512], u[512], d[512], ha1[33], tmp[FILENAME_MAX];
   FILE    *fp = NULL, *fp2 = NULL;

   (void) snprintf(tmp, sizeof(tmp), "%s.tmp", fname);

   /* Create the file if does not exist */
   if ((fp = fopen(fname, "a+")) != NULL)
      (void) fclose(fp);

   /* Open the given file and temporary file */
   if ((fp = fopen(fname, "r")) == NULL) {
      fprintf(stderr, "Cannot open %s: %s", fname, strerror(errno));
      exit(EXIT_FAILURE);
   } else if ((fp2 = fopen(tmp, "w+")) == NULL) {
      fprintf(stderr, "Cannot open %s: %s", tmp, strerror(errno));
      exit(EXIT_FAILURE);
   }

   /* Copy the stuff to temporary file */
   while (fgets(line, sizeof(line), fp) != NULL) {

      if (sscanf(line, "%[^:]:%[^:]:%*s", u, d) != 2)
         continue;

      if (!strcmp(u, user) && !strcmp(d, domain)) {
         found++;
         mg_md5(ha1, user, ":", domain, ":", pass, NULL);
         (void) fprintf(fp2, "%s:%s:%s\n", user, domain, ha1);
      } else {
         (void) fprintf(fp2, "%s", line);
      }
   }

   /* If new user, just add it */
   if (!found) {
      mg_md5(ha1, user, ":", domain, ":", pass, NULL);
      (void) fprintf(fp2, "%s:%s:%s\n", user, domain, ha1);
   }

   /* Close files */
   (void) fclose(fp);
   (void) fclose(fp2);

   /* Put the temp file in place of real file */
   (void) remove(fname);
   (void) rename(tmp, fname);

   return (ret);
}
#endif /* NO_AUTH */

static void
set_temporary_opt_value(const struct mg_option *opts, char **vals,
                        const char *name, const char *val)
{
   int i;

   for (i = 0; opts[i].name != NULL; i++)
      if (!strcmp(opts[i].name, name)) {
         if (vals[i] != NULL)
            free(vals[i]);
         //lepton modif
         vals[i]=malloc(strlen(val)+1);
         strcpy(vals[i],val);
         //vals[i] = strdup(val);
         return;
      }
   (void) fprintf(stderr, "No such option: \"%s\"\n", name);
   exit(EXIT_FAILURE);
}

static void
process_command_line_arguments(struct mg_context *ctx, char *argv[])
{
   const struct mg_option *opts;
   const char      *config_file = CONFIG_FILE;
   char line[BUFSIZ], opt[BUFSIZ], *vals[100],
        val[BUFSIZ], path[FILENAME_MAX], *p;
   FILE            *fp;
   size_t i, line_no = 0;

   /* First find out, which config file to open */
   for (i = 1; argv[i] != NULL && argv[i][0] == '-'; i += 2)
      if (argv[i + 1] == NULL)
         show_usage_and_exit(argv[0]);

   if (argv[i] != NULL && argv[i + 1] != NULL) {
      /* More than one non-option arguments are given w*/
      show_usage_and_exit(argv[0]);
   } else if (argv[i] != NULL) {
      /* Just one non-option argument is given, this is config file */
      config_file = argv[i];
   } else {
      /* No config file specified. Look for one where binary lives */
      if ((p = strrchr(argv[0], DIRSEP)) != 0) {
         snprintf(path, sizeof(path), "%.*s%s",
                  (int) (p - argv[0]) + 1, argv[0], config_file);
         config_file = path;
      }
   }

   fp = fopen(config_file, "r");

   /* If config file was set in command line and open failed, exit */
   if (fp == NULL && argv[i] != NULL) {
      (void) fprintf(stderr, "cannot open config file %s: %s\n",
                     config_file, strerror(errno));
      exit(EXIT_FAILURE);
   }

   /* Reset temporary value holders */
   (void) memset(vals, 0, sizeof(vals));
   opts = mg_get_option_list();

   if (fp != NULL) {
      (void) printf("Loading config file %s\n", config_file);

      /* Loop over the lines in config file */
      while (fgets(line, sizeof(line), fp) != NULL) {

         line_no++;

         /* Ignore empty lines and comments */
         if (line[0] == '#' || line[0] == '\n')
            continue;

         if (sscanf(line, "%s %[^\n#]", opt, val) != 2) {
            fprintf(stderr, "%s: line %d is invalid\n",
                    config_file, (int) line_no);
            exit(EXIT_FAILURE);
         }

         set_temporary_opt_value(opts, vals, opt, val);
      }

      (void) fclose(fp);
   }

   /* Now pass through the command line options */
   for (i = 1; argv[i] != NULL && argv[i][0] == '-'; i += 2)
      set_temporary_opt_value(opts, vals, &argv[i][1], argv[i + 1]);

   /* Finally, call option setters */
   for (i = 0; opts[i].name != NULL; i++) {
      if (vals[i] != NULL) {
         if (mg_set_option(ctx, opts[i].name, vals[i]) != 1) {
            (void) fprintf(stderr, "Error setting "
                           "option \"%s\"\n", opts[i].name);
            exit(EXIT_FAILURE);
         }
         free(vals[i]);
      }
   }
}

#ifdef _WIN32
static SERVICE_STATUS ss;
static SERVICE_STATUS_HANDLE hStatus;
static char service_name[20];

static void WINAPI
ControlHandler(DWORD code)
{
   if (code == SERVICE_CONTROL_STOP || code == SERVICE_CONTROL_SHUTDOWN) {
      ss.dwWin32ExitCode = 0;
      ss.dwCurrentState = SERVICE_STOPPED;
   }

   SetServiceStatus(hStatus, &ss);
}

static void WINAPI
ServiceMain(void)
{
   char path[MAX_PATH], *p, *av[] = {"mongoose_service", NULL, NULL};
   struct mg_context *ctx;

   av[1] = path;

   ss.dwServiceType      = SERVICE_WIN32;
   ss.dwCurrentState     = SERVICE_RUNNING;
   ss.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;

   hStatus = RegisterServiceCtrlHandler(service_name, ControlHandler);
   SetServiceStatus(hStatus, &ss);

   GetModuleFileName(NULL, path, sizeof(path));

   if ((p = strrchr(path, DIRSEP)) != NULL)
      *++p = '\0';

   strcat(path, CONFIG_FILE);           /* woo ! */

   Sleep(3000);
   if ((ctx = mg_start()) != NULL) {
      process_command_line_arguments(ctx, av);

      while (ss.dwCurrentState == SERVICE_RUNNING)
         Sleep(1000);
      mg_stop(ctx);
   }

   ss.dwCurrentState  = SERVICE_STOPPED;
   ss.dwWin32ExitCode = (DWORD) -1;
   SetServiceStatus(hStatus, &ss);
}

static void
try_to_run_as_nt_service(void)
{
   static SERVICE_TABLE_ENTRY service_table[] = {
      {service_name, (LPSERVICE_MAIN_FUNCTION) ServiceMain},
      {NULL, NULL}
   };

   if (StartServiceCtrlDispatcher(service_table))
      exit(EXIT_SUCCESS);
}
#endif /* _WIN32 */

int
mongoosed_main(int argc, char *argv[])
{
   char dflt_port[5] = "80";
#if !defined(NO_AUTH)
   if (argc > 1 && argv[1][0] == '-' && argv[1][1] == 'A') {
      if (argc != 6)
         show_usage_and_exit(argv[0]);
      exit(mg_edit_passwords(argv[2], argv[3], argv[4],argv[5]));
   }
#endif /* NO_AUTH */

   if (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")))
      show_usage_and_exit(argv[0]);

#if defined(_WIN32)
   (void) sprintf(service_name, "Mongoose %s", mg_version());
   try_to_run_as_nt_service();
#endif /* _WIN32 */

#ifndef _WIN32
   (void) signal(SIGCHLD, signal_handler);
#endif /* _WIN32 */

   (void) signal(SIGTERM, signal_handler);
   (void) signal(SIGINT, signal_handler);

   if ((ctx = mg_start()) == NULL) {
      (void) printf("%s\n", "Cannot initialize Mongoose context");
      exit(EXIT_FAILURE);
   }

   process_command_line_arguments(ctx, argv);
   if (mg_get_option(ctx, "ports") == NULL &&
       mg_set_option(ctx, "ports", dflt_port /*"8080"*/) != 1)
      exit(EXIT_FAILURE);

   printf("Mongoose %s started on port(s) [%s], serving directory [%s]\n",
          mg_version(),
          mg_get_option(ctx, "ports"),
          mg_get_option(ctx, "root"));
   fflush(stdout);
   while (exit_flag == 0)
      sleep(1);

   (void) printf("Exiting on signal %d, "
                 "waiting for all threads to finish...", exit_flag);
   fflush(stdout);
   mg_stop(ctx);
   (void) printf("%s", " done.\n");

   return (EXIT_SUCCESS);
}
