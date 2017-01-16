/*
The contents of this file are subject to the Mozilla Public License Version 1.1
(the "License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at http://www.mozilla.org/MPL/

Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for the
specific language governing rights and limitations under the License.

The Original Code is Lepton.

The Initial Developer of the Original Code is Philippe Le Boulanger.
Portions created by Philippe Le Boulanger are Copyright (C) 2011. All Rights Reserved.

Alternatively, the contents of this file may be used under the terms of the eCos GPL license
(the  [eCos GPL] License), in which case the provisions of [eCos GPL] License are applicable
instead of those above. If you wish to allow use of your version of this file only under the
terms of the [eCos GPL] License and not to allow others to use your version of this file under
the MPL, indicate your decision by deleting  the provisions above and replace
them with the notice and other provisions required by the [eCos GPL] License.
If you do not delete the provisions above, a recipient may use your version of this file under
either the MPL or the [eCos GPL] License."
*/


/*===========================================
Includes
=============================================*/

#include <windows.h>
#include <time.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>


#include "kernel/dev/arch/win32/dev_win32_rtc/rtc_win32_isolation.h"
/*===========================================
Global Declaration
=============================================*/


/*===========================================
Implementation
=============================================*/

/*-------------------------------------------
| Name:rtc_win32_isolation_get_gmtime
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int rtc_win32_isolation_get_gmtime(struct tm_win32_isolation* p_tm_w32_isol) {
   struct tm *newtime;
   long ltime;
   //
   time(&ltime);
   /* Obtain coordinated universal time: */
   newtime = gmtime(&ltime);
   //
   return 0;
}


/*============================================
| End of Source  : rtc_win32_isolation.c
==============================================*/