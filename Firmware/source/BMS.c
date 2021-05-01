/*
 * BMS.c
 *
 *  Created on: Apr 23, 2021
 *      Author: Mikel
 */

#include <stdbool.h>
#include "AMS_common.h"
#include "sci.h"
#include "pl455.h"

int UART_RX_RDY = 0;
int RTI_TIMEOUT = 0;

int BMS_Init(){
	// From bq76PL455A Example Code
	sciSetBaudrate(sciREG, BMS_BAUDRATE);
	WakePL455();
	CommClear();
	CommReset();

	int nSent, nRead, nTopFound = 0;
	int nDev_ID, nGrp_ID;
	uint8  bFrame[256];
	uint32  wTemp = 0;

	// Wake all devices
	// The wake tone will awaken any device that is already in shutdown and the pwrdown will shutdown any device
	// that is already awake. The least number of times to sequence wake and pwrdown will be half the number of
	// boards to cover the worst case combination of boards already awake or shutdown.
//	for(nDev_ID = 0; nDev_ID < BMS_TOTALBOARDS>>1; nDev_ID++) {
//		nSent = WriteReg(nDev_ID, 12, 0x40, 1, FRMWRT_ALL_NR);	// send out broadcast pwrdown command
//		delayms(5); //~5ms
//		WakePL455();
//		delayms(5); //~5ms
//	}

	// Mask Customer Checksum Fault bit
//	nSent = WriteReg(0, 107, 0x8000, 2, FRMWRT_ALL_NR); // clear all fault summary flags

	// Clear all faults
//	nSent = WriteReg(0, 82, 0xFFC0, 2, FRMWRT_ALL_NR);		// clear all fault summary flags
//	nSent = WriteReg(0, 81, 0x38, 1, FRMWRT_ALL_NR); // clear fault flags in the system status register

	// Auto-address all boards (section 1.2.2)
//	nSent = WriteReg(0, 14, 0x19, 1, FRMWRT_ALL_NR); // set auto-address mode on all boards
//	nSent = WriteReg(0, 12, 0x08, 1, FRMWRT_ALL_NR); // enter auto address mode on all boards, the next write to this ID will be its address

	// Set addresses for all boards in daisy-chain (section 1.2.3)
//	for (nDev_ID = 0; nDev_ID < BMS_TOTALBOARDS; nDev_ID++)
//	{
//		nSent = WriteReg(nDev_ID, 10, nDev_ID, 1, FRMWRT_ALL_NR); // send address to each board
//	}

	// Enable all communication interfaces on all boards in the stack (section 1.2.1)
//	nSent = WriteReg(0, 16, 0x10F8, 2, FRMWRT_ALL_NR);	// set communications baud rate and enable all interfaces on all boards in stack
   // nRead = ReadReg(0, 10, &wTemp, 1, 0); // 0ms timeout
    nSent = WriteReg(0, 16, 0x10E0, 2, FRMWRT_ALL_NR);  // set communications baud rate and enable all interfaces on all boards in stack


/*	for (nDev_ID = BMS_TOTALBOARDS - 1; nDev_ID >= 0; --nDev_ID)
	{
		// read device ID to see if there is a response
		nRead = ReadReg(nDev_ID, 10, &wTemp, 1, 0); // 0ms timeout

		if(nRead == 0) // if nothing is read then this board doesn't exist
			nTopFound = 0;
		else // a response was received
		{
			if(nTopFound == 0)
			{ // if the last board was not present but this one is, this is the top board
				if(nDev_ID == 0) // this is the only board
				{
					nSent = WriteReg(nDev_ID, 16, 0x1080, 2, FRMWRT_SGL_NR);	// enable only single-end comm port on board
				}
				else // this is the top board of a stack (section 1.2.5)
				{
					nSent = WriteReg(nDev_ID, 16, 0x1028, 2, FRMWRT_SGL_NR);	// enable only comm-low and fault-low for the top board
					nTopFound = 1;
				}
			}
			else // this is a middle or bottom board
			{
				if(nDev_ID == 0) // this is a bottom board of a stack (section 1.2.6)
				{
					nSent = WriteReg(nDev_ID, 16, 0x10D0, 2, FRMWRT_SGL_NR);	// enable comm-high, fault-high and single-end comm port on bottom board
				}
				else // this is a middle board
				{
					nSent = WriteReg(nDev_ID, 16, 0x1078, 2, FRMWRT_SGL_NR);	// enable comm-high, fault-high, comm-low and fault-low on all middle boards
				}
			}
		}
	}
*/
	// Clear all faults (section 1.2.7)
//	nSent = WriteReg(0, 82, 0xFFC0, 2, FRMWRT_ALL_NR); // clear all fault summary flags
//	nSent = WriteReg(0, 81, 0x38, 1, FRMWRT_ALL_NR); // clear fault flags in the system status register
//
//	delayms(10);

	// Configure AFE (section 2.2.1)
//
//	nDev_ID = 0;
//	nSent = WriteReg(nDev_ID, 60, 0x00, 1, FRMWRT_SGL_NR); // set 0 mux delay
//	nSent = WriteReg(nDev_ID, 61, 0x00, 1, FRMWRT_SGL_NR); // set 0 initial delay

	// Configure voltage and internal sample period (section 2.2.2)
//	nDev_ID = 0;
//	nSent = WriteReg(nDev_ID, 62, 0xCC, 1, FRMWRT_SGL_NR); // set 99.92us ADC sampling period

	// Configure the oversampling rate (section 2.2.3)
//	nDev_ID = 0;
//	nSent = WriteReg(nDev_ID, 7, 0x00, 1, FRMWRT_SGL_NR); // set no oversampling period


	// Select identical number of cells and channels on all modules simultaneously (section 2.2.5.2)
	nSent = WriteReg(0, 13, 0x10, 1, FRMWRT_ALL_NR); // set number of cells to 16
	nSent = WriteReg(0, 3, 0xFFFF00C0, 4, FRMWRT_ALL_NR); // select all cell, NO AUX channels, and internal digital die and internal analog die temperatures

	// Set cell over-voltage and cell under-voltage thresholds on all boards simultaneously (section 2.2.6.2)
	nSent = WriteReg(0, 144, 0xD70A, 2, FRMWRT_ALL_NR); // set OV threshold = 4.2000V (Calculation: ceil(13107*Volt) = ceil(13107*4.2) = 55050 = 0xD70A)
	nSent = WriteReg(0, 142, 0x9999, 2, FRMWRT_ALL_NR); // set UV threshold = 3.0000V (Calculation: ceil(13107*Volt) = ceil(13107*3.0) = 39231 = 0x9999)

	// Send broadcast request to all boards to sample and send results (section 3.2)
//	nSent = WriteReg(0, 2, 0x02, 1, FRMWRT_ALL_NR); // send sync sample command
//	nSent = WaitRespFrame(bFrame, /*195*/39*BMS_TOTALBOARDS, 2); // 39? bytes data (x5) + packet header (x5) + CRC (2bytes) (x5), 0ms timeout
										   // For calculations, see Page 27 of https://www.ti.com/lit/ds/slusc51c/slusc51c.pdf?ts=1619217953406


	return nSent;


}

// https://www.ti.com/lit/an/slva617a/slva617a.pdf?ts=1619120159432&ref_url=https%253A%252F%252Fwww.ti.com%252Fproduct%252FBQ76PL455A
// All functions are based on this documentation

uint32 BMS_receiveByte()
{
    sciBASE_t *sci = sciREG;
    uint32 timeout = 0;
    /*SAFETYMCUSW 28 D MR:NA <APPROVED> "Potentially infinite loop found - Hardware Status check for execution sequence" */
    while (((sci->FLR & (uint32)SCI_RX_INT) == 0U) && (timeout >= 500000))
    {
        timeout++;
    } /* Wait */

    if(timeout >= 500000){
        return 0xFFFFFFFFFFFFFFFF; // TIMEOUT
    }
    return (sci->RD & (uint32)0x000000FFU);
}

void BMS_getCRCBytes(uint8 * CRC, uint8 * message, uint8 length){
    uint16 crc_r = 0;
    crc_r = CRC16(message, length);

    CRC[0] = (crc_r>>8) & 0xFF;
    CRC[1] = (crc_r>>0) & 0xFF;

    return;
}

void BMS_sendMessage(uint8 * message, uint8 length){
    uint8 crc[2] = {0,0};
    BMS_getCRCBytes(crc, message,length);
    uint8 i = 0;
    for(i=0;i<length;i++){
        sciSendByte(sciREG, message[i]);
    }

    sciSendByte(sciREG, crc[1]);
    sciSendByte(sciREG, crc[0]);

    return;
}


// 1.2.1
void BMS_initialConfig(){
    uint8 message[4] = {0xF2,0x10,0x10,0xE0};
    BMS_sendMessage(message, 4);

//    sciSendByte(sciREG, 0xF2);      // GENERAL BROADCAST WRITE NO RESPONSE (2 BYTES)
//    sciSendByte(sciREG, 0x10);      // REGISTER ADDRESS
//    sciSendByte(sciREG, 0x10);      // DATA
//    sciSendByte(sciREG, 0xE0);      // DATA
//    sciSendByte(sciREG, 0x3F);      // CRC
//    sciSendByte(sciREG, 0x35);      // CRC

    return;
}

//1.2.2
void BMS_initAutoAddress(){
    // Configure the bq76PL455A-Q1 device to use auto-addressing to select address
    uint8 messageOne[3] = {0xF1,0x0E,0x10};
    BMS_sendMessage(messageOne, 3);

//    sciSendByte(sciREG, 0xF1);      // GENERAL BROADCAST WRITE NO RESPONSE (1 BYTE)
//    sciSendByte(sciREG, 0x0E);      // REGISTER ADDRESS
//    sciSendByte(sciREG, 0x10);      // DATA
//    sciSendByte(sciREG, 0x54);      // CRC
//    sciSendByte(sciREG, 0x5F);      // CRC

    // Configure the bq76PL455A-Q1 device to enter auto-address mode
    uint8 messageTwo[3] = {0xF1,0x0C,0x08};
    BMS_sendMessage(messageTwo, 3);

//    sciSendByte(sciREG, 0xF1);      // GENERAL BROADCAST WRITE NO RESPONSE (1 BYTE)
//    sciSendByte(sciREG, 0x0C);      // REGISTER ADDRESS
//    sciSendByte(sciREG, 0x08);      // DATA
//    sciSendByte(sciREG, 0x55);      // CRC
//    sciSendByte(sciREG, 0x35);      // CRC

    return;
}

// 1.2.3
bool BMS_setAddresses(){
    // Write a new address sequentially to the Device Address register
    switch(BMS_TOTALBOARDS){
        case 16:
            //Configure device 16 to address 15
            sciSendByte(sciREG, 0xF1);      // GENERAL BROADCAST WRITE NO RESPONSE (1 BYTE)
            sciSendByte(sciREG, 0x0A);      // REGISTER ADDRESS
            sciSendByte(sciREG, 0x0F);      // DATA
            sciSendByte(sciREG, 0x17);      // CRC
            sciSendByte(sciREG, 0x57);      // CRC
            //no break
        case 15:
            //Configure device 15 to address 14
            sciSendByte(sciREG, 0xF1);      // GENERAL BROADCAST WRITE NO RESPONSE (1 BYTE)
            sciSendByte(sciREG, 0x0A);      // REGISTER ADDRESS
            sciSendByte(sciREG, 0x0E);      // DATA
            sciSendByte(sciREG, 0xD6);      // CRC
            sciSendByte(sciREG, 0x97);      // CRC
            //no break
        case 14:
            //Configure device 14 to address 13
            sciSendByte(sciREG, 0xF1);      // GENERAL BROADCAST WRITE NO RESPONSE (1 BYTE)
            sciSendByte(sciREG, 0x0A);      // REGISTER ADDRESS
            sciSendByte(sciREG, 0x0D);      // DATA
            sciSendByte(sciREG, 0x96);      // CRC
            sciSendByte(sciREG, 0x96);      // CRC
            //no break
        case 13:
            //Configure device 13 to address 12
            sciSendByte(sciREG, 0xF1);      // GENERAL BROADCAST WRITE NO RESPONSE (1 BYTE)
            sciSendByte(sciREG, 0x0A);      // REGISTER ADDRESS
            sciSendByte(sciREG, 0x0C);      // DATA
            sciSendByte(sciREG, 0x57);      // CRC
            sciSendByte(sciREG, 0x56);      // CRC
            //no break
        case 12:
            //Configure device 12 to address 11
            sciSendByte(sciREG, 0xF1);      // GENERAL BROADCAST WRITE NO RESPONSE (1 BYTE)
            sciSendByte(sciREG, 0x0A);      // REGISTER ADDRESS
            sciSendByte(sciREG, 0x0B);      // DATA
            sciSendByte(sciREG, 0x16);      // CRC
            sciSendByte(sciREG, 0x94);      // CRC
            //no break
        case 11:
            //Configure device 11 to address 10
            sciSendByte(sciREG, 0xF1);      // GENERAL BROADCAST WRITE NO RESPONSE (1 BYTE)
            sciSendByte(sciREG, 0x0A);      // REGISTER ADDRESS
            sciSendByte(sciREG, 0x0A);      // DATA
            sciSendByte(sciREG, 0xD7);      // CRC
            sciSendByte(sciREG, 0x54);      // CRC
            //no break
        case 10:
            //Configure device 10 to address 9
            sciSendByte(sciREG, 0xF1);      // GENERAL BROADCAST WRITE NO RESPONSE (1 BYTE)
            sciSendByte(sciREG, 0x0A);      // REGISTER ADDRESS
            sciSendByte(sciREG, 0x09);      // DATA
            sciSendByte(sciREG, 0x97);      // CRC
            sciSendByte(sciREG, 0x55);      // CRC
            //no break
        case 9:
            //Configure device 9 to address 8
            sciSendByte(sciREG, 0xF1);      // GENERAL BROADCAST WRITE NO RESPONSE (1 BYTE)
            sciSendByte(sciREG, 0x0A);      // REGISTER ADDRESS
            sciSendByte(sciREG, 0x08);      // DATA
            sciSendByte(sciREG, 0x56);      // CRC
            sciSendByte(sciREG, 0x95);      // CRC
            //no break
        case 8:
            //Configure device 8 to address 7
            sciSendByte(sciREG, 0xF1);      // GENERAL BROADCAST WRITE NO RESPONSE (1 BYTE)
            sciSendByte(sciREG, 0x0A);      // REGISTER ADDRESS
            sciSendByte(sciREG, 0x07);      // DATA
            sciSendByte(sciREG, 0x16);      // CRC
            sciSendByte(sciREG, 0x91);      // CRC
            //no break
        case 7:
            //Configure device 7 to address 6
            sciSendByte(sciREG, 0xF1);      // GENERAL BROADCAST WRITE NO RESPONSE (1 BYTE)
            sciSendByte(sciREG, 0x0A);      // REGISTER ADDRESS
            sciSendByte(sciREG, 0x06);      // DATA
            sciSendByte(sciREG, 0xD7);      // CRC
            sciSendByte(sciREG, 0x51);      // CRC
            //no break
        case 6:
            //Configure device 6 to address 5
            sciSendByte(sciREG, 0xF1);      // GENERAL BROADCAST WRITE NO RESPONSE (1 BYTE)
            sciSendByte(sciREG, 0x0A);      // REGISTER ADDRESS
            sciSendByte(sciREG, 0x05);      // DATA
            sciSendByte(sciREG, 0x97);      // CRC
            sciSendByte(sciREG, 0x50);      // CRC
            //no break
        case 5:
            //Configure device 5 to address 4
            sciSendByte(sciREG, 0xF1);      // GENERAL BROADCAST WRITE NO RESPONSE (1 BYTE)
            sciSendByte(sciREG, 0x0A);      // REGISTER ADDRESS
            sciSendByte(sciREG, 0x04);      // DATA
            sciSendByte(sciREG, 0x56);      // CRC
            sciSendByte(sciREG, 0x90);      // CRC
            //no break
        case 4:
            //Configure device 4 to address 3
            sciSendByte(sciREG, 0xF1);      // GENERAL BROADCAST WRITE NO RESPONSE (1 BYTE)
            sciSendByte(sciREG, 0x0A);      // REGISTER ADDRESS
            sciSendByte(sciREG, 0x03);      // DATA
            sciSendByte(sciREG, 0x17);      // CRC
            sciSendByte(sciREG, 0x52);      // CRC
            //no break
        case 3:
            //Configure device 3 to address 2
            sciSendByte(sciREG, 0xF1);      // GENERAL BROADCAST WRITE NO RESPONSE (1 BYTE)
            sciSendByte(sciREG, 0x0A);      // REGISTER ADDRESS
            sciSendByte(sciREG, 0x02);      // DATA
            sciSendByte(sciREG, 0xD6);      // CRC
            sciSendByte(sciREG, 0x92);      // CRC
            //no break
        case 2:
            //Configure device 2 to address 1
            sciSendByte(sciREG, 0xF1);      // GENERAL BROADCAST WRITE NO RESPONSE (1 BYTE)
            sciSendByte(sciREG, 0x0A);      // REGISTER ADDRESS
            sciSendByte(sciREG, 0x01);      // DATA
            sciSendByte(sciREG, 0x96);      // CRC
            sciSendByte(sciREG, 0x93);      // CRC
            //no break
        case 1:
            //Configure device 1 to address 0
            sciSendByte(sciREG, 0xF1);      // GENERAL BROADCAST WRITE NO RESPONSE (1 BYTE)
            sciSendByte(sciREG, 0x0A);      // REGISTER ADDRESS
            sciSendByte(sciREG, 0x00);      // DATA
            sciSendByte(sciREG, 0x57);      // CRC
            sciSendByte(sciREG, 0x53);      // CRC
            return 1;
            break;
        default:
            return 0;
            break;
    }

    return 0;
}

// 1.2.4
bool BMS_checkHeartbeats(){
    uint8 i = 0;
    uint8 message[4] = {0x81,0x00,0x0A,0x00}
    for(i=0; i<BMS_TOTALBOARDS;i++){
        message[1] = i;
        BMS_sendMessage();
    }

//    uint8 expectedLength = 4;
//    uint8 receivedMessage[4] = {0x00,0x00,0x00,0x00};
//    uint8 expectedMessage[4] = {0x00,0x00,0x00,0x00};
//    uint8 i = 0;
//    bool allGood = 1;
//
//    // Write a new address sequentially to the Device Address register
//    switch(BMS_TOTALBOARDS){
//        case 16:
//            // Read Device Address register on device 16 (at address 15)
//            sciSendByte(sciREG, 0x81);      // SINGLE DEVICE WRITE W/RESPONSE
//            sciSendByte(sciREG, 0x0F);      // DEVICE ADDRESS
//            sciSendByte(sciREG, 0x0A);      // REGISTER ADDRESS
//            sciSendByte(sciREG, 0x00);      // EXPECTED RESPONSE DATA BYTES (1)
//            sciSendByte(sciREG, 0x1E);      // CRC
//            sciSendByte(sciREG, 0x9F);      // CRC
//
//            expectedMessage[0] = 0x00;      // RESPONSE
//            expectedMessage[1] = 0x0F;      // DATA
//            expectedMessage[2] = 0x40;      // CRC
//            expectedMessage[3] = 0x04;      // CRC
//
//            allGood &= BMS_messageIsExpected(expectedLength, expectedMessage, receivedMessage);
//            //no break
//        case 15:
//            // Read Device Address register on device 15 (at address 14)
//            sciSendByte(sciREG, 0x81);      // SINGLE DEVICE WRITE W/RESPONSE
//            sciSendByte(sciREG, 0x0E);      // DEVICE ADDRESS
//            sciSendByte(sciREG, 0x0A);      // REGISTER ADDRESS
//            sciSendByte(sciREG, 0x00);      // EXPECTED RESPONSE DATA BYTES (1)
//            sciSendByte(sciREG, 0x4F);      // CRC
//            sciSendByte(sciREG, 0x5F);      // CRC
//
//            expectedMessage[0] = 0x00;      // RESPONSE
//            expectedMessage[1] = 0x0E;      // DATA
//            expectedMessage[2] = 0x81;      // CRC
//            expectedMessage[3] = 0xC4;      // CRC
//
//            allGood &= BMS_messageIsExpected(expectedLength, expectedMessage, receivedMessage);
//            //no break
//        case 14:
//            // Read Device Address register on device 14 (at address 13)
//            sciSendByte(sciREG, 0x81);      // SINGLE DEVICE WRITE W/RESPONSE
//            sciSendByte(sciREG, 0x0D);      // DEVICE ADDRESS
//            sciSendByte(sciREG, 0x0A);      // REGISTER ADDRESS
//            sciSendByte(sciREG, 0x00);      // EXPECTED RESPONSE DATA BYTES (1)
//            sciSendByte(sciREG, 0xBF);      // CRC
//            sciSendByte(sciREG, 0x5F);      // CRC
//
//            expectedMessage[0] = 0x00;      // RESPONSE
//            expectedMessage[1] = 0x0D;      // DATA
//            expectedMessage[2] = 0xC1;      // CRC
//            expectedMessage[3] = 0xC5;      // CRC
//
//            allGood &= BMS_messageIsExpected(expectedLength, expectedMessage, receivedMessage);
//            //no break
//        case 13:
//            // Read Device Address register on device 13 (at address 12)
//            sciSendByte(sciREG, 0x81);      // SINGLE DEVICE WRITE W/RESPONSE
//            sciSendByte(sciREG, 0x0C);      // DEVICE ADDRESS
//            sciSendByte(sciREG, 0x0A);      // REGISTER ADDRESS
//            sciSendByte(sciREG, 0x00);      // EXPECTED RESPONSE DATA BYTES (1)
//            sciSendByte(sciREG, 0xEE);      // CRC
//            sciSendByte(sciREG, 0x9F);      // CRC
//
//            expectedMessage[0] = 0x00;      // RESPONSE
//            expectedMessage[1] = 0x0C;      // DATA
//            expectedMessage[2] = 0x00;      // CRC
//            expectedMessage[3] = 0x05;      // CRC
//
//            allGood &= BMS_messageIsExpected(expectedLength, expectedMessage, receivedMessage);
//            //no break
//        case 12:
//            // Read Device Address register on device 12 (at address 11)
//            sciSendByte(sciREG, 0x81);      // SINGLE DEVICE WRITE W/RESPONSE
//            sciSendByte(sciREG, 0x0B);      // DEVICE ADDRESS
//            sciSendByte(sciREG, 0x0A);      // REGISTER ADDRESS
//            sciSendByte(sciREG, 0x00);      // EXPECTED RESPONSE DATA BYTES (1)
//            sciSendByte(sciREG, 0x5F);      // CRC
//            sciSendByte(sciREG, 0x5E);      // CRC
//
//            expectedMessage[0] = 0x00;      // RESPONSE
//            expectedMessage[1] = 0x0B;      // DATA
//            expectedMessage[2] = 0x41;      // CRC
//            expectedMessage[3] = 0xC7;      // CRC
//
//            allGood &= BMS_messageIsExpected(expectedLength, expectedMessage, receivedMessage);
//            //no break
//        case 11:
//            // Read Device Address register on device 11 (at address 10)
//            sciSendByte(sciREG, 0x81);      // SINGLE DEVICE WRITE W/RESPONSE
//            sciSendByte(sciREG, 0x0A);      // DEVICE ADDRESS
//            sciSendByte(sciREG, 0x0A);      // REGISTER ADDRESS
//            sciSendByte(sciREG, 0x00);      // EXPECTED RESPONSE DATA BYTES (1)
//            sciSendByte(sciREG, 0x0E);      // CRC
//            sciSendByte(sciREG, 0x9E);      // CRC
//
//            expectedMessage[0] = 0x00;      // RESPONSE
//            expectedMessage[1] = 0x0A;      // DATA
//            expectedMessage[2] = 0x80;      // CRC
//            expectedMessage[3] = 0x07;      // CRC
//
//            allGood &= BMS_messageIsExpected(expectedLength, expectedMessage, receivedMessage);
//            //no break
//        case 10:
//            // Read Device Address register on device 10 (at address 9)
//            sciSendByte(sciREG, 0x81);      // SINGLE DEVICE WRITE W/RESPONSE
//            sciSendByte(sciREG, 0x09);      // DEVICE ADDRESS
//            sciSendByte(sciREG, 0x0A);      // REGISTER ADDRESS
//            sciSendByte(sciREG, 0x00);      // EXPECTED RESPONSE DATA BYTES (1)
//            sciSendByte(sciREG, 0xFE);      // CRC
//            sciSendByte(sciREG, 0x9E);      // CRC
//
//            expectedMessage[0] = 0x00;      // RESPONSE
//            expectedMessage[1] = 0x09;      // DATA
//            expectedMessage[2] = 0xC0;      // CRC
//            expectedMessage[3] = 0x06;      // CRC
//
//            allGood &= BMS_messageIsExpected(expectedLength, expectedMessage, receivedMessage);
//            //no break
//        case 9:
//            // Read Device Address register on device 9 (at address 8)
//            sciSendByte(sciREG, 0x81);      // SINGLE DEVICE WRITE W/RESPONSE
//            sciSendByte(sciREG, 0x08);      // DEVICE ADDRESS
//            sciSendByte(sciREG, 0x0A);      // REGISTER ADDRESS
//            sciSendByte(sciREG, 0x00);      // EXPECTED RESPONSE DATA BYTES (1)
//            sciSendByte(sciREG, 0xAF);      // CRC
//            sciSendByte(sciREG, 0x5E);      // CRC
//
//            expectedMessage[0] = 0x00;      // RESPONSE
//            expectedMessage[1] = 0x08;      // DATA
//            expectedMessage[2] = 0x01;      // CRC
//            expectedMessage[3] = 0xC6;      // CRC
//
//            allGood &= BMS_messageIsExpected(expectedLength, expectedMessage, receivedMessage);
//            //no break
//        case 8:
//            // Read Device Address register on device 8 (at address 7)
//            sciSendByte(sciREG, 0x81);      // SINGLE DEVICE WRITE W/RESPONSE
//            sciSendByte(sciREG, 0x07);      // DEVICE ADDRESS
//            sciSendByte(sciREG, 0x0A);      // REGISTER ADDRESS
//            sciSendByte(sciREG, 0x00);      // EXPECTED RESPONSE DATA BYTES (1)
//            sciSendByte(sciREG, 0x9F);      // CRC
//            sciSendByte(sciREG, 0x5D);      // CRC
//
//            expectedMessage[0] = 0x00;      // RESPONSE
//            expectedMessage[1] = 0x07;      // DATA
//            expectedMessage[2] = 0x41;      // CRC
//            expectedMessage[3] = 0xC2;      // CRC
//
//            allGood &= BMS_messageIsExpected(expectedLength, expectedMessage, receivedMessage);
//            //no break
//        case 7:
//            // Read Device Address register on device 7 (at address 6)
//            sciSendByte(sciREG, 0x81);      // SINGLE DEVICE WRITE W/RESPONSE
//            sciSendByte(sciREG, 0x06);      // DEVICE ADDRESS
//            sciSendByte(sciREG, 0x0A);      // REGISTER ADDRESS
//            sciSendByte(sciREG, 0x00);      // EXPECTED RESPONSE DATA BYTES (1)
//            sciSendByte(sciREG, 0xCE);      // CRC
//            sciSendByte(sciREG, 0x9D);      // CRC
//
//            expectedMessage[0] = 0x00;      // RESPONSE
//            expectedMessage[1] = 0x06;      // DATA
//            expectedMessage[2] = 0x80;      // CRC
//            expectedMessage[3] = 0x02;      // CRC
//
//            allGood &= BMS_messageIsExpected(expectedLength, expectedMessage, receivedMessage);
//            //no break
//        case 6:
//            // Read Device Address register on device 6 (at address 5)
//            sciSendByte(sciREG, 0x81);      // SINGLE DEVICE WRITE W/RESPONSE
//            sciSendByte(sciREG, 0x05);      // DEVICE ADDRESS
//            sciSendByte(sciREG, 0x0A);      // REGISTER ADDRESS
//            sciSendByte(sciREG, 0x00);      // EXPECTED RESPONSE DATA BYTES (1)
//            sciSendByte(sciREG, 0x3E);      // CRC
//            sciSendByte(sciREG, 0x9D);      // CRC
//
//            expectedMessage[0] = 0x00;      // RESPONSE
//            expectedMessage[1] = 0x05;      // DATA
//            expectedMessage[2] = 0xC0;      // CRC
//            expectedMessage[3] = 0x03;      // CRC
//
//            allGood &= BMS_messageIsExpected(expectedLength, expectedMessage, receivedMessage);
//            //no break
//        case 5:
//            // Read Device Address register on device 5 (at address 4)
//            sciSendByte(sciREG, 0x81);      // SINGLE DEVICE WRITE W/RESPONSE
//            sciSendByte(sciREG, 0x04);      // DEVICE ADDRESS
//            sciSendByte(sciREG, 0x0A);      // REGISTER ADDRESS
//            sciSendByte(sciREG, 0x00);      // EXPECTED RESPONSE DATA BYTES (1)
//            sciSendByte(sciREG, 0x6F);      // CRC
//            sciSendByte(sciREG, 0x5D);      // CRC
//
//            expectedMessage[0] = 0x00;      // RESPONSE
//            expectedMessage[1] = 0x04;      // DATA
//            expectedMessage[2] = 0x01;      // CRC
//            expectedMessage[3] = 0xC3;      // CRC
//
//            allGood &= BMS_messageIsExpected(expectedLength, expectedMessage, receivedMessage);
//            //no break
//        case 4:
//            // Read Device Address register on device 4 (at address 3)
//            sciSendByte(sciREG, 0x81);      // SINGLE DEVICE WRITE W/RESPONSE
//            sciSendByte(sciREG, 0x03);      // DEVICE ADDRESS
//            sciSendByte(sciREG, 0x0A);      // REGISTER ADDRESS
//            sciSendByte(sciREG, 0x00);      // EXPECTED RESPONSE DATA BYTES (1)
//            sciSendByte(sciREG, 0xDE);      // CRC
//            sciSendByte(sciREG, 0x9C);      // CRC
//
//            expectedMessage[0] = 0x00;      // RESPONSE
//            expectedMessage[1] = 0x03;      // DATA
//            expectedMessage[2] = 0x40;      // CRC
//            expectedMessage[3] = 0x01;      // CRC
//
//            allGood &= BMS_messageIsExpected(expectedLength, expectedMessage, receivedMessage);
//            //no break
//        case 3:
//            // Read Device Address register on device 3 (at address 2)
//            sciSendByte(sciREG, 0x81);      // SINGLE DEVICE WRITE W/RESPONSE
//            sciSendByte(sciREG, 0x02);      // DEVICE ADDRESS
//            sciSendByte(sciREG, 0x0A);      // REGISTER ADDRESS
//            sciSendByte(sciREG, 0x00);      // EXPECTED RESPONSE DATA BYTES (1)
//            sciSendByte(sciREG, 0x8F);      // CRC
//            sciSendByte(sciREG, 0x5C);      // CRC
//
//            expectedMessage[0] = 0x00;      // RESPONSE
//            expectedMessage[1] = 0x02;      // DATA
//            expectedMessage[2] = 0x81;      // CRC
//            expectedMessage[3] = 0xC1;      // CRC
//
//            allGood &= BMS_messageIsExpected(expectedLength, expectedMessage, receivedMessage);
//            //no break
//        case 2:
//            // Read Device Address register on device 2 (at address 1)
//            sciSendByte(sciREG, 0x81);      // SINGLE DEVICE WRITE W/RESPONSE
//            sciSendByte(sciREG, 0x01);      // DEVICE ADDRESS
//            sciSendByte(sciREG, 0x0A);      // REGISTER ADDRESS
//            sciSendByte(sciREG, 0x00);      // EXPECTED RESPONSE DATA BYTES (1)
//            sciSendByte(sciREG, 0x7F);      // CRC
//            sciSendByte(sciREG, 0x5C);      // CRC
//
//            expectedMessage[0] = 0x00;      // RESPONSE
//            expectedMessage[1] = 0x01;      // DATA
//            expectedMessage[2] = 0xC1;      // CRC
//            expectedMessage[3] = 0xC0;      // CRC
//
//            allGood &= BMS_messageIsExpected(expectedLength, expectedMessage, receivedMessage);
//            //no break
//        case 1:
//            // Read Device Address register on device 1 (at address 0)
//            sciSendByte(sciREG, 0x81);      // SINGLE DEVICE WRITE W/RESPONSE
//            sciSendByte(sciREG, 0x00);      // DEVICE ADDRESS
//            sciSendByte(sciREG, 0x0A);      // REGISTER ADDRESS
//            sciSendByte(sciREG, 0x00);      // EXPECTED RESPONSE DATA BYTES (1)
//            sciSendByte(sciREG, 0x2E);      // CRC
//            sciSendByte(sciREG, 0x9C);      // CRC
//
//            expectedMessage[0] = 0x00;      // RESPONSE
//            expectedMessage[1] = 0x00;      // DATA
//            expectedMessage[2] = 0x00;      // CRC
//            expectedMessage[3] = 0x00;      // CRC
//
//            allGood &= BMS_messageIsExpected(expectedLength, expectedMessage, receivedMessage);
//            break;
//        default:
//            return 0;
//            break;
//    }
//
//    return allGood;
}

bool BMS_messageIsExpected(uint8 length, uint8 * expected, uint8 * received){
    int i = 0;
    bool messageIsCorrect;
    for(i=0;i<length;i++){
        received[i] = BMS_receiveByte();
        if(received[i] == 0xFFFFFFFFFFFFFFFF){
            return 0; // TIMEOUT
        }
        messageIsCorrect &= (received[i] == expected[i]);
    }

    return messageIsCorrect;

}

// 1.2.5
void BMS_disableTopHighSideRx(){
    // Set the communication configuration on top-most device
    uint8 message[5] = {0x92,BMS_TOTALBOARDS-1,0x10,0x10,0x20};
    BMS_sendMessage(message, 5);

//    sciSendByte(sciREG, 0x92);                      // SINGLE DECIVE WRITE W/O RESPONSE (2 BYTES)
//    sciSendByte(sciREG, BMS_TOTALBOARDS-1);         // DEVICE ADDRESS
//    sciSendByte(sciREG, 0x10);                      // REGISTER ADDRESS
//    sciSendByte(sciREG, 0x10);                      // DATA (BAUD RATE 250k)
//    sciSendByte(sciREG, 0x20);                      // DATA (DISABLE HIGH SIDE)
//    sciSendByte(sciREG, 0xB5);                      // CRC
//    sciSendByte(sciREG, 0xFC);                      // CRC
}

// 1.2.6
void BMS_disableBottomLowSideRx(){
    // Set the communication configuration on bottom-most device
    uint8 message[5] = {0x92,0x00,0x10,0x10,0xC0};
    BMS_sendMessage(message, 5);

    return;
}

void BMS_clearFaults(){
    uint8 i = 0;
    uint8 message[5] = {0x92,BMS_TOTALBOARDS-1,0x52,0xFF,0xC0};
    for(i=BMS_TOTALBOARDS;i>0;i--){
        message[1] = BMS_TOTALBOARDS-1;
        BMS_sendMessage(message,5);
    }

    return;
}
