/******************************************************************************
 * FILE:			ADT_L1_1553_BM.c
 *
 * Copyright (c) 2007-2016, Alta Data Technologies LLC (ADT), All Rights Reserved.
 * Use of this software is subject to the ADT Software License (latest
 * revision), US Government or local laws and the ADT Terms and Conditions 
 * of Sale (latest revision).
 *
 * DESCRIPTION:
 * 	Source file for Layer 1 API.
 * 	Contains 1553 BM functions.
 * 
 *****************************************************************************/
/*! \file ADT_L1_1553_BM.c  
 *  \brief Source file containing 1553 BM functions 
 */
#include "ADT_L1.h"

#ifdef __cplusplus
extern "C" {
#endif


/******************************************************************************
  FUNCTION:		ADT_L1_1553_BM_Config
 *****************************************************************************/
/*! \brief Configures BM options for the device 
 *
 * This function configures BM options for the device.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param options is the value to write to the Root BM CSR.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_BM_Config(ADT_L0_UINT32 devID, ADT_L0_UINT32 options) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 data, mask;
	ADT_L0_UINT32 channel, channelRegOffset;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Read the Root BM CSR and change appropriately */
	mask = options & 0x0000073E;  /* Can only change bits 1, 2, 3, 4, 5, 8, 9, 10 */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BM_CSR, &data, 1);
	if (result == ADT_SUCCESS) {
		data &= mask | 0x00000001;  /* Clear any bits not set in options word, don't change bit 0 */
		data |= mask;  /* Set any bits set in options word */
		result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BM_CSR, &data, 1);
	}

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_BM_Start
 *****************************************************************************/
/*! \brief Starts BM operation for the device 
 *
 * This function starts BM operation for the device.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_BM_Start(ADT_L0_UINT32 devID) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 data;
	ADT_L0_UINT32 channel, channelRegOffset;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Read the starting CDP offset */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BM_STARTCDP, &data, 1);
	
	/* Write the current CDP register for BM (point to first CDP) */
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BM_CURRENTCDP, &data, 1);
	
	/* Write the API LAST CDP register for BM (point to first CDP) */
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BM_RESV_API, &data, 1);
	
	/* Clear the BM CDP COUNT register */
	data = 0;
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BM_CDPCOUNT, &data, 1);

	/* Start the BM */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BM_CSR, &data, 1);
	if (result == ADT_SUCCESS) {
		data |= 0x00000001; /* Set bit 0 (BM on) */
		result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BM_CSR, &data, 1);
	}

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_BM_Stop
 *****************************************************************************/
/*! \brief Stops BM operation for the device 
 *
 * This function stops BM operation for the device.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid device number
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_BM_Stop(ADT_L0_UINT32 devID) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 data;
	ADT_L0_UINT32 channel, channelRegOffset;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BM_CSR, &data, 1);
	if (result == ADT_SUCCESS) {
		data &= 0xFFFFFFFE;  /* Clear bit 0 */
		result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BM_CSR, &data, 1);
	}

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_BM_Clear
 *****************************************************************************/
/*! \brief Clears BM message counter for the device 
 *
 * This function clears BM message counter for the device.  This will cause
 * the BM to reset to zero for the message numbers used in the "BM Count" 
 * field of the CDP for each message.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid device number
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_BM_Clear(ADT_L0_UINT32 devID) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 data;
	ADT_L0_UINT32 channel, channelRegOffset;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	data = 0;  /* Clear the BM CDP Count register */
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BM_CDPCOUNT, &data, 1);

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_BM_FilterRead
 *****************************************************************************/
/*! \brief Reads BM filter settings for the device 
 *
 * This function reads BM filter settings for the device.  Each bit in a
 * filter word (rx filters or tx filters) corresponds to a subaddress.
 * Bit 0 is SA 0, bit 1 is SA 1, etc.  If the bit is set then the BM will
 * capture messages for that subaddress.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param rtAddr is the RT address to apply the filter for.
 * @param pRxFilters is a pointer to the filter word for RECEIVE subaddresses.
 * @param pTxFilters is a pointer to the filter word for TRANSMIT subaddresses.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid RT address
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_BM_FilterRead(ADT_L0_UINT32 devID, ADT_L0_UINT32 rtAddr, 
										 ADT_L0_UINT32 *pRxFilters, ADT_L0_UINT32 *pTxFilters) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 offset;
	ADT_L0_UINT32 channel, channelRegOffset;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (rtAddr < 32) {
		/* Offset to the appropriate RT entry */
		offset = channelRegOffset + ADT_L1_1553_BM_FILTERS + (rtAddr * 8);

		/* Read the filter settings */
		result = ADT_L0_ReadMem32(devID, offset,     pRxFilters, 1);
		result = ADT_L0_ReadMem32(devID, offset + 4, pTxFilters, 1);
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_BM_FilterWrite
 *****************************************************************************/
/*! \brief Writes BM filter settings for the device 
 *
 * This function writes BM filter settings for the device.  Each bit in a
 * filter word (rx filters or tx filters) corresponds to a subaddress.
 * Bit 0 is SA 0, bit 1 is SA 1, etc.  If the bit is set then the BM will
 * capture messages for that subaddress.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param rtAddr is the RT address to apply the filter for.
 * @param rxFilters is the filter word for RECEIVE subaddresses.
 * @param txFilters is the filter word for TRANSMIT subaddresses.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid RT address
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_BM_FilterWrite(ADT_L0_UINT32 devID, ADT_L0_UINT32 rtAddr, 
										 ADT_L0_UINT32 rxFilters, ADT_L0_UINT32 txFilters) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 offset;
	ADT_L0_UINT32 channel, channelRegOffset;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (rtAddr < 32) {
		/* Offset to the appropriate RT entry */
		offset = channelRegOffset + ADT_L1_1553_BM_FILTERS + (rtAddr * 8);

		/* Write the filter settings */
		result = ADT_L0_WriteMem32(devID, offset,     &rxFilters, 1);
		result = ADT_L0_WriteMem32(devID, offset + 4, &txFilters, 1);
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_BM_BufferCreate
 *****************************************************************************/
/*! \brief Allocates and links BM buffers 
 *
 * This function allocates and links the requested number of BM buffers.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param numMsgs is the number of BM CDP buffers requested.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_BM_BufferCreate(ADT_L0_UINT32 devID, ADT_L0_UINT32 numMsgs) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 i, data, offset_1st, offset_prev, offset_curr;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 datablock[ADT_RW_MEM_MAX_SIZE];

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Allocate and initialize the first BM CDP */
	result = ADT_L1_MemoryAlloc(devID, ADT_L1_1553_CDP_SIZE, &offset_1st);
	if (result == ADT_SUCCESS) {
		/* Clear all words in the CDP */
		memset(datablock, 0, sizeof(datablock));
		result = ADT_L0_WriteMem32(devID, channelRegOffset + offset_1st, datablock, ADT_L1_1553_CDP_SIZE/4);

		/* Set the CONTROL WORD */
		/***** Should not use hard-coded value here, option for interrupts? other? *****/
		data = 0x00000000;
		result = ADT_L0_WriteMem32(devID, channelRegOffset + offset_1st + ADT_L1_1553_CDP_CONTROL, &data, 1);

		/* Set the NEXT pointer (point to self) */
		result = ADT_L0_WriteMem32(devID, channelRegOffset + offset_1st + ADT_L1_1553_CDP_NEXT, &offset_1st, 1);

		/* Write the API info word to the CDP */
		data = 0;
		data |= ADT_L1_1553_CDP_RAPI_BM_ID;
		result = ADT_L0_WriteMem32(devID, channelRegOffset + offset_1st + ADT_L1_1553_CDP_RESV_API, &data, 1);

		/* Assign starting CDP register for BM (point to first CDP) */
		result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BM_STARTCDP, &offset_1st, 1);
		
		if (result == ADT_SUCCESS) {
			/* Allocate and initialize additional BM CDPs */
			offset_prev = offset_1st;
			for (i=1; i<numMsgs; i++) {
				result = ADT_L1_MemoryAlloc(devID, ADT_L1_1553_CDP_SIZE, &offset_curr);
				if (result == ADT_SUCCESS) {
					/* Clear all words in the CDP */
					memset(datablock, 0, sizeof(datablock));
					result = ADT_L0_WriteMem32(devID, channelRegOffset + offset_curr, datablock, ADT_L1_1553_CDP_SIZE/4);
					/* Set the NEXT pointer so PREV CDP points to this one */
					result = ADT_L0_WriteMem32(devID, channelRegOffset + offset_prev + ADT_L1_1553_CDP_NEXT, &offset_curr, 1);

					/* Set the NEXT pointer so CURRENT CDP points to FIRST CDP */
					result = ADT_L0_WriteMem32(devID, channelRegOffset + offset_curr + ADT_L1_1553_CDP_NEXT, &offset_1st, 1);

					/* Write the API info word to the CDP */
					data = i;
					data |= ADT_L1_1553_CDP_RAPI_BM_ID;
					result = ADT_L0_WriteMem32(devID, channelRegOffset + offset_curr + ADT_L1_1553_CDP_RESV_API, &data, 1);

					offset_prev = offset_curr;
				}
				else i=numMsgs;
			}

			/* Write the END CDP OFFSET to the API word in the Root SM Registers */
			result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BM_RESV_API_END, &offset_curr, 1);

		}
	}

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_BM_BufferFree
 *****************************************************************************/
/*! \brief Frees all board memory used for BM buffers. 
 *
 * This function frees all board memory used for BM buffers.
 * WARNING - THIS COMPLETELY UN-INITIALIZES THE BM.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_BM_BufferFree(ADT_L0_UINT32 devID) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 data, offset_1st, offset_next, offset_curr;
	ADT_L0_UINT32 channel, channelRegOffset;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Read pointer to the first BM CDP */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BM_STARTCDP, &offset_1st, 1);
	 
	/* Cycle through all BM CDPs until we get back to first one, freeing them as we go */
	offset_curr = offset_1st;
	do {
		/* Read the NEXT pointer for the current BM CDP */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + offset_curr, &offset_next, 1);

		/* Free the current BM CDP */
		result = ADT_L1_MemoryFree(devID, offset_curr, ADT_L1_1553_CDP_SIZE);

		/* Move to the next BM CDP */
		offset_curr = offset_next;
	}
	while (offset_next != offset_1st);

	/* Clear BM registers */
	data = 0;
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BM_CSR, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BM_STARTCDP, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BM_CURRENTCDP, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BM_CDPCOUNT, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BM_RESV_API, &data, 1);

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_BM_ReadNewMsgs
 *****************************************************************************/
/*! \brief Reads all new messages from the BM buffer 
 *
 * This function reads all new messages from the BM buffer.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param maxNumMsgs is the maximum number of messages to read (size of buffer).
 * @param pNumMsgs is the pointer to store the number of messages read.
 * @param pMsgBuffer is the pointer to store the message CDP records.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - invalid pointer
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_BM_ReadNewMsgs(ADT_L0_UINT32 devID, ADT_L0_UINT32 maxNumMsgs, 
										 ADT_L0_UINT32 *pNumMsgs, ADT_L1_1553_CDP *pMsgBuffer) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 count, byteCDPCurrOffset, byteCDPStartOffset, byteCDPEndOffset;
	ADT_L0_UINT32 byteCDPLastOffset, byteTempOffset;
	ADT_L0_UINT32 wdcount;
	ADT_L0_UINT32 noRollCnt, rollCnt, msgCnt, noRollCntSave;
	ADT_L0_UINT32 maxCdpCountPerRead = 6;    /* max 6 CDPs per ADCP read transaction */

	*pNumMsgs = 0;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS) return(result);

	/* Ensure that the pointers passed in are not NULL */
	if ((pNumMsgs != 0) && (pMsgBuffer != 0)) {

		/* Read the API LAST CDP register (next CDP to read) */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BM_RESV_API, &byteCDPLastOffset, 1);
		if (result != ADT_SUCCESS) return(result);
		
		/* Read the BM Current Offset (next CDP to be filled by PE) */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BM_CURRENTCDP, &byteCDPCurrOffset, 1);
		if (result != ADT_SUCCESS) return(result);

		/* Read the BM Start CDP Offset (first CDP in the buffer) */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BM_STARTCDP, &byteCDPStartOffset, 1);
		if (result != ADT_SUCCESS) return(result);

		/* Read the BM End CDP Offset (last CDP in the buffer) */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BM_RESV_API_END, &byteCDPEndOffset, 1);
		if (result != ADT_SUCCESS) return(result);

		/******************************************************************************************/
		/* For ENET devices, we want to read as much as we can in a block (up to 6 CDPs per read) */
		if ((devID & 0xF0000000) == ADT_DEVID_BACKPLANETYPE_ENET)
		{
			count = 0;
			if (maxNumMsgs > 0)
			{
				/* Determine how many CDPs are new, and if there is a roll-over of the linked list */
				if (byteCDPLastOffset <= byteCDPCurrOffset)  /* No Roll-Over */
				{
					noRollCnt = (byteCDPCurrOffset - byteCDPLastOffset) / ADT_L1_1553_CDP_BYTECNT;
					rollCnt = 0;
					noRollCntSave = 0;
				}
				else   /* Roll-Over has occurred */
				{
					noRollCnt = (byteCDPEndOffset - byteCDPLastOffset) / ADT_L1_1553_CDP_BYTECNT;
					rollCnt = (byteCDPCurrOffset - byteCDPStartOffset) / ADT_L1_1553_CDP_BYTECNT;
					noRollCntSave = noRollCnt;

					if ((rollCnt == 0) && (noRollCnt == 0))
						noRollCntSave = 1;
				}

				/* Now we have our key numbers of new CDPs to read.
				 *   noRollCnt = number of new CDPs before buffer roll-over
				 *   rollCnt = number of new CDPs after buffer roll-over
				 *
				 * ENET devices (using ADCP) can read up to 6 contiguous CDPs in one transaction.
				 */

				/* Read any messages before the roll-over */
				byteTempOffset = byteCDPLastOffset;
				while ((noRollCnt > 0) && (count < maxNumMsgs))
				{
					if (noRollCnt <= maxCdpCountPerRead) msgCnt = noRollCnt;
					else msgCnt = maxCdpCountPerRead;

					if ((count + msgCnt) > maxNumMsgs) msgCnt = maxNumMsgs - count;

					result = ADT_L0_ReadMem32(devID, channelRegOffset + byteTempOffset, (ADT_L0_UINT32 *) &pMsgBuffer[count], msgCnt * ADT_L1_1553_CDP_WRDCNT);
					if (result != ADT_SUCCESS) return(result);

					byteTempOffset += (msgCnt * ADT_L1_1553_CDP_BYTECNT);

					/* Save last offset to the API LAST CDP register */
					result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BM_RESV_API, &byteTempOffset, 1);
					if (result != ADT_SUCCESS) return(result);

					noRollCnt -= msgCnt;
					count += msgCnt;
				}

				/* If we have a rollover, read the END CDP */
				if (((rollCnt > 0) || (noRollCntSave != 0)) && (count < maxNumMsgs)) 
				{
					byteTempOffset = byteCDPEndOffset;
					msgCnt = 1;

					result = ADT_L0_ReadMem32(devID, channelRegOffset + byteTempOffset, (ADT_L0_UINT32 *) &pMsgBuffer[count], msgCnt * ADT_L1_1553_CDP_WRDCNT);
					if (result != ADT_SUCCESS) return(result);

					/* Save last offset to the API LAST CDP register */
					result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BM_RESV_API, &byteCDPStartOffset, 1);
					if (result != ADT_SUCCESS) return(result);

					count += msgCnt;
				}

				/* Read any messages after the roll-over */
				byteTempOffset = byteCDPStartOffset;
				while ((rollCnt > 0) && (count < maxNumMsgs))
				{
					if (rollCnt <= maxCdpCountPerRead) msgCnt = rollCnt;
					else msgCnt = maxCdpCountPerRead;

					if ((count + msgCnt) > maxNumMsgs) msgCnt = maxNumMsgs - count;

					result = ADT_L0_ReadMem32(devID, channelRegOffset + byteTempOffset, (ADT_L0_UINT32 *) &pMsgBuffer[count], msgCnt * ADT_L1_1553_CDP_WRDCNT);
					if (result != ADT_SUCCESS) return(result);

					byteTempOffset += (msgCnt * ADT_L1_1553_CDP_BYTECNT);

					/* Save last offset to the API LAST CDP register */
					result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BM_RESV_API, &byteTempOffset, 1);
					if (result != ADT_SUCCESS) return(result);

					rollCnt -= msgCnt;
					count += msgCnt;
				}

			}
			if (result == ADT_SUCCESS) *pNumMsgs = count;
		}
		/******************************************************************************************/
		else   /* Do this for NON-ENET devices */
		{
			/* Read and count messages until we get to the current offset */
			count = 0;
			while ((byteCDPLastOffset != byteCDPCurrOffset) && (count < maxNumMsgs)) {
				/* Read a message */

				/* Read first 17 words of CDP (start through STS2INFO), all but DATA */
				result = ADT_L0_ReadMem32(devID, channelRegOffset + byteCDPLastOffset, 
					(ADT_L0_UINT32 *) &pMsgBuffer[count], 17);
				if (result != ADT_SUCCESS) return(result);

				/* Only read the necessary number of data words */
				wdcount = pMsgBuffer[count].CDPStatusWord & ADT_L1_1553_CDP_STATUS_MSGWC;
				if (wdcount > 32) wdcount = 32;
				if (wdcount > 0)
				{
					result = ADT_L0_ReadMem32(devID, channelRegOffset + byteCDPLastOffset + ADT_L1_1553_CDP_DATA1INFO, 
						(ADT_L0_UINT32 *) &pMsgBuffer[count].DATAinfo[0], wdcount);
					if (result != ADT_SUCCESS) return(result);
				}

				byteCDPLastOffset = pMsgBuffer[count].NextPtr;

				/* Save CDP NEXT pointer to the API LAST CDP register */
				result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BM_RESV_API, &byteCDPLastOffset, 1);
				if (result != ADT_SUCCESS) return(result);

				count++;
			}
			if (result == ADT_SUCCESS) *pNumMsgs = count;
		}
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_BM_ReadNewMsgsDMA
 *****************************************************************************/
/*! \brief Reads all new messages from the BM buffer using DMA 
 *
 * This function reads all new messages from the BM buffer using DMA.
 * NOTE THAT ONLY SELECTED LAYER 0 (Windows and Linux) IMPLEMENT DMA AND ONLY
 * BOARD TYPES THAT USE THE PLX9056 SUPPORT DMA.  PCIE ALSO SUPPORTS DMA.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param maxNumMsgs is the maximum number of messages to read (size of buffer).
 * @param pNumMsgs is the pointer to store the number of messages read.
 * @param pMsgBuffer is the pointer to store the message CDP records.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - invalid pointer
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_BM_ReadNewMsgsDMA(ADT_L0_UINT32 devID, ADT_L0_UINT32 maxNumMsgs, 
										 ADT_L0_UINT32 *pNumMsgs, ADT_L1_1553_CDP *pMsgBuffer) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 byteCDPCurrOffset, byteCDPLastOffset;
	ADT_L0_UINT32 byteCDPStartOffset, byteCDPEndOffset;
	ADT_L0_UINT32 numMsgsToRead, readCount;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	if (maxNumMsgs > 0) {

		/* Get offset to the channel registers */
		result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
		if (result != ADT_SUCCESS)
			return(result);

		/* Ensure that the pointers passed in are not NULL */
		if ((pNumMsgs != 0) && (pMsgBuffer != 0)) {

			/* Read the API LAST CDP register (next CDP to read) */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BM_RESV_API, &byteCDPLastOffset, 1);
			if (result != ADT_SUCCESS) return(result);
			
			/* Read the BM Current Offset (next CDP to be filled by PE) */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BM_CURRENTCDP, &byteCDPCurrOffset, 1);
			if (result != ADT_SUCCESS) return(result);

			/* Read the BM Start CDP Offset (first CDP in the buffer) */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BM_STARTCDP, &byteCDPStartOffset, 1);
			if (result != ADT_SUCCESS) return(result);

			/* Read the BM End CDP Offset (last CDP in the buffer) */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BM_RESV_API_END, &byteCDPEndOffset, 1);
			if (result != ADT_SUCCESS) return(result);

			/* Determine how many CDPs are new */
			if (byteCDPCurrOffset < byteCDPLastOffset)  /* Roll-Over has occurred */
			{                    /* End of List CDP Count                    Start of List CDP Count */
				numMsgsToRead = ((byteCDPEndOffset - byteCDPLastOffset) + (byteCDPCurrOffset - byteCDPStartOffset)) / ADT_L1_1553_CDP_BYTECNT + 1;
			}
			else  /* No Roll-Over */
			{
				numMsgsToRead = (byteCDPCurrOffset - byteCDPLastOffset) / ADT_L1_1553_CDP_BYTECNT;
			}
			if (numMsgsToRead > maxNumMsgs) numMsgsToRead = maxNumMsgs;

			*pNumMsgs = 0;
			while (*pNumMsgs < numMsgsToRead)
			{
				if ((byteCDPLastOffset == byteCDPEndOffset) ||  // There is only one CDP to read before wrapping
					((byteCDPLastOffset < byteCDPCurrOffset) && (byteCDPCurrOffset - byteCDPLastOffset == ADT_L1_1553_CDP_BYTECNT))) // Thers is only one CDP left in buffer
				{
					readCount = 1;
				}
				else
				{
					/* The DMA buffer in driver is 100 32-bit words, so max to read at a time is 2 CDPs */
					readCount = 2;
				}

				result = ADT_L0_ReadMem32DMA(devID, channelRegOffset + byteCDPLastOffset, (ADT_L0_UINT32 *) &pMsgBuffer[*pNumMsgs], readCount * ADT_L1_1553_CDP_WRDCNT);
				if (result != ADT_SUCCESS) return(result);
				
				byteCDPLastOffset += readCount * ADT_L1_1553_CDP_BYTECNT;                         /* Update pointer to next CDP to read. */
				if (byteCDPLastOffset > byteCDPEndOffset) byteCDPLastOffset = byteCDPStartOffset; /* Check for wrap of Buffer.           */
				*pNumMsgs += readCount;                                                           /* Update number of CDPs read.         */

				result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BM_RESV_API, &byteCDPLastOffset, 1); /* Update to next location to read */
				if (result != ADT_SUCCESS) return(result);
			}
		}
		else result = ADT_ERR_BAD_INPUT;
	}

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_BM_CDPWrite
 *****************************************************************************/
/*! \brief Writes a CDP for the BM 
 *
 * This function writes a CDP for the BM.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param cdpNum is the CDP number.
 * @param pCdp is a pointer to the CDP structure.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_BM_CDPWrite(ADT_L0_UINT32 devID, ADT_L0_UINT32 cdpNum, ADT_L1_1553_CDP *pCdp) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 startCdp, selectedCdp;
	/* ADT_L0_UINT32 temp, i;  ONLY USED FOR INEFFICIENT METHOD, see below */

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Read the BM starting CDP address */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BM_STARTCDP, &startCdp, 1);

#if 1  /* THIS IS MORE EFFICIENT, but assumes cdpNum is within the range of allocated buffers */
	selectedCdp = startCdp + (cdpNum * ADT_L1_1553_CDP_SIZE);
#else  /* THIS IS VERY INEFFICIENT, but handles rollover if cdpNum higher than allocated number of buffers */
	/* Follow NEXT pointers to find the selected CDP */
	i = 0;
	selectedCdp = startCdp;
	while (i < cdpNum) {
		result = ADT_L0_ReadMem32(devID, channelRegOffset + selectedCdp, &temp, 1);
		selectedCdp = temp;
		i++;
	}
#endif

	if (result == ADT_SUCCESS) {

		/* Write the selected CDP (all EXCEPT the next pointer and reserved words) 
		 * Also not overwriting the API INFO word.
		 */
		/* For BLOCK write, we start at the Mask Value word (skip first 5 words of CDP) */
		result = ADT_L0_WriteMem32(devID, channelRegOffset + selectedCdp + ADT_L1_1553_CDP_MASKVALUE, &pCdp->MaskValue, (ADT_L1_1553_CDP_SIZE - ADT_L1_1553_CDP_MASKVALUE)/4);
	}

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_BM_CDPRead
 *****************************************************************************/
/*! \brief Reads a CDP for the BM 
 *
 * This function reads a CDP for the BM.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param cdpNum is the CDP number.
 * @param pCdp is a pointer to store the CDP structure.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_BM_CDPRead(ADT_L0_UINT32 devID, ADT_L0_UINT32 cdpNum, ADT_L1_1553_CDP *pCdp) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 temp, startCdp, selectedCdp, i;
	ADT_L0_UINT32 datablock[ADT_RW_MEM_MAX_SIZE];

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Read the BM starting CDP address */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BM_STARTCDP, &startCdp, 1);

	/* Follow NEXT pointers to find the selected CDP */
	i = 0;
	selectedCdp = startCdp;
	while (i < cdpNum) {
		result = ADT_L0_ReadMem32(devID, channelRegOffset + selectedCdp, &temp, 1);
		selectedCdp = temp;
		i++;
	}

	if (result == ADT_SUCCESS) {

		/* Read the selected CDP */
		memset(datablock, 0, sizeof(datablock));
		result = ADT_L0_ReadMem32(devID, channelRegOffset + selectedCdp, datablock, ADT_L1_1553_CDP_SIZE/4);
		pCdp->BMCount = datablock[1];
		pCdp->MaskValue = datablock[5];
		pCdp->MaskCompare = datablock[6];
		pCdp->CDPControlWord = datablock[7];
		pCdp->CDPStatusWord = datablock[8];
		pCdp->TimeHigh = datablock[9];
		pCdp->TimeLow = datablock[10];
		pCdp->IMGap = datablock[11];
		pCdp->Rsvd3 = datablock[12];
		pCdp->CMD1info = datablock[13];
		pCdp->CMD2info = datablock[14];
		pCdp->STS1info = datablock[15];
		pCdp->STS2info = datablock[16];
		for (i=0; i<32; i++)
			pCdp->DATAinfo[i] = datablock[17+i];

	}

	return( result );
}


#ifdef __cplusplus
}
#endif
