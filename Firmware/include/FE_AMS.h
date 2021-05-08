/*
 * FE_AMS.h
 *
 *  Created on: Apr 15, 2021
 *      Author: Mikel
 */

#ifndef FE_AMS_H_
#define FE_AMS_H_



//typedef struct {
//	unsigned int CELL_OVER_VOLTAGE : 1;        // A cell voltage is too high
//    unsigned int CELL_UNDER_VOLTAGE : 1;       // A cell voltage is too low
//    unsigned int CELL_OVER_TEMP : 1;           // A cell temperature is too high
//    unsigned int CELL_UNDER_TEMP : 1;          // A cell temperature is too low
//    unsigned int OVER_DISCHARGE_CURRENT : 1;   // Too much discharge current
//    unsigned int OVER_CHARGE_CURRENT : 1;      // Too much charge current
//    unsigned int CONTACTOR_FAULT : 1;          // A contactor failed to close or open
//    unsigned int CELL_UPDATE_FAILURE : 1;      // Failed to get voltage or temperature from cell
//    unsigned int ISOLATION_LOST : 1;           // IMD Detected an isolation fault
//    unsigned int unused : 7;
//} faults_t;

void setCurrentVehicleVoltage(uint16 x);
void setCurrentBatteryVoltage(uint16 x);
void dataConfigFnct();
void AMS_sendBMSWakeup(void);
uint16 getDelta();
void AMS_startHV(void);
void AMS_readADC();
void AMS_readSPI();
void AMS_canTX_Car();
void AMS_canTx_BMSData();
void AMS_parseBMSData(uint8 *buffer, uint8 numCells, uint8 numAux, bool digitalDie, bool analogDie);

#endif /* FE_AMS_H_ */
