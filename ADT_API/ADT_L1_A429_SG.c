/******************************************************************************
 * FILE:			ADT_L1_A429_SG.c
 *
 * Copyright (c) 2007-2016, Alta Data Technologies LLC (ADT), All Rights Reserved.
 * Use of this software is subject to the ADT Software License (latest
 * revision), US Government or local laws and the ADT Terms and Conditions 
 * of Sale (latest revision).
 *
 * DESCRIPTION:
 *	Source file for Layer 1 API.
 *	Contains A429 signal generator functions.
 *
 *****************************************************************************/
/*! \file ADT_L1_A429_SG.c  
 *  \brief Source file containing A429 signal generator functions 
 */
#include "ADT_L1.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
  FUNCTION:		ADT_L1_A429_SG_Configure
 *****************************************************************************/
/*! \brief Configures Signal Generator operation for the device 
 *
 * This function configures Signal Generator operation for the device by 
 * initializing the SG registers and assigning a TX channel.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param txChannel is the TX channel (0-15) to use for the signal generator.
 * @param slewRate selects low-speed (0) or high-speed (1) slew rate for the transmit channel.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device.
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_SG_Configure(ADT_L0_UINT32 devID, ADT_L0_UINT32 txChannel, ADT_L0_UINT32 slewRate) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 data, channel, channelRegOffset;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Clear SG registers */
	data = 0x00000000;
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_SGREG_1ST_SGCB_PTR, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_SGREG_CUR_SGCB_PTR, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_SGREG_USER_COUNTER, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_SGREG_PE_COUNTER, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_SGREG_PE_OFFSET, &data, 1);

	/* Read the PE CSR, set the txChannel in bits 16-19 */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_A429_PE_ROOT_CSR, &data, 1);
	if (result == ADT_SUCCESS) {
		data &= ~0x000F0000;						/* Clear bits 16-19 */
		data |= (txChannel & 0x0000000F) << 16;		/* set the TX channel */
		result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_PE_ROOT_CSR, &data, 1);
	}

	/* Read the TX CSR2 for the channel, select slew rate */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_A429_ROOT_TX_REGS + ADT_L1_A429_TXREG_TX_CSR2, &data, 1);
	data &= ~ADT_L1_A429_TXREG_TX_CSR2_HISLEWRATE;
	if (slewRate != 0) data |= ADT_L1_A429_TXREG_TX_CSR2_HISLEWRATE;
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_ROOT_TX_REGS + ADT_L1_A429_TXREG_TX_CSR2, &data, 1);

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_A429_SG_CreateSGCB
 *****************************************************************************/
/*! \brief Allocates and writes a SGCB 
 *
 * This function allocates and writes a SGCB to the device.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param timeHigh is the upper 32 bits of the time for this SGCB.
 * @param timeLow is the lower 32 bits of the time for this SGCB.
 * @param pVectors is a pointer to an array of 32-bit words containing vectors.
 * @param numVectors is the number of vectors (in bits, may use only part of the last word).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device.
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_SG_CreateSGCB(ADT_L0_UINT32 devID, ADT_L0_UINT32 timeHigh, 
										ADT_L0_UINT32 timeLow, ADT_L0_UINT32 *pVectors, ADT_L0_UINT32 numVectors) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 thisSGCB, tempSGCB, lastSGCB, numWords, i;
	ADT_L0_UINT32 data, channel, channelRegOffset;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
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
		result = ADT_L0_WriteMem32(devID, channelRegOffset + thisSGCB + ADT_L1_A429_SGCB_NEXTPTR, &data, 1);
		result = ADT_L0_WriteMem32(devID, channelRegOffset + thisSGCB + ADT_L1_A429_SGCB_CSR, &data, 1);
		data = timeHigh;  /* SGCB Time High */
		result = ADT_L0_WriteMem32(devID, channelRegOffset + thisSGCB + ADT_L1_A429_SGCB_TIMEHIGH, &data, 1);
		data = timeLow;  /* SGCB Time Low */
		result = ADT_L0_WriteMem32(devID, channelRegOffset + thisSGCB + ADT_L1_A429_SGCB_TIMELOW, &data, 1);
		data = numVectors*2;  /* SGCB Vector Count */
		result = ADT_L0_WriteMem32(devID, channelRegOffset + thisSGCB + ADT_L1_A429_SGCB_VECCOUNT, &data, 1);

		/* Write the vector words */
		for (i=0; i<numWords-5; i++) {
			result = ADT_L0_WriteMem32(devID, channelRegOffset + thisSGCB + ADT_L1_A429_SGCB_VECTORS + i*4, &pVectors[i], 1);
		}

		/* Look for the last existing SGCB and link to this SGCB */
		if (result == ADT_SUCCESS) {
			/* Read the starting SGCB Address */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_A429_SGREG_1ST_SGCB_PTR, &lastSGCB, 1);

			/* HOW TO HANDLE 0xFFFFFFFF (SG already ran and stopped)?  FAIL OUT FOR NOW */
			if (lastSGCB == 0xFFFFFFFF) {
				result = ADT_FAILURE;
				return( result );
			}

			if (lastSGCB == 0x00000000) {  /* If this is the first SGCB to be defined . . . */
				/* Write the pointer to the new SGCB as the start SGCB address */
				result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_SGREG_1ST_SGCB_PTR, &thisSGCB, 1);
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
				result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_A429_SGREG_USER_COUNTER, &data, 1);
				data++;
				result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_SGREG_USER_COUNTER, &data, 1);
			}
		}
	}

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_A429_SG_Free
 *****************************************************************************/
/*! \brief Frees all SG CBs for the device
 *
 * This function frees all SG CBs for the device.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device.
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_SG_Free(ADT_L0_UINT32 devID) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 startSGCBptr, currSGCBptr, nextSGCBptr, data;
	ADT_L0_UINT32 numVectors, numWords;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Make sure SG is stopped */
	result = ADT_L1_A429_SG_Stop(devID);

	/* Get the starting SGCB pointer */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_A429_SGREG_1ST_SGCB_PTR, &startSGCBptr, 1);
	currSGCBptr = startSGCBptr;
	nextSGCBptr = 1;

	/* Iterate through SGCBs and free them until back to start ptr */
	while ((result == ADT_SUCCESS) && (nextSGCBptr != startSGCBptr) && (nextSGCBptr != 0)) {
		/* Get the next SGCB pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + currSGCBptr + ADT_L1_A429_SGCB_NEXTPTR, &nextSGCBptr, 1);

		/* Get the number of vectors */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + currSGCBptr + ADT_L1_A429_SGCB_VECCOUNT, &numVectors, 1);

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
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_SGREG_1ST_SGCB_PTR, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_SGREG_CUR_SGCB_PTR, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_SGREG_USER_COUNTER, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_SGREG_PE_COUNTER, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_SGREG_PE_OFFSET, &data, 1);

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_A429_SG_Start
 *****************************************************************************/
/*! \brief Starts Signal Generator operation for the device 
 *
 * This function starts Signal Generator operation for the device.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device.
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_SG_Start(ADT_L0_UINT32 devID) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 data;
	ADT_L0_UINT32 channel, channelRegOffset;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Clear the register for current SGCB */
	data = 0x00000000;
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_SGREG_CUR_SGCB_PTR, &data, 1);

	/* Read the PE CSR, set bit 4 to start SG */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_A429_PE_ROOT_CSR, &data, 1);
	if (result == ADT_SUCCESS) {
		data |= ADT_L1_A429_PECSR_SGON;  /* Set the SG ON bit */
		result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_PE_ROOT_CSR, &data, 1);
	}

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_A429_SG_Stop
 *****************************************************************************/
/*! \brief Stops Signal Generator operation for the device 
 *
 * This function stops Signal Generator operation for the device.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device.
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_SG_Stop(ADT_L0_UINT32 devID) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 data;
	ADT_L0_UINT32 channel, channelRegOffset;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Read the PE CSR, clear bit 4 to stop SG */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_A429_PE_ROOT_CSR, &data, 1);
	if (result == ADT_SUCCESS) {
		data &= ~ADT_L1_A429_PECSR_SGON;  /* Clear the SG ON bit */
		result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_PE_ROOT_CSR, &data, 1);
	}

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_A429_SG_IsRunning
 *****************************************************************************/
/*! \brief Determines if the SG is running. 
 *
 * This function determines if the SG is running.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param pIsRunning is a pointer to store the result, 0=NotRunning, 1=Running.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not an A429 device.
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_SG_IsRunning(ADT_L0_UINT32 devID, ADT_L0_UINT32 *pIsRunning) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 data;
	ADT_L0_UINT32 channel, channelRegOffset;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Read the SG CURRENT PTR, if zero then SG is stopped */
	data = 0;
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_A429_SGREG_CUR_SGCB_PTR, &data, 1);
	if (data == 0x00000000)
		*pIsRunning = 0;
	else
		*pIsRunning = 1;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_A429_SG_WordToVectors
 *****************************************************************************/
/*! \brief Adds an A429 word to a list of vectors. 
 *
 * This function adds an A429 word to a list of vectors.
 *
 * @param a429word is the 32-bit A429 word.
 * @param halfBitTime100ns is the half bit time with 100ns LSB.
 * @param pVectors is a pointer to the array of 32-bit words containing vectors.
 * @param sizeInWords is the max size (in words) of the vector array.
 * @param pNumVectors is a pointer to a UINT32 containing the current vector count (in 2-bit 20ns vectors).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_FAILURE - Completed with error - not enough words available in vectors array
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_SG_WordToVectors(ADT_L0_UINT32 a429word, 
										ADT_L0_UINT32 halfBitTime100ns,
										ADT_L0_UINT32 *pVectors, 
										ADT_L0_UINT32 sizeInWords, 
										ADT_L0_UINT32 *pNumVectors) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 mask, i;

	/* Add the data bits (32 bit-times), send bit 0 first (Label) */
	for (i=0; i<32; i++) {
		mask = 1; 
		mask = mask << i;
		if (a429word & mask) {
			result = ADT_L1_A429_SG_AddVectors(halfBitTime100ns, ADT_L1_A429_SG_VECTOR_HIGH, pVectors, sizeInWords, pNumVectors);
			result = ADT_L1_A429_SG_AddVectors(halfBitTime100ns, ADT_L1_A429_SG_VECTOR_GND0, pVectors, sizeInWords, pNumVectors);
		}
		else {
			result = ADT_L1_A429_SG_AddVectors(halfBitTime100ns, ADT_L1_A429_SG_VECTOR_LOW, pVectors, sizeInWords, pNumVectors);
			result = ADT_L1_A429_SG_AddVectors(halfBitTime100ns, ADT_L1_A429_SG_VECTOR_GND0, pVectors, sizeInWords, pNumVectors);
		}
	}

	return( result );
}




/******************************************************************************
  FUNCTION:		ADT_L1_A429_SG_AddVectors
 *****************************************************************************/
/*! \brief Adds a sequence of high vectors to a list of vectors. 
 *
 * This function adds a sequence of vectors to a list of vectors.
 *
 * @param numVec is the number of 2-bit vectors to add (1 vector = 100ns).
 * @param vector2bit is the 2-bit pattern to add for each vector (00=GND, 01=LOW, 10=HIGH, 11=GND).
 * @param pVectors is a pointer to the array of 32-bit words containing vectors.
 * @param sizeInWords is the max size (in words) of the vector array.
 * @param pNumVectors is a pointer to a UINT32 containing the current vector count (in bits).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_SG_AddVectors(ADT_L0_UINT32 numVec, ADT_L0_UINT32 vector2bit, 
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
