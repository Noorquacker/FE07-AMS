/*
 * BMS.h
 *
 *  Created on: Apr 23, 2021
 *      Author: Mikel
 */

#ifndef INCLUDE_BMS_H_
#define INCLUDE_BMS_H_


bool BMS_Init();
uint32 BMS_receiveByte();
void BMS_getCRCBytes(uint8 * CRC, uint8 * message, uint8 length);
void BMS_sendMessage(uint8 * message, uint8 length);
void BMS_disableTopHighSideRx();
void BMS_disableBottomLowSideRx();
bool BMS_messageIsExpected(uint8 length, uint8 * expected, uint8 * received);
bool BMS_checkHeartbeats();
bool BMS_setAddresses();
void BMS_initAutoAddress();
void BMS_initialConfig();

#endif /* INCLUDE_BMS_H_ */
