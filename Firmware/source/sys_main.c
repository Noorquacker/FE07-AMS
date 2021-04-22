/** @file sys_main.c 
*   @brief Application main file
*   @date 11-Dec-2018
*   @version 04.07.01
*
*   This file contains an empty main function,
*   which can be used for the application.
*/

/* 
* Copyright (C) 2009-2018 Texas Instruments Incorporated - www.ti.com 
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
#include "can.h"
#include "het.h"
#include "gio.h"
#include "mibspi.h"
#include "spi.h"
#include "FE_AMS.h"
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
    gioInit();
    canInit();
    gioEnableNotification(gioPORTB,1);
    _enable_IRQ();
    adcInit();
    hetInit();
    mibspiInit();
    spiInit();
    gioSetBit(hetPORT1,14,1);
    gioSetBit(hetPORT1,12,1);
//    mibspiSetData(mibspiREG1, 0, TX_Data_Master);
//    mibspiSetData(mibspiREG1, 1, TX_Data_Master);
    spiDAT1_t dataconfig1_t;

        dataconfig1_t.CS_HOLD = TRUE;
        dataconfig1_t.WDEL    = TRUE;
        dataconfig1_t.DFSEL   = SPI_FMT_0;
        dataconfig1_t.CSNR    = 0xFE;

    spiDAT1_t dataconfig2_t;
            dataconfig2_t.CS_HOLD = TRUE;
            dataconfig2_t.WDEL    = TRUE;
            dataconfig2_t.DFSEL   = SPI_FMT_0;
            dataconfig2_t.CSNR    = 0xFE;


        //    mibspiTransfer(mibspiREG1, 0);
//    while(!(mibspiIsTransferComplete(mibspiREG1,0)));
//    mibspiTransfer(mibspiREG1, 1);
//    while(!(mibspiIsTransferComplete(mibspiREG1,1)));
      //  spiTransmitAndReceiveData(spiREG1, &dataconfig1_t, 1, TX_Data_Master, RX_Data_Master);


    while(1){
        spiReceiveData(spiREG1, &dataconfig1_t, 1, RX_Data_Master);
        spiReceiveData(spiREG3, &dataconfig2_t, 1, RX_Data_Slave);
        setCurrentBatteryVoltage(RX_Data_Slave[0]);
        setCurrentVehicleVoltage(RX_Data_Master[0]);
        //  spiReceiveData(spiREG1, &dataconfig2_t, 1, RX_Data_Slave);
//        spiTransmitAndReceiveData(spiREG1, &dataconfig1_t, 1, TX_Data_Master, RX_Data_Master);
//
//        spiTransmitAndReceiveData(spiREG1, &dataconfig2_t, 1, TX_Data_Slave, RX_Data_Slave);

//        mibspiTransfer(mibspiREG1, 0);
//            while(!(mibspiIsTransferComplete(mibspiREG1,0)));
//            mibspiTransfer(mibspiREG1, 1);
//            while(!(mibspiIsTransferComplete(mibspiREG1,1)));
//        mibspiGetData(mibspiREG1, 0, RX_Data_Master);
//        mibspiGetData(mibspiREG1, 1, RX_Data_Slave);

      //  gioSetBit(hetPORT1,14,1);

    }
/* USER CODE END */

    return 0;
}


/* USER CODE BEGIN (4) */
/* USER CODE END */
