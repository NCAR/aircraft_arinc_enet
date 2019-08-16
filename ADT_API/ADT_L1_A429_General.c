/******************************************************************************
 * FILE:			ADT_L1_A429_General.c
 *
 * Copyright (c) 2007-2016, Alta Data Technologies LLC (ADT), All Rights Reserved.
 * Use of this software is subject to the ADT Software License (latest
 * revision), US Government or local laws and the ADT Terms and Conditions 
 * of Sale (latest revision).
 *
 * DESCRIPTION:
 *	Source file for Layer 1 API.
 *	Contains A429 general functions.
 *
 *****************************************************************************/
/*! \file ADT_L1_A429_General.c  
 *  \brief Source file containing A429 General functions 
 */
#include "ADT_L1.h"

/* Leap year macros */
#define ISLEAPYEAR(year)        (!((year) % 4) && (((year) % 100) || !((year) % 400)))
#define YEARSIZEDAYS(year)      (ISLEAPYEAR(year) ? 366 : 365)


#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
  FUNCTION:		ADT_L1_A429_InitDefault
 *****************************************************************************/
/*! \brief Initializes a A429 device with the default settings
 *
 * This function initializes an A429 device and allocates interrupt queue.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param numIQEntries is the number of interrupt queue entries to allocate.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid number of IQ entries
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_InitDefault(ADT_L0_UINT32 devID, ADT_L0_UINT32 numIQEntries) {
	ADT_L0_UINT32 result = ADT_SUCCESS;

	result = ADT_L1_InitDevice(devID, 0);

	if (result == ADT_SUCCESS) {
		result = ADT_L1_A429_InitDevice(devID, numIQEntries);
	}

	return( result );
}

/******************************************************************************
  FUNCTION:		ADT_L1_A429_InitDefault_ExtendedOptions
 *****************************************************************************/
/*! \brief Initializes a A429 device with the default settings
 *
 * This function initializes an A429 device for standard ARINC-429 encoding/decoding
 * setup and allocates interrupt queue.
 *
 * Startup Options are added to force initialization, skip mem testing (for fast
 * startups and to force hard PE Device Level Reset of PE Device Registers.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param numIQEntries is the number of interrupt queue entries to allocate.
 * @param startupOptions control the following initialization options:
 * ADT_L1_API_DEVICEINIT_FORCEINIT				0x00000001
 * ADT_L1_API_DEVICEINIT_NOMEMTEST				0x00000002
 * ADT_L1_API_DEVICEINIT_ROOTPERESET			0x00000004
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid number of IQ entries
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_InitDefault_ExtendedOptions(ADT_L0_UINT32 devID, ADT_L0_UINT32 numIQEntries, ADT_L0_UINT32 startupOptions) {
	ADT_L0_UINT32 result = ADT_SUCCESS;

	result = ADT_L1_InitDevice(devID, startupOptions);

	if (result == ADT_SUCCESS) {
		result = ADT_L1_A429_InitDevice(devID, numIQEntries);
	}

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_A429_InitDevice
 *****************************************************************************/
/*! \brief Initializes an A429 Device 
 *
 * This function initializes an A429 Device and allocates interrupt queue.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param numIQEntries is the number of interrupt queue entries to allocate.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid number of IQ entries
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_InitDevice(ADT_L0_UINT32 devID, ADT_L0_UINT32 numIQEntries) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 pThisIQEntry, pFirstIQEntry, pLastIQEntry, i, data;

	/* Make sure this is an A429 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Initialize PE registers */
	/* Initialize PE CSR to ZERO with CHANNEL RESET */
	/* data = ADT_L1_A429_PECSR_DEVICE_RESET; */

	/* RSW 13 NOV 13 - Removed PE RESET - was causing lockup with PC104P-MA4 A429 device when using Magma */
	/* Initialize PE CSR to ZERO */
	data = 0;
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_PE_ROOT_CSR, &data, 1);

	/* Clear registers above PE root regs (ADT_L1_A429_ROOT_RX_REGS through ADT_L1_A429_USER_MEM_START) - Memory test will clear the rest of memory */
	data = 0x00000000;
	for (i=ADT_L1_A429_ROOT_RX_REGS; i<ADT_L1_A429_USER_MEM_START; i += 4) {
		result = ADT_L0_WriteMem32(devID, channelRegOffset + i, &data, 1);
	}

	/* Setup interrupt queue */
	if (numIQEntries > 0) {

		/* Allocate the first IQ Entry */
		result = ADT_L1_MemoryAlloc(devID, ADT_L1_A429_IQ_ENTRY_SIZE, &pFirstIQEntry);
		if (result != ADT_SUCCESS) return( result );

		/* Set the NEXT pointer to point to itself */
		result = ADT_L0_WriteMem32(devID, channelRegOffset + pFirstIQEntry, &pFirstIQEntry, 1);

		/* Set the START, CURRENT, and API IQ pointers to this IQ Entry */
		result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_IQ_START_IQP, &pFirstIQEntry, 1);
		result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_IQ_CURR_IQP, &pFirstIQEntry, 1);
		result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_IQ_RESV_API, &pFirstIQEntry, 1);

		/* Clear the Sequence Number in the root interrupt registers */
		i = 0;
		result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_IQ_SEQNUM, &i, 1);

		/* Allocate and link any additional IQ Entries */
		pThisIQEntry = pFirstIQEntry;
		for (i=1; i<numIQEntries; i++) {
			pLastIQEntry = pThisIQEntry;

			/* Allocate the IQ Entry */
			result = ADT_L1_MemoryAlloc(devID, ADT_L1_A429_IQ_ENTRY_SIZE, &pThisIQEntry);
			if (result != ADT_SUCCESS) return( result );

			/* Link the previous IQ Entry to this one */
			result = ADT_L0_WriteMem32(devID, channelRegOffset + pLastIQEntry, &pThisIQEntry, 1);

			/* Set the NEXT pointer to point to the first IQ Entry */
			result = ADT_L0_WriteMem32(devID, channelRegOffset + pThisIQEntry, &pFirstIQEntry, 1);
		}

	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_A429_GetConfig
 *****************************************************************************/
/*! \brief Gets the A429 TX-RX channel configuration for a device. 
 *
 * This function gets the A429 TX-RX channel configuration for a device.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param pChanConfig is the TX-RX Channel Configuration.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_GetConfig(ADT_L0_UINT32 devID, ADT_L0_UINT32 *pChanConfig) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* RX-TX Channel Selection */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_A429_PE_TXRX_CHANCONFIG, pChanConfig, 1);

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_A429_GetPEInfo
 *****************************************************************************/
/*! \brief Gets A429 PE ID and Version information 
 *
 * This function gets A429 PE ID and Version information.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param peInfo is a pointer to store the PE ID and Version.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_GetPEInfo(ADT_L0_UINT32 devID, ADT_L0_UINT32 *peInfo) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;

	/* Make sure this is an A429 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_A429_PE_ROOT_IDVER, peInfo, 1);

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_A429_TimeSet
 *****************************************************************************/
/*! \brief Sets the time tag value for the device. 
 *
 * This function sets the time tag value for the device.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param timeHigh is the upper 32-bits of the 64-bit time tag value.
 * @param timeLow is the lower 32-bits of the 64-bit time tag value.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_TimeSet(ADT_L0_UINT32 devID, ADT_L0_UINT32 timeHigh, ADT_L0_UINT32 timeLow) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, data;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Write the time value */
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_PE_TIMEHIGH, &timeHigh, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_PE_TIMELOW, &timeLow, 1);

	/* Write to the PE CSR to set the "Set Time Tag" bit (bit 10) */
	data = 0;
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_A429_PE_ROOT_CSR, &data, 1);
	data |= ADT_L1_A429_PECSR_SETTT;
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_PE_ROOT_CSR, &data, 1);

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_A429_TimeGet
 *****************************************************************************/
/*! \brief Gets the time tag value for the device. 
 *
 * This function gets the time tag value for the device.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param pTimeHigh is a pointer to store the upper 32-bits of the 64-bit time tag value.
 * @param pTimeLow is a pointer to store the lower 32-bits of the 64-bit time tag value.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_TIMEOUT - Timeout waiting for CSR bit to clear
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_TimeGet(ADT_L0_UINT32 devID, ADT_L0_UINT32 *pTimeHigh, ADT_L0_UINT32 *pTimeLow) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, data;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Write to the PE CSR to set the "Read Time Tag" bit (bit 11) */
	data = 0;
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_A429_PE_ROOT_CSR, &data, 1);
	data |= ADT_L1_A429_PECSR_READTT;
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_PE_ROOT_CSR, &data, 1);

	/* Read the time value */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_A429_PE_TIMEHIGH, pTimeHigh, 1);
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_A429_PE_TIMELOW, pTimeLow, 1);

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_A429_TimeClear
 *****************************************************************************/
/*! \brief Clears the time tag value for the device. 
 *
 * This function clears the time tag value for the device.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_TIMEOUT - Timeout waiting for CSR bit to clear
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_TimeClear(ADT_L0_UINT32 devID) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, data;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Write to the PE CSR to set the "Zero Time Tag" bit */
	data = 0;
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_A429_PE_ROOT_CSR, &data, 1);
	data |= ADT_L1_A429_PECSR_ZEROTT;
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_PE_ROOT_CSR, &data, 1);

	return( result );
}




/******************************************************************************
  IN v2.6.5.2 WE CHANGED TIME FORMAT TO INCLUDE LEAP YEARS.  THE #if 0 BLOCK
  IS THE OLD CODE THAT DID NOT INCLUDE LEAP YEAR.
 *****************************************************************************/
#if 0
/******************************************************************************
  FUNCTION:		ADT_L1_A429_IrigLatchedTimeGet
 *****************************************************************************/
/*! \brief Gets the latched IRIG and internal time value for the bank. 
 *
 * This function gets the latched IRIG and internal time value for the bank.
 * The latched IRIG time is in IRIG BCD format.
 * The latched internal time is in 64-bit binary format (not BCD).
 * The delta time is the difference between the IRIG time (converted from BCD 
 * to binary) and the internal time.
 * Add the delta time to internal time values (like the time stamp on a CDP) to
 * convert them to binary IRIG time.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param pIrigTimeHigh is a pointer to store the upper 32-bits of the 64-bit IRIG BCD time (latched).
 * @param pIrigTimeLow is a pointer to store the lower 32-bits of the 64-bit IRIG BCD time (latched).
 * @param pIntTimeHigh is a pointer to store the upper 32-bits of the 64-bit internal time (latched).
 * @param pIntTimeLow is a pointer to store the lower 32-bits of the 64-bit internal time (latched).
 * @param pDeltaTimeHigh is a pointer to store the upper 32-bits of the 64-bit delta time.
 * @param pDeltaTimeLow is a pointer to store the lower 32-bits of the 64-bit delta time.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_TIMEOUT - Timeout waiting for CSR bit to clear
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_IrigLatchedTimeGet(ADT_L0_UINT32 devID, 
															  ADT_L0_UINT32 *pIrigTimeHigh, ADT_L0_UINT32 *pIrigTimeLow, 
															  ADT_L0_UINT32 *pIntTimeHigh, ADT_L0_UINT32 *pIntTimeLow, 
															  ADT_L0_UINT32 *pDeltaTimeHigh, ADT_L0_UINT32 *pDeltaTimeLow) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, data;
	ADT_L0_UINT32 tempIrigTimeHigh, tempIrigTimeLow;
	long long unsigned int time64_Irig_sec, time64_Irig_20ns, time64_Internal, time64_Delta;
	long long unsigned int year, day, hour, minute, second;

	tempIrigTimeHigh = tempIrigTimeLow = 0;
	time64_Irig_sec = time64_Irig_20ns = time64_Internal = time64_Delta = 0;
	year = day = hour = minute = second = 0;

	/* Make sure this is an A429 bank */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Write to the PE CSR to set the "Read IRIG Time" bit (bit 8) */
	data = 0;
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_A429_PE_ROOT_CSR, &data, 1);
	data |= ADT_L1_1553_PECSR_RDIRIGTM;
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_PE_ROOT_CSR, &data, 1);

	/* Read the latched internal time value */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_A429_PE_TIMEHIGH, pIntTimeHigh, 1);
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_A429_PE_TIMELOW, pIntTimeLow, 1);
	time64_Internal = ((long long unsigned int)*pIntTimeHigh << 32) | (long long unsigned int) *pIntTimeLow;

	/* Read the latched IRIG time value */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_A429_PE_IRIGTIMEHIGH, &tempIrigTimeHigh, 1);
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_A429_PE_IRIGTIMELOW, &tempIrigTimeLow, 1);

	/* Convert BCD IRIG time to 64-bit binary equivalent */
	year = (((tempIrigTimeHigh & 0x000F0000) >> 16) * 10) + ((tempIrigTimeHigh & 0x0000F000) >> 12);
	day = (((tempIrigTimeHigh & 0x00000F00) >> 8) * 100) + (((tempIrigTimeHigh & 0x000000F0) >> 4) * 10) + (tempIrigTimeHigh & 0x0000000F);
	hour = (((tempIrigTimeLow & 0x00F00000) >> 20) * 10) + ((tempIrigTimeLow & 0x000F0000) >> 16);
	minute = (((tempIrigTimeLow & 0x0000F000) >> 12) * 10) + ((tempIrigTimeLow & 0x00000F00) >> 8);
	second =  (((tempIrigTimeLow & 0x000000F0) >> 4) * 10) + (tempIrigTimeLow & 0x0000000F);

	second++;  /* Correct for one second lag in IRIG time latched by firmware - PREVIOUS one-second sync */

	time64_Irig_sec = year * 60 * 60 * 24 * 365; 
	time64_Irig_sec += day * 60 * 60 * 24; 
	time64_Irig_sec += hour * 60 * 60;
	time64_Irig_sec += minute * 60;
	time64_Irig_sec += second;

	/* Convert IRIG time from SECONDS to 20ns LSB to calculate delta time */
	time64_Irig_20ns = time64_Irig_sec * 50000000;

	/* Calc IRIG-PE Time Delta */
	time64_Delta = time64_Irig_20ns - time64_Internal;
	*pDeltaTimeHigh = (ADT_L0_UINT32)(time64_Delta >> 32);
	*pDeltaTimeLow = (ADT_L0_UINT32)(time64_Delta & 0x00000000FFFFFFFF);

	/* Convert the IRIG time in seconds, which has been corrected for 1-second lag, back to BCD time */
	/* THIS HAS PROBLEMS FOR DAY 365 or 366:
	year = time64_Irig_sec / (60 * 60 * 24 * 365);		time64_Irig_sec -= year * 60 * 60 * 24 * 365;
	day = time64_Irig_sec / (60 * 60 * 24);				time64_Irig_sec -= day * 60 * 60 * 24;
	hour = time64_Irig_sec / (60 * 60);					time64_Irig_sec -= hour * 60 * 60;
	minute = time64_Irig_sec / 60;						time64_Irig_sec -= minute * 60;
	second = time64_Irig_sec;
	*/
	/* Do this instead: */
	if (second == 60)
	{
		second = 0;
		minute++;
		if (minute == 60)
		{
			minute = 0;
			hour++;
			if (hour == 24)
			{
				hour = 0;
				day++;
				/* We do not rollover days to year so that leap year (day 366) is ok */
			}
		}
	}
	/********************/

	tempIrigTimeHigh = tempIrigTimeLow = 0;

	tempIrigTimeHigh |= ((ADT_L0_UINT32)(year / 10) & 0xF) << 16;	year = year - (year / 10) * 10;
	tempIrigTimeHigh |= ((ADT_L0_UINT32) year & 0xF) << 12;

	tempIrigTimeHigh |= ((ADT_L0_UINT32)(day / 100) & 0xF) << 8;	day = day - (day / 100) * 100;
	tempIrigTimeHigh |= ((ADT_L0_UINT32)(day / 10) & 0xF) << 4;		day = day - (day / 10) * 10;
	tempIrigTimeHigh |= ((ADT_L0_UINT32) day & 0xF);

	tempIrigTimeLow |= ((ADT_L0_UINT32)(hour / 10) & 0xF) << 20;	hour = hour - (hour / 10) * 10;
	tempIrigTimeLow |= ((ADT_L0_UINT32) hour & 0xF) << 16;

	tempIrigTimeLow |= ((ADT_L0_UINT32)(minute / 10) & 0xF) << 12;	minute = minute - (minute / 10) * 10;
	tempIrigTimeLow |= ((ADT_L0_UINT32) minute & 0xF) << 8;

	tempIrigTimeLow |= ((ADT_L0_UINT32)(second / 10) & 0xF) << 4;	second = second - (second / 10) * 10;
	tempIrigTimeLow |= ((ADT_L0_UINT32) second & 0xF);

	*pIrigTimeHigh = tempIrigTimeHigh;
	*pIrigTimeLow = tempIrigTimeLow;

	return( result );
}
#else
/******************************************************************************
  IN v2.6.5.2 WE CHANGED TIME FORMAT TO INCLUDE LEAP YEARS.  THE #else BLOCK
  IS THE NEW CODE THAT DOES INCLUDE LEAP YEAR.
 *****************************************************************************/

/******************************************************************************
  FUNCTION:		ADT_L1_A429_IrigLatchedTimeGet
 *****************************************************************************/
/*! \brief Gets the latched IRIG and internal time value for the channel. 
 *
 * This function gets the latched IRIG and internal time value for the channel.
 * The latched IRIG time is in IRIG BCD format.
 * The latched internal time is in 64-bit binary format (not BCD).
 * The delta time is the difference between the IRIG time (converted from BCD 
 * to binary) and the internal time.
 * Add the delta time to internal time values (like the time stamp on an RxP) to
 * convert them to binary IRIG time.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param pIrigTimeHigh is a pointer to store the upper 32-bits of the 64-bit IRIG BCD time (latched).
 * @param pIrigTimeLow is a pointer to store the lower 32-bits of the 64-bit IRIG BCD time (latched).
 * @param pIntTimeHigh is a pointer to store the upper 32-bits of the 64-bit internal time (latched).
 * @param pIntTimeLow is a pointer to store the lower 32-bits of the 64-bit internal time (latched).
 * @param pDeltaTimeHigh is a pointer to store the upper 32-bits of the 64-bit delta time.
 * @param pDeltaTimeLow is a pointer to store the lower 32-bits of the 64-bit delta time.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - devID not ADT_DEVID_CHANNELTYPE_A429 
	- \ref ADT_ERR_TIMEOUT - Timeout waiting for CSR bit to clear
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_IrigLatchedTimeGet(ADT_L0_UINT32 devID, 
											  ADT_L0_UINT32 *pIrigTimeHigh, ADT_L0_UINT32 *pIrigTimeLow, 
											  ADT_L0_UINT32 *pIntTimeHigh, ADT_L0_UINT32 *pIntTimeLow, 
											  ADT_L0_UINT32 *pDeltaTimeHigh, ADT_L0_UINT32 *pDeltaTimeLow) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 data, locked;
	ADT_L0_UINT32 tempIrigTimeHigh, tempIrigTimeLow;
	long long unsigned int time64_Irig_sec, time64_Irig_20ns, time64_Internal, time64_Delta;
	long long unsigned int tempyr, year, day, hour, minute, second, dayclock, dayno;

	tempIrigTimeHigh = tempIrigTimeLow = 0;
	time64_Irig_sec = time64_Irig_20ns = time64_Internal = time64_Delta = 0;
	year = day = hour = minute = second = 0;

	/* Make sure this is an A429 bank */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Check for IRIG lock */
	result = ADT_L1_ReadDeviceMem32(devID, ADT_L1_A429_PE_ROOT_STS, &data, 1);
	if (result != ADT_SUCCESS)
		return (result);

	if ((data & ADT_L1_A429_PESTS_IRIGLOCK) == ADT_L1_A429_PESTS_IRIGLOCK)
		locked = 1;
	else 
		locked = 0;

	if (locked) 
	{
		/* Write to the PE CSR to set the "Read IRIG Time" bit (bit 8) */
		data = 0;
		result = ADT_L1_ReadDeviceMem32(devID, ADT_L1_1553_PE_ROOT_CSR, &data, 1);
		data |= ADT_L1_1553_PECSR_RDIRIGTM;
		result = ADT_L1_WriteDeviceMem32(devID, ADT_L1_1553_PE_ROOT_CSR, &data, 1);

		/* Read the latched internal time value */
		result = ADT_L1_ReadDeviceMem32(devID, ADT_L1_1553_PE_TIMEHIGH, pIntTimeHigh, 1);
		result = ADT_L1_ReadDeviceMem32(devID, ADT_L1_1553_PE_TIMELOW, pIntTimeLow, 1);
		time64_Internal = ((long long unsigned int)*pIntTimeHigh << 32) | (long long unsigned int) *pIntTimeLow;

		/* Read the latched IRIG time value */
		result = ADT_L1_ReadDeviceMem32(devID, ADT_L1_1553_PE_IRIGTIMEHIGH, &tempIrigTimeHigh, 1);
		result = ADT_L1_ReadDeviceMem32(devID, ADT_L1_1553_PE_IRIGTIMELOW, &tempIrigTimeLow, 1);

		/* Convert BCD IRIG time to 64-bit binary equivalent */
		year = (((tempIrigTimeHigh & 0x000F0000) >> 16) * 10) + ((tempIrigTimeHigh & 0x0000F000) >> 12);
		year = year + 2000;  /* IRIG gives us 2-digit year, assumes century 2000 */
		day = (((tempIrigTimeHigh & 0x00000F00) >> 8) * 100) + (((tempIrigTimeHigh & 0x000000F0) >> 4) * 10) + (tempIrigTimeHigh & 0x0000000F);
		hour = (((tempIrigTimeLow & 0x00F00000) >> 20) * 10) + ((tempIrigTimeLow & 0x000F0000) >> 16);
		minute = (((tempIrigTimeLow & 0x0000F000) >> 12) * 10) + ((tempIrigTimeLow & 0x00000F00) >> 8);
		second =  (((tempIrigTimeLow & 0x000000F0) >> 4) * 10) + (tempIrigTimeLow & 0x0000000F);

		second++;  /* Correct for one second lag in IRIG time latched by firmware - PREVIOUS one-second sync */

		/* Get time in seconds */
		time64_Irig_sec = second;
		time64_Irig_sec += minute * 60;
		time64_Irig_sec += hour * 60 * 60;
		if (day != 0)  /* DAY OF YEAR should be 1-366, but IRIG generator can send zero! */
			time64_Irig_sec += (day - 1) * 24 * 60 * 60;  /* convert 1-indexed day of year to 0-indexed day number */
		tempyr = 2000;
		while (tempyr < year)
		{
			time64_Irig_sec += (unsigned long) YEARSIZEDAYS(tempyr) * (24L * 60L * 60L);
			tempyr++;
		}

		/* Convert IRIG time from SECONDS to 20ns LSB to calculate delta time */
		time64_Irig_20ns = time64_Irig_sec * 50000000L;

		/* Calc IRIG-PE Time Delta */
		time64_Delta = time64_Irig_20ns - time64_Internal;
		*pDeltaTimeHigh = (ADT_L0_UINT32)(time64_Delta >> 32);
		*pDeltaTimeLow = (ADT_L0_UINT32)(time64_Delta & 0x00000000FFFFFFFF);

		/* Convert the IRIG time in seconds, which has been corrected for 1-second lag, back to BCD IRIG time */
		dayclock = (unsigned long long) time64_Irig_sec % (24L * 60L * 60L);
		dayno = (unsigned long long) time64_Irig_sec / (24L * 60L * 60L);

		second = dayclock % 60;
		minute = (dayclock % 3600) / 60;
		hour = dayclock / 3600;
		tempyr = 2000;
		while (dayno >= (unsigned long long) YEARSIZEDAYS(tempyr)) {
			dayno -= YEARSIZEDAYS(tempyr);
			tempyr++;
		}

		/* convert day from 0-indexed day number to 1-indexed day of year */
		day = dayno + 1;

		/* convert year to 2-digit year of century */
		year = tempyr % 100;

		tempIrigTimeHigh = tempIrigTimeLow = 0;

		tempIrigTimeHigh |= ((ADT_L0_UINT32)(year / 10) & 0xF) << 16;	year = year - (year / 10) * 10;
		tempIrigTimeHigh |= ((ADT_L0_UINT32) year & 0xF) << 12;

		tempIrigTimeHigh |= ((ADT_L0_UINT32)(day / 100) & 0xF) << 8;	day = day - (day / 100) * 100;
		tempIrigTimeHigh |= ((ADT_L0_UINT32)(day / 10) & 0xF) << 4;		day = day - (day / 10) * 10;
		tempIrigTimeHigh |= ((ADT_L0_UINT32) day & 0xF);

		tempIrigTimeLow |= ((ADT_L0_UINT32)(hour / 10) & 0xF) << 20;	hour = hour - (hour / 10) * 10;
		tempIrigTimeLow |= ((ADT_L0_UINT32) hour & 0xF) << 16;

		tempIrigTimeLow |= ((ADT_L0_UINT32)(minute / 10) & 0xF) << 12;	minute = minute - (minute / 10) * 10;
		tempIrigTimeLow |= ((ADT_L0_UINT32) minute & 0xF) << 8;

		tempIrigTimeLow |= ((ADT_L0_UINT32)(second / 10) & 0xF) << 4;	second = second - (second / 10) * 10;
		tempIrigTimeLow |= ((ADT_L0_UINT32) second & 0xF);

		*pIrigTimeHigh = tempIrigTimeHigh;
		*pIrigTimeLow = tempIrigTimeLow;
	}
	else
	{
		/* IRIG Lock not present. IRIG time value is undetermined. */
		result = ADT_FAILURE;
	}

	return( result );
}

#endif


/******************************************************************************
  FUNCTION:		ADT_L1_A429_UseExtClk
 *****************************************************************************/
/*! \brief Enables the channel to use an external clock. 
 *
 * This function enables the bank to use an external clock.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Bank #).
 * @param useExtClk enables/disables ext clock (1 = use ext clk, 0 = do not use ext clk)
 * @param clkFreq must be 1, 5, or 10 (1MHz, 5MHz, or 10MHz)
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Bad value for clkFreq
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_UseExtClk(ADT_L0_UINT32 devID, ADT_L0_UINT32 useExtClk, ADT_L0_UINT32 clkFreq) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, data;

	/* Make sure this is a A429 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Verify valid frequency */
	if ((clkFreq != 1) && (clkFreq != 5) && (clkFreq != 10))
		return(ADT_ERR_BAD_INPUT);

	/* Write to the PE CSR to set the external clock bits (bits 12-15) */
	data = 0;
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_A429_PE_ROOT_CSR, &data, 1);

	/* Clear ext clk bits and apply new settings */
	data &= ~(ADT_L1_A429_PECSR_EC_FRQIN_1MHZ | ADT_L1_A429_PECSR_EC_FRQIN_5MHZ | ADT_L1_A429_PECSR_EC_FRQIN_10MHZ | ADT_L1_A429_PECSR_USE_EC);

	switch (clkFreq) {
		case 1:
			data |= ADT_L1_A429_PECSR_EC_FRQIN_1MHZ;
			break;
		case 5:
			data |= ADT_L1_A429_PECSR_EC_FRQIN_5MHZ;
			break;
		case 10:
			data |= ADT_L1_A429_PECSR_EC_FRQIN_10MHZ;
			break;
	}

	if (useExtClk != 0) 
	{
		/* Enable external clock */
		data |= ADT_L1_A429_PECSR_USE_EC;
	}
	
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_PE_ROOT_CSR, &data, 1);

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_A429_SC_ArmTrigger
 *****************************************************************************/
/*! \brief Arms the Signal Capture for a trigger. 
 *
 * This function arms the Signal Capture for a trigger.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param rxChan specifies the receive channel (0=Rx channel 1, 1=Rx channel 2).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_SC_ArmTrigger(ADT_L0_UINT32 devID, ADT_L0_UINT32 rxChan) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, data;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (rxChan == 0)  /* Receive Channel 1 */
	{
		data = ADT_L1_A429_PE_SC_CSR_TRG_ANYACT;
		result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_PE_SC_CSR_RX1, &data, 1);
	}
	else  /* Receive Channel 2 */
	{
		data = ADT_L1_A429_PE_SC_CSR_TRG_ANYACT;
		result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_PE_SC_CSR_RX2, &data, 1);
	}

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_A429_SC_ReadBuffer
 *****************************************************************************/
/*! \brief Reads the Signal Capture data buffer. 
 *
 * This function reads the Signal Capture data buffer.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param rxChan specifies the receive channel (0=Rx channel 1, 1=Rx channel 2).
 * @param buffer is an array with 4096 eight-bit values.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_SC_ReadBuffer(ADT_L0_UINT32 devID, ADT_L0_UINT32 rxChan, ADT_L0_UINT8 *buffer) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, i, j, temp;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (rxChan == 0)  /* Receive Channel 1 */
	{
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_A429_PE_SC_CSR_RX1, &temp, 1);
		if (temp & ADT_L1_A429_PE_SC_CSR_DATARDY) 
		{
			for (i=0; i<1024; i++) 
			{
				result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_A429_PE_SC_DATA_RX1, &temp, 1);

                j = i * 4;
                buffer[j + 0] = (ADT_L0_UINT8) (temp & 0x000000FF);
                buffer[j + 1] = (ADT_L0_UINT8) ((temp & 0x0000FF00) >> 8);
                buffer[j + 2] = (ADT_L0_UINT8) ((temp & 0x00FF0000) >> 16);
                buffer[j + 3] = (ADT_L0_UINT8) ((temp & 0xFF000000) >> 24);
			}
		}
	}
	else  /* Receive Channel 2 */
	{
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_A429_PE_SC_CSR_RX2, &temp, 1);
		if (temp & ADT_L1_1553_PE_SC_CSR_DATARDY) 
		{
			for (i=0; i<1024; i++) 
			{
				result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_A429_PE_SC_DATA_RX2, &temp, 1);

                j = i * 4;
                buffer[j + 0] = (ADT_L0_UINT8) (temp & 0x000000FF);
                buffer[j + 1] = (ADT_L0_UINT8) ((temp & 0x0000FF00) >> 8);
                buffer[j + 2] = (ADT_L0_UINT8) ((temp & 0x00FF0000) >> 16);
                buffer[j + 3] = (ADT_L0_UINT8) ((temp & 0xFF000000) >> 24);
			}
		}
	}

	/* The first point in SC memory is always 0 (-11.13v).
	 * We force this to zero volts (127).
	 */
	buffer[0] = 128;

	return( result );
}

/******************************************************************************
  FUNCTION:		ADT_L1_A429_PBTimeSet
 *****************************************************************************/
/*! \brief Sets the Playback time tag value for the Device Bank. 
 *
 * This function sets the Playback time tag value for the bank.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Bank #).
 * @param timeHigh is the upper 32-bits of the 64-bit time tag value.
 * @param timeLow is the lower 32-bits of the 64-bit time tag value.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_PBTimeSet(ADT_L0_UINT32 devID, ADT_L0_UINT32 timeHigh, ADT_L0_UINT32 timeLow) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 bank, bankRegOffset, data;

	/* Make sure this is a A429 bank */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the bank registers */
	result = Internal_GetChannelRegOffset(devID, &bank, &bankRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Write the time value */
	result = ADT_L0_WriteMem32(devID, bankRegOffset + ADT_L1_A429_PE_TIMEHIGH, &timeHigh, 1);
	result = ADT_L0_WriteMem32(devID, bankRegOffset + ADT_L1_A429_PE_TIMELOW, &timeLow, 1);

	/* Write to the PE CSR to set the "Set PB Time" bit (bit 20) */
	data = 0;
	result = ADT_L0_ReadMem32(devID, bankRegOffset + ADT_L1_A429_PE_ROOT_CSR, &data, 1);
	data |= ADT_L1_A429_PECSR_PB_SETTT;
	result = ADT_L0_WriteMem32(devID, bankRegOffset + ADT_L1_A429_PE_ROOT_CSR, &data, 1);

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_A429_PBTimeGet
 *****************************************************************************/
/*! \brief Gets the Playback time tag value for the Device Bank. 
 *
 * This function gets the Playback time tag value for the Device Bank.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Bank #).
 * @param pTimeHigh is a pointer to store the upper 32-bits of the 64-bit time tag value.
 * @param pTimeLow is a pointer to store the lower 32-bits of the 64-bit time tag value.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_TIMEOUT - Timeout waiting for CSR bit to clear
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_PBTimeGet(ADT_L0_UINT32 devID, ADT_L0_UINT32 *pTimeHigh, ADT_L0_UINT32 *pTimeLow) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 bank, bankRegOffset, data;

	/* Make sure this is a A429 Device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the bank registers */
	result = Internal_GetChannelRegOffset(devID, &bank, &bankRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Write to the PE CSR to set the "Read PB Time" bit (bit 21) */
	data = 0;
	result = ADT_L0_ReadMem32(devID, bankRegOffset + ADT_L1_A429_PE_ROOT_CSR, &data, 1);
	data |= ADT_L1_A429_PECSR_PB_READTT;
	result = ADT_L0_WriteMem32(devID, bankRegOffset + ADT_L1_A429_PE_ROOT_CSR, &data, 1);

	/* Read the time value */
	result = ADT_L0_ReadMem32(devID, bankRegOffset + ADT_L1_A429_PE_TIMEHIGH, pTimeHigh, 1);
	result = ADT_L0_ReadMem32(devID, bankRegOffset + ADT_L1_A429_PE_TIMELOW, pTimeLow, 1);

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_A429_IntervalTimerGet
 *****************************************************************************/
/*! \brief Gets the Interval Timer Register for the bank. 
 *
 * This function gets the interval timer register value for the bank.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param pIntvlTmr is a pointer to store the value for the ADT_L1_A429_PE_INTVLTMR Register.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_IntervalTimerGet(ADT_L0_UINT32 devID, ADT_L0_UINT32 *pIntvlTmrReg) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;

	/* Make sure this is an A429 bank */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Read from the PE ADT_L1_A429_PE_INTVLTMR Register */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_A429_PE_INTVLTMR, pIntvlTmrReg, 1);

	return( result );
}

/******************************************************************************
  FUNCTION:		ADT_L1_A429_IntervalTimerSet
 *****************************************************************************/
/*! \brief Sets the Interval Timer Register for the bank. 
 *
 * This function sets the interval timer register for the bank.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param IntvlTmr the value for the ADT_L1_1553_PE_INTVLTMR Register.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_IntervalTimerSet(ADT_L0_UINT32 devID, ADT_L0_UINT32 intvlTmrReg) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;

	/* Make sure this is an A429 bank */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Write to the PE ADT_L1_A429_PE_INTVLTMR Register */
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_PE_INTVLTMR, &intvlTmrReg, 1);

	return( result );
}


#ifdef __cplusplus
}
#endif
