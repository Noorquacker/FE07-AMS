/*
 * FE_AMS.c
 *
 *  Created on: Apr 15, 2021
 *      Author: Mikel
 */

// INCLUDES
#include <stdbool.h>
#include "adc.h"
#include "can.h"
#include "het.h"
#include "gio.h"
#include "mibspi.h"
#include "pl455.h"
#include "sci.h"
#include "spi.h"
#include "AMS_common.h"


float64 imdPeriod = 0;
uint32 imdDutyCycle = 0;

uint32 timeoutBMS = 0;
uint32 timeoutCAR = 0;

uint16 currentSense_75 = 0;
uint16 currentSense_750 = 0;

uint16 currentVehicleVoltage = 0;
uint16 currentBatteryVoltage = 0;

uint16 stateOfCharge = 0;
uint16 minCellTemperature_Scaled = 0;
uint16 maxCellTemperature_Scaled = 0;

uint16 minCellVoltage_Scaled = 0;
uint16 maxCellVoltage_Scaled = 0;
uint16 vehicleVoltage_Scaled = 0;
uint16 batteryVoltage_Scaled = 0;

uint16 peakCurrent_Scaled = 0;
uint16 chargeCurrentLimit_Scaled = 0;
uint16 dischargeCurrentLimit_Scaled = 0;
uint16 packCurrent_Scaled = 0;

uint16 AMS_faults = 0xFFFF;

uint8 AMS_DATA[164];

bool negativeContactorState  = 0;
bool positiveContactorState  = 0;
bool prechargeContactorState = 0;

bool contactorOpen = 0;
bool contactorClosed = 0;

bool fiveKWActive = 0;
bool currentShortFault = 0;

bool scOutsideSense = 0;

bool cellOverVoltage = 0;        // A cell voltage is too high
bool cellUnderVoltage = 0;       // A cell voltage is too low
bool cellOverTemp = 0;           // A cell temperature is too high
bool cellUnderTemp = 0;          // A cell temperature is too low
bool overDischargeCurrent = 0;   // Too much discharge current
bool overChargeCurrent = 0;      // Too much charge current
bool contactorFault = 0;         // A contactor failed to close or open
bool bmsFault = 0;      		 // Failed to get voltage or temperature from cell
bool imdFault = 0;          	 // IMD Detected an isolation fault
bool bmsCommFault= 0;

spiDAT1_t dataconfig00_t = {TRUE, TRUE, SPI_FMT_0, 0xFE};

//void dataConfigFnct(){
//    dataconfig00_t.CS_HOLD = TRUE;
//    dataconfig00_t.WDEL    = TRUE;
//    dataconfig00_t.DFSEL   = SPI_FMT_0;
//    dataconfig00_t.CSNR    = 0xFE;
//}

uint16 getDelta() {
    uint16 x = currentBatteryVoltage - currentVehicleVoltage;
    x = x*(-1);
    return x;
}

void AMS_start_HV(void){
//    dataConfigFnct();
    uint16 x = getDelta();
    // Engage Negative Contactor
    gioSetBit(hetPORT1,HET1_NEGATIVE_CONTACT_CTRL,1);
    // Wait Until Negative Contactor Close is Sensed
    while(!gioGetBit(gioPORTA, GIOA_NEG_CONTACT_SENSE_FILTERED));
    // Engage Precharge Contactor
    gioSetBit(hetPORT1,HET1_PRECHARGE_CONTACT_CTRL,1);
    prechargeContactorState = 1;
    // Wait until Delta between Vehicle Voltage and Battery Voltage is greater than 5
    while(x<5){
        x = getDelta();
//        spiReceiveData(spiREG1, &dataconfig00_t, 1, RX_Data_M);
//        spiReceiveData(spiREG3, &dataconfig00_t, 1, RX_Data_S);
//        setCurrentBatteryVoltage(RX_Data_S[0]);
//        setCurrentVehicleVoltage(RX_Data_M[0]);
    }
    // Engage Positive Contactor
    gioSetBit(hetPORT1,HET1_POSITIVE_CONTACT_CTRL,1);
    // Disengage Precharge Contactor
    gioSetBit(hetPORT1,HET1_PRECHARGE_CONTACT_CTRL,0);
    prechargeContactorState = 0;

    return;
}


uint16 AMS_checkForFaults(){
	AMS_faults = 0xFFFF;
	if(!cellOverVoltage)
		AMS_faults &= ~FAULT_CELL_OVR_VOLT;

	if(!cellUnderVoltage)
		AMS_faults &= ~FAULT_CELL_UND_VOLT;

	if(!cellOverTemp)
		AMS_faults &= ~FAULT_CELL_OVR_TEMP;

	if(!cellUnderTemp)
		AMS_faults &= ~FAULT_CELL_UND_TEMP;

	if(!overDischargeCurrent)
		AMS_faults &= ~FAULT_OVER_DCG_CURRENT;

	if(!overChargeCurrent)
		AMS_faults &= ~FAULT_OVER_CHG_CURRENT;

	if(!contactorFault)
		AMS_faults &= ~FAULT_CONTACTOR;

	if(!bmsFault)
		AMS_faults &= ~FAULT_BMS;

	if(!bmsCommFault)
		AMS_faults &= ~FAULT_BMS_COMM;

	if(!imdFault)
		AMS_faults &= ~FAULT_ISOLATION;

	return AMS_faults;
}



void setCurrentVehicleVoltage(uint16 x){
    currentVehicleVoltage = x;
    return;
}

void setCurrentBatteryVoltage(uint16 x){
    currentBatteryVoltage = x;
    return;
}

void AMS_readADC() {
	adcData_t adc_data[2];

	adcStartConversion(adcREG1, adcGROUP1);
	while(!adcIsConversionComplete(adcREG1,adcGROUP1));
	adcGetData(adcREG1,adcGROUP1,&adc_data[0]);

	currentSense_75 = adc_data[0].value;
	currentSense_750 = adc_data[1].value ;

	return;
}


void AMS_readGIO() {
	//PORT A
	bmsFault = gioGetBit(gioPORTA, GIOA_BMS_FAULT);
	imdFault = gioGetBit(gioPORTA, GIOA_IMD_FAULT);
	negativeContactorState = gioGetBit(gioPORTA, GIOA_NEG_CONTACT_SENSE_FILTERED);
	positiveContactorState = gioGetBit(gioPORTA, GIOA_POS_CONTACT_SENSE_FILTERED);
	contactorOpen = gioGetBit(gioPORTA, GIOA_CONTACT_OPEN_INPUT);
	contactorClosed = gioGetBit(gioPORTA, GIOA_CONTACT_CLOSED_INPUT);

	// PORT B
	fiveKWActive = gioGetBit(gioPORTB, GIOB_5KW_INPUT);
	currentShortFault = gioGetBit(gioPORTB, GIOB_CURRENT_SHORT_FAULT);

	return;
}

void AMS_readHET(){
	hetSIGNAL_t imd_data[1];

	scOutsideSense = gioGetBit(hetPORT1, HET1_SC_OUTSIDE_SENSE);


	// READ IMD PWM
	capGetSignal(hetRAM1, cap0, &imd_data[0]);

	imdDutyCycle = imd_data[0].duty;
	imdPeriod = imd_data[0].period;
}

void AMS_canTX_Car(){
	uint8 tx_data1[8] = {0};
	uint8 tx_data2[8] = {0};
	uint8 tx_data3[8] = {0};

	tx_data1[7] = stateOfCharge & 0xFF;
	tx_data1[6] = stateOfCharge >> 8;
	tx_data1[5] = minCellTemperature_Scaled & 0xFF;
	tx_data1[4] = minCellTemperature_Scaled >> 8;
	tx_data1[3] = maxCellTemperature_Scaled & 0xFF;
	tx_data1[2] = maxCellTemperature_Scaled >> 8;
	tx_data1[1] = 0x00 |
				  ((AMS_faults & 0x01) <<7)	  |
				  prechargeContactorState <<4 |
				  negativeContactorState  <<5 |
				  positiveContactorState  << 6;
    tx_data1[0] = (AMS_faults >> 1) & 0xFF;

	tx_data2[7] = minCellVoltage_Scaled & 0xFF;
	tx_data2[6] = minCellVoltage_Scaled >> 8;
	tx_data2[5] = maxCellVoltage_Scaled & 0xFF;
	tx_data2[4]	= maxCellVoltage_Scaled >> 8;
	tx_data2[3] = vehicleVoltage_Scaled & 0xFF;
 	tx_data2[2] = vehicleVoltage_Scaled >> 8;
	tx_data2[1] = batteryVoltage_Scaled & 0xFF;
	tx_data2[0] = batteryVoltage_Scaled >> 8;

	tx_data3[7] = peakCurrent_Scaled & 0xFF;
	tx_data3[6] = peakCurrent_Scaled >> 8;
	tx_data3[5] = chargeCurrentLimit_Scaled & 0xFF;
	tx_data3[4]	= chargeCurrentLimit_Scaled >> 8;
	tx_data3[3] = dischargeCurrentLimit_Scaled & 0xFF;
 	tx_data3[2] = dischargeCurrentLimit_Scaled >> 8;
	tx_data3[1] = packCurrent_Scaled & 0xFF;
	tx_data3[0] = packCurrent_Scaled >> 8;

    canTransmit(canREG1, CANBOX_AMS_STATUS, tx_data1);
    canTransmit(canREG1, CANBOX_AMS_VOLTS, tx_data2);
    canTransmit(canREG1, CANBOX_AMS_AMPS, tx_data3);

    return;
}

void AMS_readSPI() {
	uint16 spiBatteryRX[1] = { 0 };
	uint16 spiVehicleRX[1] = { 0 };

	spiReceiveData(spiREG3, &dataconfig00_t, 1, spiBatteryRX);
	spiReceiveData(spiREG1, &dataconfig00_t, 1, spiVehicleRX);

	currentVehicleVoltage = spiVehicleRX[0];
	currentBatteryVoltage = spiBatteryRX[0];

	return;
}

void AMS_readSCI() {
	WriteReg(0, 2, 0x02, 1, FRMWRT_ALL_NR); // send sync sample command
	//WaitRespFrame(AMS_DATA, 195, 0); // 39? bytes data (x5) + packet header (x5) + CRC (2bytes) (x5), 0ms timeout
	WaitRespFrame(AMS_DATA, 39*BMS_TOTALBOARDS, 0); // 39? bytes data (x5) + packet header (x5) + CRC (2bytes) (x5), 0ms timeout
	return;
}

void getBMSData(uint8 *buffer){
    uint16 i =0;
    for(i=0; i<164;i++){
        buffer[i] = AMS_DATA[i];
    }
    return;
}

void AMS_checkPreContactorState() {
	if(contactorOpen != 0 && positiveContactorState == 0){
		prechargeContactorState = 1;
	} else {
		prechargeContactorState = 0;
	}

	return;
}



void AMS_process() {
	// Read Inputs
	AMS_readADC(); // Current Sensing
	AMS_readGIO(); //
	AMS_readHET(); //
	AMS_readSPI(); // Voltage Sensing
	AMS_readSCI(); // BMS

	//AMS_parseBMSData();
	AMS_checkPreContactorState(); // Checks Precharge Contactor State

	// Check Fault State
	AMS_checkForFaults();

	// Process the Current State of the AMS
	//AMS_processState();

	// Write CAN Outputs
	AMS_canTX_Car();
//  	AMS_canTX_BMS();  // Disabled by default, BMS currently communicates through UART

	// Write Digital Outputs`1
	//AMS_writeGIO();
	//AMS_writeHET();



	timeoutCAR++;
	timeoutBMS++;

}




//void AMS_

/*
AMS_switchState(int AMS_STATE){
	switch(AMS_STATE)
	{
	case AMS_BOOT:
	{

		break;
	}
	case AMS_IDLE:
	{

		break;
	}
	case START_HV:
	{

		break;
	}
	case RUN_HV:
	{

		break;
	}
	case STOP_HV:
	{

		break;
	}
	case AMS_CHARGE: // IDK YET...
	{

		break;
	}
	case AMS_IDLE:
	{

		break;
	}
	default:
	{
		AMS_FAULT(); //IDK YET...
		break;
	}
	}

	}
}



void AMS_sendBMSWakeup(void){
	gioSetBit(gioPORTB, GIOB_BMS_WAKEUP, 1);
	return;
}


uint32 AMS_getBMSFaultState(void){
	return gioGetBit(gioPORTA, GIOA_BMS_FAULT);
}

uint32 AMS_getIMDFaultState(void){
	return gioGetBit(gioPORTA, GIOA_IMD_FAULT);
}

uint32 AMS_getCurrentShortFaultState(void){
	return gioGetBit(gioPORTA, GIOB_CURRENT_SHORT_FAULT);
}

*/
