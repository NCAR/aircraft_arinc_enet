/******************************************************************************
 * FILE:			ADT_L1_A429_TX.c
 *
 * Copyright (c) 2007-2016, Alta Data Technologies LLC (ADT), All Rights Reserved.
 * Use of this software is subject to the ADT Software License (latest
 * revision), US Government or local laws and the ADT Terms and Conditions 
 * of Sale (latest revision).
 *
 * DESCRIPTION:
 *	Source file for Layer 1 API.
 *	Contains A429 transmit channel functions.
 *
 *****************************************************************************/
/*! \file ADT_L1_A429_TX.c  
 *  \brief Source file containing A429 transmit channel functions 
 */
#include "ADT_L1.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
  FUNCTION:		Internal_TX_GetMsgnumFromPtr
  Internal function that converts a TXCB Pointer to a message number.
  Returns message number or 0xFFFFFFFF to indicate failure.
 *****************************************************************************/
ADT_L0_UINT32 ADT_L0_CALL_CONV Internal_TX_GetMsgnumFromPtr(ADT_L0_UINT32 devID, ADT_L0_UINT32 TxChanNum, ADT_L0_UINT32 txcbPtr) {

	ADT_L0_UINT32 result = 0xFFFFFFFF;
	ADT_L0_UINT32 channel, channelRegOffset, txSetupOffset;
	ADT_L0_UINT32 max_num_msgs, txcbTablePtr, data, i, found;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if ((txcbPtr != 0) && (TxChanNum < 16))
	{
		/* Determine offset to the TX Channel Setup Regs */
		txSetupOffset = ADT_L1_A429_ROOT_TX_REGS + TxChanNum * ADT_L1_A429_TXREG_CHAN_SIZE;

		/* Read TXCB Table Pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_PTR, &txcbTablePtr, 1);
		if (txcbTablePtr != 0) 
		{
			/* Read max message number */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_SIZE, &max_num_msgs, 1);

			i = 0;  found = 0;
			while ((i < max_num_msgs) && (!found)) {
				result = ADT_L0_ReadMem32(devID, channelRegOffset + txcbTablePtr + i*4, &data, 1);
				if (data == txcbPtr)
					found = 1;
				else 
					i++;
			}
			if (found) result = i;
			else result = 0xFFFFFFFF;
		}
		else result = 0xFFFFFFFF;
	}
	else result = 0xFFFFFFFF;

	return( result );
}



/******************************************************************************
  FUNCTION:		Internal_TX_GetPtrFromMsgnum
  Internal function that converts a message number to a TXCB Pointer.
  Returns pointer or 0x00000000 for failure/no pointer.
 *****************************************************************************/
ADT_L0_UINT32 ADT_L0_CALL_CONV Internal_TX_GetPtrFromMsgnum(ADT_L0_UINT32 devID, ADT_L0_UINT32 TxChanNum, ADT_L0_UINT32 msgnum) {

	ADT_L0_UINT32 result = 0;
	ADT_L0_UINT32 channel, channelRegOffset, txSetupOffset;
	ADT_L0_UINT32 max_num_msgs, txcbTablePtr, data;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Determine offset to the TX Channel Setup Regs */
	txSetupOffset = ADT_L1_A429_ROOT_TX_REGS + TxChanNum * ADT_L1_A429_TXREG_CHAN_SIZE;

	/* Read max message number */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_SIZE, &max_num_msgs, 1);

	if (msgnum < max_num_msgs)
	{
		/* Read TXCB Table Pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_PTR, &txcbTablePtr, 1);
		if (txcbTablePtr != 0) 
		{
			/* Read the BC CB pointer for the message number */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + txcbTablePtr + msgnum*4, &data, 1);
			result = data;
		}
		else result = 0x00000000;
	}
	else result = 0x00000000;

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_A429_TX_Channel_Init
 *****************************************************************************/
/*! \brief Initializes data structures for an A429 Transmit Channel 
 *
 * This function initializes data structures for an A429 Transmit Channel.
 * This initializes the Root TX Registers for the channel and allocates 
 * a table of TXCBs.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param TxChanNum is the TX Channel number (0-15).
 * @param BitRateHz is the bit rate in Hz (500 to 500000).
 * @param numTXCB is the number of TXCB to allocate.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid Tx Channel number or invalid bit rate
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_TX_Channel_Init(ADT_L0_UINT32 devID, ADT_L0_UINT32 TxChanNum, ADT_L0_UINT32 BitRateHz, ADT_L0_UINT32 numTXCB) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, chanConfig, temp;
	ADT_L0_UINT32 i, data, txSetupOffset, size_needed, txcbOffset, halfBitTimeUs;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Make sure this channel is present/allowed */
	chanConfig = 0;
	result = ADT_L1_A429_GetConfig(devID, &chanConfig);
	if (result != ADT_SUCCESS) return(result);
	else {
		if (TxChanNum < 16) {
			temp = 0x10000;
			temp = temp << TxChanNum;
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

	if ((TxChanNum < 16) && (BitRateHz >=500) && (BitRateHz <= 500000) && (numTXCB > 0)) {
		/* Determine offset to the TX Channel Setup Regs */
		txSetupOffset = ADT_L1_A429_ROOT_TX_REGS + TxChanNum * ADT_L1_A429_TXREG_CHAN_SIZE;

		/* Clear the Root TX Registers */
		data = 0x00000000;
		for (i=0; i<ADT_L1_A429_TXREG_CHAN_SIZE; i+=4) {
			result = ADT_L0_WriteMem32(devID, channelRegOffset + txSetupOffset + i, &data, 1);
		}

		/* Initialize CSR1 and CSR2 registers for ARINC-429 settings */
		data = ADT_L1_A429_TXREG_TX_CSR1_STOPPED;  /* Set "Stopped" bit */
		result = ADT_L0_WriteMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_TX_CSR1, &data, 1);

		/* Set the provided bit rate - 10-bit period in microseconds */
		halfBitTimeUs = (ADT_L0_UINT32)((1.0 / (BitRateHz * 2.0))* 1000000) & 0x000003FF;
		/* NOTE: 0x80008430 = 32 bits per word, A429 high/low */
		data = 0x80008400 | (halfBitTimeUs << 16);

		/* if bit rate is greater than 20KHz, set high slew rate */
		if (BitRateHz > 20000) data |= ADT_L1_A429_TXREG_TX_CSR2_HISLEWRATE;

		result = ADT_L0_WriteMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_TX_CSR2, &data, 1);

		/* Write API TXCB Table Size */
		result = ADT_L0_WriteMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_SIZE, &numTXCB, 1);

		/* Allocate TXCB Table */
		size_needed = ADT_L1_A429_TXCB_SIZE * numTXCB;
		result = ADT_L1_MemoryAlloc(devID, size_needed, &txcbOffset);
		if (result == ADT_SUCCESS) {
			/* Clear the TXCB Table memory */
			data = 0x00000000;
			for (i=0; i<size_needed; i+=4) {
				result = ADT_L0_WriteMem32(devID, channelRegOffset + txcbOffset + i, &data, 1);
			}

			/* Write the API TXCB Table start pointer */
			result = ADT_L0_WriteMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_PTR, &txcbOffset, 1);

		}
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}




/******************************************************************************
  FUNCTION:		ADT_L1_A429_TX_Channel_Close
 *****************************************************************************/
/*! \brief Uninitializes data structures and frees memory for the TX Channel 
 *
 * This function uninitializes data structures and frees memory for the TX Channel.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param TxChanNum is the TX Channel number (0-15).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid Rx Channel number
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_TX_Channel_Close(ADT_L0_UINT32 devID, ADT_L0_UINT32 TxChanNum) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, numTXCB;
	ADT_L0_UINT32 i, j, k, data, txSetupOffset, size_needed, txcbTableOffset, txcbOffset, txpOffset, numTxP;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (TxChanNum >= 16) 
		return(ADT_ERR_BAD_INPUT);

	/* Determine offset to the TX Channel Setup Regs */
	txSetupOffset = ADT_L1_A429_ROOT_TX_REGS + TxChanNum * ADT_L1_A429_TXREG_CHAN_SIZE;

	/* Read the API TXCB Table Size */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_SIZE, &numTXCB, 1);

	/* Read the API TXCB Table Offset */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_PTR, &txcbTableOffset, 1);

	/* Iterate through TXCB Table to free memory used by TXCBs and TxPs */
	for (i=0; i<numTXCB; i++) {
		/* Read the TXCB pointer from the table */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + txcbTableOffset + i*4, &txcbOffset, 1);

		/* If pointer is non-zero, then there is a TXCB that needs to be freed */
		if (txcbOffset != 0x00000000) {
			/* Read the TXP Table pointer from the TXCB */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + txcbOffset + ADT_L1_A429_TXCB_API1STTXPPTR, &txpOffset, 1);

			/* Read the TXP Table size from the TXCB */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + txcbOffset + ADT_L1_A429_TXCB_APINUMTXPS, &numTxP, 1);

			/* Iterate through the TXP Table to clear memory */
			for (j=0; j<numTxP; j++) {
				/* Clear TXP memory */
				for (k=0; k<ADT_L1_A429_TXP_SIZE; k+=4) {
					data = 0x00000000;
					result = ADT_L0_WriteMem32(devID, channelRegOffset + txpOffset + j*ADT_L1_A429_TXP_SIZE + k, &data, 1);
				}
			}

			/* Free the memory for the TXP Table */
			size_needed = numTxP * ADT_L1_A429_TXP_SIZE;
			result = ADT_L1_MemoryFree(devID, txpOffset, size_needed);

			/* Clear the memory for the TXCB */
			for (j=0; j<ADT_L1_A429_TXCB_SIZE; j+=4) {
				data = 0x00000000;
				result = ADT_L0_WriteMem32(devID, channelRegOffset + txcbOffset + j, &data, 1);
			}

			/* Free the memory for the TXCB */
			result = ADT_L1_MemoryFree(devID, txcbOffset, ADT_L1_A429_TXCB_SIZE);

			/* Clear the TXCB pointer in the TXCB table */
			txcbOffset = 0x00000000;
			result = ADT_L0_WriteMem32(devID, channelRegOffset + txcbTableOffset + i*4, &txcbOffset, 1);
		}
	}

	/* Free the memory used by the TXCB Table */
	size_needed = ADT_L1_A429_TXCB_SIZE * numTXCB;
	result = ADT_L1_MemoryFree(devID, txcbTableOffset, size_needed);

	/* Clear the TX Channel Root Registers */
	data = 0x00000000;
	for (i=0; i<ADT_L1_A429_TXREG_CHAN_SIZE; i+=4) {
		result = ADT_L0_WriteMem32(devID, channelRegOffset + txSetupOffset + i, &data, 1);
	}

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_A429_TX_Channel_CB_TXPAllocate
 *****************************************************************************/
/*! \brief Allocates a TX Control Block and TXPs.
 *
 * This function allocates memory for a TX Control Block and TXPs.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param TxChanNum is the TX Channel number (0-15).
 * @param msgnum is the message number (zero-indexed) for which to allocate a TXCB.
 * @param numTXP is the number of TXPs to allocate and link for the TXCB.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid message number
	- \ref ADT_ERR_NO_TXCB_TABLE - TXCB table pointer is zero (table not allocated)
	- \ref ADT_ERR_TXCB_ALREADY_ALLOCATED - A TXCB has already been allocated for msgnum
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_TX_Channel_CB_TXPAllocate(ADT_L0_UINT32 devID, ADT_L0_UINT32 TxChanNum, ADT_L0_UINT32 msgnum, ADT_L0_UINT32 numTXP) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, txSetupOffset, size_needed;
	ADT_L0_UINT32 max_num_msgs, txcbTablePtr, data, newTxcbPtr, i;
	ADT_L0_UINT32 firstTxpPtr;
	ADT_L0_UINT32 memAvailable;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (TxChanNum >= 16)
		return(ADT_ERR_BAD_INPUT);

	/* Determine offset to the TX Channel Setup Regs */
	txSetupOffset = ADT_L1_A429_ROOT_TX_REGS + TxChanNum * ADT_L1_A429_TXREG_CHAN_SIZE;

	/* Read max message number, verify that msgnum is valid */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_SIZE, &max_num_msgs, 1);
	if (msgnum < max_num_msgs) 
	{
		/* Check for valid TXCB table pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_PTR, &txcbTablePtr, 1);
		if (txcbTablePtr != 0) 
		{
			/* Check to see if TXCB has already been allocated for the msgnum */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + txcbTablePtr + msgnum*4, &data, 1);
			if (data == 0)
			{
				/* Allocate a TXCB */
				result = ADT_L1_MemoryAlloc(devID, ADT_L1_A429_TXCB_SIZE, &newTxcbPtr);
				if (result == ADT_SUCCESS)
				{
					/* Write TXCB ptr to the TXCB table entry for this msgnum */
					result = ADT_L0_WriteMem32(devID, channelRegOffset + txcbTablePtr + msgnum*4, &newTxcbPtr, 1);

					/* Clear all words in the TXCB */
					data = 0;
					for (i=0; i<ADT_L1_A429_TXCB_SIZE; i+=4)
						result = ADT_L0_WriteMem32(devID, channelRegOffset + newTxcbPtr + i, &data, 1);

					/* Write the API message number to the TXCB */
					result = ADT_L0_WriteMem32(devID, channelRegOffset + newTxcbPtr + ADT_L1_A429_TXCB_APITXCBNUM, &msgnum, 1);

					/* Write the API number of TXPs to the TXCB */
					result = ADT_L0_WriteMem32(devID, channelRegOffset + newTxcbPtr + ADT_L1_A429_TXCB_APINUMTXPS, &numTXP, 1);

					/* Write the Total TXP Count to the TXCB */
					result = ADT_L0_WriteMem32(devID, channelRegOffset + newTxcbPtr + ADT_L1_A429_TXCB_TXPCOUNT, &numTXP, 1);

					/* Allocate the requested number of TXPs */
					size_needed = numTXP * ADT_L1_A429_TXP_SIZE;
					/* Check for sufficient meory before allocation */
					if ( ADT_L1_GetMemoryAvailable(devID, &memAvailable) == ADT_SUCCESS )
					{
						if(size_needed > memAvailable)
						{
							result = ADT_ERR_MEM_MGT_NO_MEM;
							return( result);
						}
					}
					result = ADT_L1_MemoryAlloc(devID, size_needed, &firstTxpPtr);

					/* Clear all words in the TXPs */
					data = 0;
					for (i=0; i<size_needed; i+=4)
						result = ADT_L0_WriteMem32(devID, channelRegOffset + firstTxpPtr + i, &data, 1);

						/* Set TXP CONTROL word to default A429 parity (On/Odd) settings */
						/* Set TXP RESERVED word to: channel number (22-31): TXCB number (12-21): TXP number (0-11) */
						for (i=0; i<numTXP; i++) {
							data = 0x00000030;
							result = ADT_L0_WriteMem32(devID, channelRegOffset + firstTxpPtr + i*ADT_L1_A429_TXP_SIZE + ADT_L1_A429_TXP_CONTROL, &data, 1);
							data = (TxChanNum << 22) | (msgnum << 12) | i ;
							result = ADT_L0_WriteMem32(devID, channelRegOffset + firstTxpPtr + i*ADT_L1_A429_TXP_SIZE + ADT_L1_A429_TXP_RESERVED, &data, 1);
						}


					/* Write the API first TXP pointer to the TXCB */
					result = ADT_L0_WriteMem32(devID, channelRegOffset + newTxcbPtr + ADT_L1_A429_TXCB_API1STTXPPTR, &firstTxpPtr, 1);

					/* Write the TXP table pointer to the TXCB */
					result = ADT_L0_WriteMem32(devID, channelRegOffset + newTxcbPtr + ADT_L1_A429_TXCB_TXPPTR, &firstTxpPtr, 1);

				}
			}
			else result = ADT_ERR_TXCB_ALREADY_ALLOCATED;  /* A TXCB has already been allocated for msgnum */
		}
		else result = ADT_ERR_NO_TXCB_TABLE;  /* No TXCB Table */
	}
	else result = ADT_ERR_BAD_INPUT;  /* Invalid msgnum */

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_A429_TX_Channel_CB_TXPFree
 *****************************************************************************/
/*! \brief Removes a TX Control Block and associated TXPs.
 *
 * This function frees memory for a TX Control Block and associated TXPs.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param TxChanNum is the TX Channel number (0-15).
 * @param msgnum is the message number (zero-indexed) of the TXCB to free.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid device number
	- \ref ADT_ERR_NO_TXCB_TABLE - TXCB table pointer is zero (table not allocated)
	- \ref ADT_ERR_TXCB_NOT_ALLOCATED - No BCCB has been allocated for msgnum
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_TX_Channel_CB_TXPFree(ADT_L0_UINT32 devID, ADT_L0_UINT32 TxChanNum, ADT_L0_UINT32 msgnum) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, txSetupOffset, numTXP, firstTxpPtr;
	ADT_L0_UINT32 max_num_msgs, txcbTablePtr, txcbPtr, size_needed, i, data;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (TxChanNum >= 16)
		return(ADT_ERR_BAD_INPUT);

	/* Determine offset to the TX Channel Setup Regs */
	txSetupOffset = ADT_L1_A429_ROOT_TX_REGS + TxChanNum * ADT_L1_A429_TXREG_CHAN_SIZE;

	/* Read max message number, verify that msgnum is valid */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_SIZE, &max_num_msgs, 1);
	if (msgnum < max_num_msgs) 
	{
		/* Check for valid TXCB table pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_PTR, &txcbTablePtr, 1);
		if (txcbTablePtr != 0) 
		{
			/* Check to see if TXCB has been allocated for the msgnum */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + txcbTablePtr + msgnum*4, &txcbPtr, 1);
			if (txcbPtr != 0)
			{
				/* Read the API number of TXPs from the TXCB */
				result = ADT_L0_ReadMem32(devID, channelRegOffset + txcbPtr + ADT_L1_A429_TXCB_APINUMTXPS, &numTXP, 1);

				/* Read the API first TXP pointer from the TXCB */
				result = ADT_L0_ReadMem32(devID, channelRegOffset + txcbPtr + ADT_L1_A429_TXCB_API1STTXPPTR, &firstTxpPtr, 1);

				/* Determine size of TXP table */
				size_needed = numTXP * ADT_L1_A429_TXP_SIZE;

				/* Clear all words in the TXP table */
				data = 0x00000000;
				for (i=0; i<size_needed; i+=4)
					result = ADT_L0_WriteMem32(devID, channelRegOffset + firstTxpPtr + i, &data, 1);

				/* Free the memory used by the TXP table */
				result = ADT_L1_MemoryFree(devID, firstTxpPtr, size_needed);

				/* Clear all words in the TXCB */
				data = 0x00000000;
				for (i=0; i<ADT_L1_A429_TXCB_SIZE; i+=4)
					result = ADT_L0_WriteMem32(devID, channelRegOffset + txcbPtr + i, &data, 1);

				/* Free the memory used by the TXCB */
				result = ADT_L1_MemoryFree(devID, txcbPtr, ADT_L1_A429_TXCB_SIZE);

				/* Clear the TXCB pointer in the TXCB table */
				txcbPtr = 0x00000000;
				result = ADT_L0_WriteMem32(devID, channelRegOffset + txcbTablePtr + msgnum*4, &txcbPtr, 1);

			}
			else result = ADT_ERR_TXCB_NOT_ALLOCATED;  /* A BCCB has not been allocated for msgnum */
		}
		else result = ADT_ERR_NO_TXCB_TABLE;  /* No BCCB Table */
	}
	else result = ADT_ERR_BAD_INPUT;  /* Invalid msgnum */

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_A429_TX_Channel_CB_Write
 *****************************************************************************/
/*! \brief Writes a TX Control Block 
 *
 * This function writes a TX Control Block.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param TxChanNum is the TX Channel number (0-15).
 * @param msgnum is the message number (zero-indexed) of the TXCB to write.
 * @param txcb is a pointer to the TXCB to write.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid message number
	- \ref ADT_ERR_NO_TXCB_TABLE - TXCB table pointer is zero (table not allocated)
	- \ref ADT_ERR_TXCB_NOT_ALLOCATED - No TXCB has been allocated for msgnum
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_TX_Channel_CB_Write(ADT_L0_UINT32 devID, ADT_L0_UINT32 TxChanNum, ADT_L0_UINT32 msgnum, ADT_L1_A429_TXCB *txcb) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, txSetupOffset, txCsr;
	ADT_L0_UINT32 max_num_msgs, txcbTablePtr, txcbPtr, data;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (TxChanNum >= 16)
		return(ADT_ERR_BAD_INPUT);

	/* Determine offset to the TX Channel Setup Regs */
	txSetupOffset = ADT_L1_A429_ROOT_TX_REGS + TxChanNum * ADT_L1_A429_TXREG_CHAN_SIZE;

	/* Read max message number, verify that msgnum is valid */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_SIZE, &max_num_msgs, 1);
	if (msgnum < max_num_msgs) 
	{
		/* Check for valid TXCB table pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_PTR, &txcbTablePtr, 1);
		if (txcbTablePtr != 0) 
		{
			/* Check to see if TXCB has been allocated for the msgnum */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + txcbTablePtr + msgnum*4, &txcbPtr, 1);
			if (txcbPtr != 0)
			{
				/* Write the TXCB - Next TXCB Pointer */
				data = Internal_TX_GetPtrFromMsgnum(devID, TxChanNum, txcb->NextTxcbNum);
				result = ADT_L0_WriteMem32(devID, channelRegOffset + txcbPtr + ADT_L1_A429_TXCB_NEXTPTR, &data, 1);

				/* Write the TXCB - Control Word */
				data = txcb->Control;
				result = ADT_L0_WriteMem32(devID, channelRegOffset + txcbPtr + ADT_L1_A429_TXCB_CONTROL, &data, 1);

				/* Read the TX CSR1 */
				txCsr = 0;
				result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_TX_CSR1, &txCsr, 1);

				/* Check the "TX STOPPED" bit (bit 1) */
				if (txCsr & ADT_L1_A429_TXREG_TX_CSR1_STOPPED)
				{
					/* Write the TXCB - TX Period and first TX Time Value - ONLY IF CHANNEL IS NOT RUNNING */
					data = txcb->TxPeriod500us;
					result = ADT_L0_WriteMem32(devID, channelRegOffset + txcbPtr + ADT_L1_A429_TXCB_TXTIMEINCREMENT, &data, 1);
					result = ADT_L0_WriteMem32(devID, channelRegOffset + txcbPtr + ADT_L1_A429_TXCB_TXTIMEVALUE, &data, 1);
				}

				/* We do not write to the API words (num TXPs, message number, etc.) */

			}
			else result = ADT_ERR_TXCB_NOT_ALLOCATED;  /* A TXCB has not been allocated for msgnum */
		}
		else result = ADT_ERR_NO_TXCB_TABLE;  /* No TXCB Table */
	}
	else result = ADT_ERR_BAD_INPUT;  /* Invalid message number */

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_A429_TX_Channel_CB_Read
 *****************************************************************************/
/*! \brief Reads a TX Control Block 
 *
 * This function reads a TX Control Block.
 * 
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param TxChanNum is the TX Channel number (0-15).
 * @param msgnum is the message number (zero-indexed) of the BCCB to read.
 * @param txcb is a pointer to store the BCCB read.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid message number
	- \ref ADT_ERR_NO_TXCB_TABLE - TXCB table pointer is zero (table not allocated)
	- \ref ADT_ERR_TXCB_NOT_ALLOCATED - No TXCB has been allocated for msgnum
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_TX_Channel_CB_Read(ADT_L0_UINT32 devID,  ADT_L0_UINT32 TxChanNum, ADT_L0_UINT32 msgnum, ADT_L1_A429_TXCB *txcb) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, txSetupOffset;
	ADT_L0_UINT32 max_num_msgs, txcbTablePtr, txcbPtr, data;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (TxChanNum >= 16)
		return(ADT_ERR_BAD_INPUT);

	/* Determine offset to the TX Channel Setup Regs */
	txSetupOffset = ADT_L1_A429_ROOT_TX_REGS + TxChanNum * ADT_L1_A429_TXREG_CHAN_SIZE;

	/* Read max message number, verify that msgnum is valid */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_SIZE, &max_num_msgs, 1);
	if (msgnum < max_num_msgs) 
	{
		/* Check for valid TXCB table pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_PTR, &txcbTablePtr, 1);
		if (txcbTablePtr != 0) 
		{
			/* Check to see if TXCB has been allocated for the msgnum */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + txcbTablePtr + msgnum*4, &txcbPtr, 1);
			if (txcbPtr != 0)
			{
				/* Read the TXCB - Next TXCB Pointer */
				result = ADT_L0_ReadMem32(devID, channelRegOffset + txcbPtr + ADT_L1_A429_TXCB_NEXTPTR, &data, 1);
				txcb->NextTxcbNum = Internal_TX_GetMsgnumFromPtr(devID, TxChanNum, data);

				/* Read the TXCB - Control Word */
				result = ADT_L0_ReadMem32(devID, channelRegOffset + txcbPtr + ADT_L1_A429_TXCB_CONTROL, &data, 1);
				txcb->Control = data;

				/* Read the TXCB - TX Period */
				result = ADT_L0_ReadMem32(devID, channelRegOffset + txcbPtr + ADT_L1_A429_TXCB_TXTIMEINCREMENT, &data, 1);
				txcb->TxPeriod500us = data;

				/* TXCB message number is just for user reference, not used otherwise (msg number specified in TXCB function parameters */
				txcb->TxcbNum = msgnum;


				/* We do not read to the API words (num TXPs, etc.) */

			}
			else result = ADT_ERR_TXCB_NOT_ALLOCATED;  /* A TXCB has not been allocated for msgnum */
		}
		else result = ADT_ERR_NO_TXCB_TABLE;  /* No TXCB Table */
	}
	else result = ADT_ERR_BAD_INPUT;  /* Invalid message number */

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_A429_TX_Channel_CB_TXPWrite
 *****************************************************************************/
/*! \brief Writes a TXP for a TX Control Block 
 *
 * This function writes a TXP for TX Control Block.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param TxChanNum is the TX Channel number (0-15).
 * @param msgnum is the message (TXCB) number.
 * @param txpNum is the buffer (TXP) number.
 * @param pTxp is a pointer to the TXP structure.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid message number
	- \ref ADT_ERR_NO_TXCB_TABLE - BCCB table pointer is zero (table not allocated)
	- \ref ADT_ERR_TXCB_NOT_ALLOCATED - No BCCB has been allocated for msgnum
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_TX_Channel_CB_TXPWrite(ADT_L0_UINT32 devID, ADT_L0_UINT32 TxChanNum, ADT_L0_UINT32 msgnum, ADT_L0_UINT32 txpNum, ADT_L1_A429_TXP *pTxp) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, txSetupOffset;
	ADT_L0_UINT32 max_num_msgs, txcbTablePtr, txcbPtr;
	ADT_L0_UINT32 temp, startTxp, selectedTxp, i;

	max_num_msgs = txcbTablePtr = txcbPtr = 0;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (TxChanNum >= 16)
		return(ADT_ERR_BAD_INPUT);

	/* Determine offset to the TX Channel Setup Regs */
	txSetupOffset = ADT_L1_A429_ROOT_TX_REGS + TxChanNum * ADT_L1_A429_TXREG_CHAN_SIZE;

	/* Read max message number, verify that msgnum is valid */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_SIZE, &max_num_msgs, 1);
	if (msgnum < max_num_msgs) 
	{
		/* Check for valid TXCB table pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_PTR, &txcbTablePtr, 1);
		if (txcbTablePtr != 0) 
		{
			/* Check to see if TXCB has been allocated for the msgnum */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + txcbTablePtr + msgnum*4, &txcbPtr, 1);
			if (txcbPtr != 0)
			{
				/* Check that the selected TXP number does not exceed the actual number of TXPs */
				temp = txcbPtr + ADT_L1_A429_TXCB_APINUMTXPS;
				result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &i, 1);
				if (txpNum >= i) {
					result = ADT_ERR_BAD_INPUT;
				}

				if (result == ADT_SUCCESS) {
					/* Read the API starting TXP address */
					temp = txcbPtr + ADT_L1_A429_TXCB_API1STTXPPTR;
					result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &startTxp, 1);

					/* Offset to the selected TXP */
					selectedTxp = startTxp + txpNum * ADT_L1_A429_TXP_SIZE;

					if (result == ADT_SUCCESS) {
						/* Write the selected TXP */
						/* Only allow user to write low 6 bits of control word */

						/* old:
						result = ADT_L0_ReadMem32(devID, channelRegOffset + selectedTxp + ADT_L1_A429_TXP_CONTROL, &temp, 1);
						temp &= 0xFFFFFFC0;
						temp |= pTxp->Control & 0x0000003F;
						*/

						/* new: */
						temp = pTxp->Control & 0x0000003F;


						result = ADT_L0_WriteMem32(devID, channelRegOffset + selectedTxp + ADT_L1_A429_TXP_CONTROL, &temp, 1);
						result = ADT_L0_WriteMem32(devID, channelRegOffset + selectedTxp + ADT_L1_A429_TXP_DELAY, &pTxp->Delay, 1);
						result = ADT_L0_WriteMem32(devID, channelRegOffset + selectedTxp + ADT_L1_A429_TXP_DATA, &pTxp->Data, 1);
					}
				}
			}
			else result = ADT_ERR_TXCB_NOT_ALLOCATED;  /* A TXCB has not been allocated for msgnum */
		}
		else result = ADT_ERR_NO_TXCB_TABLE;  /* No TXCB Table */
	}
	else result = ADT_ERR_BAD_INPUT;  /* Invalid message number */

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_A429_TX_Channel_CB_TXPRead
 *****************************************************************************/
/*! \brief Reads a TXP for a TX Control Block 
 *
 * This function reads a TXP for TX Control Block.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param TxChanNum is the TX Channel number (0-15).
 * @param msgnum is the message (TXCB) number.
 * @param txpNum is the buffer (TXP) number.
 * @param pTxp is a pointer to the TXP structure.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid message number
	- \ref ADT_ERR_NO_TXCB_TABLE - BCCB table pointer is zero (table not allocated)
	- \ref ADT_ERR_TXCB_NOT_ALLOCATED - No BCCB has been allocated for msgnum
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_TX_Channel_CB_TXPRead(ADT_L0_UINT32 devID, ADT_L0_UINT32 TxChanNum, ADT_L0_UINT32 msgnum, ADT_L0_UINT32 txpNum, ADT_L1_A429_TXP *pTxp) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, txSetupOffset;
	ADT_L0_UINT32 max_num_msgs, txcbTablePtr, txcbPtr;
	ADT_L0_UINT32 temp, startTxp, selectedTxp, i;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (TxChanNum >= 16)
		return(ADT_ERR_BAD_INPUT);

	/* Determine offset to the TX Channel Setup Regs */
	txSetupOffset = ADT_L1_A429_ROOT_TX_REGS + TxChanNum * ADT_L1_A429_TXREG_CHAN_SIZE;

	/* Read max message number, verify that msgnum is valid */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_SIZE, &max_num_msgs, 1);
	if (msgnum < max_num_msgs) 
	{
		/* Check for valid TXCB table pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_PTR, &txcbTablePtr, 1);
		if (txcbTablePtr != 0) 
		{
			/* Check to see if TXCB has been allocated for the msgnum */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + txcbTablePtr + msgnum*4, &txcbPtr, 1);
			if (txcbPtr != 0)
			{
				/* Check that the selected TXP number does not exceed the actual number of TXPs */
				temp = txcbPtr + ADT_L1_A429_TXCB_APINUMTXPS;
				result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &i, 1);
				if (txpNum >= i) {
					result = ADT_ERR_BAD_INPUT;
				}

				if (result == ADT_SUCCESS) {
					/* Read the API starting TXP address */
					temp = txcbPtr + ADT_L1_A429_TXCB_API1STTXPPTR;
					result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &startTxp, 1);

					/* Offset to the selected TXP */
					selectedTxp = startTxp + txpNum * ADT_L1_A429_TXP_SIZE;

					if (result == ADT_SUCCESS) {
						/* Read the selected TXP */
						result = ADT_L0_ReadMem32(devID, channelRegOffset + selectedTxp + ADT_L1_A429_TXP_CONTROL, &pTxp->Control, 1);
						result = ADT_L0_ReadMem32(devID, channelRegOffset + selectedTxp + ADT_L1_A429_TXP_DELAY, &pTxp->Delay, 1);
						result = ADT_L0_ReadMem32(devID, channelRegOffset + selectedTxp + ADT_L1_A429_TXP_DATA, &pTxp->Data, 1);
					}
				}
			}
			else result = ADT_ERR_TXCB_NOT_ALLOCATED;  /* A TXCB has not been allocated for msgnum */
		}
		else result = ADT_ERR_NO_TXCB_TABLE;  /* No TXCB Table */
	}
	else result = ADT_ERR_BAD_INPUT;  /* Invalid message number */

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_A429_TX_Channel_Start
 *****************************************************************************/
/*! \brief Starts TX operation for the TX channel 
 *
 * This function starts TX operation for TX channel on the selected message (TXCB) number.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param TxChanNum is the TX Channel number (0-15).
 * @param msgnum is the message (TXCB) number to start on.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid message number or channel number
	- \ref ADT_ERR_NO_TXCB_TABLE - TXCB table pointer is zero (table not allocated)
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_TX_Channel_Start(ADT_L0_UINT32 devID, ADT_L0_UINT32 TxChanNum, ADT_L0_UINT32 msgnum) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, txSetupOffset;
	ADT_L0_UINT32 max_num_msgs, txcbTablePtr, data;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (TxChanNum >= 16)
		return(ADT_ERR_BAD_INPUT);

	/* Determine offset to the TX Channel Setup Regs */
	txSetupOffset = ADT_L1_A429_ROOT_TX_REGS + TxChanNum * ADT_L1_A429_TXREG_CHAN_SIZE;

	/* Read max message number, verify that msgnum is valid */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_SIZE, &max_num_msgs, 1);
	if (msgnum < max_num_msgs) 
	{
		/* Check for valid TXCB table pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_PTR, &txcbTablePtr, 1);
		if (txcbTablePtr != 0) 
		{
			/* Write the starting message pointer */
			data = Internal_TX_GetPtrFromMsgnum(devID, TxChanNum, msgnum);
			result = ADT_L0_WriteMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_1ST_TXCB_PTR, &data, 1);

			/* Clear the current message pointer */
			data = 0x00000000;
			result = ADT_L0_WriteMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_CUR_TXCB_PTR, &data, 1);

			/* Read the TX CSR1 */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_TX_CSR1, &data, 1);

			/* Set bit 0 (start TX CH) and clear bit 1 (TX CH stopped) */
			data &= ~ADT_L1_A429_TXREG_TX_CSR1_STOPPED;
			data |= ADT_L1_A429_TXREG_TX_CSR1_START;

			/* Write the TX CSR */
			result = ADT_L0_WriteMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_TX_CSR1, &data, 1);
		}
		else result = ADT_ERR_NO_TXCB_TABLE;  /* No TXCB Table */
	}
	else result = ADT_ERR_BAD_INPUT;  /* Invalid message number */

	return( result );
}

/******************************************************************************
  FUNCTION:		ADT_L1_A429_TX_Channel_AperiodicSend
 *****************************************************************************/
/*! \brief Posts Aperiodic TXCB for Execution
 *
 * This function Posts Aperiodic TXCB for Execution on the selected message (TXCB) number.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param TxChanNum is the TX Channel number (0-15).
 * @param msgnum is the message (TXCB) number to start on.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid message number or channel number
	- \ref ADT_ERR_NO_TXCB_TABLE - TXCB table pointer is zero (table not allocated)
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_TX_Channel_AperiodicSend(ADT_L0_UINT32 devID, ADT_L0_UINT32 TxChanNum, ADT_L0_UINT32 msgnum) 
{
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, txSetupOffset;
	ADT_L0_UINT32 max_num_msgs, txcbTablePtr, data;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (TxChanNum >= 16)
		return(ADT_ERR_BAD_INPUT);

	/* Determine offset to the TX Channel Setup Regs */
	txSetupOffset = ADT_L1_A429_ROOT_TX_REGS + TxChanNum * ADT_L1_A429_TXREG_CHAN_SIZE;

	/* Read max message number, verify that msgnum is valid */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_SIZE, &max_num_msgs, 1);
	if (msgnum < max_num_msgs) 
	{
		/* Check for valid TXCB table pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_PTR, &txcbTablePtr, 1);
		if (txcbTablePtr != 0) 
		{
			/* Write the starting message pointer */
			data = Internal_TX_GetPtrFromMsgnum(devID, TxChanNum, msgnum);
			result = ADT_L0_WriteMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_APERIODIC_TXP, &data, 1);
		}
		else result = ADT_ERR_NO_TXCB_TABLE;  /* No TXCB Table */
	}
	else result = ADT_ERR_BAD_INPUT;  /* Invalid message number */

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_A429_TX_Channel_Stop
 *****************************************************************************/
/*! \brief Stops TX operation for the TX channel 
 *
 * This function stops TX operation for TX channel.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param TxChanNum is the TX Channel number (0-15).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid channel number
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_TX_Channel_Stop(ADT_L0_UINT32 devID, ADT_L0_UINT32 TxChanNum) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, txSetupOffset;
	ADT_L0_UINT32 data, timeout;
	ADT_L0_UINT32 txcbTablePtr, txcbPtr, max_num_msgs, msgnum, txTimeVal;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (TxChanNum >= 16)
		return(ADT_ERR_BAD_INPUT);

	/* Determine offset to the TX Channel Setup Regs */
	txSetupOffset = ADT_L1_A429_ROOT_TX_REGS + TxChanNum * ADT_L1_A429_TXREG_CHAN_SIZE;

	/* Read the TX CSR1 */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_TX_CSR1, &data, 1);

	/* Clear bit 0 (start TX) */
	data &= ~ADT_L1_A429_TXREG_TX_CSR1_START;

	/* Write the TX CSR */
	result = ADT_L0_WriteMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_TX_CSR1, &data, 1);

	/* Wait for the TX channel to stop */
	timeout = 0;
	result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_TX_CSR1, &data, 1);
	while (((data & ADT_L1_A429_TXREG_TX_CSR1_STOPPED) == 0x00000000) && (timeout < 100)) 
	{
		ADT_L0_msSleep(1);  timeout++;
		result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_TX_CSR1, &data, 1);
	}

	/***** Traverse the list of all TXCB to reset the TX Time Value to the TX Time Increment Value *****/
	/* Determine offset to the TX Channel Setup Regs */
	txSetupOffset = ADT_L1_A429_ROOT_TX_REGS + TxChanNum * ADT_L1_A429_TXREG_CHAN_SIZE;

	/* Read max message number */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_SIZE, &max_num_msgs, 1);

	/* Check for valid TXCB table pointer */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_PTR, &txcbTablePtr, 1);
	if (txcbTablePtr != 0) 
	{
		/* Iterate through all valid message numbers */
		for (msgnum=0; msgnum<max_num_msgs; msgnum++) 
		{
			/* Check to see if TXCB has been allocated for the msgnum */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + txcbTablePtr + msgnum*4, &txcbPtr, 1);
			if (txcbPtr != 0)
			{
				/* Read the TX Time Increment Value and write it to the TX Time Value */
				result = ADT_L0_ReadMem32(devID, channelRegOffset + txcbPtr + ADT_L1_A429_TXCB_TXTIMEINCREMENT, &txTimeVal, 1);
				result = ADT_L0_WriteMem32(devID, channelRegOffset + txcbPtr + ADT_L1_A429_TXCB_TXTIMEVALUE, &txTimeVal, 1);
			}
		}
	}

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_A429_TX_Channel_IsRunning
 *****************************************************************************/
/*! \brief Determines if the TX channel is running or stopped 
 *
 * This function determines if the TX channel is running or stopped.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param TxChanNum is the TX Channel number (0-15).
 * @param pIsRunning is a pointer to store the result, 0=NotRunning, 1=Running.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid channel number
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_TX_Channel_IsRunning(ADT_L0_UINT32 devID, ADT_L0_UINT32 TxChanNum, ADT_L0_UINT32 *pIsRunning) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, txCsr, txSetupOffset;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (TxChanNum >= 16)
		return(ADT_ERR_BAD_INPUT);

	/* Determine offset to the TX Channel Setup Regs */
	txSetupOffset = ADT_L1_A429_ROOT_TX_REGS + TxChanNum * ADT_L1_A429_TXREG_CHAN_SIZE;

	/* Read the TX CSR1 */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_TX_CSR1, &txCsr, 1);

	/* Check the "TX STOPPED" bit (bit 1) */
	if (txCsr & ADT_L1_A429_TXREG_TX_CSR1_STOPPED)
		*pIsRunning = 0;
	else
		*pIsRunning = 1;

	return( result );
}

/******************************************************************************
  FUNCTION:		ADT_L1_A429_TX_Channel_AperiodicIsRunning
 *****************************************************************************/
/*! \brief Determines if the TX channel is currently executing an Aperiodic list
 *
 * This function determines if the TX channel is currently executing an Aperiodic list.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param TxChanNum is the TX Channel number (0-15).
 * @param pIsRunning is a pointer to store the result, 0=NotRunning, 1=Running.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid channel number
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_TX_Channel_AperiodicIsRunning(ADT_L0_UINT32 devID, ADT_L0_UINT32 TxChanNum, ADT_L0_UINT32 *pIsRunning) 
{
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, txAperiodicPtr, txAperiodicOffset;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (TxChanNum >= 16)
		return(ADT_ERR_BAD_INPUT);

	/* Determine offset to the TX Channel Aperiodic TXCB Pointer */
	txAperiodicOffset = ADT_L1_A429_ROOT_TX_REGS + (TxChanNum * ADT_L1_A429_TXREG_CHAN_SIZE)  + ADT_L1_A429_TXREG_APERIODIC_TXP;

	/* Read the TX Aperiodic Ptr Reg */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + txAperiodicOffset, &txAperiodicPtr, 1);

	/* Check the Aperiodic Prt - will be zero if no current Aperiodic is running */
	if (txAperiodicPtr == 0)
		*pIsRunning = 0;
	else
		*pIsRunning = 1;

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_A429_TX_Channel_SendLabel
 *****************************************************************************/
/*! \brief Sends a Label (as aperiodic or one-shot) 
 *
 * This function sends a Label (as aperiodic or one-shot).
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param TxChanNum is the TX Channel number (0-15).
 * @param Label is the 32-bit A429 Label word to send.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid channel number
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_ERR_TIMEOUT - Timeout waiting for APERIODIC TXP register to clear
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_TX_Channel_SendLabel(ADT_L0_UINT32 devID, ADT_L0_UINT32 TxChanNum, ADT_L0_UINT32 Label) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, txCsr1, txCsr2, txSetupOffset, halfBitTimeUs, tempTxcbTxpOffset, temp, temp2, i, counter;
	ADT_L1_A429_TXP tempTXP;
	ADT_L0_UINT32 timeHigh, timeLow, tempLow, timeOffset;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (TxChanNum >= 16)
		return(ADT_ERR_BAD_INPUT);

	/* Determine offset to the TX Channel Setup Regs */
	txSetupOffset = ADT_L1_A429_ROOT_TX_REGS + TxChanNum * ADT_L1_A429_TXREG_CHAN_SIZE;

	/* Read the TX CSR1 */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_TX_CSR1, &txCsr1, 1);

	/* Read the TX CSR2 */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_TX_CSR2, &txCsr2, 1);
	halfBitTimeUs = (txCsr2 & 0x03FF0000) >> 16;

	/* Define the TXP for the Label */
	tempTXP.Control = ADT_L1_A429_TXP_CONTROL_PARITYON | ADT_L1_A429_TXP_CONTROL_PARITYODD;
	tempTXP.Reserved = ADT_L1_A429_APIAPERIODICSET;
	tempTXP.Delay = halfBitTimeUs * 8 * 10;  /* 4 bit-times, 100ns LSB */
	tempTXP.Data = Label;


	/* Allocate board memory for the temp TXCB + TXP */
	result = ADT_L1_MemoryAlloc(devID, ADT_L1_A429_TXCB_SIZE + ADT_L1_A429_TXP_SIZE, &tempTxcbTxpOffset);
	if (result == ADT_SUCCESS) 
	{
		/* clear the TXCB memory */
		temp = tempTxcbTxpOffset;
		temp2 = 0;
		for (i = 0; i < ADT_L1_A429_TXCB_SIZE/4; i++)
		{
			result = ADT_L0_WriteMem32(devID, channelRegOffset + temp, &temp2, 1);
			temp += 4;
		}
		
		/* Only 2 Fields of the TXCB need to be set: TXP Table Pointer and Num of TXPs */
		temp = tempTxcbTxpOffset + ADT_L1_A429_TXCB_SIZE; /* Should be TXP */
		result = ADT_L0_WriteMem32(devID, channelRegOffset + tempTxcbTxpOffset + ADT_L1_A429_TXCB_TXPPTR, &temp, 1);
		temp = 1; 
		result = ADT_L0_WriteMem32(devID, channelRegOffset + tempTxcbTxpOffset + ADT_L1_A429_TXCB_TXPCOUNT, &temp, 1);
		
		/* Write the TXP to board memory */
		result = ADT_L0_WriteMem32(devID, channelRegOffset + tempTxcbTxpOffset + ADT_L1_A429_TXCB_SIZE + ADT_L1_A429_TXP_CONTROL, &tempTXP.Control, 1);
		result = ADT_L0_WriteMem32(devID, channelRegOffset + tempTxcbTxpOffset + ADT_L1_A429_TXCB_SIZE + ADT_L1_A429_TXP_RESERVED, &tempTXP.Reserved, 1);
		result = ADT_L0_WriteMem32(devID, channelRegOffset + tempTxcbTxpOffset + ADT_L1_A429_TXCB_SIZE + ADT_L1_A429_TXP_DELAY, &tempTXP.Delay, 1);
		result = ADT_L0_WriteMem32(devID, channelRegOffset + tempTxcbTxpOffset + ADT_L1_A429_TXCB_SIZE + ADT_L1_A429_TXP_DATA, &tempTXP.Data, 1);

		/* If the channel is NOT RUNNING, send one-shot label */
		if ((txCsr1 & ADT_L1_A429_TXREG_TX_CSR1_STOPPED) || (!(txCsr1 & ADT_L1_A429_TXREG_TX_CSR1_START)))
		{
			/* Clear 1st TXCB PTR so sending one-shot does not start cyclic list (added in v3.0.1.1) */
			temp = 0;
			result = ADT_L0_WriteMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_1ST_TXCB_PTR, &temp, 1);

			/* Write the offset to the APERIODIC TXP register */
			result = ADT_L0_WriteMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_APERIODIC_TXP, &tempTxcbTxpOffset, 1);

			/* Start the channel */
			txCsr1 &= ~ADT_L1_A429_TXREG_TX_CSR1_STOPPED;
			txCsr1 |= ADT_L1_A429_TXREG_TX_CSR1_START;
			result = ADT_L0_WriteMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_TX_CSR1, &txCsr1, 1);
		}
		/* else, the channel IS RUNNING, send aperiodic label */
		else 
		{
			/* Write the offset to the APERIODIC TXCB register */
			result = ADT_L0_WriteMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_APERIODIC_TXP, &tempTxcbTxpOffset, 1);
		}

		/* Wait for APERIODIC TXCB register to clear, indicating that the firmware has started processing the aperiodic label */
		temp = 0xFFFFFFFF;
		counter = 0;
		while ((temp != 0) && (counter < 100000))
		{
			result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_APERIODIC_TXP, &temp, 1);
			counter++;
		}

		/* Read time to get a more precise delay (more precise than Sleep) */
		/* Wait two word times (36 bits) based on the halfBitTime */
		result = ADT_L1_A429_TimeGet(devID, &timeHigh, &timeLow);

		/* Use offset to avoid rollover of timeLow, this forces our timeLow
		   value to the lower half of the range so we won't encounter a
		   rollover during our short delay of two word times.
		 */
		if (timeLow > 0x7FFFFFFF) timeOffset = 0x80000000;
		else timeOffset = 0;

		tempLow = timeLow + (2 * halfBitTimeUs * 2 * 36 * 50) + timeOffset;

		while ((timeLow + timeOffset) < tempLow)
		{
			result = ADT_L1_A429_TimeGet(devID, &timeHigh, &timeLow);
		}

		/* Free the board memory used by the temp TXCB */
		result = ADT_L1_MemoryFree(devID, tempTxcbTxpOffset, ADT_L1_A429_TXCB_SIZE + ADT_L1_A429_TXP_SIZE);

		if (counter >= 100000) result = ADT_ERR_TIMEOUT;
	}

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_A429_TX_Channel_SendLabelBlock
 *****************************************************************************/
/*! \brief Sends a block of Labels (as aperiodic or one-shot) 
 *
 * This function sends a block of Labels (as aperiodic or one-shot).
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param TxChanNum is the TX Channel number (0-15).
 * @param numLables is the number of Label words to send.
 * @param pLabels is a pointer to the 32-bit A429 Label words to send.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid channel number or too many labels or null pointer
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_ERR_TIMEOUT - Timeout waiting for APERIODIC TXP register to clear
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_TX_Channel_SendLabelBlock(ADT_L0_UINT32 devID, ADT_L0_UINT32 TxChanNum, ADT_L0_UINT32 numLabels, ADT_L0_UINT32 *pLabels) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, txCsr1, txCsr2, txSetupOffset, halfBitTimeUs, tempTxcbTxpOffset, temp, temp2, i, counter;
	ADT_L1_A429_TXP tempTXP;
	ADT_L0_UINT32 *pMyLabels;
	ADT_L0_UINT32 timeHigh, timeLow, tempLow, timeOffset;

	/* Local variable to store pointer to Labels */
	pMyLabels = pLabels;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if ((TxChanNum >= 16) || (numLabels > 1000) || (pLabels = NULL))
		return(ADT_ERR_BAD_INPUT);

	/* Determine offset to the TX Channel Setup Regs */
	txSetupOffset = ADT_L1_A429_ROOT_TX_REGS + TxChanNum * ADT_L1_A429_TXREG_CHAN_SIZE;

	/* Read the TX CSR1 */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_TX_CSR1, &txCsr1, 1);

	/* Read the TX CSR2 */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_TX_CSR2, &txCsr2, 1);
	halfBitTimeUs = (txCsr2 & 0x03FF0000) >> 16;


	/* Allocate board memory for the temp TXCB + TXPs */
	result = ADT_L1_MemoryAlloc(devID, ADT_L1_A429_TXCB_SIZE + (ADT_L1_A429_TXP_SIZE * numLabels), &tempTxcbTxpOffset);
	if (result == ADT_SUCCESS) 
	{
		/* clear the TXCB memory */
		temp = tempTxcbTxpOffset;
		temp2 = 0;
		for (i = 0; i < ADT_L1_A429_TXCB_SIZE/4; i++)
		{
			result = ADT_L0_WriteMem32(devID, channelRegOffset + temp, &temp2, 1);
			temp += 4;
		}
		
		/* Only 2 Fields of the TXCB need to be set: TXP Table Pointer and Num of TXPs */
		temp = tempTxcbTxpOffset + ADT_L1_A429_TXCB_SIZE; /* Should be first TXP */
		result = ADT_L0_WriteMem32(devID, channelRegOffset + tempTxcbTxpOffset + ADT_L1_A429_TXCB_TXPPTR, &temp, 1);
		temp = numLabels; 
		result = ADT_L0_WriteMem32(devID, channelRegOffset + tempTxcbTxpOffset + ADT_L1_A429_TXCB_TXPCOUNT, &temp, 1);
		/* temp = 0; */
		/* result = ADT_L0_WriteMem32(devID, channelRegOffset + tempTxcbTxpOffset + ADT_L1_A429_TXCB_PECOUNT, &temp, 1); */

		/* Define the TXPs */
		for (i=0; i<numLabels; i++) {
			/* Define the TXP for the Label */
			tempTXP.Control = ADT_L1_A429_TXP_CONTROL_PARITYON | ADT_L1_A429_TXP_CONTROL_PARITYODD;
			tempTXP.Reserved = ADT_L1_A429_APIAPERIODICSET;
			tempTXP.Delay = halfBitTimeUs * 8 * 10;  /* 4 bit-times, 100ns LSB */
			tempTXP.Data = pMyLabels[i];

			/* Write the TXP to board memory */
			result = ADT_L0_WriteMem32(devID, channelRegOffset + tempTxcbTxpOffset + ADT_L1_A429_TXCB_SIZE + (ADT_L1_A429_TXP_SIZE * i) + ADT_L1_A429_TXP_CONTROL, &tempTXP.Control, 1);
			result = ADT_L0_WriteMem32(devID, channelRegOffset + tempTxcbTxpOffset + ADT_L1_A429_TXCB_SIZE + (ADT_L1_A429_TXP_SIZE * i) + ADT_L1_A429_TXP_RESERVED, &tempTXP.Reserved, 1);
			result = ADT_L0_WriteMem32(devID, channelRegOffset + tempTxcbTxpOffset + ADT_L1_A429_TXCB_SIZE + (ADT_L1_A429_TXP_SIZE * i) + ADT_L1_A429_TXP_DELAY, &tempTXP.Delay, 1);
			result = ADT_L0_WriteMem32(devID, channelRegOffset + tempTxcbTxpOffset + ADT_L1_A429_TXCB_SIZE + (ADT_L1_A429_TXP_SIZE * i) + ADT_L1_A429_TXP_DATA, &tempTXP.Data, 1);
		}

		/* If the channel is NOT RUNNING, send one-shot label */
		if ((txCsr1 & ADT_L1_A429_TXREG_TX_CSR1_STOPPED) || (!(txCsr1 & ADT_L1_A429_TXREG_TX_CSR1_START)))
		{
			/* Clear 1st TXCB PTR so sending one-shot does not start cyclic list (added in v3.0.1.1) */
			temp = 0;
			result = ADT_L0_WriteMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_1ST_TXCB_PTR, &temp, 1);

			/* Write the offset to the APERIODIC TXP register */
			result = ADT_L0_WriteMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_APERIODIC_TXP, &tempTxcbTxpOffset, 1);

			/* Start the channel */
			txCsr1 &= ~ADT_L1_A429_TXREG_TX_CSR1_STOPPED;
			txCsr1 |= ADT_L1_A429_TXREG_TX_CSR1_START;
			result = ADT_L0_WriteMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_TX_CSR1, &txCsr1, 1);
		}
		/* else, the channel IS RUNNING, send aperiodic label */
		else 
		{
			/* Write the offset to the APERIODIC TXCB register */
			result = ADT_L0_WriteMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_APERIODIC_TXP, &tempTxcbTxpOffset, 1);
		}

		/* Wait for APERIODIC TXCB register to clear, indicating that the firmware has started processing the aperiodic label */
		temp = 0xFFFFFFFF;
		counter = 0;
		while ((temp != 0) && (counter < 100000))
		{
			result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_APERIODIC_TXP, &temp, 1);
			counter++;
		}

		if (counter >= 100000)
		{
			result = ADT_ERR_TIMEOUT;
		}
		else
		{
			/* Wait for Current TXP Count to equal total TXP Count, indicating that the firmware is processing the last TXP */
			temp = 0;
			counter = 0;
			while ((temp < numLabels) && (counter < (10000 * numLabels)))
			{
				result = ADT_L0_ReadMem32(devID, channelRegOffset + tempTxcbTxpOffset + ADT_L1_A429_TXCB_PECOUNT, &temp, 1);
				counter++;
			}

			if (counter >= (10000 * numLabels)) 
			{
				result = ADT_ERR_TIMEOUT;
			}
			else
			{
				/* Read time to get a more precise delay (more precise than Sleep) */
				/* Wait two word times (36 bits) based on the halfBitTime */
				result = ADT_L1_A429_TimeGet(devID, &timeHigh, &timeLow);

				/* Use offset to avoid rollover of timeLow, this forces our timeLow
				   value to the lower half of the range so we won't encounter a
				   rollover during our short delay of two word times.
				 */
				if (timeLow > 0x7FFFFFFF) timeOffset = 0x80000000;
				else timeOffset = 0;

				tempLow = timeLow + (2 * halfBitTimeUs * 2 * 36 * 50) + timeOffset;

				while ((timeLow + timeOffset) < tempLow)
				{
					result = ADT_L1_A429_TimeGet(devID, &timeHigh, &timeLow);
				}
			}
		}

		/* Free the board memory used by the temp TXCB */
		result = ADT_L1_MemoryFree(devID, tempTxcbTxpOffset, ADT_L1_A429_TXCB_SIZE + (ADT_L1_A429_TXP_SIZE * numLabels));
	}

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_A429_TX_Channel_SetConfig
 *****************************************************************************/
/*! \brief Sets the protocol settings for an A429 Transmit Channel 
 *
 * This function sets the protocol settings for an A429 Transmit Channel.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param TxChanNum is the TX Channel number (0-15).
 * @param CSR1 is the value for the TX Channel CSR 1 register.
 * @param CSR2 is the value for the TX Channel CSR 2 register.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid Tx Channel number
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_TX_Channel_SetConfig(ADT_L0_UINT32 devID, ADT_L0_UINT32 TxChanNum, ADT_L0_UINT32 CSR1, ADT_L0_UINT32 CSR2) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 txSetupOffset;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (TxChanNum < 16) {
		/* Determine offset to the TX Channel Setup Regs */
		txSetupOffset = ADT_L1_A429_ROOT_TX_REGS + TxChanNum * ADT_L1_A429_TXREG_CHAN_SIZE;

		/* Write CSR1 register */
		result = ADT_L0_WriteMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_TX_CSR1, &CSR1, 1);

		/* Write CSR2 register */
		result = ADT_L0_WriteMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_TX_CSR2, &CSR2, 1);
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_A429_TX_Channel_GetConfig
 *****************************************************************************/
/*! \brief Gets the protocol settings for an A429 Transmit Channel 
 *
 * This function gets the protocol settings for an A429 Transmit Channel.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param TxChanNum is the TX Channel number (0-15).
 * @param pCSR1 is the pointer for the TX Channel CSR 1 register.
 * @param pCSR2 is the pointer for the TX Channel CSR 2 register.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid Tx Channel number
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_TX_Channel_GetConfig(ADT_L0_UINT32 devID, ADT_L0_UINT32 TxChanNum, ADT_L0_UINT32 *pCSR1, ADT_L0_UINT32 *pCSR2) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 txSetupOffset;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (TxChanNum < 16) {
		/* Determine offset to the TX Channel Setup Regs */
		txSetupOffset = ADT_L1_A429_ROOT_TX_REGS + TxChanNum * ADT_L1_A429_TXREG_CHAN_SIZE;

		/* Read CSR1 register */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_TX_CSR1, pCSR1, 1);

		/* Read CSR2 register */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_TX_CSR2, pCSR2, 1);
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}

/******************************************************************************
  FUNCTION:		ADT_L1_A429_TX_Channel_GetTxpCount
 *****************************************************************************/
/*! \brief Gets the current TXP count for an A429 Transmit Channel 
 *
 * This function returns the txp count for an A429 Transmit Channel.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param TxChanNum is the TX Channel number (0-15).
 * @param txpCnt is the pointer for the txp count.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid Tx Channel number
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_TX_Channel_GetTxpCount(ADT_L0_UINT32 devID, ADT_L0_UINT32 TxChanNum, ADT_L0_UINT32 *txpCnt) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 txSetupOffset;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (TxChanNum < 16) {
		/* Determine offset to the TX Channel Setup Regs */
		txSetupOffset = ADT_L1_A429_ROOT_TX_REGS + TxChanNum * ADT_L1_A429_TXREG_CHAN_SIZE;

		/* Read TXP count register */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_TXP_COUNT, txpCnt, 1);
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_A429_TX_Channel_CB_GetAddr
 *****************************************************************************/
/*! \brief Gets the byte address of a TX Control Block 
 *
 * This function Gets the byte address of a TX Control Block.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param TxChanNum is the TX Channel number (0-15).
 * @param msgnum is the message number (zero-indexed) of the TXCB to write.
 * @param pAddr is a pointer to store the address.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid message number
	- \ref ADT_ERR_NO_TXCB_TABLE - TXCB table pointer is zero (table not allocated)
	- \ref ADT_ERR_TXCB_NOT_ALLOCATED - No TXCB has been allocated for msgnum
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_TX_Channel_CB_GetAddr(ADT_L0_UINT32 devID, ADT_L0_UINT32 TxChanNum, ADT_L0_UINT32 msgnum, ADT_L0_UINT32 *pAddr) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, txSetupOffset;
	ADT_L0_UINT32 max_num_msgs, txcbTablePtr, txcbPtr;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (TxChanNum >= 16)
		return(ADT_ERR_BAD_INPUT);

	/* Determine offset to the TX Channel Setup Regs */
	txSetupOffset = ADT_L1_A429_ROOT_TX_REGS + TxChanNum * ADT_L1_A429_TXREG_CHAN_SIZE;

	/* Read max message number, verify that msgnum is valid */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_SIZE, &max_num_msgs, 1);
	if (msgnum < max_num_msgs) 
	{
		/* Check for valid TXCB table pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_PTR, &txcbTablePtr, 1);
		if (txcbTablePtr != 0) 
		{
			/* Check to see if TXCB has been allocated for the msgnum */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + txcbTablePtr + msgnum*4, &txcbPtr, 1);
			if (txcbPtr != 0)
			{
				*pAddr = txcbPtr;
			}
			else result = ADT_ERR_TXCB_NOT_ALLOCATED;  /* A TXCB has not been allocated for msgnum */
		}
		else result = ADT_ERR_NO_TXCB_TABLE;  /* No TXCB Table */
	}
	else result = ADT_ERR_BAD_INPUT;  /* Invalid message number */

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_A429_TX_Channel_CB_TXP_GetAddr
 *****************************************************************************/
/*! \brief Gets the byte address of a TXP for a TX Control Block 
 *
 * This function gets the byte address of a TXP for TX Control Block.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param TxChanNum is the TX Channel number (0-15).
 * @param msgnum is the message (TXCB) number.
 * @param txpNum is the buffer (TXP) number.
 * @param pAddr is a pointer to store the address.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid message number
	- \ref ADT_ERR_NO_TXCB_TABLE - TXCB table pointer is zero (table not allocated)
	- \ref ADT_ERR_TXCB_NOT_ALLOCATED - No TXCB has been allocated for msgnum
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_TX_Channel_CB_TXP_GetAddr(ADT_L0_UINT32 devID, ADT_L0_UINT32 TxChanNum, ADT_L0_UINT32 msgnum, ADT_L0_UINT32 txpNum, ADT_L0_UINT32 *pAddr) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, txSetupOffset;
	ADT_L0_UINT32 max_num_msgs, txcbTablePtr, txcbPtr;
	ADT_L0_UINT32 temp, startTxp, selectedTxp, i;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (TxChanNum >= 16)
		return(ADT_ERR_BAD_INPUT);

	/* Determine offset to the TX Channel Setup Regs */
	txSetupOffset = ADT_L1_A429_ROOT_TX_REGS + TxChanNum * ADT_L1_A429_TXREG_CHAN_SIZE;

	/* Read max message number, verify that msgnum is valid */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_SIZE, &max_num_msgs, 1);
	if (msgnum < max_num_msgs) 
	{
		/* Check for valid TXCB table pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_PTR, &txcbTablePtr, 1);
		if (txcbTablePtr != 0) 
		{
			/* Check to see if TXCB has been allocated for the msgnum */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + txcbTablePtr + msgnum*4, &txcbPtr, 1);
			if (txcbPtr != 0)
			{
				/* Check that the selected TXP number does not exceed the actual number of TXPs */
				temp = txcbPtr + ADT_L1_A429_TXCB_APINUMTXPS;
				result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &i, 1);
				if (txpNum >= i) {
					result = ADT_ERR_BAD_INPUT;
				}

				if (result == ADT_SUCCESS) {
					/* Read the API starting TXP address */
					temp = txcbPtr + ADT_L1_A429_TXCB_API1STTXPPTR;
					result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &startTxp, 1);

					/* Offset to the selected TXP */
					selectedTxp = startTxp + txpNum * ADT_L1_A429_TXP_SIZE;

					if (result == ADT_SUCCESS) {
						*pAddr = selectedTxp;
					}
				}
			}
			else result = ADT_ERR_TXCB_NOT_ALLOCATED;  /* A TXCB has not been allocated for msgnum */
		}
		else result = ADT_ERR_NO_TXCB_TABLE;  /* No TXCB Table */
	}
	else result = ADT_ERR_BAD_INPUT;  /* Invalid message number */

	return( result );
}




#ifdef __cplusplus
}
#endif
