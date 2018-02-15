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
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>

#include "kernel/core/kernelconf.h"

#include "kernel/core//errno.h"
#include "kernel/core/types.h"
#include "kernel/core/interrupt.h"
#include "kernel/core/kernel.h"

#include "kernel/core/kernel_pthread_mutex.h"
#include "kernel/core/kernel_sem.h"
#include "kernel/core/kernel_ring_buffer.h"
#include "kernel/core/kernel_mqueue.h"

/*===========================================
Global Declaration
=============================================*/


/*===========================================
Implementation
=============================================*/

/*-------------------------------------------
| Name: kernel_mqueue_get_timedwait
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int kernel_mqueue_get_timedwait(kernel_mqueue_t* kernel_mqueue, void* buf, int size, const struct timespec * abs_timeout){
   kernel_mqueue_header_t kernel_mqueue_header={0};
   kernel_ring_buffer_attr_t kernel_ring_buffer_attr;
   
   while(1){
      //
      kernel_pthread_mutex_lock(&kernel_mqueue->mutex);
      //check data available
      kernel_ring_buffer_get_attr(&kernel_mqueue->kernel_ring_buffer,&kernel_ring_buffer_attr);
      //data available?
      if(kernel_ring_buffer_attr.data_sz>0){
         break;
      }
      //no data available
      kernel_pthread_mutex_unlock(&kernel_mqueue->mutex);
      //wait
      if(kernel_sem_timedwait(&kernel_mqueue->sem_get,0,abs_timeout)<0){
         return 0;
      }
      //   
   }
   
   //get header
   kernel_ring_buffer_read(&kernel_mqueue->kernel_ring_buffer,&kernel_mqueue_header,sizeof(kernel_mqueue_header));
   //
   if(kernel_mqueue_header.size>size){
      kernel_pthread_mutex_unlock(&kernel_mqueue->mutex);
      return -1;
   }
   //get message 
   kernel_ring_buffer_read(&kernel_mqueue->kernel_ring_buffer,buf,kernel_mqueue_header.size);

   //
   kernel_pthread_mutex_unlock(&kernel_mqueue->mutex);
   kernel_sem_post(&kernel_mqueue->sem_put);
   //
   return kernel_mqueue_header.size;
}

/*-------------------------------------------
| Name: kernel_mqueue_get
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int kernel_mqueue_get(kernel_mqueue_t* kernel_mqueue, void* buf, int size){
   return kernel_mqueue_get_timedwait(kernel_mqueue,buf,size,(const struct timespec*)0);
}

/*-------------------------------------------
| Name: kernel_mqueue_put
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int kernel_mqueue_put(kernel_mqueue_t* kernel_mqueue,void* buf, int size){
   kernel_mqueue_header_t kernel_mqueue_header={0};
   kernel_ring_buffer_attr_t kernel_ring_buffer_attr;
   //
   kernel_mqueue_header.size=size;
   //
   kernel_sem_wait(&kernel_mqueue->sem_put);
   kernel_pthread_mutex_lock(&kernel_mqueue->mutex);
   
   //
   kernel_ring_buffer_get_attr(&kernel_mqueue->kernel_ring_buffer,&kernel_ring_buffer_attr);
   //space available?
   if( (kernel_mqueue_header.size+sizeof(kernel_mqueue_header_t)) > kernel_ring_buffer_attr.space_sz){
      kernel_pthread_mutex_unlock(&kernel_mqueue->mutex);
      return -1;
   }
   
   //put message header
   kernel_ring_buffer_write(&kernel_mqueue->kernel_ring_buffer,&kernel_mqueue_header,sizeof(kernel_mqueue_header));
   //put message
   kernel_ring_buffer_write(&kernel_mqueue->kernel_ring_buffer,buf,size);
   
   //
   kernel_sem_post(&kernel_mqueue->sem_get);
   //
   kernel_pthread_mutex_unlock(&kernel_mqueue->mutex);
   //
   return kernel_mqueue_header.size;
}

/*-------------------------------------------
| Name: kernel_mqueue_isempty
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int kernel_mqueue_isempty(kernel_mqueue_t* kernel_mqueue){
   kernel_ring_buffer_attr_t kernel_ring_buffer_attr;
   kernel_ring_buffer_get_attr(&kernel_mqueue->kernel_ring_buffer,&kernel_ring_buffer_attr);
   //
   if(kernel_ring_buffer_attr.data_sz==0){
      return 1;
   }
   //
   return 0;
}

/*-------------------------------------------
| Name: kernel_mqueue_flush
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int kernel_mqueue_flush(kernel_mqueue_t* kernel_mqueue){
   //
   kernel_pthread_mutex_lock(&kernel_mqueue->mutex);
   //
   //remove all message. clear ring buffer
   kernel_ring_buffer_reset(&kernel_mqueue->kernel_ring_buffer);
   //
   kernel_pthread_mutex_unlock(&kernel_mqueue->mutex);
   //
   return 0;
}

/*-------------------------------------------
| Name: kernel_mqueue_init
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int kernel_mqueue_init(kernel_mqueue_t* kernel_mqueue, kernel_mqueue_attr_t* attr){
   //
   pthread_mutexattr_t  mutex_attr=0;
   //
   if(kernel_pthread_mutex_init(&kernel_mqueue->mutex,&mutex_attr)<0)
      return -1;
   //
   if(kernel_sem_init(&kernel_mqueue->sem_put,0,1)<0)
      return -1;
   //
   if(kernel_sem_init(&kernel_mqueue->sem_get,0,0)<0)
      return -1;
   //
   kernel_ring_buffer_init(&kernel_mqueue->kernel_ring_buffer,attr->buffer,attr->size);
   //
   return 0;
}

/*===========================================
End of Source kernel_io.c
=============================================*/