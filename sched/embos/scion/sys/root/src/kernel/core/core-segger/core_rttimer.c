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
Includes
=============================================*/
#include <stdint.h>

#include "kernel/core/core_rttimer.h"
#include "kernel/core/interrupt.h"

/*===========================================
Global Declaration
=============================================*/


/*===========================================
Implementation
=============================================*/

/*-------------------------------------------
| Name:rttmr_create
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int rttmr_create(tmr_t* tmr,rttmr_attr_t* rttmr_attr){
   if(!tmr || !rttmr_attr)
      return -1;
#ifdef __KERNEL_UCORE_EMBOS
   OS_CreateTimer(tmr,rttmr_attr->func,rttmr_attr->tm_msec);
#endif
   return 0;
}

/*-------------------------------------------
| Name:rttmr_start
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int rttmr_start(tmr_t* tmr){
   if(!tmr)
      return -1;
#ifdef __KERNEL_UCORE_EMBOS
   OS_StartTimer(tmr);
#endif
   return 0;
}

/*-------------------------------------------
| Name:rttmr_stop
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int rttmr_stop(tmr_t* tmr){
   if(!tmr)
      return -1;
#ifdef __KERNEL_UCORE_EMBOS
   OS_StopTimer(tmr);
#endif
   return 0;
}

/*-------------------------------------------
| Name:rttmr_restart
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int rttmr_restart(tmr_t* tmr){
   if(!tmr)
      return -1;
#ifdef __KERNEL_UCORE_EMBOS
   OS_RetriggerTimer(tmr);
#endif
   return 0;
}

/*-------------------------------------------
| Name:rttmr_delete
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int rttmr_delete(tmr_t* tmr){
   if(!tmr)
      return -1;
#ifdef __KERNEL_UCORE_EMBOS
   OS_DeleteTimer(tmr);
#endif
   return 0;
}




/*===========================================
End of Sourcerttimer.c
=============================================*/
