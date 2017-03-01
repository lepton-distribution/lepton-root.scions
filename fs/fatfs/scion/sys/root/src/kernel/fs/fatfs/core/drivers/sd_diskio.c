/**
  ******************************************************************************
  * @file    sd_diskio.c
  * @author  MCD Application Team
  * @version V1.3.0
  * @date    08-May-2015
  * @brief   SD Disk I/O driver
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2015 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>


#include "kernel/core/errno.h"
#include "kernel/core/kernel.h"
#include "kernel/core/dirent.h"
#include "kernel/core/system.h"
#include "kernel/core/systime.h"
#include "kernel/core/stat.h"
#include "kernel/core/fcntl.h"

#include "kernel/core/kernel_io.h"
#include "kernel/core/ioctl.h"
#include "kernel/core/ioctl_hd.h"

#include "kernel/fs/vfs/vfstypes.h"
#include "kernel/fs/vfs/vfsdev.h"
#include "kernel/fs/vfs/vfs.h"

#include "kernel/fs/fatfs/fatfs.h"
#include "kernel/fs/fatfs/core/integer.h"	/* Basic integer types */
#include "kernel/fs/fatfs/core/ffconf.h"		/* FatFs configuration options */
#include "kernel/fs/fatfs/core/diskio.h"		
#include "kernel/fs/fatfs/core/ff_gen_drv.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Block Size in Bytes */
#define BLOCK_SIZE                512

/* Private variables ---------------------------------------------------------*/
/* Disk status */
static volatile DSTATUS Stat = STA_NOINIT;
static volatile desc_t g_desc_sdcard0 = INVALID_DESC;


/* Private function prototypes -----------------------------------------------*/
DSTATUS SD_initialize (BYTE);
DSTATUS SD_status (BYTE);
DRESULT SD_read (BYTE, BYTE*, DWORD, UINT);
#if _USE_WRITE == 1
  DRESULT SD_write (BYTE, const BYTE*, DWORD, UINT);
#endif /* _USE_WRITE == 1 */
#if _USE_IOCTL == 1
  DRESULT SD_ioctl (BYTE, BYTE, void*);
#endif  /* _USE_IOCTL == 1 */
  
const Diskio_drvTypeDef  SD_Driver =
{
  SD_initialize,
  SD_status,
  SD_read, 
#if  _USE_WRITE == 1
  SD_write,
#endif /* _USE_WRITE == 1 */
  
#if  _USE_IOCTL == 1
  SD_ioctl,
#endif /* _USE_IOCTL == 1 */
};

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Initializes a Drive
  * @param  lun : not used 
  * @retval DSTATUS: Operation status
  */
DSTATUS SD_initialize(BYTE lun)
{
   uint64_t sdcard_capacity;
   uint32_t sdcard_blocksize;
   //
   Stat = STA_NOINIT;
   //
   g_desc_sdcard0 = _vfs_open("/dev/sdio0",O_RDWR,0);
   if(g_desc_sdcard0<0){
      return Stat;
   }
   //
   if(kernel_io_ll_ioctl(g_desc_sdcard0,HDGETSZ,&sdcard_capacity)<0){
      return Stat;
   }
   //
   if(kernel_io_ll_ioctl(g_desc_sdcard0,HDGETSCTRSZ,0,&sdcard_blocksize)<0){
      return Stat;
   }
   //
   Stat &= ~STA_NOINIT;
   //
   return Stat;
}

/**
  * @brief  Gets Disk Status
  * @param  lun : not used
  * @retval DSTATUS: Operation status
  */
DSTATUS SD_status(BYTE lun)
{
   Stat = STA_NOINIT;
   //
   if(g_desc_sdcard0==INVALID_DESC){
      return Stat;
   }
   //
   Stat &= ~STA_NOINIT;
   //
   return Stat;
}

/**
  * @brief  Reads Sector(s)
  * @param  lun : not used
  * @param  *buff: Data buffer to store read data
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to read (1..128)
  * @retval DRESULT: Operation result
  */
DRESULT SD_read(BYTE lun, BYTE *buff, DWORD sector, UINT count)
{
   //
   if(g_desc_sdcard0==INVALID_DESC){
      return (RES_ERROR);
   }
   //
   if(kernel_io_ll_lseek(g_desc_sdcard0, (off_t) (sector *  BLOCK_SIZE),SEEK_SET)<0){
       return (RES_ERROR);
   }
   
   //
   if(kernel_io_ll_read(g_desc_sdcard0, buff,(count* BLOCK_SIZE))<0){
       return (RES_ERROR);
   }
   //
   return RES_OK;
}

/**
  * @brief  Writes Sector(s)
  * @param  lun : not used
  * @param  *buff: Data to be written
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to write (1..128)
  * @retval DRESULT: Operation result
  */
#if _USE_WRITE == 1
DRESULT SD_write(BYTE lun, const BYTE *buff, DWORD sector, UINT count)
{
  //
   if(g_desc_sdcard0==INVALID_DESC){
      return (RES_ERROR);
   }
   //
   if(kernel_io_ll_lseek(g_desc_sdcard0, (off_t) (sector *  BLOCK_SIZE),SEEK_SET)<0){
       return (RES_ERROR);
   }
   
   //
   if(kernel_io_ll_write(g_desc_sdcard0, buff,(count* BLOCK_SIZE))<0){
       return (RES_ERROR);
   }
   //
   return RES_OK;
}
#endif /* _USE_WRITE == 1 */

/**
  * @brief  I/O control operation
  * @param  lun : not used
  * @param  cmd: Control code
  * @param  *buff: Buffer to send/receive control data
  * @retval DRESULT: Operation result
  */
#if _USE_IOCTL == 1
DRESULT SD_ioctl(BYTE lun, BYTE cmd, void *buff)
{
   DRESULT res = RES_ERROR;
   uint64_t sdcard_capacity;
   uint32_t sdcard_blocksize;

   //
   if (Stat & STA_NOINIT) 
     return RES_NOTRDY;
  
  
   //
   if(g_desc_sdcard0==INVALID_DESC){
      return (RES_NOTRDY);
   }
   //
   if(kernel_io_ll_ioctl(g_desc_sdcard0,HDGETSZ,&sdcard_capacity)<0){
      return (RES_NOTRDY);
   }
   //
   if(kernel_io_ll_ioctl(g_desc_sdcard0,HDGETSCTRSZ,0,&sdcard_blocksize)<0){
      return (RES_NOTRDY);
   }
  
   switch (cmd)
   {
      /* Make sure that no pending write process */
      case CTRL_SYNC :
         res = RES_OK;
      break;

      /* Get number of sectors on the disk (DWORD) */
      case GET_SECTOR_COUNT :
         *(DWORD*)buff = sdcard_capacity / sdcard_blocksize;
         res = RES_OK;
      break;

      /* Get R/W sector size (WORD) */
      case GET_SECTOR_SIZE :
         *(WORD*)buff = sdcard_blocksize;
         res = RES_OK;
      break;

      /* Get erase block size in unit of sector (DWORD) */
      case GET_BLOCK_SIZE :
         *(DWORD*)buff = sdcard_blocksize;
      break;

      default:
         res = RES_PARERR;
   }
  
  return res;
}
#endif /* _USE_IOCTL == 1 */
  
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

