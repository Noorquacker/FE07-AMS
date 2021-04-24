/* 
* Copyright (C) 2009-2016 Texas Instruments Incorporated - www.ti.com 
* 
* 
*  Redistribution and use in source and binary forms, with or without 
*  modification, are permitted provided that the following conditions 
*  are met:
*
*    Redistributions of source code must retain the above copyright 
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the 
*    documentation and/or other materials provided with the   
*    distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/


/**
 *  @file       usb_serial_structs.c
 *
 *  @brief      Data structures defining this CDC USB device.
 * 
 */

#include "sys_common.h"
#include "usb.h"
#include "usblib.h"
#include "usb-ids.h"
#include "usbcdc.h"
#include "usbdcdc.h"
#include "usb_serial_structs.h"

/** ***************************************************************************
 *
 * The languages supported by this device.
 *
 *****************************************************************************/
const uint8 g_pLangDescriptor[] =
{
    4,
    USB_DTYPE_STRING,
    USBShort(USB_LANG_EN_UK)
};

/** ***************************************************************************
 *
 * The manufacturer string.
 *
 *****************************************************************************/
const uint8 g_pManufacturerString[] =
{
    (1 + 1) * 2,
    USB_DTYPE_STRING,
    0
};

/** ***************************************************************************
 *
 * The product string.
 *
 *****************************************************************************/
const uint8 g_pProductString[] =
{
    2 + (1 * 2),
    USB_DTYPE_STRING,
    0
};

/** ***************************************************************************
 *
 * The serial number string.
 *
 *****************************************************************************/
const uint8 g_pSerialNumberString[] =
{
    2 + (1 * 2),
    USB_DTYPE_STRING,
    0

};

/** ***************************************************************************
 *
 * The control interface description string.
 *
 *****************************************************************************/
const uint8 g_pControlInterfaceString[] =
{
    2 + (1 * 2),
    USB_DTYPE_STRING,
    0

};

/** ***************************************************************************
 *
 * The configuration description string.
 *
 *****************************************************************************/
const uint8 g_pConfigString[] =
{
    2 + (1 * 2),
    USB_DTYPE_STRING,
    0
};

/** ***************************************************************************
 *
 * The descriptor string table.
 *
 *****************************************************************************/
const uint8 * const g_pStringDescriptors[] =
{
    g_pLangDescriptor,
    g_pManufacturerString,
    g_pProductString,
    g_pSerialNumberString,
    g_pControlInterfaceString,
    g_pConfigString
};

#define NUM_STRING_DESCRIPTORS (sizeof(g_pStringDescriptors) /                \
                                sizeof(uint8 *))

/** ***************************************************************************
 *
 * The CDC device initialization and customization structures. In this case,
 * we are using USBBuffers between the CDC device class driver and the
 * application code. The function pointers and callback data values are set
 * to insert a buffer in each of the data channels, transmit and receive.
 *
 * With the buffer in place, the CDC channel callback is set to the relevant
 * channel function and the callback data is set to point to the channel
 * instance data. The buffer, in turn, has its callback set to the application
 * function and the callback data set to our CDC instance structure.
 *
 *****************************************************************************/
tCDCSerInstance g_sCDCInstance;

extern const tUSBBuffer g_sTxBuffer;
extern const tUSBBuffer g_sRxBuffer;

const tUSBDCDCDevice g_sCDCDevice =
{
    USB_VID_TI,
    USB_PID_SERIAL,
    0,
    USB_CONF_ATTR_SELF_PWR,
    ControlHandler,
    (void *)&g_sCDCDevice,
    USBBufferEventCallback,
    (void *)&g_sRxBuffer,
    USBBufferEventCallback,
    (void *)&g_sTxBuffer,
    g_pStringDescriptors,
    NUM_STRING_DESCRIPTORS,
    &g_sCDCInstance
};

/** ***************************************************************************
 *
 * Receive buffer (from the USB perspective).
 *
 *****************************************************************************/
uint8 g_pcUSBRxBuffer[UART_BUFFER_SIZE];
uint8 g_pucRxBufferWorkspace[USB_BUFFER_WORKSPACE_SIZE];
const tUSBBuffer g_sRxBuffer =
{
    FALSE,                          /* This is a receive buffer. */
    RxHandler,                      /* pfnCallback */
    (void *)&g_sCDCDevice,          /* Callback data is our device pointer. */
    USBDCDCPacketRead,              /* pfnTransfer */
    USBDCDCRxPacketAvailable,       /* pfnAvailable */
    (void *)&g_sCDCDevice,          /* pvHandle */
    g_pcUSBRxBuffer,                /* pcBuffer */
    UART_BUFFER_SIZE,               /* ulBufferSize */
    g_pucRxBufferWorkspace          /* pvWorkspace */
};

/** ***************************************************************************
 *
 * Transmit buffer (from the USB perspective).
 *
 *****************************************************************************/
uint8 g_pcUSBTxBuffer[UART_BUFFER_SIZE];
uint8 g_pucTxBufferWorkspace[USB_BUFFER_WORKSPACE_SIZE];
const tUSBBuffer g_sTxBuffer =
{
    TRUE,                           /* This is a transmit buffer. */
    TxHandler,                      /* pfnCallback */
    (void *)&g_sCDCDevice,          /* Callback data is our device pointer. */
    USBDCDCPacketWrite,             /* pfnTransfer */
    USBDCDCTxPacketAvailable,       /* pfnAvailable */
    (void *)&g_sCDCDevice,          /* pvHandle */
    g_pcUSBTxBuffer,                /* pcBuffer */
    UART_BUFFER_SIZE,               /* ulBufferSize */
    g_pucTxBufferWorkspace          /* pvWorkspace */
};
