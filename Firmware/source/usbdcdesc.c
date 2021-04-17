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
 *  @file       usbcdesc.c
 *
 *  @brief      Config descriptor parsing functions.
 * 
 */

#include "sys_common.h"
#include "usb.h"
#include "usblib.h"
#include "usbdevice.h"

/******************************************************************************
 *
 * The functions in this file mirror the descriptor parsing APIs available
 * in usblib.h but parse config descriptors defined in terms of a list of
 * sections rather than as a single block of descriptor data.
 *
 *****************************************************************************/

/** ***************************************************************************
 *
 *  \ingroup device_api
 *  @{
 *
 *****************************************************************************/

/** ***************************************************************************
 *
 *  \internal
 * 
 *  Walk to the next descriptor after the supplied one within a section-based
 *  config descriptor.
 * 
 *  \param psConfig points to the header structure for the configuration
 *  descriptor which contains \e psDesc.
 *  \param pulSec points to a variable containing the section within \e
 *  psConfig which contains \e psDesc.
 *  \param psDesc points to the descriptor that we want to step past.
 * 
 *  This function walks forward one descriptor within a config descriptor.  The
 *  value returned is a pointer to the header of the next descriptor after the
 *  descriptor supplied in \e psDesc.  If the next descriptor is in the next
 *  section, \e *pulSec will be incremented accordlingly.
 * 
 *  \return Returns a pointer to the next descriptor in the config descrptor.
 *
 *****************************************************************************/
static tDescriptorHeader *
NextConfigDescGet(const tConfigHeader * psConfig, 
                  uint32 *              pulSec,
                  tDescriptorHeader *   psDesc)
{
    /*
     * Determine where the next descriptor after the supplied one should be
     * assuming it is within the current section.
     */
    /*SAFETYMCUSW 95 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    /*SAFETYMCUSW 94 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    /*SAFETYMCUSW 184 S <INSPECTED> "Reason -  LDRA tool issue."*/
    psDesc = NEXT_USB_DESCRIPTOR(psDesc);

    /*
     * Did we run off the end of the section?
     */
    /*SAFETYMCUSW 95 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    /*SAFETYMCUSW 94 S MR:11.4 <INSPECTED> "Reason -  Acceptable deviation."*/
    if((uint8 *)psDesc >= (psConfig->psSections[*pulSec]->pucData +
                  psConfig->psSections[*pulSec]->ucSize))
    {
        /*
         * Yes - move to the next section.
         */
        (*pulSec)++;

        /*
         * Are we still within the config descriptor?
         */
        if(*pulSec < psConfig->ucNumSections)
        {
            /*
             * Yes - the new descriptor is at the start of the new section.
             */
            psDesc =
                (tDescriptorHeader *)psConfig->psSections[*pulSec]->pucData;
        }
        else
        {
            /*
             * No - we ran off the end of the descriptor so return NULL.
             */
            psDesc = (tDescriptorHeader *)0;
        }
    }

    /*
     * Return the new descriptor pointer.
     */
    return(psDesc);
}

/** ***************************************************************************
 *
 *  \internal
 * 
 *  Returns a pointer to the n-th interface descriptor in a config descriptor
 *  with the supplied interface number.
 * 
 *  \param psConfig points to the header structure for the config descriptor
 *  that is to be searched.
 *  \param ucInterfaceNumber is the interface number of the descriptor that is
 *  being queried.
 *  \param ulIndex is the zero based index of the descriptor to return.
 *  \param pulSection points to storage which will be written with the index
 *  of the section containing the returned descriptor.
 * 
 *  This function returns a pointer to the n-th interface descriptor in the
 *  supplied configuration which has the requested interface number.  It may be
 *  used by a client to retrieve the descriptors for each alternate setting
 *  of a given interface within the configuration passed.
 * 
 *  \return Returns a pointer to the n-th interface descriptor with interface
 *  number as specified or NULL of this descriptor does not exist.
 *
 *****************************************************************************/
static tInterfaceDescriptor *
ConfigAlternateInterfaceGet(const tConfigHeader * psConfig,
                            uint8                 ucInterfaceNumber,
                            uint32                ulIndex,
                            uint32 *              pulSection)
{
    tDescriptorHeader * psDescCheck;
    uint32 ulCount;
    uint32 ulSec;

    /*
     * Set up for our descriptor counting loop.
     */
    psDescCheck = (tDescriptorHeader *)psConfig->psSections[0]->pucData;
    ulCount = 0u;
    ulSec = 0u;

    /*
     * Keep looking through the supplied data until we reach the end.
     */
    while(psDescCheck != NULL)
    {
        /*
         * Does this descriptor match the type passed (if a specific type
         * has been specified)?
         */
        if((psDescCheck->bDescriptorType == USB_DTYPE_INTERFACE) &&
            (((tInterfaceDescriptor *)psDescCheck)->bInterfaceNumber ==
             ucInterfaceNumber))
        {
            /*
             * This is an interface descriptor for interface ucInterfaceNumber.
             * Determine if this is the n-th one we have found and, if so,
             * return its pointer.
             */
            if(ulCount == ulIndex)
            {
                /*
                 * Found it - return the pointer and section number.
                 */
                *pulSection = ulSec;
                return((tInterfaceDescriptor *)psDescCheck);
            }

            /*
             * Increment our count of matching descriptors found and go back
             * to look for another since we have not yet reached the n-th
             * match.
             */
            ulCount++;
        }

        /*
         * Move on to the next descriptor.
         */
        psDescCheck = NextConfigDescGet(psConfig, &ulSec, psDescCheck);
    }

    /*
     * If we drop out the end of the loop, we did not find the requested
     * descriptor so return NULL.
     */
    return((tInterfaceDescriptor *)0);
}

/** ***************************************************************************
 *
 *  \internal
 * 
 *  Determines the total length of a config descriptor defined in terms of a
 *  collection of concatenated sections.
 * 
 *  \param psConfig points to the header structure for the configuration
 *  descriptor whose size is to be determined.
 * 
 *  \return Returns the number of bytes in the config descriptor will result
 *  from concatenating the required sections.
 *
 *****************************************************************************/
uint32
USBDCDConfigDescGetSize(const tConfigHeader *psConfig)
{
    uint32 ulLoop;
    uint32 ulLen;

    ulLen = 0u;

    /*
     * Determine the size of the whole descriptor by adding the sizes of
     * each section which will be concatenated to produce it.
     */
    for(ulLoop = 0u; ulLoop < psConfig->ucNumSections; ulLoop++)
    {
        ulLen += psConfig->psSections[ulLoop]->ucSize;
    }

    return(ulLen);
}

/** ***************************************************************************
 *
 *  \internal
 * 
 *  Determines the number of individual descriptors of a particular type within
 *  a supplied configuration descriptor.
 * 
 *  \param psConfig points to the header structure for the configuration
 *  descriptor that is to be searched.
 *  \param ulType identifies the type of descriptor that is to be counted.  If
 *  the value is \b USB_DESC_ANY, the function returns the total number of
 *  descriptors regardless of type.
 * 
 *  This function can be used to count the number of descriptors of a
 *  particular type within a configuration descriptor.  The caller can provide
 *  a specific type value which the function matches against the second byte
 *  of each descriptor or, alternatively, can specify \b USB_DESC_ANY to have
 *  the function count all descriptors regardless of their type.
 * 
 *  The search performed by this function traverses through the list of
 *  sections comprising the configuration descriptor.  Note that the similar
 *  top-level function, USBDescGetNum(), searches through a single, contiguous
 *  block of data to perform the same enumeration.
 * 
 *  \return Returns the number of descriptors found in the supplied block of
 *  data.
 *
 *****************************************************************************/
uint32
USBDCDConfigDescGetNum(const tConfigHeader *psConfig, uint32 ulType)
{
    uint32 ulSection;
    uint32 ulNumDescs;

    /*
     * Initialize our counts.
     */
    ulNumDescs = 0u;

    /*
     * Determine the number of descriptors of the given type in each of the
     * sections comprising the config descriptor.  Note that this assumes each
     * section contains only whole descriptors!
     */
    for(ulSection = 0u; ulSection < (uint32)psConfig->ucNumSections;
        ulSection++)
    {
        ulNumDescs += USBDescGetNum(
                (tDescriptorHeader *)psConfig->psSections[ulSection]->pucData,
                (uint32)(psConfig->psSections[ulSection]->ucSize),
                ulType);
    }

    return(ulNumDescs);
}

/** ***************************************************************************
 *
 *  \internal
 * 
 *  Finds the n-th descriptor of a particular type within the supplied
 *  configuration descriptor.
 * 
 *  \param psConfig points to the header structure for the configuration
 *  descriptor that is to be searched.
 *  \param ulType identifies the type of descriptor that is to be found.  If
 *  the value is \b USB_DESC_ANY, the function returns a pointer to the n-th
 *  descriptor regardless of type.
 *  \param ulIndex is the zero based index of the descriptor whose pointer is
 *  to be returned.  For example, passing value 1 in \e ulIndex returns the
 *  second matching descriptor.
 *  \param pulSection points to storage which will receive the section index
 *  containing the requested descriptor.
 * 
 *  Return a pointer to the n-th descriptor of a particular type found in the
 *  configuration descriptor passed.
 * 
 *  The search performed by this function traverses through the list of
 *  sections comprising the configuration descriptor.  Note that the similar
 *  top-level function, USBDescGet(), searches through a single, contiguous
 *  block of data to perform the same enumeration.
 * 
 *  \return Returns a pointer to the header of the required descriptor if
 *  found or NULL otherwise.
 *
 *****************************************************************************/
tDescriptorHeader *
USBDCDConfigDescGet(const tConfigHeader * psConfig, uint32 ulType,
                    uint32 ulIndex, uint32 * pulSection)
{
    uint32 ulSection;
    uint32 ulTotalDescs;
    uint32 ulNumDescs;

    /*
     * Initialize our counts.
     */
    ulTotalDescs = 0u;

    /*
     * Determine the number of descriptors of the given type in each of the
     * sections comprising the config descriptor.  This allows us to determine
     * which section contains the descriptor we are being asked for.
     */
    for(ulSection = 0u; ulSection < (uint32)psConfig->ucNumSections;
        ulSection++)
    {
        /*
         * How many descriptors of the requested type exist in this section?
         */
        ulNumDescs = USBDescGetNum(
                (tDescriptorHeader *)psConfig->psSections[ulSection]->pucData,
                (uint32)(psConfig->psSections[ulSection]->ucSize),
                ulType);

        /*
         * Does this section contain the descriptor whose index we are looking
         * for?
         */
        if((ulTotalDescs + ulNumDescs) > ulIndex)
        {
            /*
             * We know the requested descriptor exists in the current
             * block so write the section number to the caller's storage.
             */
            *pulSection = ulSection;

            /*
             * Now find the actual descriptor requested and return its pointer.
             */
            return(USBDescGet(
                 (tDescriptorHeader *)psConfig->psSections[ulSection]->pucData,
                 (uint32)(psConfig->psSections[ulSection]->ucSize),
                 ulType,
                 ulIndex - ulTotalDescs));
        }

        /*
         * We have not found the required descriptor yet.  Update our running
         * count of the number of type matches found so far then move on to
         * the next section.
         */
        ulTotalDescs += ulNumDescs;
    }

    /*
     * If we drop out of the loop, we can't find the requested descriptor
     * so return NULL.
     */
    return((tDescriptorHeader *)0);
}

/** ***************************************************************************
 *
 *  \internal
 * 
 *  Determines the number of different alternate configurations for a given
 *  interface within a config descriptor.
 * 
 *  \param psConfig points to the header structure for the configuration
 *  descriptor that is to be searched.
 *  \param ucInterfaceNumber is the interface number for which the number of
 *  alternate configurations is to be counted.
 * 
 *  This function can be used to count the number of alternate settings for a
 *  specific interface within a configuration.
 * 
 *  The search performed by this function traverses through the list of
 *  sections comprising the configuration descriptor.  Note that the similar
 *  top-level function, USBDescGetNumAlternateInterfaces(), searches through
 *  a single, contiguous block of data to perform the same enumeration.
 * 
 *  \return Returns the number of alternate versions of the specified interface
 *  or 0 if the interface number supplied cannot be found in the config
 *  descriptor.
 *
 *****************************************************************************/
uint32
USBDCDConfigGetNumAlternateInterfaces(const tConfigHeader *psConfig,
                                      uint8 ucInterfaceNumber)
{
    tDescriptorHeader * psDescCheck;
    uint32 ulCount;
    uint32 ulSec;

    /*
     * Set up for our descriptor counting loop.
     */
    psDescCheck = (tDescriptorHeader *)psConfig->psSections[0]->pucData;
    ulSec = 0u;
    ulCount = 0u;

    /*
     * Keep looking through the supplied data until we reach the end.
     */
    while(psDescCheck != NULL)
    {
        /*
         * Is this an interface descriptor with the required interface number?
         */
        if((psDescCheck->bDescriptorType == USB_DTYPE_INTERFACE) &&
            (((tInterfaceDescriptor *)psDescCheck)->bInterfaceNumber ==
             ucInterfaceNumber))
        {
            /*
             * Yes - increment our count.
             */
            ulCount++;
        }

        /*
         * Move on to the next descriptor.
         */
        psDescCheck = NextConfigDescGet(psConfig, &ulSec, psDescCheck);
    }

    /*
     * Return the descriptor count to the caller.
     */
    return(ulCount);
}

/** ***************************************************************************
 *
 *  \internal
 * 
 *  Returns a pointer to the n-th interface descriptor in a configuration
 *  descriptor that applies to the supplied alternate setting number.
 * 
 *  \param psConfig points to the header structure for the configuration
 *  descriptor that is to be searched.
 *  \param ulIndex is the zero based index of the interface that is to be
 *  found.  If \e ulAlt is set to a value other than \b USB_DESC_ANY, this will
 *  be equivalent to the interface number being searched for.
 *  \param ulAlt is the alternate setting number which is to be
 *  searched for.  If this value is \b USB_DESC_ANY, the alternate setting
 *  is ignored and all interface descriptors are considered in the search.
 *  \param pulSection points to storage which will receive the index of the
 *  config descriptor section which contains the requested interface
 *  descriptor.
 * 
 *  Return a pointer to the n-th interface descriptor found in the supplied
 *  configuration descriptor.  If \e ulAlt is not \b USB_DESC_ANY, only
 *  interface descriptors which are part of the supplied alternate setting are
 *  considered in the search otherwise all interface descriptors are
 *  considered.
 * 
 *  Note that, although alternate settings can be applied on an interface-by-
 *  interface basis, the number of interfaces offered is fixed for a given
 *  config descriptor.  Hence, this function will correctly find the unique
 *  interface descriptor for that interface's alternate setting number \e
 *  ulAlt if \e ulIndex is set to the required interface number and \e ulAlt
 *  is set to a valid alternate setting number for that interface.
 * 
 *  The search performed by this function traverses through the list of
 *  sections comprising the configuration descriptor.  Note that the similar
 *  top-level function, USBDescGetInterface(), searches through a single,
 *  contiguous block of data to perform the same enumeration.
 * 
 *  \return Returns a pointer to the required interface descriptor if
 *  found or NULL otherwise.
 *
 *****************************************************************************/
tInterfaceDescriptor *
USBDCDConfigGetInterface(const tConfigHeader * psConfig, uint32 ulIndex,
                         uint32 ulAlt, uint32 * pulSection)
{
    /*
     * If we are being told to ignore the alternate configuration, this boils
     * down to a very simple query.
     */
    if(ulAlt == USB_DESC_ANY)
    {
        /*
         * Return the ulIndex-th interface descriptor we find in the
         * configuration descriptor.
         */
        return((tInterfaceDescriptor *)USBDCDConfigDescGet(psConfig,
                                                           USB_DTYPE_INTERFACE,
                                                           ulIndex,
                                                           pulSection));
    }
    else
    {
        /*
         * In this case, a specific alternate setting number is required.
         * Given that interface numbers are zero based indices, we can
         * pass the supplied ulIndex parameter directly as the interface
         * number to USBDescGetAlternateInterface to retrieve the requested
         * interface descriptor pointer.
         */
        return(ConfigAlternateInterfaceGet(psConfig, (uint8)ulIndex, ulAlt,
                                           pulSection));
    }
}

/** ***************************************************************************
 *
 *  \internal
 * 
 *  Return a pointer to the n-th endpoint descriptor in a particular interface
 *  within a configuration descriptor.
 * 
 *  \param psConfig points to the header structure for the configuration
 *  descriptor that is to be searched.
 *  \param ulInterfaceNumber is the interface number whose endpoint is to be
 *  found.
 *  \param ulAltCfg is the alternate setting number which is to be searched
 *  for.  This must be a valid alternate setting number for the requested
 *  interface.
 *  \param ulIndex is the zero based index of the endpoint that is to be
 *  found within the appropriate alternate setting for the interface.
 * 
 *  Return a pointer to the n-th endpoint descriptor found in the supplied
 *  interface descriptor.  If the \e ulIndex parameter is invalid (greater
 *  than or equal to the bNumEndpoints field of the interface descriptor) or
 *  the endpoint cannot be found within \e ulSize bytes of the interface
 *  descriptor pointer, the function will return NULL.
 * 
 *  Note that, although the USB 2.0 specification states that endpoint
 *  descriptors must follow the interface descriptor that they relate to, it
 *  also states that device specific descriptors should follow any standard
 *  descriptor that they relate to.  As a result, we cannot assume that each
 *  interface descriptor will be followed by nothing but an ordered list of
 *  its own endpoints and, hence, the function needs to be provided ulSize to
 *  limit the search range.
 * 
 *  The search performed by this function traverses through the list of
 *  sections comprising the configuration descriptor.  Note that the similar
 *  top-level function, USBDescGetInterfaceEndpoint(), searches through a
 *  single, contiguous block of data to perform the same enumeration.
 * 
 *  \return Returns a pointer to the requested endpoint descriptor if
 *  found or NULL otherwise.
 *
 *****************************************************************************/
/*SAFETYMCUSW 102 S  MR:8.3 <INSPECTED> "Reason -  LDRA tool issue."*/
tEndpointDescriptor *
USBDCDConfigGetInterfaceEndpoint(const tConfigHeader *psConfig,
                                 uint32 ulInterfaceNumber,
                                 uint32 ulAltCfg, uint32 ulIndex)
{
    tInterfaceDescriptor * psInterface;
    tDescriptorHeader *    psEndpoint;
    uint32 ulSection;
    uint32 ulCount;

    /*
     * Find the requested interface descriptor.
     */
    psInterface = USBDCDConfigGetInterface(psConfig, ulInterfaceNumber,
                                           ulAltCfg, &ulSection);

    /*
     * Did we find the requested interface?
     */
    if(psInterface != NULL)
    {
        /*
         * Is the index passed valid?
         */
        if(ulIndex >= psInterface->bNumEndpoints)
        {
            /*
             * It's out of bounds so return a NULL.
             */
            return((tEndpointDescriptor *)0);
        }
        else
        {
            /*
             * Endpoint index is valid so find the descriptor.  We start from
             * the interface descriptor and look for following endpoint
             * descriptors.
             */
            ulCount = 0u;
            psEndpoint = (tDescriptorHeader *)psInterface;

            while(psEndpoint != NULL)
            {
                if(psEndpoint->bDescriptorType == USB_DTYPE_ENDPOINT)
                {
                    /*
                     * We found an endpoint descriptor.  Have we reached the
                     * one we want?
                     */
                    if(ulCount == ulIndex)
                    {
                        /*
                         * Yes - return the descriptor pointer to the caller.
                         */
                        return((tEndpointDescriptor *)psEndpoint);
                    }

                    /*
                     * Move on to look for the next endpoint.
                     */
                    ulCount++;
                }

                /*
                 * Move to the next descriptor.
                 */
                psEndpoint = NextConfigDescGet(psConfig, &ulSection,
                                               psEndpoint);
            }
        }
    }

    /*
     * We couldn't find the requested interface or we got to the end of the
     * descriptor without finding the requested endpoint.
     */
    return((tEndpointDescriptor *)0);

}

/** ***************************************************************************
 *
 *  Close the Doxygen group.
 *  @}
 *
 *****************************************************************************/
