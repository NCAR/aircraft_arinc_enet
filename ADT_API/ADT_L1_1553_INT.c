/******************************************************************************
 * FILE:			ADT_L1_1553_INT.c
 *
 * Copyright (c) 2007-2016, Alta Data Technologies LLC (ADT), All Rights Reserved.
 * Use of this software is subject to the ADT Software License (latest
 * revision), US Government or local laws and the ADT Terms and Conditions
 * of Sale (latest revision).
 *
 * DESCRIPTION:
 *	Source file for Layer 1 API.
 *	Contains 1553 interrupt functions.
 *
 *****************************************************************************/
/*! \file ADT_L1_1553_INT.c
 *  \brief Source file containing 1553 interrupt functions
 */
#include "ADT_L1.h"

#ifdef __cplusplus
extern "C" {
#endif


/******************************************************************************
  FUNCTION:		ADT_L1_1553_INT_EnableInt
 *****************************************************************************/
/*! \brief Enables interrupts for the channel.
 *
 * This function enables interrupts for the channel.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @return
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_INT_EnableInt(ADT_L0_UINT32 devID) {
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

	/* Read PE root CSR */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &data, 1);

	/* Set HW INT bit */
	data |= 0x000000001;

	/* Write PE root CSR */
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &data, 1);

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_1553_INT_DisableInt
 *****************************************************************************/
/*! \brief Disables interrupts the channel.
 *
 * This function disables interrupts for the channel.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @return
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_INT_DisableInt(ADT_L0_UINT32 devID) {
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

	/* Read PE root CSR */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &data, 1);

	/* Clear HW INT bit */
	data &= 0xFFFFFFFE;

	/* Write PE root CSR */
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &data, 1);

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_1553_INT_GenInt
 *****************************************************************************/
/*! \brief Generates an interrupt.
 *
 * This function generates a test interrupt.  This does NOT put an entry in
 * the interrupt queue.  This function is useful for testing interrupts when
 * developing a device driver for a new operating system.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @return
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_INT_GenInt(ADT_L0_UINT32 devID) {
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

	/* Set INT PENDING bit */
	data = 0x00000001;

	/* Write PE status register */
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_STS, &data, 1);

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_1553_INT_CheckChannelIntPending
 *****************************************************************************/
/*! \brief Checks to see if an interrupt is pending for a given 1553 channel.
 *
 * This function checks to see if an interrupt is pending for a given 1553 channel.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param pIsIntPending is a pointer to store the result (0 = no int pending, 1 = int pending).
 * @return
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid channel number
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_INT_CheckChannelIntPending(ADT_L0_UINT32 devID, ADT_L0_UINT32 *pIsIntPending) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 data;
	ADT_L0_UINT32 channel, channelMask;



	/*************************** NEED TO CHANGE THIS TO NOT USE GLOBAL REGISTER *******************************
	    NOT USED? DOESNT ACCOUNT FOR >4 CHANNELS.  NOT IMPLEMENTED FOR A429.  IS THERE FOR WMUX.
		THE ROOT PE STATUS REGISTER PROVIDES INT PENDING BIT AT CHANNEL LEVEL.
	 */



	/* Determine the channel to check for */
	channel = devID & 0x000000FF;

	/* The global int pending register can handle a max of 15 1553 channels */
	if (channel < 16) {
		/* For POLLED interrupts, we should see int pending bits in bits 0-14.
		 * For HARDWARE interrupts, we should see int pending bits in bits 16-30.
		 * This approach should work for both POLLED and HARDWARE interrupts.
		 */
		channelMask = 0x00010001;
		channelMask = channelMask << channel;

		/* Read the Global Int Pending Register */
		result = ADT_L0_ReadMem32(devID, ADT_L1_GLOBAL_INTPENDING, &data, 1);  // RSW - 10 ch - is this even used? not for A429?

		if (data & channelMask)
			pIsIntPending[0] = 1;
		else
			pIsIntPending[0] = 0;
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_1553_INT_IQ_ReadEntry
 *****************************************************************************/
/*! \brief Reads one new entry from the interrupt queue.
 *
 * This function reads one new entry from the interrupt queue.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param pType is the pointer to store the IQ type/seqnum.
 * @param pInfo is the pointer to store the CDP API INFO word, BCCB message number, SGCB pointer, or PBCB pointer.
 * @return
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_FAILURE - Completed with error
	- \ref ADT_ERR_IQ_NO_NEW_ENTRY - No new entry in the queue
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_INT_IQ_ReadEntry(ADT_L0_UINT32 devID, ADT_L0_UINT32 *pType, ADT_L0_UINT32 *pInfo) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 pApiLastIQEntry, pCurrIQEntry, pCDP, pBCCB;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Read the Current IQ pointer */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_IQ_CURR_IQP, &pCurrIQEntry, 1);

	/* Read the API Last IQ pointer */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_IQ_RESV_API, &pApiLastIQEntry, 1);

	/* If the two are not equal, then there are new entries in the queue */
	if (pApiLastIQEntry != pCurrIQEntry) {
		/* Read the IQ entry Type/seqnum */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + pApiLastIQEntry + 4, pType, 1);

		/* Get info base on interrupt type */
		switch (*pType & 0xFFFF0000)
		{
		/* CDP-level interrupts */
		case 0x80000000:	/* BM CDP Interrupt */
		case 0x40000000:	/* RT CDP Interrupt */
		case 0x20000000:	/* BC CDP Interrupt */
			/* Read the IQ entry CDP offset */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + pApiLastIQEntry + 8, &pCDP, 1);
			/* Read the API INFO word from the CDP */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + pCDP + ADT_L1_1553_CDP_RESV_API, pInfo, 1);
			break;

		/* BCCB-level interrupts */
		case 0x10000000:	/* BCCB Retry Complete */
		case 0x08000000:	/* BCCB Stop */
		case 0x04000000:	/* BCCB Frame Overflow */
		case 0x02000000:	/* BCCB Message Complete */
			/* Read the IQ entry BCCB offset */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + pApiLastIQEntry + 8, &pBCCB, 1);
			/* Read the MESSAGE NUMBER word from the BCCB */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + pBCCB + ADT_L1_1553_BC_CB_APIMSGNUM, pInfo, 1);
			break;

		/* PB and SG interrupts */
		case 0x00020000:	/* PB Interrupt */
		case 0x00010000:	/* SG Interrupt */
			/* Read the IQ entry PBCB or SGCB offset */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + pApiLastIQEntry + 8, pInfo, 1);
			break;

		default:
			break;
		}

		/* Update the API Last IQ pointer to the next entry */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + pApiLastIQEntry, &pApiLastIQEntry, 1);
		result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_IQ_RESV_API, &pApiLastIQEntry, 1);
	}
	else result = ADT_ERR_IQ_NO_NEW_ENTRY;

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_1553_INT_IQ_ReadNewEntries
 *****************************************************************************/
/*! \brief Reads all new entries from the interrupt queue
 *
 * This function reads all new entries from the interrupt queue.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param maxNumEntries is the maximum number of entries to read (size of buffer).
 * @param pNumEntries is the pointer to store the number of messages read.
 * @param pType is the pointer to store the IQ type/seqnums (array sized by maxNumEntries).
 * @param pInfo is the pointer to store the CDP API INFO words (array sized by maxNumEntries).
 * @return
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - invalid pointer
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_INT_IQ_ReadNewEntries(ADT_L0_UINT32 devID, ADT_L0_UINT32 maxNumEntries, ADT_L0_UINT32 *pNumEntries,
											ADT_L0_UINT32 *pType, ADT_L0_UINT32 *pInfo) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 status = ADT_SUCCESS;
	ADT_L0_UINT32 count;

	/* Ensure that the pointers passed in are not NULL */
	if ((pNumEntries != 0) && (pType != 0) && (pInfo != 0)) {
		count = 0;

		while ((count < maxNumEntries) && (status == ADT_SUCCESS)) {
			status = ADT_L1_1553_INT_IQ_ReadEntry(devID, &pType[count], &pInfo[count]);
			if (status == ADT_SUCCESS) count++;
			else if (status != ADT_ERR_IQ_NO_NEW_ENTRY)
				result = status;
		}

		*pNumEntries = count;
	}
	else result = ADT_ERR_BAD_INPUT;

	if (*pNumEntries == 0)
		result = ADT_ERR_IQ_NO_NEW_ENTRY;


	return( result );
}

/******************************************************************************
  FUNCTION:		ADT_L1_1553_INT_IQ_ReadRawEntry
 *****************************************************************************/
/*! \brief Reads one new entry from the interrupt queue and returns the raw
 *   data from the queue.
 *
 * This function reads one new entry from the interrupt queue.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param int_buffer is the pointer to the ADT_L1_1553_INT data buffer type where the interrupt info is stored.
 * @return
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_FAILURE - Completed with error
	- \ref ADT_ERR_IQ_NO_NEW_ENTRY - No new entry in the queue
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_INT_IQ_ReadRawEntry(ADT_L0_UINT32 devID, ADT_L1_1553_INT *int_buffer) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 pApiLastIQEntry, pCurrIQEntry;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Read the Current IQ pointer */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_IQ_CURR_IQP, &pCurrIQEntry, 1);

	/* Read the API Last IQ pointer */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_IQ_RESV_API, &pApiLastIQEntry, 1);

	/* If the two are not equal, then there are new entries in the queue */
	if (pApiLastIQEntry != pCurrIQEntry) {
		/* Read the IQ entry Type/seqnum */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + pApiLastIQEntry + 0, &int_buffer->NextPtr, 1);
		result = ADT_L0_ReadMem32(devID, channelRegOffset + pApiLastIQEntry + 4, &int_buffer->Type_SeqNum, 1);
		result = ADT_L0_ReadMem32(devID, channelRegOffset + pApiLastIQEntry + 8, &int_buffer->IntData, 1);

		/* Update the API Last IQ pointer to the next entry */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + pApiLastIQEntry, &pApiLastIQEntry, 1);
		result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_IQ_RESV_API, &pApiLastIQEntry, 1);
	}
	else result = ADT_ERR_IQ_NO_NEW_ENTRY;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_INT_IQ_ReadNewRawEntries
 *****************************************************************************/
/*! \brief Reads all new entries from the interrupt queue
 *
 * This function reads all new entries from the interrupt queue.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param maxNumEntries is the maximum number of entries to read (size of buffer).
 * @param pNumEntries is the pointer to store the number of messages read.
 * @param int_buffer is the pointer to store the IQ (array sized by maxNumEntries).

 * @return
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - invalid pointer
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_INT_IQ_ReadNewRawEntries(ADT_L0_UINT32 devID, ADT_L0_UINT32 maxNumEntries, ADT_L0_UINT32 *pNumEntries, ADT_L1_1553_INT *int_buffer ) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 status = ADT_SUCCESS;
	ADT_L0_UINT32 count;

	/* Ensure that the pointers passed in are not NULL */
	if ((pNumEntries != 0) && (int_buffer != 0)) {
		count = 0;

		while ((count < maxNumEntries) && (status == ADT_SUCCESS)) {
			status = ADT_L1_1553_INT_IQ_ReadRawEntry(devID, &int_buffer[count]);
			if (status == ADT_SUCCESS) count++;
			else if (status != ADT_ERR_IQ_NO_NEW_ENTRY)
				result = status;
		}

		*pNumEntries = count;
	}
	else result = ADT_ERR_BAD_INPUT;

	if (*pNumEntries == 0)
		result = ADT_ERR_IQ_NO_NEW_ENTRY;

	return( result );
}

/******************************************************************************
  FUNCTION:		ADT_L1_1553_INT_SetIntSeqNum
 *****************************************************************************/
/*! \brief Writes the interrupt sequence number for the channel.
 *
 * This function writes the interrupt sequence number for the channel.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param seqNum is the sequence number to write.
 * @return
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_INT_SetIntSeqNum(ADT_L0_UINT32 devID, ADT_L0_UINT32 seqNum) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Write the interrupt sequence number */
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_IQ_SEQNUM, &seqNum, 1);

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_1553_INT_GetIntSeqNum
 *****************************************************************************/
/*! \brief Reads the interrupt sequence number for the channel.
 *
 * This function reads the interrupt sequence number for the channel.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param pSeqNum is the pointer to store the sequence number.
 * @return
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_INT_GetIntSeqNum(ADT_L0_UINT32 devID, ADT_L0_UINT32 *pSeqNum) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Read the interrupt sequence number */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_IQ_SEQNUM, pSeqNum, 1);

	return( result );
}




#ifdef __cplusplus
}
#endif
