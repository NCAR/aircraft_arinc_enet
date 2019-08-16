/******************************************************************************
 * FILE:			ADT_L1_1553_BC.c
 *
 * Copyright (c) 2007-2018, Alta Data Technologies LLC (ADT), All Rights Reserved.
 * Use of this software is subject to the ADT Software License (latest
 * revision), US Government or local laws and the ADT Terms and Conditions 
 * of Sale (latest revision).
 *
 * DESCRIPTION:
 *	Source file for Layer 1 API.
 *	Contains 1553 BC functions. 
 *
 *****************************************************************************/
/*! \file ADT_L1_1553_BC.c  
 *  \brief Source file containing 1553 BC functions 
 */
#include "ADT_L1.h"

#ifdef __cplusplus
extern "C" {
#endif


/******************************************************************************
  FUNCTION:		Internal_BC_GetMsgnumFromPtr
  Internal function that converts a BC CB Pointer to a message number.
  Returns message number or 0xFFFFFFFF to indicate failure.
 *****************************************************************************/
ADT_L0_UINT32 ADT_L0_CALL_CONV Internal_BC_GetMsgnumFromPtr(ADT_L0_UINT32 devID, ADT_L0_UINT32 bccbPtr) {

	ADT_L0_UINT32 result = 0xFFFFFFFF;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 max_num_msgs, bccbTablePtr, data, i, found;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (bccbPtr != 0)
	{
		/* Read BC CB Table Pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBPTR, &bccbTablePtr, 1);
		if (bccbTablePtr != 0) 
		{
			/* Read max message number */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBSIZE, &max_num_msgs, 1);

			i = 0;  found = 0;
			while ((i < max_num_msgs) && (!found)) {
				result = ADT_L0_ReadMem32(devID, channelRegOffset + bccbTablePtr + i*4, &data, 1);
				if (data == bccbPtr)
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
  FUNCTION:		Internal_BC_GetPtrFromMsgnum
  Internal function that converts a message number to a BC CB Pointer.
  Returns pointer or 0x00000000 for failure/no pointer.
 *****************************************************************************/
ADT_L0_UINT32 ADT_L0_CALL_CONV Internal_BC_GetPtrFromMsgnum(ADT_L0_UINT32 devID, ADT_L0_UINT32 msgnum) {

	ADT_L0_UINT32 result = 0;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 max_num_msgs, bccbTablePtr, data;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Read max message number */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBSIZE, &max_num_msgs, 1);

	if (msgnum < max_num_msgs)
	{
		/* Read BC CB Table Pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBPTR, &bccbTablePtr, 1);
		if (bccbTablePtr != 0) 
		{
			/* Read the BC CB pointer for the message number */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + bccbTablePtr + msgnum*4, &data, 1);
			result = data;
		}
		else result = 0x00000000;
	}
	else result = 0x00000000;

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_1553_BC_Init
 *****************************************************************************/
/*! \brief Initializes Bus Controller settings for the device 
 *
 * This function initializes BC settings for the device.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param max_num_msgs is the maximum number of BC control blocks to be used.
 * @param minors_per_major is the number of minor frames per major frame.
 * @param bcCsr is the initial value for the BC CSR.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid device number
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_BC_Init(ADT_L0_UINT32 devID, ADT_L0_UINT32 max_num_msgs, ADT_L0_UINT32 minors_per_major, ADT_L0_UINT32 bcCsr) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, data, frameCount, i, temp;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Clear all the root BC registers */
	data = 0;
	frameCount = 1;
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BC_FIRSTBCCB, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BC_CURRBCCB, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BC_CSR, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BC_MINPERMAJ, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BC_MINORFRMCNT, &frameCount, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BC_MAXFRAMECNT, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BC_TOTALFRMCNT, &frameCount, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BC_HPAMPTR, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BC_LPAMPTR, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BC_LPAMTIME, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBPTR, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBSIZE, &data, 1);

	/* Initialize the number of minor frames per major frame */
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BC_MINPERMAJ, &minors_per_major, 1);

	/* Initialize the BC CSR */
	temp = bcCsr | 0x00000002;  /* Set "BC STOPPED" bit (bit 1) */
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BC_CSR, &temp, 1);

	/* Allocate the BCCB Table */
	result = ADT_L1_MemoryAlloc(devID, max_num_msgs * 4, &data);
	if (result == ADT_SUCCESS) {
		/* Write the pointer to the BCCB table and the table size to the root BC registers */
		result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBPTR, &data, 1);
		result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBSIZE, &max_num_msgs, 1);

		/* Initialize all table entries to zero */
		temp = 0;
		for (i=0; i<max_num_msgs; i++)
			result = ADT_L0_WriteMem32(devID, channelRegOffset + data + (i*4), &temp, 1);
	}

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_BC_Close
 *****************************************************************************/
/*! \brief Frees all BC resources for the device 
 *
 * This function frees all BC resources for the device.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid device number
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_BC_Close(ADT_L0_UINT32 devID) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, data, max_num_msgs, i;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Get the size of the BCCB table in words */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBSIZE, &max_num_msgs, 1);

	/* Free all BCCB and associated CDPs */
	for (i=0; i<max_num_msgs; i++) 
	{
		result = ADT_L1_1553_BC_CB_CDPFree(devID, i);
	}

	/* Free the BCCB Table */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBPTR, &data, 1);
	result = ADT_L1_MemoryFree(devID, data, max_num_msgs*4);

	/* Clear all the root BC registers */
	data = 0;
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BC_FIRSTBCCB, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BC_CURRBCCB, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BC_CSR, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BC_MINORFRMCNT, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BC_MAXFRAMECNT, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BC_TOTALFRMCNT, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BC_MINPERMAJ, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BC_HPAMPTR, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BC_LPAMPTR, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BC_LPAMTIME, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBPTR, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBSIZE, &data, 1);

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_BC_CB_CDPAllocate
 *****************************************************************************/
/*! \brief Allocates a BC Control Block and allocates/links CDPs.
 *
 * This function allocates memory for a BC Control Block and allocates/links CDPs.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param msgnum is the message number (zero-indexed) for which to allocate a BCCB.
 * @param numCDP is the number of CDPs to allocate and link for the BCCB.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid message number
	- \ref ADT_ERR_NO_BCCB_TABLE - BCCB table pointer is zero (table not allocated)
	- \ref ADT_ERR_BCCB_ALREADY_ALLOCATED - A BCCB has already been allocated for msgnum
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_BC_CB_CDPAllocate(ADT_L0_UINT32 devID, ADT_L0_UINT32 msgnum, ADT_L0_UINT32 numCDP) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 max_num_msgs, bccbTablePtr, data, newBccbPtr;
	ADT_L0_UINT32 firstCdpPtr, prevCdpPtr, currCdpPtr, cdpIndex;
	ADT_L0_UINT32 datablock[ADT_RW_MEM_MAX_SIZE];
	ADT_L0_UINT32 memAvailable;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Read max message number, verify that msgnum is valid */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBSIZE, &max_num_msgs, 1);
	if ((msgnum < max_num_msgs) && numCDP) 
	{

		/* Check if there is enough memory to allocate */
		if ( ADT_L1_GetMemoryAvailable(devID, &memAvailable) == ADT_SUCCESS )
		{
			if((numCDP * ADT_L1_1553_CDP_SIZE) > memAvailable)
			{
				result = ADT_ERR_MEM_MGT_NO_MEM;
				return( result);
			}
		}

		/* Check for valid BCCB table pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBPTR, &bccbTablePtr, 1);
		if (bccbTablePtr != 0) 
		{
			/* Check to see if BCCB has already been allocated for the msgnum */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + bccbTablePtr + msgnum*4, &data, 1);
			if (data == 0)
			{
				/* Allocate a BCCB */
				result = ADT_L1_MemoryAlloc(devID, ADT_L1_1553_BC_CB_SIZE, &newBccbPtr);
				if (result == ADT_SUCCESS)
				{
					/* Write BCCB ptr to the BCCB table entry for this msgnum */
					result = ADT_L0_WriteMem32(devID, channelRegOffset + bccbTablePtr + msgnum*4, &newBccbPtr, 1);

					/* Clear all words in the BCCB */
					memset(datablock, 0, sizeof(datablock));  data = 0;
					result = ADT_L0_WriteMem32(devID, channelRegOffset + newBccbPtr, datablock, ADT_L1_1553_BC_CB_SIZE/4);

					/* Write the API message number to the BCCB */
					result = ADT_L0_WriteMem32(devID, channelRegOffset + newBccbPtr + ADT_L1_1553_BC_CB_APIMSGNUM, &msgnum, 1);

					/* Allocate the requested number of CDPs */
					/* Allocate the first CDP */
					result = ADT_L1_MemoryAlloc(devID, ADT_L1_1553_CDP_SIZE, &firstCdpPtr);
					if (result == ADT_SUCCESS)
					{
						/* Clear all words in the CDP */
						memset(datablock, 0, sizeof(datablock));  data = 0;
						result = ADT_L0_WriteMem32(devID, channelRegOffset + firstCdpPtr, datablock, ADT_L1_1553_CDP_SIZE/4);

						/* Write CDP next pointer to point to itself */
						result = ADT_L0_WriteMem32(devID, channelRegOffset + firstCdpPtr + ADT_L1_1553_CDP_NEXT, &firstCdpPtr, 1);

						/* Write the CDP Control Word to set no word count error */
						data = 0x3F000000;
						result = ADT_L0_WriteMem32(devID, channelRegOffset + firstCdpPtr + ADT_L1_1553_CDP_CONTROL, &data, 1);

						/* Write the API info word to the CDP to identify BC type, message number, buffer number */
						data = ADT_L1_1553_CDP_RAPI_BC_ID;
						data |= msgnum << ADT_L1_1553_CDP_RAPI_BC_MSGNUM;
						result = ADT_L0_WriteMem32(devID, channelRegOffset + firstCdpPtr + ADT_L1_1553_CDP_RESV_API, &data, 1);

						/* Write first CDP pointer to first and current CDP pointers in BCCB */
						result = ADT_L0_WriteMem32(devID, channelRegOffset + newBccbPtr + ADT_L1_1553_BC_CB_API1STCDP, &firstCdpPtr, 1);
						result = ADT_L0_WriteMem32(devID, channelRegOffset + newBccbPtr + ADT_L1_1553_BC_CB_CDPPTR, &firstCdpPtr, 1);

						prevCdpPtr = firstCdpPtr;

						/* Allocate and link any additional CDPs */
						for (cdpIndex=1; cdpIndex<numCDP; cdpIndex++)
						{
							if (result == ADT_SUCCESS)
							{
								/* Allocate a CDP */
								result = ADT_L1_MemoryAlloc(devID, ADT_L1_1553_CDP_SIZE, &currCdpPtr);
								if (result == ADT_SUCCESS)
								{
									/* Clear all words in the CDP */
									memset(datablock, 0, sizeof(datablock));  data = 0;
									result = ADT_L0_WriteMem32(devID, channelRegOffset + currCdpPtr, datablock, ADT_L1_1553_CDP_SIZE/4);

									/* Write CDP next pointer to point to first CDP */
									result = ADT_L0_WriteMem32(devID, channelRegOffset + currCdpPtr + ADT_L1_1553_CDP_NEXT, &firstCdpPtr, 1);

									/* Write CDP next pointer for the previous CDP to point to this CDP */
									result = ADT_L0_WriteMem32(devID, channelRegOffset + prevCdpPtr + ADT_L1_1553_CDP_NEXT, &currCdpPtr, 1);

									/* Write the API info word to the CDP to identify BC type, message number, buffer number */
									data = ADT_L1_1553_CDP_RAPI_BC_ID;
									data |= msgnum << ADT_L1_1553_CDP_RAPI_BC_MSGNUM;
									data |= cdpIndex << ADT_L1_1553_CDP_RAPI_BC_BUFNUM;
									result = ADT_L0_WriteMem32(devID, channelRegOffset + currCdpPtr + ADT_L1_1553_CDP_RESV_API, &data, 1);

									prevCdpPtr = currCdpPtr;
								}
							}
						}

						if (result == ADT_SUCCESS)
						{
							/* Write the number of CDPs to the BCCB */
							result = ADT_L0_WriteMem32(devID, channelRegOffset + newBccbPtr + ADT_L1_1553_BC_CB_APINUMCDP, &numCDP, 1);
						}
					}
				}
			}
			else result = ADT_ERR_BCCB_ALREADY_ALLOCATED;  /* A BCCB has already been allocated for msgnum */
		}
		else result = ADT_ERR_NO_BCCB_TABLE;  /* No BCCB Table */
	}
	else result = ADT_ERR_BAD_INPUT;  /* Invalid msgnum */

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_BC_CB_CDPFree
 *****************************************************************************/
/*! \brief Removes a BC Control Block and associated CDPs.
 *
 * This function frees memory for a BC Control Block and associated CDPs.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param msgnum is the message number (zero-indexed) of the BCCB to free.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid device number
	- \ref ADT_ERR_NO_BCCB_TABLE - BCCB table pointer is zero (table not allocated)
	- \ref ADT_ERR_BCCB_NOT_ALLOCATED - No BCCB has been allocated for msgnum
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_BC_CB_CDPFree(ADT_L0_UINT32 devID, ADT_L0_UINT32 msgnum) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 max_num_msgs, bccbTablePtr, data, bccbPtr;
	ADT_L0_UINT32 currCdpPtr, nextCdpPtr, cdpIndex, numCDP;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Read max message number, verify that msgnum is valid */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBSIZE, &max_num_msgs, 1);
	if (msgnum < max_num_msgs) 
	{
		/* Check for valid BCCB table pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBPTR, &bccbTablePtr, 1);
		if (bccbTablePtr != 0) 
		{
			/* Check to see if BCCB has been allocated for the msgnum */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + bccbTablePtr + msgnum*4, &bccbPtr, 1);
			if (bccbPtr != 0)
			{
				/* Read number of CDPs and first CDP ptr */
				result = ADT_L0_ReadMem32(devID, channelRegOffset + bccbPtr + ADT_L1_1553_BC_CB_APINUMCDP, &numCDP, 1);
				result = ADT_L0_ReadMem32(devID, channelRegOffset + bccbPtr + ADT_L1_1553_BC_CB_API1STCDP, &currCdpPtr, 1);

				/* Free all CDP for the BCCB */
				for (cdpIndex=0; cdpIndex<numCDP; cdpIndex++) 
				{
					/* Read pointer to next CDP */
					result = ADT_L0_ReadMem32(devID, channelRegOffset + currCdpPtr + ADT_L1_1553_CDP_NEXT, &nextCdpPtr, 1);

					/* Free this CDP */
					result = ADT_L1_MemoryFree(devID, currCdpPtr, ADT_L1_1553_CDP_SIZE);

					/* Move to next CDP */
					currCdpPtr = nextCdpPtr;
				}

				/* Free the BCCB */
				result = ADT_L1_MemoryFree(devID, bccbPtr, ADT_L1_1553_BC_CB_SIZE);

				/* Clear the BCCB pointer for the msgnum */
				data = 0;
				result = ADT_L0_WriteMem32(devID, channelRegOffset + bccbTablePtr + msgnum*4, &data, 1);
			}
			else result = ADT_ERR_BCCB_NOT_ALLOCATED;  /* A BCCB has not been allocated for msgnum */
		}
		else result = ADT_ERR_NO_BCCB_TABLE;  /* No BCCB Table */
	}
	else result = ADT_ERR_BAD_INPUT;  /* Invalid msgnum */

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_BC_CB_Write
 *****************************************************************************/
/*! \brief Writes a BC Control Block 
 *
 * This function writes a BC Control Block.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param msgnum is the message number (zero-indexed) of the BCCB to write.
 * @param bccb is a pointer to the BCCB to write.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid message number
	- \ref ADT_ERR_NO_BCCB_TABLE - BCCB table pointer is zero (table not allocated)
	- \ref ADT_ERR_BCCB_NOT_ALLOCATED - No BCCB has been allocated for msgnum
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_BC_CB_Write(ADT_L0_UINT32 devID, ADT_L0_UINT32 msgnum, ADT_L1_1553_BC_CB *bccb) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 max_num_msgs, bccbTablePtr, bccbPtr, data;
	ADT_L0_UINT32 datablock[16];

	/* RICH WADE - v2.5.4.1 - Changed to use BLOCK write. */
	memset(datablock, 0, sizeof(datablock));

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Read max message number, verify that msgnum is valid */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBSIZE, &max_num_msgs, 1);
	if (msgnum < max_num_msgs) 
	{
		/* Check for valid BCCB table pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBPTR, &bccbTablePtr, 1);
		if (bccbTablePtr != 0) 
		{
			/* Check to see if BCCB has been allocated for the msgnum */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + bccbTablePtr + msgnum*4, &bccbPtr, 1);
			if (bccbPtr != 0)
			{
				/* Write the BC CB - Next Msg Pointer */
				data = Internal_BC_GetPtrFromMsgnum(devID, bccb->NextMsgNum);
				result = ADT_L0_WriteMem32(devID, channelRegOffset + bccbPtr + ADT_L1_1553_BC_CB_NEXTPTR, &data, 1);

				/* Write the BC CB - Retry Word */
				datablock[0] = bccb->Retry;

				/* Write the BC CB - CSR */
				datablock[1] = bccb->Csr;

				/* Write the BC CB - CMD1 Info */
				datablock[2] = bccb->CMD1Info;

				/* Write the BC CB - CMD2 Info */
				datablock[3] = bccb->CMD2Info;

				/* Write the BC CB - Frame Time */
				datablock[4] = bccb->FrameTime;

				/* Write the BC CB - Delay Time */
				datablock[5] = bccb->DelayTime;

				/* Write the BC CB - Branch Pointer */
				datablock[6] = Internal_BC_GetPtrFromMsgnum(devID, bccb->BranchMsgNum);

				/* Write the BC CB - Start Frame */
				datablock[7] = bccb->StartFrame;

				/* Write the BC CB - Stop Frame */
				datablock[8] = bccb->StopFrame;

				/* Write the BC CB - Frame Rep Rate */
				datablock[9] = bccb->FrameRepRate;
				
				/* Write the BLOCK of data */
				result = ADT_L0_WriteMem32(devID, channelRegOffset + bccbPtr + ADT_L1_1553_BC_CB_RETRY, datablock, 10);
				
				/* We do not write to the API words (num CDPs, message number, etc.) */
			}
			else result = ADT_ERR_BCCB_NOT_ALLOCATED;  /* A BCCB has not been allocated for msgnum */
		}
		else result = ADT_ERR_NO_BCCB_TABLE;  /* No BCCB Table */
	}
	else result = ADT_ERR_BAD_INPUT;  /* Invalid message number */

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_BC_CB_WriteWords
 *****************************************************************************/
/*! \brief Writes an Array of Words to an Offset of a BCCB
 *
 * This function writes an array of words to an offset location of a BC Control Block.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param msgnum is the message number (zero-indexed) of the BCCB to write.
 * @param wordOffset is the word offset in to the BCCB where the write will occur
 * @param numOfWords is the number of words to be written from pWords
 * @param pWords is a pointer to the array of words to be written to the BCCB
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid message number
	- \ref ADT_ERR_NO_BCCB_TABLE - BCCB table pointer is zero (table not allocated)
	- \ref ADT_ERR_BCCB_NOT_ALLOCATED - No BCCB has been allocated for msgnum
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_BC_CB_WriteWords(ADT_L0_UINT32 devID, ADT_L0_UINT32 msgnum, 
															ADT_L0_UINT32 wordOffset,
															ADT_L0_UINT32 numOfWords,
															ADT_L0_UINT32 *pWords) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 max_num_msgs, bccbTablePtr, bccbPtr;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Read max message number, verify that msgnum is valid */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBSIZE, &max_num_msgs, 1);
	if (msgnum < max_num_msgs) 
	{
		/* Check for valid BCCB table pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBPTR, &bccbTablePtr, 1);
		if (bccbTablePtr != 0) 
		{
			/* Check to see if BCCB has been allocated for the msgnum */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + bccbTablePtr + (msgnum*4), &bccbPtr, 1);
			if (bccbPtr != 0)
			{
				/* Check to see if numOfWords is valid for a BCCB */
				/* Write the Array of Words to the Offset */
				if ((numOfWords) && ((wordOffset + numOfWords) <= (ADT_L1_1553_BC_CB_SIZE/4)))
					result = ADT_L0_WriteMem32(devID, channelRegOffset + bccbPtr + (wordOffset*4), pWords, numOfWords);
					/* We do not write to the API words (num CDPs, message number, etc.) */
				else 
					result = ADT_ERR_BAD_INPUT;  /* Invalid Number of Words for a BCCB */
			}
			else result = ADT_ERR_BCCB_NOT_ALLOCATED;  /* A BCCB has not been allocated for msgnum */
		}
		else result = ADT_ERR_NO_BCCB_TABLE;  /* No BCCB Table */
	}
	else result = ADT_ERR_BAD_INPUT;  /* Invalid message number */

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_BC_CB_Read
 *****************************************************************************/
/*! \brief Reads a BC Control Block 
 *
 * This function reads a BC Control Block.
 * 
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param msgnum is the message number (zero-indexed) of the BCCB to read.
 * @param bccb is a pointer to store the BCCB read.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid message number
	- \ref ADT_ERR_NO_BCCB_TABLE - BCCB table pointer is zero (table not allocated)
	- \ref ADT_ERR_BCCB_NOT_ALLOCATED - No BCCB has been allocated for msgnum
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_BC_CB_Read(ADT_L0_UINT32 devID, ADT_L0_UINT32 msgnum, ADT_L1_1553_BC_CB *bccb) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 max_num_msgs, bccbTablePtr, bccbPtr;
	ADT_L0_UINT32 datablock[16];

	/* RICH WADE - v2.5.4.1 - Changed to use BLOCK write. */
	memset(datablock, 0, sizeof(datablock));

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Read max message number, verify that msgnum is valid */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBSIZE, &max_num_msgs, 1);
	if (msgnum < max_num_msgs) 
	{
		/* Check for valid BCCB table pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBPTR, &bccbTablePtr, 1);
		if (bccbTablePtr != 0) 
		{
			/* Check to see if BCCB has been allocated for the msgnum */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + bccbTablePtr + msgnum*4, &bccbPtr, 1);
			if (bccbPtr != 0)
			{
				/* Read the BLOCK of data */
				result = ADT_L0_ReadMem32(devID, channelRegOffset + bccbPtr + ADT_L1_1553_BC_CB_NEXTPTR, datablock, 15);

				/* Read the BC CB - Next Msg Pointer */
				bccb->NextMsgNum = Internal_BC_GetMsgnumFromPtr(devID, datablock[0]);
				
				/* Read the BC CB - Retry Word */
				bccb->Retry = datablock[2];

				/* Read the BC CB - CSR */
				bccb->Csr = datablock[3];

				/* Read the BC CB - CMD1 Info */
				bccb->CMD1Info = datablock[4];

				/* Read the BC CB - CMD2 Info */
				bccb->CMD2Info = datablock[5];

				/* Read the BC CB - Frame Time */
				bccb->FrameTime = datablock[6];

				/* Read the BC CB - Delay Time */
				bccb->DelayTime = datablock[7];

				/* Read the BC CB - Branch Pointer */
				bccb->BranchMsgNum = Internal_BC_GetMsgnumFromPtr(devID, datablock[8]);

				/* Read the BC CB - Start Frame */
				bccb->StartFrame = datablock[9];

				/* Read the BC CB - Stop Frame */
				bccb->StopFrame = datablock[10];

				/* Read the BC CB - Frame Rep Rate */
				bccb->FrameRepRate = datablock[11];

				/* Read the BC CB - API Message Number */
				bccb->MsgNum = datablock[13];

				/* Read the BC CB - API Number of CDPs */
				bccb->NumBuffers = datablock[14];

			}
			else result = ADT_ERR_BCCB_NOT_ALLOCATED;  /* A BCCB has not been allocated for msgnum */
		}
		else result = ADT_ERR_NO_BCCB_TABLE;  /* No BCCB Table */
	}
	else result = ADT_ERR_BAD_INPUT;  /* Invalid message number */

	return( result );
}

/******************************************************************************
  FUNCTION:		ADT_L1_1553_BC_CB_ReadWords
 *****************************************************************************/
/*! \brief Reads an Array of Words from an Offset of a BCCB
 *
 * This function reads an array of words from an offset location of a BC Control Block.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param msgnum is the message number (zero-indexed) of the BCCB to write.
 * @param wordOffset is the word offset in to the BCCB where the read will occur
 * @param numOfWords is the number of words to be read from pWords
 * @param pWords is a pointer to the array of words to store words from the BCCB
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid message number
	- \ref ADT_ERR_NO_BCCB_TABLE - BCCB table pointer is zero (table not allocated)
	- \ref ADT_ERR_BCCB_NOT_ALLOCATED - No BCCB has been allocated for msgnum
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_BC_CB_ReadWords(ADT_L0_UINT32 devID, ADT_L0_UINT32 msgnum, 
															ADT_L0_UINT32 wordOffset,
															ADT_L0_UINT32 numOfWords,
															ADT_L0_UINT32 *pWords) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 max_num_msgs, bccbTablePtr, bccbPtr;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Read max message number, verify that msgnum is valid */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBSIZE, &max_num_msgs, 1);
	if (msgnum < max_num_msgs) 
	{
		/* Check for valid BCCB table pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBPTR, &bccbTablePtr, 1);
		if (bccbTablePtr != 0) 
		{
			/* Check to see if BCCB has been allocated for the msgnum */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + bccbTablePtr + (msgnum*4), &bccbPtr, 1);
			if (bccbPtr != 0)
			{
				/* Check to see if numOfWords is valid for a BCCB */
				/* Read the Array of Words to the Offset */
				if ((numOfWords) && ((wordOffset + numOfWords) <= (ADT_L1_1553_BC_CB_SIZE/4)))
					result = ADT_L0_ReadMem32(devID, channelRegOffset + bccbPtr + (wordOffset*4), pWords, numOfWords);
				else 
					result = ADT_ERR_BAD_INPUT;  /* Invalid Number of Words for a BCCB */
			}
			else result = ADT_ERR_BCCB_NOT_ALLOCATED;  /* A BCCB has not been allocated for msgnum */
		}
		else result = ADT_ERR_NO_BCCB_TABLE;  /* No BCCB Table */
	}
	else result = ADT_ERR_BAD_INPUT;  /* Invalid message number */

	return( result );
}

/******************************************************************************
  FUNCTION:		ADT_L1_1553_BC_CB_CDPWrite
 *****************************************************************************/
/*! \brief Writes a CDP for a BC Control Block 
 *
 * This function writes a CDP for BC Control Block.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param msgnum is the message (BC CB) number.
 * @param cdpNum is the buffer (CDP) number.
 * @param pCdp is a pointer to the CDP structure.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid message number
	- \ref ADT_ERR_NO_BCCB_TABLE - BCCB table pointer is zero (table not allocated)
	- \ref ADT_ERR_BCCB_NOT_ALLOCATED - No BCCB has been allocated for msgnum
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_BC_CB_CDPWrite(ADT_L0_UINT32 devID, ADT_L0_UINT32 msgnum, ADT_L0_UINT32 cdpNum, ADT_L1_1553_CDP *pCdp) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 max_num_msgs, bccbTablePtr, bccbPtr;
	ADT_L0_UINT32 temp, startCdp, selectedCdp, i;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Check that the msgnum does not exceed the maximum number of BC messages */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBSIZE, &max_num_msgs, 1);
	if (msgnum < max_num_msgs) 
	{
		/* Check for valid BCCB table pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBPTR, &bccbTablePtr, 1);
		if (bccbTablePtr != 0) 
		{
			/* Check to see if BCCB has been allocated for the msgnum */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + bccbTablePtr + msgnum*4, &bccbPtr, 1);
			if (bccbPtr != 0)
			{
				/* Check that the selected CDP number does not exceed the actual number of CDPs */
				temp = bccbPtr + ADT_L1_1553_BC_CB_APINUMCDP;
				result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &i, 1);
				if (cdpNum >= i) {
					result = ADT_ERR_BAD_INPUT;
				}

				if (result == ADT_SUCCESS) {
					/* Read the API starting CDP address */
					temp = bccbPtr + ADT_L1_1553_BC_CB_API1STCDP;
					result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &startCdp, 1);

#ifndef ADT_1553_USE_NONCONTIGUOUS_CDP
					/********** OPTIMIZED HERE **********/
					/* Changed in v2.6.3.5 */
					/* THIS ASSUMES THAT ALL CDPS FOR THE MESSAGE WERE ALLOCATED AS CONTIGUOUS BLOCK OF MEMORY
					   AND THAT BUFFERS ARE IN SEQUENCE (won't work if any of the pointers have been changed) */
					selectedCdp = startCdp + (cdpNum * ADT_L1_1553_CDP_SIZE);
					/************************************/
#else
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
				}
			}
			else result = ADT_ERR_BCCB_NOT_ALLOCATED;  /* A BCCB has not been allocated for msgnum */
		}
		else result = ADT_ERR_NO_BCCB_TABLE;  /* No BCCB Table */
	}
	else result = ADT_ERR_BAD_INPUT;  /* Invalid message number */

	return( result );
}

/******************************************************************************
  FUNCTION:		ADT_L1_1553_BC_CB_CDPWriteWords
 *****************************************************************************/
/*! \brief Writes 32-bit Words to a CDP for a BC Control Block 
 *
 * This function writes words to a CDP for BC Control Block.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param msgnum is the message (BC CB) number.
 * @param cdpNum is the buffer (CDP) number.
 * @param wordOffset is the word offset in to the CDP where the write will occur
 * @param numOfWords is the number of words to be written from pWords
 * @param pWords is a pointer to the array of words to be written to the CDP
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid message number, wordOffset or numOfWords
	- \ref ADT_ERR_NO_BCCB_TABLE - BCCB table pointer is zero (table not allocated)
	- \ref ADT_ERR_BCCB_NOT_ALLOCATED - No BCCB has been allocated for msgnum
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_BC_CB_CDPWriteWords(ADT_L0_UINT32 devID, ADT_L0_UINT32 msgnum, ADT_L0_UINT32 cdpNum,
										 ADT_L0_UINT32 wordOffset, ADT_L0_UINT32 numOfWords, ADT_L0_UINT32 *pWords) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 max_num_msgs, bccbTablePtr, bccbPtr;
	ADT_L0_UINT32 temp, startCdp, selectedCdp, i;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Check that the msgnum does not exceed the maximum number of BC messages */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBSIZE, &max_num_msgs, 1);
	if (msgnum < max_num_msgs) 
	{
		/* Check for valid BCCB table pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBPTR, &bccbTablePtr, 1);
		if (bccbTablePtr != 0) 
		{
			/* Check to see if BCCB has been allocated for the msgnum */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + bccbTablePtr + msgnum*4, &bccbPtr, 1);
			if (bccbPtr != 0)
			{
				/* Check that the selected CDP number does not exceed the actual number of CDPs */
				temp = bccbPtr + ADT_L1_1553_BC_CB_APINUMCDP;
				result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &i, 1);
				if (cdpNum >= i) {
					result = ADT_ERR_BAD_INPUT;
				}

				if (result == ADT_SUCCESS) {
					/* Read the API starting CDP address */
					temp = bccbPtr + ADT_L1_1553_BC_CB_API1STCDP;
					result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &startCdp, 1);

					/********** OPTIMIZED HERE **********/
					/* Changed in v2.6.3.5 */
#ifdef ADT_1553_USE_NONCONTIGUOUS_CDP
					/* Follow NEXT pointers to find the selected CDP */
					i = 0;
					selectedCdp = startCdp;
					while (i < cdpNum) {
						result = ADT_L0_ReadMem32(devID, channelRegOffset + selectedCdp, &temp, 1);
						selectedCdp = temp;
						i++;
					}
#else
					/* THIS ASSUMES THAT ALL CDPS FOR THE MESSAGE WERE ALLOCATED AS CONTIGUOUS BLOCK OF MEMORY
					   AND THAT BUFFERS ARE IN SEQUENCE (won't work if any of the pointers have been changed) */
					selectedCdp = startCdp + (cdpNum * ADT_L1_1553_CDP_SIZE);
#endif
					/************************************/


					if (result == ADT_SUCCESS) {

						/* Write the words to the selected CDP offset 
						 * This action can overwrite any CDP word.
						 */
						if ((numOfWords) && ((wordOffset + numOfWords) <= (ADT_L1_1553_CDP_SIZE/4))) 
							result = ADT_L0_WriteMem32(devID, channelRegOffset + selectedCdp + (wordOffset*4), pWords, numOfWords);
						else
							result = ADT_ERR_BAD_INPUT;
					}
				}
			}
			else result = ADT_ERR_BCCB_NOT_ALLOCATED;  /* A BCCB has not been allocated for msgnum */
		}
		else result = ADT_ERR_NO_BCCB_TABLE;  /* No BCCB Table */
	}
	else result = ADT_ERR_BAD_INPUT;  /* Invalid message number */

	return( result );
}

/******************************************************************************
  FUNCTION:		ADT_L1_1553_BC_CB_CDPRead
 *****************************************************************************/
/*! \brief Reads a CDP for a BC Control Block 
 *
 * This function reads a CDP for a BC Control Block.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param msgnum is the message (BC CB) number.
 * @param cdpNum is the buffer (CDP) number.
 * @param pCdp is a pointer to the CDP structure.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid message number
	- \ref ADT_ERR_NO_BCCB_TABLE - BCCB table pointer is zero (table not allocated)
	- \ref ADT_ERR_BCCB_NOT_ALLOCATED - No BCCB has been allocated for msgnum
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_BC_CB_CDPRead(ADT_L0_UINT32 devID, ADT_L0_UINT32 msgnum, ADT_L0_UINT32 cdpNum, ADT_L1_1553_CDP *pCdp) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 max_num_msgs, bccbTablePtr, bccbPtr;
	ADT_L0_UINT32 temp, startCdp, selectedCdp, i;
	ADT_L0_UINT32 datablock[ADT_RW_MEM_MAX_SIZE];

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Check that the msgnum does not exceed the maximum number of BC messages */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBSIZE, &max_num_msgs, 1);
	if (msgnum < max_num_msgs) 
	{
		/* Check for valid BCCB table pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBPTR, &bccbTablePtr, 1);
		if (bccbTablePtr != 0) 
		{
			/* Check to see if BCCB has been allocated for the msgnum */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + bccbTablePtr + msgnum*4, &bccbPtr, 1);
			if (bccbPtr != 0)
			{
				/* Check that the selected CDP number does not exceed the actual number of CDPs */
				temp = bccbPtr + ADT_L1_1553_BC_CB_APINUMCDP;
				result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &i, 1);
				if (cdpNum >= i) {
					result = ADT_ERR_BAD_INPUT;
				}

				if (result == ADT_SUCCESS) {
					/* Read the API starting CDP address */
					temp = bccbPtr + ADT_L1_1553_BC_CB_API1STCDP;
					result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &startCdp, 1);


					/********** OPTIMIZED HERE **********/
					/* Changed in v2.6.3.5 */
#ifdef ADT_1553_USE_NONCONTIGUOUS_CDP
					/* Follow NEXT pointers to find the selected CDP */
					i = 0;
					selectedCdp = startCdp;
					while (i < cdpNum) {
						result = ADT_L0_ReadMem32(devID, channelRegOffset + selectedCdp, &temp, 1);
						selectedCdp = temp;
						i++;
					}
#else
					/* THIS ASSUMES THAT ALL CDPS FOR THE MESSAGE WERE ALLOCATED AS CONTIGUOUS BLOCK OF MEMORY
					   AND THAT BUFFERS ARE IN SEQUENCE (won't work if any of the pointers have been changed) */
					selectedCdp = startCdp + (cdpNum * ADT_L1_1553_CDP_SIZE);
#endif
					/************************************/

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
						pCdp->CMD1info = datablock[13];
						pCdp->CMD2info = datablock[14];
						pCdp->STS1info = datablock[15];
						pCdp->STS2info = datablock[16];
						for (i=0; i<32; i++)
							pCdp->DATAinfo[i] = datablock[17+i];

					}
				}
			}
			else result = ADT_ERR_BCCB_NOT_ALLOCATED;  /* A BCCB has not been allocated for msgnum */
		}
		else result = ADT_ERR_NO_BCCB_TABLE;  /* No BCCB Table */
	}
	else result = ADT_ERR_BAD_INPUT;  /* Invalid message number */

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_BC_CB_CDPReadWords
 *****************************************************************************/
/*! \brief Reads 32-bit Words from a CDP for a BC Control Block 
 *
 * This function reads words from a CDP for a BC Control Block.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param msgnum is the message (BC CB) number.
 * @param cdpNum is the buffer (CDP) number.
 * @param wordOffset is the word offset in to the CDP where the read will occur
 * @param numOfWords is the number of words to be stored to pWords
 * @param pWords is a pointer to the array of words to be stored from the CDP
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid message number, wordOffset or numOfWords
	- \ref ADT_ERR_NO_BCCB_TABLE - BCCB table pointer is zero (table not allocated)
	- \ref ADT_ERR_BCCB_NOT_ALLOCATED - No BCCB has been allocated for msgnum
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_BC_CB_CDPReadWords(ADT_L0_UINT32 devID, ADT_L0_UINT32 msgnum, ADT_L0_UINT32 cdpNum,
										 ADT_L0_UINT32 wordOffset, ADT_L0_UINT32 numOfWords, ADT_L0_UINT32 *pWords) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 max_num_msgs, bccbTablePtr, bccbPtr;
	ADT_L0_UINT32 temp, startCdp, selectedCdp, i;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Check that the msgnum does not exceed the maximum number of BC messages */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBSIZE, &max_num_msgs, 1);
	if (msgnum < max_num_msgs) 
	{
		/* Check for valid BCCB table pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBPTR, &bccbTablePtr, 1);
		if (bccbTablePtr != 0) 
		{
			/* Check to see if BCCB has been allocated for the msgnum */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + bccbTablePtr + msgnum*4, &bccbPtr, 1);
			if (bccbPtr != 0)
			{
				/* Check that the selected CDP number does not exceed the actual number of CDPs */
				temp = bccbPtr + ADT_L1_1553_BC_CB_APINUMCDP;
				result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &i, 1);
				if (cdpNum >= i) {
					result = ADT_ERR_BAD_INPUT;
				}

				if (result == ADT_SUCCESS) {
					/* Read the API starting CDP address */
					temp = bccbPtr + ADT_L1_1553_BC_CB_API1STCDP;
					result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &startCdp, 1);

					/********** OPTIMIZED HERE **********/
					/* Changed in v2.6.3.5 */
#ifdef ADT_1553_USE_NONCONTIGUOUS_CDP
					/* Follow NEXT pointers to find the selected CDP */
					i = 0;
					selectedCdp = startCdp;
					while (i < cdpNum) {
						result = ADT_L0_ReadMem32(devID, channelRegOffset + selectedCdp, &temp, 1);
						selectedCdp = temp;
						i++;
					}
#else
					/* THIS ASSUMES THAT ALL CDPS FOR THE MESSAGE WERE ALLOCATED AS CONTIGUOUS BLOCK OF MEMORY
					   AND THAT BUFFERS ARE IN SEQUENCE (won't work if any of the pointers have been changed) */
					selectedCdp = startCdp + (cdpNum * ADT_L1_1553_CDP_SIZE);
#endif
					/************************************/

					if (result == ADT_SUCCESS) {

						/* Read words from the selected CDP offset */
						if ((numOfWords) && ((wordOffset + numOfWords) <= (ADT_L1_1553_CDP_SIZE/4))) 
							result = ADT_L0_ReadMem32(devID, channelRegOffset + selectedCdp + (wordOffset*4), pWords, numOfWords);
						else
							result = ADT_ERR_BAD_INPUT;
					}
				}
			}
			else result = ADT_ERR_BCCB_NOT_ALLOCATED;  /* A BCCB has not been allocated for msgnum */
		}
		else result = ADT_ERR_NO_BCCB_TABLE;  /* No BCCB Table */
	}
	else result = ADT_ERR_BAD_INPUT;  /* Invalid message number */

	return( result );
}

/******************************************************************************
  FUNCTION:		ADT_L1_1553_BC_Start
 *****************************************************************************/
/*! \brief Starts BC operation for the device 
 *
 * This function starts BC operation for the device on the selected message number.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param msgnum is the message number to start the BC on.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid message number
	- \ref ADT_ERR_NO_BCCB_TABLE - BCCB table pointer is zero (table not allocated)
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_BC_Start(ADT_L0_UINT32 devID, ADT_L0_UINT32 msgnum) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, bcCsr;
	ADT_L0_UINT32 max_num_msgs, bccbTablePtr, data;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Check that the msgnum does not exceed the maximum number of BC messages */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBSIZE, &max_num_msgs, 1);
	if (msgnum < max_num_msgs) 
	{
		/* Check for valid BCCB table pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBPTR, &bccbTablePtr, 1);
		if (bccbTablePtr != 0) 
		{
			/* Write the starting message pointer */
			data = Internal_BC_GetPtrFromMsgnum(devID, msgnum);
			result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BC_FIRSTBCCB, &data, 1);

			/* Read the BC CSR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_CSR, &bcCsr, 1);

			/* Set bit 0 (start BC) and clear bit 1 (BC stopped) */
			bcCsr &= ~ADT_L1_1553_BC_CSR_STOPPED;
			bcCsr |= ADT_L1_1553_BC_CSR_RUN;

			/* Write the BC CSR */
			result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BC_CSR, &bcCsr, 1);
		}
		else result = ADT_ERR_NO_BCCB_TABLE;  /* No BCCB Table */
	}
	else result = ADT_ERR_BAD_INPUT;  /* Invalid message number */

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_BC_Stop
 *****************************************************************************/
/*! \brief Stops BC operation for the device 
 *
 * This function stops BC operation for the device.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_BC_Stop(ADT_L0_UINT32 devID) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, bcCsr;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Read the BC CSR */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_CSR, &bcCsr, 1);

	/* Clear bit 0 */
	bcCsr &= ~ADT_L1_1553_BC_CSR_RUN;

	/* Write the BC CSR */
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BC_CSR, &bcCsr, 1);

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_BC_AperiodicSend
 *****************************************************************************/
/*! \brief Sends BC Aperiodic messages 
 *
 * This function injects BC Aperiodic messages into a running frame.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param priority is 0 for LOW priority or 1 for HIGH priority.
 * @param msgnum is the message number for the aperiodic message to send.
 * @param LPAMtime is the low priority aperiodic message time (100ns LSB).
 *     - If this is ZERO, then the LPAM will be sent at end of frame no
 *       matter how much or little time is remaining in the frame.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid message number
	- \ref ADT_ERR_NO_BCCB_TABLE - BCCB table pointer is zero (table not allocated)
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_BC_AperiodicSend(ADT_L0_UINT32 devID, ADT_L0_UINT32 priority, ADT_L0_UINT32 msgnum, ADT_L0_UINT32 LPAMtime) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 max_num_msgs, bccbTablePtr, data;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Check that the msgnum does not exceed the maximum number of BC messages */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBSIZE, &max_num_msgs, 1);
	if (msgnum < max_num_msgs) 
	{
		/* Check for valid BCCB table pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBPTR, &bccbTablePtr, 1);
		if (bccbTablePtr != 0) 
		{
			if (priority == 0)  /***** LOW PRIORITY aperiodic *****/
			{
				/* Write the estimated time needed to the LPAM TIME register */
				result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BC_LPAMTIME, &LPAMtime, 1);

				/* Write the starting message pointer to the LPAM POINTER register */
				data = Internal_BC_GetPtrFromMsgnum(devID, msgnum);
				result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BC_LPAMPTR, &data, 1);
			}
			else  /***** HIGH PRIORITY aperiodic *****/
			{
				/* Write the starting message pointer to the HPAM POINTER register */
				data = Internal_BC_GetPtrFromMsgnum(devID, msgnum);
				result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_BC_HPAMPTR, &data, 1);
			}
		}
		else result = ADT_ERR_NO_BCCB_TABLE;  /* No BCCB Table */
	}
	else result = ADT_ERR_BAD_INPUT;  /* Invalid message number */

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_BC_IsRunning
 *****************************************************************************/
/*! \brief Determines if the BC is running or stopped 
 *
 * This function determines if the BC is running or stopped.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param pIsRunning is a pointer to store the result, 0=NotRunning, 1=Running.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_BC_IsRunning(ADT_L0_UINT32 devID, ADT_L0_UINT32 *pIsRunning) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, bcCsr;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Read the BC CSR */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_CSR, &bcCsr, 1);

	/* Check the "BC STOPPED" bit (bit 1) */
	if (bcCsr & 0x00000002)
		*pIsRunning = 0;
	else
		*pIsRunning = 1;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_BC_AperiodicIsRunning
 *****************************************************************************/
/*! \brief Determines if the BC is processing aperiodic messages 
 *
 * This function determines if the BC is processing aperiodic messages.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param priority is 0 for LOW priority or 1 for HIGH priority.
 * @param pIsRunning is a pointer to store the result, 0=NotRunning, 1=Running.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_BC_AperiodicIsRunning(ADT_L0_UINT32 devID, ADT_L0_UINT32 priority, ADT_L0_UINT32 *pIsRunning) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, AMptr;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (priority == 0)  /***** LOW PRIORITY aperiodic *****/
	{
		/* Read the LPAM pointer register */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_LPAMPTR, &AMptr, 1);
	}
	else  /***** HIGH PRIORITY aperiodic *****/
	{
		/* Read the HPAM pointer register */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_HPAMPTR, &AMptr, 1);
	}

	/* If NON-ZERO then firmware is still processing aperiodic messages */
	if (AMptr != 0x00000000)
		*pIsRunning = 1;
	else
		*pIsRunning = 0;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_BC_GetFrameCount
 *****************************************************************************/
/*! \brief Reads the BC total frame count. 
 *
 * This function reads the BC total frame count.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param pFrameCount is a pointer to store the frame count.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_BC_GetFrameCount(ADT_L0_UINT32 devID, ADT_L0_UINT32 *pFrameCount) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, frameCount;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Read the BC TOTAL FRAME COUNT register */
	frameCount = 0;
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_TOTALFRMCNT, &frameCount, 1);

	*pFrameCount = frameCount;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_BC_InjCmdWordError
 *****************************************************************************/
/*! \brief Configures a BC message for errors on the command word 
 *
 * This function configures a BC message for errors on the command word.
 * There are three parameters to select the error type to inject.  These are
 * syncErr, manchesterErr, and parityErr.  These parameters should be used
 * in one of the following ways:
 * If syncErr, manchesterErr, and parityErr are all zero, no error will be 
 * injected.
 * If syncErr is non-zero, the sync pattern will be inverted.
 * If manchesterErr is non-zero, a manchester (zero-crossing) error will be
 * injected on bit 3 of the word.
 * If parityErr is non-zero, the parity bit will be inverted.
 * if both manchesterErr and parityErr are non-zero, a manchester (zero-crossing)
 * error will be injected on the parity bit.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param msgNum is the message number.
 * @param cmd1or2 selects the CMD word (1 or 2).
 * @param syncErr set to 1 for sync error or 0 for no sync error.
 * @param manchesterErr set to 1 for manchester error or 0 for no manchester error.
 * @param parityErr set to 1 for parity error or 0 for no parity error.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid message number
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_BC_InjCmdWordError(ADT_L0_UINT32 devID, ADT_L0_UINT32 msgNum, ADT_L0_UINT32 cmd1or2, 
											 ADT_L0_UINT32 syncErr, ADT_L0_UINT32 manchesterErr, ADT_L0_UINT32 parityErr) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 data, bccbOffset;
	ADT_L0_UINT32 channel, channelRegOffset;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Get the BCCB Offset */
	bccbOffset = Internal_BC_GetPtrFromMsgnum(devID, msgNum);

	if (bccbOffset != 0x00000000) {
		/* Read the BC CB CMD word */
		if (cmd1or2 == 1)
			result = ADT_L0_ReadMem32(devID, channelRegOffset + bccbOffset + ADT_L1_1553_BC_CB_CMD1INFO, &data, 1);
		else
			result = ADT_L0_ReadMem32(devID, channelRegOffset + bccbOffset + ADT_L1_1553_BC_CB_CMD2INFO, &data, 1);

		/* Sync Error */
		if (syncErr) {
			/* Set bit 30 to inject a sync error */
			data |= 0x40000000;
		}
		else {
			/* Clear bit 30 to for no sync error */
			data &= 0xBFFFFFFF;
		}

		/* Manchester Error */
		if (manchesterErr) {
			/* Set bit 29 to inject a sync error */
			data |= 0x20000000;
		}
		else {
			/* Clear bit 29 to for no sync error */
			data &= 0xDFFFFFFF;
		}

		/* Parity Error */
		if (parityErr) {
			/* Set bit 28 to inject a sync error */
			data |= 0x10000000;
		}
		else {
			/* Clear bit 28 to for no sync error */
			data &= 0xEFFFFFFF;
		}

		/* Write the BC CB CMD word */
		if (cmd1or2 == 1)
			result = ADT_L0_WriteMem32(devID, channelRegOffset + bccbOffset + ADT_L1_1553_BC_CB_CMD1INFO, &data, 1);
		else
			result = ADT_L0_WriteMem32(devID, channelRegOffset + bccbOffset + ADT_L1_1553_BC_CB_CMD2INFO, &data, 1);
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_BC_ReadCmdWordError
 *****************************************************************************/
/*! \brief Reads the settings for a BC message for errors on the command word 
 *
 * This function reads the settings for a BC message for errors on the command word.
 * There are three parameters that determine the error settings.  These are
 * syncErr, manchesterErr, and parityErr.  These parameters should be used
 * in one of the following ways:
 * If syncErr, manchesterErr, and parityErr are all zero, no error will be 
 * injected.
 * If syncErr is non-zero, the sync pattern will be inverted.
 * If manchesterErr is non-zero, a manchester (zero-crossing) error will be
 * injected on bit 3 of the word.
 * If parityErr is non-zero, the parity bit will be inverted.
 * if both manchesterErr and parityErr are non-zero, a manchester (zero-crossing)
 * error will be injected on the parity bit.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param msgNum is the message number.
 * @param cmd1or2 selects the CMD word (1 or 2).
 * @param pSyncErr is ptr to store sync error (1 for sync error or 0 for no sync error).
 * @param pManchesterErr is ptr to store manch error (1 for manch error or 0 for no manch error).
 * @param pParityErr ptr to store parity error (1 for parity error or 0 for no parity error).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid message number
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_BC_ReadCmdWordError(ADT_L0_UINT32 devID, ADT_L0_UINT32 msgNum, ADT_L0_UINT32 cmd1or2, 
											 ADT_L0_UINT32 *pSyncErr, ADT_L0_UINT32 *pManchesterErr, 
											 ADT_L0_UINT32 *pParityErr) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 data, bccbOffset;
	ADT_L0_UINT32 channel, channelRegOffset;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Get the BCCB Offset */
	bccbOffset = Internal_BC_GetPtrFromMsgnum(devID, msgNum);

	if (bccbOffset != 0x00000000) {
		/* Read the BC CB CMD word */
		if (cmd1or2 == 1)
			result = ADT_L0_ReadMem32(devID, channelRegOffset + bccbOffset + ADT_L1_1553_BC_CB_CMD1INFO, &data, 1);
		else
			result = ADT_L0_ReadMem32(devID, channelRegOffset + bccbOffset + ADT_L1_1553_BC_CB_CMD2INFO, &data, 1);

		/* Sync Error */
		if (data & 0x40000000) *pSyncErr = 1;
		else *pSyncErr = 0;

		/* Manchester Error */
		if (data & 0x20000000) *pManchesterErr = 1;
		else *pManchesterErr = 0;

		/* Parity Error */
		if (data & 0x10000000) *pParityErr = 1;
		else *pParityErr = 0;
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_1553_BC_CB_SetAddressBranchValues
 *****************************************************************************/
/*! \brief Sets the Referenced BCCB Address Branch Values from User BCCB-CDP Inputs
 *
 * This function Sets the Referenced BCCB Address Branch Values from User BCCB-CDP Inputs.
 *
 * WARNING:
 * This is only supported in PE version B505 and later!
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param msgNum is the working branch BCCB number to set the Branch BCCB Address, Memory Compare Address, Mask and Value.
 * @param srcMsgnum is the source message (BC CB) number for branch comparison.
 * @param srcCdpNum is the source buffer (CDP) number for branch comparison.
 * @param srcWordOffset is the word offset in to the CDP where the read will occur.
 * @param maskValue is the AND Mask pattern for the branch comparison.
 * @param compareValue is the value expected to trigger a branch (after mask applied).
 * @param destMsgNum is the destination BCCB number to branch to if comparison is true.
,

 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid message number, wordOffset or numOfWords
	- \ref ADT_ERR_NO_BCCB_TABLE - BCCB table pointer is zero (table not allocated)
	- \ref ADT_ERR_BCCB_NOT_ALLOCATED - No BCCB has been allocated for msgnum
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_BC_CB_SetAddressBranchValues(ADT_L0_UINT32 devID, ADT_L0_UINT32 msgNum, 
			   ADT_L0_UINT32 srcMsgNum, ADT_L0_UINT32 srcCdpNum, ADT_L0_UINT32 srcWordOffset, 
			   ADT_L0_UINT32 maskValue, ADT_L0_UINT32 compareValue, ADT_L0_UINT32 destMsgNum) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 max_num_msgs, bccbTablePtr, bccbPtr, bccbPtr2;
	ADT_L0_UINT32 temp, selectedCdp, i, branchWords[4];

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Check that srcMsgnum & destMsgNum do not exceed the maximum number of BC messages */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBSIZE, &max_num_msgs, 1);
	if ((srcMsgNum < max_num_msgs) && (destMsgNum < max_num_msgs)) 
	{
		/* Check for valid BCCB table pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBPTR, &bccbTablePtr, 1);
		if (bccbTablePtr != 0) 
		{
			/* Retrieve the BCCB Table Pointer Values */ 
			result = ADT_L0_ReadMem32(devID, channelRegOffset + bccbTablePtr + (srcMsgNum*4), &bccbPtr, 1);
			result = ADT_L0_ReadMem32(devID, channelRegOffset + bccbTablePtr + (destMsgNum*4), &bccbPtr2, 1);

			/* Check to see if BCCBs have been allocated for srcMsgnum & destMsgNum */
			if ((bccbPtr != 0) && (bccbPtr2 !=0))
			{
				/* Get Source CDP Address */
				/* Check that the selected souce BCCB CDP number does not exceed the actual number of CDPs */
				temp = bccbPtr + ADT_L1_1553_BC_CB_APINUMCDP;
				result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &i, 1);
				if (srcCdpNum >= i) 
					result = ADT_ERR_BAD_INPUT;

				/* Number is good - get CDP address */
				else 
				{
					/* Read the API starting CDP address */
					temp = bccbPtr + ADT_L1_1553_BC_CB_API1STCDP;
					result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &selectedCdp, 1);

					/* Follow NEXT pointers to find the address for the selected CDP */
					i = 0;
					while (i < srcCdpNum) 
					{
						result = ADT_L0_ReadMem32(devID, channelRegOffset + selectedCdp, &temp, 1);
						selectedCdp = temp;
						i++;
					}

					if (result == ADT_SUCCESS) 
					{
						/* Calc Final Address on CDP plus User Offset */
						selectedCdp += srcWordOffset * 4;
						/* Set the referenced BCCB Address Branch Values */
						branchWords[0] = bccbPtr2;
						branchWords[1] = selectedCdp;
						branchWords[2] = maskValue;
						branchWords[3] = compareValue;
						result = ADT_L1_1553_BC_CB_WriteWords(devID, msgNum, ADT_L1_1553_BC_CB_BRANCHADD/4, 4, branchWords);
					}
				}
			}
			else result = ADT_ERR_BCCB_NOT_ALLOCATED;  /* A BCCB has not been allocated for msgnums */
		}
		else result = ADT_ERR_NO_BCCB_TABLE;  /* No BCCB Table */
	}
	else 
		result = ADT_ERR_BAD_INPUT;  /* Invalid message number */

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_BC_CB_GetAddr
 *****************************************************************************/
/*! \brief Gets byte address of a BCCB
 *
 * This function gets byte address of a BC Control Block.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param msgnum is the message number (zero-indexed) of the BCCB.
 * @param pAddr is a pointer to store the address.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid message number
	- \ref ADT_ERR_NO_BCCB_TABLE - BCCB table pointer is zero (table not allocated)
	- \ref ADT_ERR_BCCB_NOT_ALLOCATED - No BCCB has been allocated for msgnum
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_BC_CB_GetAddr(ADT_L0_UINT32 devID, 
															ADT_L0_UINT32 msgnum, 
															ADT_L0_UINT32 *pAddr) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 max_num_msgs, bccbTablePtr, bccbPtr;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Read max message number, verify that msgnum is valid */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBSIZE, &max_num_msgs, 1);
	if (msgnum < max_num_msgs) 
	{
		/* Check for valid BCCB table pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBPTR, &bccbTablePtr, 1);
		if (bccbTablePtr != 0) 
		{
			/* Check to see if BCCB has been allocated for the msgnum */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + bccbTablePtr + (msgnum*4), &bccbPtr, 1);
			if (bccbPtr != 0)
			{
				*pAddr = bccbPtr;
			}
			else result = ADT_ERR_BCCB_NOT_ALLOCATED;  /* A BCCB has not been allocated for msgnum */
		}
		else result = ADT_ERR_NO_BCCB_TABLE;  /* No BCCB Table */
	}
	else result = ADT_ERR_BAD_INPUT;  /* Invalid message number */

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_BC_CB_CDP_GetAddr
 *****************************************************************************/
/*! \brief Gets the byte address of a BC CDP 
 *
 * This function gets the byte address of a BC CDP.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param msgnum is the message (BC CB) number.
 * @param cdpNum is the buffer (CDP) number.
 * @param pAddr is a pointer to store the address.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid message number or CDP number
	- \ref ADT_ERR_NO_BCCB_TABLE - BCCB table pointer is zero (table not allocated)
	- \ref ADT_ERR_BCCB_NOT_ALLOCATED - No BCCB has been allocated for msgnum
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_BC_CB_CDP_GetAddr(ADT_L0_UINT32 devID, 
															 ADT_L0_UINT32 msgnum, 
															 ADT_L0_UINT32 cdpNum,
															 ADT_L0_UINT32 *pAddr) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 max_num_msgs, bccbTablePtr, bccbPtr;
	ADT_L0_UINT32 temp, startCdp, selectedCdp, i;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Check that the msgnum does not exceed the maximum number of BC messages */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBSIZE, &max_num_msgs, 1);
	if (msgnum < max_num_msgs) 
	{
		/* Check for valid BCCB table pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_BC_APIBCCBPTR, &bccbTablePtr, 1);
		if (bccbTablePtr != 0) 
		{
			/* Check to see if BCCB has been allocated for the msgnum */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + bccbTablePtr + msgnum*4, &bccbPtr, 1);
			if (bccbPtr != 0)
			{
				/* Check that the selected CDP number does not exceed the actual number of CDPs */
				temp = bccbPtr + ADT_L1_1553_BC_CB_APINUMCDP;
				result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &i, 1);
				if (cdpNum >= i) {
					result = ADT_ERR_BAD_INPUT;
				}

				if (result == ADT_SUCCESS) {
					/* Read the API starting CDP address */
					temp = bccbPtr + ADT_L1_1553_BC_CB_API1STCDP;
					result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &startCdp, 1);

					/********** OPTIMIZED HERE **********/
					/* Changed in v2.6.3.5 */
#ifdef ADT_1553_USE_NONCONTIGUOUS_CDP
					/* Follow NEXT pointers to find the selected CDP */
					i = 0;
					selectedCdp = startCdp;
					while (i < cdpNum) {
						result = ADT_L0_ReadMem32(devID, channelRegOffset + selectedCdp, &temp, 1);
						selectedCdp = temp;
						i++;
					}
#else
					/* THIS ASSUMES THAT ALL CDPS FOR THE MESSAGE WERE ALLOCATED AS CONTIGUOUS BLOCK OF MEMORY
					   AND THAT BUFFERS ARE IN SEQUENCE (won't work if any of the pointers have been changed) */
					selectedCdp = startCdp + (cdpNum * ADT_L1_1553_CDP_SIZE);
#endif
					/************************************/


					if (result == ADT_SUCCESS) {
						*pAddr = selectedCdp;
					}
				}
			}
			else result = ADT_ERR_BCCB_NOT_ALLOCATED;  /* A BCCB has not been allocated for msgnum */
		}
		else result = ADT_ERR_NO_BCCB_TABLE;  /* No BCCB Table */
	}
	else result = ADT_ERR_BAD_INPUT;  /* Invalid message number */

	return( result );
}





#ifdef __cplusplus
}
#endif
