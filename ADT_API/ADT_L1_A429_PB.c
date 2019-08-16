/******************************************************************************
 * FILE:			ADT_L1_A429_PB.c
 *
 * Copyright (c) 2007-2016, Alta Data Technologies LLC (ADT), All Rights Reserved.
 * Use of this software is subject to the ADT Software License (latest
 * revision), US Government or local laws and the ADT Terms and Conditions 
 * of Sale (latest revision).
 *
 * DESCRIPTION:
 *	Source file for Layer 1 API.
 *	Contains A429 Playback functions.
 *
 *****************************************************************************/
/*! \file ADT_L1_A429_PB.c  
 *  \brief Source file containing A429 Playback functions 
 */
#include "stdio.h"
#include "ADT_L1.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
  FUNCTION:		Internal_PB_GetMsgnumFromPtr
  Internal function that converts a PBCB Pointer to a message number.
  Returns message number or 0xFFFFFFFF to indicate failure.
 *****************************************************************************/
ADT_L0_UINT32 ADT_L0_CALL_CONV Internal_PB_GetMsgnumFromPtr(ADT_L0_UINT32 devID, ADT_L0_UINT32 TxChanNum, ADT_L0_UINT32 PBCBPtr) {

	ADT_L0_UINT32 result = 0xFFFFFFFF;
	ADT_L0_UINT32 channel, channelRegOffset, txSetupOffset;
	ADT_L0_UINT32 max_num_msgs, PBCBTablePtr, data, i, found;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if ((PBCBPtr != 0) && (TxChanNum < 16))
	{
		/* Determine offset to the TX Channel Setup Regs */
		txSetupOffset = ADT_L1_A429_ROOT_TX_REGS + TxChanNum * ADT_L1_A429_TXREG_CHAN_SIZE;

		/* Read PBCB Table Pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_PTR, &PBCBTablePtr, 1);
		if (PBCBTablePtr != 0) 
		{
			/* Read max message number */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_SIZE, &max_num_msgs, 1);

			i = 0;  found = 0;
			while ((i < max_num_msgs) && (!found)) {
				result = ADT_L0_ReadMem32(devID, channelRegOffset + PBCBTablePtr + i*4, &data, 1);
				if (data == PBCBPtr)
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
  FUNCTION:		Internal_PB_GetPtrFromMsgnum
  Internal function that converts a message number to a PBCB Pointer.
  Returns pointer or 0x00000000 for failure/no pointer.
 *****************************************************************************/
ADT_L0_UINT32 ADT_L0_CALL_CONV Internal_PB_GetPtrFromMsgnum(ADT_L0_UINT32 devID, ADT_L0_UINT32 TxChanNum, ADT_L0_UINT32 msgnum) {

	ADT_L0_UINT32 result = 0;
	ADT_L0_UINT32 channel, channelRegOffset, txSetupOffset;
	ADT_L0_UINT32 max_num_msgs, PBCBTablePtr, data;

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
		/* Read PBCB Table Pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_PTR, &PBCBTablePtr, 1);
		if (PBCBTablePtr != 0) 
		{
			/* Read the BC CB pointer for the message number */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + PBCBTablePtr + msgnum*4, &data, 1);
			result = data;
		}
		else result = 0x00000000;
	}
	else result = 0x00000000;

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_A429_TX_Channel_PB_Init
 *****************************************************************************/
/*! \brief Initializes Playback data structures for an A429 Transmit Channel 
 *
 * This function initializes Playback data structures for an A429 Transmit Channel.
 * This initializes the Root TX Registers for the channel and allocates 
 * a table of PBCBs.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param TxChanNum is the TX Channel number (0-15).
 * @param BitRateHz is the bit rate in Hz (500 to 500000).
 * @param numPBCB is the number of PBCB to allocate.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid Tx Channel number or invalid bit rate
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_TX_Channel_PB_Init(ADT_L0_UINT32 devID, ADT_L0_UINT32 TxChanNum, ADT_L0_UINT32 BitRateHz, ADT_L0_UINT32 numPBCB) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 i, data, txSetupOffset, size_needed, PBCBOffset, halfBitTimeUs;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if ((TxChanNum < 16) && (BitRateHz >=500) && (BitRateHz <= 500000) && (numPBCB > 0)) {
		/* Determine offset to the TX Channel Setup Regs */
		txSetupOffset = ADT_L1_A429_ROOT_TX_REGS + TxChanNum * ADT_L1_A429_TXREG_CHAN_SIZE;

		/* Clear the Root TX Registers */
		data = 0x00000000;
		for (i=0; i<ADT_L1_A429_TXREG_CHAN_SIZE; i+=4) {
			result = ADT_L0_WriteMem32(devID, channelRegOffset + txSetupOffset + i, &data, 1);
		}

		/* Initialize CSR1 and CSR2 registers for ARINC-429 settings */
		data = ADT_L1_A429_TXREG_TX_CSR1_STOPPED | ADT_L1_A429_TXREG_TX_CSR1_PBMODE;  /* Set "Stopped" bit and "PB MODE" bit */
		result = ADT_L0_WriteMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_TX_CSR1, &data, 1);

		/* Set the provided bit rate - 10-bit period in microseconds */
		halfBitTimeUs = (ADT_L0_UINT32)((1.0 / (BitRateHz * 2.0))* 1000000) & 0x000003FF;
		/* NOTE: 0x80008430 = 32 bits per word, A429 high/low */
		data = 0x80008400 | (halfBitTimeUs << 16);

		/* if bit rate is greater than 50KHz, set high slew rate */
		if (BitRateHz > 20000) data |= ADT_L1_A429_TXREG_TX_CSR2_HISLEWRATE;

		result = ADT_L0_WriteMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_TX_CSR2, &data, 1);

		/* Write API PBCB Table Size */
		result = ADT_L0_WriteMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_SIZE, &numPBCB, 1);

		/* Allocate PBCB Table */
		size_needed = ADT_L1_A429_PBCB_SIZE * numPBCB;
		result = ADT_L1_MemoryAlloc(devID, size_needed, &PBCBOffset);
		if (result == ADT_SUCCESS) {
			/* Clear the PBCB Table memory */
			data = 0x00000000;
			for (i=0; i<size_needed; i+=4) {
				result = ADT_L0_WriteMem32(devID, channelRegOffset + PBCBOffset + i, &data, 1);
			}

			/* Write the API PBCB Table start pointer */
			result = ADT_L0_WriteMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_PTR, &PBCBOffset, 1);

		}
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_A429_TX_Channel_PB_Close
 *****************************************************************************/
/*! \brief Uninitializes Playback data structures and frees memory for the TX Channel 
 *
 * This function uninitializes Playback data structures and frees memory for the TX Channel.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param TxChanNum is the TX Channel number (0-15).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid Rx Channel number
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_TX_Channel_PB_Close(ADT_L0_UINT32 devID, ADT_L0_UINT32 TxChanNum) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, numPBCB;
	ADT_L0_UINT32 i, j, k, data, txSetupOffset, size_needed, PBCBTableOffset, PBCBOffset, txpOffset, numTxP;

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

	/* Read the API PBCB Table Size */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_SIZE, &numPBCB, 1);

	/* Read the API PBCB Table Offset */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_PTR, &PBCBTableOffset, 1);

	/* Iterate through PBCB Table to free memory used by PBCBs and TxPs */
	for (i=0; i<numPBCB; i++) {
		/* Read the PBCB pointer from the table */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + PBCBTableOffset + i*4, &PBCBOffset, 1);

		/* If pointer is non-zero, then there is a PBCB that needs to be freed */
		if (PBCBOffset != 0x00000000) {
			/* Read the TXP Table pointer from the PBCB */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + PBCBOffset + ADT_L1_A429_TXCB_API1STTXPPTR, &txpOffset, 1);

			/* Read the TXP Table size from the PBCB */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + PBCBOffset + ADT_L1_A429_TXCB_APINUMTXPS, &numTxP, 1);

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

			/* Clear the memory for the PBCB */
			for (j=0; j<ADT_L1_A429_PBCB_SIZE; j+=4) {
				data = 0x00000000;
				result = ADT_L0_WriteMem32(devID, channelRegOffset + PBCBOffset + j, &data, 1);
			}

			/* Free the memory for the PBCB */
			result = ADT_L1_MemoryFree(devID, PBCBOffset, ADT_L1_A429_PBCB_SIZE);

			/* Clear the PBCB pointer in the PBCB table */
			PBCBOffset = 0x00000000;
			result = ADT_L0_WriteMem32(devID, channelRegOffset + PBCBTableOffset + i*4, &PBCBOffset, 1);
		}
	}

	/* Free the memory used by the PBCB Table */
	size_needed = ADT_L1_A429_PBCB_SIZE * numPBCB;
	result = ADT_L1_MemoryFree(devID, PBCBTableOffset, size_needed);

	/* Clear the TX Channel Root Registers */
	data = 0x00000000;
	for (i=0; i<ADT_L1_A429_TXREG_CHAN_SIZE; i+=4) {
		result = ADT_L0_WriteMem32(devID, channelRegOffset + txSetupOffset + i, &data, 1);
	}

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_A429_TX_Channel_PB_CB_PXPAllocate
 *****************************************************************************/
/*! \brief Allocates a Playback Control Block (PB_CB)and PXPs.
 *
 * This function allocates memory for a PB_CB and PXPs.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param TxChanNum is the TX Channel number (0-15).
 * @param PbcbNum is the PBCB (zero-indexed) for which to allocate a PBCB/PXPs.
 * @param numPXP is the number of PXPs to allocate and link for the PBCB.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid message number
	- \ref ADT_ERR_NO_TXCB_TABLE - PBCB table pointer is zero (table not allocated)
	- \ref ADT_ERR_TXCB_ALREADY_ALLOCATED - A PBCB has already been allocated for PbcbNum
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
 *
 * !! Change List !!
 * 081123: RS: Added PBCB TXP API Tail Pointer to Init; Added API PXP Origianl Count Write
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_TX_Channel_PB_CB_PXPAllocate(ADT_L0_UINT32 devID, ADT_L0_UINT32 TxChanNum, ADT_L0_UINT32 PbcbNum, ADT_L0_UINT32 numPXP) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, txSetupOffset, size_needed;
	ADT_L0_UINT32 max_num_msgs, PBCBTablePtr, data, newPbcbPtr, i;
	ADT_L0_UINT32 firstPxpPtr;
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

	/* Read max message number, verify that PbcbNum is valid */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_SIZE, &max_num_msgs, 1);
	if (PbcbNum < max_num_msgs) 
	{
		/* Check for valid PBCB table pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_PTR, &PBCBTablePtr, 1);
		if (PBCBTablePtr != 0) 
		{
			/* Check to see if PBCB has already been allocated for the PbcbNum */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + PBCBTablePtr + PbcbNum*4, &data, 1);
			if (data == 0)
			{
				/* Allocate a PBCB */
				result = ADT_L1_MemoryAlloc(devID, ADT_L1_A429_PBCB_SIZE, &newPbcbPtr);
				if (result == ADT_SUCCESS)
				{
					/* Write PBCB ptr to the PBCB table entry for this PbcbNum */
					result = ADT_L0_WriteMem32(devID, channelRegOffset + PBCBTablePtr + PbcbNum*4, &newPbcbPtr, 1);

					/* Clear all words in the PBCB */
					data = 0;
					for (i=0; i<ADT_L1_A429_PBCB_SIZE; i+=4)
						result = ADT_L0_WriteMem32(devID, channelRegOffset + newPbcbPtr + i, &data, 1);

					/* Write the API message number to the PBCB */
					result = ADT_L0_WriteMem32(devID, channelRegOffset + newPbcbPtr + ADT_L1_A429_PBCB_APIPBCBNUM, &PbcbNum, 1);

					/* Write the API number of PXPs to the PBCB */
					result = ADT_L0_WriteMem32(devID, channelRegOffset + newPbcbPtr + ADT_L1_A429_PBCB_APINUMPXPS, &numPXP, 1);

					/* Write the Total PXP Count to the PBCB */
					result = ADT_L0_WriteMem32(devID, channelRegOffset + newPbcbPtr + ADT_L1_A429_PBCB_PXPCOUNT, &numPXP, 1);

					/* Allocate the requested number of PXPs */
					size_needed = numPXP * ADT_L1_A429_PXP_SIZE;
					/* Check for sufficient meory before allocation */
					if ( ADT_L1_GetMemoryAvailable(devID, &memAvailable) == ADT_SUCCESS )
					{
						if(size_needed > memAvailable)
						{
							result = ADT_ERR_MEM_MGT_NO_MEM;
							return( result);
						}
					}

					result = ADT_L1_MemoryAlloc(devID, size_needed, &firstPxpPtr);

					/* Clear all words in the PXPs */
					data = 0;
					for (i=0; i<size_needed; i+=4)
						result = ADT_L0_WriteMem32(devID, channelRegOffset + firstPxpPtr + i, &data, 1);

					/* Write the API first TXP pointer to the PBCB */
					result = ADT_L0_WriteMem32(devID, channelRegOffset + newPbcbPtr + ADT_L1_A429_PBCB_API1STPBPPTR, &firstPxpPtr, 1);

					/* Write the TXP table pointer to the PBCB */
					result = ADT_L0_WriteMem32(devID, channelRegOffset + newPbcbPtr + ADT_L1_A429_PBCB_PXPPTR, &firstPxpPtr, 1);
				}
			}
			else result = ADT_ERR_TXCB_ALREADY_ALLOCATED;  /* A PBCB has already been allocated for PbcbNum */
		}
		else result = ADT_ERR_NO_TXCB_TABLE;  /* No PBCB Table */
	}
	else result = ADT_ERR_BAD_INPUT;  /* Invalid PbcbNum */

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_A429_TX_Channel_PB_CB_PXPFree
 *****************************************************************************/
/*! \brief Removes a PB Control Block and associated PXPs.
 *
 * This function frees memory for a PB Control Block and associated PXPs.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param TxChanNum is the TX Channel number (0-15).
 * @param msgnum is the message number (zero-indexed) of the PBCB to free.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid device number
	- \ref ADT_ERR_NO_TXCB_TABLE - PBCB table pointer is zero (table not allocated)
	- \ref ADT_ERR_TXCB_NOT_ALLOCATED - No PBCB has been allocated for msgnum
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_TX_Channel_PB_CB_PXPFree(ADT_L0_UINT32 devID, ADT_L0_UINT32 TxChanNum, ADT_L0_UINT32 msgnum) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, txSetupOffset, numTXP, firstPxpPtr;
	ADT_L0_UINT32 max_num_msgs, PBCBTablePtr, PBCBPtr, size_needed, i, data;

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
		/* Check for valid PBCB table pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_PBCB_API1STPBPPTR, &PBCBTablePtr, 1);
		if (PBCBTablePtr != 0) 
		{
			/* Check to see if PBCB has been allocated for the msgnum */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + PBCBTablePtr + msgnum*4, &PBCBPtr, 1);
			if (PBCBPtr != 0)
			{
				/* Read the API number of TXPs from the PBCB */
				result = ADT_L0_ReadMem32(devID, channelRegOffset + PBCBPtr + ADT_L1_A429_TXCB_APINUMTXPS, &numTXP, 1);

				/* Read the API first TXP pointer from the PBCB */
				result = ADT_L0_ReadMem32(devID, channelRegOffset + PBCBPtr + ADT_L1_A429_TXCB_API1STTXPPTR, &firstPxpPtr, 1);

				/* Determine size of TXP table */
				size_needed = numTXP * ADT_L1_A429_TXP_SIZE;

				/* Clear all words in the TXP table */
				data = 0x00000000;
				for (i=0; i<size_needed; i+=4)
					result = ADT_L0_WriteMem32(devID, channelRegOffset + firstPxpPtr + i, &data, 1);

				/* Free the memory used by the TXP table */
				result = ADT_L1_MemoryFree(devID, firstPxpPtr, size_needed);

				/* Clear all words in the PBCB */
				data = 0x00000000;
				for (i=0; i<ADT_L1_A429_PBCB_SIZE; i+=4)
					result = ADT_L0_WriteMem32(devID, channelRegOffset + PBCBPtr + i, &data, 1);

				/* Free the memory used by the PBCB */
				result = ADT_L1_MemoryFree(devID, PBCBPtr, ADT_L1_A429_PBCB_SIZE);

				/* Clear the PBCB pointer in the PBCB table */
				PBCBPtr = 0x00000000;
				result = ADT_L0_WriteMem32(devID, channelRegOffset + PBCBTablePtr + msgnum*4, &PBCBPtr, 1);

			}
			else result = ADT_ERR_TXCB_NOT_ALLOCATED;  /* A BCCB has not been allocated for msgnum */
		}
		else result = ADT_ERR_NO_TXCB_TABLE;  /* No BCCB Table */
	}
	else result = ADT_ERR_BAD_INPUT;  /* Invalid msgnum */

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_A429_TX_Channel_PB_CB_Write
 *****************************************************************************/
/*! \brief Writes a TX Control Block for Playback
 *
 * This function writes a TX Control Block for Playback.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param TxChanNum is the TX Channel number (0-15).
 * @param PbcbNum is the message number (zero-indexed) of the PBCB to write.
 * @param PBCB is a pointer to the PBCB to write.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid message number
	- \ref ADT_ERR_NO_TXCB_TABLE - PBCB table pointer is zero (table not allocated)
	- \ref ADT_ERR_TXCB_NOT_ALLOCATED - No PBCB has been allocated for PbcbNum
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_TX_Channel_PB_CB_Write(ADT_L0_UINT32 devID, ADT_L0_UINT32 TxChanNum, ADT_L0_UINT32 PbcbNum, ADT_L1_A429_TXCB *PBCB) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, txSetupOffset;
	ADT_L0_UINT32 max_num_msgs, PBCBTablePtr, PBCBPtr, data;

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

	/* Read max message number, verify that PbcbNum is valid */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_SIZE, &max_num_msgs, 1);
	if (PbcbNum < max_num_msgs) 
	{
		/* Check for valid PBCB table pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_PTR, &PBCBTablePtr, 1);
		if (PBCBTablePtr != 0) 
		{
			/* Check to see if PBCB has been allocated for the PbcbNum */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + PBCBTablePtr + PbcbNum*4, &PBCBPtr, 1);
			if (PBCBPtr != 0)
			{
				/* Write the PBCB - Next PBCB Pointer */
				data = Internal_PB_GetPtrFromMsgnum(devID, TxChanNum, PBCB->NextTxcbNum);
				result = ADT_L0_WriteMem32(devID, channelRegOffset + PBCBPtr + ADT_L1_A429_TXCB_NEXTPTR, &data, 1);

				/* Write the PBCB - Control Word */
				data = PBCB->Control;
				result = ADT_L0_WriteMem32(devID, channelRegOffset + PBCBPtr + ADT_L1_A429_TXCB_CONTROL, &data, 1);
			}
			else result = ADT_ERR_TXCB_NOT_ALLOCATED;  /* A PBCB has not been allocated for PbcbNum */
		}
		else result = ADT_ERR_NO_TXCB_TABLE;  /* No PBCB Table */
	}
	else result = ADT_ERR_BAD_INPUT;  /* Invalid message number */

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_A429_TX_Channel_PB_CB_Read
 *****************************************************************************/
/*! \brief Reads a TX Control Block 
 *
 * This function reads a TX Control Block.
 * 
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param TxChanNum is the TX Channel number (0-15).
 * @param msgnum is the message number (zero-indexed) of the BCCB to read.
 * @param PBCB is a pointer to store the BCCB read.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid message number
	- \ref ADT_ERR_NO_TXCB_TABLE - PBCB table pointer is zero (table not allocated)
	- \ref ADT_ERR_TXCB_NOT_ALLOCATED - No PBCB has been allocated for msgnum
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_TX_Channel_PB_CB_Read(ADT_L0_UINT32 devID,  ADT_L0_UINT32 TxChanNum, ADT_L0_UINT32 msgnum, ADT_L1_A429_TXCB *PBCB) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, txSetupOffset;
	ADT_L0_UINT32 max_num_msgs, PBCBTablePtr, PBCBPtr, data;

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
		/* Check for valid PBCB table pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_PTR, &PBCBTablePtr, 1);
		if (PBCBTablePtr != 0) 
		{
			/* Check to see if PBCB has been allocated for the msgnum */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + PBCBTablePtr + msgnum*4, &PBCBPtr, 1);
			if (PBCBPtr != 0)
			{
				/* Read the PBCB - Next PBCB Pointer */
				result = ADT_L0_ReadMem32(devID, channelRegOffset + PBCBPtr + ADT_L1_A429_TXCB_NEXTPTR, &data, 1);
				PBCB->NextTxcbNum = Internal_PB_GetMsgnumFromPtr(devID, TxChanNum, data);

				/* Read the PBCB - Control Word */
				result = ADT_L0_ReadMem32(devID, channelRegOffset + PBCBPtr + ADT_L1_A429_TXCB_CONTROL, &data, 1);
				PBCB->Control = data;

				/* Read the PBCB - TX Period - ALWAYS ZERO FOR PLAYBACK */
				PBCB->TxPeriod500us = 0;

				/* PBCB message number is just for user reference, not used otherwise (msg number specified in PBCB function parameters */
				PBCB->TxcbNum = msgnum;
			}
			else result = ADT_ERR_TXCB_NOT_ALLOCATED;  /* A PBCB has not been allocated for msgnum */
		}
		else result = ADT_ERR_NO_TXCB_TABLE;  /* No PBCB Table */
	}
	else result = ADT_ERR_BAD_INPUT;  /* Invalid message number */

	return( result );
}

/******************************************************************************
  FUNCTION:		ADT_L1_A429_TX_Channel_PB_RXPWrite
 *****************************************************************************/
/*! \brief Converts a RXP(s) to a PB_CB PXP and writes it an open PB buffer. 
 *
 * This function converts an array of RXPs to a PB PXP and writes it to an open PB_CB buffer.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param txChanNum is the TX Channel number (0-15).
 * @param numRxps is the number of RXPs in RxpBuffer array (max 85 to support ENET)
 * @param RxpBuffer is a pointer to the RXP to write to the PB TXP.
 * @param options are the packet control word options.
 *	- ADT_L1_A429_TXCB_CONTROL_STOPONTXBCOMP		0x00000010
 *  - ADT_L1_A429_TXCB_CONTROL_INTONTXBCOMP			0x00000100
 *  - ADT_L1_A429_PB_API_ATON						0x80000000
 * @param isFirstMsg is 1 for first message in playback or 0 for NOT first message.

 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - not an A429 device
	- \ref ADT_ERR_BUFFER_FULL - PB buffer is full (Caught up to PE PB Transmission)
	- \ref ADT_ERR_BAD_INPUT - Invalid RXP
	- \ref ADT_FAILURE - Completed with error
	- \ref ADT_ERR_PBCB_TOOMANYPXPS - User Passed in Too Many PXPs for PBCB
 *
 */
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_TX_Channel_PB_RXPWrite(ADT_L0_UINT32 devID, ADT_L0_UINT32 txChanNum, ADT_L0_UINT32 numRxps, 
																  ADT_L1_A429_RXP *RxpBuffer, ADT_L0_UINT32 options, ADT_L0_UINT32 isFirstMsg) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, txChanRegOffset;
	ADT_L0_UINT32 tailPBptr, headPBptr, startPBptr, pxpPtr, nextPBptr, APIpxpCnt;
	ADT_L0_UINT32 firstTimeHigh, firstTimeLow, timeHigh, timeLow, ATflag, i;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (txChanNum >= 16)
		return(ADT_ERR_BAD_INPUT);

	if (numRxps > 85)
		return(ADT_ERR_BAD_INPUT);

	/* Get the offset to the Root TX Registers for the TX channel */
	txChanRegOffset = channelRegOffset + ADT_L1_A429_ROOT_TX_REGS + (txChanNum * ADT_L1_A429_TXREG_CHAN_SIZE);

	/* Get the API TAIL PBCB PTR */
	result = ADT_L0_ReadMem32(devID, txChanRegOffset + ADT_L1_A429_TXREG_API_TAILPTR, &tailPBptr, 1);

	/* Get the PE CURRENT PBCB PTR */
	result = ADT_L0_ReadMem32(devID, txChanRegOffset + ADT_L1_A429_TXREG_CUR_TXCB_PTR, &headPBptr, 1);

	/* If this is the FIRST MESSAGE IN PLAYBACK, then it is OK to go past head pointer
	 * (on first msg head, tail, and start ptr will be the same).
	 * We also want to STORE THE TIMESTAMP on the first message.  This allows us to convert
	 * all timestamps to time relative to start of playback (by subtracting the timestamp
	 * from the first message.
	 *
	 * If AT is on, then we do not adjust time and the playback clock will compare for absolute time values.
	 * The user must have previous set the Playback Control Clock Value to the desired time and possibly
	 * set the Root PE CSR Bit to Not Reset Clock to Zero.
	 */
	ATflag = options & ADT_L1_A429_PB_API_ATON;
	if (isFirstMsg) {
		/* Get the PE START PBCB PTR */
		result = ADT_L0_ReadMem32(devID, txChanRegOffset + ADT_L1_A429_TXREG_1ST_TXCB_PTR, &startPBptr, 1);

		/* Reset tail and head pointers to equal start pointer - First msg must be at start pointer */
		startPBptr = Internal_PB_GetPtrFromMsgnum(devID, txChanNum, 0);
		result = ADT_L0_WriteMem32(devID, txChanRegOffset + ADT_L1_A429_TXREG_1ST_TXCB_PTR, &startPBptr, 1);

		tailPBptr = startPBptr;
		result = ADT_L0_WriteMem32(devID, txChanRegOffset + ADT_L1_A429_TXREG_API_TAILPTR, &tailPBptr, 1);
		headPBptr = startPBptr;
		result = ADT_L0_WriteMem32(devID, txChanRegOffset + ADT_L1_A429_TXREG_CUR_TXCB_PTR, &headPBptr, 1);

		if (!ATflag) {
			/* Save first msg timestamp */
			firstTimeHigh = RxpBuffer[0].TimeHigh;
			firstTimeLow = RxpBuffer[0].TimeLow;
			result = ADT_L0_WriteMem32(devID, txChanRegOffset + ADT_L1_A429_PE_PB_1ST_TIME_HIGH, &firstTimeHigh, 1);
			result = ADT_L0_WriteMem32(devID, txChanRegOffset + ADT_L1_A429_PE_PB_1ST_TIME_LOW, &firstTimeLow, 1);

			/* On first msg, add one 20ns tick.  We do this so PB will wait for ext clock on synchronous PB */
			RxpBuffer[0].TimeLow++;
			if (RxpBuffer[0].TimeLow < firstTimeLow) RxpBuffer[0].TimeHigh++;  /* Handle carry */
		}
	}

	/* Else (not the first PBCB list in playback) . . .
	 * If the tail ptr equals the head ptr, then the buffer (PBCB Chain) is full - cannot write packet to buffer.
	 * If the buffer is not full, we want to read the first PBCB timestamp so we can adjust the
	 * current RXP timestamp to be time from start of playback (for relative time setup only).
	 */
	else {
		if (tailPBptr == headPBptr) 
			result = ADT_ERR_BUFFER_FULL;
		else if (!ATflag) {
			result = ADT_L0_ReadMem32(devID, txChanRegOffset + ADT_L1_A429_PE_PB_1ST_TIME_HIGH, &firstTimeHigh, 1);
			result = ADT_L0_ReadMem32(devID, txChanRegOffset + ADT_L1_A429_PE_PB_1ST_TIME_LOW, &firstTimeLow, 1);
		}
	}

	/* Write RXPs to the tail PBCB */
	if (result == ADT_SUCCESS) {

		/* Get Original Allocated API PXP Count from Tail PRT */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + tailPBptr + ADT_L1_A429_PBCB_APINUMPXPS, &APIpxpCnt, 1);

		/* If user numRxps is <= to	Allocated Pbcb PXP count, then write out PXPs */
		/* Maximum of 85 RXPs, because ENET ADCP max payload is 1400 bytes (87.5 RXPs) */
		if ((numRxps <= APIpxpCnt) && (numRxps <= 85))
		{

			/* Read the API starting PXP address from the tail PBCB */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + tailPBptr + ADT_L1_A429_PBCB_PXPPTR, &pxpPtr, 1);

			/* Loop and prepare all RXPs - Adjust Time if not AT */
			for (i = 0; i < numRxps; i++) {

				if (!ATflag) {
					/* TIMESTAMP - Need to subtract time of first message in playback to make this time relative to start of playback. */
					/* Time High */
					timeHigh = RxpBuffer[i].TimeHigh - firstTimeHigh;
					if (firstTimeLow > RxpBuffer[i].TimeLow) timeHigh -= 1;  /* Borrow/Carry (or whatever we call this) */
					/* Time Low */
					timeLow = RxpBuffer[i].TimeLow - firstTimeLow;
				}
				else /* Absolute Time - no adjustment needed */
				{
					timeHigh = RxpBuffer[i].TimeHigh;
					timeLow = RxpBuffer[i].TimeLow;
				}

				RxpBuffer[i].Control = 0;  /* The RXP Control Word is Cleared.  This will keep the PXP/TXP Parity Off */
				RxpBuffer[i].TimeHigh = timeHigh;
				RxpBuffer[i].TimeLow = timeLow;
			} /* End RXP processing For Loop */

			/* Write the block of RXPs to the device */
			result = ADT_L0_WriteMem32(devID, channelRegOffset + pxpPtr, (ADT_L0_UINT32 *) RxpBuffer, numRxps*4);

			/* write valid options to PBCB control word */
			options &= (ADT_L1_A429_PBCB_CONTROL_STOPONPBCBCOMP | ADT_L1_A429_PBCB_CONTROL_INTONPBCBCOMP);
			result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_A429_PBCB_CONTROL, &options, 1);

			/* write PBCB PXP Count Value for PE */
			result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_A429_PBCB_PXPCOUNT, &numRxps, 1);

			/* Get the NEXT PB PACKET PTR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + tailPBptr + ADT_L1_A429_PBCB_NEXTPTR, &nextPBptr, 1);

			/* Move API TAIL PB PACKET PTR to the NEXT packet */
			result = ADT_L0_WriteMem32(devID, txChanRegOffset + ADT_L1_A429_TXREG_API_TAILPTR, &nextPBptr, 1);
		}
		else
			result = ADT_ERR_PBCB_TOOMANYPXPS;
	}

	return( result );
}

/******************************************************************************
  FUNCTION:		ADT_L1_A429_TX_Channel_PB_CB_PXPRead
 *****************************************************************************/
/*! \brief Reads a Playback PXP for a PB Control Block 
 *
 * This function reads a Playback PXP for PB Control Block.
 * THIS IS GENERALLY NOT USED, HAS NOT BEEN TESTED WITH LATEST CHANGES FOR A429 PLAYBACK.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param TxChanNum is the TX Channel number (0-15).
 * @param msgnum is the message (PBCB) number.
 * @param txpNum is the buffer (PXP) number.
 * @param pTxp is a pointer to the TXP structure.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid message number
	- \ref ADT_ERR_NO_TXCB_TABLE - PBCB table pointer is zero (table not allocated)
	- \ref ADT_ERR_TXCB_NOT_ALLOCATED - No PBCB has been allocated for msgnum
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_TX_Channel_PB_CB_PXPRead(ADT_L0_UINT32 devID, ADT_L0_UINT32 TxChanNum, ADT_L0_UINT32 msgnum, ADT_L0_UINT32 pxpNum, ADT_L1_A429_RXP *pPxp) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, txSetupOffset;
	ADT_L0_UINT32 max_num_msgs, PBCBTablePtr, PBCBPtr;
	ADT_L0_UINT32 temp, startPxp, selectedPxp, i;

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
		/* Check for valid PBCB table pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_PTR, &PBCBTablePtr, 1);
		if (PBCBTablePtr != 0) 
		{
			/* Check to see if PBCB has been allocated for the msgnum */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + PBCBTablePtr + msgnum*4, &PBCBPtr, 1);
			if (PBCBPtr != 0)
			{
				/* Check that the selected PXP number does not exceed the actual number of PXPs */
				temp = PBCBPtr + ADT_L1_A429_TXCB_APINUMTXPS;
				result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &i, 1);
				if (pxpNum >= i) {
					result = ADT_ERR_BAD_INPUT;
				}

				if (result == ADT_SUCCESS) {
					/* Read the API starting TXP address */
					temp = PBCBPtr + ADT_L1_A429_TXCB_API1STTXPPTR;
					result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &startPxp, 1);

					/* Offset to the selected PXP */
					selectedPxp = startPxp + pxpNum * ADT_L1_A429_PXP_SIZE;

					if (result == ADT_SUCCESS) {
						/* Read the selected PXP */
						result = ADT_L0_ReadMem32(devID, channelRegOffset + selectedPxp + ADT_L1_A429_PXP_CONTROL, &pPxp->Control, 1);
						result = ADT_L0_ReadMem32(devID, channelRegOffset + selectedPxp + ADT_L1_A429_PXP_TIMEHIGH, &pPxp->TimeHigh, 1);
						result = ADT_L0_ReadMem32(devID, channelRegOffset + selectedPxp + ADT_L1_A429_PXP_TIMELOW, &pPxp->TimeLow, 1);
						result = ADT_L0_ReadMem32(devID, channelRegOffset + selectedPxp + ADT_L1_A429_PXP_DATA, &pPxp->Data, 1);

						/* NOTE:  For PLAYBACK, the RESERVED word and DELAY word are used as the
						          64-bit 20ns TIMESTAMP for the TXP.
					    */
					}
				}
			}
			else result = ADT_ERR_TXCB_NOT_ALLOCATED;  /* A PBCB has not been allocated for msgnum */
		}
		else result = ADT_ERR_NO_TXCB_TABLE;  /* No PBCB Table */
	}
	else result = ADT_ERR_BAD_INPUT;  /* Invalid message number */

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_A429_TX_Channel_PB_Start
 *****************************************************************************/
/*! \brief Starts Playback operation for the TX channel 
 *
 * This function starts Playback operation for TX channel on the selected message (PBCB) number.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param TxChanNum is the TX Channel number (0-15).
 * @param msgnum is the message (PBCB) number to start on.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid message number or channel number
	- \ref ADT_ERR_NO_TXCB_TABLE - PBCB table pointer is zero (table not allocated)
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_TX_Channel_PB_Start(ADT_L0_UINT32 devID, ADT_L0_UINT32 TxChanNum) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, txSetupOffset;
	ADT_L0_UINT32 max_num_msgs, PBCBTablePtr, data, msgnum;

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

	/* Playback ALWAYS starts on first message */
	msgnum = 0;

	/* Read max message number, verify that msgnum is valid */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_SIZE, &max_num_msgs, 1);
	if (msgnum < max_num_msgs) 
	{
		/* Check for valid PBCB table pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_API_TXCB_PTR, &PBCBTablePtr, 1);
		if (PBCBTablePtr != 0) 
		{
			/* Write the starting message pointer */
			data = Internal_PB_GetPtrFromMsgnum(devID, TxChanNum, msgnum);
			result = ADT_L0_WriteMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_1ST_TXCB_PTR, &data, 1);
            
			/* clear TXP count */
			data = 0x00000000;
			result = ADT_L0_WriteMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_TXP_COUNT, &data, 1);

			/* Read the TX CSR1 */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_TX_CSR1, &data, 1);

			/* Set bit 0 (start BC) and clear bit 1 (BC stopped) */
			data &= ~ADT_L1_A429_TXREG_TX_CSR1_STOPPED;
			data |= ADT_L1_A429_TXREG_TX_CSR1_START;

			/* Write the TX CSR */
			result = ADT_L0_WriteMem32(devID, channelRegOffset + txSetupOffset + ADT_L1_A429_TXREG_TX_CSR1, &data, 1);
		}
		else result = ADT_ERR_NO_TXCB_TABLE;  /* No PBCB Table */
	}
	else result = ADT_ERR_BAD_INPUT;  /* Invalid message number */

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_A429_TX_Channel_PB_Stop
 *****************************************************************************/
/*! \brief Stops Playback operation for the TX channel 
 *
 * This function stops Playback operation for TX channel.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param TxChanNum is the TX Channel number (0-15).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid channel number
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_TX_Channel_PB_Stop(ADT_L0_UINT32 devID, ADT_L0_UINT32 TxChanNum) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, txSetupOffset;
	ADT_L0_UINT32 data;

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

	return( result );
}

/******************************************************************************
  FUNCTION:		ADT_L1_A429_PB_Start
 *****************************************************************************/
/*! \brief Starts Playback operation for the Bank Device 
 *
 * This function starts Playback operation for Bank Device (All TX channels)
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param TxChanNum is the TX Channel number (0-15).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid channel number
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_PB_Start(ADT_L0_UINT32 devID) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 data;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Read Root PE Register */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_A429_PE_ROOT_CSR, &data, 1);
	data |= ADT_L1_A429_PECSR_PBON;
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_PE_ROOT_CSR, &data, 1);

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_A429_PB_Stop
 *****************************************************************************/
/*! \brief Stops Playback operation for the Bank Device 
 *
 * This function stops Playback operation for Bank Device (All TX channels)
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param TxChanNum is the TX Channel number (0-15).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid channel number
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_PB_Stop(ADT_L0_UINT32 devID) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 data;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Read/Write Root PE Register to stop*/
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_A429_PE_ROOT_CSR, &data, 1);
	data &= ~ADT_L1_A429_PECSR_PBON;
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_PE_ROOT_CSR, &data, 1);

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_A429_TX_Channel_PB_IsRunning
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
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_TX_Channel_PB_IsRunning(ADT_L0_UINT32 devID, ADT_L0_UINT32 TxChanNum, ADT_L0_UINT32 *pIsRunning) {
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
  FUNCTION:		ADT_L1_A429_TX_Channel_PB_SetConfig
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
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_TX_Channel_PB_SetConfig(ADT_L0_UINT32 devID, ADT_L0_UINT32 TxChanNum, ADT_L0_UINT32 CSR1, ADT_L0_UINT32 CSR2) {
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
  FUNCTION:		ADT_L1_A429_TX_Channel_PB_GetConfig
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
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_TX_Channel_PB_GetConfig(ADT_L0_UINT32 devID, ADT_L0_UINT32 TxChanNum, ADT_L0_UINT32 *pCSR1, ADT_L0_UINT32 *pCSR2) {
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


#ifdef __cplusplus
}
#endif
