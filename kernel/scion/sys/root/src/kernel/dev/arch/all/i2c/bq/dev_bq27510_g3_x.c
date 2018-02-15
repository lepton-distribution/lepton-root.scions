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


/*===========================================
Includes
=============================================*/
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "kernel/core/kernelconf.h"
#include "kernel/core/types.h"
#include "kernel/core/dirent.h"
#include "kernel/core/interrupt.h"
#include "kernel/core/kernel.h"
#include "kernel/core/system.h"
#include "kernel/core/fcntl.h"
#include "kernel/core/stat.h"
#include "kernel/core/ioctl.h"
#include "kernel/core/ioctl_i2c.h"
#include "kernel/core/ioctl_pwr.h"
#include "kernel/core/kernel_io.h"
#include "kernel/core/kernel_printk.h"

#include "kernel/fs/vfs/vfstypes.h"

#include "kernel/dev/arch/all/i2c/bq/dev_bq27510_g3_x.h"

   
/*===========================================
Global Declaration
=============================================*/
#define DEBUG_KERNEL_PRINTK
#define I2CADDR_ARGC 4

//
static unsigned char i2c_data[48] = {0};


/*===========================================
Implementation
=============================================*/
/*-------------------------------------------
| Name: xtoi
| Description:
| Parameters:
| Return Type:
| Comments: 
| See:
---------------------------------------------*/
static unsigned htoi(unsigned u)
{
	if (isdigit(u))
		return (u - '0'); 
	else if (islower(u))
		return (10 + u - 'a'); 
	else if (isupper(u))
		return (10 + u - 'A'); 
	return (16);
}

/*-------------------------------------------
| Name: swap_uint16
| Description:
| Parameters:
| Return Type:
| Comments: swap unsigned short
| See:
---------------------------------------------*/
static uint16_t swap_uint16( uint16_t val ) {
    return (val << 8) | (val >> 8 );
}

/*-------------------------------------------
| Name: swap_int16
| Description:
| Parameters:
| Return Type:
| Comments: Byte swap short
| See:
---------------------------------------------*/
static int16_t swap_int16( int16_t val ) {
    return (val << 8) | ((val >> 8) & 0xFF);
}

/*-------------------------------------------
| Name: checksum
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static uint8_t checksum(uint8_t *data){
	uint16_t sum = 0;
	int i;
	for (i = 0; i < 32; i++)
		sum += data[i];
   //
	sum &= 0xFF;
   //
	return 0xFF - sum;
}

/*-------------------------------------------
| Name: _i2c_transfer_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int _i2c_transfer_read(desc_t desc,unsigned char* data,int size){
   dev_bq27510_g3_info_t * p_bq27510_g3_info = (dev_bq27510_g3_info_t*)ofile_lst[desc].p;
   desc_t desc_i2c_read;
   //
   if(!p_bq27510_g3_info)
      return -1;
   //
   desc_i2c_read=ofile_lst[desc].desc_nxt[1];
   
   //i2c access
   kernel_io_ll_ioctl(desc_i2c_read,I2C_LOCK);
   
   //i2c addr
   kernel_io_ll_ioctl(desc_i2c_read,I2C_SLAVEADDR,p_bq27510_g3_info->i2c_addr);
   
   //
   if(kernel_io_ll_read(desc_i2c_read,data,size)<0){
      //i2c access
      kernel_io_ll_ioctl(desc_i2c_read,I2C_UNLOCK);
      return -1;
   }
   
   //i2c access
   kernel_io_ll_ioctl(desc_i2c_read,I2C_UNLOCK);
   //
   return size;
   
}

/*-------------------------------------------
| Name: _i2c_transfer_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int _i2c_transfer_write(desc_t desc,unsigned char* data,int size){
   dev_bq27510_g3_info_t * p_bq27510_g3_info = (dev_bq27510_g3_info_t*)ofile_lst[desc].p;
   desc_t desc_i2c_write;
   //
   if(!p_bq27510_g3_info)
      return -1;
   //
   desc_i2c_write=ofile_lst[desc].desc_nxt[1];
   
   //i2c access
   kernel_io_ll_ioctl(desc_i2c_write,I2C_LOCK);
   
   //i2c addr
   kernel_io_ll_ioctl(desc_i2c_write,I2C_SLAVEADDR,p_bq27510_g3_info->i2c_addr);
   
   //
   if(kernel_io_ll_write(desc_i2c_write,data,size)<0){
      //i2c access
      kernel_io_ll_ioctl(desc_i2c_write,I2C_UNLOCK);
      return -1;
   }
   
   //i2c access
    kernel_io_ll_ioctl(desc_i2c_write,I2C_UNLOCK);
   //
   return size;
}

/*-------------------------------------------
| Name: drv_i2c_bq275xx_read_control_value
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int32_t drv_i2c_bq275xx_read_control_value(desc_t desc, uint16_t command,uint16_t sub_command, uint16_t* value){
   unsigned char i2c_data_length = 0;
   volatile int i2c_error = 0;
   uint16_t* p_data = (uint16_t*)&i2c_data[0];
   
   // send command
   i2c_data_length= 3;
   i2c_data[0] = (command) & 0x00FF;//command address
   i2c_data[1] = (sub_command) & 0x00FF;//lsb sub command
   i2c_data[2] = (sub_command>>8) & 0x00FF;//msb sub command
   if(_i2c_transfer_write(desc,i2c_data,i2c_data_length)<0){
      i2c_error= -1;
      return -1;
   }
   
   //
   if(value==(uint16_t*)0){
      return 0;
   }
   
   //
   __kernel_usleep(2000); 
   
   //read result
   i2c_data_length= 1;
   i2c_data[0]=(command) & 0x00FF;//command address
   if(_i2c_transfer_write(desc,i2c_data,i2c_data_length)<0){
      i2c_error= -1;
      return -1;
   }
   // read command result
   i2c_data_length= 2;
   //
   memset(i2c_data,0,sizeof(i2c_data));
   //
   if(_i2c_transfer_read(desc,i2c_data,i2c_data_length)<0){
      i2c_error= -1;
      return -1;
   }
   //
   *value=*p_data;
   //
	return i2c_error;
}

/*-------------------------------------------
| Name: drv_i2c_bq275xx_read_value
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int32_t drv_i2c_bq275xx_read_value(desc_t desc, uint16_t command, uint8_t* value, unsigned int single){
   unsigned char i2c_data_length = 0;
   volatile int i2c_error = 0;
   uint8_t* p_data = (uint8_t*)&i2c_data[0];
   
   //read value
   i2c_data_length= 1;
   i2c_data[0]=(command) & 0x00FF;//command address
   if(_i2c_transfer_write(desc,i2c_data,i2c_data_length)<0){
      i2c_error= -1;
      return i2c_error;
   }
   // read command result
   if(single){
     i2c_data_length= 1;
   }else{
     i2c_data_length= 2;
   }
   //
   __kernel_usleep(2000); 
   //
   memset(i2c_data,0,sizeof(i2c_data));
   //
   if(_i2c_transfer_read(desc,i2c_data,i2c_data_length)<0){
      i2c_error= -1;
      return i2c_error;
   }
   //
   if(single){
      *value=*p_data;
   }else{   
     value[0]=p_data[0];
     value[1]=p_data[1];
   }
   //
	return i2c_error;
}

/*-------------------------------------------
| Name: drv_i2c_bq275xx_write_value
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int32_t drv_i2c_bq275xx_write_value(desc_t desc, uint32_t command, uint16_t value, unsigned int single){
   unsigned char i2c_data_length = 0;
   volatile int i2c_error = 0;
   
   //read value
   i2c_data_length= 1;
   i2c_data[0]=(command) & 0x00FF;//command address
   if(single){
      i2c_data[1]=(value) & 0x00FF;
      i2c_data_length+=1;
   }else{
      i2c_data[1]=(value) & 0x00FF;
      i2c_data[2]=(value>>8) & 0x00FF;
      i2c_data_length+=2;
   }
   //
   if(_i2c_transfer_write(desc,i2c_data,i2c_data_length)<0){
      i2c_error= -1;
      return i2c_error;
   }
	return 0;
}

/*-------------------------------------------
| Name: drv_i2c_bq275xx_read_block_data
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int32_t drv_i2c_bq275xx_read_block_data(desc_t desc, uint16_t command, uint8_t* data, uint8_t length){
   unsigned char i2c_data_length = 0;
   volatile int i2c_error = 0;
   
   //write command value
   i2c_data_length= 1;
   i2c_data[0]=(command) & 0x00FF;//command address
   if(_i2c_transfer_write(desc,i2c_data,i2c_data_length)<0){
      i2c_error= -1;
      return i2c_error;
   }
   //
   __kernel_usleep(2000); 
   // read data
   i2c_data_length= length;
   //
   memset(i2c_data,0,sizeof(i2c_data));
   //
   if(_i2c_transfer_read(desc,data,length)<0){
      i2c_error= -1;
      return i2c_error;
   }
   //
	return length;
}

/*-------------------------------------------
| Name: drv_i2c_bq275xx_write_block_data
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int32_t drv_i2c_bq275xx_write_block_data(desc_t desc, uint16_t command, uint8_t* data, uint8_t length){
   unsigned char i2c_address = 0;	  
   unsigned char i2c_data_length = 0;
   volatile int i2c_error = 0;
   
   //write command value and data
   i2c_data_length= 1;
   i2c_data[0]=(command) & 0x00FF;//command address
   //copy data
   memcpy(&i2c_data[1],data,length);
   //
   i2c_data_length= i2c_data_length + length;
   //
   if(_i2c_transfer_write(desc,i2c_data,i2c_data_length)<0){
      i2c_error= -1;
      return i2c_error;
   }
   //
	return length;
}

/*-------------------------------------------
| Name:  drv_i2c_bq275xx_dataflash_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int32_t drv_i2c_bq275xx_dataflash_read(desc_t desc,uint8_t *buf, uint8_t size,uint8_t subclass, uint8_t offset){
  //
   uint8_t cksum_calc, cksum;
   uint8_t blk_offset = offset >> 5;
   static uint8_t data[32]={0};

   //prepare read data block
	drv_i2c_bq275xx_write_value(desc,BLOCK_DATA_CONTROL, 0, 1);
	__kernel_usleep(5000);
   //
	drv_i2c_bq275xx_write_value(desc,BLOCK_DATA_CLASS, subclass, 1);
   __kernel_usleep(5000); 
   //
	drv_i2c_bq275xx_write_value(desc,DATA_BLOCK, blk_offset, 1);
   __kernel_usleep(5000); 
   
   //read data block
	drv_i2c_bq275xx_read_block_data(desc,BLOCK_DATA, data, 32);
   __kernel_usleep(5000);
   
   //check sum
	cksum_calc = checksum(data);
	drv_i2c_bq275xx_read_value(desc,BLOCK_DATA_CHECKSUM,(uint8_t*)&cksum,1);
   __kernel_usleep(5000);
  
   if(cksum_calc!=cksum)
     return - 1;
   
   //copy value
   memcpy(buf,&data[offset],size);
   //
   return size;
}

/*-------------------------------------------
| Name:  drv_i2c_bq275xx_dataflash_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int32_t drv_i2c_bq275xx_dataflash_write(desc_t desc,uint8_t subclass, uint8_t offset,uint8_t *buf, uint8_t size){
  //
   uint8_t cksum_calc, cksum;
   uint8_t blk_offset = offset >> 5;
   static uint8_t data[32]={0};

   //prepare read data block
	drv_i2c_bq275xx_write_value(desc,BLOCK_DATA_CONTROL, 0, 1);
	__kernel_usleep(5000); 
   //
	drv_i2c_bq275xx_write_value(desc,BLOCK_DATA_CLASS, subclass, 1);
   __kernel_usleep(5000); 
   //
	drv_i2c_bq275xx_write_value(desc,DATA_BLOCK, blk_offset, 1);
   __kernel_usleep(5000); 
   
   //read data block
	drv_i2c_bq275xx_read_block_data(desc,BLOCK_DATA, data, 32);
   __kernel_usleep(5000);
   
   //
	cksum_calc = checksum(data);
	drv_i2c_bq275xx_read_value(desc,BLOCK_DATA_CHECKSUM,(uint8_t*)&cksum,1);
   __kernel_usleep(5000);
   //
   if(cksum_calc!=cksum)
     return - 1;
   
   //copy new value
   memcpy(&data[offset],buf,size);
   
   //prepare write data block
	drv_i2c_bq275xx_write_value(desc,BLOCK_DATA_CONTROL, 0, 1);
	__kernel_usleep(5000); 
   //
	drv_i2c_bq275xx_write_value(desc,BLOCK_DATA_CLASS, subclass, 1);
   __kernel_usleep(5000); 
   //
	drv_i2c_bq275xx_write_value(desc,DATA_BLOCK, blk_offset, 1);
   __kernel_usleep(5000);
   
   //write data block
	drv_i2c_bq275xx_write_block_data(desc,BLOCK_DATA, data, 32);
   __kernel_usleep(5000);
   
   //calc new check sum
	cksum_calc = checksum(data);
	drv_i2c_bq275xx_write_value(desc,BLOCK_DATA_CHECKSUM,cksum_calc,1);
   __kernel_usleep(5000);
   
   //
   memset(data,0xff,32);
   //
   //prepare read data block for control
	drv_i2c_bq275xx_write_value(desc,BLOCK_DATA_CONTROL, 0, 1);
	__kernel_usleep(5000); 
   //
	drv_i2c_bq275xx_write_value(desc,BLOCK_DATA_CLASS, subclass, 1);
   __kernel_usleep(5000); 
   //
	drv_i2c_bq275xx_write_value(desc,DATA_BLOCK, blk_offset, 1);
   __kernel_usleep(5000);
   
   //read data block
	drv_i2c_bq275xx_read_block_data(desc,BLOCK_DATA, data, 32);
   __kernel_usleep(5000);
   
    //
	cksum_calc = checksum(data);
	drv_i2c_bq275xx_read_value(desc,BLOCK_DATA_CHECKSUM,(uint8_t*)&cksum,1);
   __kernel_usleep(5000);
   //
   if(cksum_calc!=cksum)
     return - 1;
   
   //compare value
   if(memcmp(&data[offset],buf,size)!=0)
      return -1;
   //
   return size;
}


/*-------------------------------------------
| Name:  drv_i2c_bq275xx_write_configuration
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int32_t drv_i2c_bq275xx_write_configuration(desc_t desc){
   dev_bq27510_g3_info_t * p_bq27510_g3_info = (dev_bq27510_g3_info_t*)ofile_lst[desc].p;
   
   //
   for(int i=0; i<p_bq27510_g3_info->data_flash_register_size;i++){
      
      #if defined(DEBUG_KERNEL_PRINTK)
         kernel_printk("battery: config %d \r\n",i);
      #endif
      //          
      switch(p_bq27510_g3_info->p_data_flash_register[i].type){
         //
         case DATA_TYPE_U1:
         case DATA_TYPE_I1:
         case DATA_TYPE_H1:{
            uint8_t val = p_bq27510_g3_info->p_data_flash_register[i].value;
            //
            if(drv_i2c_bq275xx_dataflash_write(desc, p_bq27510_g3_info->p_data_flash_register[i].subclass,
                                             p_bq27510_g3_info->p_data_flash_register[i].offset,
                                             (uint8_t*)&val,
                                             p_bq27510_g3_info->p_data_flash_register[i].length)<0){
               #if defined(DEBUG_KERNEL_PRINTK)
                  kernel_printk("battery: write config %s failed\r\n",p_bq27510_g3_info->p_data_flash_register[i].comment);
               #endif
            }
         }
         break;
         
         //
         case DATA_TYPE_U2:{
            uint16_t val_bigendian = swap_uint16(p_bq27510_g3_info->p_data_flash_register[i].value);
            //
            if(drv_i2c_bq275xx_dataflash_write(desc, p_bq27510_g3_info->p_data_flash_register[i].subclass,
                                             p_bq27510_g3_info->p_data_flash_register[i].offset,
                                             (uint8_t*)&val_bigendian,
                                             p_bq27510_g3_info->p_data_flash_register[i].length)<0){
               #if defined(DEBUG_KERNEL_PRINTK)
                  kernel_printk("battery: write config %s failed\r\n",p_bq27510_g3_info->p_data_flash_register[i].comment);
               #endif
            }
         }
         
         //
         case DATA_TYPE_I2:{
            int16_t val_bigendian = swap_int16(p_bq27510_g3_info->p_data_flash_register[i].value);
            //
            if(drv_i2c_bq275xx_dataflash_write(desc, p_bq27510_g3_info->p_data_flash_register[i].subclass,
                                             p_bq27510_g3_info->p_data_flash_register[i].offset,
                                             (uint8_t*)&val_bigendian,
                                             p_bq27510_g3_info->p_data_flash_register[i].length)<0){
               #if defined(DEBUG_KERNEL_PRINTK)
                  kernel_printk("battery: write config %s failed\r\n",p_bq27510_g3_info->p_data_flash_register[i].comment);
               #endif
            }
         }
         break;
         
         //
         case DATA_TYPE_S:{
           if(drv_i2c_bq275xx_dataflash_write(desc, p_bq27510_g3_info->p_data_flash_register[i].subclass,
                                             p_bq27510_g3_info->p_data_flash_register[i].offset,
                                             (uint8_t*)p_bq27510_g3_info->p_data_flash_register[i].value,
                                             p_bq27510_g3_info->p_data_flash_register[i].length)<0){
               #if defined(DEBUG_KERNEL_PRINTK)
                    kernel_printk("battery: write config %s failed:%s\r\n",p_bq27510_g3_info->p_data_flash_register[i].comment,(uint8_t*)p_bq27510_g3_info->p_data_flash_register[i].value);
               #endif
           } 
         }
         
         //
         default:
         break;
      }//end switch
      
      //
      __kernel_usleep(100000);
   }
   
   //
   return 0;
}

/*-------------------------------------------
| Name:  drv_i2c_bq275xx_check_configuration
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int32_t drv_i2c_bq275xx_check_configuration(desc_t desc){
   int error_count=0;
   dev_bq27510_g3_info_t * p_bq27510_g3_info = (dev_bq27510_g3_info_t*)ofile_lst[desc].p;
  
   //
   for(int i=0; i<p_bq27510_g3_info->data_flash_register_size;i++){
      switch(p_bq27510_g3_info->p_data_flash_register[i].type){
         //
         case DATA_TYPE_U1:
         case DATA_TYPE_I1:
         case DATA_TYPE_H1:{
            int32_t val=0;
            if(drv_i2c_bq275xx_dataflash_read(desc,(uint8_t*)&val,
                                                p_bq27510_g3_info->p_data_flash_register[i].length,  
                                                p_bq27510_g3_info->p_data_flash_register[i].subclass,
                                                p_bq27510_g3_info->p_data_flash_register[i].offset)<0){
               #if defined(DEBUG_KERNEL_PRINTK)
                  kernel_printk("battery: read config %s failed\r\n",p_bq27510_g3_info->p_data_flash_register[i].comment);
               #endif
            }else{
               if(p_bq27510_g3_info->p_data_flash_register[i].value!=val){
                  error_count++;
                  #if defined(DEBUG_KERNEL_PRINTK)
                     kernel_printk("battery: %s not valid\r\n",p_bq27510_g3_info->p_data_flash_register[i].comment);
                  #endif
               }
               //
               #if defined(DEBUG_KERNEL_PRINTK)
                  kernel_printk("battery: %s = %d\r\n",p_bq27510_g3_info->p_data_flash_register[i].comment,val);
               #endif
            }
         }
         break;
         
         //
         case DATA_TYPE_U2:{
            uint16_t val_bigendian= 0;
            //
            if(drv_i2c_bq275xx_dataflash_read(desc, (uint8_t*)&val_bigendian,
                                                p_bq27510_g3_info->p_data_flash_register[i].length,
                                                p_bq27510_g3_info->p_data_flash_register[i].subclass,
                                                p_bq27510_g3_info->p_data_flash_register[i].offset)<0){
               #if defined(DEBUG_KERNEL_PRINTK)
                  kernel_printk("battery: read config %s failed\r\n",p_bq27510_g3_info->p_data_flash_register[i].comment);
               #endif
            }else{
               val_bigendian = swap_uint16(val_bigendian);
               //
               if(p_bq27510_g3_info->p_data_flash_register[i].value!=val_bigendian){
                  error_count++;
                  #if defined(DEBUG_KERNEL_PRINTK)
                     kernel_printk("battery: %s not valid\r\n",p_bq27510_g3_info->p_data_flash_register[i].comment);
                  #endif
               }
               //
               #if defined(DEBUG_KERNEL_PRINTK)
                  kernel_printk("battery: %s = %d\r\n",p_bq27510_g3_info->p_data_flash_register[i].comment,val_bigendian);
               #endif
            }
         }
         
         //
         case DATA_TYPE_I2:{
            int16_t val_bigendian = 0;
            //
            if(drv_i2c_bq275xx_dataflash_read(desc, (uint8_t*)&val_bigendian,
                                                p_bq27510_g3_info->p_data_flash_register[i].length,
                                                p_bq27510_g3_info->p_data_flash_register[i].subclass,
                                                p_bq27510_g3_info->p_data_flash_register[i].offset)<0){
               #if defined(DEBUG_KERNEL_PRINTK)
                  kernel_printk("battery: read config %s failed\r\n",p_bq27510_g3_info->p_data_flash_register[i].comment);
               #endif
            }else{
               val_bigendian = swap_int16(val_bigendian);
               //
               if(p_bq27510_g3_info->p_data_flash_register[i].value!=val_bigendian){
                  error_count++;
                  #if defined(DEBUG_KERNEL_PRINTK)
                     kernel_printk("battery: %s not valid\r\n",p_bq27510_g3_info->p_data_flash_register[i].comment);
                  #endif
               }
               //
               #if defined(DEBUG_KERNEL_PRINTK)
                  kernel_printk("battery:%d %s = %d\r\n",i,p_bq27510_g3_info->p_data_flash_register[i].comment,val_bigendian);
               #endif
            }
         }
         break;
         
         
          //
         case DATA_TYPE_S:{
            uint8_t buf[16]={0};
            //
            if(p_bq27510_g3_info->p_data_flash_register[i].length>sizeof(buf)){
               #if defined(DEBUG_KERNEL_PRINTK)
                  kernel_printk("battery: read config %s failed: length too long\r\n",p_bq27510_g3_info->p_data_flash_register[i].comment);
               #endif
               break;
            }
            //
            if(drv_i2c_bq275xx_dataflash_read(desc,  buf,
                                                p_bq27510_g3_info->p_data_flash_register[i].length,
                                                p_bq27510_g3_info->p_data_flash_register[i].subclass,
                                                p_bq27510_g3_info->p_data_flash_register[i].offset)<0){
               #if defined(DEBUG_KERNEL_PRINTK)
                  kernel_printk("battery: read config %s failed\r\n",p_bq27510_g3_info->p_data_flash_register[i].comment);
               #endif
            }else{
               //
               if(memcmp((void*)p_bq27510_g3_info->p_data_flash_register[i].value,buf,p_bq27510_g3_info->p_data_flash_register[i].length)!=0){
                  error_count++;
                  #if defined(DEBUG_KERNEL_PRINTK)
                     kernel_printk("battery: %s not valid\r\n",p_bq27510_g3_info->p_data_flash_register[i].comment);
                  #endif
               }
               //
               #if defined(DEBUG_KERNEL_PRINTK)
                  kernel_printk("battery:%d %s = %s\r\n",i,p_bq27510_g3_info->p_data_flash_register[i].comment,buf);
               #endif
            }
         }
         break;
         
         //
         default:
         break;
      }//end switch
      //
      __kernel_usleep(100000);
   }
   
   //
   #if defined(DEBUG_KERNEL_PRINTK)
      if(error_count==0){
         kernel_printk("battery:read all parameters ok\r\n");
      }else{
        kernel_printk("error: battery read all parameters failed\r\n");
      }
   #endif
   
   //
   return -error_count;
}

/*-------------------------------------------
| Name:  drv_i2c_bq275xx_reset
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static uint16_t drv_i2c_bq275xx_reset(desc_t desc){
 return drv_i2c_bq275xx_read_control_value(desc,0x00,0x0041,(uint16_t*)0);
}

/*-------------------------------------------
| Name:  drv_i2c_bq275xx_read_id
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static uint16_t drv_i2c_bq275xx_read_id(desc_t desc){
   uint16_t id=0;
   //
   drv_i2c_bq275xx_read_control_value(desc,0x00,0x0001,&id);
   //
	return id;
}

/*-------------------------------------------
| Name:  drv_i2c_bq275xx_read_status
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static uint16_t drv_i2c_bq275xx_read_status(desc_t desc){
   uint16_t voltage=0;
   //
   drv_i2c_bq275xx_read_value(desc,0x0a,(uint8_t*)&voltage,0);
   //
	return voltage;
}

/*-------------------------------------------
| Name:  drv_i2c_bq275xx_read_voltage
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static uint16_t drv_i2c_bq275xx_read_voltage(desc_t desc){
   uint16_t voltage=0;
   //
   drv_i2c_bq275xx_read_value(desc,0x08,(uint8_t*)&voltage,0);
   //
	return voltage;
}

/*-------------------------------------------
| Name:  drv_i2c_bq275xx_read_temperature
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static uint16_t drv_i2c_bq275xx_read_temperature(desc_t desc){
	return 0;
}

/*-------------------------------------------
| Name:  drv_i2c_bq275xx_read_remaining_capacity
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static uint16_t drv_i2c_bq275xx_read_remaining_capacity(desc_t desc){
	uint16_t remaining_capacity=0;
   //
   drv_i2c_bq275xx_read_value(desc,0x10,(uint8_t*)&remaining_capacity,0);
   //
   return remaining_capacity;
}

/*-------------------------------------------
| Name:  drv_i2c_bq275xx_read_state_of_charge
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static uint16_t drv_i2c_bq275xx_read_state_of_charge(desc_t desc){
   uint16_t state_of_charge=0;
   //
   drv_i2c_bq275xx_read_value(desc,0x20,(uint8_t*)&state_of_charge,0);
   //
	return state_of_charge;
}

/*-------------------------------------------
| Name:  drv_i2c_bq275xx_read_instantaneous_current
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static uint16_t drv_i2c_bq275xx_read_instantaneous_current(desc_t desc){
   uint16_t current=0;
   //
   drv_i2c_bq275xx_read_value(desc,0x22,(uint8_t*)&current,0);
   //
	return current;
}

/*-------------------------------------------
| Name:dev_bq27510_g3_x_load
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_bq27510_g3_x_load(dev_bq27510_g3_info_t * p_bq27510_g3_info){
   pthread_mutexattr_t mutex_attr=0;
   //
   p_bq27510_g3_info->desc_r=INVALID_DESC;
   p_bq27510_g3_info->desc_w=INVALID_DESC;
   //
   kernel_pthread_mutex_init(&p_bq27510_g3_info->mutex,&mutex_attr);
   //
   return 0;
}

/*-------------------------------------------
| Name:dev_bq27510_g3_x_open
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_bq27510_g3_x_open(desc_t desc, int o_flag,dev_bq27510_g3_info_t * p_bq27510_g3_info){
                          
   //
   if( p_bq27510_g3_info->desc_r<0 &&  p_bq27510_g3_info->desc_w<0) {   
   }
   //
   if(o_flag & O_RDONLY) {
      if(p_bq27510_g3_info->desc_r<0) {
         p_bq27510_g3_info->desc_r = desc;
      }
      else
         return -1;                //already open
   }

   if(o_flag & O_WRONLY) {
      if(p_bq27510_g3_info->desc_w<0) {
         p_bq27510_g3_info->desc_w = desc;
      }
      else
         return -1;                //already open
   }

   if(!ofile_lst[desc].p)
      ofile_lst[desc].p=p_bq27510_g3_info;

   //
   if( p_bq27510_g3_info->desc_r>=0 &&  p_bq27510_g3_info->desc_w>=0) {
     
   }
   return 0;
}

/*-------------------------------------------
| Name:dev_bq27510_g3_x_close
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_bq27510_g3_x_close(desc_t desc){
  dev_bq27510_g3_info_t * p_bq27510_g3_info = (dev_bq27510_g3_info_t*)ofile_lst[desc].p;
  //
  if(!p_bq27510_g3_info)
   return -1;
  // 
  if(ofile_lst[desc].oflag & O_RDONLY) {
      if(!ofile_lst[desc].nb_reader) {
         p_bq27510_g3_info->desc_r = -1;
      }
   }
   //
   if(ofile_lst[desc].oflag & O_WRONLY) {
      if(!ofile_lst[desc].nb_writer) {
         p_bq27510_g3_info->desc_w = -1;
      }
   }
   //
   if(p_bq27510_g3_info->desc_r<0 && p_bq27510_g3_info->desc_w<0) {
   }

   return 0;
}



/*-------------------------------------------
| Name:dev_bq27510_g3_x_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_bq27510_g3_x_read(desc_t desc, char* buf,int size){
   return _i2c_transfer_read(desc,buf,size);
}

/*-------------------------------------------
| Name:dev_bq27510_g3_x_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_bq27510_g3_x_write(desc_t desc, const char* buf,int size){
    return _i2c_transfer_write(desc,buf,size);
}

/*-------------------------------------------
| Name:dev_bq27510_g3_x_seek
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_bq27510_g3_x_seek(desc_t desc,int offset,int origin){
   return 0;
}

/*-------------------------------------------
| Name:dev_bq27510_g3_x_ioctl
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_bq27510_g3_x_ioctl(desc_t desc,int request,va_list ap) {
   dev_bq27510_g3_info_t * p_bq27510_g3_info = (dev_bq27510_g3_info_t*)ofile_lst[desc].p;

   //
   switch(request) {
      //
      case I_LINK:{
         int argc;
         char** argv;
         int i;
         int fd;
         uint8_t i2c_addr;
         uint16_t chip_id;
         //
         fd=va_arg(ap, int);//not used
         //
         argc=va_arg(ap, int);
         argv=va_arg(ap, char**);
         //
         if(argc<(I2CADDR_ARGC+1))
           return -1;
         //
        
         if(!p_bq27510_g3_info)
            return -1;
   
         //
         for(i=I2CADDR_ARGC; i<argc; i++) {
           
            switch(i){
               case I2CADDR_ARGC:
                  if(argv[i][0]!='0' || argv[i][1]!='x')
                     return -1;
                  
                  i2c_addr=htoi(argv[i][2])*16+htoi(argv[i][3]);
                  //
                  p_bq27510_g3_info->i2c_addr=i2c_addr;
                  //
                  chip_id = drv_i2c_bq275xx_read_id(desc);
                  if(chip_id!= p_bq27510_g3_info->chip_id){
                     return -1;
                  }
               break;
               
               default:
                  //nothing to do
               break;
            }
         }
      }
      return 0;
      
      //   
      case I_UNLINK:
        //nothing to do, see _vfs_ioctl2() 
        //ioctl of device driver is call with request I_LINK, I_UNLINK.
      return 0;

      //
      case IOCTL_CMD_BQ27510_RESET:
         drv_i2c_bq275xx_reset(desc);
      break;
      
      //
      case IOCTL_CMD_BQ27510_CFGCHK:{
         int* p_status=va_arg(ap, int*);
         *p_status = drv_i2c_bq275xx_check_configuration(desc);
      }
      break;
      
       //
      case IOCTL_CMD_BQ27510_CFGWR:{
         int* p_status=va_arg(ap, int*);
         *p_status = drv_i2c_bq275xx_write_configuration(desc);
      }
      break;
      //
      case PWRGETCURRENT:{
         int* p_value=va_arg(ap, int*);
         *p_value = drv_i2c_bq275xx_read_instantaneous_current(desc);
      }
      break;
      //
      case PWRGETVOLT:{
         int* p_value=va_arg(ap, int*);
         *p_value = drv_i2c_bq275xx_read_voltage(desc);
      }
      break;
      //
      case PWRGETTEMP:{
         int* p_value=va_arg(ap, int*);
         *p_value = drv_i2c_bq275xx_read_temperature(desc);
      }
      break;
      //
      case PWRGETCAPACITY:{
         int* p_value=va_arg(ap, int*);
         *p_value = drv_i2c_bq275xx_read_remaining_capacity(desc);
      }
      break;
      //
      case PWRGETSOC:{
         int* p_value=va_arg(ap, int*);
         *p_value = drv_i2c_bq275xx_read_state_of_charge(desc);
      }
      break;
      
   

      //
      default:
         return -1;

   }

   return 0;
}


/*============================================
| End of Source  : dev_bq27510_g3_x.c
==============================================*/