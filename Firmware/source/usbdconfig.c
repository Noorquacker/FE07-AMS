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
 *  @file       usbdconfig.c
 *
 *  @brief      High level USB device configuration function.
 * 
 */

#include "sys_common.h"
#include "hw_usb.h"
#include "usb.h"
#include "usblib.h"
#include "usbdevice.h"

/** ***************************************************************************
 *
 *  \ingroup device_api
 *  @{
 *
 *****************************************************************************/

/** ***************************************************************************
 *
 * Mask used to preserve various endpoint configuration flags.
 *
 *****************************************************************************/
#define EP_FLAGS_MASK           (USB_EP_MODE_MASK | USB_EP_DEV_IN |           \
                                 USB_EP_DEV_OUT)

/** ***************************************************************************
 *
 * Structure used in compiling FIFO size and endpoint properties from a
 * configuration descriptor.
 *
 *****************************************************************************/
typedef struct
{
    uint32 ulSize[2];
    uint32 ulType[2];
}
tUSBEndpointInfo;

/** ***************************************************************************
 *
 * Indices used when accessing the tUSBEndpointInfo structure.
 *
 *****************************************************************************/
#define EP_INFO_IN              0u
#define EP_INFO_OUT             1u

static uint32 USBDevFifoSizeToBytes(uint8 flags)
{
    uint8 sz = (uint8)((uint8)flags & (uint8)0x0Fu);
    uint32 numbytes;
    
    if ((flags & (uint8)USB_FIFO_SIZE_DB_FLAG) != 0u) {
        sz = sz + 1u;
    }
        
    numbytes = (uint32)((uint8)8u << sz);
    return (numbytes);
}


/** ***************************************************************************
 *
 * Given a maximum packet size and the user's FIFO scaling requirements,
 * determine the flags to use to configure the endpoint FIFO and the number
 * of bytes of FIFO space occupied.
 *
 *****************************************************************************/
static uint32
GetEndpointFIFOSize(uint32 ulMaxPktSize, const tFIFOEntry * psFIFOParams,
                    uint32 * pupBytesUsed)
{
    uint32 ulBytes;
    uint8 ulLoop;
    uint32 uFIFOSize;

    /*
     * A zero multiplier would not be a good thing.
     */
    ASSERT(psFIFOParams->cMultiplier);

    /*
     * What is the basic size required for a single buffered FIFO entry
     * containing the required number of packets?
     */
    ulBytes = ulMaxPktSize * (uint32)psFIFOParams->cMultiplier;

    /*
     * Now we need to find the nearest supported size that accommodates the
     * requested size.  Step through each of the supported sizes until we
     * find one that will do.
     */
    for(ulLoop = USB_FIFO_SZ_8; ulLoop <= USB_FIFO_SZ_1024; ulLoop++)
    {
        /*
         * How many bytes does this FIFO value represent?
         */
        uFIFOSize = USBDevFifoSizeToBytes(ulLoop);

        /*
         * Is this large enough to satisfy the request?
         */
        if(uFIFOSize >= ulBytes)
        {
            /*
             * Yes - are we being asked to double-buffer the FIFO for this
             * endpoint?
             */
            if(psFIFOParams->bDoubleBuffer)
            {
                /*
                 * Yes - FIFO requirement is double in this case.
                 */
                *pupBytesUsed = uFIFOSize * 2u;
                return((uint32)(ulLoop | USB_FIFO_SIZE_DB_FLAG));
            }
            else
            {
                /*
                 * No double buffering so just return the size and associated
                 * flag.
                 */
                *pupBytesUsed = uFIFOSize;
                return((uint32)ulLoop);
            }
        }
    }

    /*
     * If we drop out, we can't support the FIFO size requested.  Signal a
     * problem by returning 0 in the pBytesUsed
     */
    *pupBytesUsed = 0u;
    
    return(USB_FIFO_SZ_8);
}

/** ***************************************************************************
 *
 * Translate a USB endpoint descriptor into the values we need to pass to the
 * USBDevEndpointConfigSet() API.
 *
 *****************************************************************************/
static void
GetEPDescriptorType(tEndpointDescriptor * psEndpoint, uint32 * pulEPIndex,
                    uint32 * pulMaxPktSize, uint32 * pulFlags)
{
    /*
     * Get the endpoint index.
     */
    *pulEPIndex = psEndpoint->bEndpointAddress & USB_EP_DESC_NUM_M;

    /*
     * Extract the maximum packet size.
     */
    *pulMaxPktSize = psEndpoint->wMaxPacketSize & USB_EP_MAX_PACKET_COUNT_M;

    /*
     * Is this an IN or an OUT endpoint?
     */
    *pulFlags = ((psEndpoint->bEndpointAddress & USB_EP_DESC_IN) != 0u) ?
                 USB_EP_DEV_IN : USB_EP_DEV_OUT;

    /*
     * Set the endpoint mode.
     */
    switch(psEndpoint->bmAttributes & USB_EP_ATTR_TYPE_M)
    {
        case USB_EP_ATTR_CONTROL:
            *pulFlags |= USB_EP_MODE_CTRL;
            break;

        case USB_EP_ATTR_BULK:
            *pulFlags |= USB_EP_MODE_BULK;
            break;

        case USB_EP_ATTR_INT:
            *pulFlags |= USB_EP_MODE_INT;
            break;

        case USB_EP_ATTR_ISOC:
            *pulFlags |= USB_EP_MODE_ISOC;
            break;
            
        default:
            break;
    }
}

/** ***************************************************************************
 *
 *  Configure the USB controller appropriately for the device whose config
 *  descriptor is passed.
 * 
 *  \param ulIndex is the zero-based index of the USB controller which is to
 *  be configured.
 *  \param psConfig is a pointer to the configuration descriptor that the
 *  USB controller is to be set up to support.
 *  \param psFIFOConfig is a pointer to an array of NUM_USB_EP tFIFOConfig
 *  structures detailing how the FIFOs are to be set up for each endpoint
 *  used by the configuration.
 * 
 *  This function may be used to initialize a USB controller to operate as
 *  the device whose configuration descriptor is passed.  The function
 *  enables the USB controller, partitions the FIFO appropriately and
 *  configures each endpoint required by the configuration.  If the supplied
 *  configuration supports multiple alternate settings for any interface,
 *  the USB FIFO is set up assuming the worst case use (largest packet size
 *  for a given endpoint in any alternate setting using that endpoint) to
 *  allow for on-the-fly alternate setting changes later.  On return from this
 *  function, the USB controller is configured for correct operation of
 *  the default configuration of the device described by the descriptor passed.
 * 
 *  The \e psFIFOConfig parameter allows the caller to provide additional
 *  information on USB FIFO configuration that cannot be determined merely
 *  by parsing the configuration descriptor.  The descriptor provides
 *  information on the endpoints that are to be used and the maximum packet
 *  size for each but cannot determine whether, for example, double buffering
 *  is to be used or how many packets the application wants to be able to
 *  store in a given endpoint's FIFO.
 * 
 *  USBDCDConfig() is an optional call and applications may chose to make
 *  direct calls to SysCtlPeripheralEnable(), SysCtlUSBPLLEnable(),
 *  USBDevEndpointConfigSet() and USBFIFOConfigSet() instead of using this
 *  function.  If this function is used, it must be called prior to
 *  USBDCDInit() since this call assumes that the low level hardware
 *  configuration has been completed before it is made.
 * 
 *  \return Returns \b TRUE on success or \b FALSE on failure.
 *
 *****************************************************************************/
tBoolean
USBDeviceConfig(uint32 ulIndex, const tConfigHeader *psConfig,
                const tFIFOConfig *psFIFOConfig)
{
    uint32 ulLoop;
    uint32 ulCount;
    uint32 ulNumInterfaces;
    uint32 ulEpIndex;
    uint32 ulEpType;
    uint32 ulMaxPkt;
    uint32 ulNumEndpoints;
    uint32 ulFlags;
    uint32 ulBytesUsed;
    uint32 ulSection;
    tInterfaceDescriptor * psInterface;
    tEndpointDescriptor * psEndpoint;
    tUSBEndpointInfo psEPInfo[NUM_USB_EP - 1];

    /*
     * We only support 1 USB controller currently.
     */
    ASSERT(ulIndex == 0);

    /*
     * Catch bad pointers in a debug build.
     */
    ASSERT(psConfig);
    ASSERT(psFIFOConfig);

    /*
     * Clear out our endpoint info.
     */
    for(ulLoop = 0u; ulLoop < (NUM_USB_EP - 1u); ulLoop++)
    {
        psEPInfo[ulLoop].ulSize[EP_INFO_IN] = 0u;
        psEPInfo[ulLoop].ulType[EP_INFO_IN] = 0u;
        psEPInfo[ulLoop].ulSize[EP_INFO_OUT] = 0u;
        psEPInfo[ulLoop].ulType[EP_INFO_OUT] = 0u;
    }

    /*
     * How many (total) endpoints does this configuration describe?
     */
    ulNumEndpoints = USBDCDConfigDescGetNum(psConfig,
                                            USB_DTYPE_ENDPOINT);

    /*
     * How many interfaces are included?
     */
    ulNumInterfaces = USBDCDConfigDescGetNum(psConfig,
                                             USB_DTYPE_INTERFACE);

    /*
     * Look at each endpoint and determine the largest max packet size for
     * each endpoint.  This will determine how we partition the USB FIFO.
     */
    for(ulLoop = 0u; ulLoop < ulNumEndpoints; ulLoop++)
    {
        /*
         * Get a pointer to the endpoint descriptor.
         */
        /*SAFETYMCUSW 95 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
        /*SAFETYMCUSW 94 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
        psEndpoint = (tEndpointDescriptor *)USBDCDConfigDescGet(
                                psConfig, USB_DTYPE_ENDPOINT, ulLoop,
                                &ulSection);

        /*
         * Extract the endpoint number and whether it is an IN or OUT
         * endpoint.
         */
        ulEpIndex = (uint32)
                        psEndpoint->bEndpointAddress & USB_EP_DESC_NUM_M;
        ulEpType =  ((psEndpoint->bEndpointAddress & USB_EP_DESC_IN) != 0u)
                     ? EP_INFO_IN : EP_INFO_OUT;

        /*
         * Make sure the endpoint number is valid for our controller.  If not,
         * return FALSE to indicate an error.  Note that 0 is invalid since
         * you shouldn't reference endpoint 0 in the config descriptor.
         */
        if((ulEpIndex >= NUM_USB_EP) || (ulEpIndex == 0u))
        {
            return(FALSE);
        }

        /*
         * Does this endpoint have a max packet size requirement larger than
         * any previous use we have seen?
         */
        if(psEndpoint->wMaxPacketSize >
           psEPInfo[ulEpIndex - 1].ulSize[ulEpType])
        {
            /*
             * Yes - remember the new maximum packet size.
             */
            psEPInfo[ulEpIndex - 1].ulSize[ulEpType] =
                psEndpoint->wMaxPacketSize;
        }
    }

	/* Unlock the configuration registers */
	USBDevCfgUnlock(USBD_0_BASE);
    
    /*
     * At this point, we have determined the maximum packet size required
     * for each endpoint by any possible alternate setting of any interface
     * in this configuration.  Now determine the endpoint settings required
     * for the interface setting we are actually going to use.
     */
    for(ulLoop = 0u; ulLoop < ulNumInterfaces; ulLoop++)
    {
        /*
         * Get the next interface descriptor in the config descriptor.
         */
        psInterface = USBDCDConfigGetInterface(psConfig,
                                               ulLoop,
                                               USB_DESC_ANY,
                                               &ulSection);

        /*
         * Is this the default interface (bAlternateSetting set to 0)?
         */
        if((psInterface != NULL) && (psInterface->bAlternateSetting == 0u))
        {
            /*
             * This is an interface we are interested in so gather the
             * information on its endpoints.
             */
            ulNumEndpoints = (uint32)psInterface->bNumEndpoints;

            /*
             * Walk through each endpoint in this interface and configure
             * it appropriately.
             */
            for(ulCount = 0u; ulCount < ulNumEndpoints; ulCount++)
            {
                /*
                 * Get a pointer to the endpoint descriptor.
                 */
                psEndpoint = USBDCDConfigGetInterfaceEndpoint(psConfig,
                                            (uint32)psInterface->bInterfaceNumber,
                                            (uint32)psInterface->bAlternateSetting,
                                            ulCount);

                /*
                 * Make sure we got a good pointer.
                 */
                if(psEndpoint != NULL)
                {
                    /*
                     * Determine maximum packet size and flags from the
                     * endpoint descriptor.
                     */
                    GetEPDescriptorType(psEndpoint, &ulEpIndex, &ulMaxPkt,
                                        &ulFlags);

                    /*
                     * Make sure no-one is trying to configure endpoint 0.
                     */
                    if(ulEpIndex == 0u)
                    {
                        return(FALSE);
                    }

                    /*
                     * Include any additional flags that the user wants.
                     */
                    if((ulFlags & (USB_EP_DEV_IN | USB_EP_DEV_OUT)) ==
                        USB_EP_DEV_IN)
                    {
                        /*
                         * This is an IN endpoint.
                         */
                        ulFlags |= (uint32)(
                                  psFIFOConfig->sIn[ulEpIndex - 1].usEPFlags);
                        psEPInfo[ulEpIndex - 1].ulType[EP_INFO_IN] = ulFlags;
                    }
                    else
                    {
                        /*
                         * This is an OUT endpoint.
                         */
                        ulFlags |= (uint32)(
                                  psFIFOConfig->sOut[ulEpIndex - 1].usEPFlags);
                        psEPInfo[ulEpIndex - 1].ulType[EP_INFO_OUT] = ulFlags;
                    }

                    /*
                     * Set the endpoint configuration.
                     */
                    /*SAFETYMCUSW 458 S MR:10.1,10.2 <INSPECTED> "Reason -  LDRA tool issue."*/
                    USBDevEndpointConfigSet(USB0_BASE,
                                            (uint16)INDEX_TO_USB_EP(ulEpIndex),
                                            ulMaxPkt, ulFlags);
                }
            }
        }
    }

    /*
     * At this point, we have configured all the endpoints that are to be
     * used by this configuration's alternate setting 0.  Now we go on and
     * partition the FIFO based on the maximum packet size information we
     * extracted earlier.  Endpoint 0 is automatically configured to use the
     * first MAX_PACKET_SIZE_EP0 bytes of the FIFO so we start from there.
     */
    ulCount = MAX_PACKET_SIZE_EP0;
    for(ulLoop = 1u; ulLoop < NUM_USB_EP; ulLoop++)
    {
        /*
         * Configure the IN endpoint at this index if it is referred to
         * anywhere.
         */
        if(psEPInfo[ulLoop - 1].ulSize[EP_INFO_IN] != 0u)
        {
            /*
             * What FIFO size flag do we use for this endpoint?
             */
            ulMaxPkt = GetEndpointFIFOSize(
                                     psEPInfo[ulLoop - 1].ulSize[EP_INFO_IN],
                                     &(psFIFOConfig->sIn[ulLoop - 1]),
                                     &ulBytesUsed);

            /*
             * If we are told that 0 bytes of FIFO will be used, this implies
             * that there is an error in psFIFOConfig or the descriptor
             * somewhere so return an error indicator to the caller.
             */
            if(ulBytesUsed == 0u)
            {
                return(FALSE);
            }

            /*
             * Now actually configure the FIFO for this endpoint.
             */
            USBFIFOConfigSet(USB0_BASE, INDEX_TO_USB_EP(ulLoop), ulCount,
                             ulMaxPkt, USB_EP_DEV_IN);
            ulCount += ulBytesUsed;
        }

        /*
         * Configure the OUT endpoint at this index.
         */
        if(psEPInfo[ulLoop - 1].ulSize[EP_INFO_OUT] != 0u)
        {
            /*
             * What FIFO size flag do we use for this endpoint?
             */
            ulMaxPkt = GetEndpointFIFOSize(
                                     psEPInfo[ulLoop - 1].ulSize[EP_INFO_OUT],
                                     &(psFIFOConfig->sOut[ulLoop - 1]),
                                     &ulBytesUsed);

            /*
             * If we are told that 0 bytes of FIFO will be used, this implies
             * that there is an error in psFIFOConfig or the descriptor
             * somewhere so return an error indicator to the caller.
             */
            if(ulBytesUsed == 0u)
            {
                return(FALSE);
            }

            /*
             * Now actually configure the FIFO for this endpoint.
             */
            USBFIFOConfigSet(USB0_BASE, INDEX_TO_USB_EP(ulLoop), ulCount,
                             ulMaxPkt, USB_EP_DEV_OUT);
            ulCount += ulBytesUsed;
        }

    }
	
	/* Lock the configuration registers */
	USBDevCfgLock(USBD_0_BASE);
	
	/* Send a zero packet */
	USBEndpointDataPut(USBD_0_BASE, 0u, 0, 0u);
	USBEndpointDataSend(USBD_0_BASE, 0u, (uint32)USBD_EP_DIR_IN);

    /*
     * If we get to the end, all is well.
     */
    return(TRUE);
}

/** ***************************************************************************
 *
 *  Configure the affected USB endpoints appropriately for one alternate
 *  interface setting.
 * 
 *  \param ulIndex is the zero-based index of the USB controller which is to
 *  be configured.
 *  \param psConfig is a pointer to the configuration descriptor that contains
 *  the interface whose alternate settings is to be configured.
 *  \param ucInterfaceNum is the number of the interface whose alternate
 *  setting is to be configured.  This number corresponds to the
 *  bInterfaceNumber field in the desired interface descriptor.
 *  \param ucAlternateSetting is the alternate setting number for the desired
 *  interface.  This number corresponds to the bAlternateSetting field in the
 *  desired interface descriptor.
 * 
 *  This function may be used to reconfigure the endpoints of an interface
 *  for operation in one of the interface's alternate settings.  Note that this
 *  function assumes that the endpoint FIFO settings will not need to change
 *  and only the endpoint mode is changed.  This assumption is valid if the
 *  USB controller was initialized using a previous call to USBDCDConfig().
 * 
 *  In reconfiguring the interface endpoints, any additional configuration
 *  bits set in the endpoint configuration other than the direction (\b
 *  USB_EP_DEV_IN or \b USB_EP_DEV_OUT) and mode (\b USB_EP_MODE_MASK) are
 *  preserved.
 * 
 *  \return Returns \b TRUE on success or \b FALSE on failure.
 *
 *****************************************************************************/
tBoolean
USBDeviceConfigAlternate(uint32 ulIndex, const tConfigHeader *psConfig,
                         uint8 ucInterfaceNum,
                         uint8 ucAlternateSetting)
{
    uint32 ulNumInterfaces;
    uint32 ulNumEndpoints;
    uint32 ulLoop;
    uint32 ulCount;
    uint32 ulMaxPkt;
    uint32 ulOldMaxPkt;
    uint32 ulFlags;
    uint32 ulOldFlags;
    uint32 ulSection;
    uint32 ulEpIndex;
    tInterfaceDescriptor * psInterface;
    tEndpointDescriptor * psEndpoint;

    /*
     * How many interfaces are included in the descriptor?
     */
    ulNumInterfaces = USBDCDConfigDescGetNum(psConfig,
                                             USB_DTYPE_INTERFACE);

    /*
     * Find the interface descriptor for the supplied interface and alternate
     * setting numbers.
     */

    for(ulLoop = 0u; ulLoop < ulNumInterfaces; ulLoop++)
    {
        /*
         * Get the next interface descriptor in the config descriptor.
         */
        psInterface = USBDCDConfigGetInterface(psConfig, ulLoop, USB_DESC_ANY,
                                               &ulSection);

        /*
         * Is this the default interface (bAlternateSetting set to 0)?
         */
        if((psInterface != NULL) &&
           (psInterface->bInterfaceNumber == ucInterfaceNum) &&
           (psInterface->bAlternateSetting == ucAlternateSetting))
        {
            /*
             * This is an interface we are interested in and the descriptor
             * representing the alternate setting we want so go ahead and
             * reconfigure the endpoints.
             */

            /*
             * How many endpoints does this interface have?
             */
            ulNumEndpoints = (uint32)psInterface->bNumEndpoints;

            /*
             * Walk through each endpoint in turn.
             */
            for(ulCount = 0u; ulCount < ulNumEndpoints; ulCount++)
            {
                /*
                 * Get a pointer to the endpoint descriptor.
                 */
                psEndpoint = USBDCDConfigGetInterfaceEndpoint(psConfig,
                                              (uint32)psInterface->bInterfaceNumber,
                                              (uint32)psInterface->bAlternateSetting,
                                              ulCount);

                /*
                 * Make sure we got a good pointer.
                 */
                if(psEndpoint != NULL)
                {
                    /*
                     * Determine maximum packet size and flags from the
                     * endpoint descriptor.
                     */
                    GetEPDescriptorType(psEndpoint, &ulEpIndex, &ulMaxPkt,
                                        &ulFlags);

                    /*
                     * Make sure no-one is trying to configure endpoint 0.
                     */
                    if(ulEpIndex == 0u)
                    {
                        return(FALSE);
                    }

                    /*
                     * Get the existing endpoint configuration and mask in the
                     * new mode and direction bits, leaving everything else
                     * unchanged.
                     */
                    ulOldFlags = ulFlags;
                    /*SAFETYMCUSW 458 S MR:10.1,10.2 <INSPECTED> "Reason -  LDRA tool issue."*/
                    USBDevEndpointConfigGet(USB0_BASE,
                                            (uint16)INDEX_TO_USB_EP(ulEpIndex),
                                            &ulOldMaxPkt,
                                            &ulOldFlags);

                    /*
                     * Mask in the previous DMA and auto-set bits.
                     */
                    /*SAFETYMCUSW 185 S <INSPECTED> "Reason -  LDRA tool issue."*/
                    ulFlags = (ulFlags & EP_FLAGS_MASK) |
                              (ulOldFlags & ~EP_FLAGS_MASK);

                    /*
                     * Set the endpoint configuration.
                     */
                    /*SAFETYMCUSW 458 S MR:10.1,10.2 <INSPECTED> "Reason -  LDRA tool issue."*/
                    USBDevEndpointConfigSet(USB0_BASE,
                                            (uint16)INDEX_TO_USB_EP(ulEpIndex),
                                            ulMaxPkt, ulFlags);
                }
            }

            /*
             * At this point, we have reconfigured the desired interface so
             * return indicating all is well.
             */
            return(TRUE);
        }
    }

    return(FALSE);
}

/** ***************************************************************************
 *
 * Close the Doxygen group.
 *  @}
 *
 *****************************************************************************/
