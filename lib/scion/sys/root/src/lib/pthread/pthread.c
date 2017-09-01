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
#include <stdint.h>
#include <stdarg.h>

#include "kernel/core/kernelconf.h"
#include "kernel/core/signal.h"
#include "kernel/core/syscall.h"
#include "kernel/core/libstd.h"

#include "lib/pthread/pthread.h"


/*============================================
| Global Declaration
==============================================*/


/*============================================
| Implementation
==============================================*/
/*--------------------------------------------
| Name:        pthread_create
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,void *(*start_routine)(void*), void *arg){
   pthread_create_t pthread_create_dt;

   pthread_create_dt.arg             = arg;
   pthread_create_dt.attr            = (pthread_attr_t *)attr;
   pthread_create_dt.start_routine   = start_routine;

   __mk_syscall(_SYSCALL_PTHREAD_CREATE,pthread_create_dt);

   if(pthread_create_dt.ret<0)
      return -1;
   //
   if( !(*thread= (pthread_t)pthread_create_dt.kernel_pthread) )
      return -1;

   return 0;
}


/*--------------------------------------------
| Name:        pthread_cancel
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int pthread_cancel(pthread_t thread){
   kernel_pthread_t* kernel_pthread = (kernel_pthread_t*)thread;

   //exit myself?
   if (thread == (pthread_t)kernel_pthread_self()) {
      pthread_exit((void*)0);
      // bye!
   }
   //
   return pthread_kill((pthread_t)kernel_pthread, SIGTHRKLL);
}

/*--------------------------------------------
| Name:        pthread_exit
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
void pthread_exit(void *value_ptr){
   kernel_pthread_t* kernel_pthread = kernel_pthread_self();
   //
   if (process_lst[kernel_pthread->pid]->pthread_ptr != kernel_pthread) {
      //secondary thread
      pthread_exit_t pthread_exit_dt;
      pthread_exit_dt.kernel_pthread = kernel_pthread;
      pthread_exit_dt.value_ptr = value_ptr;

      //
      kernel_pthread_exit_cleanup(pthread_exit_dt.kernel_pthread);
      //
      __mk_syscall(_SYSCALL_PTHREAD_EXIT, pthread_exit_dt);
   }else {
      //main thread
      _system_exit(0);
   }
   //
   return;
}

/*--------------------------------------------
| Name:        pthread_join
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int pthread_join(pthread_t pthread, void **value_ptr) {
   kernel_pthread_t* kernel_pthread = kernel_pthread_self();
   pthread_join_t pthread_join_dt;

   //join on himself?
   if (((kernel_pthread_t*)pthread) == kernel_pthread_self()) {
      return -1;
   }
   //
   pthread_join_dt.kernel_pthread = (kernel_pthread_t*)pthread;
   pthread_join_dt.value_ptr = value_ptr;
   //
   __mk_syscall(_SYSCALL_PTHREAD_JOIN, pthread_join_dt);
   //
   return  pthread_join_dt.ret;
}

/*--------------------------------------------
| Name:        pthread_once
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int pthread_once(pthread_once_t *once_control, void(*init_routine)(void)) {
   kernel_pthread_t* kernel_pthread = kernel_pthread_self();
   pid_t pid = kernel_pthread->pid;
   //
   kernel_pthread_mutex_lock(&process_lst[pid]->thread_once_mutex);
   //
   if (*once_control == PTHREAD_ONCE_INIT) {
      //call init routine
      init_routine();  
      //
      *once_control = (pthread_once_t)0;
   }
   //
   kernel_pthread_mutex_unlock(&process_lst[pid]->thread_once_mutex);
   //
   return 0;
}

/*--------------------------------------------
| Name:        pthread_self
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
pthread_t pthread_self(void){
   return (pthread_t)kernel_pthread_self();
}



/*--------------------------------------------
| Name:        pthread_key_create
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int pthread_key_create(kernel_pthread_key_t *key, void(*destr_function) (void *)) {
   return kernel_pthread_key_create(kernel_pthread_self, key, destr_function);
}

/*--------------------------------------------
| Name:        pthread_key_delete
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int pthread_key_delete(kernel_pthread_key_t key) {
   return kernel_pthread_key_delete(kernel_pthread_self(), key);
}

/*--------------------------------------------
| Name:        pthread_setspecific
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int pthread_setspecific(kernel_pthread_key_t key, const void *pointer) {
   return kernel_pthread_setspecific(kernel_pthread_self(), key, pointer);
}

/*--------------------------------------------
| Name:        pthread_getspecific
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
void * pthread_getspecific(kernel_pthread_key_t key) {
   return kernel_pthread_getspecific(kernel_pthread_self(), key);
}

/*============================================
| End of Source  : pthread.c
==============================================*/
