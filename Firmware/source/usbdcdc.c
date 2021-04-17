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
 *  @file       usbdcdc.c
 *
 *  @brief      USB CDC ACM (serial) device class driver.
 * 
 */

#include "sys_common.h"
#include "hw_usb.h"

#include "usb.h"
#include "usblib.h"
#include "usbdevice.h"
#include "usbcdc.h"
#include "usbdcdc.h"



/** ***************************************************************************
 *
 *  \ingroup cdc_device_class_api
 *  @{
 *
 *****************************************************************************/

/******************************************************************************
 *
 * Some assumptions and deviations from the CDC specification
 * ----------------------------------------------------------
 *
 * 1.  Although the CDC specification indicates that the following requests
 * should be supported by ACM CDC devices, these don't seem relevant to a
 * virtual COM port implementation and are never seen when connecting to a
 * Windows host and running either Hyperterminal or TeraTerm.  As a result,
 * this implementation does not support them and stalls endpoint 0 if they are
 * received.
 *          - SEND_ENCAPSULATED_COMMAND
 *          - GET_ENCAPSULATED_RESPONSE
 *          - SET_COMM_FEATURE
 *          - GET_COMM_FEATURE
 *          - CLEAR_COMM_FEATURE
 *
 * 2.  The CDC specification is very clear on the fact that an ACM device
 * should offer two interfaces - a control interface offering an interrupt IN
 * endpoint and a data interface offering bulk IN and OUT endpoints.  Using
 * this descriptor configuration, however, Windows insists on enumerating the
 * device as two separate entities resulting in two virtual COM ports or one
 * COM port and an Unknown Device (depending upon INF contents) appearing
 * in Device Manager.  This implementation, derived by experimentation and
 * examination of other virtual COM and CDC solutions, uses only a single
 * interface combining all three endpoints.  This appears to satisfy
 * Windows2000, XP and Vista and operates as intended using the Hyperterminal
 * and TeraTerm terminal emulators.  Your mileage may vary with other
 * (untested) operating systems!
 *
 *****************************************************************************/

/** ***************************************************************************
 *
 * The subset of endpoint status flags that we consider to be reception
 * errors.  These are passed to the client via USB_EVENT_ERROR if seen.
 *
 *****************************************************************************/
#define USB_RX_ERROR_FLAGS      (USBERR_DEV_RX_DATA_ERROR | \
                                 USBERR_DEV_RX_OVERRUN |    \
                                 USBERR_DEV_RX_FIFO_FULL)

/** ***************************************************************************
 *
 * Size of the buffer to hold request-specific data read from the host.  This
 * must be sized to accommodate the largest request structure that we intend
 * processing.
 *
 *****************************************************************************/
#define MAX_REQUEST_DATA_SIZE sizeof(tLineCoding)

/** ***************************************************************************
 *
 * This macro is used to diable the bit band operartion. Need to undefine this 
 * macro to use the bit band operation.
 *
 *****************************************************************************/
#define DISABLE_BIT_BAND

/******************************************************************************
 *
 * Flags that may appear in usDeferredOpFlags to indicate some operation that
 * has been requested but could not be processed at the time it was received.
 *
 *****************************************************************************/
#define CDC_DO_SERIAL_STATE_CHANGE 0U
#define CDC_DO_SEND_BREAK          1U
#define CDC_DO_CLEAR_BREAK         2U
#define CDC_DO_LINE_CODING_CHANGE  3U
#define CDC_DO_LINE_STATE_CHANGE   4U
#define CDC_DO_PACKET_RX           5U

/** ***************************************************************************
 *
 * The subset of deferred operations which result in the receive channel
 * being blocked.
 *
 *****************************************************************************/
 /* 
  * Commented macro RX_BLOCK_OPS - it contains bits corresponding to
  * CDC_DO_SEND_BREAK, CDC_DO_LINE_CODING_CHANGE and CDC_DO_LINE_STATE_CHANGE.
  */

/** ***************************************************************************
 *
 * Endpoints to use for each of the required endpoints in the driver.
 *
 *****************************************************************************/
#define CONTROL_ENDPOINT        (USB_EP_1)
#define DATA_IN_ENDPOINT        (USB_EP_2)
#define DATA_OUT_ENDPOINT       (USB_EP_1)

/** ***************************************************************************
 *
 * The following are the USB interface numbers for the CDC serial device.
 *
 *****************************************************************************/
#define SERIAL_INTERFACE_CONTROL    0u
#define SERIAL_INTERFACE_DATA       1u

/** ***************************************************************************
 *
 * Maximum packet size for the bulk endpoints used for serial data
 * transmission and reception and the associated FIFO sizes to set aside
 * for each endpoint.
 *
 *****************************************************************************/
#define DATA_IN_EP_FIFO_SIZE    (USB_FIFO_SZ_64)
#define DATA_OUT_EP_FIFO_SIZE   (USB_FIFO_SZ_64)
#define CTL_IN_EP_FIFO_SIZE     (USB_FIFO_SZ_16)

#define DATA_IN_EP_MAX_SIZE     USB_FIFO_SZ_TO_BYTES(DATA_IN_EP_FIFO_SIZE)
#define DATA_OUT_EP_MAX_SIZE    USB_FIFO_SZ_TO_BYTES(DATA_IN_EP_FIFO_SIZE)
#define CTL_IN_EP_MAX_SIZE      USB_FIFO_SZ_TO_BYTES(CTL_IN_EP_FIFO_SIZE)

/** ***************************************************************************
 *
 * The collection of serial state flags indicating character errors.
 *
 *****************************************************************************/
#define USB_CDC_SERIAL_ERRORS (USB_CDC_SERIAL_STATE_OVERRUN |                 \
                               USB_CDC_SERIAL_STATE_PARITY |                  \
                               USB_CDC_SERIAL_STATE_FRAMING)

/** ***************************************************************************
 *
 * Device Descriptor.  This is stored in RAM to allow several fields to be
 * changed at runtime based on the client's requirements.
 *
 *****************************************************************************/
uint8 g_pCDCSerDeviceDescriptor[18] =
{
    18,                     /* Size of this structure. */
    USB_DTYPE_DEVICE,       /* Type of this structure. */
    0x10,                   /* USB version 1.1 - Byte 0 of 0x0110
                               (if we say 2.0, hosts assume high-speed -
                               see USB 2.0 spec 9.2.6.6) */
    0x01,                   /* USB version 1.1 - Byte 1 of 0x0110 */  
    USB_CLASS_CDC,          /* USB Device Class (spec 5.1.1) */
    0,                      /* USB Device Sub-class (spec 5.1.1) */
    USB_CDC_PROTOCOL_NONE,  /* USB Device protocol (spec 5.1.1) */
    64,                     /* Maximum packet size for default pipe. */
    0x00,                   /* Vendor ID - Byte 0 (filled in during 
                               USBDCDCInit). */
    0x00,                   /* Vendor ID - Byte 1 */
    0x00,                   /* Product ID - Byte 0 (filled in during 
                               USBDCDCInit). */
    0x00,                   /* Product ID - Byte 1 */
    0x00,                   /* Device Version BCD - Byte 0 of 0x100 */
    0x01,                   /* Device Version BCD - Byte 1 of 0x100 */
    1,                      /* Manufacturer string identifier. */
    2,                      /* Product string identifier. */
    3,                      /* Product serial number. */
    1                       /* Number of configurations. */
};

#define USB_CDC_SERV_DESC_SIZE  0x09

/** ***************************************************************************
 *
 * CDC Serial configuration descriptor.
 *
 * It is vital that the configuration descriptor bConfigurationValue field
 * (byte 6) is 1 for the first configuration and increments by 1 for each
 * additional configuration defined here.  This relationship is assumed in the
 * device stack for simplicity even though the USB 2.0 specification imposes
 * no such restriction on the bConfigurationValue values.
 *
 * Note that this structure is deliberately located in RAM since we need to
 * be able to patch some values in it based on client requirements.
 *
 *****************************************************************************/
uint8 g_pCDCSerDescriptor[USB_CDC_SERV_DESC_SIZE] =
{
    /*
     * Configuration descriptor header.
     */
    9,                          /* Size of the configuration descriptor. */
    USB_DTYPE_CONFIGURATION,    /* Type of this descriptor. */
    USB_CDC_SERV_DESC_SIZE,     /* The total size of this full structure, this
                                 * will be patched so it is just set to the
                                 * size of this structure. 
                                 * Byte 0 of USB_CDC_SERV_DESC_SIZE */
    0,                          /* Byte 1 of USB_CDC_SERV_DESC_SIZE */
    2,                          /* The number of interfaces in this
                                 * configuration. */
    1,                          /* The unique value for this configuration. */
    5,                          /* The string identifier that describes this
                                 * configuration. */
    USB_CONF_ATTR_SELF_PWR,     /* Bus Powered, Self Powered, remote wake up. */
    250,                        /* The maximum power in 2mA increments. */
};

const tConfigSection g_sCDCSerConfigSection =
{
    sizeof(g_pCDCSerDescriptor),
    g_pCDCSerDescriptor
};

#define USB_IF_ASSOC_DESC_SIZE  0x08

/** ***************************************************************************
 *
 * This is the Interface Association Descriptor for the serial device used in
 * composite devices.
 *
 *****************************************************************************/
uint8 g_pIADSerDescriptor[USB_IF_ASSOC_DESC_SIZE] =
{

    8,                          /* Size of the interface descriptor. */
    USB_DTYPE_INTERFACE_ASC,    /* Interface Association Type. */
    0x0,                        /* Default starting interface is 0. */
    0x2,                        /* Number of interfaces in this association. */
    USB_CLASS_CDC,              /* The device class for this association. */
    USB_CDC_SUBCLASS_ABSTRACT_MODEL, /* The device subclass for this
                                      * association. */
    USB_CDC_PROTOCOL_V25TER,    /* The protocol for this association. */
    0                           /* The string index for this association. */
};

const tConfigSection g_sIADSerConfigSection =
{
    sizeof(g_pIADSerDescriptor),
    g_pIADSerDescriptor
};

#define USB_CDC_CTRL_IF_DESC_SIZE  35U

/** ***************************************************************************
 *
 * This is the control interface for the serial device.
 *
 *****************************************************************************/
const uint8 g_pCDCSerCommInterface[USB_CDC_CTRL_IF_DESC_SIZE] =
{
    /*
     * Communication Class Interface Descriptor.
     */
    9,                          /* Size of the interface descriptor. */
    USB_DTYPE_INTERFACE,        /* Type of this descriptor. */
    SERIAL_INTERFACE_CONTROL,   /* The index for this interface. */
    0,                          /* The alternate setting for this interface. */
    1,                          /* The number of endpoints used by this
                                 * interface. */
    USB_CLASS_CDC,              /* The interface class constant defined by
                                 * USB-IF (spec 5.1.3). */
    USB_CDC_SUBCLASS_ABSTRACT_MODEL,    /* The interface sub-class constant
                                         * defined by USB-IF (spec 5.1.3). */
    USB_CDC_PROTOCOL_V25TER,    /* The interface protocol for the sub-class
                                 * specified above. */
    4,                          /* The string index for this interface. */

    /*
     * Communication Class Interface Functional Descriptor - Header
     */
    5,                          /* Size of the functional descriptor. */
    USB_CDC_CS_INTERFACE,       /* CDC interface descriptor */
    USB_CDC_FD_SUBTYPE_HEADER,  /* Header functional descriptor */
    0x10,                       /* Complies with CDC version 1.1 
                                   Byte 0 of 0x0110 */
    0x01,                        /* Byte 1 of 0x0110 */

    /*
     * Communication Class Interface Functional Descriptor - ACM
     */
    4,                          /* Size of the functional descriptor. */
    USB_CDC_CS_INTERFACE,       /* CDC interface descriptor */
    USB_CDC_FD_SUBTYPE_ABSTRACT_CTL_MGMT,
    USB_CDC_ACM_SUPPORTS_LINE_PARAMS | USB_CDC_ACM_SUPPORTS_SEND_BREAK,

    /*
     * Communication Class Interface Functional Descriptor - Unions
     */
    5,                          /* Size of the functional descriptor. */
    USB_CDC_CS_INTERFACE,       /* CDC interface descriptor */
    USB_CDC_FD_SUBTYPE_UNION,
    SERIAL_INTERFACE_CONTROL,
    SERIAL_INTERFACE_DATA,      /* Data interface number */

    /*
     * Communication Class Interface Functional Descriptor - Call Management
     */
    5,                          /* Size of the functional descriptor. */
    USB_CDC_CS_INTERFACE,       /* CDC interface descriptor */
    USB_CDC_FD_SUBTYPE_CALL_MGMT,
    USB_CDC_CALL_MGMT_HANDLED,
    SERIAL_INTERFACE_DATA,      /* Data interface number */

    /*
     * Endpoint Descriptor (interrupt, IN)
     */
    7,                              /* The size of the endpoint descriptor. */
    USB_DTYPE_ENDPOINT,             /* Descriptor type is an endpoint. */
    USB_EP_DESC_IN | USB_EP_TO_INDEX(CONTROL_ENDPOINT),
    USB_EP_ATTR_INT,                /* Endpoint is an interrupt endpoint. */
    /*SAFETYMCUSW 185 S <INSPECTED> "Reason -  LDRA tool issue."*/
    /*SAFETYMCUSW 185 S <INSPECTED> "Reason -  LDRA tool issue."*/
    /*SAFETYMCUSW 93 S <INSPECTED> "Reason -  LDRA tool issue."*/
    /*SAFETYMCUSW 93 S <INSPECTED> "Reason -  LDRA tool issue."*/
    USBShort(CTL_IN_EP_MAX_SIZE),   /* The maximum packet size. */
    10                             /* The polling interval for this endpoint. */
};

const tConfigSection g_sCDCSerCommInterfaceSection =
{
    sizeof(g_pCDCSerCommInterface),
    g_pCDCSerCommInterface
};

#define USB_CDC_DATA_IF_DESC_SIZE  23U

/** ***************************************************************************
 *
 * This is the Data interface for the serial device.
 *
 *****************************************************************************/
const uint8 g_pCDCSerDataInterface[USB_CDC_DATA_IF_DESC_SIZE] =
{
    /*
     * Communication Class Data Interface Descriptor.
     */
    9,                          /* Size of the interface descriptor. */
    USB_DTYPE_INTERFACE,        /* Type of this descriptor. */
    SERIAL_INTERFACE_DATA,      /* The index for this interface. */
    0,                          /* The alternate setting for this interface. */
    2,                          /* The number of endpoints used by this
                                 * interface. */
    USB_CLASS_CDC_DATA,         /* The interface class constant defined by
                                 * USB-IF (spec 5.1.3). */
    0,                          /* The interface sub-class constant
                                 * defined by USB-IF (spec 5.1.3). */
    USB_CDC_PROTOCOL_NONE,      /* The interface protocol for the sub-class
                                 * specified above. */
    0,                          /* The string index for this interface. */

    /*
     * Endpoint Descriptor
     */
    7,                              /* The size of the endpoint descriptor. */
    USB_DTYPE_ENDPOINT,             /* Descriptor type is an endpoint. */
    USB_EP_DESC_IN | USB_EP_TO_INDEX(DATA_IN_ENDPOINT),
    USB_EP_ATTR_BULK,               /* Endpoint is a bulk endpoint. */
    /*SAFETYMCUSW 185 S <INSPECTED> "Reason -  LDRA tool issue."*/
    /*SAFETYMCUSW 185 S <INSPECTED> "Reason -  LDRA tool issue."*/
    /*SAFETYMCUSW 93 S <INSPECTED> "Reason -  LDRA tool issue."*/
    /*SAFETYMCUSW 93 S <INSPECTED> "Reason -  LDRA tool issue."*/
    USBShort(DATA_IN_EP_MAX_SIZE),  /* The maximum packet size. */
    0,                              /* The polling interval for this endpoint. */

    /*
     * Endpoint Descriptor
     */
    7,                              /* The size of the endpoint descriptor. */
    USB_DTYPE_ENDPOINT,             /* Descriptor type is an endpoint. */
    USB_EP_DESC_OUT | USB_EP_TO_INDEX(DATA_OUT_ENDPOINT),
    USB_EP_ATTR_BULK,               /* Endpoint is a bulk endpoint. */
    /*SAFETYMCUSW 185 S <INSPECTED> "Reason -  LDRA tool issue."*/
    /*SAFETYMCUSW 185 S <INSPECTED> "Reason -  LDRA tool issue."*/
    /*SAFETYMCUSW 93 S <INSPECTED> "Reason -  LDRA tool issue."*/
    /*SAFETYMCUSW 93 S <INSPECTED> "Reason -  LDRA tool issue."*/
    USBShort(DATA_OUT_EP_MAX_SIZE), /* The maximum packet size. */
    0,                              /* The polling interval for this endpoint. */
};

const tConfigSection g_sCDCSerDataInterfaceSection =
{
    sizeof(g_pCDCSerDataInterface),
    g_pCDCSerDataInterface
};

/** ***************************************************************************
 *
 * This array lists all the sections that must be concatenated to make a
 * single, complete CDC ACM configuration descriptor.
 *
 *****************************************************************************/
const tConfigSection * g_psCDCSerSections[3] =
{
    &g_sCDCSerConfigSection,
    &g_sCDCSerCommInterfaceSection,
    &g_sCDCSerDataInterfaceSection,
};

#define NUM_CDCSER_SECTIONS (sizeof(g_psCDCSerSections) /                     \
                             sizeof(tConfigSection *))

/** ***************************************************************************
 *
 * The header for the single configuration.  This is the root of the data
 * structure that defines all the bits and pieces that are pulled together to
 * generate the configuration descriptor.
 *
 *****************************************************************************/
const tConfigHeader g_sCDCSerConfigHeader =
{
    /*SAFETYMCUSW 185 S <INSPECTED> "Reason -  LDRA tool issue."*/
    /*SAFETYMCUSW 185 S <INSPECTED> "Reason -  LDRA tool issue."*/
    NUM_CDCSER_SECTIONS,
    g_psCDCSerSections
};

/** ***************************************************************************
 *
 * This array lists all the sections that must be concatenated to make a
 * single, complete CDC ACM configuration descriptor used in composite devices.
 * The only addition is the g_sIADSerConfigSection.
 *
 *****************************************************************************/
const tConfigSection * g_psCDCCompSerSections[4] =
{
    &g_sCDCSerConfigSection,
    &g_sIADSerConfigSection,
    &g_sCDCSerCommInterfaceSection,
    &g_sCDCSerDataInterfaceSection,
};

#define NUM_COMP_CDCSER_SECTIONS (sizeof(g_psCDCCompSerSections) /  \
                                  sizeof(tConfigSection *))

/** ***************************************************************************
 *
 * The header for the composite configuration.  This is the root of the data
 * structure that defines all the bits and pieces that are pulled together to
 * generate the configuration descriptor.
 *
 *****************************************************************************/
const tConfigHeader g_sCDCCompSerConfigHeader =
{
    /*SAFETYMCUSW 185 S <INSPECTED> "Reason -  LDRA tool issue."*/
    /*SAFETYMCUSW 185 S <INSPECTED> "Reason -  LDRA tool issue."*/
    NUM_COMP_CDCSER_SECTIONS,
    g_psCDCCompSerSections
};

/** ***************************************************************************
 *
 * Configuration Descriptor for the CDC serial class device.
 *
 *****************************************************************************/
const tConfigHeader * const g_pCDCSerConfigDescriptors[1] =
{
    &g_sCDCSerConfigHeader
};

/** ***************************************************************************
 *
 * Configuration Descriptor for the CDC serial class device used in a composite
 * device.
 *
 *****************************************************************************/
const tConfigHeader * const g_pCDCCompSerConfigDescriptors[1] =
{
    &g_sCDCCompSerConfigHeader
};

/** ***************************************************************************
 *
 * Inline functions to convert between USB controller base address and an 
 * index.  These are currently trivial but are included to allow for the 
 * possibility of supporting more than one controller in the future.
 *
 *****************************************************************************/
static inline uint32_t UsbBaseToIndex (tCDCSerInstance * inst);
static inline tCDCSerInstance * UsbIndexToBase (uint32_t index);

/*SAFETYMCUSW 340 S MR:19.7 <INSPECTED> "Reason -  Acceptable deviation."*/
#define USB_BASE_TO_INDEX(BaseAddr) (0)

/*SAFETYMCUSW 340 S MR:19.7 <INSPECTED> "Reason -  Acceptable deviation."*/
#define USB_INDEX_TO_BASE(Index) (USB0_BASE)
/******************************************************************************
 *
 * Forward references for device handler callbacks
 *
 *****************************************************************************/
static void HandleRequests(void * pvInstance, tUSBRequest * pUSBRequest);
static void HandleConfigChange(void * pvInstance, uint32 ulInfo);
static void HandleEP0Data(void * pvInstance, uint32 ulDataSize);
static void HandleDisconnect(void * pvInstance);
static void HandleEndpoints(void * pvInstance, uint32 ulStatus);
static void HandleSuspend(void * pvInstance);
static void HandleResume(void * pvInstance);
static void HandleDevice(void * pvInstance, uint32 ulRequest,
                         void * pvRequestData);

extern uint32
	USBEndpointStatus(uint32 ulBase, uint32 ulEndpoint);
extern void
	USBDevEndpointStatusClear(uint32 ulBase, uint32 ulEndpoint,
                           uint32 ulFlags);

/** ***************************************************************************
 *
 * The device information structure for the USB serial device.
 *
 *****************************************************************************/
tDeviceInfo g_sCDCSerDeviceInfo =
{
    /*
     * Device event handler callbacks.
     */
    {
        /*
         * GetDescriptor
         */
        0,

        /*
         * RequestHandler
         */
        HandleRequests,

        /*
         * InterfaceChange
         */
        0,

        /*
         * ConfigChange
         */
        /*SAFETYMCUSW 576 S MR: 11.1 <INSPECTED> "Reason -  LDRA tool issue."*/
        HandleConfigChange,

        /*
         * DataReceived
         */
        /*SAFETYMCUSW 576 S MR: 11.1 <INSPECTED> "Reason -  LDRA tool issue."*/
        HandleEP0Data,

        /*
         * DataSentCallback
         */
        0,

        /*
         * ResetHandler
         */
        0,

        /*
         * SuspendHandler
         */
        /*SAFETYMCUSW 576 S MR: 11.1 <INSPECTED> "Reason -  LDRA tool issue."*/
        HandleSuspend,

        /*
         * ResumeHandler
         */
        /*SAFETYMCUSW 576 S MR: 11.1 <INSPECTED> "Reason -  LDRA tool issue."*/
        HandleResume,

        /*
         * DisconnectHandler
         */
        /*SAFETYMCUSW 576 S MR: 11.1 <INSPECTED> "Reason -  LDRA tool issue."*/
        HandleDisconnect,

        /*
         * EndpointHandler
         */
        /*SAFETYMCUSW 576 S MR: 11.1 <INSPECTED> "Reason -  LDRA tool issue."*/
        HandleEndpoints,

        /*
         * Device handler.
         */
        /*SAFETYMCUSW 576 S MR: 11.1 <INSPECTED> "Reason -  LDRA tool issue."*/
        HandleDevice
    },

    /*
     * The common device descriptor.
     */
    g_pCDCSerDeviceDescriptor,

    /*
     * Default to no interrupt endpoint.
     */
    g_pCDCCompSerConfigDescriptors,

    /*
     * String descriptors will be passed in.
     */
    0,
    0,

    /*
     * Use the default USB FIFO configuration.
     */
    &g_sUSBDefaultFIFOConfig,

    /*
     * Zero out the instance pointer by default.
     */
    0
};

static inline uint32_t UsbBaseToIndex (tCDCSerInstance * inst)
{
    return (0);
}

static inline tCDCSerInstance * UsbIndexToBase (uint32_t index)
{
    /*SAFETYMCUSW 440 S MR:11.3 <INSPECTED> "Reason -  Acceptable deviation."*/
    return ((tCDCSerInstance *)(USB0_BASE));
}

/** ***************************************************************************
 *
 * Set or clear deferred operation flags in an "atomic" manner.
 *
 * \param pusDeferredOp points to the flags variable which is to be modified.
 * \param usBit indicates which bit number is to be set or cleared.
 * \param bSet indicates the state that the flag must be set to.  If \b TRUE,
 * the flag is set, if \b FALSE, the flag is cleared.
 *
 * This function safely sets or clears a bit in a flag variable.  The operation
 * makes use of bitbanding to ensure that the operation is atomic (no read-
 * modify-write is required).
 *
 * \return None.
 *
 *****************************************************************************/
static void
SetDeferredOpFlag(volatile uint32 * pusDeferredOp, uint16 usBit, tBoolean bSet)
{
#ifdef DISABLE_BIT_BAND

	if(bSet)
	{
        /*SAFETYMCUSW 185 S <INSPECTED> "Reason -  LDRA tool issue."*/
        /*SAFETYMCUSW 95 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
        /*SAFETYMCUSW 94 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
		HWREG(pusDeferredOp) |= (1u << usBit);
	}
	else
	{
        /*SAFETYMCUSW 185 S <INSPECTED> "Reason -  LDRA tool issue."*/
        /*SAFETYMCUSW 95 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
        /*SAFETYMCUSW 94 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
		HWREG(pusDeferredOp) &= ~(1u << usBit);
	}
	
#else
	/*
     * Set the flag bit to 1 or 0 using a bitband access.
     */

	 HWREGBITH(pusDeferredOp, usBit) = bSet ? 1 : 0;
#endif
}

/** ***************************************************************************
 *
 * Determines whether or not a client has consumed all received data previously
 * passed to it.
 *
 *  \param psDevice is the pointer to the device instance structure as returned
 *  by USBDCDCInit().
 *
 * This function is called to determine whether or not a device has consumed
 * all data previously passed to it via its receive callback.
 *
 * \return Returns \b TRUE on success or \b FALSE on failure.
 *
 *****************************************************************************/
static tBoolean
DeviceConsumedAllData(const tUSBDCDCDevice *psDevice)
{
    uint32 ulRemaining;

    /*
     * Send the device an event requesting that it tell us how many bytes
     * of data it still has to process.
     */
    ulRemaining = psDevice->pfnRxCallback(psDevice->pvRxCBData,
    USB_EVENT_DATA_REMAINING, 0, (void *)0);

    /*
     * If any data remains to be processed, return FALSE, else return TRUE.
     */
    return ((ulRemaining != 0u) ? false : true);
}

/** ***************************************************************************
 *
 * Notifies the client that it should set or clear a break condition.
 *
 * \param psDevice is the pointer to the device instance structure as returned
 * by USBDCDCInit().
 * \param bSend is \b TRUE if a break condition is to be set or \b FALSE if
 * it is to be cleared.
 *
 * This function is called to instruct the client to start or stop sending a
 * break condition on its serial transmit line.
 *
 * \return Returns \b TRUE on success or \b FALSE on failure.
 *
 *****************************************************************************/
static void
SendBreak(const tUSBDCDCDevice *psDevice, tBoolean bSend)
{
    tCDCSerInstance *psInst;

    /*
     * Get our instance data pointer.
     */
    psInst = psDevice->psPrivateCDCSerData;

    /*
     * Set the break state flags as necessary.  If we are turning the break on,
     * set the flag to tell ourselves that we need to notify the client when
     * it is time to turn it off again.
     */
    SetDeferredOpFlag(&psInst->usDeferredOpFlags, (uint16)CDC_DO_SEND_BREAK, FALSE);
    SetDeferredOpFlag(&psInst->usDeferredOpFlags, (uint16)CDC_DO_CLEAR_BREAK, bSend);

    /*
     * Tell the client to start or stop sending the break.
     */
    psDevice->pfnControlCallback(psDevice->pvControlCBData,
                                 (bSend ? USBD_CDC_EVENT_SEND_BREAK :
                                          USBD_CDC_EVENT_CLEAR_BREAK), 0,
                                 (void *)0);
}

/** ***************************************************************************
 *
 * Notifies the client of a host request to set the serial communication
 * parameters.
 *
 * \param psDevice is the device instance whose communication parameters are to
 * be set.
 *
 * This function is called to notify the client when the host requests a change
 * in the serial communication parameters (baud rate, parity, number of bits
 * per character and number of stop bits) to use.
 *
 * \return None.
 *
 *****************************************************************************/
static void
SendLineCodingChange(const tUSBDCDCDevice *psDevice)
{
    tCDCSerInstance *psInst;

    /*
     * Get our instance data pointer.
     */
    psInst = psDevice->psPrivateCDCSerData;

    /*
     * Clear the flag we use to tell ourselves that the line coding change has
     * yet to be notified to the client.
     */
    SetDeferredOpFlag(&psInst->usDeferredOpFlags, (uint16)CDC_DO_LINE_CODING_CHANGE,
                      FALSE);

    /*
     * Tell the client to update their serial line coding parameters.
     */
    psDevice->pfnControlCallback(psDevice->pvControlCBData,
                                 USBD_CDC_EVENT_SET_LINE_CODING, 0,
                                 &(psInst->sLineCoding));
}

/** ***************************************************************************
 *
 * Notifies the client of a host request to set the RTS and DTR handshake line
 * states.
 *
 * \param psDevice is the device instance whose break condition is to be set or
 * cleared.
 *
 * This function is called to notify the client when the host requests a change
 * in the state of one or other of the RTS and DTR handshake lines.
 *
 * \return None.
 *
 *****************************************************************************/
static void
SendLineStateChange(const tUSBDCDCDevice *psDevice)
{
    tCDCSerInstance *psInst;

    /*
     * Get our instance data pointer.
     */
    psInst = psDevice->psPrivateCDCSerData;

    /*
     * Clear the flag we use to tell ourselves that the line coding change has
     * yet to be notified to the client.
     */
    SetDeferredOpFlag(&psInst->usDeferredOpFlags, (uint16)CDC_DO_LINE_STATE_CHANGE,
                      FALSE);

    /*
     * Tell the client to update their serial line coding parameters.
     */
    psDevice->pfnControlCallback(psDevice->pvControlCBData,
                                 USBD_CDC_EVENT_SET_CONTROL_LINE_STATE,
                                 psInst->usControlLineState,
                                 (void *)0);
}

/** ***************************************************************************
 *
 * Notifies the client of a break request if no data remains to be processed.
 *
 * \param psDevice is the device instance that is to be commanded to send a
 * break condition.
 *
 * This function is called when the host requests that the device set a break
 * condition on the serial transmit line.  If no data received from the host
 * remains to be processed, the break request is passed to the control
 * callback.  If data is outstanding, the call is ignored (with the operation
 * being retried on the next timer tick).
 *
 * \return Returns \b TRUE if the break notification was sent, \b FALSE
 * otherwise.
 *
 *****************************************************************************/
static tBoolean
CheckAndSendBreak(const tUSBDCDCDevice *psDevice, uint16 usDuration)
{
    tBoolean bCanSend;

    /*
     * Has the client consumed all data received from the host yet?
     */
    bCanSend = DeviceConsumedAllData(psDevice);

    /*
     * Can we send the break request?
     */
    if(bCanSend)
    {
        /*
         * Pass the break request on to the client since no data remains to be
         * consumed.
         */
        SendBreak(psDevice, ((usDuration != 0u) ? true : false));
    }

    /*
     * Tell the caller whether or not we sent the notification.
     */
    return (bCanSend);
}

/** ***************************************************************************
 *
 * Notifies the client of a request to change the serial line parameters if no
 * data remains to be processed.
 *
 * \param psDevice is the device instance whose line coding parameters are to
 * be changed.
 *
 * This function is called when the host requests that the device change the
 * serial line coding parameters.  If no data received from the host remains
 * to be processed, the request is passed to the control callback.  If data is
 * outstanding, the call is ignored (with the operation being retried on the
 * next timer tick).
 *
 * \return Returns \b TRUE if the notification was sent, \b FALSE otherwise.
 *
 *****************************************************************************/
static tBoolean
CheckAndSendLineCodingChange(const tUSBDCDCDevice *psDevice)
{
    tBoolean bCanSend;

    /*
     * Has the client consumed all data received from the host yet?
     */
    bCanSend = DeviceConsumedAllData(psDevice);

    /*
     * Can we send the break request?
     */
    if(bCanSend)
    {
        /*
         * Pass the request on to the client since no data remains to be
         * consumed.
         */
        SendLineCodingChange(psDevice);
    }

    /*
     * Tell the caller whether or not we sent the notification.
     */
    return (bCanSend);
}

/** ***************************************************************************
 *
 * Notifies the client of a request to change the handshake line states if no
 * data remains to be processed.
 *
 * \param psDevice is the device instance whose handshake line states are to
 * be changed.
 *
 * This function is called when the host requests that the device change the
 * state of one or other of the RTS or DTR handshake lines.  If no data
 * received from the host remains to be processed, the request is passed to
 * the control callback.  If data is outstanding, the call is ignored (with
 * the operation being retried on the next timer tick).
 *
 * \return Returns \b TRUE if the notification was sent, \b FALSE otherwise.
 *
 *****************************************************************************/
static tBoolean
CheckAndSendLineStateChange(const tUSBDCDCDevice *psDevice)
{
    tBoolean bCanSend;

    /*
     * Has the client consumed all data received from the host yet?
     */
    bCanSend = DeviceConsumedAllData(psDevice);

    /*
     * Can we send the break request?
     */
    if(bCanSend)
    {
        /*
         * Pass the request on to the client since no data remains to be
         * consumed.
         */
        SendLineStateChange(psDevice);
    }

    /*
     * Tell the caller whether or not we sent the notification.
     */
    return (bCanSend);
}

/** ***************************************************************************
 *
 * Notifies the client of a change in the serial line state.
 *
 * \param psInst is the instance whose serial state is to be reported.
 *
 * This function is called to send the current serial state information to
 * the host via the the interrupt IN endpoint.  This notification informs the
 * host of problems or conditions such as parity errors, breaks received,
 * framing errors, etc.
 *
 * \return Returns \b TRUE on success or \b FALSE on failure.
 *
 *****************************************************************************/
static tBoolean
SendSerialState(const tUSBDCDCDevice * psDevice)
{
    tUSBRequest sRequest;
    uint16 serialState;
    tCDCSerInstance *psInst;
    uint32 retcode;

    /*
     * Get a pointer to our instance data.
     */
    psInst = psDevice->psPrivateCDCSerData;

    /*
     * Remember that we are in the middle of sending a notification.
     */
    psInst->eCDCInterruptState = CDC_STATE_WAIT_DATA;

    /*
     * Clear the flag we use to indicate that a send is required.
     */
    SetDeferredOpFlag(&psInst->usDeferredOpFlags, CDC_DO_SERIAL_STATE_CHANGE,
                      FALSE);
    /*
     * Take a snapshot of the serial state.
     */
    serialState = psInst->usSerialState;

    /*
     * Build the request we will use to send the notification.
     */
    sRequest.bmRequestType = (USB_RTYPE_DIR_IN | USB_RTYPE_CLASS |
    USB_RTYPE_INTERFACE);
    sRequest.bRequest = USB_CDC_NOTIFY_SERIAL_STATE;
    sRequest.wValue = 0;
    sRequest.wIndex = 0;
    sRequest.wLength = USB_CDC_NOTIFY_SERIAL_STATE_SIZE;

    /*
     * Write the request structure to the USB FIFO.
     */
    /*SAFETYMCUSW 95 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    /*SAFETYMCUSW 94 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    retcode = USBEndpointDataPut(psInst->ulUSBBase, 
                                  (uint16)psInst->ucControlEndpoint,
                                  (uint8 *)(&sRequest),
                                  sizeof(tUSBRequest));
                                  
    /*SAFETYMCUSW 95 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    /*SAFETYMCUSW 94 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    retcode = USBEndpointDataPut(psInst->ulUSBBase, 
                                  (uint16)psInst->ucControlEndpoint,
                                  (uint8 *)(&serialState),
                                  (uint32)USB_CDC_NOTIFY_SERIAL_STATE_SIZE);

    /*
     * Did we correctly write the data to the endpoint FIFO?
     */
    if(retcode == 0u)
    {
        /*
         * We put the data into the FIFO so now schedule it to be
         * sent.
         */
        retcode = USBEndpointDataSend(psInst->ulUSBBase,
                                       (uint16)psInst->ucControlEndpoint,
                                       USB_TRANS_IN);
    }

    /*
     * If an error occurred, mark the endpoint as idle (to prevent possible
     * lockup) and return an error.
     */
    if(retcode != 0u)
    {
        psInst->eCDCInterruptState = CDC_STATE_IDLE;
        return (FALSE);
    }
    else
    {
        /*
         * Everything went fine.  Clear the error bits that we just notified
         * and return TRUE.
         */
        /*SAFETYMCUSW 334 S MR: 10.5 <INSPECTED> "Reason -  LDRA tool issue."*/
        psInst->usSerialState &= (uint16)(~(serialState & (uint16)USB_CDC_SERIAL_ERRORS));
        return (TRUE);
    }
}

/** ***************************************************************************
 *
 * Receives notifications related to data received from the host.
 *
 * \param psDevice is the device instance whose endpoint is to be processed.
 * \param ulStatus is the USB interrupt status that caused this function to
 * be called.
 *
 * This function is called from HandleEndpoints for all interrupts signaling
 * the arrival of data on the bulk OUT endpoint (in other words, whenever the
 * host has sent us a packet of data).  We inform the client that a packet
 * is available and, on return, check to see if the packet has been read.  If
 * not, we schedule another notification to the client for a later time.
 *
 * \return Returns \b TRUE on success or \b FALSE on failure.
 *
 *****************************************************************************/
tBoolean
ProcessDataFromHost(const tUSBDCDCDevice * psDevice, uint32 ulStatus)
{
    uint32 ulEPStatus;
    uint32 dataSize;
    tCDCSerInstance * psInst;
    tBoolean isControlBlocked, isRxBlocked;

    /*
     * Get a pointer to our instance data.
     */
    psInst = psDevice->psPrivateCDCSerData;

    /*
     * Get the endpoint status to see why we were called.
     */
    ulEPStatus = USBEndpointStatus(psInst->ulUSBBase,
                                   (uint32)psInst->ucBulkOUTEndpoint);

    /*
     * Clear the status bits.
     */
    USBDevEndpointStatusClear(psInst->ulUSBBase, 
                              (uint32)psInst->ucBulkOUTEndpoint,
                              ulEPStatus);

    /*
     * Has a packet been received?
     */
    if((ulEPStatus & USB_DEV_RX_PKT_RDY) != 0u)
    {
        /*
         * Set the flag we use to indicate that a packet read is pending.  This
         * will be cleared if the packet is read.  If the client doesn't read
         * the packet in the context of the USB_EVENT_RX_AVAILABLE callback,
         * the event will be notified later during tick processing.
         */
        SetDeferredOpFlag(&psInst->usDeferredOpFlags, (uint16)CDC_DO_PACKET_RX, TRUE);

        /*
         * Is the receive channel currently blocked?
         */
        isControlBlocked = psInst->bControlBlocked;
        isRxBlocked = psInst->bRxBlocked;
        if((!isControlBlocked) && (!isRxBlocked))
        {
            /*
             * How big is the packet we've just been sent?
             */
            dataSize = USBEndpointDataAvail(psInst->ulUSBBase,
                                            (uint16)psInst->ucBulkOUTEndpoint);

            /*
             * The receive channel is not blocked so let the caller know
             * that a packet is waiting.  The parameters are set to indicate
             * that the packet has not been read from the hardware FIFO yet.
             */
            psDevice->pfnRxCallback(psDevice->pvRxCBData,
                                    USB_EVENT_RX_AVAILABLE, dataSize,
                                    (void *)0);
        }

	
    }
    else
    {
        /*
         * No packet was received.  Some error must have been reported.  Check
         * and pass this on to the client if necessary.
         */
        if((ulEPStatus & USB_RX_ERROR_FLAGS) != 0u)
        {
            /*
             * This is an error we report to the client so...
             */
            psDevice->pfnRxCallback(psDevice->pvRxCBData,
                                    USB_EVENT_ERROR,
                                    (ulEPStatus & USB_RX_ERROR_FLAGS),
                                    (void *)0);
        }

        return (FALSE);
    }

    return (TRUE);
}

/** ***************************************************************************
 *
 * Receives notifications related to interrupt messages sent to the host.
 *
 * \param psDevice is the device instance whose endpoint is to be processed.
 * \param ulStatus is the USB interrupt status that caused this function to
 * be called.
 *
 * This function is called from HandleEndpoints for all interrupts originating
 * from the interrupt IN endpoint (in other words, whenever a notification has
 * been transmitted to the USB host).
 *
 * \return Returns \b TRUE on success or \b FALSE on failure.
 *
 *****************************************************************************/
tBoolean
ProcessNotificationToHost(const tUSBDCDCDevice * psDevice,
                          uint32 ulStatus)
{
    uint32 ulEPStatus;
    tCDCSerInstance * psInst;
    tBoolean bRetcode;
    uint32 deferredOpFlags;

    /*
     * Assume all will go well until we have reason to believe otherwise.
     */
    bRetcode = TRUE;

    /*
     * Get a pointer to our instance data.
     */
    psInst = psDevice->psPrivateCDCSerData;

    /*
     * Get the endpoint status to see why we were called.
     */
    ulEPStatus = USBEndpointStatus(psInst->ulUSBBase,
                                   (uint32)(psInst->ucControlEndpoint));

    /*
     * Clear the status bits.
     */
    USBDevEndpointStatusClear(psInst->ulUSBBase,
                              (uint32)(psInst->ucControlEndpoint), 
                              ulEPStatus);

    /*
     * Did the state change while we were waiting for the previous notification
     * to complete?
     */
    deferredOpFlags = psInst->usDeferredOpFlags;
    if((deferredOpFlags & (1u << CDC_DO_SERIAL_STATE_CHANGE)) != 0u)
    {
        /*
         * The state changed while we were waiting so we need to schedule
         * another notification immediately.
         */
        bRetcode = SendSerialState(psDevice);
    }
    else
    {
        /*
         * Our last notification completed and we didn't have any new
         * notifications to make so the interrupt channel is now idle again.
         */
        psInst->eCDCInterruptState = CDC_STATE_IDLE;
    }

    /*
     * Tell the caller how things went.
     */
    return (bRetcode);
}

/** ***************************************************************************
 *
 * Receives notifications related to data sent to the host.
 *
 * \param psDevice is the device instance whose endpoint is to be processed.
 * \param ulStatus is the USB interrupt status that caused this function to
 * be called.
 *
 * This function is called from HandleEndpoints for all interrupts originating
 * from the bulk IN endpoint (in other words, whenever data has been
 * transmitted to the USB host).  We examine the cause of the interrupt and,
 * if due to completion of a transmission, notify the client.
 *
 * \return Returns \b TRUE on success or \b FALSE on failure.
 *
 *****************************************************************************/
tBoolean
ProcessDataToHost(const tUSBDCDCDevice * psDevice, uint32 ulStatus)
{
    tCDCSerInstance * psInst;
    uint32 ulEPStatus, txSize;

    /*
     * Get a pointer to our instance data.
     */
    psInst = psDevice->psPrivateCDCSerData;

    /*
     * Get the endpoint status to see why we were called.
     */
    ulEPStatus = USBEndpointStatus(psInst->ulUSBBase,
                                   (uint32)(psInst->ucBulkINEndpoint));

    /*
     * Clear the status bits.
     */
    USBDevEndpointStatusClear(psInst->ulUSBBase,
                              (uint32)(psInst->ucBulkINEndpoint), 
                              ulEPStatus);

    /*
     * Our last transmission completed.  Clear our state back to idle and
     * see if we need to send any more data.
     */
    psInst->eCDCTxState = CDC_STATE_IDLE;

    /*
     * Notify the client that the last transmission completed.
     */
    txSize = (uint32)psInst->usLastTxSize;
    psInst->usLastTxSize = 0u;
    psDevice->pfnTxCallback(psDevice->pvTxCBData, USB_EVENT_TX_COMPLETE,
                            txSize, (void *)0);

    return (TRUE);
}

/** ***************************************************************************
 *
 * Called by the USB stack for any activity involving one of our endpoints
 * other than EP0.  This function is a fan out that merely directs the call to
 * the correct handler depending upon the endpoint and transaction direction
 * signaled in ulStatus.
 *
 *****************************************************************************/
static void
HandleEndpoints(void * pvInstance, uint32 ulStatus)
{
    const tUSBDCDCDevice * psDeviceInst;
    tCDCSerInstance * psInst;

    ASSERT(pvInstance != 0);

    /*
     * Determine if the serial device is in single or composite mode because
     * the meaning of ulIndex is different in both cases.
     */
    psDeviceInst = pvInstance;
    psInst = psDeviceInst->psPrivateCDCSerData;

	/*
     * Handler for the interrupt IN notification endpoint.
     */
    /*SAFETYMCUSW 184 S <INSPECTED> "Reason -  LDRA tool issue."*/
    if((ulStatus & (1u << USB_EP_TO_INDEX(psInst->ucControlEndpoint))) != 0u)
    {
        /*
         * We have sent an interrupt notification to the host.
         */
        ProcessNotificationToHost(psDeviceInst, ulStatus);
    }

    /*
     * Handler for the bulk OUT data endpoint.
     */
    /*SAFETYMCUSW 184 S <INSPECTED> "Reason -  LDRA tool issue."*/
    if((ulStatus & (0x10000u << USB_EP_TO_INDEX(psInst->ucBulkOUTEndpoint))) != 0u)
    {
        /*
         * Data is being sent to us from the host.
         */
        ProcessDataFromHost(psDeviceInst, ulStatus);
    }

    /*
     * Handler for the bulk IN data endpoint.
     */
    /*SAFETYMCUSW 184 S <INSPECTED> "Reason -  LDRA tool issue."*/
    if((ulStatus & (1u << USB_EP_TO_INDEX(psInst->ucBulkINEndpoint))) != 0u)
    {
		ProcessDataToHost(psDeviceInst, ulStatus);
    }
}

/** ***************************************************************************
 *
 * Called by the USB stack whenever a configuration change occurs.
 *
 *****************************************************************************/
static void
HandleConfigChange(void * pvInstance, uint32 ulInfo)
{
    tCDCSerInstance * psInst;
    const tUSBDCDCDevice * psDevice;
    boolean isConnected;

    ASSERT(pvInstance != 0);

    /*
     * Create a device instance pointer.
     */
    /*SAFETYMCUSW 95 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    /*SAFETYMCUSW 94 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    psDevice = (const tUSBDCDCDevice *)pvInstance;

    /*
     * Get a pointer to our instance data.
     */
    psInst = psDevice->psPrivateCDCSerData;

    /*
     * Set all our endpoints to idle state.
     */
    psInst->eCDCInterruptState = CDC_STATE_IDLE;
    psInst->eCDCRequestState = CDC_STATE_IDLE;
    psInst->eCDCRxState = CDC_STATE_IDLE;
    psInst->eCDCTxState = CDC_STATE_IDLE;

    /*
     * If we are not currently connected so let the client know we are open
     * for business.
     */
    isConnected = psInst->bConnected;
    if(!isConnected)
    {
        /*
         * Pass the connected event to the client.
         */
        psDevice->pfnControlCallback(psDevice->pvControlCBData,
                                     USB_EVENT_CONNECTED, 0, (void *)0);
    }

    /*
     * Remember that we are connected.
     */
    psInst->bConnected = TRUE;
}

/** ***************************************************************************
 *
 * USB data received callback.
 *
 * This function is called by the USB stack whenever any data requested from
 * EP0 is received.
 *
 *****************************************************************************/
static void
HandleEP0Data(void * pvInstance, uint32 ulDataSize)
{
    const tUSBDCDCDevice * psDevice;
    tCDCSerInstance * psInst;
    tBoolean bRetcode;
    tCDCState reqState;
    uint8 pendingRequest;

    ASSERT(pvInstance != 0);

    /*
     * Create a device instance pointer.
     */
    /*SAFETYMCUSW 95 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    /*SAFETYMCUSW 94 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    psDevice = (const tUSBDCDCDevice *)pvInstance;

    /*
     * If we were not passed any data, just return.
     */
    if(ulDataSize == 0u)
    {
        return;
    }

    /*
     * Get our instance data pointer.
     */
    psInst = psDevice->psPrivateCDCSerData;

    /*
     * Make sure we are actually expecting something.
     */
    reqState = psInst->eCDCRequestState;
    if(reqState != CDC_STATE_WAIT_DATA)
    {
        return;
    }

    /*
     * Process the data received.  This will be a request-specific data
     * block associated with the last request received.
     */
    pendingRequest = psInst->ucPendingRequest;
    switch (pendingRequest)
    {
        /*
         * We just got the line coding structure.  Make sure the client has
         * read all outstanding data then pass it back to initiate a change
         * in the line state.
         */
        case USB_CDC_SET_LINE_CODING:
        {
            if(ulDataSize != sizeof(tLineCoding))
            {
                USBDCDStallEP0(0u);
            }
            else
            {
				/* TODO: Should we be really sending a ZLP?? */
				USBEndpointDataPut(USBD_0_BASE, 0u, 0, 0u);
				USBEndpointDataSend(USBD_0_BASE, 0u, (uint32)USBD_EP_DIR_IN);
				
                /*
                 * Set the flag telling us that we need to send a line coding
                 * notification to the client.
                 */
                SetDeferredOpFlag(&psInst->usDeferredOpFlags,
                                  (uint16)CDC_DO_LINE_CODING_CHANGE, TRUE);

                /*
                 * See if we can send the notification immediately.
                 */
                bRetcode = CheckAndSendLineCodingChange(psDevice);

                /*
                 * If we couldn't send the line coding change request to the
                 * client, block reception of more data from the host until
                 * previous data is processed and we send the change request.
                 */
                if(!bRetcode)
                {
                    psInst->bRxBlocked = TRUE;
                }
            }
            break;
        }

            /*
             * Oops - we seem to be waiting on a request which has not yet been
             * coded here.  Flag the error and stall EP0 anyway (even though
             * this would indicate a coding error).
             */
        default:
        {
            USBDCDStallEP0(0u);
            break;
        }
    }

    /*
     * All is well.  Set the state back to IDLE.
     */
    psInst->eCDCRequestState = CDC_STATE_IDLE;
}

/** ***************************************************************************
 *
 * Device instance specific handler.
 *
 *****************************************************************************/
static void
HandleDevice(void * pvInstance, uint32 ulRequest, void * pvRequestData)
{
    tCDCSerInstance * psInst;
    uint8 * pucData1;

    /*
     * Create the serial instance data.
     */
    /*SAFETYMCUSW 95 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    /*SAFETYMCUSW 94 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    psInst = ((tUSBDCDCDevice *)pvInstance)->psPrivateCDCSerData;

    /*
     * Create the char array used by the events supported by the USB CDC
     * serial class.
     */
    /*SAFETYMCUSW 95 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    /*SAFETYMCUSW 94 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    pucData1 = (uint8 *)pvRequestData;

    switch(ulRequest)
    {
        /*
         * This was an interface change event.
         */
        case USB_EVENT_COMP_IFACE_CHANGE:
        {
            /*
             * Save the change to the appropriate interface number.
             */
            /*SAFETYMCUSW 436 S MR:17.1 <INSPECTED> "Reason -  Acceptable deviation."*/
            if(pucData1[0] == SERIAL_INTERFACE_CONTROL)
            {
                /*SAFETYMCUSW 436 S MR:17.1 <INSPECTED> "Reason -  Acceptable deviation."*/
                psInst->ucInterfaceControl = pucData1[1];
            }
            else 
            {
                /*SAFETYMCUSW 436 S MR:17.1 <INSPECTED> "Reason -  Acceptable deviation."*/
                if(pucData1[0] == SERIAL_INTERFACE_DATA)
                {
                    /*SAFETYMCUSW 436 S MR:17.1 <INSPECTED> "Reason -  Acceptable deviation."*/
                    psInst->ucInterfaceData = pucData1[1];
                }
            }
            break;
        }

        /*
         * This was an endpoint change event.
         */
        case USB_EVENT_COMP_EP_CHANGE:
        {
            /*
             * Determine if this is an IN or OUT endpoint that has changed.
             */
            /*SAFETYMCUSW 436 S MR:17.1 <INSPECTED> "Reason -  Acceptable deviation."*/
            if((pucData1[0] & USB_EP_DESC_IN) != 0u)
            {
                /*
                 * Determine which IN endpoint to modify.
                 */
                /*SAFETYMCUSW 436 S MR:17.1 <INSPECTED> "Reason -  Acceptable deviation."*/
                if((pucData1[0] & 0x7fu) == USB_EP_TO_INDEX(CONTROL_ENDPOINT))
                {
                    /*SAFETYMCUSW 436 S MR:17.1 <INSPECTED> "Reason -  Acceptable deviation."*/
                    /*SAFETYMCUSW 184 S <INSPECTED> "Reason -  LDRA tool issue."*/
                    psInst->ucControlEndpoint =
                        INDEX_TO_USB_EP(pucData1[1] & 0x7fu);
                }
                else
                {
                    /*SAFETYMCUSW 436 S MR:17.1 <INSPECTED> "Reason -  Acceptable deviation."*/
                    /*SAFETYMCUSW 184 S <INSPECTED> "Reason -  LDRA tool issue."*/
                    psInst->ucBulkINEndpoint =
                        INDEX_TO_USB_EP(pucData1[1] & 0x7fu);
                }
            }
            else
            {
                /*
                 * Extract the new endpoint number.
                 */
                /*SAFETYMCUSW 436 S MR:17.1 <INSPECTED> "Reason -  Acceptable deviation."*/
                /*SAFETYMCUSW 184 S <INSPECTED> "Reason -  LDRA tool issue."*/
                psInst->ucBulkOUTEndpoint =
                    INDEX_TO_USB_EP(pucData1[1] & 0x7fu);
            }
            break;
        }

        /*
         * Handle class specific reconfiguring of the configuration descriptor
         * once the composite class has built the full descriptor.
         */
        case USB_EVENT_COMP_CONFIG:
        {
            /*
             * This sets the bFirstInterface of the Interface Association
             * descriptor to the first interface which is the control
             * interface used by this instance.
             */
            /*SAFETYMCUSW 436 S MR:17.1 <INSPECTED> "Reason -  Acceptable deviation."*/
            pucData1[2] = psInst->ucInterfaceControl;

            /*
             * This sets the bMasterInterface of the Union descriptor to the
             * Control interface and the bSlaveInterface of the Union
             * Descriptor to the Data interface used by this instance.
             */
            /*SAFETYMCUSW 436 S MR:17.1 <INSPECTED> "Reason -  Acceptable deviation."*/
            pucData1[29] = psInst->ucInterfaceControl;
            /*SAFETYMCUSW 436 S MR:17.1 <INSPECTED> "Reason -  Acceptable deviation."*/
            pucData1[30] = psInst->ucInterfaceData;

            /*
             * This sets the bDataInterface of the Union descriptor to the
             * Data interface used by this instance.
             */
            /*SAFETYMCUSW 436 S MR:17.1 <INSPECTED> "Reason -  Acceptable deviation."*/
            pucData1[35] = psInst->ucInterfaceData;
            break;
        }

        default:
        {
            break;
        }
    }
}

/** ***************************************************************************
 *
 * USB non-standard request callback.
 *
 * This function is called by the USB stack whenever any non-standard request
 * is made to the device.  The handler should process any requests that it
 * supports or stall EP0 in any unsupported cases.
 *
 *****************************************************************************/
static void
HandleRequests(void * pvInstance, tUSBRequest * pUSBRequest)
{
    const tUSBDCDCDevice * psDevice;
    tCDCSerInstance * psInst;
	tLineCoding sLnCoding;
	tBoolean bRetcode;
	

    ASSERT(pvInstance != 0);

    /*
     * Create a device instance pointer.
     */
    /*SAFETYMCUSW 95 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    /*SAFETYMCUSW 94 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    psDevice = (const tUSBDCDCDevice *)pvInstance;

    /*
     * Get our instance data pointer.
     */
    psInst = psDevice->psPrivateCDCSerData;

    /*
     * Only handle requests meant for this interface.
     */
    if(pUSBRequest->wIndex != psInst->ucInterfaceControl)
    {
        return;
    }

    /*
     * Handle each of the requests that we expect from the host.
     */
    switch(pUSBRequest->bRequest)
    {
        /*
         * Set the serial communication parameters.
         */
        case USB_CDC_SET_LINE_CODING:
        {
            /*
             * Remember the request we are processing.
             */
            psInst->ucPendingRequest = USB_CDC_SET_LINE_CODING;

            /*
             * Set the state to indicate we are waiting for data.
             */
            psInst->eCDCRequestState = CDC_STATE_WAIT_DATA;

            /*
             * Now read the payload of the request.  We handle the actual
             * operation in the data callback once this data is received.
             */
            /*SAFETYMCUSW 95 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
            /*SAFETYMCUSW 94 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
            USBDCDRequestDataEP0(0u, (uint8 *)(&psInst->sLineCoding),
                                 sizeof(tLineCoding));

            /*
             * ACK what we have already received.  We must do this after
             * requesting the data or we get into a race condition where the
             * data may return before we have set the stack state appropriately
             * to receive it.
             */
            USBDevEndpointDataAck(psInst->ulUSBBase, USB_EP_0, FALSE);

            break;
        }

        /*
         * Return the serial communication parameters.
         */
        case USB_CDC_GET_LINE_CODING:
        {
            /*
             * ACK what we have already received
             */
             USBDevEndpointDataAck(psInst->ulUSBBase, USB_EP_0, FALSE);

            /*
             * Ask the client for the current line coding.
             */
            psDevice->pfnControlCallback(psDevice->pvControlCBData,
                                        USBD_CDC_EVENT_GET_LINE_CODING, 0,
                                         &sLnCoding);
          	/*
             * Send the line coding information back to the host.
             */
            /*SAFETYMCUSW 95 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
            /*SAFETYMCUSW 94 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
            USBDCDSendDataEP0(0u, (uint8 *)(&sLnCoding),
                              sizeof(tLineCoding));

            break;
        }

        case USB_CDC_SET_CONTROL_LINE_STATE:
        {
            /*
             * ACK what we have already received
             */
            USBDevEndpointDataAck(psInst->ulUSBBase, USB_EP_0, FALSE);

            /*
             * Set the handshake lines as required.
             */
            psInst->usControlLineState = pUSBRequest->wValue;

            /*
             * Remember that we are due to notify the client of a line
             * state change.
             */
            SetDeferredOpFlag(&psInst->usDeferredOpFlags,
                              (uint16)CDC_DO_LINE_STATE_CHANGE, TRUE);

            /*
             * See if we can notify now.
             */
            bRetcode = CheckAndSendLineStateChange(psDevice);

            /*
             * If we couldn't send the line state change request to the
             * client, block reception of more data from the host until
             * previous data is processed and we send the change request.
             */
            if(!bRetcode)
            {
                psInst->bRxBlocked = TRUE;
            }

        	/* Send a zero packet */
        	USBEndpointDataPut(USBD_0_BASE, 0u, 0, 0u);
        	USBEndpointDataSend(USBD_0_BASE, 0u, (uint32)USBD_EP_DIR_IN);
            break;
        }

        case USB_CDC_SEND_BREAK:
        {
            /*
             * ACK what we have already received
             */
            USBDevEndpointDataAck(psInst->ulUSBBase, USB_EP_0, FALSE);

            /*
             * Keep a copy of the requested break duration.
             */
            psInst->usBreakDuration = pUSBRequest->wValue;

            /*
             * Remember that we need to send a break request.
             */
            SetDeferredOpFlag(&psInst->usDeferredOpFlags,
                              (uint16)CDC_DO_SEND_BREAK, TRUE);

            /*
             * Send the break request if all outstanding receive data has been
             * processed.
             */
            bRetcode = CheckAndSendBreak(psDevice, pUSBRequest->wValue);

            /*
             * If we couldn't send the line coding change request to the
             * client, block reception of more data from the host until
             * previous data is processed and we send the change request.
             */
            if(!bRetcode)
            {
                psInst->bRxBlocked = TRUE;
            }

            break;
        }

        default:
        {
            /*
             * We stall endpoint 0 if we receive any other request.
             * 
             * USB_CDC_SEND_ENCAPSULATED_COMMAND, 
             * USB_CDC_GET_ENCAPSULATED_COMMAND - This implementation makes 
             * use of no communication protocol so this request is meaningless.
             *
             * USB_CDC_SET_COMM_FEATURE, USB_CDC_GET_COMM_FEATURE,
             * USB_CDC_CLEAR_COMM_FEATURE -
             * These requests are apparently required by an ACM device but does
             * not appear relevant to a virtual COM port and is never used by
             * Windows (or, at least, is not seen when using Hyperterminal or
             * TeraTerm via a Windows virtual COM port).  We stall endpoint 0
             * to indicate that we do not support the request.
             *
             * USB_CDC_SET_AUX_LINE_STATE, USB_CDC_SET_HOOK_STATE,
             * USB_CDC_PULSE_SETUP, USB_CDC_SEND_PULSE, 
             * USB_CDC_SET_PULSE_TIME, USB_CDC_RING_AUX_JACK,
             * USB_CDC_SET_RINGER_PARMS, USB_CDC_GET_RINGER_PARMS,
             * USB_CDC_SET_OPERATION_PARMS, USB_CDC_GET_OPERATION_PARMS,
             * USB_CDC_SET_LINE_PARMS, USB_CDC_GET_LINE_PARMS, 
             * USB_CDC_DIAL_DIGITS, USB_CDC_SET_UNIT_PARAMETER,
             * USB_CDC_GET_UNIT_PARAMETER, USB_CDC_CLEAR_UNIT_PARAMETER,
             * USB_CDC_GET_PROFILE, USB_CDC_SET_ETHERNET_MULTICAST_FILTERS,
             * USB_CDC_SET_ETHERNET_POWER_MANAGEMENT_PATTERN_FILTER,
             * USB_CDC_GET_ETHERNET_POWER_MANAGEMENT_PATTERN_FILTER,
             * USB_CDC_SET_ETHERNET_PACKET_FILTER,
             * USB_CDC_GET_ETHERNET_STATISTIC, USB_CDC_SET_ATM_DATA_FORMAT,
             * USB_CDC_GET_ATM_DEVICE_STATISTICS, USB_CDC_SET_ATM_DEFAULT_VC,
             * USB_CDC_GET_ATM_VC_STATISTICS - These are valid CDC requests 
             * but not ones that an ACM device should receive.
             *
             * Otherwise the request is not part of the CDC specification.
             */
            
            USBDCDStallEP0(0u);
            break;
        }
    }
}

/** ***************************************************************************
 *
 * This function is called by the USB device stack whenever the device is
 * disconnected from the host.
 *
 *****************************************************************************/
static void
HandleDisconnect(void * pvInstance)
{
    const tUSBDCDCDevice * psCDCDevice;
    tCDCSerInstance * psInst;
    tBoolean connected;

    ASSERT(pvInstance != 0);

    /*
     * Create the instance pointer.
     */
    /*SAFETYMCUSW 95 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    /*SAFETYMCUSW 94 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    psCDCDevice = (const tUSBDCDCDevice *)pvInstance;

    /*
     * Get a pointer to our instance data.
     */
    psInst = psCDCDevice->psPrivateCDCSerData;

    /*
     * If we are not currently connected and we have a control callback,
     * let the client know we are open for business.
     */
    connected = psInst->bConnected;
    if(connected)
    {
        /*
         * Pass the disconnected event to the client.
         */
        psCDCDevice->pfnControlCallback(psCDCDevice->pvControlCBData,
                                        USB_EVENT_DISCONNECTED, 0, (void *)0);
    }

    /*
     * Remember that we are no longer connected.
     */
    psInst->bConnected = FALSE;
}

/** ***************************************************************************
 *
 * This function is called by the USB device stack whenever the bus is put into
 * suspend state.
 *
 *****************************************************************************/
static void
HandleSuspend(void * pvInstance)
{
    const tUSBDCDCDevice * psCDCDevice;

    ASSERT(pvInstance != 0);

    /*
     * Create the instance pointer.
     */
    /*SAFETYMCUSW 95 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    /*SAFETYMCUSW 94 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    psCDCDevice = (const tUSBDCDCDevice *)pvInstance;

    /*
     * Pass the event on to the client.
     */
    psCDCDevice->pfnControlCallback(psCDCDevice->pvControlCBData,
                                    USB_EVENT_SUSPEND, 0, (void *)0);
}

/** ***************************************************************************
 *
 * This function is called by the USB device stack whenever the bus is taken
 * out of suspend state.
 *
 *****************************************************************************/
static void
HandleResume(void * pvInstance)
{
    tUSBDCDCDevice * psCDCDevice;

    ASSERT(pvInstance != 0);

    /*
     * Create the instance pointer.
     */
    /*SAFETYMCUSW 95 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    /*SAFETYMCUSW 94 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    psCDCDevice = (tUSBDCDCDevice *)pvInstance;

    /*
     * Pass the event on to the client.
     */
    psCDCDevice->pfnControlCallback(psCDCDevice->pvControlCBData,
                                    USB_EVENT_RESUME, 0, (void *)0);
}


/** ***************************************************************************
 *
 *  Initializes CDC device operation when used with a composite device.
 * 
 *  \param ulIndex is the index of the USB controller in use.
 *  \param psCDCDevice points to a structure containing parameters customizing
 *  the operation of the CDC device.
 * 
 *  This call is very similar to USBDCDCInit() except that it is used for
 *  initializing an instance of the serial device for use in a composite device.
 * 
 *  \return Returns NULL on failure or the psCDCDevice pointer on success.
 *
 *****************************************************************************/
void *
USBDCDCCompositeInit(uint32 ulIndex, const tUSBDCDCDevice * psCDCDevice)
{
    tCDCSerInstance * psInst;
    tDeviceDescriptor * psDevDesc;

    /*
     * Check parameter validity.
     */
    ASSERT(ulIndex == 0);
    ASSERT(psCDCDevice);
    ASSERT(psCDCDevice->psPrivateCDCSerData);
    ASSERT(psCDCDevice->pfnControlCallback);
    ASSERT(psCDCDevice->pfnRxCallback);
    ASSERT(psCDCDevice->pfnTxCallback);

    /*
     * Create an instance pointer to the private data area.
     */
    psInst = psCDCDevice->psPrivateCDCSerData;

    /*
     * Set the default endpoint and interface assignments.
     */
    psInst->ucBulkINEndpoint = DATA_IN_ENDPOINT;
    psInst->ucBulkOUTEndpoint = DATA_OUT_ENDPOINT;
    psInst->ucInterfaceControl = SERIAL_INTERFACE_CONTROL;
    psInst->ucInterfaceData = SERIAL_INTERFACE_DATA;

    /*
     * By default do not use the interrupt control endpoint.  The single
     * instance CDC serial device will turn this on in USBDCDCInit
     */
    psInst->ucControlEndpoint = CONTROL_ENDPOINT;

    /*
     * Initialize the workspace in the passed instance structure.
     */
    psInst->psConfDescriptor = (tConfigDescriptor *)g_pCDCSerDescriptor;
    psInst->psDevInfo = &g_sCDCSerDeviceInfo;
    psInst->ulUSBBase = USBD_0_BASE;
    psInst->eCDCRxState = CDC_STATE_UNCONFIGURED;
    psInst->eCDCTxState = CDC_STATE_UNCONFIGURED;
    psInst->eCDCInterruptState = CDC_STATE_UNCONFIGURED;
    psInst->eCDCRequestState = CDC_STATE_UNCONFIGURED;
    psInst->ucPendingRequest = 0u;
    psInst->usBreakDuration = 0u;
    psInst->usSerialState = 0u;
    psInst->usDeferredOpFlags = 0u;
    psInst->usControlLineState = 0u;
    psInst->bRxBlocked = FALSE;
    psInst->bControlBlocked = FALSE;
    psInst->bConnected = FALSE;

    /*
     * Fix up the device descriptor with the client-supplied values.
     */
    psDevDesc = (tDeviceDescriptor *)psInst->psDevInfo->pDeviceDescriptor;
    psDevDesc->idVendor = psCDCDevice->usVID;
    psDevDesc->idProduct = psCDCDevice->usPID;

    /*
     * Fix up the configuration descriptor with client-supplied values.
     */
    psInst->psConfDescriptor->bmAttributes = psCDCDevice->ucPwrAttributes;
    psInst->psConfDescriptor->bMaxPower =
                (uint8)(psCDCDevice->usMaxPowermA / 2);

    /*
     * Plug in the client's string stable to the device information
     * structure.
     */
    psInst->psDevInfo->ppStringDescriptors = psCDCDevice->ppStringDescriptors;
    psInst->psDevInfo->ulNumStringDescriptors
            = psCDCDevice->ulNumStringDescriptors;

    /*
     * Return the pointer to the instance indicating that everything went well.
     */
    /*SAFETYMCUSW 203 S MR:11.5 <INSPECTED> "Reason -  Acceptable deviation."*/
    /*SAFETYMCUSW 95 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    /*SAFETYMCUSW 94 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    return((void *)psCDCDevice);
}

/** ***************************************************************************
 *
 *  Initializes CDC device operation for a given USB controller.
 * 
 *  \param ulIndex is the index of the USB controller which is to be
 *  initialized for CDC device operation.
 *  \param psCDCDevice points to a structure containing parameters customizing
 *  the operation of the CDC device.
 * 
 *  An application wishing to make use of a USB CDC communication channel and
 *  appear as a virtual serial port on the host system must call this function
 *  to initialize the USB controller and attach the device to the USB bus.
 *  This function performs all required USB initialization.
 * 
 *  The value returned by this function is the \e psCDCDevice pointer passed
 *  to it if successful.  This pointer must be passed to all later calls to the
 *  CDC class driver to identify the device instance.
 * 
 *  The USB CDC device class driver  offers packet-based transmit and receive
 *  operation.  If the application would rather use block based communication
 *  with transmit and receive buffers, USB buffers on the transmit and receive
 *  channels may be used to offer this functionality.
 * 
 *  Transmit Operation:
 * 
 *  Calls to USBDCDCPacketWrite() must send no more than 64 bytes of data at a
 *  time and may only be made when no other transmission is currently
 *  outstanding.
 * 
 *  Once a packet of data has been acknowledged by the USB host, a
 *  \b USB_EVENT_TX_COMPLETE event is sent to the application callback to
 *  inform it that another packet may be transmitted.
 * 
 *  Receive Operation:
 * 
 *  An incoming USB data packet will result in a call to the application
 *  callback with event \b USB_EVENT_RX_AVAILABLE.  The application must then
 *  call USBDCDCPacketRead(), passing a buffer capable of holding the received
 *  packet to retrieve the data and acknowledge reception to the USB host.  The
 *  size of the received packet may be queried by calling
 *  USBDCDCRxPacketAvailable().
 * 
 *  \note The application must not make any calls to the low level USB Device
 *  API if interacting with USB via the CDC device class API.  Doing so
 *  will cause unpredictable (though almost certainly unpleasant) behavior.
 * 
 *  \return Returns NULL on failure or the psCDCDevice pointer on success.
 *
 *****************************************************************************/
void * USBDCDCInit(uint32 ulIndex, const tUSBDCDCDevice * psCDCDevice)
{
    void * pvRet;
    tCDCSerInstance * psInst;

    /*
     * Initialize the internal state for this class.
     */
    /*SAFETYMCUSW 64 S MR: 1.2 <INSPECTED> "Reason -  LDRA tool issue, assumes void procedure."*/
    pvRet = USBDCDCCompositeInit(ulIndex, psCDCDevice);

    if(pvRet != NULL)
    {
        /*
         * Create an instance pointer to the private data area.
         */
        psInst = psCDCDevice->psPrivateCDCSerData;

        /*
         * Set the instance data for this device so that USBDCDInit() call can
         * have the instance data.
         */
        /*SAFETYMCUSW 203 S MR:11.5 <INSPECTED> "Reason -  Acceptable deviation."*/
        /*SAFETYMCUSW 95 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
        /*SAFETYMCUSW 94 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
        psInst->psDevInfo->pvInstance = (void *)psCDCDevice;

        /*
         * Enable the default interrupt control endpoint if this class is not
         * being used in a composite device.
         */
        psInst->ucControlEndpoint = CONTROL_ENDPOINT;

        /*
         * Use the configuration descriptor with the interrupt control endpoint.
         */
        psInst->psDevInfo->ppConfigDescriptors = g_pCDCSerConfigDescriptors;

        /*
         * All is well so now pass the descriptors to the lower layer and put
         * the CDC device on the bus.
         */
        USBDCDInit(ulIndex, psInst->psDevInfo);
    }

    /*SAFETYMCUSW 71 S MR:17.6 <INSPECTED> "Reason -  False positive."*/
    return(pvRet);
}

/** ***************************************************************************
 *
 *  Shuts down the CDC device instance.
 * 
 *  \param pvInstance is the pointer to the device instance structure as returned
 *  by USBDCDCInit().
 * 
 *  This function terminates CDC operation for the instance supplied and
 *  removes the device from the USB bus.  This function should not be called
 *  if the CDC device is part of a composite device and instead the
 *  USBDCompositeTerm() function should be called for the full composite
 *  device.
 * 
 *  Following this call, the \e pvInstance instance should not me used in any
 *  other calls.
 * 
 *  \return None.
 *
 *****************************************************************************/
void
USBDCDCTerm(void * pvInstance)
{
    tCDCSerInstance *psInst;

    ASSERT(pvInstance);

    /*
     * Get a pointer to our instance data.
     */
    /*SAFETYMCUSW 95 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    /*SAFETYMCUSW 94 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    psInst = ((tUSBDCDCDevice *)pvInstance)->psPrivateCDCSerData;

    /*
     * Terminate the requested instance.
     */
    USBDCDTerm(UsbBaseToIndex(psInst->ulUSBBase));

    psInst->ulUSBBase = 0u;
    psInst->psDevInfo = (tDeviceInfo *)0;
    psInst->psConfDescriptor = (tConfigDescriptor *)0;

    return;
}

/** ***************************************************************************
 *
 *  Sets the client-specific pointer for the control callback.
 * 
 *  \param devInst is the pointer to the device instance structure as
 *  returned by USBDCDCInit().
 *  \param pvCBData is the pointer that client wishes to be provided on each
 *  event sent to the control channel callback function.
 * 
 *  The client uses this function to change the callback pointer passed in
 *  the first parameter on all callbacks to the \e pfnControlCallback function
 *  passed on USBDCDCInit().
 * 
 *  If a client wants to make runtime changes in the callback pointer, it must
 *  ensure that the psCDCDevice structure passed to USBDCDCInit() resides in
 *  RAM.  If this structure is in flash, callback pointer changes will not be
 *  possible.
 * 
 *  \return Returns the previous callback pointer that was being used for
 *  this instance's control callback.
 *
 *****************************************************************************/
void *
USBDCDCSetControlCBData(tUSBDCDCDevice * devInst, void * pvCBData)
{
    void * pvOldValue;

    ASSERT(devInst);

    /*
     * Set the callback pointer for the control channel after remembering the
     * previous value.
     */
    pvOldValue = devInst->pvControlCBData;
    devInst->pvControlCBData = pvCBData;

    /*
     * Return the previous callback data value.
     */
    /*SAFETYMCUSW 71 S MR:17.6 <INSPECTED> "Reason -  False positive."*/
    return (pvOldValue);
}

/** ***************************************************************************
 *
 *  Sets the client-specific data parameter for the receive channel callback.
 * 
 *  \param pvInstance is the pointer to the device instance structure as
 *  returned by USBDCDCInit().
 *  \param pvCBData is the pointer that client wishes to be provided on each
 *  event sent to the receive channel callback function.
 * 
 *  The client uses this function to change the callback pointer passed in
 *  the first parameter on all callbacks to the \e pfnRxCallback function
 *  passed on USBDCDCInit().
 * 
 *  If a client wants to make runtime changes in the callback pointer, it must
 *  ensure that the psCDCDevice structure passed to USBDCDCInit() resides in
 *  RAM.  If this structure is in flash, callback data changes will not be
 *  possible.
 * 
 *  \return Returns the previous callback pointer that was being used for
 *  this instance's receive callback.
 *
 *****************************************************************************/
void *
USBDCDCSetRxCBData(void * pvInstance, void * pvCBData)
{
    tUSBDCDCDevice * devInst;
    void * pvOldValue;

    ASSERT(pvInstance);
    
    /*SAFETYMCUSW 95 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    /*SAFETYMCUSW 94 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    devInst = (tUSBDCDCDevice *)pvInstance;

    /*
     * Set the callback data for the receive channel after remembering the
     * previous value.
     */
    pvOldValue = devInst->pvRxCBData;
    devInst->pvRxCBData = pvCBData;

    /*
     * Return the previous callback pointer.
     */
    /*SAFETYMCUSW 71 S MR:17.6 <INSPECTED> "Reason -  False positive."*/
    return (pvOldValue);
}

/** ***************************************************************************
 *
 *  Sets the client-specific data parameter for the transmit callback.
 * 
 *  \param pvInstance is the pointer to the device instance structure as
 *  returned by USBDCDCInit().
 *  \param pvCBData is the pointer that client wishes to be provided on each
 *  event sent to the transmit channel callback function.
 * 
 *  The client uses this function to change the callback pointer passed in
 *  the first parameter on all callbacks to the \e pfnTxCallback function
 *  passed on USBDCDCInit().
 * 
 *  If a client wants to make runtime changes in the callback pointer, it must
 *  ensure that the psCDCDevice structure passed to USBDCDCInit() resides in
 *  RAM.  If this structure is in flash, callback data changes will not be
 *  possible.
 * 
 *  \return Returns the previous callback pointer that was being used for
 *  this instance's transmit callback.
 *
 *****************************************************************************/
void *
USBDCDCSetTxCBData(void * pvInstance, void * pvCBData)
{
    tUSBDCDCDevice * devInst;
    void * pvOldValue;

    ASSERT(pvInstance);
    
    /*
     * Get our instance data pointer
     */
    /*SAFETYMCUSW 95 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    /*SAFETYMCUSW 94 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    devInst = (tUSBDCDCDevice *)pvInstance;

    /*
     * Set the callback data for the transmit channel after remembering the
     * previous value.
     */
    pvOldValue = devInst->pvTxCBData;
    devInst->pvTxCBData = pvCBData;

    /*
     * Return the previous callback pointer.
     */
    /*SAFETYMCUSW 71 S MR:17.6 <INSPECTED> "Reason -  False positive."*/
    return (pvOldValue);
}

/** ***************************************************************************
 *
 *  Transmits a packet of data to the USB host via the CDC data interface.
 * 
 *  \param pvInstance is the pointer to the device instance structure as
 *  returned by USBDCDCInit().
 *  \param pcData points to the first byte of data which is to be transmitted.
 *  \param ulLength is the number of bytes of data to transmit.
 *  \param bLast indicates whether more data is to be written before a packet
 *  should be scheduled for transmission.  If \b TRUE, the client will make
 *  a further call to this function.  If \b FALSE, no further call will be
 *  made and the driver should schedule transmission of a short packet.
 * 
 *  This function schedules the supplied data for transmission to the USB
 *  host in a single USB packet.  If no transmission is currently ongoing
 *  the data is immediately copied to the relevant USB endpoint FIFO.  If the
 *  \e bLast parameter is \b TRUE, the newly written packet is then scheduled
 *  for transmission.  Whenever a USB packet is acknowledged by the host, a
 *  USB_EVENT_TX_COMPLETE event will be sent to the application transmit
 *  callback indicating that more data can now be transmitted.
 * 
 *  The maximum value for ulLength is 64 bytes (the maximum USB packet size
 *  for the bulk endpoints in use by CDC).  Attempts to send more data than
 *  this will result in a return code of 0 indicating that the data cannot be
 *  sent.
 * 
 *  \return Returns the number of bytes actually sent.  At this level, this
 *  will either be the number of bytes passed (if less than or equal to the
 *  maximum packet size for the USB endpoint in use and no outstanding
 *  transmission ongoing) or 0 to indicate a failure.
 *
 *****************************************************************************/
uint32
USBDCDCPacketWrite(void * pvInstance, uint8 * pcData,
                   uint32 ulLength, tBoolean bLast)
{
    tCDCSerInstance *psInst;
    uint32 retcode;
    tCDCState cdcState;
	
    ASSERT(pvInstance);

    /*
     * Get our instance data pointer
     */
    /*SAFETYMCUSW 95 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    /*SAFETYMCUSW 94 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    psInst = ((tUSBDCDCDevice *)pvInstance)->psPrivateCDCSerData;

	/*
     * Can we send the data provided?
     */
    cdcState = psInst->eCDCTxState;
    /*SAFETYMCUSW 185 S <INSPECTED> "Reason -  LDRA tool issue."*/
    /*SAFETYMCUSW 334 S MR: 10.5 <INSPECTED> "Reason -  LDRA tool issue."*/
    if((ulLength > DATA_IN_EP_MAX_SIZE) ||
       (cdcState != CDC_STATE_IDLE))
    {
        /*
         * Either the packet was too big or we are in the middle of sending
         * another packet.  Return 0 to indicate that we can't send this data.
         */
        return (0);
    }

    /*
     * Copy the data into the USB endpoint FIFO.
     */
    retcode = USBEndpointDataPut(psInst->ulUSBBase,
                                  (uint16)(psInst->ucBulkINEndpoint), 
                                  pcData,
                                  ulLength);

    /*
     * Did we copy the data successfully?
     */
    if(retcode == 0u)
    {
        /*
         * Remember how many bytes we sent.
         */
        psInst->usLastTxSize += (uint16)ulLength;

        /*
         * If this is the last call for this packet, schedule transmission.
         */
        if(bLast)
        {
            /*
             * Send the packet to the host if we have received all the data we
             * can expect for this packet.
             */
            psInst->eCDCTxState = CDC_STATE_WAIT_DATA;
            retcode = USBEndpointDataSend(psInst->ulUSBBase,
                                           (uint16)(psInst->ucBulkINEndpoint),
                                           USB_TRANS_IN);
        }
    }

    /*
     * Did an error occur while trying to send the data?
     */
    if(retcode == 0u)
    {
        /*
         * No - tell the caller we sent all the bytes provided.
         */
        return (ulLength);
    }
    else
    {
        /*
         * Yes - tell the caller we couldn't send the data.
         */
        return (0);
    }
}

/** ***************************************************************************
 *
 *  Reads a packet of data received from the USB host via the CDC data
 *  interface.
 * 
 *  \param pvInstance is the pointer to the device instance structure as
 *  returned by USBDCDCInit().
 *  \param pcData points to a buffer into which the received data will be
 *  written.
 *  \param ulLength is the size of the buffer pointed to by pcData.
 *  \param bLast indicates whether the client will make a further call to
 *  read additional data from the packet.
 * 
 *  This function reads up to ulLength bytes of data received from the USB
 *  host into the supplied application buffer.
 * 
 *  \note The \e bLast parameter is ignored in this implementation since the
 *  end of a packet can be determined without relying upon the client to
 *  provide this information.
 * 
 *  \return Returns the number of bytes of data read.
 *
 *****************************************************************************/
uint32
USBDCDCPacketRead(void * pvInstance, uint8 * pcData,
                  uint32 ulLength, tBoolean bLast)
{
    uint32 ulEPStatus, ulPkt;
    uint32 ulCount = 0U;
    tCDCSerInstance *psInst;
    sint32 iRetcode;
    tBoolean isRxBlocked;
    tBoolean isControlBlocked;
    ASSERT(pvInstance);

    /*
     * Get our instance data pointer
     */
    /*SAFETYMCUSW 94 S MR:11.1,11.2,11.4 <INSPECTED> "Reason -  Casting is required here."*/
    /*SAFETYMCUSW 95 S MR:11.1,11.4 <INSPECTED> "Reason -  Casting is required here."*/
    psInst = ((tUSBDCDCDevice *)pvInstance)->psPrivateCDCSerData;
    if (NULL == psInst)
    {
        /*SAFETYMCUSW 13 S MR:14.4 <INSPECTED> "Reason -  Allowed as per MISRA-C 2012 for single exit point."*/
        goto Exit;
    }
    
    /*
     * Does the relevant endpoint FIFO have a packet waiting for us?
     */
    ulEPStatus = USBEndpointStatus(psInst->ulUSBBase,
                                   (uint32)(psInst->ucBulkOUTEndpoint));

    if((ulEPStatus & USB_DEV_RX_PKT_RDY) != 0U)
    {
        /*
         * If receive is currently blocked or the buffer we were passed is
         * (potentially) too small, set the flag telling us that we have a
         * packet waiting but return 0.
         */
        isRxBlocked = psInst->bRxBlocked;
        isControlBlocked = psInst->bControlBlocked;
    
        if((isRxBlocked != FALSE) || (isControlBlocked != FALSE))
        {
            SetDeferredOpFlag(&psInst->usDeferredOpFlags,
                              (uint16)CDC_DO_PACKET_RX, TRUE);
            ulCount = 0U;
            /*SAFETYMCUSW 13 S MR:14.4 <INSPECTED> "Reason -  Allowed as per MISRA-C 2012 for single exit point."*/
            goto Exit;
        }
        else
        {
            /*
             * It is OK to receive the new packet.  How many bytes are
             * available for us to receive?
             */
            ulPkt = USBEndpointDataAvail(psInst->ulUSBBase,
                                         (uint16)(psInst->ucBulkOUTEndpoint));

            /*
             * Get as much data as we can.
             */
            ulCount = ulLength;
            iRetcode = USBEndpointDataGet(psInst->ulUSBBase,
                                          (uint16)(psInst->ucBulkOUTEndpoint),
                                          pcData, &ulCount);
            if (-1 == iRetcode)
            {
                ulCount = 0U;
                /*SAFETYMCUSW 13 S MR:14.4 <INSPECTED> "Reason -  Allowed as per MISRA-C 2012 for single exit point."*/
                goto Exit;
            }

            /*
             * Did we read the last of the packet data?
             */
            if(ulCount == ulPkt)
            {
                /*
                 * Clear the endpoint status so that we know no packet is
                 * waiting.
                 */
                USBDevEndpointStatusClear(psInst->ulUSBBase,
                                          (uint32)(psInst->ucBulkOUTEndpoint),
                                          ulEPStatus);

                /*
                 * Acknowledge the data, thus freeing the host to send the
                 * next packet.
                 */
                USBDevEndpointDataAck(psInst->ulUSBBase,
                                      (uint16)(psInst->ucBulkOUTEndpoint),
                                      TRUE);

                /*
                 * Clear the flag we set to indicate that a packet read is
                 * pending.
                 */
                SetDeferredOpFlag(&psInst->usDeferredOpFlags,
                                  (uint16)CDC_DO_PACKET_RX, FALSE);

            }
        }
    }

Exit:
    return (ulCount);
}

/** ***************************************************************************
 *
 *  Returns the number of free bytes in the transmit buffer.
 * 
 *  \param pvInstance is the pointer to the device instance structure as
 *  returned by USBDCDCInit().
 * 
 *  This function returns the maximum number of bytes that can be passed on a
 *  call to USBDCDCPacketWrite and accepted for transmission.  The value
 *  returned will be the maximum USB packet size (64) if no transmission is
 *  currently outstanding or 0 if a transmission is in progress.
 * 
 *  \return Returns the number of bytes available in the transmit buffer.
 *
 *****************************************************************************/
uint32
USBDCDCTxPacketAvailable(void * pvInstance)
{
    tCDCSerInstance *psInst;
    uint32 bufAvail = 0U;
    tCDCState txState;

    ASSERT(pvInstance);

    /*
     * Get our instance data pointer.
     */
    /*SAFETYMCUSW 94 S MR:11.1,11.2,11.4 <INSPECTED> "Reason -  Casting is required here."*/
    /*SAFETYMCUSW 95 S MR:11.1,11.4 <INSPECTED> "Reason -  Casting is required here."*/
    psInst = ((tUSBDCDCDevice *)pvInstance)->psPrivateCDCSerData;
    if (NULL == psInst)
    {
        /*SAFETYMCUSW 13 S MR:14.4 <INSPECTED> "Reason -  Allowed as per MISRA-C 2012 for single exit point."*/
        goto Exit;
    }

    /*
     * Do we have a packet transmission currently ongoing?
     */
    txState = psInst->eCDCTxState;
    if(txState != CDC_STATE_IDLE)
    {
        /*
         * We are not ready to receive a new packet so return 0.
         */
        bufAvail = 0U;
    }
    else
    {
        /*
         * We can receive a packet so return the max packet size for the
         * relevant endpoint.
         */
        /*SAFETYMCUSW 185 S <INSPECTED> "Reason -  LDRA tool issue."*/
        /*SAFETYMCUSW 334 S MR: 10.5 <INSPECTED> "Reason -  LDRA tool issue."*/
        /*SAFETYMCUSW 93 S <INSPECTED> "Reason -  Acceptable deviation."*/
        bufAvail = DATA_IN_EP_MAX_SIZE;
    }
    
Exit:
    return (bufAvail);
}

/** ***************************************************************************
 *
 *  Determines whether a packet is available and, if so, the size of the
 *  buffer required to read it.
 * 
 *  \param pvInstance is the pointer to the device instance structure as
 *  returned by USBDCDCInit().
 * 
 *  This function may be used to determine if a received packet remains to be
 *  read and allows the application to determine the buffer size needed to
 *  read the data.
 * 
 *  \return Returns 0 if no received packet remains unprocessed or the
 *  size of the packet if a packet is waiting to be read.
 *
 *****************************************************************************/
uint32
USBDCDCRxPacketAvailable(void * pvInstance)
{
    uint32 ulEPStatus;
    uint32 dataAvail = 0U;
    tCDCSerInstance *psInst;
    tBoolean isRxBlocked;
    tBoolean isControlBlocked;

    ASSERT(pvInstance);

    /*
     * Get our instance data pointer
     */
    /*SAFETYMCUSW 94 S MR:11.1,11.2,11.4 <INSPECTED> "Reason -  Casting is required here."*/
    /*SAFETYMCUSW 95 S MR:11.1,11.4 <INSPECTED> "Reason -  Casting is required here."*/
    psInst = ((tUSBDCDCDevice *)pvInstance)->psPrivateCDCSerData;
    if (NULL == psInst)
    {
        /*SAFETYMCUSW 13 S MR:14.4 <INSPECTED> "Reason -  Allowed as per MISRA-C 2012 for single exit point."*/
        goto Exit;
    }

    /*
     * If receive is currently blocked, return 0.
     */
    isRxBlocked = psInst->bRxBlocked;
    isControlBlocked = psInst->bControlBlocked;
    
    if((isRxBlocked != FALSE) || (isControlBlocked != FALSE))
    {
        /*SAFETYMCUSW 13 S MR:14.4 <INSPECTED> "Reason -  Allowed as per MISRA-C 2012 for single exit point."*/
        goto Exit;
    }

    /*
     * Does the relevant endpoint FIFO have a packet waiting for us?
     */
    ulEPStatus = USBEndpointStatus(psInst->ulUSBBase,
                                   (uint32)(psInst->ucBulkOUTEndpoint));

    if((ulEPStatus & USB_DEV_RX_PKT_RDY) != 0U)
    {
        /*
         * Yes - a packet is waiting.  How big is it?
         */
        dataAvail = USBEndpointDataAvail(psInst->ulUSBBase,
                                         (uint16)(psInst->ucBulkOUTEndpoint));
    }
    else
    {
        /*
         * There is no packet waiting to be received.
         */
        dataAvail = 0U;
    }

Exit:
    return(dataAvail);
}

/** ***************************************************************************
 *
 *  Informs the CDC module of changes in the serial control line states or
 *  receive error conditions.
 * 
 *  \param pvInstance is the pointer to the device instance structure as
 *  returned by USBDCDCInit().
 *  \param usState indicates the states of the various control lines and
 *  any receive errors detected.  Bit definitions are as for the USB CDC
 *  SerialState asynchronous notification and are defined in header file
 *  usbcdc.h.
 * 
 *  The application should call this function whenever the state of any of
 *  the incoming RS232 handshake signals changes or in response to a receive
 *  error or break condition.  The usState parameter is the ORed combination
 *  of the following flags with each flag indicating the presence of that
 *  condition.
 * 
 *  - USB_CDC_SERIAL_STATE_OVERRUN
 *  - USB_CDC_SERIAL_STATE_PARITY
 *  - USB_CDC_SERIAL_STATE_FRAMING
 *  - USB_CDC_SERIAL_STATE_RING_SIGNAL
 *  - USB_CDC_SERIAL_STATE_BREAK
 *  - USB_CDC_SERIAL_STATE_TXCARRIER
 *  - USB_CDC_SERIAL_STATE_RXCARRIER
 * 
 *  This function should be called only when the state of any flag changes.
 * 
 *  \return None.
 *
 *****************************************************************************/
void
USBDCDCSerialStateChange(void * pvInstance, uint16 usState)
{
    tCDCSerInstance *psInst;
    tCDCState       interruptState;

    ASSERT(pvInstance);

    /*
     * Get our instance data pointer
     */
    /*SAFETYMCUSW 94 S MR:11.1,11.2,11.4 <INSPECTED> "Reason -  Casting is required here."*/
    /*SAFETYMCUSW 95 S MR:11.1,11.4 <INSPECTED> "Reason -  Casting is required here."*/
    psInst = ((tUSBDCDCDevice *)pvInstance)->psPrivateCDCSerData;

    /*
     * Add the newly reported state bits to the current collection.  We do this
     * in case two state changes occur back-to-back before the first has been
     * notified.  There are two distinct types of signals that we report here
     * and we deal with them differently:
     *
     * 1.  Errors (overrun, parity, framing error) are ORed together so that
     *     any reported error is sent on the next notification.
     * 2.  Signal line states (RI, break, TX carrier, RX carrier) always
     *     report the last state notified to us.  The implementation here will
     *     send an interrupt showing the last state but, if two state changes
     *     occur very quickly, the host may receive a notification containing
     *     the same state that was last reported (in other words, a short pulse
     *     will be lost).  It would be possible to reduce the likelihood of
     *     this happening by building a queue of state changes and sending
     *     these in order but you are left with exactly the same problem if the
     *     queue fills up.  For now, therefore, we run the risk of missing very
     *     short pulses on the "steady-state" signal lines.
     */
    psInst->usSerialState |= (usState & USB_CDC_SERIAL_ERRORS);
    /*SAFETYMCUSW 185 S <INSPECTED> "Reason -  LDRA tool issue."*/
    psInst->usSerialState &= ~USB_CDC_SERIAL_ERRORS;
    /*SAFETYMCUSW 185 S <INSPECTED> "Reason -  LDRA tool issue."*/
    psInst->usSerialState |= (usState & ~USB_CDC_SERIAL_ERRORS);

    /*
     * Set the flag indicating that a serial state change is to be sent.
     */
    SetDeferredOpFlag(&psInst->usDeferredOpFlags, CDC_DO_SERIAL_STATE_CHANGE,
    					TRUE);

    /*
     * Can we send the state change immediately?
     */
    interruptState = psInst->eCDCInterruptState;
    if(interruptState == CDC_STATE_IDLE)
    {
        /*
         * The interrupt channel is free so send the notification immediately.
         * If we can't do this, the tick timer will catch this next time
         * round.
         */
        psInst->eCDCInterruptState = CDC_STATE_WAIT_DATA;
        SendSerialState(pvInstance);
    }

    return;
}

/** ***************************************************************************
 *
 *  Reports the device power status (bus- or self-powered) to the USB library.
 * 
 *  \param pvInstance is the pointer to the CDC device instance structure.
 *  \param ucPower indicates the current power status, either \b
 *  USB_STATUS_SELF_PWR or \b USB_STATUS_BUS_PWR.
 * 
 *  Applications which support switching between bus- or self-powered
 *  operation should call this function whenever the power source changes
 *  to indicate the current power status to the USB library.  This information
 *  is required by the USB library to allow correct responses to be provided
 *  when the host requests status from the device.
 * 
 *  \return None.
 *
 *****************************************************************************/
void
USBDCDCPowerStatusSet(void * pvInstance, uint8 ucPower)
{
    ASSERT(pvInstance);

    /*
     * Pass the request through to the lower layer.
     */
    USBDCDPowerStatusSet(0U, ucPower);
}

/** ***************************************************************************
 *
 *  Requests a remote wakeup to resume communication when in suspended state.
 * 
 *  \param pvInstance is the pointer to the CDC device instance structure.
 * 
 *  When the bus is suspended, an application which supports remote wakeup
 *  (advertised to the host via the config descriptor) may call this function
 *  to initiate remote wakeup signaling to the host.  If the remote wakeup
 *  feature has not been disabled by the host, this will cause the bus to
 *  resume operation within 20mS.  If the host has disabled remote wakeup,
 *  \b FALSE will be returned to indicate that the wakeup request was not
 *  successful.
 * 
 *  \return Returns \b TRUE if the remote wakeup is not disabled and the
 *  signaling was started or \b FALSE if remote wakeup is disabled or if
 *  signaling is currently ongoing following a previous call to this function.
 *
 *****************************************************************************/
tBoolean
USBDCDCRemoteWakeupRequest(void * pvInstance)
{
    ASSERT(pvInstance);

    /*
     * Pass the request through to the lower layer.
     */
    return(USBDCDRemoteWakeupRequest(0U));
}

/** ***************************************************************************
 *
 *  Close the Doxygen group.
 *  @}
 *
 *****************************************************************************/
