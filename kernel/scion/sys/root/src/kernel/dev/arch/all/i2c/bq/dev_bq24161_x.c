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

#include "kernel/dev/arch/all/i2c/bq/dev_bq24161_x.h"

   
/*===========================================
Global Declaration
=============================================*/
#define DEBUG_KERNEL_PRINTK
#define I2CADDR_ARGC 4

//
#define BQ24161_TIMEOUT ((int)(10)) //seconds

#define BQ24161_REGISTER_ADDRESS_STATUS               ((uint8_t)(0x00))
#define BQ24161_REGISTER_ADDRESS_BATTSUPPLY_STATUS    ((uint8_t)(0x01))
#define BQ24161_REGISTER_ADDRESS_CONTROL              ((uint8_t)(0x02))
//
#define BQ24161_KERNEL_THREAD_STACK_SIZE  2048
#define BQ24161_KERNEL_THREAD_PRIORITY    80
static _macro_stack_addr char bq24161_kernel_thread_stack[BQ24161_KERNEL_THREAD_STACK_SIZE];


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
   dev_bq24161_info_t * p_dev_bq24161_info = (dev_bq24161_info_t*)ofile_lst[desc].p;
   desc_t desc_i2c_read;
   uint8_t bufr[1]={0};
   //
   //
   if(!p_dev_bq24161_info)
      return -1;
   //
   desc_i2c_read=ofile_lst[desc].desc_nxt[0];
   
   //i2c access
   kernel_io_ll_ioctl(desc_i2c_read,I2C_LOCK);
   
   //i2c addr
   kernel_io_ll_ioctl(desc_i2c_read,I2C_SLAVEADDR,p_dev_bq24161_info->i2c_addr);
   
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
   dev_bq24161_info_t * p_dev_bq24161_info = (dev_bq24161_info_t*)ofile_lst[desc].p;
   desc_t desc_i2c_write;
   uint8_t bufw[2]={0};
   //
   if(!p_dev_bq24161_info)
      return -1;
   //
   desc_i2c_write=ofile_lst[desc].desc_nxt[1];
   
   //i2c access
   kernel_io_ll_ioctl(desc_i2c_write,I2C_LOCK);
   
   //i2c addr
   kernel_io_ll_ioctl(desc_i2c_write,I2C_SLAVEADDR,p_dev_bq24161_info->i2c_addr);
   //
   bufw[0]=register_address;
   bufw[1]=register_value;
   //
   if(kernel_io_ll_write(desc_i2c_write,bufw,/*8*/2)<0){
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
| Name: bq24161_x_kernel_thread_routine
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static void* bq24161_x_kernel_thread_routine(void* arg){
   dev_bq24161_info_t * p_dev_bq24161_info = (dev_bq24161_info_t*)arg;
   desc_t desc = p_dev_bq24161_info->desc_w;
   int timeout_cpt=0;
   uint8_t v;
   
   
   //
   for(;;){
      __kernel_usleep(1000000);
      if(--timeout_cpt>0)
         continue;
      //
      timeout_cpt=BQ24161_TIMEOUT;
      //
      if( p_dev_bq24161_info->command == BQ24161_CMD_CHRGSTOP)
         continue;
     
      //watchdog
      bq24161_read_register(desc,BQ24161_REGISTER_ADDRESS_STATUS,&v);
      //read or write in status registrer periodicly rearm watchdog timer 
      v = v | (0x80); // set to 1 bit7 to rearm watchdog
      
      //safety timer
      bq24161_write_register(desc,BQ24161_REGISTER_ADDRESS_STATUS,v);
      //
      for(int i=0;i<p_dev_bq24161_info->registers_list_size;i++){
         if(bq24161_write_register(desc,
                                    p_dev_bq24161_info->p_registers_list[i].address,
                                    p_dev_bq24161_info->p_registers_list[i].value)<0){
            kernel_printk("bq21161: write error on register 0x%02x \r\n", p_dev_bq24161_info->p_registers_list[i].address);
         }                       
      }
      
   }
   //
   return (void*)0;
}

/*-------------------------------------------
| Name: dev_bq24161_x_load
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_bq24161_x_load(dev_bq24161_info_t * p_dev_bq24161_info){
   pthread_mutexattr_t mutex_attr=0;
   //
   p_dev_bq24161_info->desc_r=INVALID_DESC;
   p_dev_bq24161_info->desc_w=INVALID_DESC;
   //
   p_dev_bq24161_info->command= BQ24161_CMD_NONE;
   //
   kernel_pthread_mutex_init(&p_dev_bq24161_info->mutex,&mutex_attr);
   //
   return 0;
}

/*-------------------------------------------
| Name:dev_bq24161_x_open
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_bq24161_x_open(desc_t desc, int o_flag,dev_bq24161_info_t * p_dev_bq24161_info){
                          
   //
   if( p_dev_bq24161_info->desc_r<0 &&  p_dev_bq24161_info->desc_w<0) {   
   }
   //
   if(o_flag & O_RDONLY) {
      if(p_dev_bq24161_info->desc_r<0) {
         p_dev_bq24161_info->desc_r = desc;
      }
      else
         return -1;                //already open
   }

   if(o_flag & O_WRONLY) {
      if(p_dev_bq24161_info->desc_w<0) {
         p_dev_bq24161_info->desc_w = desc;
      }
      else
         return -1;                //already open
   }

   if(!ofile_lst[desc].p)
      ofile_lst[desc].p=p_dev_bq24161_info;

   //
   if( p_dev_bq24161_info->desc_r>=0 &&  p_dev_bq24161_info->desc_w>=0) {
     
   }
   return 0;
}

/*-------------------------------------------
| Name:dev_bq24161_x_close
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_bq24161_x_close(desc_t desc){
  dev_bq24161_info_t * p_dev_bq24161_info = (dev_bq24161_info_t*)ofile_lst[desc].p;
  //
  if(!p_dev_bq24161_info)
   return -1;
  // 
  if(ofile_lst[desc].oflag & O_RDONLY) {
      if(!ofile_lst[desc].nb_reader) {
         p_dev_bq24161_info->desc_r = -1;
      }
   }
   //
   if(ofile_lst[desc].oflag & O_WRONLY) {
      if(!ofile_lst[desc].nb_writer) {
         p_dev_bq24161_info->desc_w = -1;
      }
   }
   //
   if(p_dev_bq24161_info->desc_r<0 && p_dev_bq24161_info->desc_w<0) {
   }

   return 0;
}



/*-------------------------------------------
| Name:dev_bq24161_x_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_bq24161_x_read(desc_t desc, char* buf,int size){
   return -1;
}

/*-------------------------------------------
| Name:dev_bq24161_x_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_bq24161_x_write(desc_t desc, const char* buf,int size){
    return -1;
}

/*-------------------------------------------
| Name:dev_bq24161_x_seek
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_bq24161_x_seek(desc_t desc,int offset,int origin){
   return 0;
}

/*-------------------------------------------
| Name:dev_bq24161_x_ioctl
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_bq24161_x_ioctl(desc_t desc,int request,va_list ap) {
   dev_bq24161_info_t * p_dev_bq24161_info = (dev_bq24161_info_t*)ofile_lst[desc].p;

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
        
         if(!p_dev_bq24161_info)
            return -1;
   
         //
         for(i=I2CADDR_ARGC; i<argc; i++) {
           
            switch(i){
               case I2CADDR_ARGC:
                  if(argv[i][0]!='0' || argv[i][1]!='x')
                     return -1;
                  
                  i2c_addr=htoi(argv[i][2])*16+htoi(argv[i][3]);
                  //
                  p_dev_bq24161_info->i2c_addr=i2c_addr;
               break;
               
               default:
                  //nothing to do
               break;
            }
         }
         //
         pthread_attr_t          thread_attr={0}; 
         //
         thread_attr.stacksize = BQ24161_KERNEL_THREAD_STACK_SIZE;
         thread_attr.stackaddr = (void*)&bq24161_kernel_thread_stack;
         thread_attr.priority  = BQ24161_KERNEL_THREAD_PRIORITY;
         thread_attr.timeslice = 1;
         thread_attr.name = "kernel_pthread_bq24161";
         //
         kernel_pthread_create(&p_dev_bq24161_info->kernel_thread,&thread_attr,bq24161_x_kernel_thread_routine,p_dev_bq24161_info);
      }
      return 0;
      
      //   
      case I_UNLINK:
        //nothing to do, see _vfs_ioctl2() 
        //ioctl of device driver is call with request I_LINK, I_UNLINK.
      return 0;
   
      //
      case IOCTL_CMD_BQ24161_CFGWR:{
         uint32_t* p_status=va_arg(ap, uint32_t*);
         uint8_t status=0;
         //
         for(int i=0;i<p_dev_bq24161_info->registers_list_size;i++){
            if(bq24161_write_register(desc,
                                       p_dev_bq24161_info->p_registers_list[i].address,
                                       p_dev_bq24161_info->p_registers_list[i].value)<0){
               kernel_printk("bq21161: write error on register 0x%02x \r\n", p_dev_bq24161_info->p_registers_list[i].address);
            }else{
               kernel_printk("bq21161: write on register 0x%02x ok\r\n", p_dev_bq24161_info->p_registers_list[i].address);
            }                         
         }
         //
         if(p_status)
            *p_status=0;
         //
         bq24161_read_register(desc,BQ24161_REGISTER_ADDRESS_STATUS,&status);
         //
         if(p_status)
            *p_status=status;
         //
         bq24161_read_register(desc,BQ24161_REGISTER_ADDRESS_BATTSUPPLY_STATUS,&status);
         //
         if(p_status)
            *p_status=(*p_status | (status<<8));
      }
      break;
      
      //
      case IOCTL_CMD_BQ24161_STAT:{
         uint32_t* p_status=va_arg(ap, uint32_t*);
         uint8_t status=0;
         //
         if(p_status)
            *p_status=0;
         //
         bq24161_read_register(desc,BQ24161_REGISTER_ADDRESS_STATUS,&status);
         //
         if(p_status)
            *p_status=status;
         //
         bq24161_read_register(desc,BQ24161_REGISTER_ADDRESS_BATTSUPPLY_STATUS,&status);
         //
         if(p_status)
            *p_status=(*p_status | (status<<8));
         //
         bq24161_read_register(desc,BQ24161_REGISTER_ADDRESS_CONTROL,&status);
         //
         if(p_status)
            *p_status=(*p_status | (status<<16));
      }
      break;
      
      //
      case IOCTL_CMD_BQ24161_WTCHDG:{
         uint32_t* p_status=va_arg(ap, uint32_t*);
         uint8_t status=0;
         //
         if(bq24161_write_register(desc,BQ24161_REGISTER_ADDRESS_STATUS,0x80)<0){
            kernel_printk("bq21161: cannot reset watchdog\r\n");
         }else{
            kernel_printk("bq21161: reset watchdog ok\r\n");
         }                  
         //
         if(p_status)
            *p_status=0;
         //
         bq24161_read_register(desc,BQ24161_REGISTER_ADDRESS_STATUS,&status);
         //
         if(p_status)
            *p_status=status;
         //
         bq24161_read_register(desc,BQ24161_REGISTER_ADDRESS_BATTSUPPLY_STATUS,&status);
         //
         if(p_status)
            *p_status=(*p_status | (status<<8));
      }
      break;
      
      //
      case IOCTL_CMD_BQ24161_RSTCHRGCYCL:{
         uint8_t status=0;
         uint32_t* p_status=va_arg(ap, uint32_t*);
         //
         bq24161_read_register(desc,BQ24161_REGISTER_ADDRESS_CONTROL,&status);
         //
         status = status & ~(0x80);
         status = status | 0x01;
         //
         bq24161_write_register(desc,BQ24161_REGISTER_ADDRESS_CONTROL,status);
         //
         __kernel_usleep(1000000);
         //
         bq24161_read_register(desc,BQ24161_REGISTER_ADDRESS_CONTROL,&status);
         //
         status = status & ~(0x81);
         //
         bq24161_write_register(desc,BQ24161_REGISTER_ADDRESS_CONTROL,status);
         //
         if(p_status)
            *p_status=0;
         //
         bq24161_read_register(desc,BQ24161_REGISTER_ADDRESS_STATUS,&status);
         //
         if(p_status)
            *p_status=status;
         //
         bq24161_read_register(desc,BQ24161_REGISTER_ADDRESS_BATTSUPPLY_STATUS,&status);
         //
         if(p_status)
            *p_status=(*p_status | (status<<8));
         //
         bq24161_read_register(desc,BQ24161_REGISTER_ADDRESS_CONTROL,&status);
         //
         if(p_status)
            *p_status=(*p_status | (status<<16));
      }
      break;
      
      //
      case IOCTL_CMD_BQ24161_STOPCHRG:{
         uint8_t status=0;
         uint32_t* p_status=va_arg(ap, uint32_t*);
         
         //
         p_dev_bq24161_info->command = BQ24161_CMD_CHRGSTOP;
         __kernel_usleep(1000000);
         //
          
         //reset all
         bq24161_read_register(desc,BQ24161_REGISTER_ADDRESS_CONTROL,&status);
         //
         __kernel_usleep(100000);
         //
         status = status & ~(0x80);
         //
         bq24161_write_register(desc,BQ24161_REGISTER_ADDRESS_CONTROL,status);
         
         //
         __kernel_usleep(100000);
          
         //HIGHZ impedance bit and charge disable bit 
         bq24161_read_register(desc,BQ24161_REGISTER_ADDRESS_CONTROL,&status);
         //
         __kernel_usleep(100000);
         //
         status = status | 0x03;
         //
         bq24161_write_register(desc,BQ24161_REGISTER_ADDRESS_CONTROL,status);
         //
         __kernel_usleep(100000);
         //
         bq24161_read_register(desc,BQ24161_REGISTER_ADDRESS_CONTROL,&status);
         //
         if(p_status)
             *p_status=status;
         // 
      }
      break;
      
      
      
      //
      default:
         return -1;

   }

   return 0;
}


/*============================================
| End of Source  : dev_bq24161_x.c
==============================================*/