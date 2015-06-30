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

/*===========================================
Compiler Directive
=============================================*/
#ifndef _RTTIMER_H_
#define _RTTIMER_H_


/*===========================================
Includes
=============================================*/
#include "kernel/core/kal.h"


/*===========================================
Declaration
=============================================*/
#if defined(__KERNEL_UCORE_EMBOS)
typedef void (*_tmr_func_t)(void);
typedef _tmr_func_t tmr_func_t;
typedef OS_TIMER tmr_t;

#elif defined(__KERNEL_UCORE_FREERTOS)
typedef void (*_tmr_func_t)(void);
typedef _tmr_func_t tmr_func_t;
typedef xTimerHandle tmr_t;

#elif defined(__KERNEL_UCORE_ECOS)
typedef cyg_handle_t alrm_hdl_t;
typedef cyg_alarm alrm_t;
typedef void (*_tmr_func_t)(alrm_hdl_t alarm_handle, cyg_addrword_t data );
typedef _tmr_func_t tmr_func_t;
typedef struct tmr_st {
   alrm_hdl_t alarm_hdl;
   alrm_t alarm_obj;
}tmr_t;

#elif defined(USE_KERNEL_STATIC)
typedef void (*_tmr_func_t)(void);
typedef _tmr_func_t tmr_func_t;
typedef int tmr_t;

#endif


typedef struct rttmr_attr_st {
   time_t tm_msec; //delay
   tmr_func_t func;
#if defined __KERNEL_UCORE_ECOS
   cyg_addrword_t data;
#endif
}rttmr_attr_t;

int rttmr_create(tmr_t* tmr,rttmr_attr_t* rttmr_attr);
int rttmr_start(tmr_t* tmr);
int rttmr_stop(tmr_t* tmr);
int rttmr_restart(tmr_t* tmr);
int rttmr_delete(tmr_t* tmr);

#endif
