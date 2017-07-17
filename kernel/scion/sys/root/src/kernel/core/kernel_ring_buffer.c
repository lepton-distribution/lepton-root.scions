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

#include <stdarg.h>
#include <stdint.h>

#include "kernel/core/types.h"
#include "kernel/core/interrupt.h"
#include "kernel/core/syscall.h"
#include "kernel/core/kernel.h"
#include "kernel/core/process.h"

#include "kernel/core/kernel_ring_buffer.h"

/*===========================================
Global Declaration
=============================================*/




/*===========================================
Implementation
=============================================*/


/*--------------------------------------------
| Name: kernel_ring_buffer_init
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int kernel_ring_buffer_init(kernel_ring_buffer_t* p_kernel_ring_buffer, void *buffer, int16_t size){
  //
  if(p_kernel_ring_buffer==(kernel_ring_buffer_t*)(0))
    return -1;
 
  //
  if(buffer==(void*)0)
    return -1;
  //
  p_kernel_ring_buffer->p_buffer = (uint8_t*)buffer;
  p_kernel_ring_buffer->sz=size;
  p_kernel_ring_buffer->r=0;
  p_kernel_ring_buffer->w=0;
  //
  return 0;
}

/*--------------------------------------------
| Name: kernel_ring_buffer_read
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int kernel_ring_buffer_read_min(kernel_ring_buffer_t* p_kernel_ring_buffer,void *buffer,int16_t min_size,int16_t max_size){
   int16_t r;
   int16_t w;
   int16_t sz;
   int16_t available_sz;
   uint8_t* p_ring_buffer;
   int16_t size=max_size;
   
   //
   if(p_kernel_ring_buffer==(kernel_ring_buffer_t*)(0))
      return -1;
   
   //
   r= p_kernel_ring_buffer->r;
   w= p_kernel_ring_buffer->w;
   sz = p_kernel_ring_buffer->sz;
   
   p_ring_buffer =  (uint8_t*) p_kernel_ring_buffer->p_buffer;

   //available data
   if(r<=w)
      available_sz=(w-r);
   else
      available_sz=(sz-r)+w;

   //
   if (available_sz == 0)
      return -1;
   
   //
   if (available_sz>=min_size && available_sz<=max_size) {
      //copy all data available in user buffer
      size = available_sz; 
   }else if (available_sz>max_size){
      //copy all data available in user buffer
      size = max_size;
   }else{
      return -1;
   }
   
   //
   if( (r+size) <= sz ){
      memcpy(buffer,p_ring_buffer+r,size);
      r = ((r+size)==sz)? 0 : r+size;
   }else{
      memcpy(buffer,p_ring_buffer+r,sz-r);
      memcpy(((uint8_t*)buffer)+(sz-r),p_ring_buffer+(sz-r),size-(sz-r));
      r=size-(sz-r);
   }
   //
   p_kernel_ring_buffer->r=r;
   //
   return size;
}
   
/*--------------------------------------------
| Name: kernel_ring_buffer_read
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int kernel_ring_buffer_read(kernel_ring_buffer_t* p_kernel_ring_buffer,void *buffer,int16_t size){
   return kernel_ring_buffer_read_min(p_kernel_ring_buffer,buffer,0,size);
}

/*--------------------------------------------
| Name: kernel_ring_buffer_write
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int kernel_ring_buffer_write(kernel_ring_buffer_t* p_kernel_ring_buffer,const void *buffer,int16_t size){
   int16_t r;
   int16_t w;
   int16_t sz;
   int16_t free_sz;
   uint8_t* p_ring_buffer;
   
   //
   if(p_kernel_ring_buffer==(kernel_ring_buffer_t*)(0))
      return -1;
   
   //  
   r= p_kernel_ring_buffer->r;
   w= p_kernel_ring_buffer->w;
   sz = p_kernel_ring_buffer->sz;
   
   p_ring_buffer =  (uint8_t*) p_kernel_ring_buffer->p_buffer;
  
  //available size
  if(w<r)
    free_sz=(r-w); 
  else
    free_sz = (sz-w)+r;
   
  //    
  if(size>free_sz)
    return -1; //not enough space
  
   //
   if( (w+size) <= sz ){
      memcpy(p_ring_buffer+w,buffer,size);
      w = ((w+size)==sz)? 0 : w+size;
   }else{
      memcpy(p_ring_buffer+w,buffer,sz-w);
      memcpy(p_ring_buffer,((uint8_t*)buffer)+(sz-w),size-(sz-w));
      w=size-(sz-w);
   }
   //
   p_kernel_ring_buffer->w=w;
   //
   return size;
}

/*--------------------------------------------
| Name: kernel_ring_buffer_reset
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int kernel_ring_buffer_reset(kernel_ring_buffer_t* p_kernel_ring_buffer){
    p_kernel_ring_buffer->r = p_kernel_ring_buffer->w;
    return 0;
}

/*--------------------------------------------
| Name: kernel_ring_buffer_get_attr
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int kernel_ring_buffer_get_attr(kernel_ring_buffer_t* p_kernel_ring_buffer, kernel_ring_buffer_attr_t* attr){
   int16_t r;
   int16_t w;
   int16_t sz;
   int16_t available_data_sz;
   int16_t free_space_sz;
   
   //
   if(p_kernel_ring_buffer==(kernel_ring_buffer_t*)(0))
      return -1;
   
   //
   r= p_kernel_ring_buffer->r;
   w= p_kernel_ring_buffer->w;
   sz = p_kernel_ring_buffer->sz;
   
   //available data size
   if(r<=w)
      available_data_sz=(w-r);
   else
      available_data_sz=(sz-r)+w;
   
   //free space available size
   if(w<r)
      free_space_sz=(r-w); 
   else
      free_space_sz = (sz-w)+r;
   
   //
   attr->p_buffer=p_kernel_ring_buffer->p_buffer;
   attr->sz=sz;
   //
   attr->data_sz=available_data_sz;
   attr->space_sz=free_space_sz;
   attr->r=r;
   attr->w=w;
   
   return 0;   
}

/*===========================================
End of Source kernel_ring_buffer.c
=============================================*/
