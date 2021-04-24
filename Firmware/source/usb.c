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
 *  @file       usb.c
 *
 *  @brief      Driver for the USB Interface.
 * 
 */

/** ***************************************************************************
 *
 *  \ingroup usb_api
 *  @{
 *
 *****************************************************************************/
#include "sys_common.h"
#include "hw_usb.h"
#include "usb.h"

/*SAFETYMCUSW 440 S MR:11.3 <INSPECTED> "Reason -  Casting is required here."*/
usbdRegs * usbd0Regs = (usbdRegs *) USBD_0_BASE;

/** ***************************************************************************
 *
 *  Initialize the USB Device
 * 
 *  \param ulBase specifies the USB module base address.
 *  \param usFlags specifies the bus/self powered and endianness for data & dma.
 * 			Should be a combination of the following flags
 *  			USBD_PWR_BUS_PWR or USBD_PWR_SELF_PWR
 * 				USBD_DATA_ENDIAN_LITTLE or USBD_DATA_ENDIAN_BIG
 * 				USBD_DMA_ENDIAN_LITTLE or USBD_DMA_ENDIAN_BIG
 *  \param usFifoPtr specifies the start of the EP0 FIFO.
 * 
 *  This function will initialize the USB Device controller specified by the 
 *  \e ulBase parameter.
 * 
 *  \return None
 *   
 *  Note This function does not intiate a device connect (pull ups are 
 *  not enabled). Also the EP0 is intialized with FIFO size of 64Bytes.
 * 
 *
 *****************************************************************************/
void USBDevInit(uint32 ulBase, uint16 usFlags, uint16 usFifoPtr)
{
	uint32 ulIndex;
	
    /* Check the arguments. */
    ASSERT(ulBase == USBD_0_BASE);
	ASSERT((usFlags == USBD_PWR_BUS_PWR) 		|| 
		   (usFlags == USBD_PWR_SELF_PWR) 		||
		   (usFlags == USBD_DATA_ENDIAN_LITTLE) 	||
		   (usFlags == USBD_DATA_ENDIAN_BIG) 	||
		   (usFlags == USBD_DMA_ENDIAN_LITTLE) 	||
		   (usFlags == USBD_DMA_ENDIAN_BIG));
   ASSERT((usFifoPtr <= 0xFF));

	/* Clear the register & setup the user provided value */
	/*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Statically defined non-null hardware register address" */
    usbd0Regs->syscon1 = (usFlags & (USBD_SYSCON1_DATA_ENDIAN 	| 
									USBD_SYSCON1_DMA_ENDIAN 	|
									USBD_SYSCON1_SELF_PWR)) 	|
						USBD_SYSCON1_SOFF_DIS;
	
	/* Disable EPn & clear FIFO configuration */
    for (ulIndex=0u; ulIndex < USBD_EP_RX_MAX; ulIndex++) {
    	usbd0Regs->epn_rx[ulIndex] = 0u;
    }
	for (ulIndex=0u; ulIndex < USBD_EP_TX_MAX; ulIndex++) {
    	usbd0Regs->epn_tx[ulIndex] = 0u;
    }
	
	/* Setup EP0 FIFO to 64Bytes and enable EP0 */
    USBDevEp0Config(ulBase, (uint16)USB_FIFO_SZ_64, usFifoPtr);

	/* Lock the configuration to indicate EP0 is ready for use */
    usbd0Regs->syscon1 = USBD_SYSCON1_CFG_LOCK;
	
	/* Now setup rest of the configuration items */
	usbd0Regs->syscon1 = (usFlags & (USBD_SYSCON1_DATA_ENDIAN 	| 
									USBD_SYSCON1_DMA_ENDIAN 	|
									USBD_SYSCON1_SELF_PWR)) 	|
						USBD_SYSCON1_SOFF_DIS					|
						USBD_SYSCON1_CFG_LOCK					|
						USBD_SYSCON1_AUTODEC_DIS;

	/* Clear out SYSCON2 */
    usbd0Regs->syscon2 = 0u;
    
	/* Setup EP0 for transaction. Clear the EP0 FIFO & flags, disagle generation
	  of HALT condition and enable the FIFO. Before doing this select EP0 */
	usbd0Regs->epnum = 0u;
	/* Clear the halt flag */
    /*SAFETYMCUSW 184 S <INSPECTED> "Reason -  LDRA tool issue."*/
    /*SAFETYMCUSW 185 S <INSPECTED> "Reason -  LDRA tool issue."*/
    USBD_REG_BIT_CLR(usbd0Regs->ctrl, USBD_CTRL_SET_HALT);
    /*SAFETYMCUSW 184 S <INSPECTED> "Reason -  LDRA tool issue."*/
    USBD_REG_BIT_SET(usbd0Regs->ctrl, USBD_CTRL_CLR_EP);
    /*SAFETYMCUSW 184 S <INSPECTED> "Reason -  LDRA tool issue."*/
    USBD_REG_BIT_SET(usbd0Regs->ctrl, USBD_CTRL_SET_FIFO_EN);
	
	return;
}


/** ***************************************************************************
 *
 *  Initialize the USB Device's EP0 
 * 
 *  \param ulBase specifies the USB module base address.
 *  \param usSize FIFO size. Supported values are USB_FIFO_SZ_8/USB_FIFO_SZ_16/
 * 			USB_FIFO_SZ_32/USB_FIFO_SZ_64.
 *  \param usFifoPtr specifies the start of the EP0 FIFO.
 * 
 *  This function will initialize the USB Device controller specified by the 
 *  \e ulBase parameter.  The \e uFlags parameter is not used by this 
 *  implementation.
 * 
 *  \return None
 *   
 *
 *****************************************************************************/
void USBDevEp0Config(uint32 ulBase, uint16 usSize, uint16 usFifoPtr)
{
	/* Check the arguments. */
    ASSERT(ulBase == USBD_0_BASE);
	ASSERT((usSize == USB_FIFO_SZ_8) || 
		   (usSize == USB_FIFO_SZ_16) ||
		   (usSize == USB_FIFO_SZ_32) ||
		   (usSize == USB_FIFO_SZ_64));
	ASSERT((usFifoPtr <= 0xFF));
	
	/*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Statically defined non-null hardware register address" */
    usbd0Regs->ep0 =  ((uint16)(usSize << 12)) | usFifoPtr;
	return;
}

/** ***************************************************************************
 *
 *  Disable control interrupts on a given USB device controller.
 * 
 *  \param ulBase specifies the USB module base address.
 *  \param usFlags specifies which control interrupts to disable.
 * 
 *  This function will disable the interrupts for the USB device controller
 *  specified by the \e ulBase parameter.  The \e usFlags parameter specifies
 *  which control interrupts to disable.  The flags passed in the \e usFlags
 *  parameters should be the definitions that start with \b USBD_INT_EN_*
 * 
 *  \return None.
 *
 *****************************************************************************/
void USBIntDisable(uint32 ulBase, uint16 usFlags)
{
    /* Check the arguments. */
    ASSERT(ulBase == USBD_0_BASE);
    ASSERT((usFlags & ~(USBD_INT_EN_ALL)) == 0);
	
	/* Clear the specified bits */
    /*SAFETYMCUSW 184 S <INSPECTED> "Reason -  LDRA tool issue."*/
    /*SAFETYMCUSW 185 S <INSPECTED> "Reason -  LDRA tool issue."*/
	/*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Statically defined non-null hardware register address" */
    USBD_REG_BIT_CLR(usbd0Regs->irq_en, usFlags);
	
	return;
}

/** ***************************************************************************
 *
 *  Enable control interrupts on a given USB device controller.
 * 
 *  \param ulBase specifies the USB module base address.
 *  \param usFlags specifies which control interrupts to enable.
 * 
 *  This function will enable the control interrupts for the USB device controller
 *  specified by the \e ulBase parameter.  The \e usFlags parameter specifies
 *  which control interrupts to enable.  The flags passed in the \e usFlags
 *  parameters should be the definitions that start with \b USBD_INT_EN_* and
 *  not any other \b USB_INT flags.
 * 
 *  \return None.
 *
 *****************************************************************************/
void USBIntEnable(uint32 ulBase, uint16 usFlags)
{
    /* Check the arguments. */
    ASSERT(ulBase == USBD_0_BASE);
    ASSERT((usFlags & (~USBD_INT_EN_ALL)) == 0);
	
	/* Set the specified bits */
    /*SAFETYMCUSW 184 S <INSPECTED> "Reason -  LDRA tool issue."*/
	/*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Statically defined non-null hardware register address" */
    USBD_REG_BIT_SET(usbd0Regs->irq_en, usFlags);
	
	return;
}

/** ***************************************************************************
 *
 *  Returns the control interrupt status on a given USB device controller.
 * 
 *  \param ulBase specifies the USB module base address.
 * 
 *  This function will read interrupt status for a USB device controller.
 *  The bit values returned should be compared against the \b USBD_INT_SRC_*
 *  values.
 * 
 *  \return Returns the status of the control interrupts for a USB device controller.
 *
 *****************************************************************************/
uint16 USBIntStatus(uint32 ulBase)
{
    /* Check the arguments. */
    ASSERT(ulBase == USBD_0_BASE);

    /* Return the combined interrupt status. */
    return(usbd0Regs->irqsrc);
}

/** ***************************************************************************
 *
 *  Stalls the specified endpoint in device mode.
 * 
 *  \param ulBase specifies the USB module base address.
 *  \param usEndpoint specifies the endpoint to stall.
 *  \param usFlags specifies whether to stall the IN or OUT endpoint.
 * 
 *  This function will cause to endpoint number passed in to go into a stall
 *  condition.  If the \e usFlags parameter is \b USB_EP_DEV_IN then the stall
 *  will be issued on the IN portion of this endpoint.  If the \e usFlags
 *  parameter is \b USB_EP_DEV_OUT then the stall will be issued on the OUT
 *  portion of this endpoint.
 * 
 *  \note This function should only be called in device mode.
 * 
 *  \return None.
 *
 *****************************************************************************/
void USBDevEndpointStall(uint32 ulBase, uint16 usEndpoint,
                    uint16 usFlags)
{
    /* Check the arguments. */
    ASSERT(ulBase == USBD_0_BASE);
    ASSERT((usFlags & ~(USB_EP_DIR_IN | USB_EP_DIR_OUT)) == 0);
    ASSERT((usEndpoint == USB_EP_0) || (usEndpoint == USB_EP_1) ||
           (usEndpoint == USB_EP_2) || (usEndpoint == USB_EP_3) ||
           (usEndpoint == USB_EP_4) || (usEndpoint == USB_EP_5) ||
           (usEndpoint == USB_EP_6) || (usEndpoint == USB_EP_7) ||
           (usEndpoint == USB_EP_8) || (usEndpoint == USB_EP_9) ||
           (usEndpoint == USB_EP_10) || (usEndpoint == USB_EP_11) ||
           (usEndpoint == USB_EP_12) || (usEndpoint == USB_EP_13) ||
           (usEndpoint == USB_EP_14) || (usEndpoint == USB_EP_15));

	/*SAFETYMCUSW 184 S <INSPECTED> "Reason -  LDRA tool issue."*/
	USBD_REG_SET_ONE(usbd0Regs->epnum, (USBD_EP_NUM_EP_SEL | usFlags)); /* Select EP */
	
    /*SAFETYMCUSW 184 S <INSPECTED> "Reason -  LDRA tool issue."*/
	USBD_REG_SET_ONE(usbd0Regs->ctrl, USBD_CTRL_SET_HALT);				/* halt the selected EP */
	
    /*SAFETYMCUSW 184 S <INSPECTED> "Reason -  LDRA tool issue."*/
    /*SAFETYMCUSW 185 S <INSPECTED> "Reason -  LDRA tool issue."*/
    /*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Statically defined non-null hardware register address" */
    USBD_REG_BIT_CLR(usbd0Regs->epnum, USBD_EP_NUM_EP_SEL);				/* Deselct EP */
	
	return;
}

/** ***************************************************************************
 *
 *  Clears the stall condition on the specified endpoint in device mode.
 * 
 *  \param ulBase specifies the USB module base address.
 *  \param usEndpoint specifies which endpoint to remove the stall condition.
 *  \param usFlags specifies whether to remove the stall condition from the IN
 *  or the OUT portion of this endpoint.
 * 
 *  This function will cause the endpoint number passed in to exit the stall
 *  condition.  If the \e usFlags parameter is \b USB_EP_DEV_IN then the stall
 *  will be cleared on the IN portion of this endpoint.  If the \e usFlags
 *  parameter is \b USB_EP_DEV_OUT then the stall will be cleared on the OUT
 *  portion of this endpoint.
 * 
 *  \note This function should only be called in device mode.
 * 
 *  \return None.
 *
 *****************************************************************************/
void USBDevEndpointStallClear(uint32 ulBase, uint16 usEndpoint, uint16 usFlags)
{
    /* Check the arguments. */
    ASSERT(ulBase == USBD_0_BASE);
    ASSERT((usEndpoint == USB_EP_0) || (usEndpoint == USB_EP_1) ||
           (usEndpoint == USB_EP_2) || (usEndpoint == USB_EP_3) ||
           (usEndpoint == USB_EP_4) || (usEndpoint == USB_EP_5) ||
           (usEndpoint == USB_EP_6) || (usEndpoint == USB_EP_7) ||
           (usEndpoint == USB_EP_8) || (usEndpoint == USB_EP_9) ||
           (usEndpoint == USB_EP_10) || (usEndpoint == USB_EP_11) ||
           (usEndpoint == USB_EP_12) || (usEndpoint == USB_EP_13) ||
           (usEndpoint == USB_EP_14) || (usEndpoint == USB_EP_15));
    ASSERT((usFlags & ~(USB_EP_DEV_IN | USB_EP_DEV_OUT)) == 0);

    /* Select the specified endpoint and clear the stall */
    /*SAFETYMCUSW 184 S <INSPECTED> "Reason -  LDRA tool issue."*/
	USBD_REG_SET_ONE(usbd0Regs->epnum, (USBD_EP_NUM_EP_SEL | usFlags));
    /*SAFETYMCUSW 184 S <INSPECTED> "Reason -  LDRA tool issue."*/
	USBD_REG_BIT_SET(usbd0Regs->ctrl, USBD_CTRL_CLR_HALT);
    /*SAFETYMCUSW 184 S <INSPECTED> "Reason -  LDRA tool issue."*/
    /*SAFETYMCUSW 185 S <INSPECTED> "Reason -  LDRA tool issue."*/
	/*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Statically defined non-null hardware register address" */
    USBD_REG_BIT_CLR(usbd0Regs->epnum, USBD_EP_NUM_EP_SEL);
}

/** ***************************************************************************
 *
 *  Connects the USB device controller to the bus in device mode.
 * 
 *  \param ulBase specifies the USB module base address.
 * 
 *  This function will cause the soft connect feature of the USB device controller to
 *  be enabled.  Call USBDisconnect() to remove the USB device from the bus.
 * 
 * 
 *  \return None.
 *
 *****************************************************************************/
void USBDevConnect(uint32 ulBase)
{
	/* Check the arguments. */
    ASSERT(ulBase == USBD_0_BASE);

    /* Enable connection to the USB bus. */
    /*SAFETYMCUSW 184 S <INSPECTED> "Reason -  LDRA tool issue."*/
	/*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Statically defined non-null hardware register address" */
    USBD_REG_BIT_SET(usbd0Regs->syscon1, USBD_SYSCON1_PULLUP_EN);
}

/** ***************************************************************************
 *
 *  Removes the USB device controller from the bus in device mode.
 * 
 *  \param ulBase specifies the USB module base address.
 * 
 *  This function will cause the soft disconnect feature of the USB device controller to
 *  remove the device from the USB bus.  A call to USBDevConnect() is needed to
 *  reconnect to the bus.
 * 
 * 
 *  \return None.
 *
 *****************************************************************************/
void USBDevDisconnect(uint32 ulBase)
{
	/* Check the arguments. */
    ASSERT(ulBase == USBD_0_BASE);

    /* Remove connection to the USB bus. */
    /*SAFETYMCUSW 184 S <INSPECTED> "Reason -  LDRA tool issue."*/
    /*SAFETYMCUSW 185 S <INSPECTED> "Reason -  LDRA tool issue."*/
	/*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Statically defined non-null hardware register address" */
    USBD_REG_BIT_CLR(usbd0Regs->syscon1, USBD_SYSCON1_PULLUP_EN);
}

/** ***************************************************************************
 *
 *  Sets the address in device mode.
 * 
 *  \param ulBase specifies the USB module base address.
 *  \param ulAddress is the address to use for a device.
 * 
 *  This function will set the device address on the USB bus.  This address was
 *  likely received via a SET ADDRESS command from the host controller.
 * 
 *  \note This function is not available on this controller. This is maintained 
 *  		for compatibility.
 * 
 *  \return None.
 *
 *****************************************************************************/
void USBDevAddrSet(uint32 ulBase, uint32 ulAddress)
{
	/* Check the arguments. */
    ASSERT(ulBase == USBD_0_BASE);

    /* Request is auto decoded, so nothing to be done here */
}


/** ***************************************************************************
 *
 *  Determine the number of bytes of data available in a given endpoint's FIFO.
 * 
 *  \param ulBase specifies the USB module base address.
 *  \param usEndpoint is the endpoint to access.
 * 
 *  This function will return the number of bytes of data currently available
 *  in the FIFO for the given receive (OUT) endpoint.  It may be used prior to
 *  calling USBEndpointDataGet() to determine the size of buffer required to
 *  hold the newly-received packet.
 * 
 *  \return This call will return the number of bytes available in a given
 *  endpoint FIFO.
 *
 *****************************************************************************/
uint16 USBEndpointDataAvail(uint32 ulBase, uint16 usEndpoint)
{
    uint16 retVal;
    
	/* Check the arguments. */
    ASSERT(ulBase == USBD_0_BASE);
    ASSERT((usEndpoint == USB_EP_0) || (usEndpoint == USB_EP_1) ||
           (usEndpoint == USB_EP_2) || (usEndpoint == USB_EP_3) ||
           (usEndpoint == USB_EP_4) || (usEndpoint == USB_EP_5) ||
           (usEndpoint == USB_EP_6) || (usEndpoint == USB_EP_7) ||
           (usEndpoint == USB_EP_8) || (usEndpoint == USB_EP_9) ||
           (usEndpoint == USB_EP_10) || (usEndpoint == USB_EP_11) ||
           (usEndpoint == USB_EP_12) || (usEndpoint == USB_EP_13) ||
           (usEndpoint == USB_EP_14) || (usEndpoint == USB_EP_15));
    
	usEndpoint = usEndpoint >> 4;
	
	/* Setup the epnum register. Note that it's always OUT EP */
    /*SAFETYMCUSW 184 S <INSPECTED> "Reason -  LDRA tool issue."*/
	USBD_REG_SET_ONE(usbd0Regs->epnum, (USBD_EP_NUM_EP_SEL | usEndpoint | USBD_EP_DIR_OUT));
	
	/* Read the count from the RXFstat register */
	retVal = usbd0Regs->rxf_stat & USBD_RXFSTAT_RXF_COUNT;
	
	/* Remove access to the EP */
    /*SAFETYMCUSW 184 S <INSPECTED> "Reason -  LDRA tool issue."*/
    /*SAFETYMCUSW 185 S <INSPECTED> "Reason -  LDRA tool issue."*/
	/*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Statically defined non-null hardware register address" */
    USBD_REG_BIT_CLR(usbd0Regs->epnum, USBD_EP_NUM_EP_SEL);
	
	usEndpoint = usEndpoint << 4;
	
	return (retVal);	
}


/** ***************************************************************************
 *
 *  Retrieves data from the given endpoint's FIFO.
 * 
 *  \param ulBase specifies the USB module base address.
 *  \param usEndpoint is the endpoint to access.
 *  \param pucData is a pointer to the data area used to return the data from
 *  the FIFO.
 *  \param pulSize is initially the size of the buffer passed into this call
 *  via the \e pucData parameter.  It will be set to the amount of data
 *  returned in the buffer.
 * 
 *  This function will return the data from the FIFO for the given endpoint.
 *  The \e pulSize parameter should indicate the size of the buffer passed in
 *  the \e pulData parameter.  The data in the \e pulSize parameter will be
 *  changed to match the amount of data returned in the \e pucData parameter.
 *  If a zero byte packet was received this call will not return a error but
 *  will instead just return a zero in the \e pulSize parameter.  The only
 *  error case occurs when there is no data packet available.
 * 
 *  \return This call will return 0, or -1 if no packet was received.
 *
 *****************************************************************************/
sint32 USBEndpointDataGet(uint32 ulBase, uint16 usEndpoint, uint8 * pucData, uint32 * pulSize)
{
    uint16 usEpNum;
    uint32 usAvailLen, i;

	/* Check the arguments. */
    ASSERT(ulBase == USBD_0_BASE);
    ASSERT((usEndpoint == USB_EP_0) || (usEndpoint == USB_EP_1) ||
           (usEndpoint == USB_EP_2) || (usEndpoint == USB_EP_3) ||
           (usEndpoint == USB_EP_4) || (usEndpoint == USB_EP_5) ||
           (usEndpoint == USB_EP_6) || (usEndpoint == USB_EP_7) ||
           (usEndpoint == USB_EP_8) || (usEndpoint == USB_EP_9) ||
           (usEndpoint == USB_EP_10) || (usEndpoint == USB_EP_11) ||
           (usEndpoint == USB_EP_12) || (usEndpoint == USB_EP_13) ||
           (usEndpoint == USB_EP_14) || (usEndpoint == USB_EP_15));

    usEndpoint = usEndpoint >> 4;
	
	/* backup the EP number to restore later */
	usEpNum = usbd0Regs->epnum & USBD_EP_NUM_EP_NUM_MASK;
	
	/* Setup access to the endpoint (since we are trying to read it's always a OUT EP) */
	usbd0Regs->epnum = USBD_EP_NUM_EP_SEL | USBD_EP_DIR_OUT | (usEndpoint & USBD_EP_NUM_EP_NUM_MASK);
	
	/* Retrieve the available data length & adjust actual length accordingly */
	/*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Statically defined non-null hardware register address" */
    usAvailLen = usbd0Regs->rxf_stat & USBD_RXFSTAT_RXF_COUNT;
	if (usAvailLen > *pulSize) {
		usAvailLen = *pulSize;
	} else {
		/* Update the return value accordingly */
		*pulSize = usAvailLen;
	}
	
	/* Get the data out */
	for (i=0u; i < usAvailLen; i++) {
        /*SAFETYMCUSW 436 S MR:17.1,17.4 <INSPECTED> "Reason -  Acceptable deviation."*/
		/*SAFETYMCUSW 95 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
        /*SAFETYMCUSW 94 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
	    pucData[i] = *((volatile uint8 *)(&usbd0Regs->data));
	}
	
	/* Restore the epnym register */
	usbd0Regs->epnum = usEpNum;
	
	/* setup to receive data on ep */
    /*SAFETYMCUSW 184 S <INSPECTED> "Reason -  LDRA tool issue."*/
	USBD_REG_SET_ONE(usbd0Regs->ctrl, USBD_CTRL_SET_FIFO_EN);

	usEndpoint = usEndpoint << 4;

    return(0);
}

/** ***************************************************************************
 *
 *  Retrieves the setup packet from EP0 Setup FIFO
 * 
 *  \param ulBase specifies the USB module base address.
 *  \param sPkt Pointer to the data area for storing the setup packet.
 * 			Atleast 8 bytes should be available.
 *  \param pusPktSize On return this contains the size of the setup packet (8Bytes)
 * 
 *  This function will retrieves the 8Byte long setup packet from the EP0 setup
 *  FIFO.
 * 
 *  \return None.
 *
 *****************************************************************************/
void USBDevGetSetupPacket (uint32 ulBase, uint8 * sPkt, uint16 * pusPktSize)
{
	register volatile uint8 * 	pSetupFifo;

	/* Check the arguments. */
	ASSERT(ulBase == USBD_0_BASE);

    /*SAFETYMCUSW 95 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    /*SAFETYMCUSW 94 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
	pSetupFifo = (volatile uint8 *)(&usbd0Regs->data);
    
	/* Select the setup IFO. This will clear the event flag */
	usbd0Regs->epnum = USBD_EP_NUM_SETUP_SEL;

	/* Extract the setup packet */
    /*SAFETYMCUSW 436 S MR:17.1,17.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    /*SAFETYMCUSW 436 S MR:17.1,17.4 <INSPECTED> "Reason -  Acceptable deviation."*/
	sPkt[0]	=  pSetupFifo[0];
    /*SAFETYMCUSW 436 S MR:17.1,17.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    /*SAFETYMCUSW 436 S MR:17.1,17.4 <INSPECTED> "Reason -  Acceptable deviation."*/
	sPkt[1]	=  pSetupFifo[0];
    /*SAFETYMCUSW 436 S MR:17.1,17.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    /*SAFETYMCUSW 436 S MR:17.1,17.4 <INSPECTED> "Reason -  Acceptable deviation."*/
	sPkt[2]	=  pSetupFifo[0];
    /*SAFETYMCUSW 436 S MR:17.1,17.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    /*SAFETYMCUSW 436 S MR:17.1,17.4 <INSPECTED> "Reason -  Acceptable deviation."*/
	sPkt[3]	=  pSetupFifo[0];
    /*SAFETYMCUSW 436 S MR:17.1,17.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    /*SAFETYMCUSW 436 S MR:17.1,17.4 <INSPECTED> "Reason -  Acceptable deviation."*/
	sPkt[4]	=  pSetupFifo[0];
    /*SAFETYMCUSW 436 S MR:17.1,17.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    /*SAFETYMCUSW 436 S MR:17.1,17.4 <INSPECTED> "Reason -  Acceptable deviation."*/
	sPkt[5]	=  pSetupFifo[0];
    /*SAFETYMCUSW 436 S MR:17.1,17.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    /*SAFETYMCUSW 436 S MR:17.1,17.4 <INSPECTED> "Reason -  Acceptable deviation."*/
	sPkt[6]	=  pSetupFifo[0];
    /*SAFETYMCUSW 436 S MR:17.1,17.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    /*SAFETYMCUSW 436 S MR:17.1,17.4 <INSPECTED> "Reason -  Acceptable deviation."*/
	sPkt[7]	=  pSetupFifo[0];

	*pusPktSize = 8u;

	/* Deselect the setup FIFO */
    /*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Statically defined non-null hardware register address" */
	usbd0Regs->epnum = 0u;

	return;
}


/** ***************************************************************************
 *
 *  Acknowledge that data was read from the given endpoint's FIFO in device
 *  mode.
 * 
 *  \param ulBase specifies the USB module base address.
 *  \param usEndpoint is the endpoint to access.
 *  \param bIsLastPacket This parameter is not used.
 * 
 *  This function acknowledges that the data was read from the endpoint's FIFO.
 *  The \e bIsLastPacket parameter is set to a \b true value if this is the
 *  last in a series of data packets on endpoint zero.  The \e bIsLastPacket
 *  parameter is not used for endpoints other than endpoint zero.  This call
 *  can be used if processing is required between reading the data and
 *  acknowledging that the data has been read.
 * 
 * 
 *  \return None.
 *
 *****************************************************************************/
void USBDevEndpointDataAck(uint32 ulBase, uint16 usEndpoint, tBoolean bIsLastPacket)
{
	uint16 epNum;
    
	/* Check the arguments. */
    ASSERT(ulBase == USBD_0_BASE);
    ASSERT((usEndpoint == USB_EP_0) || (usEndpoint == USB_EP_1) ||
           (usEndpoint == USB_EP_2) || (usEndpoint == USB_EP_3) ||
           (usEndpoint == USB_EP_4) || (usEndpoint == USB_EP_5) ||
           (usEndpoint == USB_EP_6) || (usEndpoint == USB_EP_7) ||
           (usEndpoint == USB_EP_8) || (usEndpoint == USB_EP_9) ||
           (usEndpoint == USB_EP_10) || (usEndpoint == USB_EP_11) ||
           (usEndpoint == USB_EP_12) || (usEndpoint == USB_EP_13) ||
           (usEndpoint == USB_EP_14) || (usEndpoint == USB_EP_15));

	/* backup the EP number to restore later */
	epNum = usbd0Regs->epnum & USBD_EP_NUM_EP_NUM_MASK;
	
	/* Select the endpoint, (Always read (in) endpoint) */	   
	usbd0Regs->epnum = USBD_EP_DIR_OUT | USBD_EP_NUM_EP_SEL | (usEndpoint & USBD_EP_NUM_EP_NUM_MASK);

    /*SAFETYMCUSW 184 S <INSPECTED> "Reason -  LDRA tool issue."*/
	USBD_REG_SET_ONE(usbd0Regs->epnum, (USBD_EP_NUM_EP_SEL | usEndpoint | USBD_EP_DIR_OUT));
	/* Clear endpoint */
    /*SAFETYMCUSW 184 S <INSPECTED> "Reason -  LDRA tool issue."*/
	USBD_REG_SET_ONE(usbd0Regs->ctrl, USBD_CTRL_CLR_EP);
	/* Enable the fifo */
    /*SAFETYMCUSW 184 S <INSPECTED> "Reason -  LDRA tool issue."*/
	USBD_REG_SET_ONE(usbd0Regs->ctrl, USBD_CTRL_SET_FIFO_EN);
	/* Remove fifo access */
    /*SAFETYMCUSW 184 S <INSPECTED> "Reason -  LDRA tool issue."*/
    /*SAFETYMCUSW 185 S <INSPECTED> "Reason -  LDRA tool issue."*/
	USBD_REG_BIT_CLR(usbd0Regs->epnum, USBD_EP_NUM_EP_SEL);
	
	/* Restore the epnym register */
    /*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Statically defined non-null hardware register address" */
	usbd0Regs->epnum = epNum;

	return;
}


/** ***************************************************************************
 *
 *  Puts data into the given endpoint's FIFO.
 * 
 *  \param ulBase specifies the USB module base address.
 *  \param usEndpoint is the endpoint to access.
 *  \param pucData is a pointer to the data area used as the source for the
 *  data to put into the FIFO.
 *  \param ulSize is the amount of data to put into the FIFO.
 * 
 *  This function will put the data from the \e pucData parameter into the FIFO
 *  for this endpoint.  If a packet is already pending for transmission then
 *  this call will not put any of the data into the FIFO and will return -1.
 *  Care should be taken to not write more data than can fit into the FIFO
 *  allocated by the call to USBFIFOConfig().
 * 
 *  \return This call will return 0 on success, or -1 to indicate that the FIFO
 *  is in use and cannot be written.
 *
 *****************************************************************************/
uint32
USBEndpointDataPut(uint32 ulBase, uint16 usEndpoint,
                   uint8 * pucData, uint32 ulSize)
{
    uint16 epNum;
	uint32 txSize = ulSize;
    uint32 i;
	uint8 * buff = pucData;

    /* Check the arguments. */
    ASSERT(ulBase == USBD_0_BASE);
    ASSERT((usEndpoint == USB_EP_0) || (usEndpoint == USB_EP_1) ||
           (usEndpoint == USB_EP_2) || (usEndpoint == USB_EP_3) ||
           (usEndpoint == USB_EP_4) || (usEndpoint == USB_EP_5) ||
           (usEndpoint == USB_EP_6) || (usEndpoint == USB_EP_7) ||
           (usEndpoint == USB_EP_8) || (usEndpoint == USB_EP_9) ||
           (usEndpoint == USB_EP_10) || (usEndpoint == USB_EP_11) ||
           (usEndpoint == USB_EP_12) || (usEndpoint == USB_EP_13) ||
           (usEndpoint == USB_EP_14) || (usEndpoint == USB_EP_15));

    usEndpoint = usEndpoint >> 4;
	
	/* backup the EP number to restore later */
    /*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Statically defined non-null hardware register address" */
	epNum = usbd0Regs->epnum & USBD_EP_NUM_EP_NUM_MASK;
	
	/* Enable access to endpoint fifo */
	if (txSize > 64u) {
		txSize = 64u;
	}
	
	/* Setup epnum register for OUT EP */
	usbd0Regs->epnum = USBD_EP_NUM_EP_SEL | USBD_EP_DIR_IN | (usEndpoint & USBD_EP_NUM_EP_NUM_MASK);

	/* Dump the data */
	for (i=0u; i<txSize; i++) {
        /*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Statically defined non-null hardware register address" */
		/*SAFETYMCUSW 95 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
        /*SAFETYMCUSW 94 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
        *((volatile uint8 *)(&usbd0Regs->data)) = *buff;
		buff++;
	}
	
	/* Restore the epnum register */
	/* usbd0Regs->epnum = USBD_EP_NUM_EP_SEL | USBD_EP_DIR_IN | epNum; ### */
	
	usEndpoint = usEndpoint << 4;

    return(0);
}

/** ***************************************************************************
 *
 *  Starts the transfer of data from an endpoint's FIFO.
 * 
 *  \param ulBase specifies the USB module base address.
 *  \param usEndpoint is the endpoint to access.
 *  \param ulTransType Not used.
 * 
 *  This function will start the transfer of data from the FIFO for a given
 *  endpoint.
 * 
 *  \return This call will return 0 on success, or -1 if a transmission is
 *  already in progress.
 *
 *****************************************************************************/
uint32 USBEndpointDataSend(uint32 ulBase, uint16 usEndpoint, uint32 ulTransType)
{
	/* Check the arguments. */
    ASSERT(ulBase == USBD_0_BASE);
    ASSERT((usEndpoint == USB_EP_0) || (usEndpoint == USB_EP_1) ||
           (usEndpoint == USB_EP_2) || (usEndpoint == USB_EP_3) ||
           (usEndpoint == USB_EP_4) || (usEndpoint == USB_EP_5) ||
           (usEndpoint == USB_EP_6) || (usEndpoint == USB_EP_7) ||
           (usEndpoint == USB_EP_8) || (usEndpoint == USB_EP_9) ||
           (usEndpoint == USB_EP_10) || (usEndpoint == USB_EP_11) ||
           (usEndpoint == USB_EP_12) || (usEndpoint == USB_EP_13) ||
           (usEndpoint == USB_EP_14) || (usEndpoint == USB_EP_15));
    
    usEndpoint = usEndpoint >> 4;

	/* Setup the ep fifo */
    usbd0Regs->epnum = USBD_EP_NUM_EP_SEL | USBD_EP_DIR_IN | (usEndpoint & USBD_EP_NUM_EP_NUM_MASK);
	
	/* Enable the fifo */
    /*SAFETYMCUSW 184 S <INSPECTED> "Reason -  LDRA tool issue."*/
	USBD_REG_SET_ONE(usbd0Regs->ctrl, USBD_CTRL_SET_FIFO_EN);
	
	/* Remove fifo access */
    /*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Statically defined non-null hardware register address" */
    /*SAFETYMCUSW 184 S <INSPECTED> "Reason -  LDRA tool issue."*/
    /*SAFETYMCUSW 185 S <INSPECTED> "Reason -  LDRA tool issue."*/
	USBD_REG_BIT_CLR(usbd0Regs->epnum, USBD_EP_NUM_EP_SEL);
	
	usEndpoint = usEndpoint << 4;

    return(0);
}

/** ***************************************************************************
 *
 *  Resets the USB Device Controller
 * 
 *  \param void
 * 
 *  \return None.
 *
 *  \note Since the USB Device reset is handled by the host, this is a dummy 
 *  function & maintained for compatibility purpose.
 *
 *****************************************************************************/
void USBReset(void)
{
	return;
}


/** ***************************************************************************
 *
 *  Sets the FIFO configuration for an endpoint.
 * 
 *  \param ulBase specifies the USB module base address.
 *  \param usEndpoint is the endpoint to access.
 *  \param uFIFOAddress is the starting address for the FIFO.
 *  \param uFIFOSize is the size of the FIFO in bytes.
 *  \param uFlags specifies what information to set in the FIFO configuration.
 * 
 *  This function will set the starting FIFO RAM address and size of the FIFO
 *  for a given endpoint.  Endpoint zero does not have a dynamically
 *  configurable FIFO so this function should not be called for endpoint zero.
 *  The \e uFIFOSize parameter should be one of the values in the
 *  \b USB_FIFO_SZ_ values.  If the endpoint is going to use double buffering
 *  it should use the values with the \b _DB at the end of the value.  For
 *  example, use \b USB_FIFO_SZ_16_DB to configure an endpoint to have a 16
 *  byte double buffered FIFO.  If a double buffered FIFO is used, then the
 *  actual size of the FIFO will be twice the size indicated by the
 *  \e uFIFOSize parameter.  This means that the \b USB_FIFO_SZ_16_DB value
 *  will use 32 bytes of the USB controller's FIFO memory.
 * 
 *  The \e uFIFOAddress value should be a multiple of 8 bytes and directly
 *  indicates the starting address in the USB controller's FIFO RAM.  For
 *  example, a value of 64 indicates that the FIFO should start 64 bytes into
 *  the USB controller's FIFO memory.  The \e uFlags value specifies whether
 *  the endpoint's OUT or IN FIFO should be configured.  If in host mode, use
 *  \b USB_EP_HOST_OUT or \b USB_EP_HOST_IN, and if in device mode use
 *  \b USB_EP_DEV_OUT or \b USB_EP_DEV_IN.
 * 
 *  \return None.
 *
 *****************************************************************************/
void USBFIFOConfigSet(uint32 ulBase, uint32 usEndpoint,
                 	 	 uint32 uFIFOAddress, uint32 uFIFOSize,
                 	 	 uint16 uFlags)
{
	uint16 usSetVal;
	
    /*
     * Check the arguments.
     */
    ASSERT(ulBase == USBD_0_BASE);
    ASSERT((usEndpoint == USB_EP_1) || (usEndpoint == USB_EP_2) ||
           (usEndpoint == USB_EP_3) || (usEndpoint == USB_EP_4) ||
           (usEndpoint == USB_EP_5) || (usEndpoint == USB_EP_6) ||
           (usEndpoint == USB_EP_7) || (usEndpoint == USB_EP_8) ||
           (usEndpoint == USB_EP_9) || (usEndpoint == USB_EP_10) ||
           (usEndpoint == USB_EP_11) || (usEndpoint == USB_EP_12) ||
           (usEndpoint == USB_EP_13) || (usEndpoint == USB_EP_14) ||
           (usEndpoint == USB_EP_15));
	ASSERT(uFIFOSize <= USB_FIFO_SZ_1024);
	ASSERT(uFIFOAddress <= 0xFF);
	
	usEndpoint = usEndpoint >> 4;
	
	/* Retrieve the contents from the correct EP configuration register */
	if((uFlags & USB_EP_DEV_IN) != 0u) {
		usSetVal = usbd0Regs->epn_tx[usEndpoint-1];
    } else {
		usSetVal = usbd0Regs->epn_rx[usEndpoint-1];
    }
	/* Clear the fields that we will be setting */
	usSetVal &= ((uint16)~(USBD_TX_EP_VALID | USBD_TX_EP_SIZE | USBD_TX_EP_PTR));

    usSetVal = ((uint16)((uint16)uFIFOSize << 12u)) | (uint16)uFIFOAddress | (uint16)USBD_TX_EP_VALID;
	
	/* Set the updated values */
	if((uFlags & USB_EP_DEV_IN) != 0u) {
		usbd0Regs->epn_tx[usEndpoint-1] = usSetVal;
    } else {
		usbd0Regs->epn_rx[usEndpoint-1] = usSetVal;
    }

	/* Now we have the FIFO setup, enable it. But first select the EP */
	if ((uFlags & USB_EP_DEV_IN) != 0u) {
		/* Enable the fifo for the TX path (IN EP) */
        /*SAFETYMCUSW 184 S <INSPECTED> "Reason -  LDRA tool issue."*/
		USBD_REG_SET_ONE(usbd0Regs->epnum, (USBD_EP_NUM_EP_SEL | USBD_EP_DIR_IN | (uint16)usEndpoint));
		usbd0Regs->ctrl = USBD_CTRL_SET_FIFO_EN;
		/* 
         * TODO Need to att 6wait states here..(For now assuming tha the 
         * init delays are sufficient)
         */
	} else {
		/* Enable the fifo for the RX path (OUT EP) */
        /*SAFETYMCUSW 184 S <INSPECTED> "Reason -  LDRA tool issue."*/
		USBD_REG_SET_ONE(usbd0Regs->epnum, usEndpoint);
		usbd0Regs->ctrl = USBD_CTRL_SET_FIFO_EN;
	}

	usEndpoint = usEndpoint << 4;
	
	return;
}


/** ***************************************************************************
 *
 *  Gets the current configuration for an endpoint.
 * 
 *  \param ulBase specifies the USB module base address.
 *  \param usEndpoint is the endpoint to access.
 *  \param pulMaxPacketSize is a pointer which will be written with the
 *  maximum packet size for this endpoint.
 *  \param puFlags is a pointer which will be written with the current
 *  endpoint settings. On entry to the function, this pointer must contain
 *  either \b USB_EP_DEV_IN or \b USB_EP_DEV_OUT to indicate whether the IN or
 *  OUT endpoint is to be queried.
 * 
 *  This function will return the basic configuration for an endpoint in device
 *  mode. The values returned in \e *pulMaxPacketSize and \e *puFlags are
 *  equivalent to the \e ulMaxPacketSize and \e uFlags previously passed to
 *  USBDevEndpointConfigSet() for this endpoint.
 * 
 *  \note This function should only be called in device mode.
 * 
 *  \return None.
 *
 *****************************************************************************/
void
USBDevEndpointConfigGet(uint32     ulBase, 
                        uint16     usEndpoint,
                        uint32 *   pulMaxPacketSize,
                        uint32 *   puFlags)
{
	uint16 uRegister;

    /*
     * Check the arguments.
     */
    ASSERT(ulBase == USBD_0_BASE);
    ASSERT(pulMaxPacketSize && puFlags);
    ASSERT((usEndpoint == USB_EP_1) || (usEndpoint == USB_EP_2) ||
           (usEndpoint == USB_EP_3) || (usEndpoint == USB_EP_4) ||
           (usEndpoint == USB_EP_5) || (usEndpoint == USB_EP_6) ||
           (usEndpoint == USB_EP_7) || (usEndpoint == USB_EP_8) ||
           (usEndpoint == USB_EP_9) || (usEndpoint == USB_EP_10) ||
           (usEndpoint == USB_EP_11) || (usEndpoint == USB_EP_12) ||
           (usEndpoint == USB_EP_13) || (usEndpoint == USB_EP_14) ||
           (usEndpoint == USB_EP_15));
		   
	usEndpoint = usEndpoint >> 4;

    /*
     * Determine if a transmit or receive endpoint is being queried.
     */
    if((*puFlags & USB_EP_DEV_IN) != 0u)
    {
        /*
         * Clear the flags other than the direction bit.
         */
        *puFlags = USB_EP_DEV_IN;
        uRegister = usbd0Regs->epn_tx[usEndpoint-1];
    } else {
        /*
		 * Clear the flags other than the direction bit.
		 */
		*puFlags = USB_EP_DEV_OUT;
		uRegister = usbd0Regs->epn_rx[usEndpoint-1];
    }
	/*
	 * Are we in isochronous mode?
	 */
	if((uRegister & USBD_TX_EP_ISO) != 0u)
	{
        /*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Only valid non NULL input parameters are allowed in this API" */
		*puFlags |= USB_EP_MODE_ISOC;
	}
	else
	{
		/*
		 * The hardware doesn't differentiate between bulk, interrupt
		 * and control mode for the endpoint so we just set something
		 * that isn't isochronous.  This ensures that anyone modifying
		 * the returned flags in preparation for a call to
		 * USBDevEndpointConfigSet will not see an unexpected mode change.
		 * If they decode the returned mode, however, they may be in for
		 * a surprise.
		 */
		*puFlags |= USB_EP_MODE_BULK;
	}

	/*
	 * Get the maximum packet size & mode
	 */
	uRegister = (uRegister & USBD_TX_EP_SIZE) >> 12u;
	*pulMaxPacketSize = 8u * ((uint16)(1u << uRegister));
	
	usEndpoint = usEndpoint << 4u;

	return;
}


/** ***************************************************************************
 *
 *  Sets the configuration for an endpoint.
 * 
 *  \param ulBase specifies the USB module base address.
 *  \param usEndpoint is the endpoint to access.
 *  \param ulMaxPacketSize is the maximum packet size for this endpoint.
 *  \param uFlags are used to configure other endpoint settings.
 * 
 *  This function will set the basic configuration for an endpoint in device
 *  mode.  Endpoint zero does not have a dynamic configuration, so this
 *  function should not be called for endpoint zero.  The \e uFlags parameter
 *  determines some of the configuration while the other parameters provide the
 *  rest.
 * 
 *  The \b USB_EP_MODE_ flags define what the type is for the given endpoint.
 * 
 *  - \b USB_EP_MODE_CTRL is a control endpoint.
 *  - \b USB_EP_MODE_ISOC is an isochronous endpoint.
 *  - \b USB_EP_MODE_BULK is a bulk endpoint.
 *  - \b USB_EP_MODE_INT is an interrupt endpoint.
 * 
 * 
 *  \note This function should only be called in device mode.
 * 
 *  \return None.
 *
 *****************************************************************************/
void
USBDevEndpointConfigSet(uint32 ulBase, uint16 usEndpoint,
                        uint32 ulMaxPacketSize, uint32 uFlags)
{
	uint16 uSetVal;

    /*
     * Check the arguments.
     */
    ASSERT(ulBase == USBD_0_BASE);
    ASSERT((usEndpoint == USB_EP_1) || (usEndpoint == USB_EP_2) ||
           (usEndpoint == USB_EP_3) || (usEndpoint == USB_EP_4) ||
           (usEndpoint == USB_EP_5) || (usEndpoint == USB_EP_6) ||
           (usEndpoint == USB_EP_7) || (usEndpoint == USB_EP_8) ||
           (usEndpoint == USB_EP_9) || (usEndpoint == USB_EP_10) ||
           (usEndpoint == USB_EP_11) || (usEndpoint == USB_EP_12) ||
           (usEndpoint == USB_EP_13) || (usEndpoint == USB_EP_14) ||
           (usEndpoint == USB_EP_15));
	ASSERT(ulMaxPacketSize <= 0x255);

	usEndpoint = usEndpoint >> 4;
	
    /* Determine if a transmit or receive endpoint is being configured. */
    if((uFlags & USB_EP_DEV_IN) != 0u) {
    	uSetVal = usbd0Regs->epn_tx[usEndpoint-1];
    } else {
    	uSetVal = usbd0Regs->epn_rx[usEndpoint-1];
    }
	/*
     * Clear the fields that we configure/care (Note that EP bit defn., 
     * are same for RX & TX)
	 * Need to configure size here since the Get API expects this value
     */
	uSetVal &= ((uint16)~(USBD_TX_EP_ISO | USBD_TX_EP_SIZE));
	
	if ((uFlags & USBD_TX_EP_ISO) != 0u)
	{
		uSetVal |= USB_EP_MODE_ISOC;
	}	
	uSetVal |= (uint16)((uint16)ulMaxPacketSize << 12u);

	/* Update the correct EP configuration register */
    if((uFlags & USB_EP_DEV_IN) != 0u)
    {
    	usbd0Regs->epn_tx[usEndpoint-1] = uSetVal;
    } else {
    	usbd0Regs->epn_rx[usEndpoint-1] = uSetVal;
    }

	usEndpoint = usEndpoint << 4;
	return;
}

void
USBDevSetDevCfg(uint32 ulBase)
{
	/*
	 * Check the arguments.
	 */
	ASSERT(ulBase == USBD_0_BASE);

	/* Set the SYSCON2.DEV_CFG bit */
    /*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Statically defined non-null hardware register address" */
    /*SAFETYMCUSW 184 S <INSPECTED> "Reason -  LDRA tool issue."*/
	USBD_REG_SET_ONE(usbd0Regs->syscon2, USBD_SYSCON2_DEV_CFG);

}

void
USBDevClearDevCfg(uint32 ulBase)
{
	/*
	 * Check the arguments.
	 */
	ASSERT(ulBase == USBD_0_BASE);

	/* Set the SYSCON2.DEV_CFG bit */
    /*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Statically defined non-null hardware register address" */
    /*SAFETYMCUSW 184 S <INSPECTED> "Reason -  LDRA tool issue."*/
	USBD_REG_SET_ONE(usbd0Regs->syscon2, USBD_SYSCON2_CLR_CFG);

}

uint16 USBDevGetEPnStat(uint32 ulBase)
{
	/*
	 * Check the arguments.
	 */
	ASSERT(ulBase == USBD_0_BASE);
	return (usbd0Regs->epn_stat & (USBD_EPN_STAT_RX_IT_SRC | USBD_EPN_STAT_TX_IT_SRC));
}

void USBDevPullEnableDisable(uint32 ulBase, uint32 ulSet)
{
	/*
	 * Check the arguments.
	 */
	ASSERT(ulBase == USBD_0_BASE);

	if (ulSet != 0u) {
        /*SAFETYMCUSW 184 S <INSPECTED> "Reason -  LDRA tool issue."*/
		USBD_REG_BIT_SET(usbd0Regs->syscon1, USBD_SYSCON1_PULLUP_EN);
	} else {
        /*SAFETYMCUSW 184 S <INSPECTED> "Reason -  LDRA tool issue."*/
        /*SAFETYMCUSW 185 S <INSPECTED> "Reason -  LDRA tool issue."*/
		USBD_REG_BIT_CLR(usbd0Regs->syscon1,USBD_SYSCON1_PULLUP_EN);
	}
}
	

void USBIntStatusClear (uint16 uFlag) {
    /*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Statically defined non-null hardware register address" */
    /*SAFETYMCUSW 184 S <INSPECTED> "Reason -  LDRA tool issue."*/
	USBD_REG_SET_ONE(usbd0Regs->irqsrc, uFlag);
	return;
}

uint16 USBDevGetDevStat(uint32 ulBase)
{
	/* Check the arguments. */
	ASSERT(ulBase == USBD_0_BASE);

	return(usbd0Regs->dev_stat);
}

void USBDevCfgUnlock(uint32 ulBase)
{
	/* Check the arguments. */
	ASSERT(ulBase == USBD_0_BASE);

    /*SAFETYMCUSW 184 S <INSPECTED> "Reason -  LDRA tool issue."*/
    /*SAFETYMCUSW 185 S <INSPECTED> "Reason -  LDRA tool issue."*/
    /*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Statically defined non-null hardware register address" */
	USBD_REG_BIT_CLR(usbd0Regs->syscon1, USBD_SYSCON1_CFG_LOCK);
	return;
}

void USBDevCfgLock(uint32 ulBase)
{
	/* Check the arguments. */
	ASSERT(ulBase == USBD_0_BASE);

    /*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Statically defined non-null hardware register address" */
    /*SAFETYMCUSW 184 S <INSPECTED> "Reason -  LDRA tool issue."*/
	USBD_REG_BIT_SET(usbd0Regs->syscon1, USBD_SYSCON1_CFG_LOCK);
	return;
}
/** ***************************************************************************
 *
 * Close the Doxygen group.
 *  @}
 *
 *****************************************************************************/
