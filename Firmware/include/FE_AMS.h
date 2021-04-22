/*
 * FE_AMS.h
 *
 *  Created on: Apr 15, 2021
 *      Author: Mikel
 */

#ifndef FE_AMS_H_
#define FE_AMS_H_

void setCurrentVehicleVoltage(uint16 x);
void setCurrentBatteryVoltage(uint16 x);
void dataConfigFnct();
void AMS_sendBMSWakeup(void);
uint16 getDelta();
void AMS_start_HV(void);
void AMS_readADC();


#endif /* FE_AMS_H_ */
