/******************************************************************************
 * FILE:			ADT_L1_A429_RX_MC.c
 *
 * Copyright (c) 2007-2016, Alta Data Technologies LLC (ADT), All Rights Reserved.
 * Use of this software is subject to the ADT Software License (latest
 * revision), US Government or local laws and the ADT Terms and Conditions 
 * of Sale (latest revision).
 *
 * DESCRIPTION:
 *	Source file for Layer 1 API.
 *	Contains A429 multi-channel receive functions.
 *
 *****************************************************************************/
/*! \file ADT_L1_A429_RX_MC.c  
 *  \brief Source file containing A429 multi-channel receive functions 
 */
#include "ADT_L1.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
  FUNCTION:		ADT_L1_A429_RXMC_BufferCreate
 *****************************************************************************/
/*! \brief Allocates the multi-channel receive buffer. 
 *
 * This function allocates the A429 multi-channel receive buffer.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param numRxP is the number of RxP buffers requested.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - not an A429 device
	- \ref ADT_ERR_BAD_INPUT - invalid number of RxP (cannot be zero)
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_RXMC_BufferCreate(ADT_L0_UINT32 devID, ADT_L0_UINT32 numRxP) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 i, data, offset, size_needed, temp;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Must have non-zero number of RxP */
	if (numRxP == 0)
		return(ADT_ERR_BAD_INPUT);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Allocate and initialize the Multi-Channel RxP Data Table */
	size_needed = ADT_L1_A429_RXP_HDR_SIZE + ADT_L1_A429_RXP_SIZE * numRxP;
	result = ADT_L1_MemoryAlloc(devID, size_needed, &offset);
	if (result == ADT_SUCCESS) {

		/* Write the Multi-Channel RxP Data Table Pointer */
		data = offset;
		result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_PE_MCRXP_DATATBLPTR, &data, 1);

		/* Clear all words */
		/* THIS IS NOT NEEDED, ADT_L1_MemoryAlloc clears the allocated memory *
		data = 0;
		for (i=0; i<size_needed; i+=4)
			result = ADT_L0_WriteMem32(devID, channelRegOffset + offset + i, &data, 1);
		*/

		/* Set the Total Number of RxPs in Data Table Header */
		data = numRxP;
		result = ADT_L0_WriteMem32(devID, channelRegOffset + offset + ADT_L1_A429_RXP_HDR_TOTAL_RXPCNT, &data, 1);

		/* Initialize all RxP with RxP number in the API info word (lower 16-bits of CONTROL word) */
		/* Bit 15 is set to indicate that this is a MCRX RXP */
		/* This is SLOW for AltaView with ENET-A429 (20000 RXPs), so we only do this
		   if NOT AltaView (numRxP < 20000).  This allows user applications to use the
		   API Info word for interrupts etc without causing slow init for ENET-A429.
		 */
		if (numRxP < 20000) {
			for (i=0; i<numRxP; i++) {
				data = 0x00008000 | i;
				temp = channelRegOffset + offset + ADT_L1_A429_RXP_HDR_SIZE + (i * ADT_L1_A429_RXP_SIZE) + ADT_L1_A429_RXP_CONTROL;
				result = ADT_L0_WriteMem32(devID, temp, &data, 1);
			}
		}
		/***/
	}

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_A429_RXMC_BufferFree
 *****************************************************************************/
/*! \brief Frees all board memory used for the multi-channel receive buffer. 
 *
 * This function frees all board memory used for the multi-channel receive buffer.
 * WARNING - THIS COMPLETELY UN-INITIALIZES MULTI-CHANNEL RECEIVE.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_RXMC_BufferFree(ADT_L0_UINT32 devID) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 data, offset, size_needed, numRxP;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Read the Multi-Channel RxP Data Table Pointer */
	data = 0;
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_A429_PE_MCRXP_DATATBLPTR, &data, 1);
	offset = data;
	 
	/* Get the Total Number of RxPs in Data Table Header */
	data = 0;
	result = ADT_L0_ReadMem32(devID, channelRegOffset + offset + ADT_L1_A429_RXP_HDR_TOTAL_RXPCNT, &data, 1);
	numRxP = data;

	/* Free the memory */
	size_needed = ADT_L1_A429_RXP_HDR_SIZE + ADT_L1_A429_RXP_SIZE * numRxP;
	result = ADT_L1_MemoryFree(devID, offset, size_needed);

	/* Clear the Multi-Channel RxP Data Table Pointer */
	data = 0;
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_PE_MCRXP_DATATBLPTR, &data, 1);

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_A429_RXMC_ReadNewRxPs
 *****************************************************************************/
/*! \brief Reads all new RxPs from the A429 Multi-Channel RX buffer 
 *
 * This function reads all new RxPs from the A429 Multi-Channel RX buffer.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param maxNumRxPs is the maximum number of RxPs to read (size of buffer).
 * @param pNumRxPs is the pointer to store the number of RxPs read.
 * @param pRxPBuffer is the pointer to store the RxP records.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - invalid pointer
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_RXMC_ReadNewRxPs(ADT_L0_UINT32 devID, ADT_L0_UINT32 maxNumRxPs, 
										 ADT_L0_UINT32 *pNumRxPs, ADT_L1_A429_RXP *pRxPBuffer) {

	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 count, RxPCurrIndex, RxPLastIndex;
	ADT_L0_UINT32 temp, offset, numRxP;
	ADT_L0_UINT32 noRollCnt, rxpCnt;
	ADT_L0_UINT32 maxRxpCountPerRead = 90;    /* max 90 RxPs per ADCP read transaction */

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Ensure that the pointers passed in are not NULL */
	if ((pNumRxPs != 0) && (pRxPBuffer != 0)) {
		/* Read the Multi-Channel RxP Data Table Pointer */
		offset = 0;
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_A429_PE_MCRXP_DATATBLPTR, &offset, 1);
	 
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
  FUNCTION:		ADT_L1_A429_RXMC_ReadNewRxPsDMA
 *****************************************************************************/
/*! \brief Reads all new RxPs from the A429 Multi-Channel RX buffer using DMA
 *
 * This function reads all new RxPs from the A429 Multi-Channel RX buffer using DMA.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param maxNumRxPs is the maximum number of RxPs to read (size of buffer).
 * @param pNumRxPs is the pointer to store the number of RxPs read.
 * @param pRxPBuffer is the pointer to store the RxP records.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - invalid pointer
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_RXMC_ReadNewRxPsDMA(ADT_L0_UINT32 devID, ADT_L0_UINT32 maxNumRxPs, 
										 ADT_L0_UINT32 *pNumRxPs, ADT_L1_A429_RXP *pRxPBuffer) {

	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 count, RxPCurrIndex, RxPLastIndex;
	ADT_L0_UINT32 temp, offset, numRxP;
	ADT_L0_UINT32 noRollCnt, rxpCnt;
	ADT_L0_UINT32 maxRxpCountPerRead = 90;    /* max 90 RxPs per ADCP read transaction */
	ADT_L0_UINT32 maxRxpCountPerDMA  = 25;    /* max 25 per DMA */

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Ensure that the pointers passed in are not NULL */
	if ((pNumRxPs != 0) && (pRxPBuffer != 0)) {
		/* Read the Multi-Channel RxP Data Table Pointer */
		offset = 0;
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_A429_PE_MCRXP_DATATBLPTR, &offset, 1);
	 
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

			/* ENET - This is the main code for reading multiple RxPs per ADCP transacti0n */
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

		else   /* Do this for NON-ENET devices - THIS IS WHERE WE USE DMA */
		{
			/* PCI/PCIE - This is the main code for reading multiple RxPs (max 25) per DMA transaction */
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
  FUNCTION:		ADT_L1_A429_RXMC_ReadRxP
 *****************************************************************************/
/*! \brief Reads a specific RxP from the multi-channel data table
 *
 * This function reads a specific RxP from the multi-channel data table.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param RxP_index is the index to the RxP to read.
 * @param pRxP is the pointer to store the RxP.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - invalid pointer
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_RXMC_ReadRxP(ADT_L0_UINT32 devID, ADT_L0_UINT32 RxP_index, ADT_L1_A429_RXP *pRxP) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 temp, data, dataTableOffset, numRxP;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Ensure that the pointers passed in are not NULL */
	if (pRxP != 0) {
		/* Read the MCRX Data Table Pointer */
		dataTableOffset = 0;
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_A429_PE_MCRXP_DATATBLPTR, &dataTableOffset, 1);
	 
		/* Get the Total Number of RxPs in Data Table Header */
		numRxP = 0;
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
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_A429_RXMC_WriteRxP
 *****************************************************************************/
/*! \brief Writes a specific RxP to the multi-channel data table
 *
 * This function writes a specific RxP to the multi-channel data table.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param RxP_index is the index to the RxP to write.
 * @param pRxP is the pointer to the RxP to write.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - invalid pointer, invalid channel number
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_RXMC_WriteRxP(ADT_L0_UINT32 devID, ADT_L0_UINT32 RxP_index, ADT_L1_A429_RXP *pRxP){
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 temp, dataTableOffset, numRxP;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);
	/* Ensure that the pointers passed in are not NULL */
	if (pRxP != 0) {
		/* Read the MCRX Data Table Pointer */
		dataTableOffset = 0;
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_A429_PE_MCRXP_DATATBLPTR, &dataTableOffset, 1);
	 
		/* Get the Total Number of RxPs in Data Table Header */
		numRxP = 0;
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
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}


#ifdef __cplusplus
}
#endif
