/** @file sys_main.c 
*   @brief Application main file
*   @date 07-July-2017
*   @version 04.07.00
*
*   This file contains an empty main function,
*   which can be used for the application.
*/

/* 
* Copyright (C) 2009-2016 Texas Instruments Incorporated - www.ti.com 
* 
* 
*  Redistribution and use in source and binary forms, with or without 
*  modification, are permitted provided that the following conditions 
*  are met:
*
*    Redistributions of source code must retain the above copyright 
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the 
*    documentation and/or other materials provided with the   
*    distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/


/* USER CODE BEGIN (0) */
/* USER CODE END */

/* Include Files */

#include "sys_common.h"

/* USER CODE BEGIN (1) */
#include "adc.h"
#include "BMS.h"
#include "can.h"
#include "het.h"
#include "gio.h"
#include "mibspi.h"
#include "rti.h"
#include "sci.h"
#include "spi.h"
#include "sys_vim.h"
#include "pl455.h"
#include "AMS_common.h"
#include "FE_AMS.h"
//#include "swi_util.h"

/* USER CODE END */

/** @fn void main(void)
*   @brief Application main function
*   @note This function is empty by default.
*
*   This function is called after startup.
*   The user can use this function to implement the application.
*/

/* USER CODE BEGIN (2) */
uint16 TX_Data_Master[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint16 TX_Data_Slave[16]  = { 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20 };
uint16 RX_Data_Master[16] = { 0 };
uint16 RX_Data_Slave[16]  = { 0 };

/* USER CODE END */

uint8	emacAddress[6U] = 	{0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU};
uint32 	emacPhyAddress	=	0U;

int main(void)
{
/* USER CODE BEGIN (3) */
//    systemInit();

    gioInit();
    canInit();
    adcInit();
    hetInit();
    mibspiInit();
    sciInit();
    spiInit();

    sciSetBaudrate(sciREG, BMS_BAUDRATE);

    bool how = BMS_Init();
    uint8 BMS_DATA[55*BMS_TOTALBOARDS] = {0};
//    BMS_getBroadcastData(BMS_DATA, 55);
    BMS_getAllIndividualData(BMS_DATA,55);
    AMS_parseBMSData(BMS_DATA,16,8,1,1);
//    rtiInit();
//    vimInit();
//
    _enable_IRQ();
//
//    WakePL455();
//
//    CommClear();
//
//    CommReset();
//
//    _enable_IRQ();

    gioEnableNotification(gioPORTB,1);




    gioSetBit(hetPORT1,14,1);
    gioSetBit(hetPORT1,12,1);


    /*spiDAT1_t dataconfig1_t;
        dataconfig1_t.CS_HOLD = TRUE;
        dataconfig1_t.WDEL    = TRUE;
        dataconfig1_t.DFSEL   = SPI_FMT_0;
        dataconfig1_t.CSNR    = 0xFE;*/

    while(1){
        //gioToggleBit(hetPORT1,12);

//        CommClear();
//        delayms(5);
        gioSetBit(hetPORT1,12,0);
        gioSetBit(hetPORT1,12,1);
        AMS_readSPI();//1
        gioSetBit(hetPORT1,12,0);
        gioSetBit(hetPORT1,12,1);
        AMS_readADC();//2
        gioSetBit(hetPORT1,12,0);
        gioSetBit(hetPORT1,12,1);
        AMS_readGIO();//3
//        gioSetBit(hetPORT1,14,0);
//        gioSetBit(hetPORT1,14,1);
//        AMS_readHET();
        gioSetBit(hetPORT1,12,0);
        gioSetBit(hetPORT1,12,1);
      //  AMS_startHV();//4
        gioSetBit(hetPORT1,12,0);
        gioSetBit(hetPORT1,12,1);
        BMS_getAllIndividualData(BMS_DATA,55);//5
        gioSetBit(hetPORT1,12,0);
        gioSetBit(hetPORT1,12,1);
        AMS_parseBMSData(BMS_DATA,16,8,1,1);//6
        gioSetBit(hetPORT1,12,0);
        gioSetBit(hetPORT1,12,1);
        AMS_canTX_Car();//7
        gioSetBit(hetPORT1,12,0);
        gioSetBit(hetPORT1,12,1);
      //  AMS_canTx_BMSData();//8
        gioSetBit(hetPORT1,12,0);
        gioSetBit(hetPORT1,HET1_NEGATIVE_CONTACT_CTRL,0);
        delayms(1000);
    }
/* USER CODE END */

    return 0;
}


/* USER CODE BEGIN (4) */



/* USER CODE END */
