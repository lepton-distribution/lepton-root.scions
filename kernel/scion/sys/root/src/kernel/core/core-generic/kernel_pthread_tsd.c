/*
The contents of this file are subject to the Mozilla Public License Version 1.1
(the "License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at http://www.mozilla.org/MPL/

Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for the
specific language governing rights and limitations under the License.

The Original Code is Lepton.

The Initial Developer of the Original Code is Philippe Le Boulanger.
Portions created by Philippe Le Boulanger are Copyright (C) 2016 <lepton.phlb@gmail.com>.
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

/*===========================================
Includes
=============================================*/

#include <stdlib.h>
#include <stdint.h>

#include "kernel/core/limits.h"
#include "kernel/core/errno.h"
#include "kernel/core/signal.h"
#include "kernel/core/kernel_pthread.h"
#include "kernel/core/kernel.h"
#include "kernel/core/process.h"
#include "kernel/core/system.h"

#include "kernel/core/env.h"
#include "kernel/core/malloc.h"

/*===========================================
Global Declaration
=============================================*/






/*===========================================
Implementation
=============================================*/


/*--------------------------------------------
| Name: kernel_pthread_key_create
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int kernel_pthread_key_create(kernel_pthread_t* kernel_pthread, kernel_pthread_key_t *key, void(*destr_function) (void *)) {
   pid_t pid = kernel_pthread->pid;
   // if no process, it's a stand alone kernel pthread, return -1. not supported.
   if (kernel_pthread->pid <= 0)
      return -1;

   //enter tsd process mutex
   kernel_pthread_mutex_lock(&process_lst[pid]->thread_specfic_data_mutex);
   
   //allocate tsd vector key 
   unsigned char i;
   unsigned char d;
   kernel_pthread_key_t _key = 0;
   for (i = 0; i<PTHREAD_KEYS_MAX; i++) {
      for (d = 0; d<(sizeof(tsd_keys_vector_t)*8); d++) {
         if (!((process_lst[pid]->thread_specfic_data_keys_vector[i] >> d) & 0x01)) {
            _key = ((i << ((sizeof(tsd_keys_vector_t) + 2))) + d); //i*8+d
            //
            process_lst[pid]->thread_specfic_data_keys_vector[i] |= (0x01 << d);
            //
            process_lst[pid]->thread_specfic_data_destructor[_key] = destr_function;
            //
            *key = _key;
            //leave tsd process mutex
            kernel_pthread_mutex_unlock(&process_lst[pid]->thread_specfic_data_mutex);
            return 0;
         }
      }//end for d
   }//end for i
   
   //leave tsd process mutex
   kernel_pthread_mutex_unlock(&process_lst[pid]->thread_specfic_data_mutex);
   return -1;
}

/*--------------------------------------------
| Name: kernel_pthread_key_delete
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int kernel_pthread_key_delete(kernel_pthread_t* kernel_pthread, kernel_pthread_key_t key) {
   pid_t pid = kernel_pthread->pid;
   // if no process, it's a stand alone kernel pthread, return -1. not supported.
   if (kernel_pthread->pid <= 0)
      return -1;
   //check key range
   if (key >= PTHREAD_KEYS_MAX)
      return -1;
 
   //enter tsd process mutex
   kernel_pthread_mutex_lock(&process_lst[pid]->thread_specfic_data_mutex);
   //
   unsigned char i;
   unsigned char d;
   //
   i = (key >> (sizeof(tsd_keys_vector_t) + 2)); // i/8
   d = key - (i << (sizeof(tsd_keys_vector_t) + 2)); //i modulo 8
   //check key validity
   if (!((process_lst[pid]->thread_specfic_data_keys_vector[i] >> d) & 0x01)) {
      //leave tsd process mutex
      kernel_pthread_mutex_unlock(&process_lst[pid]->thread_specfic_data_mutex);
      //
      return -1;  //already deleted
   }
   
   //call destructor data???
   //No, See in POSIX open group specification: No such cleanup is done by pthread_key_delete()

   //deallocate tsd vector key
   process_lst[pid]->thread_specfic_data_keys_vector[i] &= (~(0x01 << d));

   //leave tsd process mutex
   kernel_pthread_mutex_unlock(&process_lst[pid]->thread_specfic_data_mutex);
   //
   return 0;
}

/*--------------------------------------------
| Name: kernel_pthread_setspecific
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int kernel_pthread_setspecific(kernel_pthread_t* kernel_pthread, kernel_pthread_key_t key, const void *pointer) {
   pid_t pid = kernel_pthread->pid;
   // if no process, it's a stand alone kernel pthread, return -1. not supported.
   if (kernel_pthread->pid <= 0)
      return -1;
   //check key range
   if (key >= PTHREAD_KEYS_MAX)
      return -1;

   //enter tsd process mutex
   kernel_pthread_mutex_lock(&process_lst[pid]->thread_specfic_data_mutex);
   //
   unsigned char i;
   unsigned char d;
   //
   i = (key >> (sizeof(tsd_keys_vector_t) + 2)); // i/8
   d = key - (i << (sizeof(tsd_keys_vector_t) + 2)); //i modulo 8
   //check key validity
   if (!((process_lst[pid]->thread_specfic_data_keys_vector[i] >> d) & 0x01)) {
      //leave tsd process mutex
      kernel_pthread_mutex_unlock(&process_lst[pid]->thread_specfic_data_mutex);
      //
      return -1;  //already deleted
   }

   //set pthread specific data

   //leave tsd process mutex
   kernel_pthread_mutex_unlock(&process_lst[pid]->thread_specfic_data_mutex);
   //
   return 0;
}

/*--------------------------------------------
| Name: kernel_pthread_getspecific
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
void * kernel_pthread_getspecific(kernel_pthread_t* kernel_pthread,kernel_pthread_key_t key) {
   pid_t pid = kernel_pthread->pid;
   // if no process, it's a stand alone kernel pthread, return -1. not supported.
   if (kernel_pthread->pid <= 0)
      return (void *)0;
   //check key range
   if (key >= PTHREAD_KEYS_MAX)
      return (void *)0;

   //enter tsd process mutex
   kernel_pthread_mutex_lock(&process_lst[pid]->thread_specfic_data_mutex);
   //
   unsigned char i;
   unsigned char d;
   //
   i = (key >> (sizeof(tsd_keys_vector_t) + 2)); // i/8
   d = key - (i << (sizeof(tsd_keys_vector_t) + 2)); //i modulo 8
   //check key validity
   if (!((process_lst[pid]->thread_specfic_data_keys_vector[i] >> d) & 0x01)) {
      //leave tsd process mutex
      kernel_pthread_mutex_unlock(&process_lst[pid]->thread_specfic_data_mutex);
      //
      return (void*)0;  //already deleted
   }
   //leave tsd process mutex
   kernel_pthread_mutex_unlock(&process_lst[pid]->thread_specfic_data_mutex);
   //
   return kernel_pthread->specific_data_array[key];
   
}

/*--------------------------------------------
| Name: kernel_pthread_cleanup_specific
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int kernel_pthread_cleanup_specific(kernel_pthread_t *kernel_pthread) {
   pid_t pid = kernel_pthread->pid;
   // if no process, it's a stand alone kernel pthread, return -1. not supported.
   if (kernel_pthread->pid <= 0)
      return -1;

   //enter tsd process mutex
   kernel_pthread_mutex_lock(&process_lst[pid]->thread_specfic_data_mutex);
   //
   for (kernel_pthread_key_t _key = 0; _key < PTHREAD_KEYS_MAX; _key++) {
      //
      unsigned char i;
      unsigned char d;
      //
      i = (_key >> (sizeof(tsd_keys_vector_t) + 2)); // i/8
      d = _key - (i << (sizeof(tsd_keys_vector_t) + 2)); //i modulo 8
      //check key validity
      if (!((process_lst[pid]->thread_specfic_data_keys_vector[i] >> d) & 0x01)) {
         //not valid 
         continue;
      }
      //valid
      //leave tsd process mutex
      kernel_pthread_mutex_unlock(&process_lst[pid]->thread_specfic_data_mutex);
      //call destructor with specific data key 
      pfn_pthread_specific_data_destructor_t pfn_pthread_specific_data_destructor = process_lst[pid]->thread_specfic_data_destructor[_key];
      if(pfn_pthread_specific_data_destructor != (pfn_pthread_specific_data_destructor_t)0) {
         pfn_pthread_specific_data_destructor(kernel_pthread->specific_data_array[_key]);
      }
      //re-enter tsd process mutex
      kernel_pthread_mutex_lock(&process_lst[pid]->thread_specfic_data_mutex);
      // mark key entry as free
      kernel_pthread->specific_data_array[_key] = (void*)0;


   }
   //leave tsd process mutex
   kernel_pthread_mutex_unlock(&process_lst[pid]->thread_specfic_data_mutex);

   //
   return 0;
}


/*===========================================
End of Source kernel_pthread_tsd.c
=============================================*/