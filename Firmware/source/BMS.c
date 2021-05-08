/*
 * BMS.c
 *
 *  Created on: Apr 23, 2021
 *      Author: Mikel
 */


// All functions are based on this documentation:
// https://www.ti.com/lit/an/slva617a/slva617a.pdf?ts=1619120159432&ref_url=https%253A%252F%252Fwww.ti.com%252Fproduct%252FBQ76PL455A
// (also available in the /Resources/BMS/ folder in the git repo

#include <stdbool.h>
#include <math.h>
#include "AMS_common.h"
#include "gio.h"
#include "sci.h"
#include "BMS.h"
#include "pl455.h"


// MAKE SURE TO DEFINE BMS_TOTALBOARDS in AMS_common.h
bool BMS_Init(){
	bool initSuccess = true;
	uint16 faults[BMS_TOTALBOARDS] = {0};
	uint8 i = 0;
	uint16 timeout = 0;

	sciSetBaudrate(sciREG, BMS_BAUDRATE);
	for(i=0;i<BMS_TOTALBOARDS;i++){
		delayms(2); //~5ms
		BMS_wakeup();
	}

	BMS_initialConfig();
	delayms(2);
	BMS_initAutoAddress();
	delayms(2);
	BMS_setAddresses();
	delayms(2);
	while(!BMS_checkHeartbeats()){
		if(timeout > 100){
			initSuccess = false;
			return initSuccess;
		}
		delayms(2); //~5ms
		BMS_wakeup();
		delayms(2);
		BMS_initialConfig();
		delayms(2);
		BMS_initAutoAddress();
		delayms(2);
		BMS_setAddresses();
		delayms(2);
		timeout++;
	}
	initSuccess &= BMS_checkHeartbeats();
	delayms(2);
	BMS_disableTopHighSideRx();
	delayms(2);
	BMS_disableBottomLowSideRx();
	delayms(2);
	BMS_clearFaults();
	delayms(2);
	BMS_configureGPIO();
	delayms(2);
	BMS_setSamplingDelay();
	delayms(2);
	BMS_setSamplePeriods();
	delayms(2);
	initSuccess &= BMS_checkAllFaults(faults);
	delayms(2);
	BMS_setAllModulesNumChannels(16,8,1,1);
	delayms(2);
	BMS_setAllOvervolt(BMS_CELL_OVERVOLTAGE_THRESHOLD);
	delayms(2);
	BMS_setAllUndervolt(BMS_CELL_UNDERVOLTAGE_THRESHOLD);
	delayms(2);
	initSuccess &= BMS_checkAllFaults(faults);

	return initSuccess;
}


void BMS_wakeup(){
	// toggle wake signal
	gioSetBit(gioPORTB, GIOB_BMS_WAKEUP, 0); // assert wake (active low)
	delayus(10);
	gioToggleBit(gioPORTB, GIOB_BMS_WAKEUP); // deassert wake
}


// Modification of sciReceiveByte with an added timeout, let's hope it works :sunglasses: :grimacing:
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
        return 0xFFFFFFFF; // TIMEOUT
    }
    return (sci->RD & (uint32)0x000000FFU);
}

bool BMS_receiveMessage(uint8 * message, uint8 length){
	uint8 i = 0;
	uint32 rx = 0x00;
	for(i=0;i<length;i++){
		rx = BMS_receiveByte();
		if(rx == 0xFFFFFFFF){
			return false;
		}
		message[i] = rx & 0xFF;
	}
	return true;
}

void BMS_getCRCBytes(uint8 * CRC, uint8 * message, uint8 length){
    uint16 crc_r = 0;
    int l = length;
    crc_r = CRC16(message, l);

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


bool BMS_messageIsExpected(uint8 * expected, uint8 * received, uint8 length){
    int i = 0;
    bool messageIsCorrect = 1;
    uint32 x = 0;

    sciReceive(sciREG, length, received);

    for(i=0;i<length;i++){
//        x = BMS_receiveByte();
//        if(x == 0xFFFFFFFF){
//            return 0; // TIMEOUT
//        }
//        received[i] = x & 0xFF;
        messageIsCorrect &= (received[i] == expected[i]);
    }

    return messageIsCorrect;

}


// 1.2.1
void BMS_initialConfig(){
    uint8 message[4] = {0xF2,0x10,0x10,0xE0};
    BMS_sendMessage(message, 4);

    return;
}

//1.2.2
void BMS_initAutoAddress(){
    // Configure the bq76PL455A-Q1 device to use auto-addressing to select address
    uint8 messageOne[3] = {0xF1,0x0E,0x10};
    BMS_sendMessage(messageOne, 3);

    delayms(5);
    // Configure the bq76PL455A-Q1 device to enter auto-address mode
    uint8 messageTwo[3] = {0xF1,0x0C,0x08};
    BMS_sendMessage(messageTwo, 3);

    return;
}

// 1.2.3
void BMS_setAddresses(){
    // Write a new address sequentially to the Device Address register
	uint8 i = 0;
	uint8 message[3] = {0xF1,0x0A,0x00};


	for(i=0; i<BMS_TOTALBOARDS;i++){
		message[2] = i;
		delayms(5);
		BMS_sendMessage(message,3);
	}

	return;
}

// 1.2.4
bool BMS_checkHeartbeats(){
	bool boardsAreAlive = true;
    uint8 i = 0;
    uint8 message[4] = {0x81,0x00,0x0A,0x00};
	uint8 expectedMessage[4] = {0x00,0x00,0x00,0x00};
	uint8 receivedMessage[4] = {0x00,0x00,0x00,0x00};
	uint8 expectedData[2] = {0x00,0x00};
	uint8 expectedCRC[2] = {0x00,0x00};

//	BMS_sendMessage(message,4);
//
//	delayms(5);
//	message[1] = 1;
//	BMS_sendMessage(message,4);
//
//	message[1] = 0;
//	delayms(5);
//	BMS_sendMessage(message,4);
//	delayms(5);
    for(i=0; i<BMS_TOTALBOARDS;i++){
        message[1] = i;
        delayms(5);
        expectedData[1] = i;
        BMS_getCRCBytes(expectedCRC, expectedData, 2);
        expectedMessage[0] = expectedData[0];
        expectedMessage[1] = expectedData[1];
        expectedMessage[2] = expectedCRC[1];
        expectedMessage[3] = expectedCRC[0];

        BMS_sendMessage(message,4);
//        sciReceive(sciREG, 4, receivedMessage);
        boardsAreAlive &= BMS_messageIsExpected(expectedMessage, receivedMessage, 4);
        delayms(5);
    }
    delayms(5);
    return boardsAreAlive;
}


// 1.2.5
void BMS_disableTopHighSideRx(){
    // Set the communication configuration on top-most device
    uint8 message[5] = {0x92,BMS_TOTALBOARDS-1,0x10,0x10,0x20};
    BMS_sendMessage(message, 5);

}

// 1.2.6
void BMS_disableBottomLowSideRx(){
    // Set the communication configuration on bottom-most device
    uint8 message[5] = {0x92,0x00,0x10,0x10,0xC0};
    BMS_sendMessage(message, 5);
    return;
}

// 1.2.7
void BMS_clearFaults(){
    uint8 i = 0;
    uint8 message[5] = {0x92,BMS_TOTALBOARDS-1,0x52,0xFF,0xC0};
    for(i=BMS_TOTALBOARDS;i>0;i--){
        delayms(2);
        message[1] = BMS_TOTALBOARDS-1;
        BMS_sendMessage(message,5);
    }

    return;
}

// 2.2.1
void BMS_setSamplingDelay(){
    uint8 i = 0;
	uint8 message[4] = {0x91,0x00,0x3D,0x00};

	for(i=0;i<BMS_TOTALBOARDS;i++){
		message[1] = i;
		BMS_sendMessage(message,4);
	    delayus(500);
	}
	return;
}

// 2.2.2
void BMS_setSamplePeriods(){
    uint8 i = 0;
	uint8 message[4] = {0x91,0x00,0x3E,0xBC};

	for(i=0;i<BMS_TOTALBOARDS;i++){
		message[1] = i;
		BMS_sendMessage(message,4);
	}
	return;
}

// 2.2.3
void BMS_configOversample(){
	uint8 i = 0;
	uint8 message[4] = {0x91,0x00,0x07,0x00};

	for(i=0;i<BMS_TOTALBOARDS;i++){
		message[1] = i;
		BMS_sendMessage(message,4);
	}
	return;
}

// 2.2.4
uint16 BMS_checkFault(uint8 device){
	uint16 faults = 0xFFFF;
	uint8 checkStatus[4] = {0x81,device,0x51,0x00};
	uint8 noFaultsMessage[4] = {0x00,0x00,0x00,0x00};
	uint8 receivedMessage[5] = {0x00,0x00,0x00,0x00};

	// Check Device Status
    delayms(5);
	BMS_sendMessage(checkStatus,4);
	if(BMS_messageIsExpected(noFaultsMessage, receivedMessage, 4))
		return 0;

	// If Device Status is NOT 0x00000000
	// Check Fault Summary
	delayms(5);
	uint8 checkFaultSummary[4] = {0x81,device,0x52,0x01};
	BMS_sendMessage(checkFaultSummary,4);
	if(!BMS_receiveMessage(receivedMessage, 5)){
		return faults;
	}


    delayms(5);

	faults &= receivedMessage[1]<<8 & 0xFFFF;
	faults &= receivedMessage[2]<<0 & 0xFFFF;
	return faults;
}

// 2.2.4
bool BMS_checkAllFaults(uint16 * buffer){
	bool isFaulted = 1;
	uint8 i = 0;
	for(i=0; i<BMS_TOTALBOARDS; i++){
		buffer[i] = BMS_checkFault(i);
		if(buffer[i] != 0){
			isFaulted &= 1;
		}
	}
	return isFaulted;
}

// 2.2.5.1
void BMS_setSingleModuleNumChannels(uint8 device, uint8 numCells, uint8 numAux, bool digitalDie, bool analogDie){
    // Select Number of Cells and Channels on a Single Module
	// Write to Register 13 - Number of Cell Channels
	uint8 message[4] = {0x91,device,0x0D,numCells};
	BMS_sendMessage(message, 4);

    uint8 i = 0;
    uint16 cells = 1;
    for(i=0;i<numCells;i++){
    	cells |= (1<<i);
    }
    uint8 cellBytes[2] = {cells<<8&0xFF, cells&0xFF};

    uint8 aux = 1;
	for(i=0;i<numAux;i++){
		aux |= (1<<i);
	}

	uint8 die = 0x00;
	if(digitalDie){
		die |= (1<<8);
	}
	if(analogDie){
		die |= (1<<7);
	}

    // Write to Register 3 - Number of Cell Channels, AuxChannels, Internal Digital Die and Analog Die Temps
    uint8 messageTwo[7] = {0x94,device,0x03,cellBytes[0],cellBytes[1],aux,die};
	BMS_sendMessage(messageTwo, 7);

    return;
}

// 2.2.5.2
void BMS_setAllModulesNumChannels(uint8 numCells, uint8 numAux, bool digitalDie, bool analogDie){
	// Select Number of Cells and Channels on a Single Module
	// Write to Register 13 - Number of Cell Channels
	uint8 message[3] = {0xF1,0x0D,numCells};
    delayms(5);
	BMS_sendMessage(message, 3);
    delayms(5);

	uint8 i = 0;
	uint16 cells = 1;
	for(i=0;i<numCells;i++){
		cells |= (1<<i);
	}
	uint8 cellBytes[2] = {(cells>>8)&0xFF, (uint8)(cells)};

	uint8 aux = 1;
	for(i=0;i<numAux;i++){
		aux |= (1<<i);
	}

	uint8 die = 0x00;
	if(digitalDie){
		die |= (1<<7);
	}
	if(analogDie){
		die |= (1<<6);
	}

	// Write to Register 3 - Number of Cell Channels, AuxChannels, Internal Digital Die and Analog Die Temps
	uint8 messageTwo[6] = {0xF4,0x03,cellBytes[0],cellBytes[1],aux,die};
	BMS_sendMessage(messageTwo, 6);

	return;
}

void BMS_setDefaultModulesNumChannels(){

}

void BMS_setSingleModuleOvervolt(uint8 device,float threshold){
	uint16 OV = (uint16)(ceil(13107*threshold));
	uint8 OVBytes[2] = {((OV<<8) & 0xFF), (OV & 0xFF)};
	uint8 message[5] = {0x92,device,0x90,OVBytes[0],OVBytes[1]};
	BMS_sendMessage(message, 5);
	return;
}

void BMS_setSingleModuleUndervolt(uint8 device,float threshold){
	uint16 UV = (uint16)(ceil(13107*threshold));
	uint8 UVBytes[2] = {((UV<<8) & 0xFF), (UV & 0xFF)};
	uint8 message[5] = {0x92,device,0x8E,UVBytes[0],UVBytes[1]};
	BMS_sendMessage(message, 5);
	return;
}

void BMS_setAllOvervolt(float threshold){
	uint16 OV = (uint16)(ceil(13107*threshold));
	uint8 OVBytes[2] = {((OV>>8) & 0xFF), (OV & 0xFF)};
	uint8 message[4] = {0xF2,0x90,OVBytes[0],OVBytes[1]};
	BMS_sendMessage(message, 4);
	return;
}

void BMS_setAllUndervolt(float threshold){
	uint16 UV = (uint16)(ceil(13107*threshold));
	uint8 UVBytes[2] = {((UV>>8) & 0xFF), (UV & 0xFF)};
	uint8 message[4] = {0xF2,0x8E,UVBytes[0],UVBytes[1]};
	BMS_sendMessage(message, 4);
	return;
}

// 5.2
void BMS_configureGPIO(){
    uint8 i = 0;
    uint8 message[4] = {0x91,0x00,0x7B,0x00};
    for(i=0;i<BMS_TOTALBOARDS;i++){
        // Turn off all pulldowns
        message[1] = i;
        BMS_sendMessage(message,4);
        delayms(5);
        // Turn off all pullups
        message[2] = 0x7A;
        BMS_sendMessage(message,4);
        delayms(5);
        // Configure as inputs
        message[2] = 0x78;
        message[3] = 0x00; // ALL INPUTS
        BMS_sendMessage(message,4);
        delayms(5);
    }

    return;
}

// 3.2 Get Data from all boards with a broadcast request.
// Boards will respond in Descending Order (Board at highest address first, down to address 0)
// Datasize is the amount of data that is expected from only ONE of the boards
bool BMS_getBroadcastData(uint8 * buffer, uint16 datasize){
    bool success = true;
	uint8 message[3] = {0xE1,0x02,BMS_TOTALBOARDS-1};
	uint8 crc[2] = {0,0};
    BMS_getCRCBytes(crc, message,3);
    uint8 Tmessage[5] = {message[0],message[1],message[2],crc[1],crc[0]};
    uint8 tmp2[1] = {0};
    sciReceive(sciREG,1,tmp2);
//	BMS_sendMessage(message, 3);
    delayms(5);
	sciSend(sciREG, 5, Tmessage);
	sciReceive(sciREG, datasize, buffer);
    delayms(10);

//    sciSend(sciREG, 5, Tmessage);
//    sciReceive(sciREG,1,tmp2);

	//return true;
//	 success &= BMS_receiveMessage(buffer, datasize*BMS_TOTALBOARDS);
//
//	 sciSend(sciREG,1,Tmessage[1]);
//
//	 return success;
}


// 3.3.1 - Send Broadcast Request to All bq76PL455A-Q1 Devices to Sample and Store Results
void BMS_syncSampleAll(){
	uint8 message[3] = {0xF1,0x02,0x00};
	BMS_sendMessage(message, 3);
    delayus(500);
	return;
}

// 3.3.2 Get Data from all boards with a individual request.
// Boards will respond in Descending Order (Board at highest address first, down to address 0)
// Datasize is the amount of data that is expected from only ONE of the boards
bool BMS_getAllIndividualData(uint8 * buffer, uint16 datasize){
	bool success = true;
	uint8 i = 0;
	uint8 j = 0;
    delayus(5000);
	BMS_syncSampleAll();
	uint8 tmp[55] = {0};
	uint8 tmp2[1] = {0};
	BMS_receiveByte(); //sciReceive(sciREG,1,tmp2);
    uint8 message[4] = {0x81,0x00,0x02,0x20};
//    uint8 message[9] = {0x96,0x00,0x02,0x00,0xFF,0xFF,0x0F,0xC0,0x00};

	for(i=0;i<BMS_TOTALBOARDS;i++){
		message[1] = BMS_TOTALBOARDS-(1+i);
		BMS_sendMessage(message, 4);
//		success &= BMS_receiveMessage(tmp, datasize);
		sciReceive(sciREG,datasize,tmp);
		for(j=0;j<datasize;j++){
			buffer[(i*datasize)+j] = tmp[j];
		}
        delayus(5000);
	}

	delayus(2500);
//    BMS_sendMessage(message, 4);
//    BMS_receiveMessage(tmp, datasize);



	return success;
}
