/*
 * FE_AMS.c
 *
 *  Created on: Apr 15, 2021
 *      Author: Mikel
 */

// INCLUDES
#include "het.h"
#include "gio.h"
#include "mibspi.h"
#include "spi.h"


// GIO PORT A PIN DEFINITIONS
#define GIOA_BMS_FAULT 0
#define GIOA_NEG_CONTACT_SENSE_FILTERED 1
#define GIOA_POS_CONTACT_SENSE_FILTERED 2
#define GIOA_IMD_FAULT 5
#define GIOA_CONTACT_OPEN_INPUT 6
#define GIOA_CONTACT_CLOSED_INPUT 7


// GIO PORT B PIN DEFINITIONS
// INPUTS
#define GIOB_5KW_INPUT 1
#define GIOB_CURRENT_SHORT_FAULT 2
// OUTPUTS
#define GIOB_AMS_FAULT_OUTPUT 0
#define GIOB_BMS_WAKEUP 3


// CAN PORT DEFINITIONS
#define CAR_CAN_PORT canREG1
#define BMS_CAN_PORT canREG2 // NOT CURRENTLY USED


// SPI PORT DEFINITIONS
#define VOLTAGE_MONITOR_SPI_PORT mibspiREG1


// HET PORT 1 PIN DEFINITIONS
// INPUTS
#define HET1_IMD_PWM_INPUT 2 // PWM
#define HET1_SC_OUTSIDE_SENSE 4
#define HET1_SC_OUTSIDE_SENSE_DUPLICATE 15 // DUPLICATE - UNUSED
// OUTPUTS
#define HET1_LED_INDICATOR_1 12 // This is switched with Indicator 2 due to incorrect silk screen
#define HET1_LED_INDICATOR_2 14 // This is switched with Indicator 1 due to incorrect silk screen
#define HET1_PRECHARGE_CONTACT_CTRL 16
#define HET1_NEGATIVE_CONTACT_CTRL 18
#define HET1_POSITIVE_CONTACT_CTRL 20


//uint8_t buffer[64];

//AMS_getVoltageData(uint16_t &buffer){
//    mibspiGetData(mibspiREG1, 0, buffer);
// //       uint32 mibspiGetData(mibspiBASE_t *mibspi, uint32 group, uint16 * data)
//
//}

uint16 currentVehicleVoltage = 0;
uint16 currentBatteryVoltage = 0;

uint16 RX_Data_M[16] = { 0 };
uint16 RX_Data_S[16]  = { 0 };

spiDAT1_t dataconfig00_t;

void dataConfigFnct(){
    dataconfig00_t.CS_HOLD = TRUE;
    dataconfig00_t.WDEL    = TRUE;
    dataconfig00_t.DFSEL   = SPI_FMT_0;
    dataconfig00_t.CSNR    = 0xFE;
}

uint16 getDelta() {
    uint16 x = currentBatteryVoltage - currentVehicleVoltage;
    x = x*(-1);
    return x;
}

void AMS_start_HV(void){
    dataConfigFnct();
    uint16 x = getDelta();
    // Engage Negative Contactor
    gioSetBit(hetPORT1,HET1_NEGATIVE_CONTACT_CTRL,1);
    // Wait Until Negative Contactor Close is Sensed
    while(!gioGetBit(gioPORTA, GIOA_NEG_CONTACT_SENSE_FILTERED));
    // Engage Precharge Contactor
    gioSetBit(hetPORT1,HET1_PRECHARGE_CONTACT_CTRL,1);
    // Wait until Delta between Vehicle Voltage and Battery Voltage is greater than 5
    while(x<5){
        x = getDelta();
        spiReceiveData(spiREG1, &dataconfig00_t, 1, RX_Data_M);
        spiReceiveData(spiREG3, &dataconfig00_t, 1, RX_Data_S);
        setCurrentBatteryVoltage(RX_Data_S[0]);
        setCurrentVehicleVoltage(RX_Data_M[0]);
    }
    // Engage Positive Contactor
    gioSetBit(hetPORT1,HET1_POSITIVE_CONTACT_CTRL,1);
    // Disengage Precharge Contactor
    gioSetBit(hetPORT1,HET1_PRECHARGE_CONTACT_CTRL,0);

    return;
}


uint8 AMS_checkForFaults(){
	uint8 x = 0xFF;
	x |= ~((gioGetBit(gioPORTA, GIOA_BMS_FAULT))<<0);
	x |= ~((gioGetBit(gioPORTA, GIOA_IMD_FAULT))<<1);
	x |= ~((gioGetBit(gioPORTB, GIOB_CURRENT_SHORT_FAULT))<<2);
	x |= ~((gioGetBit(gioPORTB, GIOB_CURRENT_SHORT_FAULT))<<3);
	x |= ~((gioGetBit(hetPORT1, HET1_SC_OUTSIDE_SENSE))<<4);

	return x;
}



void setCurrentVehicleVoltage(uint16 x){
    currentVehicleVoltage = x;
    return;
}

void setCurrentBatteryVoltage(uint16 x){
    currentBatteryVoltage = x;
    return;
}


void AMS_process() {
	// Read Inputs
	AMS_readADC();
	AMS_readGIO();
	AMS_readHET();

	// Check Fault State
	AMS_checkForFaults();

	// Process the Current State of the AMS
	AMS_processState();

	// Write CAN Outputs
	AMS_canTX_Car();
//	AMS_canTX_BMS();  // Disabled by default, BMS currently communicates through UART

	// Write Digital Outputs
	AMS_writeGIO();
	AMS_writeHET();



	timeoutCAR++;
	timeoutBMS++;




}


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
