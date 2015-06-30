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

//based on http://www.meduna.org/sw_ecosftpserver_en.html

#ifndef FTPD_H
#define FTPD_H

//#include <pkgconf/net_ftpserver.h>
//#include <limits.h>
//#include "cyg/kernel/kapi.h"

typedef int (*check_password_t)(const char *, const char *); /* 0 for OK, non-null for error */

typedef struct ftp_server {
	/* Configurable part, memset to 0 and then configure */
	unsigned int port;

	unsigned int max_nr_of_clients;
	unsigned int nr_of_clients;
	unsigned int firstport;	/* first and last ports to use, if we're */
	unsigned int lastport;	/* packet filter friendly. */

	check_password_t check_pwd;

	char chrootdir[PATH_MAX + 1];	/* root of the visible filesystem */

	/* Internal data, don't touch */
	pthread_mutex_t mutex;
	struct ftp_session *active_sessions;

} ftp_server_t;


//int ftpd_server( ftp_server_t * );

#endif
