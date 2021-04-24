/*
 * BMS.c
 *
 *  Created on: Apr 23, 2021
 *      Author: Mikel
 */

#include <stdbool.h>
#include "AMS_common.h"
#include "sci.h"
#include "pl455.h"

int UART_RX_RDY = 0;
int RTI_TIMEOUT = 0;

int BMS_Init(){
	// From bq76PL455A Example Code
	sciSetBaudrate(sciREG, BMS_BAUDRATE);
	WakePL455();
	CommClear();
	CommReset();

	int nSent, nRead, nTopFound = 0;
	int nDev_ID, nGrp_ID;
	uint8  bFrame[256];
	uint32  wTemp = 0;

	// Wake all devices
	// The wake tone will awaken any device that is already in shutdown and the pwrdown will shutdown any device
	// that is already awake. The least number of times to sequence wake and pwrdown will be half the number of
	// boards to cover the worst case combination of boards already awake or shutdown.
	for(nDev_ID = 0; nDev_ID < BMS_TOTALBOARDS>>1; nDev_ID++) {
		nSent = WriteReg(nDev_ID, 12, 0x40, 1, FRMWRT_ALL_NR);	// send out broadcast pwrdown command
		delayms(5); //~5ms
		WakePL455();
		delayms(5); //~5ms
	}

	// Mask Customer Checksum Fault bit
	nSent = WriteReg(0, 107, 0x8000, 2, FRMWRT_ALL_NR); // clear all fault summary flags

	// Clear all faults
	nSent = WriteReg(0, 82, 0xFFC0, 2, FRMWRT_ALL_NR);		// clear all fault summary flags
	nSent = WriteReg(0, 81, 0x38, 1, FRMWRT_ALL_NR); // clear fault flags in the system status register

	// Auto-address all boards (section 1.2.2)
	nSent = WriteReg(0, 14, 0x19, 1, FRMWRT_ALL_NR); // set auto-address mode on all boards
	nSent = WriteReg(0, 12, 0x08, 1, FRMWRT_ALL_NR); // enter auto address mode on all boards, the next write to this ID will be its address

	// Set addresses for all boards in daisy-chain (section 1.2.3)
	for (nDev_ID = 0; nDev_ID < BMS_TOTALBOARDS; nDev_ID++)
	{
		nSent = WriteReg(nDev_ID, 10, nDev_ID, 1, FRMWRT_ALL_NR); // send address to each board
	}

	// Enable all communication interfaces on all boards in the stack (section 1.2.1)
	nSent = WriteReg(0, 16, 0x10F8, 2, FRMWRT_ALL_NR);	// set communications baud rate and enable all interfaces on all boards in stack
    nRead = ReadReg(0, 10, &wTemp, 1, 0); // 0ms timeout

	for (nDev_ID = BMS_TOTALBOARDS - 1; nDev_ID >= 0; --nDev_ID)
	{
		// read device ID to see if there is a response
		nRead = ReadReg(nDev_ID, 10, &wTemp, 1, 0); // 0ms timeout

		if(nRead == 0) // if nothing is read then this board doesn't exist
			nTopFound = 0;
		else // a response was received
		{
			if(nTopFound == 0)
			{ // if the last board was not present but this one is, this is the top board
				if(nDev_ID == 0) // this is the only board
				{
					nSent = WriteReg(nDev_ID, 16, 0x1080, 2, FRMWRT_SGL_NR);	// enable only single-end comm port on board
				}
				else // this is the top board of a stack (section 1.2.5)
				{
					nSent = WriteReg(nDev_ID, 16, 0x1028, 2, FRMWRT_SGL_NR);	// enable only comm-low and fault-low for the top board
					nTopFound = 1;
				}
			}
			else // this is a middle or bottom board
			{
				if(nDev_ID == 0) // this is a bottom board of a stack (section 1.2.6)
				{
					nSent = WriteReg(nDev_ID, 16, 0x10D0, 2, FRMWRT_SGL_NR);	// enable comm-high, fault-high and single-end comm port on bottom board
				}
				else // this is a middle board
				{
					nSent = WriteReg(nDev_ID, 16, 0x1078, 2, FRMWRT_SGL_NR);	// enable comm-high, fault-high, comm-low and fault-low on all middle boards
				}
			}
		}
	}

	// Clear all faults (section 1.2.7)
	nSent = WriteReg(0, 82, 0xFFC0, 2, FRMWRT_ALL_NR); // clear all fault summary flags
	nSent = WriteReg(0, 81, 0x38, 1, FRMWRT_ALL_NR); // clear fault flags in the system status register

	delayms(10);

	// Configure AFE (section 2.2.1)

	nDev_ID = 0;
	nSent = WriteReg(nDev_ID, 60, 0x00, 1, FRMWRT_SGL_NR); // set 0 mux delay
	nSent = WriteReg(nDev_ID, 61, 0x00, 1, FRMWRT_SGL_NR); // set 0 initial delay

	// Configure voltage and internal sample period (section 2.2.2)
	nDev_ID = 0;
	nSent = WriteReg(nDev_ID, 62, 0xCC, 1, FRMWRT_SGL_NR); // set 99.92us ADC sampling period

	// Configure the oversampling rate (section 2.2.3)
	nDev_ID = 0;
	nSent = WriteReg(nDev_ID, 7, 0x00, 1, FRMWRT_SGL_NR); // set no oversampling period


	// Select identical number of cells and channels on all modules simultaneously (section 2.2.5.2)
	nSent = WriteReg(0, 13, 0x10, 1, FRMWRT_ALL_NR); // set number of cells to 16
	nSent = WriteReg(0, 3, 0xFFFF00C0, 4, FRMWRT_ALL_NR); // select all cell, NO AUX channels, and internal digital die and internal analog die temperatures

	// Set cell over-voltage and cell under-voltage thresholds on all boards simultaneously (section 2.2.6.2)
	nSent = WriteReg(0, 144, 0xD70A, 2, FRMWRT_ALL_NR); // set OV threshold = 4.2000V (Calculation: ceil(13107*Volt) = ceil(13107*4.2) = 55050 = 0xD70A)
	nSent = WriteReg(0, 142, 0x9999, 2, FRMWRT_ALL_NR); // set UV threshold = 3.0000V (Calculation: ceil(13107*Volt) = ceil(13107*3.0) = 39231 = 0x9999)

	// Send broadcast request to all boards to sample and send results (section 3.2)
	nSent = WriteReg(0, 2, 0x02, 1, FRMWRT_ALL_NR); // send sync sample command
	nSent = WaitRespFrame(bFrame, /*195*/39, 2); // 39? bytes data (x5) + packet header (x5) + CRC (2bytes) (x5), 0ms timeout
										   // For calculations, see Page 27 of https://www.ti.com/lit/ds/slusc51c/slusc51c.pdf?ts=1619217953406


	return nSent;


}
