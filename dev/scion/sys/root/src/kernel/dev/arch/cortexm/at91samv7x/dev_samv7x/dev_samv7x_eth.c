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
#include "kernel/core/fcntl.h"
#include "kernel/core/core_rttimer.h"
#include "kernel/fs/vfs/vfstypes.h"
#include "kernel/core/ioctl_eth.h"

#include "board.h"

/*===========================================
Global Declaration
=============================================*/

#define _mem_barrier_   __DSB();

//
#define TX_BUFFERS        32     /** Must be a power of 2 */
#define RX_BUFFERS        32     /** Must be a power of 2 */
#define BUFFER_SIZE        512
#define DUMMY_SIZE              2
#define DUMMY_BUFF_SIZE         512
//
#define GMAC_CAF_DISABLE  0
#define GMAC_CAF_ENABLE   1
#define GMAC_NBC_DISABLE  0
#define GMAC_NBC_ENABLE   1

/* The GMAC driver instance */
COMPILER_ALIGNED(32) static sGmacd gGmacd;
/* The GMACB driver instance */
COMPILER_ALIGNED(32) static GMacb gGmacb;

/** The PINs for GMAC */
static const Pin gmacPins[] = {BOARD_GMAC_RUN_PINS};
static const Pin gmacResetPin = BOARD_GMAC_RESET_PIN;

/** TX descriptors list */
COMPILER_SECTION(".ram_nocache")
COMPILER_ALIGNED(32) static sGmacTxDescriptor gTxDs[TX_BUFFERS], gDummyTxDs[DUMMY_SIZE];


/** TX callbacks list */
COMPILER_ALIGNED(32) static fGmacdTransferCallback gTxCbs[TX_BUFFERS], gDummyTxCbs[DUMMY_SIZE];

/** RX descriptors list */
COMPILER_SECTION(".ram_nocache")
COMPILER_ALIGNED(32) static sGmacRxDescriptor gRxDs[RX_BUFFERS], gDummyRxDs[DUMMY_SIZE];

/** Send Buffer */
/* Section 3.6 of AMBA 2.0 spec states that burst should not cross 1K Boundaries.
   Receive buffer manager writes are burst of 2 words => 3 lsb bits of the address
   shall be set to 0 */
COMPILER_ALIGNED(32) static uint8_t pTxBuffer[TX_BUFFERS * BUFFER_SIZE], pTxDummyBuffer[DUMMY_SIZE * DUMMY_BUFF_SIZE];

/** Receive Buffer */
COMPILER_ALIGNED(32) static uint8_t pRxBuffer[RX_BUFFERS * BUFFER_SIZE], pRxDummyBuffer[DUMMY_SIZE * DUMMY_BUFF_SIZE];

//
static const char dev_samv7x_eth_name[]="eth0\0";

static int dev_samv7x_eth_load(void);
static int dev_samv7x_eth_open(desc_t desc, int o_flag);
//
static int dev_samv7x_eth_isset_read(desc_t desc);
static int dev_samv7x_eth_isset_write(desc_t desc);
static int dev_samv7x_eth_close(desc_t desc);
static int dev_samv7x_eth_seek(desc_t desc,int offset,int origin);
static int dev_samv7x_eth_read(desc_t desc, char* buf,int cb);
static int dev_samv7x_eth_write(desc_t desc, const char* buf,int cb);
static int dev_samv7x_eth_ioctl(desc_t desc,int request,va_list ap);
static int dev_samv7x_eth_seek(desc_t desc,int offset,int origin);

//
dev_map_t dev_samv7x_eth_map={
   dev_samv7x_eth_name,
   S_IFCHR,
   dev_samv7x_eth_load,
   dev_samv7x_eth_open,
   dev_samv7x_eth_close,
   dev_samv7x_eth_isset_read,
   dev_samv7x_eth_isset_write,
   dev_samv7x_eth_read,
   dev_samv7x_eth_write,
   dev_samv7x_eth_seek,
   dev_samv7x_eth_ioctl
};

//
static desc_t desc_eth_r = -1;
static desc_t desc_eth_w = -1;
//
/*static*/ unsigned int eth_packet_recv_w;
/*static*/  unsigned int eth_packet_recv_r;
//
/*static*/  unsigned int eth_packet_send_w;
/*static*/  unsigned int eth_packet_send_r;
//
typedef struct eth_samv7x_info_st{
   int desc_r;
   int desc_w;
   //
   //
   uint8_t mac_addr[6];
   //
   uint8_t link_up;
}eth_samv7x_info_t;

//
static eth_samv7x_info_t eth_samv7x_info={
   .mac_addr={0xFC,0xC2,0x3D,0x08,0xF3,0x3e}
};
/*===========================================
Implementation
=============================================*/

/**
 * Gmac interrupt handler
 */
void GMAC_Handler(void)
{
   //
   __hw_enter_interrupt();
   //
	GMACD_Handler(&gGmacd, GMAC_QUE_0);
   //
   __hw_leave_interrupt();
}

/*-------------------------------------------
| Name:GmacdRxTransferCallback
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static void GmacdRxTransferCallback(uint32_t status){
   
   /*(GMAC_RSR) Buffer Not Available */
   if(status&(GMAC_RSR_BNA)){
      return;
   }
   /*(GMAC_RSR) Receive Overrun */
   if(status&(GMAC_RSR_RXOVR)){
       return;
   }
   /*(GMAC_RSR) HRESP Not OK */
   if(status&(GMAC_RSR_HNO)){
       return;
   }
   /*(GMAC_RSR) Frame Received */
   if(status&(GMAC_RSR_REC)){
      //
      if(eth_packet_recv_w==eth_packet_recv_r){
         __fire_io_int(ofile_lst[desc_eth_r].owner_pthread_ptr_read);
      }
      //
      eth_packet_recv_w++;
   }
  
}

/*-------------------------------------------
| Name:GmacdTxTransferCallback
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
static void GmacdTxTransferCallback(uint32_t status){
   
   /*(GMAC_TSR) Used Bit Read */
   if(status&(GMAC_TSR_UBR)){
      //return;
   }
   /*(GMAC_TSR) Collision Occurred */
   if(status&(GMAC_TSR_COL)){
      return;
   }
   /*(GMAC_TSR) Retry Limit Exceeded */
   if(status&(GMAC_TSR_RLE)){
      return;
   }
   /*(GMAC_TSR) Transmit Go */
   if(status&(GMAC_TSR_TXGO)){
      return;
   }
   /*(GMAC_TSR) Transmit Frame Corruption Due to AHB Error */
   if(status&(GMAC_TSR_TFC)){
      return;
   }
   /*(GMAC_TSR) HRESP Not OK */
   if(status&(GMAC_TSR_HRESP)){
      return;
   }
   /*(GMAC_TSR) Transmit Complete */
   if(status&(GMAC_TSR_TXCOMP)){
      //
      eth_packet_send_r++;
      //
      if(eth_packet_send_r==eth_packet_send_w){
         __fire_io_int(ofile_lst[desc_eth_w].owner_pthread_ptr_write);
      }
   }
   
}

/*-------------------------------------------
| Name:dev_samv7x_eth_load
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_samv7x_eth_load(void){
   return 0;
}

/*-------------------------------------------
| Name:dev_samv7x_eth_open
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_samv7x_eth_open(desc_t desc, int o_flag){
   //
   if(desc_eth_r<0 && desc_eth_w<0) {
      sGmacd *pGmacd = &gGmacd;
      GMacb  *pGmacb = &gGmacb;
      sGmacInit Que0, Que;
     //first startup
     // start ethernet periheral
       _mem_barrier_
      /* Init GMAC driver structure */
      memset(&Que0, 0, sizeof(Que0));
      Que0.bIsGem = 1;
      Que0.bDmaBurstLength = 4;
      Que0.pRxBuffer =pRxBuffer;
      Que0.pRxD = gRxDs;
      Que0.wRxBufferSize = BUFFER_SIZE;
      Que0.wRxSize = RX_BUFFERS;
      Que0.pTxBuffer = pTxBuffer;
      Que0.pTxD = gTxDs;
      Que0.wTxBufferSize = BUFFER_SIZE;
      Que0.wTxSize = TX_BUFFERS;
      Que0.pTxCb = gTxCbs;



      memset(&Que, 0, sizeof(Que));
      Que.bIsGem = 1;
      Que.bDmaBurstLength = 4;
      Que.pRxBuffer =pRxDummyBuffer;
      Que.pRxD = gDummyRxDs;
      Que.wRxBufferSize = DUMMY_BUFF_SIZE;
      Que.wRxSize = DUMMY_SIZE;
      Que.pTxBuffer = pTxDummyBuffer;
      Que.pTxD = gDummyTxDs;
      Que.wTxBufferSize = DUMMY_BUFF_SIZE;
      Que.wTxSize = DUMMY_SIZE;
      Que.pTxCb = gDummyTxCbs;
      //
      //SCB_CleanInvalidateDCache();
      //
      GMACD_Init(pGmacd, GMAC, ID_GMAC, GMAC_CAF_ENABLE, GMAC_NBC_DISABLE);
      GMACD_InitTransfer(pGmacd, &Que, GMAC_QUE_2);

      GMACD_InitTransfer(pGmacd, &Que, GMAC_QUE_1);

      GMACD_InitTransfer(pGmacd, &Que0, GMAC_QUE_0);

      GMAC_SetAddress(gGmacd.pHw, 0, eth_samv7x_info.mac_addr);

      /* Setup interrupts */  
      NVIC_SetPriority(GMAC_IRQn, (1u << __NVIC_PRIO_BITS) - 2u);
      NVIC_ClearPendingIRQ(GMAC_IRQn);
      NVIC_EnableIRQ(GMAC_IRQn);
      /* Init GMACB driver */
      GMACB_Init(pGmacb, pGmacd, BOARD_GMAC_PHY_ADDR);
      
      //hard reset phy 
      const Pin *pResetPins=&gmacResetPin;
	   uint32_t nbResetPins=1;
      //
      PIO_Configure(pResetPins, nbResetPins);
		PIO_Clear(pResetPins);
		__kernel_usleep(100000);//100ms
		PIO_Set(pResetPins);
      
      //soft reset phy 
      GMACB_ResetPhy(pGmacb);
      
      /* PHY initialize */
      if (!GMACB_InitPhy(pGmacb, BOARD_MCK,  0, 0,  gmacPins, PIO_LISTSIZE(gmacPins))){
       // printf("P: PHY Initialize ERROR!\n\r");
        return -1;
      }

      /* Auto Negotiate, work in RMII mode */
      if (!GMACB_AutoNegotiate(pGmacb)){
        //printf( "P: Auto Negotiate ERROR!\n\r" ) ;
        return -1;
      }
   }
   //
   if(o_flag & O_RDONLY) {
      if(desc_eth_r<0) {
         desc_eth_r = desc;
         //
         eth_packet_recv_r = 0;
         eth_packet_recv_w = 0;
         //
         if(!ofile_lst[desc].p)
            ofile_lst[desc].p=&eth_samv7x_info;
         //
         GMACD_SetRxCallback(&gGmacd,GmacdRxTransferCallback,GMAC_QUE_0);
      }
      else
         return -1;                //already open
   }

   if(o_flag & O_WRONLY) {
      if(desc_eth_w<0) {
         desc_eth_w = desc;
         //
         eth_packet_send_w = 0;
         eth_packet_send_r = 0;
         //
         if(!ofile_lst[desc].p)
            ofile_lst[desc].p=&eth_samv7x_info;
      }
      else
         return -1;                //already open
   }

   //unmask IRQ
   if(desc_eth_r>=0 && desc_eth_w>=0) {
     
   }
   return 0;
}

/*-------------------------------------------
| Name:dev_samv7x_eth_close
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_samv7x_eth_close(desc_t desc){
  // 
  if(ofile_lst[desc].oflag & O_RDONLY) {
      if(!ofile_lst[desc].nb_reader) {
         desc_eth_r = -1;
      }
   }
   //
   if(ofile_lst[desc].oflag & O_WRONLY) {
      if(!ofile_lst[desc].nb_writer) {
         desc_eth_w = -1;
      }
   }
   //
   if(desc_eth_r<0 && desc_eth_w<0) {
     //to do:stop ethernet periheral
   }

   return 0;
}

/*-------------------------------------------
| Name:dev_samv7x_eth_isset_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_samv7x_eth_isset_read(desc_t desc){
   if(eth_packet_recv_r!=eth_packet_recv_w){
      return 0;
   }
   //
   return -1;
}

/*-------------------------------------------
| Name:dev_samv7x_eth_isset_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_samv7x_eth_isset_write(desc_t desc){
   if(eth_packet_send_w==eth_packet_send_r){
      return 0;
   }
   //
   return -1;
}

/*-------------------------------------------
| Name:dev_samv7x_eth_read
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_samv7x_eth_read(desc_t desc, char* buf,int size){
   uint8_t rc;
   uint32_t frmlen;
   //
   //SCB_CleanInvalidateDCache();
   //
   rc = GMACD_Poll(&gGmacd, (uint8_t*)buf, (uint32_t)size, (uint32_t*)&frmlen, GMAC_QUE_0);
   if (rc != GMACD_OK){
      if(eth_packet_recv_r<eth_packet_recv_w)
         eth_packet_recv_r++;//drop packet
      return -1;
   }
   //
   if(eth_packet_recv_r<eth_packet_recv_w)
      eth_packet_recv_r++;
   //
   memory_sync();
   //
   return frmlen;
}

/*-------------------------------------------
| Name:dev_samv7x_eth_write
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_samv7x_eth_write(desc_t desc, const char* buf,int size){
   uint8_t rc;
   //
   eth_packet_send_w++;
   //
   //SCB_CleanInvalidateDCache();
   //
   rc = GMACD_Send(&gGmacd, (void*)buf, size, GmacdTxTransferCallback, GMAC_QUE_0);
   if (rc != GMACD_OK) {
     eth_packet_send_w--;
     return -1;
   }
   
   return 0;
}

/*-------------------------------------------
| Name:dev_samv7x_eth_seek
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_samv7x_eth_seek(desc_t desc,int offset,int origin){
   return -1;
}

/*-------------------------------------------
| Name:dev_samv7x_eth_ioctl
| Description:
| Parameters:
| Return Type:
| Comments:
| See:
---------------------------------------------*/
int dev_samv7x_eth_ioctl(desc_t desc,int request,va_list ap) {
   switch(request) {

   //reset interface
   case ETHRESET: {
   }
   break;

   //status interface
   case ETHSTAT: {
      eth_stat_t* p_eth_stat = va_arg( ap, eth_stat_t*);
      if(!p_eth_stat)
         return -1;
      //
      //*p_eth_stat = p_eth_info->eth_stat;
   }
   break;

   case ETHSETHWADDRESS: {
      unsigned char* p_eth_hwaddr = va_arg( ap, unsigned char*);
      if(!p_eth_hwaddr)
         return -1;
      //stop ehternet interface
     
      // Disable all interrupt
     
      //set mac address
      eth_samv7x_info.mac_addr[0] = p_eth_hwaddr[0];
      eth_samv7x_info.mac_addr[1] = p_eth_hwaddr[1];
      eth_samv7x_info.mac_addr[2] = p_eth_hwaddr[2];
      eth_samv7x_info.mac_addr[3] = p_eth_hwaddr[3];
      eth_samv7x_info.mac_addr[4] = p_eth_hwaddr[4];
      eth_samv7x_info.mac_addr[5] = p_eth_hwaddr[5];
      //	
      //gmacif_setmac(eth_samv7x_info.mac_addr);
         
      //reopen and restart ethernet interface
      //dmfe_open(p_eth_info);
   }
   break;

   case ETHGETHWADDRESS: {
      unsigned char* p_eth_hwaddr = va_arg( ap, unsigned char*);
      if(!p_eth_hwaddr)
         return -1;
      //
      p_eth_hwaddr[0] = eth_samv7x_info.mac_addr[0];
      p_eth_hwaddr[1] = eth_samv7x_info.mac_addr[1];
      p_eth_hwaddr[2] = eth_samv7x_info.mac_addr[2];
      p_eth_hwaddr[3] = eth_samv7x_info.mac_addr[3];
      p_eth_hwaddr[4] = eth_samv7x_info.mac_addr[4];
      p_eth_hwaddr[5] = eth_samv7x_info.mac_addr[5];
   }
   break;

   //
   default:
      return -1;
   }

   //
   return 0;
}


/*============================================
| End of Source  : dev_samv7x_eth.c
==============================================*/