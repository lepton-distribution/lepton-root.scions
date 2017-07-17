/*
The contents of this file are subject to the Mozilla Public License Version 1.1 
(the "License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at http://www.mozilla.org/MPL/

Software distributed under the License is distributed on an "AS IS" basis, 
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for the 
specific language governing rights and limitations under the License.

The Original Code is Lepton.

The Initial Developer of the Original Code is Philippe Le Boulanger.
Portions created by Philippe Le Boulanger are Copyright (C) 2011 <lepton.phlb@gmail.com>.
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

#include "kernel/core/kernelconf.h"
#include "kernel/core/types.h"
#include "kernel/core/dirent.h"
#include "kernel/core/stat.h"
#include "kernel/core/interrupt.h"
#include "kernel/core/kernel.h"
#include "kernel/core/system.h"
#include "kernel/core/ioctl.h"
#include "kernel/core/fcntl.h"
#include "kernel/core/cpu.h"
#include "kernel/fs/vfs/vfstypes.h"

#include "board.h"
#include "libsdmmc.h"
#include "Media.h"
#include "MEDSdcard.h"

#include "kernel/dev/arch/cortexm/at91samv7x/dev_samv7x/dev_samv7x_cpu_x.h"
#include "kernel/dev/arch/cortexm/at91samv7x/dev_samv7x/dev_samv7x_hsmci_x.h"

/*===========================================
Global Declaration
=============================================*/
static const char dev_samv71xplained_ultra_sdio_name[]="sdio0\0";

static int dev_samv71xplained_ultra_sdio_load(void);
static int dev_samv71xplained_ultra_sdio_open(desc_t desc, int o_flag);


dev_map_t dev_samv71xplained_ultra_sdio_map={
   dev_samv71xplained_ultra_sdio_name,
   S_IFBLK,
   dev_samv71xplained_ultra_sdio_load,
   dev_samv71xplained_ultra_sdio_open,
   dev_samv71x_hsmci_x_close,
   dev_samv71x_hsmci_x_isset_read,
   dev_samv71x_hsmci_x_isset_write,
   dev_samv71x_hsmci_x_read,
   dev_samv71x_hsmci_x_write,
   dev_samv71x_hsmci_x_seek,
   dev_samv71x_hsmci_x_ioctl
};


/** SD card pins instance. */
static const Pin pinsSd[] = {BOARD_MCI_PINS_SLOTA, BOARD_MCI_PIN_CK};

/** SD card detection pin instance. */
static const Pin pinsCd[] = {BOARD_MCI_PIN_CD};

//
COMPILER_ALIGNED(32) static samv71x_hsmci_info_t samv71xplained_ultra_hsmci_sdio_info; 
//
COMPILER_SECTION(".ram_nocache")
COMPILER_ALIGNED(32) static uint8_t rw_sdio_dma_buffer[512];


/*===========================================
Implementation
=============================================*/


/*-------------------------------------------
| Name:dev_samv71xplained_ultra_sdio_load
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_samv71xplained_ultra_sdio_load(void) {
   
   //
   samv71xplained_ultra_hsmci_sdio_info.base_sd_pins = &pinsSd[0];
   samv71xplained_ultra_hsmci_sdio_info.base_sdcd_pins = &pinsCd[0];
   //
   samv71xplained_ultra_hsmci_sdio_info.desc_r=-1;
   samv71xplained_ultra_hsmci_sdio_info.desc_w=-1;
   //
   samv71xplained_ultra_hsmci_sdio_info.rw_aligned_dma_buffer_size= sizeof(rw_sdio_dma_buffer);
   samv71xplained_ultra_hsmci_sdio_info.p_rw_aligned_dma_buffer = rw_sdio_dma_buffer;
   //
   PIO_Configure(pinsSd, PIO_LISTSIZE(pinsSd));
   PIO_Configure(pinsCd, PIO_LISTSIZE(pinsCd));
   
   return dev_samv71x_hsmci_x_load(&samv71xplained_ultra_hsmci_sdio_info);
}
   
/*-------------------------------------------
| Name:dev_samv71xplained_ultra_sdio_open
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static int dev_samv71xplained_ultra_sdio_open(desc_t desc, int o_flag) {
   return dev_samv71x_hsmci_x_open(desc, o_flag, &samv71xplained_ultra_hsmci_sdio_info);
}

/*============================================
| End of Source  : dev_samv71xplained_ultra_sdio.c
==============================================*/