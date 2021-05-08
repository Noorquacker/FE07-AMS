/*
 * AMS_common.h
 *
 *  Created on: Apr 23, 2021
 *      Author: Mikel
 */

#ifndef INCLUDE_AMS_COMMON_H_
#define INCLUDE_AMS_COMMON_H_

// BMS Constants (used in BMS.c & pl455.c)
#define BMS_TOTALBOARDS					5  		// number of BMS boards (MAX 16, MIN 1)
#define BMS_BAUDRATE 					250000  // default baud for BMS boards - DO NOT CHANGE w/o changing BMS functions
#define BMS_CELL_OVERVOLTAGE_THRESHOLD	4.2		// Overvolt Fault Threshold for Cells
#define BMS_CELL_UNDERVOLTAGE_THRESHOLD	3.0		// Undervolt Fault Threshold for Cells
#define BMS_AUX_OVERVOLTAGE_THRESHOLD	4.2		// Overvolt Fault Threshold for AUX Inputs (Temp Sensors)
#define BMS_AUX_UNDERVOLTAGE_THRESHOLD	3.0		// Undervolt Fault Threshold for AUX Inputs (Temp Sensors)


// GIO PORT A PIN DEFINITIONS
#define GIOA_BMS_FAULT 					0
#define GIOA_NEG_CONTACT_SENSE_FILTERED 1
#define GIOA_POS_CONTACT_SENSE_FILTERED 2
#define GIOA_IMD_FAULT 					5
#define GIOA_CONTACT_OPEN_INPUT			6
#define GIOA_CONTACT_CLOSED_INPUT 		7


// GIO PORT B PIN DEFINITIONS
// INPUTS
#define GIOB_5KW_INPUT 					1
#define GIOB_CURRENT_SHORT_FAULT 		2
// OUTPUTS
#define GIOB_AMS_FAULT_OUTPUT 			0
#define GIOB_BMS_WAKEUP 				3


// CAN PORT DEFINITIONS
#define CAR_CAN_PORT 					canREG1
#define BMS_CAN_PORT 					canREG2 // NOT CURRENTLY USED


// SPI PORT DEFINITIONS
#define VOLTAGE_MONITOR_SPI_PORT 		mibspiREG1


// HET PORT 1 PIN DEFINITIONS
// INPUTS
#define HET1_IMD_PWM_INPUT 				2 // PWM
#define HET1_SC_OUTSIDE_SENSE 			4
#define HET1_SC_OUTSIDE_SENSE_DUPLICATE 15 // DUPLICATE - UNUSED
// OUTPUTS
#define HET1_LED_INDICATOR_1 			12 // This is switched with Indicator 2 due to incorrect silk screen
#define HET1_LED_INDICATOR_2 			14 // This is switched with Indicator 1 due to incorrect silk screen
#define HET1_PRECHARGE_CONTACT_CTRL		16
#define HET1_NEGATIVE_CONTACT_CTRL		18
#define HET1_POSITIVE_CONTACT_CTRL 		20

// CAR CAN Message Boxes	    		NUMBER  TX/RX
#define CANBOX_AMS_STATUS       		1U      //TX
#define CANBOX_AMS_VOLTS        		2U      //TX
#define CANBOX_AMS_AMPS         		3U      //TX
#define CANBOX_BMS_DATA                 4U      //TX


#define FAULT_CELL_OVR_VOLT				1<<0
#define FAULT_CELL_UND_VOLT				1<<1
#define FAULT_CELL_OVR_TEMP				1<<2
#define FAULT_CELL_UND_TEMP				1<<3
#define FAULT_OVER_DCG_CURRENT			1<<4
#define FAULT_OVER_CHG_CURRENT			1<<5
#define FAULT_CONTACTOR					1<<6
#define FAULT_BMS						1<<7
#define FAULT_BMS_COMM					1<<8
#define FAULT_ISOLATION					1<<9

#endif /* INCLUDE_AMS_COMMON_H_ */
