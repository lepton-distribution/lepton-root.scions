/*
The contents of this file are subject to the Mozilla Public License Version 1.1
(the "License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at http://www.mozilla.org/MPL/

Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for the
specific language governing rights and limitations under the License.

The Original Code is Lepton.

The Initial Developer of the Original Code is Philippe Le Boulanger.
Portions created by Philippe Le Boulanger are Copyright (C) 2011 <lepton.phlb@gmail.com>.
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
#ifndef __KERNEL_RING_BUFFER_H__
#define __KERNEL_RING_BUFFER_H__


/*============================================
| Includes
==============================================*/


/*============================================
| Declaration
==============================================*/

typedef struct kernel_ring_buffer_st{
  uint8_t*  p_buffer;
  uint32_t  options;
  int16_t sz;
  int16_t r;
  int16_t w;
}kernel_ring_buffer_t;
  
typedef struct kernel_ring_buffer_attr_st{
  uint8_t*  p_buffer;
  int16_t sz;
  //
  uint32_t  options;
  //
  int16_t data_sz;
  int16_t space_sz;
  int16_t r;
  int16_t w;
}kernel_ring_buffer_attr_t;

//
int kernel_ring_buffer_read_min(kernel_ring_buffer_t* p_kernel_ring_buffer,void *buffer,int16_t max_size, int16_t min_size);

int kernel_ring_buffer_init(kernel_ring_buffer_t* p_kernel_ring_buffer,void *buffer,int16_t size);
int kernel_ring_buffer_read(kernel_ring_buffer_t* p_kernel_ring_buffer,void *buffer,int16_t size);

int kernel_ring_buffer_write(kernel_ring_buffer_t* p_kernel_ring_buffer,const void *buffer,int16_t size);
int kernel_ring_buffer_get_attr(kernel_ring_buffer_t* p_kernel_ring_buffer, kernel_ring_buffer_attr_t* attr);

int kernel_ring_buffer_reset(kernel_ring_buffer_t* p_kernel_ring_buffer);

#define __kernel_ring_buffer_is_empty(__kernel_ring_buffer__) ((__kernel_ring_buffer__).r==(__kernel_ring_buffer__).w?1:0)
#define __kernel_ring_buffer_is_not_empty(__kernel_ring_buffer__) ((__kernel_ring_buffer__).r!=(__kernel_ring_buffer__).w?1:0)

//vrb_is_empty(3), vrb_is_full(3), vrb_is_not_empty(3), vrb_is_not_full(3),
#endif
