/*
The contents of this file are subject to the Mozilla Public License Version 1.1
(the "License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at http://www.mozilla.org/MPL/

Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for the
specific language governing rights and limitations under the License.

The Original Code is Lepton.

The Initial Developer of the Original Code is Philippe Le Boulanger.
Portions created by Philippe Le Boulanger are Copyright (C) 2011. All Rights Reserved.

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
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "kernel/core/types.h"
#include "kernel/core/interrupt.h"
#include "kernel/core/system.h"
#include "kernel/core/stat.h"
#include "kernel/core/fcntl.h"

#include "kernel/fs/vfs/vfsdev.h"
#include "kernel/fs/vfs/vfstypes.h"

#include "kernel/core/ioctl.h"
#include "kernel/core/ioctl_lcd.h"
#include "kernel/core/ioctl_spi.h"
#include "kernel/core/ioctl_fb.h"

/*===========================================
Global Declaration
=============================================*/
const char dev_win32_lcd_matrix_name[] = "lcd0";

int dev_win32_lcd_matrix_load(void);
int dev_win32_lcd_matrix_open(desc_t desc, int o_flag);
int dev_win32_lcd_matrix_close(desc_t desc);
int dev_win32_lcd_matrix_isset_read(desc_t desc);
int dev_win32_lcd_matrix_isset_write(desc_t desc);
int dev_win32_lcd_matrix_read(desc_t desc, char* buf, int size);
int dev_win32_lcd_matrix_write(desc_t desc, const char* buf, int size);
int dev_win32_lcd_matrix_seek(desc_t desc, off_t offset, int origin);
int dev_win32_lcd_matrix_ioctl(desc_t desc, int request, va_list ap);


dev_map_t dev_win32_lcd_matrix_map = {
   dev_win32_lcd_matrix_name,
   S_IFBLK,
   dev_win32_lcd_matrix_load,
   dev_win32_lcd_matrix_open,
   dev_win32_lcd_matrix_close,
   __fdev_not_implemented,
   __fdev_not_implemented,
   dev_win32_lcd_matrix_read,
   dev_win32_lcd_matrix_write,
   dev_win32_lcd_matrix_seek,
   dev_win32_lcd_matrix_ioctl //ioctl
};

#define LCD_MATRIX_DISPLAY_X_SIZE 256   //pixels
#define LCD_MATRIX_DISPLAY_Y_SIZE  64   //pixels
#define LCD_MATRIX_DISPLAY_BPP 4 //4 bits/pixel

#define LCD_MATRIX_DISPLAY_DISPLAY_SIZE  (LCD_MATRIX_DISPLAY_X_SIZE*LCD_MATRIX_DISPLAY_Y_SIZE)


#define BUF_SIZE (unsigned long)((float)(LCD_MATRIX_DISPLAY_X_SIZE*LCD_MATRIX_DISPLAY_Y_SIZE) * (float)( (float)LCD_MATRIX_DISPLAY_BPP/(float)(sizeof(unsigned char)*8.0) ))


HANDLE hLcdMatrixSemaphore=INVALID_HANDLE_VALUE;
HANDLE hLcdMatrixMapFile = INVALID_HANDLE_VALUE;
unsigned char *pLcdMatrixMapViewOfFileBuffer;

static desc_t lcd_matrix_display_desc_w = -1;
static unsigned char lcd_matrix_display_frame_buffer[BUF_SIZE] = {0};

static int refreshStatus = FALSE;
/*===========================================
Implementation
=============================================*/

/*-------------------------------------------
| Name:win32_lcd_matrix_frame_buffer_flush
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int win32_lcd_matrix_frame_buffer_flush() {
   BOOL bSucceed;
   LONG lPreviousCount;
   DWORD dwError;
   
   //flush could be call directly without write if app layer use video buffer ioctl(,LCDGETVADDR,)
   if (win32_lcd_matrix_connect2simulation() < 0)
      return -1;

   //
   CopyMemory((PVOID)pLcdMatrixMapViewOfFileBuffer, lcd_matrix_display_frame_buffer, sizeof(lcd_matrix_display_frame_buffer));
   //
   bSucceed = ReleaseSemaphore(hLcdMatrixSemaphore, 1, &lPreviousCount);
   dwError = GetLastError();

   return 0;
}

/*-------------------------------------------
| Name:win32_lcd_matrix_connect2simulation
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int win32_lcd_matrix_connect2simulation() {
   BOOL bSucceed;
   LONG lPreviousCount;
   DWORD dwError;
   int frame_buffer_size = BUF_SIZE;

   //already connected?
   if (hLcdMatrixSemaphore != INVALID_HANDLE_VALUE && hLcdMatrixMapFile != INVALID_HANDLE_VALUE) {
      return 0;
   }

   //not connected
   if (hLcdMatrixSemaphore == INVALID_HANDLE_VALUE) {

      hLcdMatrixSemaphore = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, TRUE, "testmapsemaphore");
      if (hLcdMatrixSemaphore == INVALID_HANDLE_VALUE) {
         dwError = GetLastError();
         return -1;
      }
   }

   //
   if (hLcdMatrixMapFile == INVALID_HANDLE_VALUE) {
      hLcdMatrixMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, "MyFileMappingObject");
      if (hLcdMatrixMapFile == INVALID_HANDLE_VALUE) {
         dwError = GetLastError();
         return -1;
      }
   }

   //
   pLcdMatrixMapViewOfFileBuffer = (LPTSTR)MapViewOfFile(hLcdMatrixMapFile, FILE_MAP_ALL_ACCESS, 0, 0, BUF_SIZE);

   //clear screen
   memset(lcd_matrix_display_frame_buffer, 0x00, sizeof(lcd_matrix_display_frame_buffer));
   //
   CopyMemory((PVOID)pLcdMatrixMapViewOfFileBuffer, lcd_matrix_display_frame_buffer, sizeof(lcd_matrix_display_frame_buffer));
   //
   bSucceed = ReleaseSemaphore(hLcdMatrixSemaphore, 1, &lPreviousCount);
   dwError = GetLastError();
   //
   if (bSucceed == 0) {
      return -1;
   }
   //
   return 0;
}

/*-------------------------------------------
| Name:dev_win32_lcd_matrix_load
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_win32_lcd_matrix_load(void) {

   return 0;
}

/*-------------------------------------------
| Name:dev_win32_lcd_matrix_open
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_win32_lcd_matrix_open(desc_t desc, int o_flag) {

   //
   if (o_flag & O_RDONLY) {
     
   }

   if (o_flag & O_WRONLY) {

   }

   ofile_lst[desc].offset = 0;

   return 0;
}

/*-------------------------------------------
| Name:dev_win32_lcd_matrix_close
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_win32_lcd_matrix_close(desc_t desc) {
   if (ofile_lst[desc].oflag & O_WRONLY) {
      if (!ofile_lst[desc].nb_writer) {
        
      }
   }

   return 0;
}

/*-------------------------------------------
| Name:dev_win32_lcd_matrix_isset_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_win32_lcd_matrix_isset_read(desc_t desc) {
   return -1;
}

/*-------------------------------------------
| Name:dev_win32_lcd_matrix_isset_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_win32_lcd_matrix_isset_write(desc_t desc) {
   return -1;
}

/*-------------------------------------------
| Name:dev_win32_lcd_matrix_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_win32_lcd_matrix_read(desc_t desc, char* buf, int size) {
   return 0;
}

/*-------------------------------------------
| Name:dev_win32_lcd_matrix_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_win32_lcd_matrix_write(desc_t desc, const char* buf, int size) {
   int i;
   int cb = 0;
   //
   if (win32_lcd_matrix_connect2simulation() < 0)
      return -1;
   //
   if ((ofile_lst[desc].offset + size) > sizeof(lcd_matrix_display_frame_buffer)) {
      size = sizeof(lcd_matrix_display_frame_buffer) - ofile_lst[desc].offset;
   }

   //
   for (i = 0; i<size; i++) {
      lcd_matrix_display_frame_buffer[ofile_lst[desc].offset + i] = buf[i];
      cb++;
   }

   return cb;
}

/*-------------------------------------------
| Name:dev_win32_lcd_matrix_seek
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_win32_lcd_matrix_seek(desc_t desc, off_t offset, int origin) {
   switch (origin) {

   case SEEK_SET:
      ofile_lst[desc].offset = offset;
      break;

   case SEEK_CUR:
      ofile_lst[desc].offset += offset;
      break;

   case SEEK_END:
      //to do: warning in SEEK_END (+ or -)????
      ofile_lst[desc].offset += offset;
      break;
   }

   return ofile_lst[desc].offset;
}

/*--------------------------------------------
| Name:        dev_win32_lcd_matrix_ioctl
| Description:
| Parameters:  none
| Return Type: none
| Comments:
| See:
----------------------------------------------*/
static int dev_win32_lcd_matrix_ioctl(desc_t desc, int request, va_list ap) {

   switch (request) {

      case I_LINK: {
      return 0;
      }
      break;

      //
      case I_UNLINK: {
      }
      break;

      //flush internal buffer of lcd device driver
      case LCDFLSBUF:
         win32_lcd_matrix_frame_buffer_flush();
      break;

      //get physical video buffer address
      case LCDGETVADDR: {
         unsigned long* vaddr = va_arg(ap, unsigned long*);
         *vaddr = (unsigned long)&lcd_matrix_display_frame_buffer[0];
      }
      break;

      //
      case LCDGETCONTRAST: {
         unsigned int* p_v = va_arg(ap, unsigned int*);
         *p_v = 0;
      }
      break;
      //
      case LCDGETLUMINOSITY: {
         unsigned int* p_v = va_arg(ap, unsigned int*);
         *p_v = 0;
      }
      break;
      //
      case LCDSETCONTRAST: {
         unsigned int p_v = va_arg(ap, int);
         //to do
      }
      break;
      //
      case LCDSETLUMINOSITY: {
         unsigned int p_v = va_arg(ap, int);
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
         fbcmap_t** fbcmap = va_arg(ap, fbcmap_t**);
         //same fields
         *fbcmap_len = 0;
         *fbcmap = (fbcmap_t*)0;
      }
      break;

      case FBIOPUTCMAP: {
         fbcmap_t* fbcmap = va_arg(ap, fbcmap_t*);
         // at91sam9261_lcd_set_lut((const PALETTEENTRY *)fbcmap);
         }
      break;

      case FBIOGET_DISPINFO: {
         fb_info_t * fb_info = va_arg(ap, fb_info_t*);
         if (!fb_info)
         return -1;
         //
         fb_info->x_res = LCD_MATRIX_DISPLAY_X_SIZE;    //x res
         fb_info->y_res = LCD_MATRIX_DISPLAY_Y_SIZE;    //y res
         //
         fb_info->bpp = LCD_MATRIX_DISPLAY_BPP;
         //
         fb_info->line_len = LCD_MATRIX_DISPLAY_X_SIZE;    //line length

         fb_info->smem_start = (unsigned long)&lcd_matrix_display_frame_buffer[0];    //addr of framebuffer
         fb_info->smem_len = sizeof(lcd_matrix_display_frame_buffer);    //size of framebuffer

         fb_info->cmap = (fbcmap_t*)0;    //color map from screen
         fb_info->cmap_len = 0;   //color map length

         fb_info->desc_w = -1;
         fb_info->next = (void*)0;    //next framebuffer data
      }
      break;

      //
      default:
      return -1;

   }

   return 0;
}


/*============================================
| End of Source  : dev_win32_lcd_matrix.c
==============================================*/
