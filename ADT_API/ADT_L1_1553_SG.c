/******************************************************************************
 * FILE:			ADT_L1_1553_SG.c
 *
 * Copyright (c) 2007-2016, Alta Data Technologies LLC (ADT), All Rights Reserved.
 * Use of this software is subject to the ADT Software License (latest
 * revision), US Government or local laws and the ADT Terms and Conditions 
 * of Sale (latest revision).
 *
 * DESCRIPTION:
 *	Source file for Layer 1 API.
 *	Contains 1553 Signal Generator functions.  Only supports 2-bit SG mode.
 *
 *	TO DO:
 *	   Add protection against SG and BC/PB at same time.
 *     Create_SGCB - CONSIDER ADDING PARAMETER FOR OPTIONS - WAIT FOR TRG IN, 
 *                   GEN TRG OUT, GEN INTERRUPT, INCR COUNTER
 *
 *****************************************************************************/
/*! \file ADT_L1_1553_SG.c  
 *  \brief Source file containing 1553 Signal Generator functions 
 */
#include <stdio.h>
#include "ADT_L1.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
  FUNCTION:		ADT_L1_1553_SG_Configure
 *****************************************************************************/
/*! \brief Configures Signal Generator operation for the device 
 *
 * This function configures Signal Generator operation for the device by 
 * initializing the SG registers.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not a 1553 channel.
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_SG_Configure(ADT_L0_UINT32 devID) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 data, channel, channelRegOffset;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Clear SG registers */
	data = 0x00000000;
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_SG_STARTSGCB, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_SG_CURRSGCB, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_SG_USERCOUNT, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_SG_PECOUNT, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_SG_PEOFFSET, &data, 1);

	/* Read the PE CSR, clear bit 5 to enable 2-bit SG */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &data, 1);
	if (result == ADT_SUCCESS) {
		data &= ~0x00000020;  /* Clear bit 5 */
		result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &data, 1);
	}

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_SG_CreateSGCB
 *****************************************************************************/
/*! \brief Allocates and writes a SGCB 
 *
 * This function allocates and writes a SGCB to the device.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param bus is the 1553 bus (A or B).
 * @param timeHigh is the upper 32 bits of the time for this SGCB.
 * @param timeLow is the lower 32 bits of the time for this SGCB.
 * @param pVectors is a pointer to an array of 32-bit words containing vectors.
 * @param numVectors is the number of vectors (in bits, may use only part of the last word).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not a 1553 channel.
	- \ref ADT_FAILURE - Completed with error

	CONSIDER ADDING PARAMETER FOR OPTIONS - WAIT FOR TRG IN, GEN TRG OUT, GEN INTERRUPT, INCR COUNTER

*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_SG_CreateSGCB(ADT_L0_UINT32 devID, char bus, ADT_L0_UINT32 timeHigh, 
										ADT_L0_UINT32 timeLow, ADT_L0_UINT32 *pVectors, ADT_L0_UINT32 numVectors) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 thisSGCB, tempSGCB, lastSGCB, numWords, i;
	ADT_L0_UINT32 data, channel, channelRegOffset;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Determine size of memory to allocate for the SGCB */
	numWords = 5 + numVectors / 16;
	if ((numVectors % 16) != 0) numWords++;

	/* Allocate board memory for the SGCB */
	result = ADT_L1_MemoryAlloc(devID, numWords*4, &thisSGCB);
	if (result == ADT_SUCCESS) {
		/* Write SGCB Header Words */
		data = 0x00000000;  /* SGCB NEXT PTR (0x00000000 means STOP, this is last SGCB) */
		result = ADT_L0_WriteMem32(devID, channelRegOffset + thisSGCB + ADT_L1_1553_SGCB_NEXTPTR, &data, 1);
		if ((bus == 'A')||(bus == 'a'))  /* SGCB CSR - Select bus */
			data = 0x80000000;  /* Bus A */
		else
			data = 0x00000000;  /* Bus B */
		result = ADT_L0_WriteMem32(devID, channelRegOffset + thisSGCB + ADT_L1_1553_SGCB_CSR, &data, 1);
		data = timeHigh;  /* SGCB Time High */
		result = ADT_L0_WriteMem32(devID, channelRegOffset + thisSGCB + ADT_L1_1553_SGCB_TIMEHIGH, &data, 1);
		data = timeLow;  /* SGCB Time Low */
		result = ADT_L0_WriteMem32(devID, channelRegOffset + thisSGCB + ADT_L1_1553_SGCB_TIMELOW, &data, 1);
		data = numVectors*2;  /* SGCB Vector Count */
		result = ADT_L0_WriteMem32(devID, channelRegOffset + thisSGCB + ADT_L1_1553_SGCB_VECCOUNT, &data, 1);

		/* Write the vector words */
		for (i=0; i<numWords-5; i++) {
			result = ADT_L0_WriteMem32(devID, channelRegOffset + thisSGCB + ADT_L1_1553_SGCB_VECTORS + i*4, &pVectors[i], 1);
		}

		/* Look for the last existing SGCB and link to this SGCB */
		if (result == ADT_SUCCESS) {
			/* Read the starting SGCB Address */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_SG_STARTSGCB, &lastSGCB, 1);

			/* HOW TO HANDLE 0xFFFFFFFF (SG already ran and stopped)?  FAIL OUT FOR NOW */
			if (lastSGCB == 0xFFFFFFFF) {
				result = ADT_FAILURE;
				return( result );
			}

			if (lastSGCB == 0x00000000) {  /* If this is the first SGCB to be defined . . . */
				/* Write the pointer to the new SGCB as the start SGCB address */
				result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_SG_STARTSGCB, &thisSGCB, 1);
			}
			else {  /* If other SGCBs have already been defined . . . */
				/* Find the last SGCB and set it's NEXT pointer to the new SGCB */
				/* NOTE: This assumes that SGCBs do NOT loop - last one points to 0x00000000 */
				tempSGCB = lastSGCB;
				while (tempSGCB != 0x00000000) {
					lastSGCB = tempSGCB;
					result = ADT_L0_ReadMem32(devID, channelRegOffset + lastSGCB, &tempSGCB, 1);
				}
				result = ADT_L0_WriteMem32(devID, channelRegOffset + lastSGCB, &thisSGCB, 1);
			}

			if (result == ADT_SUCCESS) {
				/* Increment the SGCB user count */
				result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_SG_USERCOUNT, &data, 1);
				data++;
				result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_SG_USERCOUNT, &data, 1);
			}
		}
	}

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_1553_SG_Free
 *****************************************************************************/
/*! \brief Frees all SG CBs for the device
 *
 * This function frees all SG CBs for the device.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not a 1553 channel.
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_SG_Free(ADT_L0_UINT32 devID) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 startSGCBptr, currSGCBptr, nextSGCBptr, data;
	ADT_L0_UINT32 numVectors, numWords;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Make sure SG is stopped */
	result = ADT_L1_1553_SG_Stop(devID);

	/* Get the starting SGCB pointer */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_SG_STARTSGCB, &startSGCBptr, 1);
	currSGCBptr = startSGCBptr;
	nextSGCBptr = 1;

	/* Iterate through SGCBs and free them until back to start ptr */
	while ((result == ADT_SUCCESS) && (nextSGCBptr != startSGCBptr) && (nextSGCBptr != 0)) {
		/* Get the next SGCB pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + currSGCBptr + ADT_L1_1553_SGCB_NEXTPTR, &nextSGCBptr, 1);

		/* Get the number of vectors */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + currSGCBptr + ADT_L1_1553_SGCB_VECCOUNT, &numVectors, 1);

		/* Determine the number of words in the SGCB */
		numWords = 5 + numVectors / 32;
		if ((numVectors % 32) != 0) numWords++;

		/* Free the current PB packet */
		result = ADT_L1_MemoryFree(devID, currSGCBptr, numWords * 4);

		/* Move to next SG CB */
		currSGCBptr = nextSGCBptr;
	}

	/* Clear the SG registers */
	data = 0;
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_SG_STARTSGCB, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_SG_CURRSGCB, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_SG_USERCOUNT, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_SG_PECOUNT, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_SG_PEOFFSET, &data, 1);

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_SG_Start
 *****************************************************************************/
/*! \brief Starts Signal Generator operation for the device 
 *
 * This function starts Signal Generator operation for the device.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not a 1553 channel.
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_SG_Start(ADT_L0_UINT32 devID) {
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

	/* Clear the register for current SGCB */
	data = 0x00000000;
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_SG_CURRSGCB, &data, 1);

	/* Read the PE CSR, set bit 4 to start SG */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &data, 1);
	if (result == ADT_SUCCESS) {
		data |= 0x00000010;  /* Set bit 4 */
		result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &data, 1);
	}

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_SG_Stop
 *****************************************************************************/
/*! \brief Stops Signal Generator operation for the device 
 *
 * This function stops Signal Generator operation for the device.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not a 1553 channel.
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_SG_Stop(ADT_L0_UINT32 devID) {
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

	/* Read the PE CSR, clear bit 4 to stop SG */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &data, 1);
	if (result == ADT_SUCCESS) {
		data &= 0xFFFFFFEF;  /* Clear bit 4 */
		result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &data, 1);
	}

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_1553_SG_IsRunning
 *****************************************************************************/
/*! \brief Determines if the SG is running. 
 *
 * This function determines if the SG is running.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param pIsRunning is a pointer to store the result, 0=NotRunning, 1=Running.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not a 1553 channel.
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_SG_IsRunning(ADT_L0_UINT32 devID, ADT_L0_UINT32 *pIsRunning) {
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

	/* Read the SG CURRENT PTR, if zero then SG is stopped */
	data = 0;
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_SG_CURRSGCB, &data, 1);
	if (data == 0x00000000)
		*pIsRunning = 0;
	else
		*pIsRunning = 1;

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_1553_SG_WordToVectors
 *****************************************************************************/
/*! \brief Adds a 1553 word to a list of vectors. 
 *
 * This function adds a 1553 word to a list of vectors.
 *
 * @param m1553word is the 16-bit 1553 word (bit 31 is SYNC (1=cmd sync, 0 = data sync), bit 30 is PARITY (1=parity error), bits 16-29 not used, set to zero).
 * @param pVectors is a pointer to the array of 32-bit words containing vectors.
 * @param sizeInWords is the max size (in words) of the vector array.
 * @param pNumVectors is a pointer to a UINT32 containing the current vector count (in 2-bit 20ns vectors).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_FAILURE - Completed with error - not enough words available in vectors array
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_SG_WordToVectors(ADT_L0_UINT32 m1553word, 
										ADT_L0_UINT32 *pVectors, 
										ADT_L0_UINT32 sizeInWords, 
										ADT_L0_UINT32 *pNumVectors) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 mask, i, parity = 0;

	/* Add SYNC (three bit-times) */
	if (m1553word & 0x80000000) {  /* CMD SYNC */
		result = ADT_L1_1553_SG_AddVectors(75, ADT_L1_1553_SG_VECTOR_HIGH, pVectors, sizeInWords, pNumVectors);
		result = ADT_L1_1553_SG_AddVectors(75, ADT_L1_1553_SG_VECTOR_LOW, pVectors, sizeInWords, pNumVectors);
	}
	else {  /* DATA SYNC */
		result = ADT_L1_1553_SG_AddVectors(75, ADT_L1_1553_SG_VECTOR_LOW, pVectors, sizeInWords, pNumVectors);
		result = ADT_L1_1553_SG_AddVectors(75, ADT_L1_1553_SG_VECTOR_HIGH, pVectors, sizeInWords, pNumVectors);
	}

	/* Add the data bits (sixteen bit-times) */
	for (i=16; i>0; i--) {
		mask = 1; 
		mask = mask << (i-1);
		if (m1553word & mask) {
			parity++;
			result = ADT_L1_1553_SG_AddVectors(25, ADT_L1_1553_SG_VECTOR_HIGH, pVectors, sizeInWords, pNumVectors);
			result = ADT_L1_1553_SG_AddVectors(25, ADT_L1_1553_SG_VECTOR_LOW, pVectors, sizeInWords, pNumVectors);
		}
		else {
			result = ADT_L1_1553_SG_AddVectors(25, ADT_L1_1553_SG_VECTOR_LOW, pVectors, sizeInWords, pNumVectors);
			result = ADT_L1_1553_SG_AddVectors(25, ADT_L1_1553_SG_VECTOR_HIGH, pVectors, sizeInWords, pNumVectors);
		}
	}

	/* Add parity (one bit-time) */
	if (m1553word & 0x40000000) {  /* Set parity error if selected */
		parity++;
	}
	if (parity & 0x0001) {  /* Make parity bit zero */
		result = ADT_L1_1553_SG_AddVectors(25, ADT_L1_1553_SG_VECTOR_LOW, pVectors, sizeInWords, pNumVectors);
		result = ADT_L1_1553_SG_AddVectors(25, ADT_L1_1553_SG_VECTOR_HIGH, pVectors, sizeInWords, pNumVectors);
	}
	else {  /* Make parity bit one */
		result = ADT_L1_1553_SG_AddVectors(25, ADT_L1_1553_SG_VECTOR_HIGH, pVectors, sizeInWords, pNumVectors);
		result = ADT_L1_1553_SG_AddVectors(25, ADT_L1_1553_SG_VECTOR_LOW, pVectors, sizeInWords, pNumVectors);
	}

	return( result );
}




/******************************************************************************
  FUNCTION:		ADT_L1_1553_SG_AddVectors
 *****************************************************************************/
/*! \brief Adds a sequence of high vectors to a list of vectors. 
 *
 * This function adds a sequence of vectors to a list of vectors.
 *
 * @param numVec is the number of 2-bit vectors to add (1 vector = 20ns).
 * @param vector2bit is the 2-bit pattern to add for each vector (00=GND, 01=LOW, 10=HIGH, 11=GND).
 * @param pVectors is a pointer to the array of 32-bit words containing vectors.
 * @param sizeInWords is the max size (in words) of the vector array.
 * @param pNumVectors is a pointer to a UINT32 containing the current vector count (in bits).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_SG_AddVectors(ADT_L0_UINT32 numVec, ADT_L0_UINT32 vector2bit, 
										ADT_L0_UINT32 *pVectors, ADT_L0_UINT32 sizeInWords, ADT_L0_UINT32 *pNumVectors)
{
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 wordIndex, bitIndex;
	ADT_L0_UINT32 i, orMask, andMask;

	wordIndex = *pNumVectors / 16;  /* 16 2-bit vectors per 32-bit word */
	bitIndex  = 32 - ((*pNumVectors % 16) * 2);

	if (wordIndex < sizeInWords) {
		for (i=0; i<numVec; i++) {
			/* andMask is used to CLEAR the two bits for the vector */
			andMask = 0x00000003;
			andMask = andMask << (bitIndex-2);
			andMask = ~andMask;
			pVectors[wordIndex] &= andMask;

			/* orMask is used to SET the appropriate bits for the vector */
			orMask = vector2bit & 0x00000003;
			orMask = orMask << (bitIndex-2);
			pVectors[wordIndex] |= orMask;
			bitIndex -= 2;

			pNumVectors[0]++;
			if (bitIndex == 0) {
				wordIndex++;
				bitIndex = 32;
				if (wordIndex >= sizeInWords)
					return(ADT_FAILURE);
			}
		}
	}
	else result = ADT_FAILURE;

	return( result );
}



#ifdef __cplusplus
}
#endif
