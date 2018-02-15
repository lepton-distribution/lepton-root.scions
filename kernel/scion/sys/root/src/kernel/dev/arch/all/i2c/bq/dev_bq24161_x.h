/*
The contents of this file are subject to the Mozilla Public License Version 1.1
(the "License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at http://www.mozilla.org/MPL/

Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for the
specific language governing rights and limitations under the License.

The Original Code is Lepton.

The Initial Developer of the Original Code is Philippe Le Boulanger.
Portions created by Philippe Le Boulanger are Copyright (C) 2015 <lepton.phlb@gmail.com>.
All Rights Reserved.

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
#ifndef __DEV_BQ24161_H__
#define __DEV_BQ24161_H__


/*============================================
| Includes
==============================================*/


/*============================================
| Declaration
==============================================*/
#define  IOCTL_CMD_BQ24161_CFGWR          (0x01)
#define  IOCTL_CMD_BQ24161_STAT           (0x02)
#define  IOCTL_CMD_BQ24161_WTCHDG         (0x03)
#define  IOCTL_CMD_BQ24161_RSTCHRGCYCL    (0x04)
#define  IOCTL_CMD_BQ24161_STOPCHRG       (0x05)


//
#define BQ24161_CMD_NONE      ((uint8_t)(0))
#define BQ24161_CMD_CHRGSTOP  ((uint8_t)(1))
//
typedef struct dev_bq24161_register_st{
   uint8_t address;
   uint8_t value;
}dev_bq24161_register_t;

//
typedef struct dev_bq24161_info_st {
   uint8_t i2c_addr;
   //
   desc_t desc_r;
   desc_t desc_w;
   //
   uint8_t registers_list_size;
   dev_bq24161_register_t* p_registers_list;
   
   //
   uint8_t command;
   
   //
   kernel_pthread_t kernel_thread;
   //
   kernel_pthread_mutex_t mutex;
}dev_bq24161_info_t;


//
int dev_bq24161_x_load(dev_bq24161_info_t * p_bq24161_info);
int dev_bq24161_x_open(desc_t desc, int o_flag,dev_bq24161_info_t * p_bq24161_info);
//
int dev_bq24161_x_close(desc_t desc);
int dev_bq24161_x_read(desc_t desc, char* buf,int cb);
int dev_bq24161_x_write(desc_t desc, const char* buf,int cb);
int dev_bq24161_x_seek(desc_t desc,int offset,int origin);
int dev_bq24161_x_ioctl(desc_t desc,int request,va_list ap);


#endif //__DEV_BQ24161_H__
