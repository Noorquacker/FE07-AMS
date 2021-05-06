/*
 * BMS.h
 *
 *  Created on: Apr 23, 2021
 *      Author: Mikel
 */

#ifndef INCLUDE_BMS_H_
#define INCLUDE_BMS_H_

// MAKE SURE TO DEFINE BMS_TOTALBOARDS in AMS_common.h (otherwise, define here)
void BMS_configureGPIO();

bool BMS_Init();
uint32 BMS_receiveByte();
bool BMS_receiveMessage(uint8 * message, uint8 length);
void BMS_getCRCBytes(uint8 * CRC, uint8 * message, uint8 length);
void BMS_sendMessage(uint8 * message, uint8 length);
bool BMS_messageIsExpected(uint8 * expected, uint8 * received, uint8 length);

void BMS_wakeup();
void BMS_initialConfig();
void BMS_initAutoAddress();
void BMS_setAddresses();
bool BMS_checkHeartbeats();
void BMS_disableTopHighSideRx();
void BMS_disableBottomLowSideRx();
void BMS_clearFaults();
void BMS_setSamplingDelay();
void BMS_setSamplePeriods();
uint16 BMS_checkFault(uint8 device);
bool BMS_checkAllFaults(uint16 * buffer);
void BMS_setSingleModuleNumChannels(uint8 device, uint8 numCells, uint8 numAux, bool digitalDie, bool analogDie);
void BMS_setAllModulesNumChannels(uint8 numCells, uint8 numAux, bool digitalDie, bool analogDie);
void BMS_setSingleModuleOvervolt(uint8 device,float threshold);
void BMS_setSingleModuleUndervolt(uint8 device,float threshold);
void BMS_setAllOvervolt(float threshold);
void BMS_setAllUndervolt(float threshold);
bool BMS_getBroadcastData(uint8 * buffer, uint16 datasize);
void BMS_syncSampleAll();
bool BMS_getAllIndividualData(uint8 * buffer, uint16 datasize);
#endif /* INCLUDE_BMS_H_ */
