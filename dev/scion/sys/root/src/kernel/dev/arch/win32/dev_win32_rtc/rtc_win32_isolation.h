
/*
The contents of this file are subject to the Mozilla Public License Version 1.1
(the "License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at http://www.mozilla.org/MPL/

Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for the
specific language governing rights and limitations under the License.

The Original Code is Lepton.

The Initial Developer of the Original Code is Philippe Le Boulanger.
Portions created by Philippe Le Boulanger are Copyright (C) 2016. All Rights Reserved.

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
#ifndef __RTC_WIN32_ISOLATION_H__
#define __RTC_WIN32_ISOLATION_H__


/*============================================
| Includes
==============================================*/



/*============================================
| Declaration
==============================================*/

struct tm_win32_isolation
{
   int tm_sec;   // seconds after the minute - [0, 60] including leap second
   int tm_min;   // minutes after the hour - [0, 59]
   int tm_hour;  // hours since midnight - [0, 23]
   int tm_mday;  // day of the month - [1, 31]
   int tm_mon;   // months since January - [0, 11]
   int tm_year;  // years since 1900
   int tm_wday;  // days since Sunday - [0, 6]
   int tm_yday;  // days since January 1 - [0, 365]
   int tm_isdst; // daylight savings time flag
};

int rtc_win32_isolation_get_gmtime(struct tm_win32_isolation* p_tm_w32_isol);


#endif

