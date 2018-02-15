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
#ifndef __DEV_BQ27510_G3_H__
#define __DEV_BQ27510_G3_H__


/*============================================
| Includes
==============================================*/


/*============================================
| Declaration
==============================================*/

#define  IOCTL_CMD_BQ27510_RESET  (IOCTL_CMD_PWR_OFFSET+0)
#define  IOCTL_CMD_BQ27510_CFGCHK (IOCTL_CMD_PWR_OFFSET+1)
#define  IOCTL_CMD_BQ27510_CFGWR  (IOCTL_CMD_PWR_OFFSET+2)


/*
 * SBS Commands for DF access - these are pretty standard
 * So, no need to go in the command array
 */
#define BLOCK_DATA_CLASS      0x3E
#define DATA_BLOCK			   0x3F
#define BLOCK_DATA			   0x40
#define BLOCK_DATA_CHECKSUM	0x60
#define BLOCK_DATA_CONTROL		0x61


#define DATA_TYPE_U1   1  // uint8_t
#define DATA_TYPE_I1   2  // int8_t

#define DATA_TYPE_H1   3  // uint8_t

#define DATA_TYPE_U2   4 // uint16_t
#define DATA_TYPE_I2   5 // int16_t

#define DATA_TYPE_S   6 // string


// set aligment on 1 byte
#pragma pack (push,1)
//
typedef struct data_flash_register_st { 
	uint8_t  subclass;
	uint8_t  offset;
	uint8_t  type;
   uint8_t  length;
	int32_t value;
   const char* comment;
}data_flash_register_t;
//
#pragma pack (pop)
// restore previous alignment


typedef struct dev_bq27510_g3_info_st {
   uint8_t i2c_addr;
   uint16_t chip_id;
   //
   desc_t desc_r;
   desc_t desc_w;
   //
   int  data_flash_register_size;
   const data_flash_register_t* p_data_flash_register;
   //
   kernel_pthread_mutex_t mutex;
}dev_bq27510_g3_info_t;


//
int dev_bq27510_g3_x_load(dev_bq27510_g3_info_t * p_bq27510_g3_info);
int dev_bq27510_g3_x_open(desc_t desc, int o_flag,dev_bq27510_g3_info_t * p_bq27510_g3_info);
//
int dev_bq27510_g3_x_close(desc_t desc);
int dev_bq27510_g3_x_read(desc_t desc, char* buf,int cb);
int dev_bq27510_g3_x_write(desc_t desc, const char* buf,int cb);
int dev_bq27510_g3_x_seek(desc_t desc,int offset,int origin);
int dev_bq27510_g3_x_ioctl(desc_t desc,int request,va_list ap);


#endif //__DEV_BQ27510_G3_H__
