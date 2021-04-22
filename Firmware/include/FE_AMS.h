/*
 * FE_AMS.h
 *
 *  Created on: Apr 15, 2021
 *      Author: Mikel
 */

#ifndef FE_AMS_H_
#define FE_AMS_H_

typdef struct {
    uint8 AMS_FAULT_CELL_OVER_VOLTAGE : 1;        // A cell voltage is too high
    uint8 AMS_FAULT_CELL_UNDER_VOLTAGE : 1;       // A cell voltage is too low
    uint8 AMS_FAULT_CELL_OVER_TEMP : 1;           // A cell temperature is too high
    uint8 AMS_FAULT_CELL_UNDER_TEMP : 1;          // A cell temperature is too low
    uint8 AMS_FAULT_OVER_DISCHARGE_CURRENT : 1;   // Too much discharge current
    uint8 AMS_FAULT_OVER_CHARGE_CURRENT : 1;      // Too much charge current
    uint8 AMS_FAULT_CONTACTOR_FAULT : 1;          // A contactor failed to close or open
    uint8 AMS_FAULT_CELL_UPDATE_FAILURE : 1;      // Failed to get voltage or temperature from cell
    uint8 AMS_FAULT_ISOLOATION_LOST : 1;          // IMD Detected an isolation fault
} Faults_t;

void setCurrentVehicleVoltage(uint16 x);
void setCurrentBatteryVoltage(uint16 x);
void dataConfigFnct();
void AMS_sendBMSWakeup(void);
uint16 getDelta();
void AMS_start_HV(void);
void AMS_readADC();


#endif /* FE_AMS_H_ */
