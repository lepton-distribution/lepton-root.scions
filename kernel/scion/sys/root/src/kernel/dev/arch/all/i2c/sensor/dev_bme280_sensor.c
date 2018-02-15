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
#include "kernel/core/kernel_io.h"
#include "kernel/core/kernel_printk.h"

#include "kernel/fs/vfs/vfstypes.h"



   
/*===========================================
Global Declaration
=============================================*/
#define DEBUG_KERNEL_PRINTK
#define I2CADDR_ARGC 4

//I2C ADDRESS 0x28H << 1 = 0x50: 
//for stm32f4:
//mount /dev/bme280 /dev/i2c1 /dev/press0 0x50
//for same70
//mount /dev/bme280 /dev/i2c1 /dev/press0 0x28

//
static const char dev_bme280_name[]="bme280";

//
static int dev_bme280_load(void);
static int dev_bme280_open(desc_t desc, int o_flag);
static int dev_bme280_close(desc_t desc);
static int dev_bme280_seek(desc_t desc,int offset,int origin);
static int dev_bme280_read(desc_t desc, char* buf,int cb);
static int dev_bme280_write(desc_t desc, const char* buf,int cb);
static int dev_bme280_ioctl(desc_t desc,int request,va_list ap);

//
dev_map_t dev_bme280_map={
   dev_bme280_name,
   S_IFBLK,
   dev_bme280_load,
   dev_bme280_open,
   dev_bme280_close,
   __fdev_not_implemented,
   __fdev_not_implemented,
   dev_bme280_read,
   dev_bme280_write,
   dev_bme280_seek,
   dev_bme280_ioctl
};
//
typedef struct dev_bme280_register_st{
   uint8_t address;
   uint8_t value;
}dev_bme280_register_t;

//
typedef struct dev_bme280_info_st {
   uint8_t i2c_addr;
   //
   desc_t desc_r;
   desc_t desc_w;
   //
   kernel_pthread_mutex_t mutex;
}dev_bme280_info_t;

//
static dev_bme280_info_t g_dev_bme280_info;

/*===========================================
Implementation
=============================================*/



/*-------------------------------------------
| Name: htoi
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
| Name: bq24161_write_register
| Description:
| Parameters:
| Return Type:
| Comments: 
| See:
---------------------------------------------*/
static int bq24161_read_register(desc_t desc, uint8_t register_address, uint8_t* p_register_value){
   dev_bme280_info_t * p_dev_bme280_info = (dev_bme280_info_t*)ofile_lst[desc].p;
   desc_t desc_i2c_read;
   uint8_t bufr[1]={0};
   //
   //
   if(!p_dev_bme280_info)
      return -1;
   //
   desc_i2c_read=ofile_lst[desc].desc_nxt[0];
   
   //i2c access
   kernel_io_ll_ioctl(desc_i2c_read,I2C_LOCK);
   
   //i2c addr
   kernel_io_ll_ioctl(desc_i2c_read,I2C_SLAVEADDR,p_dev_bme280_info->i2c_addr);
   
   //
   bufr[0]=register_address;
   if(kernel_io_ll_write(desc_i2c_read,bufr,1)<0){
      //i2c access
      kernel_io_ll_ioctl(desc_i2c_read,I2C_UNLOCK);
      return -1;
   }
   //
   __kernel_usleep(100000); //100ms
   
   //
   if(kernel_io_ll_read(desc_i2c_read,bufr,sizeof(bufr))<0){
      //i2c access
      kernel_io_ll_ioctl(desc_i2c_read,I2C_UNLOCK);
      return -1;
   }
   
   //i2c access
   kernel_io_ll_ioctl(desc_i2c_read,I2C_UNLOCK);
   //
   *p_register_value = bufr[0];
   //
   return 0;
}

/*-------------------------------------------
| Name: bq24161_write_register
| Description:
| Parameters:
| Return Type:
| Comments: 
| See:
---------------------------------------------*/
static int bq24161_write_register(desc_t desc, uint8_t register_address, uint8_t register_value){
   dev_bme280_info_t * p_dev_bme280_info = (dev_bme280_info_t*)ofile_lst[desc].p;
   desc_t desc_i2c_write;
   uint8_t bufw[2]={0};
   //
   if(!p_dev_bme280_info)
      return -1;
   //
   desc_i2c_write=ofile_lst[desc].desc_nxt[1];
   
   //i2c access
   kernel_io_ll_ioctl(desc_i2c_write,I2C_LOCK);
   
   //i2c addr
   kernel_io_ll_ioctl(desc_i2c_write,I2C_SLAVEADDR,p_dev_bme280_info->i2c_addr);
   //
   bufw[0]=register_address;
   bufw[1]=register_value;
   //
   if(kernel_io_ll_write(desc_i2c_write,bufw,2)<0){
      //i2c access
      kernel_io_ll_ioctl(desc_i2c_write,I2C_UNLOCK);
      return -1;
   }
   
   //i2c access
   kernel_io_ll_ioctl(desc_i2c_write,I2C_UNLOCK);
   //
   return 0;
}



/*-------------------------------------------
| Name: dev_bme280_load
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_bme280_load(void){
   pthread_mutexattr_t mutex_attr=0;
   //
   g_dev_bme280_info.desc_r=INVALID_DESC;
   g_dev_bme280_info.desc_w=INVALID_DESC;
   //
   kernel_pthread_mutex_init(&g_dev_bme280_info.mutex,&mutex_attr);
   //
   return 0;
}

/*-------------------------------------------
| Name:dev_bme280_open
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_bme280_open(desc_t desc, int o_flag){
   dev_bme280_info_t * p_dev_bme280_info = &g_dev_bme280_info;                   
   //
   if( p_dev_bme280_info->desc_r<0 &&  p_dev_bme280_info->desc_w<0) {   
   }
   //
   if(o_flag & O_RDONLY) {
      if(p_dev_bme280_info->desc_r<0) {
         p_dev_bme280_info->desc_r = desc;
      }
      else
         return -1;                //already open
   }

   if(o_flag & O_WRONLY) {
      if(p_dev_bme280_info->desc_w<0) {
         p_dev_bme280_info->desc_w = desc;
      }
      else
         return -1;                //already open
   }

   if(!ofile_lst[desc].p)
      ofile_lst[desc].p=p_dev_bme280_info;

   //
   if( p_dev_bme280_info->desc_r>=0 &&  p_dev_bme280_info->desc_w>=0) {
     
   }
   return 0;
}

/*-------------------------------------------
| Name:dev_bme280_close
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_bme280_close(desc_t desc){
  dev_bme280_info_t * p_dev_bme280_info = (dev_bme280_info_t*)ofile_lst[desc].p;
  //
  if(!p_dev_bme280_info)
   return -1;
  // 
  if(ofile_lst[desc].oflag & O_RDONLY) {
      if(!ofile_lst[desc].nb_reader) {
         p_dev_bme280_info->desc_r = -1;
      }
   }
   //
   if(ofile_lst[desc].oflag & O_WRONLY) {
      if(!ofile_lst[desc].nb_writer) {
         p_dev_bme280_info->desc_w = -1;
      }
   }
   //
   if(p_dev_bme280_info->desc_r<0 && p_dev_bme280_info->desc_w<0) {
   }

   return 0;
}



/*-------------------------------------------
| Name:dev_bme280_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_bme280_read(desc_t desc, char* buf,int size){
   dev_bme280_info_t * p_dev_bme280_info = (dev_bme280_info_t*)ofile_lst[desc].p;
   desc_t desc_i2c_read;
   //
   //
   if(!p_dev_bme280_info)
      return -1;
   //
   desc_i2c_read=ofile_lst[desc].desc_nxt[0];
   
   //i2c access
   kernel_io_ll_ioctl(desc_i2c_read,I2C_LOCK);
   
   //i2c addr
   kernel_io_ll_ioctl(desc_i2c_read,I2C_SLAVEADDR,p_dev_bme280_info->i2c_addr);
   
   //
   if(kernel_io_ll_read(desc_i2c_read,buf,size)<0){
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
| Name:dev_bme280_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_bme280_write(desc_t desc, const char* buf,int size){
    return -1;
}

/*-------------------------------------------
| Name:dev_bme280_seek
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_bme280_seek(desc_t desc,int offset,int origin){
   return 0;
}

/*-------------------------------------------
| Name:dev_bme280_ioctl
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_bme280_ioctl(desc_t desc,int request,va_list ap) {
   dev_bme280_info_t * p_dev_bme280_info = (dev_bme280_info_t*)ofile_lst[desc].p;

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
        
         if(!p_dev_bme280_info)
            return -1;
   
         //
         for(i=I2CADDR_ARGC; i<argc; i++) {
           
            switch(i){
               case I2CADDR_ARGC:
                  if(argv[i][0]!='0' || argv[i][1]!='x')
                     return -1;
                  
                  i2c_addr=htoi(argv[i][2])*16+htoi(argv[i][3]);
                  //
                  p_dev_bme280_info->i2c_addr=i2c_addr;
               break;
               
               default:
                  //nothing to do
               break;
            }
         }
         //
#if 0
         char sensor_buffer[4];
         int error;
         //
         error=bme280_read_register(desc,sensor_buffer,sizeof(sensor_buffer));
         //
         return error;
#else
         return 0;
#endif
      }
      return 0;
      
      //   
      case I_UNLINK:
        //nothing to do, see _vfs_ioctl2() 
        //ioctl of device driver is call with request I_LINK, I_UNLINK.
      return 0;
   
     
      
      
      //
      default:
         return -1;

   }

   return 0;
}


/*============================================
| End of Source  : dev_bme280.c
==============================================*/