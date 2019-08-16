/******************************************************************************
 * FILE:			ADT_L1_A429_RX.c
 *
 * Copyright (c) 2007-2016, Alta Data Technologies LLC (ADT), All Rights Reserved.
 * Use of this software is subject to the ADT Software License (latest
 * revision), US Government or local laws and the ADT Terms and Conditions 
 * of Sale (latest revision).
 *
 * DESCRIPTION:
 *	Source file for Layer 1 API.
 *	Contains A429 receive channel functions.
 *
 *****************************************************************************/
/*! \file ADT_L1_A429_RX.c  
 *  \brief Source file containing A429 receive channel functions 
 */
#include "ADT_L1.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
  FUNCTION:		ADT_L1_A429_RX_Channel_Init
 *****************************************************************************/
/*! \brief Initializes data structures for an A429 Receive Channel 
 *
 * This function initializes data structures for an A429 Receive Channel.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param RxChanNum is the RX Channel number (0-15).
 * @param BitRateHz is the bit rate in Hz (500 to 500000).
 * @param numRxP is the number of RxP to allocate.
 * @param includeInMcRx - 0 = do not include in MCRX buffer, 1 = include in MCRX buffer.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid Rx Channel number or invalid bit rate
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_RX_Channel_Init(ADT_L0_UINT32 devID, ADT_L0_UINT32 RxChanNum, ADT_L0_UINT32 BitRateHz, 
														   ADT_L0_UINT32 numRxP, ADT_L0_UINT32 options) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, chanConfig, temp;
	ADT_L0_UINT32 j, data, rxSetupOffset, halfBitTimeUs, size_needed, dataTableOffset, labelCVTOffset, rxpOffset;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Make sure this channel is present/allowed */
	chanConfig = 0;
	result = ADT_L1_A429_GetConfig(devID, &chanConfig);
	if (result != ADT_SUCCESS) return(result);
	else {
		if (RxChanNum < 16) {
			temp = 1;
			temp = temp << RxChanNum;
			if ((chanConfig & temp) == 0) result = ADT_ERR_BAD_INPUT;
		}
		else result = ADT_ERR_BAD_INPUT;
	}
	if (result != ADT_SUCCESS)
		return(result);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if ((RxChanNum < 16) && (BitRateHz >=500) && (BitRateHz <= 500000) && (numRxP > 0)) {
		/* Determine offset to the RX Channel Setup Regs */
		rxSetupOffset = ADT_L1_A429_ROOT_RX_REGS + RxChanNum * ADT_L1_A429_RXREG_CHAN_SIZE;

		/* Initialize SETUP registers for ARINC-429 settings */
		/* Set the provided bit rate - 10-bit period in microseconds */
		halfBitTimeUs = (ADT_L0_UINT32)((1.0 / (BitRateHz * 2.0))* 1000000) & 0x000003FF;

		/* NOTE: 0x80008430 = 32 bits per word, A429 high/low/parity */
		data = 0x80008430 | (halfBitTimeUs << 16);
		if (options & ADT_L1_A429_API_RX_MCON) data |= ADT_L1_A429_RXREG_SETUP1_MCRX;
		result = ADT_L0_WriteMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A429_RXREG_SETUP1, &data, 1);

		/* Start sequence number at zero */
		data = 0x00000000 * (RxChanNum + 1);
		result = ADT_L0_WriteMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A429_RXREG_SETUP2, &data, 1);

		/* Clear RxP Count */
		data = 0x00000000;
		result = ADT_L0_WriteMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A429_RXREG_RXPCNT, &data, 1);

		/* Initialize mask/compare registers */
		data = 0x00000000;
		result = ADT_L0_WriteMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A429_RXREG_MASK1, &data, 1);
		result = ADT_L0_WriteMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A429_RXREG_MASK2, &data, 1);

		/* Init Mask Values  */
		data = 0xFFFFFFFF;
		result = ADT_L0_WriteMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A429_RXREG_COMPARE1, &data, 1);
		result = ADT_L0_WriteMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A429_RXREG_COMPARE2, &data, 1);

		/* Allocate a Data Table */
		size_needed = ADT_L1_A429_RXP_HDR_SIZE + (ADT_L1_A429_RXP_SIZE * numRxP);
		result = ADT_L1_MemoryAlloc(devID, size_needed, &dataTableOffset);

		if (result == ADT_SUCCESS) {

			/* Write out RX CH Data Table Pointer to Root CH Setup */
			result = ADT_L0_WriteMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A429_RXREG_DATATBLPTR, &dataTableOffset, 1);

			/* Clear the RX CH Data Table Header memory */
			data = 0;
			for (j=0; j<ADT_L1_A429_RXP_HDR_SIZE; j+=sizeof(ADT_L0_UINT32)) 
				result = ADT_L0_WriteMem32(devID, channelRegOffset + dataTableOffset + j, &data, 1);

			/* Initialize Data Table Header with Total RxP count */
			result = ADT_L0_WriteMem32(devID, channelRegOffset + dataTableOffset + ADT_L1_A429_RXP_HDR_TOTAL_RXPCNT, &numRxP, 1);

			/* Initialize the RxP buffers */
			for (j=0; j<numRxP; j++) {
				rxpOffset = dataTableOffset + ADT_L1_A429_RXP_HDR_SIZE + ADT_L1_A429_RXP_SIZE * j;

				/* Write the CONTROL word - channel number and API RXP number */
				data = (RxChanNum << 24) | j;
				result = ADT_L0_WriteMem32(devID, channelRegOffset + rxpOffset + ADT_L1_A429_RXP_CONTROL, &data, 1);

				/* Clear time and data words */
				data = 0xFFFFFFFF;
				result = ADT_L0_WriteMem32(devID, channelRegOffset + rxpOffset + ADT_L1_A429_RXP_TIMEHIGH, &data, 1);
				result = ADT_L0_WriteMem32(devID, channelRegOffset + rxpOffset + ADT_L1_A429_RXP_TIMELOW, &data, 1);
				data = 0;
				result = ADT_L0_WriteMem32(devID, channelRegOffset + rxpOffset + ADT_L1_A429_RXP_DATA, &data, 1);
			}

			/* If selected, allocate memory, setup Label CVT Area and Root Pointer */
			if (options & ADT_L1_A429_API_RX_LABELCVTON) {
				
				/* Allocate the Label CVT RXP Table */
				size_needed = ADT_L1_A429_RXP_SIZE * ADT_L1_A429_API_NUMOF429LABELS;
				result = ADT_L1_MemoryAlloc(devID, size_needed, &labelCVTOffset);
				
				/* Write out CVT Root Pointer */
				result = ADT_L0_WriteMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A429_RXREG_LABELCVTPTR, &labelCVTOffset, 1);

				/* Initialize the RxP buffers */
				for (j = 0; j < ADT_L1_A429_API_NUMOF429LABELS; j++) {
					rxpOffset = labelCVTOffset + (ADT_L1_A429_RXP_SIZE * j);

					/* Write the CONTROL word - channel number & raw label number */
					data = (RxChanNum << 24) | j;
					result = ADT_L0_WriteMem32(devID, channelRegOffset + rxpOffset + ADT_L1_A429_RXP_CONTROL, &data, 1);

					/* Clear time and data words */
					data = 0xFFFFFFFF;
					result = ADT_L0_WriteMem32(devID, channelRegOffset + rxpOffset + ADT_L1_A429_RXP_TIMEHIGH, &data, 1);
					result = ADT_L0_WriteMem32(devID, channelRegOffset + rxpOffset + ADT_L1_A429_RXP_TIMELOW, &data, 1);
					data = 0;
					result = ADT_L0_WriteMem32(devID, channelRegOffset + rxpOffset + ADT_L1_A429_RXP_DATA, &data, 1);
				}
			}
			else {
				/* Write zero/NULL to Root RX CH CVT Pointer */
				data = 0;
				result = ADT_L0_WriteMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A429_RXREG_LABELCVTPTR, &data, 1);
			}
		}
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_A429_RX_Channel_Close
 *****************************************************************************/
/*! \brief Uninitializes data structures and frees memory for the RX Channel 
 *
 * This function uninitializes data structures and frees memory for the RX Channel.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param RxChanNum is the RX Channel number (0-15).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid Rx Channel number
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_RX_Channel_Close(ADT_L0_UINT32 devID, ADT_L0_UINT32 RxChanNum) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, size_needed, numRxP;
	ADT_L0_UINT32 data, rxSetupOffset, dataTableOffset;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (RxChanNum >= 16) 
		return(ADT_ERR_BAD_INPUT);

	/* Determine offset to the RX Channel Setup Regs */
	rxSetupOffset = ADT_L1_A429_ROOT_RX_REGS + RxChanNum * ADT_L1_A429_RXREG_CHAN_SIZE;

	/* Read the offset to the RX Channel Data Table */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A429_RXREG_DATATBLPTR, &dataTableOffset, 1);

	/* Read the number of RxP */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + dataTableOffset + ADT_L1_A429_RXP_HDR_TOTAL_RXPCNT, &numRxP, 1);

	/* Determine size of the RxP Data Table */
	size_needed = ADT_L1_A429_RXP_HDR_SIZE + ADT_L1_A429_RXP_SIZE * numRxP;

	/* Free the memory used by the Data Table */
	result = ADT_L1_MemoryFree(devID, dataTableOffset, size_needed);

	/* Clear the RX Channel Data Table pointer */
	dataTableOffset = 0;
	result = ADT_L0_WriteMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A429_RXREG_DATATBLPTR, &dataTableOffset, 1);

	/* Clear the RX Channel Setup Registers */
	data = 0;
	result = ADT_L0_WriteMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A429_RXREG_SETUP1, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A429_RXREG_SETUP2, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A429_RXREG_RXPCNT, &data, 1);

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_A429_RX_Channel_Start
 *****************************************************************************/
/*! \brief Starts the RX channel 
 *
 * This function starts the RX channel.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param RxChanNum is the RX Channel number (0-15).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid Rx Channel number
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_RX_Channel_Start(ADT_L0_UINT32 devID, ADT_L0_UINT32 RxChanNum) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 data, rxSetupOffset;
	ADT_L0_UINT32 channel, channelRegOffset;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	if (RxChanNum >= 16) 
		return(ADT_ERR_BAD_INPUT);

	/* Determine offset to the RX Channel Setup Regs */
	rxSetupOffset = ADT_L1_A429_ROOT_RX_REGS + RxChanNum * ADT_L1_A429_RXREG_CHAN_SIZE;

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Read the RX Channel Setup1 Register */
	data = 0;
	result = ADT_L0_ReadMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A429_RXREG_SETUP1, &data, 1);

	/* Set bit 0 to start RX operation */
	data |= ADT_L1_A429_RXREG_SETUP1_RXON;
	result = ADT_L0_WriteMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A429_RXREG_SETUP1, &data, 1);

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_A429_RX_Channel_Stop
 *****************************************************************************/
/*! \brief Stops the RT function of the channel 
 *
 * This function stops the RT function of the channel.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param RxChanNum is the RX Channel number (0-15).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid Rx Channel number
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_RX_Channel_Stop(ADT_L0_UINT32 devID, ADT_L0_UINT32 RxChanNum) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 data, rxSetupOffset;
	ADT_L0_UINT32 channel, channelRegOffset;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	if (RxChanNum >= 16) 
		return(ADT_ERR_BAD_INPUT);

	/* Determine offset to the RX Channel Setup Regs */
	rxSetupOffset = ADT_L1_A429_ROOT_RX_REGS + RxChanNum * ADT_L1_A429_RXREG_CHAN_SIZE;

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Read the RX Channel Setup1 Register */
	data = 0;
	result = ADT_L0_ReadMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A429_RXREG_SETUP1, &data, 1);

	/* Clear bit 0 to start RX operation */
	data &= ~ADT_L1_A429_RXREG_SETUP1_RXON;
	result = ADT_L0_WriteMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A429_RXREG_SETUP1, &data, 1);

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_A429_RX_Channel_ReadNewRxPs
 *****************************************************************************/
/*! \brief Reads all new RxPs from the channel data table
 *
 * This function reads all new RxPs from the channel data table.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param RxChanNum is the RX Channel number (0-15).
 * @param maxNumRxPs is the maximum number of RxPs to read (size of buffer).
 * @param pNumRxPs is the pointer to store the number of RxPs read.
 * @param pRxPBuffer is the pointer to store the RxP records.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - invalid pointer or invalid channel number
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_RX_Channel_ReadNewRxPs(ADT_L0_UINT32 devID, 
										ADT_L0_UINT32 RxChanNum, ADT_L0_UINT32 maxNumRxPs,  
										ADT_L0_UINT32 *pNumRxPs, ADT_L1_A429_RXP *pRxPBuffer) {

	/* NEW APPROACH - READS MULTIPLE RXPS, LIKE WE DO FOR MCRX */
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, rxSetupOffset;
	ADT_L0_UINT32 count, RxPCurrIndex, RxPLastIndex;
	ADT_L0_UINT32 temp, offset, numRxP;
	ADT_L0_UINT32 noRollCnt, rxpCnt;
	ADT_L0_UINT32 maxRxpCountPerRead = 90;    /* max 90 RxPs per ADCP read transaction */

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Determine offset to the RX Channel Setup Regs */
	rxSetupOffset = ADT_L1_A429_ROOT_RX_REGS + RxChanNum * ADT_L1_A429_RXREG_CHAN_SIZE;

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Ensure that the pointers passed in are not NULL */
	if ((pNumRxPs != 0) && (pRxPBuffer != 0)) {
		/* Read the RX Channel Data Table Pointer */
		offset = 0;
		result = ADT_L0_ReadMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A429_RXREG_DATATBLPTR, &offset, 1);


		/* Get the Total Number of RxPs in Data Table Header */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + offset + ADT_L1_A429_RXP_HDR_TOTAL_RXPCNT, &numRxP, 1);

		/* Read the API TAIL INDEX register */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + offset + ADT_L1_A429_RXP_HDR_API_TAIL_INDEX, &RxPLastIndex, 1);

		/* Read the RxP Current Index */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + offset + ADT_L1_A429_RXP_HDR_CURRENT_RXPCNT, &RxPCurrIndex, 1);

		/* Check for rollover and reset RxPCurrIndex if there is one */
		if (RxPCurrIndex >= numRxP)
			RxPCurrIndex = 0;

		/******************************************************************************************/
		/* For ENET devices, we want to read as much as we can in a block (up to 90 RxPs per read) */
		if ((devID & 0xF0000000) == ADT_DEVID_BACKPLANETYPE_ENET)
		{

			/* This is the main code for reading multiple RxPs per ADCP transacti0n */
			count = 0;
			if (maxNumRxPs > 0)
			{
				/* Determine how many RXPs are new, and if there is a roll-over of the linked list */
				if (RxPLastIndex <= RxPCurrIndex)  /* No Roll-Over */
				{
					noRollCnt = RxPCurrIndex - RxPLastIndex;
				}
				else   /* Roll-Over has occurred */
				{
					noRollCnt = numRxP - RxPLastIndex;
				}

				/* Read RxPs prior to rollover */
				while ((noRollCnt > 0) && (count < maxNumRxPs))
				{
					/* Determine how many RxPs we can read in a block */
					rxpCnt = noRollCnt;
					if (rxpCnt > maxRxpCountPerRead) rxpCnt = maxRxpCountPerRead;
					if ((rxpCnt + count) > maxNumRxPs) rxpCnt = maxNumRxPs - count;

					/* Read a block of RxPs */
					temp = channelRegOffset + offset + ADT_L1_A429_RXP_HDR_SIZE + (RxPLastIndex * ADT_L1_A429_RXP_SIZE);
					result = ADT_L0_ReadMem32(devID, temp, (ADT_L0_UINT32 *) &pRxPBuffer[count], rxpCnt * ADT_L1_1553_RXP_WRDCNT);
				
					/* Update Last Index */
					RxPLastIndex += rxpCnt;
					if (RxPLastIndex >= numRxP) RxPLastIndex = 0;

					count += rxpCnt;
					noRollCnt -= rxpCnt;
				}

			}

			/* Save last index to the API TAIL INDEX register */
			result = ADT_L0_WriteMem32(devID, channelRegOffset + offset + ADT_L1_A429_RXP_HDR_API_TAIL_INDEX, &RxPLastIndex, 1);

			*pNumRxPs = count;

		}
		/******************************************************************************************/

		else   /* Do this for NON-ENET devices */
		{
			/* Read and count RxPs until we get to the current offset */
			count = 0;
			while ((RxPLastIndex != RxPCurrIndex) && (count < maxNumRxPs)) {
				temp = channelRegOffset + offset + ADT_L1_A429_RXP_HDR_SIZE + (RxPLastIndex * ADT_L1_A429_RXP_SIZE);

				/* Read a RxP as a block */
				result = ADT_L0_ReadMem32(devID, temp, (ADT_L0_UINT32 *) &pRxPBuffer[count], ADT_L1_1553_RXP_WRDCNT);

				/* Update Last Index */
				RxPLastIndex += 1;
				if (RxPLastIndex >= numRxP) {
					if (RxPLastIndex == RxPCurrIndex) {
						RxPCurrIndex = 0; /* set RxPCurrIndex to force the while loop to exit */
						RxPLastIndex = 0;
					}
					else
						RxPLastIndex = 0;
				}

				count++;
			}

			/* Save last index to the API TAIL INDEX register */
			result = ADT_L0_WriteMem32(devID, channelRegOffset + offset + ADT_L1_A429_RXP_HDR_API_TAIL_INDEX, &RxPLastIndex, 1);

			*pNumRxPs = count;
		}

	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_A429_RX_Channel_ReadNewRxPsDMA
 *****************************************************************************/
/*! \brief Reads all new RxPs from the channel data table
 *
 * This function reads all new RxPs from the channel data table.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param RxChanNum is the RX Channel number (0-15).
 * @param maxNumRxPs is the maximum number of RxPs to read (size of buffer).
 * @param pNumRxPs is the pointer to store the number of RxPs read.
 * @param pRxPBuffer is the pointer to store the RxP records.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - invalid pointer or invalid channel number
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_RX_Channel_ReadNewRxPsDMA(ADT_L0_UINT32 devID, 
										ADT_L0_UINT32 RxChanNum, ADT_L0_UINT32 maxNumRxPs,  
										ADT_L0_UINT32 *pNumRxPs, ADT_L1_A429_RXP *pRxPBuffer) {

	/* NEW APPROACH - READS MULTIPLE RXPS, LIKE WE DO FOR MCRX */
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, rxSetupOffset;
	ADT_L0_UINT32 count, RxPCurrIndex, RxPLastIndex;
	ADT_L0_UINT32 temp, offset, numRxP;
	ADT_L0_UINT32 noRollCnt, rxpCnt;
	ADT_L0_UINT32 maxRxpCountPerRead = 90;    /* ENET - max 90 RxPs per ADCP read transaction */
	ADT_L0_UINT32 maxRxpCountPerDMA = 25;     /* PCI/PCIE - max 25 RxPs per DMA read transaction */

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Determine offset to the RX Channel Setup Regs */
	rxSetupOffset = ADT_L1_A429_ROOT_RX_REGS + RxChanNum * ADT_L1_A429_RXREG_CHAN_SIZE;

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Ensure that the pointers passed in are not NULL */
	if ((pNumRxPs != 0) && (pRxPBuffer != 0)) {
		/* Read the RX Channel Data Table Pointer */
		offset = 0;
		result = ADT_L0_ReadMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A429_RXREG_DATATBLPTR, &offset, 1);
	 
		/* Get the Total Number of RxPs in Data Table Header */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + offset + ADT_L1_A429_RXP_HDR_TOTAL_RXPCNT, &numRxP, 1);

		/* Read the API TAIL INDEX register */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + offset + ADT_L1_A429_RXP_HDR_API_TAIL_INDEX, &RxPLastIndex, 1);

		/* Read the RxP Current Index */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + offset + ADT_L1_A429_RXP_HDR_CURRENT_RXPCNT, &RxPCurrIndex, 1);

		/* Check for rollover and reset RxPCurrIndex if there is one */
		if (RxPCurrIndex >= numRxP)
			RxPCurrIndex = 0;

		/******************************************************************************************/
		/* For ENET devices, we want to read as much as we can in a block (up to 90 RxPs per read) */
		if ((devID & 0xF0000000) == ADT_DEVID_BACKPLANETYPE_ENET)
		{

			/* This is the main code for reading multiple RxPs per ADCP transacti0n */
			count = 0;
			if (maxNumRxPs > 0)
			{
				/* Determine how many RXPs are new, and if there is a roll-over of the linked list */
				if (RxPLastIndex <= RxPCurrIndex)  /* No Roll-Over */
				{
					noRollCnt = RxPCurrIndex - RxPLastIndex;
				}
				else   /* Roll-Over has occurred */
				{
					noRollCnt = numRxP - RxPLastIndex;
				}

				/* Read RxPs prior to rollover */
				while ((noRollCnt > 0) && (count < maxNumRxPs))
				{
					/* Determine how many RxPs we can read in a block */
					rxpCnt = noRollCnt;
					if (rxpCnt > maxRxpCountPerRead) rxpCnt = maxRxpCountPerRead;
					if ((rxpCnt + count) > maxNumRxPs) rxpCnt = maxNumRxPs - count;

					/* Read a block of RxPs */
					temp = channelRegOffset + offset + ADT_L1_A429_RXP_HDR_SIZE + (RxPLastIndex * ADT_L1_A429_RXP_SIZE);
					result = ADT_L0_ReadMem32(devID, temp, (ADT_L0_UINT32 *) &pRxPBuffer[count], rxpCnt * ADT_L1_1553_RXP_WRDCNT);
				
					/* Update Last Index */
					RxPLastIndex += rxpCnt;
					if (RxPLastIndex >= numRxP) RxPLastIndex = 0;

					count += rxpCnt;
					noRollCnt -= rxpCnt;
				}

			}

			/* Save last index to the API TAIL INDEX register */
			result = ADT_L0_WriteMem32(devID, channelRegOffset + offset + ADT_L1_A429_RXP_HDR_API_TAIL_INDEX, &RxPLastIndex, 1);

			*pNumRxPs = count;

		}
		/******************************************************************************************/

		else   /* Do this for NON-ENET devices */
		{

			/* This is the main code for reading multiple RxPs per DMA transacti0n */
			count = 0;
			if (maxNumRxPs > 0)
			{
				/* Determine how many RXPs are new, and if there is a roll-over of the linked list */
				if (RxPLastIndex <= RxPCurrIndex)  /* No Roll-Over */
				{
					noRollCnt = RxPCurrIndex - RxPLastIndex;
				}
				else   /* Roll-Over has occurred */
				{
					noRollCnt = numRxP - RxPLastIndex;
				}

				/* Read RxPs prior to rollover */
				while ((noRollCnt > 0) && (count < maxNumRxPs))
				{
					/* Determine how many RxPs we can read in a block */
					rxpCnt = noRollCnt;
					if (rxpCnt > maxRxpCountPerDMA) rxpCnt = maxRxpCountPerDMA;
					if ((rxpCnt + count) > maxNumRxPs) rxpCnt = maxNumRxPs - count;

					/* Read a block of RxPs */
					temp = channelRegOffset + offset + ADT_L1_A429_RXP_HDR_SIZE + (RxPLastIndex * ADT_L1_A429_RXP_SIZE);
					result = ADT_L0_ReadMem32DMA(devID, temp, (ADT_L0_UINT32 *) &pRxPBuffer[count], rxpCnt * ADT_L1_1553_RXP_WRDCNT);
				
					/* Update Last Index */
					RxPLastIndex += rxpCnt;
					if (RxPLastIndex >= numRxP) RxPLastIndex = 0;

					count += rxpCnt;
					noRollCnt -= rxpCnt;
				}

			}

			/* Save last index to the API TAIL INDEX register */
			result = ADT_L0_WriteMem32(devID, channelRegOffset + offset + ADT_L1_A429_RXP_HDR_API_TAIL_INDEX, &RxPLastIndex, 1);

			*pNumRxPs = count;

		}

	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_A429_RX_Channel_ReadRxP
 *****************************************************************************/
/*! \brief Reads a specific RxP from the channel data table
 *
 * This function reads a specific RxP from the channel data table.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param RxChanNum is the RX Channel number (0-15).
 * @param RxP_index is the index to the RxP to read.
 * @param pRxP is the pointer to store the RxP.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - invalid pointer, invalid channel number
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_RX_Channel_ReadRxP(ADT_L0_UINT32 devID, 
										ADT_L0_UINT32 RxChanNum, ADT_L0_UINT32 RxP_index,  
										ADT_L1_A429_RXP *pRxP) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, rxSetupOffset;
	ADT_L0_UINT32 temp, data, dataTableOffset, numRxP;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	if (RxChanNum >= 16) 
		return(ADT_ERR_BAD_INPUT);

	/* Determine offset to the RX Channel Setup Regs */
	rxSetupOffset = ADT_L1_A429_ROOT_RX_REGS + RxChanNum * ADT_L1_A429_RXREG_CHAN_SIZE;

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Ensure that the pointers passed in are not NULL */
	if (pRxP != 0) {
		/* Read the RX Channel Data Table Pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A429_RXREG_DATATBLPTR, &dataTableOffset, 1);

		if (dataTableOffset != 0)
		{
			/* Get the Total Number of RxPs in Data Table Header */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + dataTableOffset + ADT_L1_A429_RXP_HDR_TOTAL_RXPCNT, &numRxP, 1);

			/* Make sure the RxP index is valid */
			if (RxP_index < numRxP) {
				temp = channelRegOffset + dataTableOffset + ADT_L1_A429_RXP_HDR_SIZE + (RxP_index * ADT_L1_A429_RXP_SIZE);

				/* Read the RxP */
				result = ADT_L0_ReadMem32(devID, temp + ADT_L1_A429_RXP_CONTROL, &data, 1);
				pRxP->Control = data;
				result = ADT_L0_ReadMem32(devID, temp + ADT_L1_A429_RXP_TIMEHIGH, &data, 1);
				pRxP->TimeHigh = data;
				result = ADT_L0_ReadMem32(devID, temp + ADT_L1_A429_RXP_TIMELOW, &data, 1);
				pRxP->TimeLow = data;
				result = ADT_L0_ReadMem32(devID, temp + ADT_L1_A429_RXP_DATA, &data, 1);
				pRxP->Data = data;

			}
			else result = ADT_ERR_BAD_INPUT;
		}
		else result = ADT_ERR_BAD_CHAN_NUM;  /* RX channel not initialized */
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}

/******************************************************************************
  FUNCTION:		ADT_L1_A429_RX_Channel_CVTReadRxP
 *****************************************************************************/
/*! \brief Reads a specific RxP from the channel current value table
 *
 * This function reads a specific RxP from the channel current value table.
 * The index to the RXP CVT Table is the raw label value (LSB bit zero).
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param RxChanNum is the RX Channel number (0-15).
 * @param RxP_index is the index to the RxP to read.
 * @param pRxP is the pointer to store the RxP.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - invalid pointer, invalid channel number
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_RX_Channel_CVTReadRxP(ADT_L0_UINT32 devID, 
										ADT_L0_UINT32 RxChanNum, ADT_L0_UINT32 labelIndex,  
										ADT_L1_A429_RXP *pRxP) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, rxSetupOffset;
	ADT_L0_UINT32 temp, data, cvtTableOffset;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	if (RxChanNum >= 16) 
		return(ADT_ERR_BAD_INPUT);

	/* Determine offset to the RX Channel Setup Regs */
	rxSetupOffset = ADT_L1_A429_ROOT_RX_REGS + RxChanNum * ADT_L1_A429_RXREG_CHAN_SIZE;

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result == ADT_SUCCESS) {

		/* Get CVT Address - Should not be NULL (zero) */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A429_RXREG_LABELCVTPTR, &cvtTableOffset, 1);
		if (result == ADT_SUCCESS) {

			/* Check for zero/NULL CVT pointer - not defined at Init */
			if (cvtTableOffset != 0 ) {

				/* Ensure that the RXP is not NULL */
				if (pRxP != 0) {
				 
					/* Calc RXP address in CVT */
					temp = channelRegOffset + cvtTableOffset + (labelIndex * ADT_L1_A429_RXP_SIZE);

					/* Read the RxP */
					result = ADT_L0_ReadMem32(devID, temp + ADT_L1_A429_RXP_CONTROL, &data, 1);
					pRxP->Control = data;
					result = ADT_L0_ReadMem32(devID, temp + ADT_L1_A429_RXP_TIMEHIGH, &data, 1);
					pRxP->TimeHigh = data;
					result = ADT_L0_ReadMem32(devID, temp + ADT_L1_A429_RXP_TIMELOW, &data, 1);
					pRxP->TimeLow = data;
					result = ADT_L0_ReadMem32(devID, temp + ADT_L1_A429_RXP_DATA, &data, 1);
					pRxP->Data = data;
				}
				else result = ADT_ERR_BAD_INPUT;
			}
			else result = ADT_ERR_NORXCHCVT_ALLOCATED;
		}
	}

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_A429_RX_Channel_WriteRxP
 *****************************************************************************/
/*! \brief Writes a specific RxP to the channel data table
 *
 * This function writes a specific RxP to the channel data table.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param RxChanNum is the RX Channel number (0-15).
 * @param RxP_index is the index to the RxP to write.
 * @param pRxP is the pointer to the RxP to write.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - invalid pointer, invalid channel number
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_RX_Channel_WriteRxP(ADT_L0_UINT32 devID, 
										ADT_L0_UINT32 RxChanNum, ADT_L0_UINT32 RxP_index,  
										ADT_L1_A429_RXP *pRxP) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, rxSetupOffset;
	ADT_L0_UINT32 temp, dataTableOffset, numRxP;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	if (RxChanNum >= 16) 
		return(ADT_ERR_BAD_INPUT);

	/* Determine offset to the RX Channel Setup Regs */
	rxSetupOffset = ADT_L1_A429_ROOT_RX_REGS + RxChanNum * ADT_L1_A429_RXREG_CHAN_SIZE;

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Ensure that the pointers passed in are not NULL */
	if (pRxP != 0) {
		/* Read the RX Channel Data Table Pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A429_RXREG_DATATBLPTR, &dataTableOffset, 1);
	 
		if (dataTableOffset != 0)
		{
			/* Get the Total Number of RxPs in Data Table Header */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + dataTableOffset + ADT_L1_A429_RXP_HDR_TOTAL_RXPCNT, &numRxP, 1);

			/* Make sure the RxP index is valid */
			if (RxP_index < numRxP) {
				temp = channelRegOffset + dataTableOffset + ADT_L1_A429_RXP_HDR_SIZE + (RxP_index * ADT_L1_A429_RXP_SIZE);

				/* Write the RxP */
				result = ADT_L0_WriteMem32(devID, temp + ADT_L1_A429_RXP_CONTROL, &pRxP->Control, 1);
				result = ADT_L0_WriteMem32(devID, temp + ADT_L1_A429_RXP_TIMEHIGH, &pRxP->TimeHigh, 1);
				result = ADT_L0_WriteMem32(devID, temp + ADT_L1_A429_RXP_TIMELOW, &pRxP->TimeLow, 1);
				result = ADT_L0_WriteMem32(devID, temp + ADT_L1_A429_RXP_DATA, &pRxP->Data, 1);
			}
			else result = ADT_ERR_BAD_INPUT;
		}
		else result = ADT_ERR_BAD_CHAN_NUM;  /* RX channel not initialized */
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_A429_RX_Channel_CVTWriteRxP
 *****************************************************************************/
/*! \brief Writes a specific RxP to the channel current value table (CVT)
 *
 * This function writes a specific RxP to the channel data table.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param RxChanNum is the RX Channel number (0-15).
 * @param RxP_index is the index to the RxP to write.
 * @param pRxP is the pointer to the RxP to write.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - invalid pointer, invalid channel number
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_RX_Channel_CVTWriteRxP(ADT_L0_UINT32 devID, ADT_L0_UINT32 RxChanNum, 
																  ADT_L0_UINT32 labelIndex, ADT_L1_A429_RXP *pRxP) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, rxSetupOffset;
	ADT_L0_UINT32 temp, cvtTableOffset;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	if (RxChanNum >= 16) 
		return(ADT_ERR_BAD_INPUT);

	/* Determine offset to the RX Channel Setup Regs */
	rxSetupOffset = ADT_L1_A429_ROOT_RX_REGS + RxChanNum * ADT_L1_A429_RXREG_CHAN_SIZE;

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result == ADT_SUCCESS){

		/* Get CVT Address - Should not be NULL (zero) */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A429_RXREG_LABELCVTPTR, &cvtTableOffset, 1);
		if (result == ADT_SUCCESS) {

			/* Check that CVT was defined at Init */
			if (cvtTableOffset != 0) {

				/* Ensure that the pointers passed in & CVT are not NULL */
				if (pRxP != 0) {

					/* Calc RXP address in CVT */
					temp = channelRegOffset + cvtTableOffset + (labelIndex * ADT_L1_A429_RXP_SIZE);

					/* Write the RxP */
					result = ADT_L0_WriteMem32(devID, temp + ADT_L1_A429_RXP_CONTROL, &pRxP->Control, 1);
					result = ADT_L0_WriteMem32(devID, temp + ADT_L1_A429_RXP_TIMEHIGH, &pRxP->TimeHigh, 1);
					result = ADT_L0_WriteMem32(devID, temp + ADT_L1_A429_RXP_TIMELOW, &pRxP->TimeLow, 1);
					result = ADT_L0_WriteMem32(devID, temp + ADT_L1_A429_RXP_DATA, &pRxP->Data, 1);
				}
				else result = ADT_ERR_BAD_INPUT;
			}
			else result = ADT_ERR_NORXCHCVT_ALLOCATED;
		}
	}

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_A429_RX_Channel_SetConfig
 *****************************************************************************/
/*! \brief Sets the protocol settings for an A429 Receive Channel 
 *
 * This function sets the protocol settings for an A429 Receive Channel.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param RxChanNum is the RX Channel number (0-15).
 * @param Setup1 is the value for the RX Channel Setup1 register.
 * @param Setup2 is the value for the RX Channel Setup2 register.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid Rx Channel number
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_RX_Channel_SetConfig(ADT_L0_UINT32 devID, ADT_L0_UINT32 RxChanNum, ADT_L0_UINT32 Setup1, ADT_L0_UINT32 Setup2) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 rxSetupOffset;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (RxChanNum < 16) {
		/* Determine offset to the RX Channel Setup Regs */
		rxSetupOffset = ADT_L1_A429_ROOT_RX_REGS + RxChanNum * ADT_L1_A429_RXREG_CHAN_SIZE;

		/* Write Setup1 register */
		result = ADT_L0_WriteMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A429_RXREG_SETUP1, &Setup1, 1);

		/* Write Setup2 register */
		result = ADT_L0_WriteMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A429_RXREG_SETUP2, &Setup2, 1);
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_A429_RX_Channel_GetConfig
 *****************************************************************************/
/*! \brief Gets the protocol settings for an A429 Receive Channel 
 *
 * This function gets the protocol settings for an A429 Receive Channel.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param RxChanNum is the RX Channel number (0-15).
 * @param pSetup1 is the pointer for the RX Channel Setup1 register.
 * @param pSetup2 is the pointer for the RX Channel Setup2 register.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid Rx Channel number
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_RX_Channel_GetConfig(ADT_L0_UINT32 devID, ADT_L0_UINT32 RxChanNum, ADT_L0_UINT32 *pSetup1, ADT_L0_UINT32 *pSetup2) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 rxSetupOffset;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (RxChanNum < 16) {
		/* Determine offset to the RX Channel Setup Regs */
		rxSetupOffset = ADT_L1_A429_ROOT_RX_REGS + RxChanNum * ADT_L1_A429_RXREG_CHAN_SIZE;

		/* Read Setup1 register */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A429_RXREG_SETUP1, pSetup1, 1);

		/* Read Setup2 register */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A429_RXREG_SETUP2, pSetup2, 1);
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_A429_RX_Channel_SetMaskCompare
 *****************************************************************************/
/*! \brief Sets the mask/compare interrupt settings for an A429 Receive Channel 
 *
 * This function sets the mask/compare interrupt settings for an A429 Receive Channel.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param RxChanNum is the RX Channel number (0-15).
 * @param mask1 is the mask1 value for the RX channel.
 * @param compare1 is the compare1 value for the RX channel.
 * @param mask2 is the mask2 value for the RX channel.
 * @param compare2 is the compare2 value for the RX channel.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid Rx Channel number
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_RX_Channel_SetMaskCompare(ADT_L0_UINT32 devID, ADT_L0_UINT32 RxChanNum, ADT_L0_UINT32 mask1, ADT_L0_UINT32 compare1, ADT_L0_UINT32 mask2, ADT_L0_UINT32 compare2) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 rxSetupOffset;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (RxChanNum < 16) {
		/* Determine offset to the RX Channel Setup Regs */
		rxSetupOffset = ADT_L1_A429_ROOT_RX_REGS + RxChanNum * ADT_L1_A429_RXREG_CHAN_SIZE;

		/* Write Mask1 register */
		result = ADT_L0_WriteMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A429_RXREG_MASK1, &mask1, 1);

		/* Write Compare1 register */
		result = ADT_L0_WriteMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A429_RXREG_COMPARE1, &compare1, 1);

		/* Write Mask1 register */
		result = ADT_L0_WriteMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A429_RXREG_MASK2, &mask2, 1);

		/* Write Compare1 register */
		result = ADT_L0_WriteMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A429_RXREG_COMPARE2, &compare2, 1);
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_A429_RX_Channel_GetMaskCompare
 *****************************************************************************/
/*! \brief Gets the mask/compare interrupt settings for an A429 Receive Channel 
 *
 * This function gets the mask/compare interrupt settings for an A429 Receive Channel.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param RxChanNum is the RX Channel number (0-15).
 * @param pMask1 is the pointer for the mask1 value for the RX channel.
 * @param pCompare1 is the pointer for the compare1 value for the RX channel.
 * @param pMask2 is the pointer for the mask2 value for the RX channel.
 * @param pCompare2 is the pointer for the compare2 value for the RX channel.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid Rx Channel number
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_RX_Channel_GetMaskCompare(ADT_L0_UINT32 devID, ADT_L0_UINT32 RxChanNum, ADT_L0_UINT32 *pMask1, ADT_L0_UINT32 *pCompare1, ADT_L0_UINT32 *pMask2, ADT_L0_UINT32 *pCompare2) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 rxSetupOffset;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (RxChanNum < 16) {
		/* Determine offset to the RX Channel Setup Regs */
		rxSetupOffset = ADT_L1_A429_ROOT_RX_REGS + RxChanNum * ADT_L1_A429_RXREG_CHAN_SIZE;

		/* Read Mask1 register */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A429_RXREG_MASK1, pMask1, 1);

		/* Read Compare1 register */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A429_RXREG_COMPARE1, pCompare1, 1);

		/* Read Mask1 register */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A429_RXREG_MASK2, pMask2, 1);

		/* Read Compare1 register */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A429_RXREG_COMPARE2, pCompare2, 1);
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_A717_RX_Channel_SetConfig
 *****************************************************************************/
/*! \brief Sets the protocol settings for an A717 Receive Channel 
 *
 * This function sets the protocol settings for an A717 Receive Channel.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param RxChanNum is the RX Channel number (0-15).
 * @param Csr is the value for the A717 CSR register.
 * @param Sync1 is the value for the A717 SubFrame #1 Sync Word register.
 * @param Sync2 is the value for the A717 SubFrame #2 Sync Word register.
 * @param Sync3 is the value for the A717 SubFrame #3 Sync Word register.
 * @param Sync4 is the value for the A717 SubFrame #4 Sync Word register.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid Rx Channel number
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A717_RX_Channel_SetConfig(ADT_L0_UINT32 devID, ADT_L0_UINT32 RxChanNum, ADT_L0_UINT32 Csr, 
												ADT_L0_UINT32 Sync1, ADT_L0_UINT32 Sync2, ADT_L0_UINT32 Sync3, ADT_L0_UINT32 Sync4) {

	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 rxSetupOffset;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Determine offset to the RX Channel Setup Regs */
	rxSetupOffset = ADT_L1_A429_ROOT_RX_REGS + RxChanNum * ADT_L1_A429_RXREG_CHAN_SIZE;

	/* Write A717 csr register */
	result = ADT_L0_WriteMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A717_RXREG_CSR, &Csr, 1);
	/* Write A717 sync1 register */
	result = ADT_L0_WriteMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A717_RXREG_SYNC1, &Sync1, 1);
	/* Write A717 sync2 register */
	result = ADT_L0_WriteMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A717_RXREG_SYNC2, &Sync2, 1);
	/* Write A717 sync3 register */
	result = ADT_L0_WriteMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A717_RXREG_SYNC3, &Sync3, 1);
	/* Write A717 sync4 register */
	result = ADT_L0_WriteMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A717_RXREG_SYNC4, &Sync4, 1);


	return( result );
}

/******************************************************************************
  FUNCTION:		ADT_L1_A717_RX_Channel_GetConfig
 *****************************************************************************/
/*! \brief Gets the protocol settings for an A717 Receive Channel 
 *
 * This function gets the protocol settings for an A717 Receive Channel.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param RxChanNum is the RX Channel number (0-15).
 * @param pCsr is the value for the A717 CSR register.
 * @param pSync1 is the pointer to the A717 SubFrame #1 Sync Word register.
 * @param pSync2 is the pointer to the A717 SubFrame #2 Sync Word register.
 * @param pSync3 is the pointer to the A717 SubFrame #3 Sync Word register.
 * @param pSync4 is the pointer to the A717 SubFrame #4 Sync Word register.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid Rx Channel number
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A717_RX_Channel_GetConfig(ADT_L0_UINT32 devID, ADT_L0_UINT32 RxChanNum, ADT_L0_UINT32 *pCsr, 
												ADT_L0_UINT32 *pSync1, ADT_L0_UINT32 *pSync2, ADT_L0_UINT32 *pSync3, ADT_L0_UINT32 *pSync4) {

	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 rxSetupOffset;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Determine offset to the RX Channel Setup Regs */
	rxSetupOffset = ADT_L1_A429_ROOT_RX_REGS + RxChanNum * ADT_L1_A429_RXREG_CHAN_SIZE;

	/* Write A717 csr register */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A717_RXREG_CSR, pCsr, 1);
	/* Write A717 sync1 register */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A717_RXREG_SYNC1, pSync1, 1);
	/* Write A717 sync2 register */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A717_RXREG_SYNC2, pSync2, 1);
	/* Write A717 sync3 register */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A717_RXREG_SYNC3, pSync3, 1);
	/* Write A717 sync4 register */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + rxSetupOffset + ADT_L1_A717_RXREG_SYNC4, pSync4, 1);


	return( result );
}


#ifdef __cplusplus
}
#endif
