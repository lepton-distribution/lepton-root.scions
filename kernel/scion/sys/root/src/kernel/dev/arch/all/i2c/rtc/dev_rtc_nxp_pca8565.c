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

#include "kernel/fs/vfs/vfstypes.h"

#include "kernel/dev/arch/all/i2c/rtc/dev_rtc_nxp_pca8565.h"

/*===========================================
Global Declaration
=============================================*/
#define I2CADDR_ARGC 4
#define NXP_PCA8565_MEMORY_SIZE 20

//unsigned char dev_rtc_nxp_pca8565_buffer[NXP_PCA8565_MEMORY_SIZE+2]={0};
static unsigned char dev_rtc_nxp_pca8565_buffer[NXP_PCA8565_MEMORY_SIZE+4];

static const char dev_rtc_nxp_pca8565_addr   =  0x68; //already right shift 0x68=(rtc addr)>>1.

static const char dev_rtc_nxp_pca8565_name[]="pca8565\0rtc_nxp_pca8565\0";

static int dev_rtc_nxp_pca8565_load(void);
static int dev_rtc_nxp_pca8565_open(desc_t desc, int o_flag);
static int dev_rtc_nxp_pca8565_close(desc_t desc);
static int dev_rtc_nxp_pca8565_seek(desc_t desc,int offset,int origin);
static int dev_rtc_nxp_pca8565_read(desc_t desc, char* buf,int cb);
static int dev_rtc_nxp_pca8565_write(desc_t desc, const char* buf,int cb);
static int dev_rtc_nxp_pca8565_ioctl(desc_t desc,int request,va_list ap);

//specific rtc device function
int dev_rtc_nxp_pca8565_settime(desc_t desc,char* buf,int size);
int dev_rtc_nxp_pca8565_gettime(desc_t desc,char* buf,int size);

dev_rtc_t dev_rtc_nxp_pca8565_ext={
   dev_rtc_nxp_pca8565_settime,
   dev_rtc_nxp_pca8565_gettime
};

//
dev_map_t dev_rtc_nxp_pca8565_map={
   dev_rtc_nxp_pca8565_name,
   S_IFBLK,
   dev_rtc_nxp_pca8565_load,
   dev_rtc_nxp_pca8565_open,
   dev_rtc_nxp_pca8565_close,
   __fdev_not_implemented,
   __fdev_not_implemented,
   dev_rtc_nxp_pca8565_read,
   dev_rtc_nxp_pca8565_write,
   dev_rtc_nxp_pca8565_seek,
   dev_rtc_nxp_pca8565_ioctl, 
   (pfdev_ext_t)&dev_rtc_nxp_pca8565_ext
};

//from netBSD
#define BCD_TO_BIN(val) ((val)=((val)&15) + ((val)>>4)*10)
#define BIN_TO_BCD(val) ((val)=(((val)/10)<<4) + (val)%10)

typedef struct dev_rtc_nxp_pca8565_info_st {
   uint8_t i2c_addr;
}dev_rtc_nxp_pca8565_info_t;

static dev_rtc_nxp_pca8565_info_t  g_dev_rtc_nxp_pca8565_info;

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
| Name:dev_rtc_nxp_pca8565_setalarm
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_rtc_nxp_pca8565_setalarm(desc_t desc, uint8_t hour, uint8_t min, uint8_t sec){
   dev_rtc_nxp_pca8565_info_t * p_dev_rtc_nxp_pca8565_info = (dev_rtc_nxp_pca8565_info_t*)ofile_lst[desc].p;
   desc_t desc_i2c_write;
   char bufw[6]={0};
   /*
   _tm.tm_sec  = buf[0];
   _tm.tm_min  = buf[1];
   _tm.tm_hour = buf[2];
   _tm.tm_mday = buf[3];
   _tm.tm_mon  = buf[4];
   _tm.tm_year = buf[5];
   */
   
   //
   memset(bufw,0,sizeof(bufw));
   //offset; register Minute_alarm (address 09h)  Hour_alarm (address 0Ah)
   bufw[0]=0x09;
   //minutes
   bufw[1]=BIN_TO_BCD(min /*tm.tm_min*/);
   bufw[1]&=0x7f;
   //hours
   bufw[2]=BIN_TO_BCD(hour /*tm.tm_hour*/);
   bufw[2]&=0x3f;
   //day alarm is disabled
   bufw[3]=0x80;
   //weekday alarm is disabled
   bufw[4]=0x80;

   
   //
   if(!p_dev_rtc_nxp_pca8565_info)
      return -1;
   //
   desc_i2c_write=ofile_lst[desc].desc_nxt[1];
   
   //i2c access
   kernel_io_ll_ioctl(desc_i2c_write,I2C_LOCK);
   
   //i2c addr
   kernel_io_ll_ioctl(desc_i2c_write,I2C_SLAVEADDR,p_dev_rtc_nxp_pca8565_info->i2c_addr);
   
   //set alarm hour min
   if(kernel_io_ll_write(desc_i2c_write,bufw,5)<0){
      //i2c access
      kernel_io_ll_ioctl(desc_i2c_write,I2C_UNLOCK);
      return -1;
   }
   
   //
   __kernel_usleep(100000);
   
   //set alarm flag and alarm interrupt enable
   //offset: register Control_2 (address 01h)
   bufw[0]=0x01;
   //
   bufw[1]=0x02;
   ///
   if(kernel_io_ll_write(desc_i2c_write,bufw,2)<0){
      //i2c access
      kernel_io_ll_ioctl(desc_i2c_write,I2C_UNLOCK);
      return -1;
   }
   
   //i2c access
   kernel_io_ll_ioctl(desc_i2c_write,I2C_UNLOCK);

   return 0;
}

/*-------------------------------------------
| Name:dev_rtc_nxp_pca8565_settime
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_rtc_nxp_pca8565_settime(desc_t desc,char* buf,int size){
   dev_rtc_nxp_pca8565_info_t * p_dev_rtc_nxp_pca8565_info = (dev_rtc_nxp_pca8565_info_t*)ofile_lst[desc].p;
   desc_t desc_i2c_write;
   char bufw[9]={0};
   /*
   _tm.tm_sec  = buf[0];
   _tm.tm_min  = buf[1];
   _tm.tm_hour = buf[2];
   _tm.tm_mday = buf[3];
   _tm.tm_mon  = buf[4];
   _tm.tm_year = buf[5];
   */
   
   //
   memset(bufw,0,sizeof(bufw));
   //
   //offset;
   bufw[0]=0x02;
   //seconds
   bufw[1]=BIN_TO_BCD(buf[0] /*tm.tm_sec*/);
   bufw[1]&=0x7f;
   //minutes
   bufw[2]=BIN_TO_BCD(buf[1] /*tm.tm_min*/);
   bufw[2]&=0x7f;
   //hours
   bufw[3]=BIN_TO_BCD(buf[2] /*tm.tm_hour*/);
   bufw[3]&=0x3f;

   //day
   bufw[4]=BIN_TO_BCD(buf[3] /*tm.tm_mday*/);
   
   //day of week
   //bufw[5]
   
   //month
   buf[4]++; /*tm.tm_mon*/
   bufw[6]=BIN_TO_BCD(buf[4] /*tm.tm_mon*/); //nxp_pca8565 month:1..12
   bufw[6]|=0x80; //indicates the century is x + 1
   //year
   buf[5]%=100; /*tm.tm_year*/
   bufw[7]=BIN_TO_BCD(buf[5] /*tm.tm_year*/);
   
   
   //
   if(!p_dev_rtc_nxp_pca8565_info)
      return -1;
   //
   desc_i2c_write=ofile_lst[desc].desc_nxt[1];
   
   //i2c access
   kernel_io_ll_ioctl(desc_i2c_write,I2C_LOCK);
   
   //i2c addr
   kernel_io_ll_ioctl(desc_i2c_write,I2C_SLAVEADDR,p_dev_rtc_nxp_pca8565_info->i2c_addr);
   
   //
   if(kernel_io_ll_write(desc_i2c_write,bufw,8)<0){
      //i2c access
      kernel_io_ll_ioctl(desc_i2c_write,I2C_UNLOCK);
      return -1;
   }
   
   //i2c access
   kernel_io_ll_ioctl(desc_i2c_write,I2C_UNLOCK);

   return 0;
}

/*-------------------------------------------
| Name:dev_rtc_nxp_pca8565_gettime
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_rtc_nxp_pca8565_gettime(desc_t desc,char* buf,int size){
   dev_rtc_nxp_pca8565_info_t * p_dev_rtc_nxp_pca8565_info = (dev_rtc_nxp_pca8565_info_t*)ofile_lst[desc].p;
   desc_t desc_i2c_read;
   char bufr[8]={0};
   
   /*
   _tm.tm_sec  = buf[0];
   _tm.tm_min  = buf[1];
   _tm.tm_hour = buf[2];
   _tm.tm_mday = buf[3];
   _tm.tm_mon  = buf[4];
   _tm.tm_year = buf[5];
   */
  
   //
   if(!p_dev_rtc_nxp_pca8565_info)
      return -1;
   //
   desc_i2c_read=ofile_lst[desc].desc_nxt[0];
   
   //i2c access
   kernel_io_ll_ioctl(desc_i2c_read,I2C_LOCK);
   
   //i2c addr
   kernel_io_ll_ioctl(desc_i2c_read,I2C_SLAVEADDR,p_dev_rtc_nxp_pca8565_info->i2c_addr);
   
   //
   for(uint32_t i=0xffff;i>0;i--){
      for(uint32_t j=0xfff;j>0;j--)asm("nop");
   }
   
   //
   bufr[0]=0x02;
   if(kernel_io_ll_write(desc_i2c_read,bufr,1)<0){
      //i2c access
      kernel_io_ll_ioctl(desc_i2c_read,I2C_UNLOCK);
      return -1;
   }
   
   //
   //for(uint32_t i=0xfff;i>0;i--){
      //for(uint32_t j=0xfff;j>0;j--)asm("nop");
   //}
   
       
   //
   if(kernel_io_ll_read(desc_i2c_read,bufr,sizeof(bufr))<0){
      //i2c access
      kernel_io_ll_ioctl(desc_i2c_read,I2C_UNLOCK);
      return -1;
   }
   
   //i2c access
   kernel_io_ll_ioctl(desc_i2c_read,I2C_UNLOCK);
 
   //seconds
   bufr[0]&=0x7f;
   buf[0]  = BCD_TO_BIN(bufr[0]);
   //minutes
   bufr[1]&=0x7f;
   buf[1]  = BCD_TO_BIN(bufr[1]);
   //hours
   bufr[2]&=0x3f;
   buf[2] = BCD_TO_BIN(bufr[2]);
   
   //day
   bufr[3]&=0x3f;
   buf[3] = BCD_TO_BIN(bufr[3]);
   
   //day of week
   //bufr[4]
   
   //month
   bufr[5]&=0x1f;
   buf[4] = (BCD_TO_BIN(bufr[5])); //_tm.tm_mon 0..11 posix see <time.h>
   buf[4]--;
   //year
   buf[5] = BCD_TO_BIN(bufr[6]);
   buf[5] += 100;

   return 0;
}

/*-------------------------------------------
| Name:dev_rtc_nxp_pca8565_load
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_rtc_nxp_pca8565_load(void){
   //
   g_dev_rtc_nxp_pca8565_info.i2c_addr=0x51;
   //
   return 0;
}

/*-------------------------------------------
| Name:dev_rtc_nxp_pca8565_isset_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_rtc_nxp_pca8565_isset_read(desc_t desc){
   return -1;
}

/*-------------------------------------------
| Name:dev_rtc_nxp_pca8565_isset_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_rtc_nxp_pca8565_isset_write(desc_t desc){
   return -1;
}

/*-------------------------------------------
| Name:dev_rtc_nxp_pca8565_open
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_rtc_nxp_pca8565_open(desc_t desc, int o_flag){
   //
   if(o_flag & O_RDONLY) {
   }

   if(o_flag & O_WRONLY) {
   }
   
   if(!ofile_lst[desc].p){
      ofile_lst[desc].p=&g_dev_rtc_nxp_pca8565_info;
   }

   ofile_lst[desc].offset = 0;
   return 0;
}

/*-------------------------------------------
| Name:dev_rtc_nxp_pca8565_close
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_rtc_nxp_pca8565_close(desc_t desc){
   return 0;
}

/*-------------------------------------------
| Name:dev_rtc_nxp_pca8565_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_rtc_nxp_pca8565_read(desc_t desc, char* buf,int cb){
   return cb;
}

/*-------------------------------------------
| Name:dev_rtc_nxp_pca8565_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_rtc_nxp_pca8565_write(desc_t desc, const char* buf,int cb){
   return cb;
}

/*-------------------------------------------
| Name:dev_rtc_nxp_pca8565_seek
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_rtc_nxp_pca8565_seek(desc_t desc,int offset,int origin){
   switch(origin) {

      case SEEK_SET:
         if(offset>=NXP_PCA8565_MEMORY_SIZE)
            return -1;
         ofile_lst[desc].offset=offset;
         break;

      case SEEK_CUR:
         if(ofile_lst[desc].offset+offset>=NXP_PCA8565_MEMORY_SIZE)
            return -1;
         ofile_lst[desc].offset+=offset;
         break;

      case SEEK_END:
         //to do: warning in SEEK_END (+ or -)????
         if(ofile_lst[desc].offset+offset>=NXP_PCA8565_MEMORY_SIZE)
            return -1;
         ofile_lst[desc].offset+=offset;
         break;
   }

   return ofile_lst[desc].offset;
}

/*-------------------------------------------
| Name:dev_rtc_nxp_pca8565_ioctl
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_rtc_nxp_pca8565_ioctl(desc_t desc,int request,va_list ap) {
    dev_rtc_nxp_pca8565_info_t * p_dev_rtc_nxp_pca8565_info = (dev_rtc_nxp_pca8565_info_t*)ofile_lst[desc].p;

   //
   switch(request) {
      //
      case I_LINK:{
#if 0
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
        
         if(!p_dev_rtc_nxp_pca8565_info)
            return -1;
   
         //
         for(i=I2CADDR_ARGC; i<argc; i++) {
           
            switch(i){
               case I2CADDR_ARGC:
                  if(argv[i][0]!='0' || argv[i][1]!='x')
                     return -1;
                  //
                  i2c_addr=htoi(argv[i][2])*16+htoi(argv[i][3]);
                  //
                  p_dev_rtc_nxp_pca8565_info->i2c_addr=i2c_addr;
                  
               break;
               
               default:
                  //nothing to do
               break;
            }
         }
#endif
      }
      return 0;
      
      //   
      case I_UNLINK:
        //nothing to do, see _vfs_ioctl2() 
        //ioctl of device driver is call with request I_LINK, I_UNLINK.
      return 0;   

      
      case RTC_NXP_PCA8565_SETALRM:{
         int hour = va_arg(ap, int);
         int min  = va_arg(ap, int);
         int sec  = va_arg(ap, int);
         return dev_rtc_nxp_pca8565_setalarm(desc, hour, min, sec);
      }   
      break;
      //
      default:
         return -1;

   }

   return 0;   
}


/** @} */
/** @} */
/** @} */

/*===========================================
End of Sourcedev_rtc_nxp_pca8565.c
=============================================*/
