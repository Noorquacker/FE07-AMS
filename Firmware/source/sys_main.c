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
    systemInit();

    gioInit();
    canInit();
    adcInit();
    hetInit();
    mibspiInit();
    sciInit();
    spiInit();

    sciSetBaudrate(sciREG, BMS_BAUDRATE);
//    rtiInit();
//    vimInit();
//
//    _enable_IRQ();
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
        dataconfig1_t.CSNR    = 0xFE;

    spiDAT1_t dataconfig2_t;
            dataconfig2_t.CS_HOLD = TRUE;
            dataconfig2_t.WDEL    = TRUE;
            dataconfig2_t.DFSEL   = SPI_FMT_0;
            dataconfig2_t.CSNR    = 0xFE;*/

    uint8 buffer[164] = {0};
//    int result = 0;
//    result = BMS_Init();
//    AMS_readSCI();
//    getBMSData(buffer);
    uint8 send[5] = {0xE1, 0x02, 0x01, 0x90, 0x96};
/*
    int i = 0;
    for(i=0; i<20; i++){
        sciSendByte(sciREG, 0xF9);
        sciSendByte(sciREG, 0x00);
        sciSendByte(sciREG, 0x0C);
        sciSendByte(sciREG, 0x40);
        sciSendByte(sciREG, 0x34);
        sciSendByte(sciREG, 0x6C);
        delayms(12);
    }

    sciSendByte(sciREG, 0xFA);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0x10);
    sciSendByte(sciREG, 0x10);
    sciSendByte(sciREG, 0xE0);
    sciSendByte(sciREG, 0xD5);
    sciSendByte(sciREG, 0x99);
    delayus(911);

    sciSendByte(sciREG, 0xF9);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0x0E);
    sciSendByte(sciREG, 0x10);
    sciSendByte(sciREG, 0x35);
    sciSendByte(sciREG, 0x30);
    delayus(1103);

    sciSendByte(sciREG, 0xF9);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0x0C);
    sciSendByte(sciREG, 0x08);
    sciSendByte(sciREG, 0x34);
    sciSendByte(sciREG, 0x5A);
    delayus(4571);

/*    sciSendByte(sciREG, 0xF9);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0x0A);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0x36);
    sciSendByte(sciREG, 0x3C);
    sciSendByte(sciREG, 0xF9);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0x0A);
    sciSendByte(sciREG, 0x01);
    sciSendByte(sciREG, 0xF7);
    sciSendByte(sciREG, 0xFC);
    sciSendByte(sciREG, 0xF9);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0x0A);
    sciSendByte(sciREG, 0x02);
    sciSendByte(sciREG, 0xB7);
    sciSendByte(sciREG, 0xFD);
    sciSendByte(sciREG, 0xF9);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0x0A);
    sciSendByte(sciREG, 0x03);
    sciSendByte(sciREG, 0x76);
    sciSendByte(sciREG, 0x3D);
    sciSendByte(sciREG, 0xF9);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0x0A);
    sciSendByte(sciREG, 0x04);
    sciSendByte(sciREG, 0x37);
    sciSendByte(sciREG, 0xFF);
    sciSendByte(sciREG, 0xF9);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0x0A);
    sciSendByte(sciREG, 0xF9);
    sciSendByte(sciREG, 0x05);
    sciSendByte(sciREG, 0xF6);
    sciSendByte(sciREG, 0x3F);
    sciSendByte(sciREG, 0xF9);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0x0A);
    sciSendByte(sciREG, 0x06);
    sciSendByte(sciREG, 0xB6);
    sciSendByte(sciREG, 0x3E); */


/*
    sciSendByte(sciREG, 0x9A);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0x10);
    sciSendByte(sciREG, 0x10);
    sciSendByte(sciREG, 0xC0);
    sciSendByte(sciREG, 0x11);
    sciSendByte(sciREG, 0xAF);
    delayus(3296);

    sciSendByte(sciREG, 0x89);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0x0A);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0xDB);
    sciSendByte(sciREG, 0x7F);

    uint32 first_four[4];
    for(i = 0; i<4; i++){
        first_four[i] = sciReceiveByte(sciREG);
    }
    delayus(1248);

    sciSendByte(sciREG, 0x89);
    sciSendByte(sciREG, 0x01);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0x0A);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0xDA);
    sciSendByte(sciREG, 0x83);

    uint32 second_four[4];
    for(i = 0; i<4; i++){
        second_four[i] = sciReceiveByte(sciREG);
    }
    delayus(1574);

    sciSendByte(sciREG, 0x9A);
    sciSendByte(sciREG, 0x01);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0x10);
    sciSendByte(sciREG, 0x10);
    sciSendByte(sciREG, 0x20);
    sciSendByte(sciREG, 0x2D);
    sciSendByte(sciREG, 0xE7);
    delayus(2212);

    sciSendByte(sciREG, 0x89);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0x01);
    sciSendByte(sciREG, 0x1C);
    sciSendByte(sciREG, 0x1F);

    uint32 third_five[5];
    for(i = 0; i<5; i++){
        third_five[i] = sciReceiveByte(sciREG);
    }
    delayus(8613);

    sciSendByte(sciREG, 0x89);
    sciSendByte(sciREG, 0x01);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0x01);
    sciSendByte(sciREG, 0x1D);
    sciSendByte(sciREG, 0xE3);

    uint32 fourth_five[5];
    for(i = 0; i<5; i++){
        fourth_five[i] = sciReceiveByte(sciREG);
    }
    delayms(16);

    sciSendByte(sciREG, 0xFA);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0x52);
    sciSendByte(sciREG, 0xFF);
    sciSendByte(sciREG, 0xFF);
    sciSendByte(sciREG, 0x78);
    sciSendByte(sciREG, 0x75);
    delayms(100);

    sciSendByte(sciREG, 0xF9);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0x51);
    sciSendByte(sciREG, 0x38);
    sciSendByte(sciREG, 0x0C);
    sciSendByte(sciREG, 0xDE);
    delayms(2250);

    // READY TO TALK :)

    sciSendByte(sciREG, 0x89);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0xF4);
    sciSendByte(sciREG, 0x03);
    sciSendByte(sciREG, 0xDB);
    sciSendByte(sciREG, 0x1E);

    uint32 a_seven[7];
    for(i = 0; i<7; i++){
        a_seven[i] = sciReceiveByte(sciREG);
    }
    delayms(12);


    sciSendByte(sciREG, 0x9C);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0xF0);
    sciSendByte(sciREG, 0xED);
    sciSendByte(sciREG, 0xF0);
    sciSendByte(sciREG, 0x22);
    sciSendByte(sciREG, 0xF7);
    sciSendByte(sciREG, 0x25);
    sciSendByte(sciREG, 0x55);
    delayms(5);

    sciSendByte(sciREG, 0x89);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0x52);
    sciSendByte(sciREG, 0x01);
    sciSendByte(sciREG, 0x21);
    sciSendByte(sciREG, 0x7F);
    uint32 b_five[5];
    for(i = 0; i<5; i++){
        b_five[i] = sciReceiveByte(sciREG);
    }
    delayms(28);

    sciSendByte(sciREG, 0x89);
    sciSendByte(sciREG, 0x01);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0xF4);
    sciSendByte(sciREG, 0x03);
    sciSendByte(sciREG, 0xDA);
    sciSendByte(sciREG, 0xE2);
    uint32 c_seven[7];
    for(i = 0; i<7; i++){
        c_seven[i] = sciReceiveByte(sciREG);
    }
    delayms(18);

    sciSendByte(sciREG, 0x9C);
    sciSendByte(sciREG, 0x01);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0xF0);
    sciSendByte(sciREG, 0xED);
    sciSendByte(sciREG, 0x50);
    sciSendByte(sciREG, 0x23);
    sciSendByte(sciREG, 0xF7);
    sciSendByte(sciREG, 0x34);
    sciSendByte(sciREG, 0x27);
    delayms(3);

    sciSendByte(sciREG, 0x89);
    sciSendByte(sciREG, 0x01);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0x52);
    sciSendByte(sciREG, 0x01);
    sciSendByte(sciREG, 0x20);
    sciSendByte(sciREG, 0x83);
    uint32 d_five[5];
    for(i = 0; i<5; i++){
        d_five[i] = sciReceiveByte(sciREG);
    }
    delayms(28);

    sciSendByte(sciREG, 0xF9);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0x02);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0x31);
    sciSendByte(sciREG, 0xFC);
    delayms(108);

    sciSendByte(sciREG, 0x9A);
    sciSendByte(sciREG, 0x01);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0x52);
    sciSendByte(sciREG, 0x01);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0x80);
    sciSendByte(sciREG, 0x7B);
    delayms(2);

    sciSendByte(sciREG, 0x9C);
    sciSendByte(sciREG, 0x01);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0x60);
    sciSendByte(sciREG, 0xFF);
    sciSendByte(sciREG, 0xFF);
    sciSendByte(sciREG, 0xFF);
    sciSendByte(sciREG, 0xFF);
    sciSendByte(sciREG, 0x98);
    sciSendByte(sciREG, 0x65);
    delayms(4);

    sciSendByte(sciREG, 0x99);
    sciSendByte(sciREG, 0x01);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0x60);
    sciSendByte(sciREG, 0x40);
    sciSendByte(sciREG, 0x34);
    sciSendByte(sciREG, 0x10);
    delayms(2);

    sciSendByte(sciREG, 0x89);
    sciSendByte(sciREG, 0x01);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0x60);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0xF4);
    sciSendByte(sciREG, 0x23);
    uint32 e_four[4];
    for(i = 0; i<4; i++){
        e_four[i] = sciReceiveByte(sciREG);
    }
    delayms(12);

    sciSendByte(sciREG, 0x89);
    sciSendByte(sciREG, 0x01);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0xF4);
    sciSendByte(sciREG, 0x03);
    sciSendByte(sciREG, 0xDA);
    sciSendByte(sciREG, 0xE2);
    uint32 f_seven[7];
    for(i = 0; i<7; i++){
        f_seven[i] = sciReceiveByte(sciREG);
    }
    delayms(15);

    sciSendByte(sciREG,0x9c);
    sciSendByte(sciREG,0x01);
    sciSendByte(sciREG,0x00);
    sciSendByte(sciREG,0xF0);
    sciSendByte(sciREG,0xED);
    sciSendByte(sciREG,0x50);
    sciSendByte(sciREG,0x23);
    sciSendByte(sciREG,0xF7);
    sciSendByte(sciREG,0x34);
    sciSendByte(sciREG,0x27);
    delayms(3);

    sciSendByte(sciREG,0x89);
    sciSendByte(sciREG,0x01);
    sciSendByte(sciREG,0x00);
    sciSendByte(sciREG,0x52);
    sciSendByte(sciREG,0x01);
    sciSendByte(sciREG,0x20);
    sciSendByte(sciREG,0x83);
    uint32 g_five[5];
    for(i = 0; i<5; i++){
        g_five[i] = sciReceiveByte(sciREG);
    }
    delayms(25);

    sciSendByte(sciREG,0x9A);
    sciSendByte(sciREG,0x01);
    sciSendByte(sciREG,0x00);
    sciSendByte(sciREG,0x52);
    sciSendByte(sciREG,0x04);
    sciSendByte(sciREG,0x00);
    sciSendByte(sciREG,0x83);
    sciSendByte(sciREG,0x2B);

*/
    //sciSend(sciREG, 5, send);
    sciSendByte(sciREG, 0xE1);
    sciSendByte(sciREG, 0x02);
    sciSendByte(sciREG, 0x00);
    sciSendByte(sciREG, 0x90);
    sciSendByte(sciREG, 0x96);
//    sciSendByte(sciREG, 0xE1);
//    sciSendByte(sciREG, 0x02);
//    sciSendByte(sciREG, 0x01);
//    sciSendByte(sciREG, 0x90);
//    sciSendByte(sciREG, 0x96);
//    sciReceive(sciREG,39*BMS_TOTALBOARDS,&buffer);
    uint32 bufferr[39*BMS_TOTALBOARDS];
    int i = 0;
    for(i = 0; i<39*BMS_TOTALBOARDS; i++){
        bufferr[i] = sciReceiveByte(sciREG);
    }

    while(1){
//        getBMSData(buffer);
//        gioToggleBit(hetPORT1,12);
//        delayms(500);

    	/*spiReceiveData(spiREG1, &dataconfig1_t, 1, RX_Data_Master);
        spiReceiveData(spiREG3, &dataconfig2_t, 1, RX_Data_Slave);
        setCurrentBatteryVoltage(RX_Data_Slave[0]);
        setCurrentVehicleVoltage(RX_Data_Master[0]);*/

    }
/* USER CODE END */

    return 0;
}


/* USER CODE BEGIN (4) */



/* USER CODE END */
