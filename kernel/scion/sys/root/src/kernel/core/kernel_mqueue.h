/*
The contents of this file are subject to the Mozilla Public License Version 1.1
(the "License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at http://www.mozilla.org/MPL/

Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for the
specific language governing rights and limitations under the License.

The Original Code is Lepton.

The Initial Developer of the Original Code is Philippe Le Boulanger.
Portions created by Philippe Le Boulanger are Copyright (C) 2017 <lepton.phlb@gmail.com>.
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


/*============================================
| Compiler Directive
==============================================*/
#ifndef __KERNEL_MQUEUE_H__
#define __KERNEL_MQUEUE_H__


/*============================================
| Includes
==============================================*/




/*============================================
| Declaration
==============================================*/

typedef struct kernel_mqueue_header_st{
   uint16_t size;
}kernel_mqueue_header_t;

//
typedef struct kernel_mqueue_st{
      kernel_pthread_mutex_t  mutex;
      kernel_sem_t            sem_put;
      kernel_sem_t            sem_get;
      kernel_ring_buffer_t    kernel_ring_buffer;
}kernel_mqueue_t;

//
typedef struct kernel_mqueue_attr_st{
   int size;
   uint8_t* buffer;
}kernel_mqueue_attr_t;

//
int kernel_mqueue_isempty(kernel_mqueue_t* kernel_mqueue);
//
int kernel_mqueue_put(kernel_mqueue_t* kernel_mqueue,void* buf, int size);
int kernel_mqueue_get_timedwait(kernel_mqueue_t* kernel_mqueue, void* buf, int size, const struct timespec * abs_timeout);
int kernel_mqueue_get(kernel_mqueue_t* kernel_mqueue,void* buf, int size);
int kernel_mqueue_flush(kernel_mqueue_t* kernel_mqueue);
int kernel_mqueue_init(kernel_mqueue_t* kernel_mqueue, kernel_mqueue_attr_t* attr);


#endif






