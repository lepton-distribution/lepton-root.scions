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
| Includes
==============================================*/
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>

#include "kernel/core/types.h"
#include "kernel/core/interrupt.h"
#include "kernel/core/kernel.h"
#include "kernel/core/system.h"
#include "kernel/core/malloc.h"
#include "kernel/core/fcntl.h"
#include "kernel/fs/vfs/vfs.h"
#include "kernel/fs/vfs/vfstypes.h"
#include "kernel/core/stat.h"
#include "kernel/core/ioctl.h"
#include "kernel/core/ioctl_lcd.h"
#include "kernel/core/ioctl_spi.h"
#include "kernel/core/ioctl_fb.h"

#include "kernel/dev/arch/all/lcd/dev_oled_ssd1322.h"


/*============================================
| Global Declaration
==============================================*/
//
static const char dev_oled_ssd1322_name[]="lcd0\0";

int dev_oled_ssd1322_load(void);
int dev_oled_ssd1322_open(desc_t desc, int o_flag);
int dev_oled_ssd1322_close(desc_t desc);
int dev_oled_ssd1322_isset_read(desc_t desc);
int dev_oled_ssd1322_isset_write(desc_t desc);
int dev_oled_ssd1322_read(desc_t desc, char* buf,int size);
int dev_oled_ssd1322_write(desc_t desc, const char* buf,int size);
int dev_oled_ssd1322_seek(desc_t desc,int offset,int origin);
int dev_oled_ssd1322_ioctl(desc_t desc,int request,va_list ap);


dev_map_t dev_oled_ssd1322_map={
   dev_oled_ssd1322_name,
   S_IFBLK,
   dev_oled_ssd1322_load,
   dev_oled_ssd1322_open,
   dev_oled_ssd1322_close,
   __fdev_not_implemented,
   __fdev_not_implemented,
   dev_oled_ssd1322_read,
   dev_oled_ssd1322_write,
   dev_oled_ssd1322_seek,
   dev_oled_ssd1322_ioctl
};



#define ENABLE_GRAYSCALE_TABLE   0x00
#define SET_COLUMN_ADDRESS			0x15
#define WRITE_RAM_COMMAND			0x5c
#define READ_RAM_COMMAND			0x5d
#define SET_ROW_ADDRESS				0x75
#define SET_DISPLAY_START_LINE   0xa1
#define SET_DISPLAY_OFFSET			0xa2
#define SET_DISPLAY_MODE_OFF		0xa4
#define SET_DISPLAY_MODE_ON	   0xa5
#define SET_DISPLAY_MODE_NORMAL	0xa6
#define SET_DISPLAY_MODE_INVERSE	0xa7
#define ENABLE_PARTIAL_DISPLAY	0xa8
#define EXIT_PARTIAL_DISPLAY		0xa9
#define FUNCTION_SELECTION			0xab
#define SLEEP_MODE_ON				0xae
#define SLEEP_MODE_OFF				0xaf
#define SET_PHASE_LENGTH			0xb1
#define SET_FRONT_CLOCK_DIVIDER	0xb3
#define SET_GPIO					   0xb5
#define SET_SECOND_PRECHARGE_PERIOD	0xb6
#define SET_GRAY_SCALE_TABLE		0xb8
#define SELECT_DEFAULT_GRAYSCALE	0xb9
#define SET_PRECHARGE_VOLTAGE		0xbb
#define SET_VCOMH					   0xbe
#define SET_CONTRAST_CURRENT		0xc1
#define MASTER_CONTRAST_CURRENT	0xc7
#define SET_MUX_RATIO				0xca
#define SET_COMMAND_LOCK			0xfd

#define OLED_DISPLAY_X_SIZE  256
#define OLED_DISPLAY_Y_SIZE   64
#define OLED_DISPLAY_BPP       4  //(4bits/pixel)

#define OLED_DISPLAY_SIZE     (OLED_DISPLAY_X_SIZE*OLED_DISPLAY_Y_SIZE)

typedef struct link_info_st{
  desc_t desc_oled_io;
  desc_t desc_spi_w;
  desc_t desc_w;
  uint8_t * p_fbuf;
}link_info_t;

static link_info_t link_info_oled_ssd1322;


static void ssd1322_reset(desc_t desc_w){   
   uint8_t v8;  
   desc_t desc_oled_io = link_info_oled_ssd1322.desc_oled_io;
   va_list ap={0};
   //
   if(desc_oled_io <0){
      return;
   }
   // RESET
   ofile_lst[desc_oled_io].pfsop->fdev.fdev_ioctl(desc_oled_io,IOCTL_OLED_SSD1322_IO_RESET,ap);
}

static void ssd1322_write_command(desc_t desc_w, uint8_t cmd){   
   uint8_t v8;  
   desc_t desc_oled_io = link_info_oled_ssd1322.desc_oled_io;
   desc_t desc_spi_w = link_info_oled_ssd1322.desc_spi_w;
   va_list ap={0};
   
   //
   if(desc_oled_io <0 || desc_spi_w<0){
      return;
   }

	//CS=0;
   //DC=0;
   ofile_lst[desc_oled_io].pfsop->fdev.fdev_ioctl(desc_oled_io,IOCTL_OLED_SSD1322_IO_CMD_ENTER,ap);
	
   //send SPI data
   v8 = cmd;
   ofile_lst[desc_spi_w].pfsop->fdev.fdev_write(desc_spi_w,(const char *)&v8,sizeof(v8));
	
   //DC=1;
	//CS=1;
   ofile_lst[desc_oled_io].pfsop->fdev.fdev_ioctl(desc_oled_io,IOCTL_OLED_SSD1322_IO_LEAVE,ap);
}

//
static void ssd1322_write_data(desc_t desc_w, uint8_t *data, int len){
	
   desc_t desc_oled_io = link_info_oled_ssd1322.desc_oled_io;
   desc_t desc_spi_w = link_info_oled_ssd1322.desc_spi_w;
   va_list ap={0};
   
   //
   if(desc_oled_io <0 || desc_spi_w<0){
      return;
   }

	//CS=0;
   //DC=1;
   ofile_lst[desc_oled_io].pfsop->fdev.fdev_ioctl(desc_oled_io,IOCTL_OLED_SSD1322_IO_DATA_ENTER,ap);
	
   //send SPI data
   ofile_lst[desc_spi_w].pfsop->fdev.fdev_write(desc_spi_w,(const char *)data,len);
	
   //DC=1;
	//CS=1;
   ofile_lst[desc_oled_io].pfsop->fdev.fdev_ioctl(desc_oled_io,IOCTL_OLED_SSD1322_IO_LEAVE,ap);
}

//
static void ssd1322_sleep(desc_t desc_w){
	ssd1322_write_command(desc_w, SLEEP_MODE_ON);
}

//
static void ssd1322_unsleep(desc_t desc_w){
	ssd1322_write_command(desc_w, SLEEP_MODE_OFF);
}

void ssd1322_set_gray_scale_table(desc_t desc_w)
{  
   uint8_t data[]={0x0C,0x18,0x24,0x30,0x3C,0x48,0x54,0x60,0x6C,0x78,0x84,0x90,0x9C,0xA8,0xB4};
	//
   ssd1322_write_command(desc_w,0xB8);			// Set Gray Scale Table
	//
	ssd1322_write_data(desc_w,data,sizeof(data));
   //
	ssd1322_write_command(desc_w,0x00);			// Enable Gray Scale Table
}

static int ssd1322_init(desc_t desc_w)
{
	uint8_t data[3];
	int i;
   
   ssd1322_reset(desc_w);

	ssd1322_write_command(desc_w, SET_COMMAND_LOCK);
	data[0] = 0x12;
	ssd1322_write_data(desc_w, &data[0], 1);

	ssd1322_write_command(desc_w, SLEEP_MODE_ON);

	ssd1322_write_command(desc_w, SET_FRONT_CLOCK_DIVIDER);
	data[0] = 0x91; /*0x91*/;
	ssd1322_write_data(desc_w, &data[0], 1);

	ssd1322_write_command(desc_w, SET_MUX_RATIO);
	data[0] = 0x3f;
	ssd1322_write_data(desc_w, &data[0], 1);

	ssd1322_write_command(desc_w, SET_DISPLAY_OFFSET);
	data[0] = 0x00;
	ssd1322_write_data(desc_w, &data[0], 1);

	ssd1322_write_command(desc_w, SET_DISPLAY_START_LINE);
	data[0] = 0x00;
	ssd1322_write_data(desc_w, &data[0], 1);
   
 
	ssd1322_write_command(desc_w, 0xa0);
	data[0] = 0x14;
	data[1] = 0x11;
	ssd1322_write_data(desc_w, &data[0], 2);

	ssd1322_write_command(desc_w, SET_GPIO);
	data[0] = 0x00;
	ssd1322_write_data(desc_w, &data[0], 1);

	ssd1322_write_command(desc_w, FUNCTION_SELECTION);
	data[0] = 0x01;
	ssd1322_write_data(desc_w, &data[0], 1);

	ssd1322_write_command(desc_w, 0xb4);
	data[0] = 0xa0;
	data[1] = 0xfd;
	ssd1322_write_data(desc_w, &data[0], 2);

   ssd1322_write_command(desc_w, MASTER_CONTRAST_CURRENT);
	data[0] = 0x0f;
	ssd1322_write_data(desc_w, &data[0], 1);
   
	ssd1322_write_command(desc_w, SET_CONTRAST_CURRENT);
	data[0] = 0x9f;
	ssd1322_write_data(desc_w, &data[0], 1);

	ssd1322_write_command(desc_w, SELECT_DEFAULT_GRAYSCALE);
   //ssd1322_set_gray_scale_table(desc_w);

	ssd1322_write_command(desc_w, SET_PHASE_LENGTH);
	data[0] = 0xe2; /*0xe2*/;
	ssd1322_write_data(desc_w, &data[0], 1);

	ssd1322_write_command(desc_w, 0xd1);
	data[0] = 0x82;
	data[1] = 0x20;
	ssd1322_write_data(desc_w, &data[0], 2);

	ssd1322_write_command(desc_w, SET_PRECHARGE_VOLTAGE);
	data[0] = 0x1f;
	ssd1322_write_data(desc_w, &data[0], 1);

	ssd1322_write_command(desc_w, SET_SECOND_PRECHARGE_PERIOD);
	data[0] = 0x08; /*0x08*/;
	ssd1322_write_data(desc_w, &data[0], 1);

	ssd1322_write_command(desc_w, SET_VCOMH);
	data[0] = 0x07;
	ssd1322_write_data(desc_w, &data[0], 1);

	ssd1322_write_command(desc_w, SET_DISPLAY_MODE_NORMAL);
   
   //ssd1322_write_command(desc_w, EXIT_PARTIAL_DISPLAY);
   
	ssd1322_write_command(desc_w, SLEEP_MODE_OFF);


   //
   //! Set the row
	ssd1322_write_command(desc_w, SET_ROW_ADDRESS);
	data[0] = 0x00;
	data[1] = 0x3f;//0x7F;//0x3f;
	ssd1322_write_data(desc_w, &data[0], 2);

	//! Set the column
	ssd1322_write_command(desc_w, SET_COLUMN_ADDRESS);
	data[0] = 0x1c;//0x00;//0x1c;
	data[1] = 0x1c+0x3F;//0x77;//0x5b;
	ssd1322_write_data(desc_w, &data[0], 2);
   
   //switch to RAM command
   ssd1322_write_command(desc_w, WRITE_RAM_COMMAND);
   
   
   //to remove test: fill all
   //
   //memset(link_info_oled_ssd1322.p_fbuf,0x11,OLED_DISPLAY_SIZE/2);
	//ssd1322_write_data(desc_w, link_info_oled_ssd1322.p_fbuf, OLED_DISPLAY_SIZE/2);
   //
   //__kernel_usleep(1000000); //1s 
   //
   //clean display 
   memset(link_info_oled_ssd1322.p_fbuf,0x00,OLED_DISPLAY_SIZE/2);
	ssd1322_write_data(desc_w, link_info_oled_ssd1322.p_fbuf, OLED_DISPLAY_SIZE/2);
   //
   __kernel_usleep(1000000);
   
   //
#if 0
   memset(link_info_oled_ssd1322.p_fbuf,0x11,1);
   memset(link_info_oled_ssd1322.p_fbuf+(256/2),0x11,1);
   memset(link_info_oled_ssd1322.p_fbuf+2*(256/2),0x11,1);
   memset(link_info_oled_ssd1322.p_fbuf+3*(256/2),0x11,1);
   memset(link_info_oled_ssd1322.p_fbuf+4*(256/2),0x11,1);
   ssd1322_write_data(desc_w, link_info_oled_ssd1322.p_fbuf, OLED_DISPLAY_SIZE/2); 
#endif

	return 0;
}




/*-------------------------------------------
| Name:dev_oled_ssd1322_load
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_oled_ssd1322_load(void){
   link_info_oled_ssd1322.desc_oled_io=-1;
   link_info_oled_ssd1322.desc_spi_w=-1;
   link_info_oled_ssd1322.p_fbuf= (void*)0;
     
   return 0;
}

/*-------------------------------------------
| Name:dev_oled_ssd1322_open
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_oled_ssd1322_open(desc_t desc, int o_flag){

   //
   if(o_flag & O_RDONLY) 
   {
   }

   if(o_flag & O_WRONLY) {
   }

   ofile_lst[desc].offset=0;

   return 0;
}

/*-------------------------------------------
| Name:dev_oled_ssd1322_close
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_oled_ssd1322_close(desc_t desc){

   if(ofile_lst[desc].oflag & O_WRONLY) {
      if(!ofile_lst[desc].nb_writer) {
      }
   }

   return 0;
}

/*-------------------------------------------
| Name:dev_oled_ssd1322_isset_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_oled_ssd1322_isset_read(desc_t desc){
   return -1;
}

/*-------------------------------------------
| Name:dev_oled_ssd1322_isset_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_oled_ssd1322_isset_write(desc_t desc){
   return -1;
}

/*-------------------------------------------
| Name:dev_oled_ssd1322_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_oled_ssd1322_read(desc_t desc, char* buf,int size){
   return 0;
}

/*-------------------------------------------
| Name:dev_oled_ssd1322_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_oled_ssd1322_write(desc_t desc, const char* buf,int size){
   int cb=size;
   
   //
   if(ofile_lst[desc].offset+size>=(OLED_DISPLAY_SIZE/2))
      cb=((OLED_DISPLAY_SIZE/2)-ofile_lst[desc].offset);
   //
   memcpy(link_info_oled_ssd1322.p_fbuf+ofile_lst[desc].offset,buf,cb);
   //
	ssd1322_write_data(link_info_oled_ssd1322.desc_w, link_info_oled_ssd1322.p_fbuf, cb);
   //
   ofile_lst[desc].offset+=cb;
   //
   return cb;
}

/*-------------------------------------------
| Name:dev_oled_ssd1322_seek
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_oled_ssd1322_seek(desc_t desc,int offset,int origin){
   switch(origin) {

   case SEEK_SET:
      ofile_lst[desc].offset=offset;
      break;

   case SEEK_CUR:
      ofile_lst[desc].offset+=offset;
      break;

   case SEEK_END:
      ofile_lst[desc].offset+=offset;
      break;
   }

   return ofile_lst[desc].offset;
}

/*--------------------------------------------
| Name:        dev_oled_ssd1322_ioctl
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
int dev_oled_ssd1322_ioctl(desc_t desc,int request,va_list ap){

   switch(request) {
     
    case I_LINK: {
         desc_t desc_oled_io = INVALID_DESC;
         desc_t desc_spi_w = INVALID_DESC;

         //must be open in O_RDWR mode
         if((ofile_lst[desc].oflag&O_RDWR)!=O_RDWR)
            return -1;
         //
         desc_oled_io = ofile_lst[desc].desc_nxt[0];
         if(desc_oled_io==INVALID_DESC)
            return -1;
         //
         desc_spi_w = ofile_lst[desc_oled_io].desc_nxt[0];
         if(desc_spi_w==INVALID_DESC)
            return -1;
         //set info
         link_info_oled_ssd1322.desc_oled_io=desc_oled_io;
         link_info_oled_ssd1322.desc_spi_w=desc_spi_w;
         link_info_oled_ssd1322.desc_w=desc;
         //
         link_info_oled_ssd1322.p_fbuf = _sys_malloc(OLED_DISPLAY_X_SIZE*OLED_DISPLAY_Y_SIZE);
         //
         ofile_lst[desc].p=&link_info_oled_ssd1322;
         //
         ssd1322_init(desc);
         //test_lcd_main();
         //
         return 0;
      }
      break;

      //
      case I_UNLINK: {
      }
      break;
      
      //flush internal buffer of lcd device driver
      case LCDFLSBUF:
        ssd1322_write_data(link_info_oled_ssd1322.desc_w, link_info_oled_ssd1322.p_fbuf, OLED_DISPLAY_SIZE/2);
      //
      break;

      //get physical video buffer address
      case LCDGETVADDR: {
         unsigned long* vaddr= va_arg( ap, unsigned long*);
         *vaddr=(unsigned long)link_info_oled_ssd1322.p_fbuf;
      }
      break;


      case LCDGETCONTRAST: {
         unsigned int* p_v= va_arg( ap, unsigned int*);
         *p_v = 0;
      }
      break;

      case LCDGETLUMINOSITY: {
         unsigned int* p_v= va_arg( ap, unsigned int*);
         *p_v = 0;
      }
      break;

      case LCDSETCONTRAST: {
         unsigned int p_v= va_arg( ap, int);
         //to do
      }
      break;

      case LCDSETLUMINOSITY: {
         unsigned int p_v= va_arg( ap, int);
         //to do
      }
      break;

      case LCDSETBACKLIGHTON: {
      }
      break;

      case LCDSETBACKLIGHTOFF: {
      }
      break;

      //color map (palette)
      case FBIOGETCMAP: {
         unsigned int * fbcmap_len = va_arg(ap, unsigned int *);
         fbcmap_t** fbcmap= va_arg( ap, fbcmap_t**);
         //same fields
         *fbcmap_len = 0;
         *fbcmap = (fbcmap_t*)0;
      }
      break;

      case FBIOPUTCMAP: {
         fbcmap_t* fbcmap= va_arg( ap, fbcmap_t*);
        // at91sam9261_lcd_set_lut((const PALETTEENTRY *)fbcmap);
      }
      break;

      case FBIOGET_DISPINFO: {
         fb_info_t * fb_info = va_arg(ap, fb_info_t*);
         if(!fb_info)
            return -1;
         //
         fb_info->x_res=OLED_DISPLAY_X_SIZE;    //x res
         fb_info->y_res=OLED_DISPLAY_Y_SIZE;    //y res
         //
         fb_info->bpp=OLED_DISPLAY_BPP;
         //
         fb_info->line_len=OLED_DISPLAY_X_SIZE;    //line length

         fb_info->smem_start=(unsigned long)link_info_oled_ssd1322.p_fbuf;    //addr of framebuffer
         fb_info->smem_len=OLED_DISPLAY_SIZE/2;    //size of framebuffer

         fb_info->cmap=(fbcmap_t*)0;    //color map from screen
         fb_info->cmap_len=0;   //color map length

         fb_info->desc_w=-1;
         fb_info->next=(void*)0;    //next framebuffer data
      }
      break;



      //
      default:
         return -1;

   }

   return 0;
}

/*============================================
| End of Source  : dev_oled_ssd1322.c
==============================================*/

