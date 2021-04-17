/*
 * FE_AMS.c
 *
 *  Created on: Apr 15, 2021
 *      Author: Mikel
 */

// INCLUDES
#include "gio.h"


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

