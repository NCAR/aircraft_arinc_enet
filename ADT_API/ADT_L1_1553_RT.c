/******************************************************************************
 * FILE:			ADT_L1_1553_RT.c
 *
 * Copyright (c) 2007-2018, Alta Data Technologies LLC (ADT), All Rights Reserved.
 * Use of this software is subject to the ADT Software License (latest
 * revision), US Government or local laws and the ADT Terms and Conditions 
 * of Sale (latest revision).
 *
 * DESCRIPTION:
 *	Source file for Layer 1 API.
 *	Contains 1553 RT functions.
 *
 *	TO DO:
 *	  In RT_Init, check for and handle ext RT - smooth transition.
 *
 *****************************************************************************/
/*! \file ADT_L1_1553_RT.c  
 *  \brief Source file containing 1553 RT functions 
 */
#include "ADT_L1.h"

#ifdef __cplusplus
extern "C" {
#endif


/******************************************************************************
  FUNCTION:		ADT_L1_1553_RT_Init
 *****************************************************************************/
/*! \brief Initializes data structures for the RT 
 *
 * This function initializes data structures for the RT.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param rtAddr is the RT address.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid RT address
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_RT_Init(ADT_L0_UINT32 devID, ADT_L0_UINT32 rtAddr) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 data, rtCbOffset, ftOffset, rtSaCbOffset, cdpOffset;
	ADT_L0_UINT32 channel, channelRegOffset, peCSR, i;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (rtAddr < 32) {
		/* Read the PE CSR - this tells us if the board is configured for SINGLE or MULTIPLE RT */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &peCSR, 1);

		/* Determine RT CB offset based on SINGLE or MULTIPLE RT configuration */
		if (peCSR & 0x00000008) {  /* Multiple RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS + (rtAddr * ADT_L1_1553_RT_CB_SIZE);
		}
		else {  /* Single RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS;
		}

		/* Clear the ROOT RT CSR (except external RT address and single RT address bits), 
		   set "do not increment on error" and "allow Tx inhibit" bits.
		   THIS TURNS OFF THE RT IF IT CAME UP LIVE (external RT address).
	    */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_RT_CSR, &data, 1);
		data &= 0xFFFF0000;
		data |= 0x00000006;
		result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_RT_CSR, &data, 1);

		/* Initialize the RT Control Block - Setup default structures */

		/* Filter Table Pointer - Allocate a Filter Table */
		result = ADT_L1_MemoryAlloc(devID, ADT_L1_1553_RT_FT_SIZE, &ftOffset);
		if (result != ADT_SUCCESS) return( result );
		result = ADT_L0_WriteMem32(devID, channelRegOffset + rtCbOffset + ADT_L1_1553_RT_CB_FTPTR, &ftOffset, 1);

		/* Default Control Block - Allocate a CB and point all FT entries to it */
		result = ADT_L1_MemoryAlloc(devID, ADT_L1_1553_RT_SA_CB_SIZE, &rtSaCbOffset);
		if (result != ADT_SUCCESS) return( result );
		/* Clear illegal WC/MC bits - all legal */
		data = 0x00000000;
		result = ADT_L0_WriteMem32(devID, channelRegOffset + rtSaCbOffset + ADT_L1_1553_RT_SA_CB_ILLEGALBITS, &data, 1);
		/* Default to one CDP */
		data = 0x00000001;
		result = ADT_L0_WriteMem32(devID, channelRegOffset + rtSaCbOffset + ADT_L1_1553_RT_SA_CB_APINUMCDP, &data, 1);
		
		/* Default CDP - Allocate a CDP and point the Default CB to it */
		result = ADT_L1_MemoryAlloc(devID, ADT_L1_1553_CDP_SIZE, &cdpOffset);
		if (result != ADT_SUCCESS) return( result );
		result = ADT_L0_WriteMem32(devID, channelRegOffset + rtSaCbOffset + ADT_L1_1553_RT_SA_CB_CDPPTR, &cdpOffset, 1);
		result = ADT_L0_WriteMem32(devID, channelRegOffset + rtSaCbOffset + ADT_L1_1553_RT_SA_CB_APISTARTCDP, &cdpOffset, 1);
		/* Set CDP NEXT PTR to point to itself */
		result = ADT_L0_WriteMem32(devID, channelRegOffset + cdpOffset, &cdpOffset, 1);
		/* Write the API info word to the CDP */
		data = rtAddr << ADT_L1_1553_CDP_RAPI_RT_RTADDR;
		data |= 0x000A0000;  /* Set MULTIPLE bits for TR and SA - default buffer */
		data |= ADT_L1_1553_CDP_RAPI_RT_ID;
		result = ADT_L0_WriteMem32(devID, channelRegOffset + cdpOffset + ADT_L1_1553_CDP_RESV_API, &data, 1);
		/* Point all FT entries to our default control block */
		for (i=0; i<(ADT_L1_1553_RT_FT_SIZE / 4); i++) {
			result = ADT_L0_WriteMem32(devID, channelRegOffset + ftOffset + (i*4), &rtSaCbOffset, 1);
		}

		/* RT CB CSR - 4us dead-bus response time, respond with status (not monitor) */
		data = 0x00028002;  
		result = ADT_L0_WriteMem32(devID, channelRegOffset + rtCbOffset + ADT_L1_1553_RT_CB_RTCSR, &data, 1);

		/* RT Status */
		data = rtAddr << 11;  
		result = ADT_L0_WriteMem32(devID, channelRegOffset + rtCbOffset + ADT_L1_1553_RT_CB_RTSTS, &data, 1);

		/* RT Last Command */
		data = 0x00000000;  
		result = ADT_L0_WriteMem32(devID, channelRegOffset + rtCbOffset + ADT_L1_1553_RT_CB_RTLCMD, &data, 1);
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_RT_Close
 *****************************************************************************/
/*! \brief Uninitializes data structures and frees memory for the RT 
 *
 * This function uninitializes data structures and frees memory for the RT.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param rtAddr is the RT address.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid RT address
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_RT_Close(ADT_L0_UINT32 devID, ADT_L0_UINT32 rtAddr) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 data, rtCbOffset, ftOffset, rtSaCbOffset, cdpOffset, nextCdpOffset, numCdp;
	ADT_L0_UINT32 cdpSizeInBytes;
	ADT_L0_UINT32 channel, channelRegOffset, peCSR, i, j, k;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (rtAddr < 32) {
		/* Read the PE CSR - this tells us if the board is configured for SINGLE or MULTIPLE RT */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &peCSR, 1);

		/* Determine RT CB offset based on SINGLE or MULTIPLE RT configuration */
		if (peCSR & 0x00000008) {  /* Multiple RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS + (rtAddr * ADT_L1_1553_RT_CB_SIZE);
		}
		else {  /* Single RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS;
		}

		/* Clear data structures and free all memory associated with the RT CB */
		data = 0;  
		result = ADT_L0_WriteMem32(devID, channelRegOffset + rtCbOffset + ADT_L1_1553_RT_CB_RTCSR, &data, 1);
		result = ADT_L0_WriteMem32(devID, channelRegOffset + rtCbOffset + ADT_L1_1553_RT_CB_RTSTS, &data, 1);
		result = ADT_L0_WriteMem32(devID, channelRegOffset + rtCbOffset + ADT_L1_1553_RT_CB_RTLCMD, &data, 1);

		/* Get the FT PTR */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + rtCbOffset + ADT_L1_1553_RT_CB_FTPTR, &ftOffset, 1);
		/* If we have a non-zero FT PTR then there is memory to free for this RT */
		if (ftOffset != 0x00000000)
		{
			for (i=0; i<128; i++) {  /* Iterate through all filter table entries */

				/* Get the RT SA CB PTR */
				result = ADT_L0_ReadMem32(devID, channelRegOffset + ftOffset + (i * 4), &rtSaCbOffset, 1);

				/* Get the number of CDPs from the CB */
				result = ADT_L0_ReadMem32(devID, channelRegOffset + rtSaCbOffset + ADT_L1_1553_RT_SA_CB_APINUMCDP, &numCdp, 1);

				/* If the number of CDPs is NON-ZERO then this has NOT already been freed.
				 * We have to be careful not to free the same memory more than once.
				 * Things like the default RT structures point multiple filter table
				 * entries to the same RT SA CB.  If we see a ZERO here, this tells us
				 * that the CB and associated CDPs have already been freed.
				 */
				if (numCdp != 0) {

					/* Get the first CDP pointer */
					result = ADT_L0_ReadMem32(devID, channelRegOffset + rtSaCbOffset + ADT_L1_1553_RT_SA_CB_APISTARTCDP, &cdpOffset, 1);

					for (j=0; j<numCdp; j++) {
						/* Get the next CDP pointer */
						result = ADT_L0_ReadMem32(devID, channelRegOffset + cdpOffset, &nextCdpOffset, 1);

						cdpSizeInBytes = ADT_L1_1553_CDP_SIZE;

						/* Clear and free the CDP */
						data = 0;
						for (k=0; k<(cdpSizeInBytes/4); k++) {
							result = ADT_L0_WriteMem32(devID, channelRegOffset + cdpOffset + (k * 4), &data, 1);
						}
						result = ADT_L1_MemoryFree(devID, cdpOffset, cdpSizeInBytes);

						/* Move to the next CDP */
						cdpOffset = nextCdpOffset;
					}

					/* Clear and Free the RT SA CB */
					data = 0;
					result = ADT_L0_WriteMem32(devID, channelRegOffset + rtSaCbOffset + (0 * 4), &data, 1);
					result = ADT_L0_WriteMem32(devID, channelRegOffset + rtSaCbOffset + (1 * 4), &data, 1);
					result = ADT_L0_WriteMem32(devID, channelRegOffset + rtSaCbOffset + (2 * 4), &data, 1);
					result = ADT_L0_WriteMem32(devID, channelRegOffset + rtSaCbOffset + (3 * 4), &data, 1);
					result = ADT_L1_MemoryFree(devID, rtSaCbOffset, ADT_L1_1553_RT_SA_CB_SIZE);

					/* Clear the FT entry */
					data = 0;
					result = ADT_L0_WriteMem32(devID, channelRegOffset + ftOffset + (i * 4), &data, 1);
				}
			}

			/* Free the filter table */
			result = ADT_L1_MemoryFree(devID, ftOffset, ADT_L1_1553_RT_FT_SIZE);

			/* Clear the RT FT PTR */
			data = 0;
			result = ADT_L0_WriteMem32(devID, channelRegOffset + rtCbOffset + ADT_L1_1553_RT_CB_FTPTR, &data, 1);
		}
		else result = ADT_ERR_BAD_INPUT;

	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_RT_Start
 *****************************************************************************/
/*! \brief Starts the RT function of the channel 
 *
 * This function starts the RT function of the channel.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_RT_Start(ADT_L0_UINT32 devID) {
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

	/* Read the RT CSR */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_RT_CSR, &data, 1);

	/* Set bit 0 to start RT operation */
	data |= 0x00000001;
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_RT_CSR, &data, 1);

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_RT_Stop
 *****************************************************************************/
/*! \brief Stops the RT function of the channel 
 *
 * This function stops the RT function of the channel.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_RT_Stop(ADT_L0_UINT32 devID) {
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

	/* Read the RT CSR */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_RT_CSR, &data, 1);

	/* Clear bit 0 to stop RT operation */
	data &= 0xFFFFFFFE;
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_RT_CSR, &data, 1);

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_RT_Enable
 *****************************************************************************/
/*! \brief Enables a specific RT 
 *
 * This function enables a specific RT.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param rtAddr is the RT address.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid RT address
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_RT_Enable(ADT_L0_UINT32 devID, ADT_L0_UINT32 rtAddr) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 data, rtCbOffset;
	ADT_L0_UINT32 channel, channelRegOffset, peCSR, sRtAddr;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (rtAddr < 32) {
		/* Read the PE CSR - this tells us if the board is configured for SINGLE or MULTIPLE RT */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &peCSR, 1);

		/* Determine RT CB offset based on SINGLE or MULTIPLE RT configuration */
		if (peCSR & 0x00000008) {  /* Multiple RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS + (rtAddr * ADT_L1_1553_RT_CB_SIZE);
		}
		else {  /* Single RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS;

			/* Read the RT CSR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_RT_CSR, &data, 1);

			/* Get the SRT address (bits 16-20) */
			sRtAddr = (data & 0x001F0000) >> 16;

			/* Verify that selected RT address matches the Single RT Address */
			if (rtAddr != sRtAddr) result = ADT_ERR_BAD_INPUT;
		}

		if (result == ADT_SUCCESS) {
			/* Read the RT CB CSR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + rtCbOffset + ADT_L1_1553_RT_CB_RTCSR, &data, 1);

			/* Set bit 0 to enable the RT */
			data |= 0x00000001;
			result = ADT_L0_WriteMem32(devID, channelRegOffset + rtCbOffset + ADT_L1_1553_RT_CB_RTCSR, &data, 1);
		}
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_RT_Disable
 *****************************************************************************/
/*! \brief Disables a specific RT 
 *
 * This function disables a specific RT.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param rtAddr is the RT address.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid RT address
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_RT_Disable(ADT_L0_UINT32 devID, ADT_L0_UINT32 rtAddr) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 data, rtCbOffset;
	ADT_L0_UINT32 channel, channelRegOffset, peCSR, sRtAddr;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (rtAddr < 32) {
		/* Read the PE CSR - this tells us if the board is configured for SINGLE or MULTIPLE RT */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &peCSR, 1);

		/* Determine RT CB offset based on SINGLE or MULTIPLE RT configuration */
		if (peCSR & 0x00000008) {  /* Multiple RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS + (rtAddr * ADT_L1_1553_RT_CB_SIZE);
		}
		else {  /* Single RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS;

			/* Read the RT CSR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_RT_CSR, &data, 1);

			/* Get the SRT address (bits 16-20) */
			sRtAddr = (data & 0x001F0000) >> 16;

			/* Verify that selected RT address matches the Single RT Address */
			if (rtAddr != sRtAddr) result = ADT_ERR_BAD_INPUT;
		}

		if (result == ADT_SUCCESS) {
			/* Read the RT CB CSR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + rtCbOffset + ADT_L1_1553_RT_CB_RTCSR, &data, 1);

			/* Clear bit 0 to disable the RT */
			data &= 0xFFFFFFFE;
			result = ADT_L0_WriteMem32(devID, channelRegOffset + rtCbOffset + ADT_L1_1553_RT_CB_RTCSR, &data, 1);

			/* Delay 1ms to allow firmware to finish processing any currently executing message for the RT */
			ADT_L1_msSleep(1);
		}
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_RT_Monitor
 *****************************************************************************/
/*! \brief Configures a RT to either MONITOR or RESPOND 
 *
 * This function configures a RT to either MONITOR or RESPOND.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param rtAddr is the RT address.
 * @param isMonitor set to 1 for MONITOR or 0 for RESPOND.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid RT address
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_RT_Monitor(ADT_L0_UINT32 devID, ADT_L0_UINT32 rtAddr, ADT_L0_UINT32 isMonitor) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 data, rtCbOffset;
	ADT_L0_UINT32 channel, channelRegOffset, peCSR, sRtAddr;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (rtAddr < 32) {
		/* Read the PE CSR - this tells us if the board is configured for SINGLE or MULTIPLE RT */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &peCSR, 1);

		/* Determine RT CB offset based on SINGLE or MULTIPLE RT configuration */
		if (peCSR & 0x00000008) {  /* Multiple RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS + (rtAddr * ADT_L1_1553_RT_CB_SIZE);
		}
		else {  /* Single RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS;

			/* Read the RT CSR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_RT_CSR, &data, 1);

			/* Get the SRT address (bits 16-20) */
			sRtAddr = (data & 0x001F0000) >> 16;

			/* Verify that selected RT address matches the Single RT Address */
			if (rtAddr != sRtAddr) result = ADT_ERR_BAD_INPUT;
		}

		if (result == ADT_SUCCESS) {
			/* Read the RT CB CSR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + rtCbOffset + ADT_L1_1553_RT_CB_RTCSR, &data, 1);

			if (isMonitor) {
				/* Clear bit 1 to disable response */
				data &= 0xFFFFFFFD;
			}
			else {
				/* Set bit 1 to enable response */
				data |= 0x00000002;
			}

			result = ADT_L0_WriteMem32(devID, channelRegOffset + rtCbOffset + ADT_L1_1553_RT_CB_RTCSR, &data, 1);
		}
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_RT_SetRespTime
 *****************************************************************************/
/*! \brief Sets the Status Response time for the specified RT 
 *
 * This function sets the Status Response time (100ns LSB) for the specified RT.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param rtAddr is the RT address.
 * @param respTime100ns is the response time (100ns LSB, max value allowed 4095 = 409.5us).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid RT address or resp time
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_RT_SetRespTime(ADT_L0_UINT32 devID, ADT_L0_UINT32 rtAddr, ADT_L0_UINT32 respTime100ns) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 data, rtCbOffset, temp;
	ADT_L0_UINT32 channel, channelRegOffset, peCSR, sRtAddr;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if ((rtAddr < 32) || (respTime100ns > 4095)) {
		/* Read the PE CSR - this tells us if the board is configured for SINGLE or MULTIPLE RT */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &peCSR, 1);

		/* Determine RT CB offset based on SINGLE or MULTIPLE RT configuration */
		if (peCSR & 0x00000008) {  /* Multiple RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS + (rtAddr * ADT_L1_1553_RT_CB_SIZE);
		}
		else {  /* Single RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS;

			/* Read the RT CSR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_RT_CSR, &data, 1);

			/* Get the SRT address (bits 16-20) */
			sRtAddr = (data & 0x001F0000) >> 16;

			/* Verify that selected RT address matches the Single RT Address */
			if (rtAddr != sRtAddr) result = ADT_ERR_BAD_INPUT;
		}

		if (result == ADT_SUCCESS) {
			/* Read the RT CB CSR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + rtCbOffset + ADT_L1_1553_RT_CB_RTCSR, &data, 1);

			/* Clear bits 12-23 (response time) */
			data &= 0xFF000FFF;

			/* OR in the new response time */
			temp = respTime100ns << 12;
			data |= temp;

			/* Write the RT CB CSR */
			result = ADT_L0_WriteMem32(devID, channelRegOffset + rtCbOffset + ADT_L1_1553_RT_CB_RTCSR, &data, 1);
		}
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_RT_GetRespTime
 *****************************************************************************/
/*! \brief Gets the Status Response time for the specified RT 
 *
 * This function gets the Status Response time (100ns LSB) for the specified RT.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param rtAddr is the RT address.
 * @param pRespTime100ns is the pointer to store the response time (100ns LSB).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid RT address
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_RT_GetRespTime(ADT_L0_UINT32 devID, ADT_L0_UINT32 rtAddr, ADT_L0_UINT32 * pRespTime100ns) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 data, rtCbOffset;
	ADT_L0_UINT32 channel, channelRegOffset, peCSR, sRtAddr;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (rtAddr < 32) {
		/* Read the PE CSR - this tells us if the board is configured for SINGLE or MULTIPLE RT */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &peCSR, 1);

		/* Determine RT CB offset based on SINGLE or MULTIPLE RT configuration */
		if (peCSR & 0x00000008) {  /* Multiple RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS + (rtAddr * ADT_L1_1553_RT_CB_SIZE);
		}
		else {  /* Single RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS;

			/* Read the RT CSR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_RT_CSR, &data, 1);

			/* Get the SRT address (bits 16-20) */
			sRtAddr = (data & 0x001F0000) >> 16;

			/* Verify that selected RT address matches the Single RT Address */
			if (rtAddr != sRtAddr) result = ADT_ERR_BAD_INPUT;
		}

		if (result == ADT_SUCCESS) {
			/* Read the RT CB CSR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + rtCbOffset + ADT_L1_1553_RT_CB_RTCSR, &data, 1);

			/* Get bits 12-23 (response time) */
			*pRespTime100ns = (data & 0x00FFF000) >> 12;
		}
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_RT_InjStsWordError
 *****************************************************************************/
/*! \brief Configures a RT for errors on the status word 
 *
 * This function configures a RT for errors on the status word.
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
 * @param rtAddr is the RT address.
 * @param syncErr set to 1 for sync error or 0 for no sync error.
 * @param manchesterErr set to 1 for manchester error or 0 for no manchester error.
 * @param parityErr set to 1 for parity error or 0 for no parity error.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid RT address
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_RT_InjStsWordError(ADT_L0_UINT32 devID, ADT_L0_UINT32 rtAddr, 
											 ADT_L0_UINT32 syncErr, ADT_L0_UINT32 manchesterErr, ADT_L0_UINT32 parityErr) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 data, rtCbOffset;
	ADT_L0_UINT32 channel, channelRegOffset, peCSR, sRtAddr;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (rtAddr < 32) {
		/* Read the PE CSR - this tells us if the board is configured for SINGLE or MULTIPLE RT */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &peCSR, 1);

		/* Determine RT CB offset based on SINGLE or MULTIPLE RT configuration */
		if (peCSR & 0x00000008) {  /* Multiple RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS + (rtAddr * ADT_L1_1553_RT_CB_SIZE);
		}
		else {  /* Single RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS;

			/* Read the RT CSR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_RT_CSR, &data, 1);

			/* Get the SRT address (bits 16-20) */
			sRtAddr = (data & 0x001F0000) >> 16;

			/* Verify that selected RT address matches the Single RT Address */
			if (rtAddr != sRtAddr) result = ADT_ERR_BAD_INPUT;
		}

		if (result == ADT_SUCCESS) {
			/* Read the RT CB CSR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + rtCbOffset + ADT_L1_1553_RT_CB_RTCSR, &data, 1);

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

			result = ADT_L0_WriteMem32(devID, channelRegOffset + rtCbOffset + ADT_L1_1553_RT_CB_RTCSR, &data, 1);
		}
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_RT_ReadStsWordError
 *****************************************************************************/
/*! \brief Reads the settings for a RT for errors on the status word 
 *
 * This function reads the settings for a RT for errors on the status word.
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
 * @param rtAddr is the RT address.
 * @param pSyncErr is ptr to store sync error (1 for sync error or 0 for no sync error).
 * @param pManchesterErr is ptr to store manch error (1 for manch error or 0 for no manch error).
 * @param pParityErr ptr to store parity error (1 for parity error or 0 for no parity error).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid RT address
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_RT_ReadStsWordError(ADT_L0_UINT32 devID, ADT_L0_UINT32 rtAddr, 
											 ADT_L0_UINT32 *pSyncErr, ADT_L0_UINT32 *pManchesterErr, 
											 ADT_L0_UINT32 *pParityErr) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 data, rtCbOffset;
	ADT_L0_UINT32 channel, channelRegOffset, peCSR, sRtAddr;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (rtAddr < 32) {
		/* Read the PE CSR - this tells us if the board is configured for SINGLE or MULTIPLE RT */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &peCSR, 1);

		/* Determine RT CB offset based on SINGLE or MULTIPLE RT configuration */
		if (peCSR & 0x00000008) {  /* Multiple RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS + (rtAddr * ADT_L1_1553_RT_CB_SIZE);
		}
		else {  /* Single RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS;

			/* Read the RT CSR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_RT_CSR, &data, 1);

			/* Get the SRT address (bits 16-20) */
			sRtAddr = (data & 0x001F0000) >> 16;

			/* Verify that selected RT address matches the Single RT Address */
			if (rtAddr != sRtAddr) result = ADT_ERR_BAD_INPUT;
		}

		if (result == ADT_SUCCESS) {
			/* Read the RT CB CSR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + rtCbOffset + ADT_L1_1553_RT_CB_RTCSR, &data, 1);

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
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_RT_SetOptions
 *****************************************************************************/
/*! \brief Configures a RT CB for transmitter inhibits and mode code options 
 *
 * This function configures a RT CB for transmitter inhibits and mode code options.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param rtAddr is the RT address.
 * @param allowDBC set to 1 to ALLOW DYNAMIC BUS CONTROL or 0 to IGNORE DYNAMIC BUS CONTROL.
 * @param txInh_A is set to 1 to INHIBIT BUS A or 0 to ENABLE BUS A.
 * @param txInh_B is set to 1 to INHIBIT BUS B or 0 to ENABLE BUS B.
 * @param clrSRonTxVectorWd is set to 1 to CLEAR SR BIT ON TX VECTOR WORD or 0 to NOT CHANGE SR ON TX VECTOR WORD.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid RT address
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_RT_SetOptions(ADT_L0_UINT32 devID, ADT_L0_UINT32 rtAddr, ADT_L0_UINT32 allowDBC, 
										ADT_L0_UINT32 txInh_A, ADT_L0_UINT32 txInh_B, ADT_L0_UINT32 clrSRonTxVectorWd) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 data, rtCbOffset;
	ADT_L0_UINT32 channel, channelRegOffset, peCSR, sRtAddr;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (rtAddr < 32) {
		/* Read the PE CSR - this tells us if the board is configured for SINGLE or MULTIPLE RT */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &peCSR, 1);

		/* Determine RT CB offset based on SINGLE or MULTIPLE RT configuration */
		if (peCSR & 0x00000008) {  /* Multiple RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS + (rtAddr * ADT_L1_1553_RT_CB_SIZE);
		}
		else {  /* Single RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS;

			/* Read the RT CSR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_RT_CSR, &data, 1);

			/* Get the SRT address (bits 16-20) */
			sRtAddr = (data & 0x001F0000) >> 16;

			/* Verify that selected RT address matches the Single RT Address */
			if (rtAddr != sRtAddr) result = ADT_ERR_BAD_INPUT;
		}

		if (result == ADT_SUCCESS) {
			/* Read the RT CB CSR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + rtCbOffset + ADT_L1_1553_RT_CB_RTCSR, &data, 1);

			if (allowDBC) {
				/* Set bit 9 to enable DBC */
				data |= 0x00000200;
			}
			else {
				/* Clear bit 9 to disable DBC */
				data &= 0xFFFFFDFF;
			}

			if (txInh_B) {
				/* Set bit 11 to inhibit Bus B */
				data |= 0x00000800;
			}
			else {
				/* Clear bit 8 to enable Bus B */
				data &= 0xFFFFF7FF;
			}

			if (txInh_A) {
				/* Set bit 10 to inhibit Bus A */
				data |= 0x00000400;
			}
			else {
				/* Clear bit 10 to enable Bus A */
				data &= 0xFFFFFBFF;
			}

			if (clrSRonTxVectorWd) {
				/* Set bit 4 to enable clear SR on TX VECTOR WORD */
				data |= 0x00000010;
			}
			else {
				/* Clear bit 4 to disable clear SR on TX VECTOR WORD */
				data &= 0xFFFFFFEF;
			}

			result = ADT_L0_WriteMem32(devID, channelRegOffset + rtCbOffset + ADT_L1_1553_RT_CB_RTCSR, &data, 1);
		}
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_RT_GetOptions
 *****************************************************************************/
/*! \brief Gets the settings for a RT CB for transmitter inhibits and mode code options 
 *
 * This function gets the settings for a RT CB for transmitter inhibits and mode code options.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param rtAddr is the RT address.
 * @param pAllowDBC is the pointer to store DBC setting (1 to ALLOW DYNAMIC BUS CONTROL or 0 to IGNORE DYNAMIC BUS CONTROL).
 * @param pTxInh_A is the pointer to store BUS A INHIBIT setting (1 to INHIBIT BUS A or 0 to ENABLE BUS A).
 * @param pTxInh_B is the pointer to store BUS B INHIBIT setting (1 to INHIBIT BUS B or 0 to ENABLE BUS B).
 * @param pClrSRonTxVectorWd is the pointer to store CLEAR SR setting (1 to CLEAR SR BIT ON TX VECTOR WORD or 0 to NOT CHANGE SR ON TX VECTOR WORD).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid RT address
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_RT_GetOptions(ADT_L0_UINT32 devID, ADT_L0_UINT32 rtAddr, ADT_L0_UINT32 *pAllowDBC, 
										ADT_L0_UINT32 *pTxInh_A, ADT_L0_UINT32 *pTxInh_B, ADT_L0_UINT32 *pClrSRonTxVectorWd) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 data, rtCbOffset;
	ADT_L0_UINT32 channel, channelRegOffset, peCSR, sRtAddr;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (rtAddr < 32) {
		/* Read the PE CSR - this tells us if the board is configured for SINGLE or MULTIPLE RT */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &peCSR, 1);

		/* Determine RT CB offset based on SINGLE or MULTIPLE RT configuration */
		if (peCSR & 0x00000008) {  /* Multiple RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS + (rtAddr * ADT_L1_1553_RT_CB_SIZE);
		}
		else {  /* Single RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS;

			/* Read the RT CSR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_RT_CSR, &data, 1);

			/* Get the SRT address (bits 16-20) */
			sRtAddr = (data & 0x001F0000) >> 16;

			/* Verify that selected RT address matches the Single RT Address */
			if (rtAddr != sRtAddr) result = ADT_ERR_BAD_INPUT;
		}

		if (result == ADT_SUCCESS) {
			/* Read the RT CB CSR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + rtCbOffset + ADT_L1_1553_RT_CB_RTCSR, &data, 1);

			/* Extract the relevant settings */
			*pAllowDBC = (data & 0x00000200) >> 9;
/*			*pTxInh_B  = (data & 0x00000100) >> 8;
			*pTxInh_A  = (data & 0x00000080) >> 7;*/
			*pTxInh_B  = (data & 0x00000800) >> 11;
			*pTxInh_A  = (data & 0x00000400) >> 10;
			*pClrSRonTxVectorWd = (data & 0x00000010) >> 4;
		}

	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_RT_SetSingleRTAddr
 *****************************************************************************/
/*! \brief Sets the single RT address 
 *
 * This function sets the single RT address for the device.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param rtAddr is the RT address (0-31).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid RT address
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_RT_SetSingleRTAddr(ADT_L0_UINT32 devID, ADT_L0_UINT32 rtAddr) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 data, temp;
	ADT_L0_UINT32 channel, channelRegOffset;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (rtAddr < 32) {
		temp = rtAddr << 16;

		/* Read the RT CSR */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_RT_CSR, &data, 1);

		/* Set the SRT address (bits 16-20) */
		data &= 0xFFE0FFFF;
		data |= temp;
		result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_RT_CSR, &data, 1);

	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_RT_GetSingleRTAddr
 *****************************************************************************/
/*! \brief Gets the single RT address 
 *
 * This function gets the single RT address for the device.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param pRtAddr is the pointer to store the single RT address.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_RT_GetSingleRTAddr(ADT_L0_UINT32 devID, ADT_L0_UINT32 *pRtAddr) {
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

	/* Read the RT CSR */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_RT_CSR, &data, 1);

	/* Get the SRT address (bits 16-20) */
	pRtAddr[0] = (data & 0x001F0000) >> 16;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_RT_GetExternalRTAddr
 *****************************************************************************/
/*! \brief Gets the External RT address 
 *
 * This function gets the external RT address for the device.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param pRtAddr is the pointer to store the external RT address.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid parity on external RT address
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_RT_GetExternalRTAddr(ADT_L0_UINT32 devID, ADT_L0_UINT32 *pRtAddr) {
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

	/* Read the RT CSR */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_RT_CSR, &data, 1);

	/* Get the external RT address (bits 24-28) */
	pRtAddr[0] = (data & 0x1F000000) >> 24;

	/* Check external RT parity (bit 31) */
	if (data & 0x80000000)
		result = ADT_ERR_BAD_INPUT;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_RT_GetLastCmd
 *****************************************************************************/
/*! \brief Gets the last command seen by the RT 
 *
 * This function gets the last command seen by the RT.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param rtAddr is the RT address.
 * @param pLastCmd is the pointer to store the last command word.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid RT address
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_RT_GetLastCmd(ADT_L0_UINT32 devID, ADT_L0_UINT32 rtAddr, ADT_L0_UINT32 *pLastCmd) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 data, rtCbOffset;
	ADT_L0_UINT32 channel, channelRegOffset, peCSR, sRtAddr;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (rtAddr < 32) {
		/* Read the PE CSR - this tells us if the board is configured for SINGLE or MULTIPLE RT */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &peCSR, 1);

		/* Determine RT CB offset based on SINGLE or MULTIPLE RT configuration */
		if (peCSR & 0x00000008) {  /* Multiple RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS + (rtAddr * ADT_L1_1553_RT_CB_SIZE);
		}
		else {  /* Single RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS;

			/* Read the RT CSR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_RT_CSR, &data, 1);

			/* Get the SRT address (bits 16-20) */
			sRtAddr = (data & 0x001F0000) >> 16;

			/* Verify that selected RT address matches the Single RT Address */
			if (rtAddr != sRtAddr) result = ADT_ERR_BAD_INPUT;
		}

		if (result == ADT_SUCCESS) {
			/* Read the Last Command from the RT control block for the RT address */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + rtCbOffset + ADT_L1_1553_RT_CB_RTLCMD, &data, 1);

			pLastCmd[0] = data;
		}
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_RT_StatusWrite
 *****************************************************************************/
/*! \brief Writes the Status word for the RT 
 *
 * This function writes the Status word for the RT.  Note that you CANNOT 
 * change the RT Address (bits 11-15) or the Message Error (ME) bit (bit 10).
 * The ME bit is strictly defined by the MIL-STD-1553 specification and is 
 * controlled by the Alta Protocol Engine.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param rtAddr is the RT address.
 * @param stsWord is the status word.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid RT address
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_RT_StatusWrite(ADT_L0_UINT32 devID, ADT_L0_UINT32 rtAddr, ADT_L0_UINT32 stsWord) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 data, temp, tempSts, rtCbOffset;
	ADT_L0_UINT32 channel, channelRegOffset, peCSR, sRtAddr;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (rtAddr < 32) {
		/* Read the PE CSR - this tells us if the board is configured for SINGLE or MULTIPLE RT */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &peCSR, 1);

		/* Determine RT CB offset based on SINGLE or MULTIPLE RT configuration */
		if (peCSR & 0x00000008) {  /* Multiple RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS + (rtAddr * ADT_L1_1553_RT_CB_SIZE);
		}
		else {  /* Single RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS;

			/* Read the RT CSR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_RT_CSR, &data, 1);

			/* Get the SRT address (bits 16-20) */
			sRtAddr = (data & 0x001F0000) >> 16;

			/* Verify that selected RT address matches the Single RT Address */
			if (rtAddr != sRtAddr) result = ADT_ERR_BAD_INPUT;
		}

		if (result == ADT_SUCCESS) {
			/* Mask out bits that user is not allowed to change (RT addr, ME) */
			tempSts = stsWord & 0x000003FF;
			tempSts = tempSts << 16;

			/* Read the Status from the RT control block for the RT address */
			temp = rtCbOffset + ADT_L1_1553_RT_CB_RTSTS;
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &data, 1);

			/* Replace the user Status OR bits (upper 16-bits) */
			data &= 0x0000FFFF;
			data |= tempSts;
			result = ADT_L0_WriteMem32(devID, channelRegOffset + temp, &data, 1);
		}
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_RT_StatusRead
 *****************************************************************************/
/*! \brief Reads the Status word for the RT 
 *
 * This function reads the Status word for the RT.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param rtAddr is the RT address.
 * @param pStsWord is the pointer to store the status word.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid RT address
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_RT_StatusRead(ADT_L0_UINT32 devID, ADT_L0_UINT32 rtAddr, ADT_L0_UINT32 *pStsWord) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 data, temp, tempSts, rtCbOffset;
	ADT_L0_UINT32 channel, channelRegOffset, peCSR, sRtAddr;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if (rtAddr < 32) {
		/* Read the PE CSR - this tells us if the board is configured for SINGLE or MULTIPLE RT */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &peCSR, 1);

		/* Determine RT CB offset based on SINGLE or MULTIPLE RT configuration */
		if (peCSR & 0x00000008) {  /* Multiple RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS + (rtAddr * ADT_L1_1553_RT_CB_SIZE);
		}
		else {  /* Single RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS;

			/* Read the RT CSR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_RT_CSR, &data, 1);

			/* Get the SRT address (bits 16-20) */
			sRtAddr = (data & 0x001F0000) >> 16;

			/* Verify that selected RT address matches the Single RT Address */
			if (rtAddr != sRtAddr) result = ADT_ERR_BAD_INPUT;
		}

		if (result == ADT_SUCCESS) {
			/* Read the Status from the RT control block for the RT address */
			temp = rtCbOffset + ADT_L1_1553_RT_CB_RTSTS;
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &data, 1);

			/* Combine the User Status OR bits with the Firmware Status word */
			tempSts = data & 0x0000FFFF;
			data = data >> 16;
			tempSts |= data;
			pStsWord[0] = tempSts;
		}
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_RT_SA_LegalizationWrite
 *****************************************************************************/
/*! \brief Writes the legalization settings for the RT/SA 
 *
 * This function writes the legalization settings for the RT/SA.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param rtAddr is the RT address.
 * @param tr is transmit(1) or receive(0).
 * @param subAddr is the subaddress.
 * @param illegalBits contains one bit for each word count, 0=legal, 1=illegal.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid RT address, T/R, or subaddress
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_RT_SA_LegalizationWrite(ADT_L0_UINT32 devID, ADT_L0_UINT32 rtAddr, ADT_L0_UINT32 tr, ADT_L0_UINT32 subAddr, ADT_L0_UINT32 illegalBits) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, peCSR;
	ADT_L0_UINT32 temp, rtCbOffset, rtFtPtr, rtSaCbPtr, data, sRtAddr;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if ((rtAddr < 32) && (tr < 2) && (subAddr < 32)) {
		/* Read the PE CSR - this tells us if the board is configured for SINGLE or MULTIPLE RT */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &peCSR, 1);

		/* Determine RT CB offset based on SINGLE or MULTIPLE RT configuration */
		if (peCSR & 0x00000008) {  /* Multiple RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS + (rtAddr * ADT_L1_1553_RT_CB_SIZE);
		}
		else {  /* Single RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS;

			/* Read the RT CSR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_RT_CSR, &data, 1);

			/* Get the SRT address (bits 16-20) */
			sRtAddr = (data & 0x001F0000) >> 16;

			/* Verify that selected RT address matches the Single RT Address */
			if (rtAddr != sRtAddr) result = ADT_ERR_BAD_INPUT;
		}

		if (result == ADT_SUCCESS) {
			/* Get the RT Filter Table Pointer */
			temp = rtCbOffset + ADT_L1_1553_RT_CB_FTPTR;
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &rtFtPtr, 1);

			/* Get the RT SA Control Block Pointer */
			temp = rtFtPtr + (tr * 32 * 4) + (subAddr * 4);
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &rtSaCbPtr, 1);

			/* Write the legalization word */
			temp = rtSaCbPtr + ADT_L1_1553_RT_SA_CB_ILLEGALBITS;
			result = ADT_L0_WriteMem32(devID, channelRegOffset + temp, &illegalBits, 1);
		}

	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_1553_RT_SA_LegalizationRead
 *****************************************************************************/
/*! \brief Reads the legalization settings for the RT/SA 
 *
 * This function reads the legalization settings for the RT/SA.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param rtAddr is the RT address.
 * @param tr is transmit(1) or receive(0).
 * @param subAddr is the subaddress.
 * @param pIllegalBits is the pointer to store the value read.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid RT address, T/R, or subaddress
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_RT_SA_LegalizationRead(ADT_L0_UINT32 devID, ADT_L0_UINT32 rtAddr, ADT_L0_UINT32 tr, ADT_L0_UINT32 subAddr, ADT_L0_UINT32 *pIllegalBits) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, peCSR;
	ADT_L0_UINT32 temp, rtCbOffset, rtFtPtr, rtSaCbPtr, data, sRtAddr;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if ((rtAddr < 32) && (tr < 2) && (subAddr < 32)) {
		/* Read the PE CSR - this tells us if the board is configured for SINGLE or MULTIPLE RT */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &peCSR, 1);

		/* Determine RT CB offset based on SINGLE or MULTIPLE RT configuration */
		if (peCSR & 0x00000008) {  /* Multiple RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS + (rtAddr * ADT_L1_1553_RT_CB_SIZE);
		}
		else {  /* Single RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS;

			/* Read the RT CSR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_RT_CSR, &data, 1);

			/* Get the SRT address (bits 16-20) */
			sRtAddr = (data & 0x001F0000) >> 16;

			/* Verify that selected RT address matches the Single RT Address */
			if (rtAddr != sRtAddr) result = ADT_ERR_BAD_INPUT;
		}

		if (result == ADT_SUCCESS) {
			/* Get the RT Filter Table Pointer */
			temp = rtCbOffset + ADT_L1_1553_RT_CB_FTPTR;
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &rtFtPtr, 1);

			/* Get the RT SA Control Block Pointer */
			temp = rtFtPtr + (tr * 32 * 4) + (subAddr * 4);
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &rtSaCbPtr, 1);

			/* Read the legalization word */
			temp = rtSaCbPtr + ADT_L1_1553_RT_SA_CB_ILLEGALBITS;
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, pIllegalBits, 1);
		}

	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_1553_RT_MC_LegalizationWrite
 *****************************************************************************/
/*! \brief Writes the legalization settings for the RT MODE CODE 
 *
 * This function writes the legalization settings for the RT MODE CODE.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param rtAddr is the RT address.
 * @param tr is transmit(1) or receive(0).
 * @param modeCode is the mode code.
 * @param illegalBits contains one bit for each word count, 0=legal, 1=illegal.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid RT address or subaddress
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_RT_MC_LegalizationWrite(ADT_L0_UINT32 devID, ADT_L0_UINT32 rtAddr, ADT_L0_UINT32 tr, ADT_L0_UINT32 modeCode, ADT_L0_UINT32 illegalBits) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, peCSR;
	ADT_L0_UINT32 temp, rtCbOffset, rtFtPtr, rtSaCbPtr, data, sRtAddr;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if ((rtAddr < 32) && (tr < 2) && (modeCode < 32)) {
		/* Read the PE CSR - this tells us if the board is configured for SINGLE or MULTIPLE RT */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &peCSR, 1);

		/* Determine RT CB offset based on SINGLE or MULTIPLE RT configuration */
		if (peCSR & 0x00000008) {  /* Multiple RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS + (rtAddr * ADT_L1_1553_RT_CB_SIZE);
		}
		else {  /* Single RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS;

			/* Read the RT CSR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_RT_CSR, &data, 1);

			/* Get the SRT address (bits 16-20) */
			sRtAddr = (data & 0x001F0000) >> 16;

			/* Verify that selected RT address matches the Single RT Address */
			if (rtAddr != sRtAddr) result = ADT_ERR_BAD_INPUT;
		}

		if (result == ADT_SUCCESS) {
			/* Get the RT Filter Table Pointer */
			temp = rtCbOffset + ADT_L1_1553_RT_CB_FTPTR;
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &rtFtPtr, 1);

			/* Get the RT MC Control Block Pointer */
			temp = rtFtPtr + (64 * 4) + (tr * 32 * 4) + (modeCode * 4);
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &rtSaCbPtr, 1);

			/* Write the legalization word */
			temp = rtSaCbPtr + ADT_L1_1553_RT_SA_CB_ILLEGALBITS;
			result = ADT_L0_WriteMem32(devID, channelRegOffset + temp, &illegalBits, 1);
		}

	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_1553_RT_MC_LegalizationRead
 *****************************************************************************/
/*! \brief Reads the legalization settings for the RT MODE CODE 
 *
 * This function reads the legalization settings for the RT MODE CODE.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param rtAddr is the RT address.
 * @param tr is transmit(1) or receive(0).
 * @param modeCode is the mode code.
 * @param pIllegalBits is the pointer to store the value read.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid RT address or subaddress
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_RT_MC_LegalizationRead(ADT_L0_UINT32 devID, ADT_L0_UINT32 rtAddr, ADT_L0_UINT32 tr, ADT_L0_UINT32 modeCode, ADT_L0_UINT32 *pIllegalBits) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, peCSR;
	ADT_L0_UINT32 temp, rtCbOffset, rtFtPtr, rtSaCbPtr, data, sRtAddr;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if ((rtAddr < 32) && (tr < 2) && (modeCode < 32)) {
		/* Read the PE CSR - this tells us if the board is configured for SINGLE or MULTIPLE RT */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &peCSR, 1);

		/* Determine RT CB offset based on SINGLE or MULTIPLE RT configuration */
		if (peCSR & 0x00000008) {  /* Multiple RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS + (rtAddr * ADT_L1_1553_RT_CB_SIZE);
		}
		else {  /* Single RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS;

			/* Read the RT CSR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_RT_CSR, &data, 1);

			/* Get the SRT address (bits 16-20) */
			sRtAddr = (data & 0x001F0000) >> 16;

			/* Verify that selected RT address matches the Single RT Address */
			if (rtAddr != sRtAddr) result = ADT_ERR_BAD_INPUT;
		}

		if (result == ADT_SUCCESS) {
			/* Get the RT Filter Table Pointer */
			temp = rtCbOffset + ADT_L1_1553_RT_CB_FTPTR;
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &rtFtPtr, 1);

			/* Get the RT MC Control Block Pointer */
			temp = rtFtPtr + (64 * 4) + (tr * 32 * 4) + (modeCode * 4);
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &rtSaCbPtr, 1);

			/* Write the legalization word */
			temp = rtSaCbPtr + ADT_L1_1553_RT_SA_CB_ILLEGALBITS;
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, pIllegalBits, 1);
		}

	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_1553_RT_SA_CDPAllocate
 *****************************************************************************/
/*! \brief Allocates a RT SA/MC Control Block and allocates/links CDPs.
 *
 * This function allocates and links the requested number of CDPs for the RT/SA.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param rtAddr is the RT address.
 * @param tr is transmit(1) or receive(0).
 * @param subAddr is the subaddress.
 * @param numCDP is the requested number of CDP buffers.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid RT address, TR, subaddress, or numCDP
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_RT_SA_CDPAllocate(ADT_L0_UINT32 devID, ADT_L0_UINT32 rtAddr, 
											ADT_L0_UINT32 tr, ADT_L0_UINT32 subAddr, 
											ADT_L0_UINT32 numCDP) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, peCSR, sRtAddr;
	ADT_L0_UINT32 data, temp, rtCbOffset, rtFtPtr, rtSaCbPtr, lastCdpPtr, cdpPtr, firstCdpPtr, i;
	ADT_L0_UINT32 memAvailable;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if ((rtAddr < 32) && (tr < 2) && (subAddr < 32) && (numCDP > 0))
	{
		/* Read the PE CSR - this tells us if the board is configured for SINGLE or MULTIPLE RT */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &peCSR, 1);

		/* Determine RT CB offset based on SINGLE or MULTIPLE RT configuration */
		if (peCSR & 0x00000008) /* Multiple RT configuration */
		{
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS + (rtAddr * ADT_L1_1553_RT_CB_SIZE);
		}
		else /* Single RT configuration */
		{
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS;

			/* Read the RT CSR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_RT_CSR, &data, 1);

			/* Get the SRT address (bits 16-20) */
			sRtAddr = (data & 0x001F0000) >> 16;

			/* Verify that selected RT address matches the Single RT Address */
			if (rtAddr != sRtAddr) result = ADT_ERR_BAD_INPUT;
		}

		/* Check if there is enough memory to allocate */
		result = ADT_L1_GetMemoryAvailable(devID, &memAvailable);
		if (result == ADT_SUCCESS)
		{

			if((numCDP * ADT_L1_1553_CDP_SIZE) > memAvailable)
			{
				result = ADT_ERR_MEM_MGT_NO_MEM;
				return( result);
			}
			
			/* Get the RT Filter Table Pointer */
			temp = rtCbOffset + ADT_L1_1553_RT_CB_FTPTR;
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &rtFtPtr, 1);

			/* Allocate a new RT SA/MC Control Block, set filter table entry to point to this */
			result = ADT_L1_MemoryAlloc(devID, ADT_L1_1553_RT_SA_CB_SIZE, &rtSaCbPtr);
			if (result != ADT_SUCCESS) return( result );

			temp = rtFtPtr + (tr * 32 * 4) + (subAddr * 4);
			result = ADT_L0_WriteMem32(devID, channelRegOffset + temp, &rtSaCbPtr, 1);

			/* Write the RT SA/MC Control Block legalization word - default to all legal */
			data = 0x00000000;
			temp = rtSaCbPtr + ADT_L1_1553_RT_SA_CB_ILLEGALBITS;
			result = ADT_L0_WriteMem32(devID, channelRegOffset + temp, &data, 1);

			/* Write the RT SA/MC Control Block API number of CDPs */
			temp = rtSaCbPtr + ADT_L1_1553_RT_SA_CB_APINUMCDP;
			result = ADT_L0_WriteMem32(devID, channelRegOffset + temp, &numCDP, 1);

			/* Allocate and link the first CDP */
			result = ADT_L1_MemoryAlloc(devID, ADT_L1_1553_CDP_SIZE, &firstCdpPtr);
			if (result != ADT_SUCCESS) return( result );
					
			/* Write CDP Control Word */
			data = 0x3F000000;
			result = ADT_L0_WriteMem32(devID, channelRegOffset + firstCdpPtr + 0x1C, &data, 1);

			/* Set NEXT PTR to point to itself */
			result = ADT_L0_WriteMem32(devID, channelRegOffset + firstCdpPtr, &firstCdpPtr, 1);
			
			/* Write the API info word to the CDP */
			data = rtAddr << ADT_L1_1553_CDP_RAPI_RT_RTADDR;
			data |= tr << 18;
			data |= subAddr << 12;
			data |= ADT_L1_1553_CDP_RAPI_RT_ID;
			result = ADT_L0_WriteMem32(devID, channelRegOffset + firstCdpPtr + ADT_L1_1553_CDP_RESV_API, &data, 1);

			/* Write the RT SA/MC Control Block CDP pointer */
			temp = rtSaCbPtr + ADT_L1_1553_RT_SA_CB_CDPPTR;
			result = ADT_L0_WriteMem32(devID, channelRegOffset + temp, &firstCdpPtr, 1);

			/* Write the RT SA/MC Control Block API start CDP pointer */
			temp = rtSaCbPtr + ADT_L1_1553_RT_SA_CB_APISTARTCDP;
			result = ADT_L0_WriteMem32(devID, channelRegOffset + temp, &firstCdpPtr, 1);

			/* Allocate and link any additional CDPs */
			cdpPtr = firstCdpPtr;
			for (i=1; i<numCDP; i++) {
				lastCdpPtr = cdpPtr;

				result = ADT_L1_MemoryAlloc(devID, ADT_L1_1553_CDP_SIZE, &cdpPtr);
				if (result != ADT_SUCCESS) return( result );
						
				/* Write CDP Control Word */
				data = 0x3F000000;
				result = ADT_L0_WriteMem32(devID, channelRegOffset + cdpPtr + 0x1C, &data, 1);

				result = ADT_L0_WriteMem32(devID, channelRegOffset + lastCdpPtr, &cdpPtr, 1);
				result = ADT_L0_WriteMem32(devID, channelRegOffset + cdpPtr, &firstCdpPtr, 1);

				/* Write the API info word to the CDP */
				data = rtAddr << ADT_L1_1553_CDP_RAPI_RT_RTADDR;
				data |= tr << 18;
				data |= subAddr << 12;
				data |= i & 0x00000FFF;
				data |= ADT_L1_1553_CDP_RAPI_RT_ID;
				result = ADT_L0_WriteMem32(devID, channelRegOffset + cdpPtr + ADT_L1_1553_CDP_RESV_API, &data, 1);

			}
		}
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_RT_SA_CDPFree
 *****************************************************************************/
/*! \brief Frees the RT SA/MC Control Block and CDPs.
 *
 * This function frees the RT SA Control Block and CDPs for the RT/SA and 
 * resets to default buffer.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param rtAddr is the RT address.
 * @param tr is transmit(1) or receive(0).
 * @param subAddr is the subaddress.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid RT address, TR, subaddress, or numCDP
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_RT_SA_CDPFree(ADT_L0_UINT32 devID, ADT_L0_UINT32 rtAddr, 
										ADT_L0_UINT32 tr, ADT_L0_UINT32 subAddr) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, peCSR, sRtAddr;
	ADT_L0_UINT32 data, temp, rtCbOffset, rtFtPtr, rtSaCbPtr, rtSaCbPtrDefault, cdpPtr, nextCdpPtr;
	ADT_L0_UINT32 i, j, k, numCdp, cdpSizeInBytes;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if ((rtAddr < 32) && (tr < 2) && (subAddr < 32)) {
		/* Read the PE CSR - this tells us if the board is configured for SINGLE or MULTIPLE RT */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &peCSR, 1);

		/* Determine RT CB offset based on SINGLE or MULTIPLE RT configuration */
		if (peCSR & 0x00000008) {  /* Multiple RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS + (rtAddr * ADT_L1_1553_RT_CB_SIZE);
		}
		else {  /* Single RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS;

			/* Read the RT CSR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_RT_CSR, &data, 1);

			/* Get the SRT address (bits 16-20) */
			sRtAddr = (data & 0x001F0000) >> 16;

			/* Verify that selected RT address matches the Single RT Address */
			if (rtAddr != sRtAddr) result = ADT_ERR_BAD_INPUT;
		}

		if (result == ADT_SUCCESS) {
			/* Get the RT Filter Table Pointer */
			temp = rtCbOffset + ADT_L1_1553_RT_CB_FTPTR;
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &rtFtPtr, 1);

			/* Get the DEFAULT RT SA CB PTR from the MC31 entry */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + rtFtPtr + (127 * 4), &rtSaCbPtrDefault, 1);

			/* Get the RT SA CB PTR (to be freed) */
			i = tr * 32 + subAddr;
			result = ADT_L0_ReadMem32(devID, channelRegOffset + rtFtPtr + (i * 4), &rtSaCbPtr, 1);

			/* Set the filter table entry to the default RT SA CB */
			result = ADT_L0_WriteMem32(devID, channelRegOffset + rtFtPtr + (i * 4), &rtSaCbPtrDefault, 1);

			/* Get the number of CDPs from the CB */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + rtSaCbPtr + ADT_L1_1553_RT_SA_CB_APINUMCDP, &numCdp, 1);

			/* If the number of CDPs is NON-ZERO then this has NOT already been freed.
			 * We have to be careful not to free the same memory more than once.
			 * Things like the default RT structures point multiple filter table
			 * entries to the same RT SA CB.  If we see a ZERO here, this tells us
			 * that the CB and associated CDPs have already been freed.
			 */
			if (numCdp != 0) {

				/* Get the first CDP pointer */
				result = ADT_L0_ReadMem32(devID, channelRegOffset + rtSaCbPtr + ADT_L1_1553_RT_SA_CB_APISTARTCDP, &cdpPtr, 1);

				for (j=0; j<numCdp; j++) {
					/* Get the next CDP pointer */
					result = ADT_L0_ReadMem32(devID, channelRegOffset + cdpPtr, &nextCdpPtr, 1);

					cdpSizeInBytes = ADT_L1_1553_CDP_SIZE;

					/* Clear and free the CDP */
					data = 0;
					for (k=0; k<(cdpSizeInBytes/4); k++) {
						result = ADT_L0_WriteMem32(devID, channelRegOffset + cdpPtr + (k * 4), &data, 1);
					}
					result = ADT_L1_MemoryFree(devID, cdpPtr, cdpSizeInBytes);

					/* Move to the next CDP */
					cdpPtr = nextCdpPtr;
				}

				/* Clear and Free the RT SA CB */
				data = 0;
				result = ADT_L0_WriteMem32(devID, channelRegOffset + rtSaCbPtr + (0 * 4), &data, 1);
				result = ADT_L0_WriteMem32(devID, channelRegOffset + rtSaCbPtr + (1 * 4), &data, 1);
				result = ADT_L0_WriteMem32(devID, channelRegOffset + rtSaCbPtr + (2 * 4), &data, 1);
				result = ADT_L0_WriteMem32(devID, channelRegOffset + rtSaCbPtr + (3 * 4), &data, 1);
				result = ADT_L1_MemoryFree(devID, rtSaCbPtr, ADT_L1_1553_RT_SA_CB_SIZE);
			}
		}

	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_RT_SA_CDPWrite
 *****************************************************************************/
/*! \brief Writes a CDP for the RT/SA 
 *
 * This function writes a CDP for the RT/SA.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param rtAddr is the RT address.
 * @param tr is transmit(1) or receive(0).
 * @param subAddr is the subaddress.
 * @param cdpNum is the index of the CDP to write.
 * @param pCdp is a pointer to the CDP structure to be written.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid RT address, TR, subaddress, or cdpNum
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_RT_SA_CDPWrite(ADT_L0_UINT32 devID, ADT_L0_UINT32 rtAddr, ADT_L0_UINT32 tr, 
										 ADT_L0_UINT32 subAddr, ADT_L0_UINT32 cdpNum, ADT_L1_1553_CDP *pCdp) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, peCSR, data, sRtAddr;
	ADT_L0_UINT32 temp, rtCbOffset, rtFtPtr, rtSaCbPtr, startCdp, selectedCdp, i;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if ((rtAddr < 32) && (tr < 2) && (subAddr < 32)) {
		/* Read the PE CSR - this tells us if the board is configured for SINGLE or MULTIPLE RT */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &peCSR, 1);

		/* Determine RT CB offset based on SINGLE or MULTIPLE RT configuration */
		if (peCSR & 0x00000008) {  /* Multiple RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS + (rtAddr * ADT_L1_1553_RT_CB_SIZE);
		}
		else {  /* Single RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS;

			/* Read the RT CSR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_RT_CSR, &data, 1);

			/* Get the SRT address (bits 16-20) */
			sRtAddr = (data & 0x001F0000) >> 16;

			/* Verify that selected RT address matches the Single RT Address */
			if (rtAddr != sRtAddr) result = ADT_ERR_BAD_INPUT;
		}

		if (result == ADT_SUCCESS) {
			/* Get the RT Filter Table Pointer */
			temp = rtCbOffset + ADT_L1_1553_RT_CB_FTPTR;
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &rtFtPtr, 1);

			/* Get the RT SA Control Block Pointer */
			temp = rtFtPtr + (tr * 32 * 4) + (subAddr * 4);
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &rtSaCbPtr, 1);

			/* Check that the selected CDP number does not exceed the actual number of CDPs */
			temp = rtSaCbPtr + ADT_L1_1553_RT_SA_CB_APINUMCDP;
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &i, 1);
			if (cdpNum >= i) {
				result = ADT_ERR_BAD_INPUT;
			}

			if (result == ADT_SUCCESS) {
				/* Read the API starting CDP address */
				temp = rtSaCbPtr + ADT_L1_1553_RT_SA_CB_APISTARTCDP;
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
				/************************************/
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
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}

/******************************************************************************
  FUNCTION:		ADT_L1_1553_RT_SA_CDPWriteWords
 *****************************************************************************/
/*! \brief Writes a 32-bit Words to a CDP for the RT/SA 
 *
 * This function writes Words to a CDP for the RT/SA.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param rtAddr is the RT address.
 * @param tr is transmit(1) or receive(0).
 * @param subAddr is the subaddress.
 * @param cdpNum is the index of the CDP to write.
 * @param wordOffset is the word offset in to the CDP where the write will occur
 * @param numOfWords is the number of words to be written from pWords
 * @param pWords is a pointer to the array of words to be written to the CDP
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid RT address, TR, subaddress, cdpNum, numOfWords or wordOffset
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_RT_SA_CDPWriteWords(ADT_L0_UINT32 devID, ADT_L0_UINT32 rtAddr, ADT_L0_UINT32 tr, 
										 ADT_L0_UINT32 subAddr, ADT_L0_UINT32 cdpNum, ADT_L0_UINT32 wordOffset, 
										 ADT_L0_UINT32 numOfWords, ADT_L0_UINT32 *pWords) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, peCSR, data, sRtAddr;
	ADT_L0_UINT32 temp, rtCbOffset, rtFtPtr, rtSaCbPtr, startCdp, selectedCdp, i;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if ((rtAddr < 32) && (tr < 2) && (subAddr < 32)) {
		/* Read the PE CSR - this tells us if the board is configured for SINGLE or MULTIPLE RT */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &peCSR, 1);

		/* Determine RT CB offset based on SINGLE or MULTIPLE RT configuration */
		if (peCSR & 0x00000008) {  /* Multiple RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS + (rtAddr * ADT_L1_1553_RT_CB_SIZE);
		}
		else {  /* Single RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS;

			/* Read the RT CSR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_RT_CSR, &data, 1);

			/* Get the SRT address (bits 16-20) */
			sRtAddr = (data & 0x001F0000) >> 16;

			/* Verify that selected RT address matches the Single RT Address */
			if (rtAddr != sRtAddr) result = ADT_ERR_BAD_INPUT;
		}

		if (result == ADT_SUCCESS) {
			/* Get the RT Filter Table Pointer */
			temp = rtCbOffset + ADT_L1_1553_RT_CB_FTPTR;
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &rtFtPtr, 1);

			/* Get the RT SA Control Block Pointer */
			temp = rtFtPtr + (tr * 32 * 4) + (subAddr * 4);
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &rtSaCbPtr, 1);

			/* Check that the selected CDP number does not exceed the actual number of CDPs */
			temp = rtSaCbPtr + ADT_L1_1553_RT_SA_CB_APINUMCDP;
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &i, 1);
			if (cdpNum >= i) {
				result = ADT_ERR_BAD_INPUT;
			}

			if (result == ADT_SUCCESS) {
				/* Read the API starting CDP address */
				temp = rtSaCbPtr + ADT_L1_1553_RT_SA_CB_APISTARTCDP;
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
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}

/******************************************************************************
  FUNCTION:		ADT_L1_1553_RT_SA_CDPRead
 *****************************************************************************/
/*! \brief Reads a CDP for the RT/SA 
 *
 * This function reads a CDP for the RT/SA.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param rtAddr is the RT address.
 * @param tr is transmit(1) or receive(0).
 * @param subAddr is the subaddress.
 * @param cdpNum is the index of the CDP to write.
 * @param pCdp is a pointer to the CDP structure to be read.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid RT address, TR, subaddress, or cdpNum
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_RT_SA_CDPRead(ADT_L0_UINT32 devID, ADT_L0_UINT32 rtAddr, ADT_L0_UINT32 tr, 
										ADT_L0_UINT32 subAddr, ADT_L0_UINT32 cdpNum, ADT_L1_1553_CDP *pCdp) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, peCSR, data, sRtAddr;
	ADT_L0_UINT32 temp, rtCbOffset, rtFtPtr, rtSaCbPtr, startCdp, selectedCdp, i;
	ADT_L0_UINT32 datablock[ADT_RW_MEM_MAX_SIZE];

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if ((rtAddr < 32) && (tr < 2) && (subAddr < 32)) {
		/* Read the PE CSR - this tells us if the board is configured for SINGLE or MULTIPLE RT */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &peCSR, 1);

		/* Determine RT CB offset based on SINGLE or MULTIPLE RT configuration */
		if (peCSR & 0x00000008) {  /* Multiple RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS + (rtAddr * ADT_L1_1553_RT_CB_SIZE);
		}
		else {  /* Single RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS;

			/* Read the RT CSR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_RT_CSR, &data, 1);

			/* Get the SRT address (bits 16-20) */
			sRtAddr = (data & 0x001F0000) >> 16;

			/* Verify that selected RT address matches the Single RT Address */
			if (rtAddr != sRtAddr) result = ADT_ERR_BAD_INPUT;
		}

		if (result == ADT_SUCCESS) {
			/* Get the RT Filter Table Pointer */
			temp = rtCbOffset + ADT_L1_1553_RT_CB_FTPTR;
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &rtFtPtr, 1);

			/* Get the RT SA Control Block Pointer */
			temp = rtFtPtr + (tr * 32 * 4) + (subAddr * 4);
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &rtSaCbPtr, 1);

			/* Check that the selected CDP number does not exceed the actual number of CDPs */
			temp = rtSaCbPtr + ADT_L1_1553_RT_SA_CB_APINUMCDP;
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &i, 1);
			if (cdpNum >= i) {
				result = ADT_ERR_BAD_INPUT;
			}

			if (result == ADT_SUCCESS) {
				/* Read the API starting CDP address */
				temp = rtSaCbPtr + ADT_L1_1553_RT_SA_CB_APISTARTCDP;
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
				/************************************/
#endif


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
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_RT_SA_CDPReadWords
 *****************************************************************************/
/*! \brief Reads 32-bit Words from a CDP for the RT/SA 
 *
 * This function reads Words from a CDP for the RT/SA.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param rtAddr is the RT address.
 * @param tr is transmit(1) or receive(0).
 * @param subAddr is the subaddress.
 * @param cdpNum is the index of the CDP to write.
 * @param wordOffset is the word offset in to the CDP where the write will occur
 * @param numOfWords is the number of words to be written from pWords
 * @param pWords is a pointer to the array of words to be written to the CDP
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid RT address, TR, subaddress, or cdpNum
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_RT_SA_CDPReadWords(ADT_L0_UINT32 devID, ADT_L0_UINT32 rtAddr, ADT_L0_UINT32 tr, 
										 ADT_L0_UINT32 subAddr, ADT_L0_UINT32 cdpNum, ADT_L0_UINT32 wordOffset, 
										 ADT_L0_UINT32 numOfWords, ADT_L0_UINT32 *pWords) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, peCSR, data, sRtAddr;
	ADT_L0_UINT32 temp, rtCbOffset, rtFtPtr, rtSaCbPtr, startCdp, selectedCdp, i;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if ((rtAddr < 32) && (tr < 2) && (subAddr < 32)) {
		/* Read the PE CSR - this tells us if the board is configured for SINGLE or MULTIPLE RT */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &peCSR, 1);

		/* Determine RT CB offset based on SINGLE or MULTIPLE RT configuration */
		if (peCSR & 0x00000008) {  /* Multiple RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS + (rtAddr * ADT_L1_1553_RT_CB_SIZE);
		}
		else {  /* Single RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS;

			/* Read the RT CSR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_RT_CSR, &data, 1);

			/* Get the SRT address (bits 16-20) */
			sRtAddr = (data & 0x001F0000) >> 16;

			/* Verify that selected RT address matches the Single RT Address */
			if (rtAddr != sRtAddr) result = ADT_ERR_BAD_INPUT;
		}

		if (result == ADT_SUCCESS) {
			/* Get the RT Filter Table Pointer */
			temp = rtCbOffset + ADT_L1_1553_RT_CB_FTPTR;
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &rtFtPtr, 1);

			/* Get the RT SA Control Block Pointer */
			temp = rtFtPtr + (tr * 32 * 4) + (subAddr * 4);
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &rtSaCbPtr, 1);

			/* Check that the selected CDP number does not exceed the actual number of CDPs */
			temp = rtSaCbPtr + ADT_L1_1553_RT_SA_CB_APINUMCDP;
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &i, 1);
			if (cdpNum >= i) {
				result = ADT_ERR_BAD_INPUT;
			}

			if (result == ADT_SUCCESS) {
				/* Read the API starting CDP address */
				temp = rtSaCbPtr + ADT_L1_1553_RT_SA_CB_APISTARTCDP;
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

					/* Read the words from the selected CDP offset */
					if ((numOfWords) && ((wordOffset + numOfWords) <= (ADT_L1_1553_CDP_SIZE/4)))
							result = ADT_L0_ReadMem32(devID, channelRegOffset + selectedCdp + (wordOffset*4), pWords, numOfWords);
					else
						result = ADT_ERR_BAD_INPUT;
				}
			}
		}
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}

/******************************************************************************
  FUNCTION:		ADT_L1_1553_RT_MC_CDPAllocate
 *****************************************************************************/
/*! \brief Allocates a RT SA/MC Control Block and allocates/links CDPs.
 *
 * This function allocates and links the requested number of CDPs for the 
 * RT MODE CODE
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param rtAddr is the RT address.
 * @param tr is transmit(1) or receive(0).
 * @param modeCode is the mode code.
 * @param numCDP is the requested number of CDP buffers.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid RT address, TR, subaddress, or numCDP
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_RT_MC_CDPAllocate(ADT_L0_UINT32 devID, ADT_L0_UINT32 rtAddr, 
											ADT_L0_UINT32 tr, ADT_L0_UINT32 modeCode, 
											ADT_L0_UINT32 numCDP) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, peCSR, sRtAddr;
	ADT_L0_UINT32 data, temp, rtCbOffset, rtFtPtr, rtSaCbPtr, lastCdpPtr, cdpPtr, firstCdpPtr, i;
	ADT_L0_UINT32 memAvailable;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* We reserve MC 31 as always on DEFAULT buffer. */
	if ((rtAddr < 32) && (tr < 2) && (modeCode < 31) && (numCDP > 0)) {
		/* Read the PE CSR - this tells us if the board is configured for SINGLE or MULTIPLE RT */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &peCSR, 1);

		/* Determine RT CB offset based on SINGLE or MULTIPLE RT configuration */
		if (peCSR & 0x00000008) {  /* Multiple RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS + (rtAddr * ADT_L1_1553_RT_CB_SIZE);
		}
		else {  /* Single RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS;

			/* Read the RT CSR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_RT_CSR, &data, 1);

			/* Get the SRT address (bits 16-20) */
			sRtAddr = (data & 0x001F0000) >> 16;

			/* Verify that selected RT address matches the Single RT Address */
			if (rtAddr != sRtAddr) result = ADT_ERR_BAD_INPUT;
		}

		if (result == ADT_SUCCESS) {

			if ( ADT_L1_GetMemoryAvailable(devID, &memAvailable) == ADT_SUCCESS )
			{
				if((numCDP * ADT_L1_1553_CDP_SIZE) > memAvailable)
				{
					result = ADT_ERR_MEM_MGT_NO_MEM;
					return( result);
				}
			}

			/* Get the RT Filter Table Pointer */
			temp = rtCbOffset + ADT_L1_1553_RT_CB_FTPTR;
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &rtFtPtr, 1);

			/* Allocate a new RT SA/MC Control Block, set filter table entry to point to this */
			result = ADT_L1_MemoryAlloc(devID, ADT_L1_1553_RT_SA_CB_SIZE, &rtSaCbPtr);
			if (result != ADT_SUCCESS) return( result );

			temp = rtFtPtr + (64 * 4) + (tr * 32 * 4) + (modeCode * 4);
			result = ADT_L0_WriteMem32(devID, channelRegOffset + temp, &rtSaCbPtr, 1);

			/* Write the RT SA/MC Control Block legalization word - default to all legal */
			data = 0x00000000;
			temp = rtSaCbPtr + ADT_L1_1553_RT_SA_CB_ILLEGALBITS;
			result = ADT_L0_WriteMem32(devID, channelRegOffset + temp, &data, 1);

			/* Write the RT SA/MC Control Block number of CDPs */
			temp = rtSaCbPtr + ADT_L1_1553_RT_SA_CB_APINUMCDP;
			result = ADT_L0_WriteMem32(devID, channelRegOffset + temp, &numCDP, 1);

			/* Allocate and link the first CDP */
			result = ADT_L1_MemoryAlloc(devID, ADT_L1_1553_CDP_SIZE, &firstCdpPtr);
			if (result != ADT_SUCCESS) return( result );
					
			/* Write CDP Control Word */
			data = 0x3F000000;  
			result = ADT_L0_WriteMem32(devID, channelRegOffset + firstCdpPtr + 0x1C, &data, 1);

			/* Set the NEXT PTR to point to itself */
			result = ADT_L0_WriteMem32(devID, channelRegOffset + firstCdpPtr, &firstCdpPtr, 1);

			/* Write the API info word to the CDP */
			data = rtAddr << ADT_L1_1553_CDP_RAPI_RT_RTADDR;
			data |= tr << 18;
			data |= modeCode << 12;
			data |= ADT_L1_1553_CDP_RAPI_RT_ID | ADT_L1_1553_CDP_RAPI_MC_ID;
			result = ADT_L0_WriteMem32(devID, channelRegOffset + firstCdpPtr + ADT_L1_1553_CDP_RESV_API, &data, 1);

			/* Write the RT SA/MC Control Block CDP pointer */
			temp = rtSaCbPtr + ADT_L1_1553_RT_SA_CB_CDPPTR;
			result = ADT_L0_WriteMem32(devID, channelRegOffset + temp, &firstCdpPtr, 1);

			/* Write the RT SA/MC Control Block API start CDP pointer */
			temp = rtSaCbPtr + ADT_L1_1553_RT_SA_CB_APISTARTCDP;
			result = ADT_L0_WriteMem32(devID, channelRegOffset + temp, &firstCdpPtr, 1);

			/* Allocate and link any additional CDPs */
			cdpPtr = firstCdpPtr;
			for (i=1; i<numCDP; i++) {
				lastCdpPtr = cdpPtr;

				result = ADT_L1_MemoryAlloc(devID, ADT_L1_1553_CDP_SIZE, &cdpPtr);
				if (result != ADT_SUCCESS) return( result );
						
				/* Write CDP Control Word */
				data = 0x3F000000;  
				result = ADT_L0_WriteMem32(devID, channelRegOffset + cdpPtr + 0x1C, &data, 1);

				result = ADT_L0_WriteMem32(devID, channelRegOffset + lastCdpPtr, &cdpPtr, 1);
				result = ADT_L0_WriteMem32(devID, channelRegOffset + cdpPtr, &firstCdpPtr, 1);

				/* Write the API info word to the CDP */
				data = rtAddr << ADT_L1_1553_CDP_RAPI_RT_RTADDR;
				data |= tr << 18;
				data |= modeCode << 12;
				data |= i & 0x00000FFF;
				data |= ADT_L1_1553_CDP_RAPI_RT_ID | ADT_L1_1553_CDP_RAPI_MC_ID;
				result = ADT_L0_WriteMem32(devID, channelRegOffset + cdpPtr + ADT_L1_1553_CDP_RESV_API, &data, 1);

			}
		}
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_1553_RT_MC_CDPFree
 *****************************************************************************/
/*! \brief Frees the RT MC Control Block and CDPs.
 *
 * This function frees the RT MC Control Block and CDPs for the RT/MC and 
 * resets to default buffer.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param rtAddr is the RT address.
 * @param tr is transmit(1) or receive(0).
 * @param modeCode is the mode code.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid RT address, TR, subaddress, or numCDP
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_RT_MC_CDPFree(ADT_L0_UINT32 devID, ADT_L0_UINT32 rtAddr, 
										ADT_L0_UINT32 tr, ADT_L0_UINT32 modeCode) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, peCSR, sRtAddr;
	ADT_L0_UINT32 data, temp, rtCbOffset, rtFtPtr, rtSaCbPtr, rtSaCbPtrDefault, cdpPtr, nextCdpPtr;
	ADT_L0_UINT32 i, j, k, numCdp, cdpSizeInBytes;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* We reserve MC 31 as always on DEFAULT buffer. */
	if ((rtAddr < 32) && (tr < 2) && (modeCode < 31)) {
		/* Read the PE CSR - this tells us if the board is configured for SINGLE or MULTIPLE RT */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &peCSR, 1);

		/* Determine RT CB offset based on SINGLE or MULTIPLE RT configuration */
		if (peCSR & 0x00000008) {  /* Multiple RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS + (rtAddr * ADT_L1_1553_RT_CB_SIZE);
		}
		else {  /* Single RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS;

			/* Read the RT CSR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_RT_CSR, &data, 1);

			/* Get the SRT address (bits 16-20) */
			sRtAddr = (data & 0x001F0000) >> 16;

			/* Verify that selected RT address matches the Single RT Address */
			if (rtAddr != sRtAddr) result = ADT_ERR_BAD_INPUT;
		}

		if (result == ADT_SUCCESS) {
			/* Get the RT Filter Table Pointer */
			temp = rtCbOffset + ADT_L1_1553_RT_CB_FTPTR;
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &rtFtPtr, 1);

			/* Get the DEFAULT RT SA CB PTR from the MC31 entry */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + rtFtPtr + (127 * 4), &rtSaCbPtrDefault, 1);

			/* Get the RT SA CB PTR (to be freed) */
			i = 64 + tr * 32 + modeCode;
			result = ADT_L0_ReadMem32(devID, channelRegOffset + rtFtPtr + (i * 4), &rtSaCbPtr, 1);

			/* Set the filter table entry to the default RT SA CB */
			result = ADT_L0_WriteMem32(devID, channelRegOffset + rtFtPtr + (i * 4), &rtSaCbPtrDefault, 1);

			/* Get the number of CDPs from the CB */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + rtSaCbPtr + ADT_L1_1553_RT_SA_CB_APINUMCDP, &numCdp, 1);

			/* If the number of CDPs is NON-ZERO then this has NOT already been freed.
			 * We have to be careful not to free the same memory more than once.
			 * Things like the default RT structures point multiple filter table
			 * entries to the same RT SA CB.  If we see a ZERO here, this tells us
			 * that the CB and associated CDPs have already been freed.
			 */
			if (numCdp != 0) {

				/* Get the first CDP pointer */
				result = ADT_L0_ReadMem32(devID, channelRegOffset + rtSaCbPtr + ADT_L1_1553_RT_SA_CB_APISTARTCDP, &cdpPtr, 1);

				for (j=0; j<numCdp; j++) {
					/* Get the next CDP pointer */
					result = ADT_L0_ReadMem32(devID, channelRegOffset + cdpPtr, &nextCdpPtr, 1);

					cdpSizeInBytes = ADT_L1_1553_CDP_SIZE;

					/* Clear and free the CDP */
					data = 0;
					for (k=0; k<(cdpSizeInBytes/4); k++) {
						result = ADT_L0_WriteMem32(devID, channelRegOffset + cdpPtr + (k * 4), &data, 1);
					}
					result = ADT_L1_MemoryFree(devID, cdpPtr, cdpSizeInBytes);

					/* Move to the next CDP */
					cdpPtr = nextCdpPtr;
				}

				/* Clear and Free the RT SA CB */
				data = 0;
				result = ADT_L0_WriteMem32(devID, channelRegOffset + rtSaCbPtr + (0 * 4), &data, 1);
				result = ADT_L0_WriteMem32(devID, channelRegOffset + rtSaCbPtr + (1 * 4), &data, 1);
				result = ADT_L0_WriteMem32(devID, channelRegOffset + rtSaCbPtr + (2 * 4), &data, 1);
				result = ADT_L0_WriteMem32(devID, channelRegOffset + rtSaCbPtr + (3 * 4), &data, 1);
				result = ADT_L1_MemoryFree(devID, rtSaCbPtr, ADT_L1_1553_RT_SA_CB_SIZE);
			}
		}
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_1553_RT_MC_CDPWrite
 *****************************************************************************/
/*! \brief Writes a CDP for the RT MODE CODE 
 *
 * This function writes a CDP for the RT MODE CODE.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param rtAddr is the RT address.
 * @param tr is transmit(1) or receive(0).
 * @param modeCode is the mode code.
 * @param cdpNum is the index of the CDP to write.
 * @param pCdp is a pointer to the CDP structure to be written.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid RT address, TR, modeCode, or cdpNum
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_RT_MC_CDPWrite(ADT_L0_UINT32 devID, ADT_L0_UINT32 rtAddr, ADT_L0_UINT32 tr, 
										 ADT_L0_UINT32 modeCode, ADT_L0_UINT32 cdpNum, ADT_L1_1553_CDP *pCdp) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, peCSR, data, sRtAddr;
	ADT_L0_UINT32 temp, rtCbOffset, rtFtPtr, rtSaCbPtr, startCdp, selectedCdp, i;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if ((rtAddr < 32) && (tr < 2) && (modeCode < 32)) {
		/* Read the PE CSR - this tells us if the board is configured for SINGLE or MULTIPLE RT */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &peCSR, 1);

		/* Determine RT CB offset based on SINGLE or MULTIPLE RT configuration */
		if (peCSR & 0x00000008) {  /* Multiple RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS + (rtAddr * ADT_L1_1553_RT_CB_SIZE);
		}
		else {  /* Single RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS;

			/* Read the RT CSR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_RT_CSR, &data, 1);

			/* Get the SRT address (bits 16-20) */
			sRtAddr = (data & 0x001F0000) >> 16;

			/* Verify that selected RT address matches the Single RT Address */
			if (rtAddr != sRtAddr) result = ADT_ERR_BAD_INPUT;
		}

		if (result == ADT_SUCCESS) {
			/* Get the RT Filter Table Pointer */
			temp = rtCbOffset + ADT_L1_1553_RT_CB_FTPTR;
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &rtFtPtr, 1);

			/* Get the RT MC Control Block Pointer */
			temp = rtFtPtr + (64 * 4) + (tr * 32 * 4) + (modeCode * 4);
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &rtSaCbPtr, 1);

			/* Check that the selected CDP number does not exceed the actual number of CDPs */
			temp = rtSaCbPtr + ADT_L1_1553_RT_SA_CB_APINUMCDP;
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &i, 1);
			if (cdpNum >= i) {
				result = ADT_ERR_BAD_INPUT;
			}

			if (result == ADT_SUCCESS) {
				/* Read the API starting CDP address */
				temp = rtSaCbPtr + ADT_L1_1553_RT_SA_CB_APISTARTCDP;
				result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &startCdp, 1);

				/* Follow NEXT pointers to find the selected CDP */
				i = 0;
				selectedCdp = startCdp;
				while (i < cdpNum) {
					result = ADT_L0_ReadMem32(devID, channelRegOffset + selectedCdp, &temp, 1);
					selectedCdp = temp;
					i++;
				}

				if (result == ADT_SUCCESS) {

					/* Write the selected CDP (all EXCEPT the next pointer and reserved words) 
					 * Also not overwriting the API INFO word.
					 */
					/* For BLOCK write, we start at the Mask Value word (skip first 5 words of CDP) */
					result = ADT_L0_WriteMem32(devID, channelRegOffset + selectedCdp + ADT_L1_1553_CDP_MASKVALUE, &pCdp->MaskValue, (ADT_L1_1553_CDP_SIZE - ADT_L1_1553_CDP_MASKVALUE)/4);
				}
			}
		}
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}

/******************************************************************************
  FUNCTION:		ADT_L1_1553_RT_MC_CDPWriteWords
 *****************************************************************************/
/*! \brief Writes 32-bit Words to the CDP for the RT MODE CODE 
 *
 * This function writes Words to a CDP for the RT MODE CODE.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param rtAddr is the RT address.
 * @param tr is transmit(1) or receive(0).
 * @param modeCode is the mode code.
 * @param cdpNum is the index of the CDP to write.
 * @param wordOffset is the word offset in to the CDP where the write will occur
 * @param numOfWords is the number of words to be written from pWords
 * @param pWords is a pointer to the array of words to be written to the CDP
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid RT address, TR, modeCode, cdpNum, wordOffset or numOfWords
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_RT_MC_CDPWriteWords(ADT_L0_UINT32 devID, ADT_L0_UINT32 rtAddr, ADT_L0_UINT32 tr, 
										 ADT_L0_UINT32 modeCode, ADT_L0_UINT32 cdpNum, ADT_L0_UINT32 wordOffset, 
										 ADT_L0_UINT32 numOfWords, ADT_L0_UINT32 *pWords) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, peCSR, data, sRtAddr;
	ADT_L0_UINT32 temp, rtCbOffset, rtFtPtr, rtSaCbPtr, startCdp, selectedCdp, i;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if ((rtAddr < 32) && (tr < 2) && (modeCode < 32)) {
		/* Read the PE CSR - this tells us if the board is configured for SINGLE or MULTIPLE RT */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &peCSR, 1);

		/* Determine RT CB offset based on SINGLE or MULTIPLE RT configuration */
		if (peCSR & 0x00000008) {  /* Multiple RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS + (rtAddr * ADT_L1_1553_RT_CB_SIZE);
		}
		else {  /* Single RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS;

			/* Read the RT CSR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_RT_CSR, &data, 1);

			/* Get the SRT address (bits 16-20) */
			sRtAddr = (data & 0x001F0000) >> 16;

			/* Verify that selected RT address matches the Single RT Address */
			if (rtAddr != sRtAddr) result = ADT_ERR_BAD_INPUT;
		}

		if (result == ADT_SUCCESS) {
			/* Get the RT Filter Table Pointer */
			temp = rtCbOffset + ADT_L1_1553_RT_CB_FTPTR;
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &rtFtPtr, 1);

			/* Get the RT MC Control Block Pointer */
			temp = rtFtPtr + (64 * 4) + (tr * 32 * 4) + (modeCode * 4);
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &rtSaCbPtr, 1);

			/* Check that the selected CDP number does not exceed the actual number of CDPs */
			temp = rtSaCbPtr + ADT_L1_1553_RT_SA_CB_APINUMCDP;
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &i, 1);
			if (cdpNum >= i) {
				result = ADT_ERR_BAD_INPUT;
			}

			if (result == ADT_SUCCESS) {
				/* Read the API starting CDP address */
				temp = rtSaCbPtr + ADT_L1_1553_RT_SA_CB_APISTARTCDP;
				result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &startCdp, 1);

				/* Follow NEXT pointers to find the selected CDP */
				i = 0;
				selectedCdp = startCdp;
				while (i < cdpNum) {
					result = ADT_L0_ReadMem32(devID, channelRegOffset + selectedCdp, &temp, 1);
					selectedCdp = temp;
					i++;
				}

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
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}

/******************************************************************************
  FUNCTION:		ADT_L1_1553_RT_MC_CDPRead
 *****************************************************************************/
/*! \brief Reads a CDP for the RT MODE CODE 
 *
 * This function reads a CDP for the RT MODE CODE.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param rtAddr is the RT address.
 * @param tr is transmit(1) or receive(0).
 * @param modeCode is the mode code.
 * @param cdpNum is the index of the CDP to write.
 * @param pCdp is a pointer to the CDP structure to be read.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid RT address, TR, subaddress, or cdpNum
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_RT_MC_CDPRead(ADT_L0_UINT32 devID, ADT_L0_UINT32 rtAddr, ADT_L0_UINT32 tr, 
										ADT_L0_UINT32 modeCode, ADT_L0_UINT32 cdpNum, ADT_L1_1553_CDP *pCdp) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, peCSR, data, sRtAddr;
	ADT_L0_UINT32 temp, rtCbOffset, rtFtPtr, rtSaCbPtr, startCdp, selectedCdp, i;
	ADT_L0_UINT32 datablock[ADT_RW_MEM_MAX_SIZE];

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if ((rtAddr < 32) && (tr < 2) && (modeCode < 32)) {
		/* Read the PE CSR - this tells us if the board is configured for SINGLE or MULTIPLE RT */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &peCSR, 1);

		/* Determine RT CB offset based on SINGLE or MULTIPLE RT configuration */
		if (peCSR & 0x00000008) {  /* Multiple RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS + (rtAddr * ADT_L1_1553_RT_CB_SIZE);
		}
		else {  /* Single RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS;

			/* Read the RT CSR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_RT_CSR, &data, 1);

			/* Get the SRT address (bits 16-20) */
			sRtAddr = (data & 0x001F0000) >> 16;

			/* Verify that selected RT address matches the Single RT Address */
			if (rtAddr != sRtAddr) result = ADT_ERR_BAD_INPUT;
		}

		if (result == ADT_SUCCESS) {
			/* Get the RT Filter Table Pointer */
			temp = rtCbOffset + ADT_L1_1553_RT_CB_FTPTR;
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &rtFtPtr, 1);

			/* Get the RT MC Control Block Pointer */
			temp = rtFtPtr + (64 * 4) + (tr * 32 * 4) + (modeCode * 4);
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &rtSaCbPtr, 1);

			/* Check that the selected CDP number does not exceed the actual number of CDPs */
			temp = rtSaCbPtr + ADT_L1_1553_RT_SA_CB_APINUMCDP;
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &i, 1);
			if (cdpNum >= i) {
				result = ADT_ERR_BAD_INPUT;
			}

			if (result == ADT_SUCCESS) {
				/* Read the API starting CDP address */
				temp = rtSaCbPtr + ADT_L1_1553_RT_SA_CB_APISTARTCDP;
				result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &startCdp, 1);

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
					pCdp->CMD1info = datablock[13];
					pCdp->CMD2info = datablock[14];
					pCdp->STS1info = datablock[15];
					pCdp->STS2info = datablock[16];
					for (i=0; i<32; i++)
						pCdp->DATAinfo[i] = datablock[17+i];
				}
			}
		}
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_1553_RT_MC_CDPReadWords
 *****************************************************************************/
/*! \brief Reads 32-bit Words from the CDP for the RT MODE CODE 
 *
 * This function reads words from CDP for the RT MODE CODE.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param rtAddr is the RT address.
 * @param tr is transmit(1) or receive(0).
 * @param modeCode is the mode code.
 * @param cdpNum is the index of the CDP to write.
 * @param wordOffset is the word offset in to the CDP where the write will occur
 * @param numOfWords is the number of words to be written from pWords
 * @param pWords is a pointer to the array of words to be written to the CDP
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid RT address, TR, modeCode, cdpNum, wordOffset, numOfWords
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_RT_MC_CDPReadWords(ADT_L0_UINT32 devID, ADT_L0_UINT32 rtAddr, ADT_L0_UINT32 tr, 
										 ADT_L0_UINT32 modeCode, ADT_L0_UINT32 cdpNum, ADT_L0_UINT32 wordOffset, 
										 ADT_L0_UINT32 numOfWords, ADT_L0_UINT32 *pWords) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, peCSR, data, sRtAddr;
	ADT_L0_UINT32 temp, rtCbOffset, rtFtPtr, rtSaCbPtr, startCdp, selectedCdp, i;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if ((rtAddr < 32) && (tr < 2) && (modeCode < 32)) {
		/* Read the PE CSR - this tells us if the board is configured for SINGLE or MULTIPLE RT */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &peCSR, 1);

		/* Determine RT CB offset based on SINGLE or MULTIPLE RT configuration */
		if (peCSR & 0x00000008) {  /* Multiple RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS + (rtAddr * ADT_L1_1553_RT_CB_SIZE);
		}
		else {  /* Single RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS;

			/* Read the RT CSR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_RT_CSR, &data, 1);

			/* Get the SRT address (bits 16-20) */
			sRtAddr = (data & 0x001F0000) >> 16;

			/* Verify that selected RT address matches the Single RT Address */
			if (rtAddr != sRtAddr) result = ADT_ERR_BAD_INPUT;
		}

		if (result == ADT_SUCCESS) {
			/* Get the RT Filter Table Pointer */
			temp = rtCbOffset + ADT_L1_1553_RT_CB_FTPTR;
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &rtFtPtr, 1);

			/* Get the RT MC Control Block Pointer */
			temp = rtFtPtr + (64 * 4) + (tr * 32 * 4) + (modeCode * 4);
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &rtSaCbPtr, 1);

			/* Check that the selected CDP number does not exceed the actual number of CDPs */
			temp = rtSaCbPtr + ADT_L1_1553_RT_SA_CB_APINUMCDP;
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &i, 1);
			if (cdpNum >= i) {
				result = ADT_ERR_BAD_INPUT;
			}

			if (result == ADT_SUCCESS) {
				/* Read the API starting CDP address */
				temp = rtSaCbPtr + ADT_L1_1553_RT_SA_CB_APISTARTCDP;
				result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &startCdp, 1);

				/* Follow NEXT pointers to find the selected CDP */
				i = 0;
				selectedCdp = startCdp;
				while (i < cdpNum) {
					result = ADT_L0_ReadMem32(devID, channelRegOffset + selectedCdp, &temp, 1);
					selectedCdp = temp;
					i++;
				}

				if (result == ADT_SUCCESS) {

					/* Read the words from the selected CDP offset */ 
					if ((numOfWords) && ((wordOffset + numOfWords) <= (ADT_L1_1553_CDP_SIZE/4))) 
						result = ADT_L0_ReadMem32(devID, channelRegOffset + selectedCdp + (wordOffset*4), pWords, numOfWords);
					else
						result = ADT_ERR_BAD_INPUT;
				}
			}
		}
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_1553_RT_SA_CDP_GetAddr
 *****************************************************************************/
/*! \brief Gets the byte address of a CDP for the RT/SA 
 *
 * This function gets the byte address of a CDP for the RT/SA.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param rtAddr is the RT address.
 * @param tr is transmit(1) or receive(0).
 * @param subAddr is the subaddress.
 * @param cdpNum is the index of the CDP to write.
 * @param pAddr is a pointer to store the address.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid RT address, TR, subaddress, cdpNum, numOfWords or wordOffset
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_RT_SA_CDP_GetAddr(ADT_L0_UINT32 devID, ADT_L0_UINT32 rtAddr, ADT_L0_UINT32 tr, 
										 ADT_L0_UINT32 subAddr, ADT_L0_UINT32 cdpNum, ADT_L0_UINT32 *pAddr) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, peCSR, data, sRtAddr;
	ADT_L0_UINT32 temp, rtCbOffset, rtFtPtr, rtSaCbPtr, startCdp, selectedCdp, i;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if ((rtAddr < 32) && (tr < 2) && (subAddr < 32)) {
		/* Read the PE CSR - this tells us if the board is configured for SINGLE or MULTIPLE RT */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &peCSR, 1);

		/* Determine RT CB offset based on SINGLE or MULTIPLE RT configuration */
		if (peCSR & 0x00000008) {  /* Multiple RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS + (rtAddr * ADT_L1_1553_RT_CB_SIZE);
		}
		else {  /* Single RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS;

			/* Read the RT CSR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_RT_CSR, &data, 1);

			/* Get the SRT address (bits 16-20) */
			sRtAddr = (data & 0x001F0000) >> 16;

			/* Verify that selected RT address matches the Single RT Address */
			if (rtAddr != sRtAddr) result = ADT_ERR_BAD_INPUT;
		}

		if (result == ADT_SUCCESS) {
			/* Get the RT Filter Table Pointer */
			temp = rtCbOffset + ADT_L1_1553_RT_CB_FTPTR;
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &rtFtPtr, 1);

			/* Get the RT SA Control Block Pointer */
			temp = rtFtPtr + (tr * 32 * 4) + (subAddr * 4);
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &rtSaCbPtr, 1);

			/* Check that the selected CDP number does not exceed the actual number of CDPs */
			temp = rtSaCbPtr + ADT_L1_1553_RT_SA_CB_APINUMCDP;
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &i, 1);
			if (cdpNum >= i) {
				result = ADT_ERR_BAD_INPUT;
			}

			if (result == ADT_SUCCESS) {
				/* Read the API starting CDP address */
				temp = rtSaCbPtr + ADT_L1_1553_RT_SA_CB_APISTARTCDP;
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
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_1553_RT_MC_CDP_GetAddr
 *****************************************************************************/
/*! \brief Gets the byte address of the CDP for the RT MODE CODE 
 *
 * This function gets the byte address of a CDP for the RT MODE CODE.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param rtAddr is the RT address.
 * @param tr is transmit(1) or receive(0).
 * @param modeCode is the mode code.
 * @param cdpNum is the index of the CDP to write.
 * @param pAddr is a pointer to store the address.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid RT address, TR, modeCode, cdpNum, wordOffset or numOfWords
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_RT_MC_CDP_GetAddr(ADT_L0_UINT32 devID, ADT_L0_UINT32 rtAddr, ADT_L0_UINT32 tr, 
										 ADT_L0_UINT32 modeCode, ADT_L0_UINT32 cdpNum, ADT_L0_UINT32 *pAddr) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, peCSR, data, sRtAddr;
	ADT_L0_UINT32 temp, rtCbOffset, rtFtPtr, rtSaCbPtr, startCdp, selectedCdp, i;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	if ((rtAddr < 32) && (tr < 2) && (modeCode < 32)) {
		/* Read the PE CSR - this tells us if the board is configured for SINGLE or MULTIPLE RT */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &peCSR, 1);

		/* Determine RT CB offset based on SINGLE or MULTIPLE RT configuration */
		if (peCSR & 0x00000008) {  /* Multiple RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS + (rtAddr * ADT_L1_1553_RT_CB_SIZE);
		}
		else {  /* Single RT configuration */
			rtCbOffset = ADT_L1_1553_RT_CTLBLOCKS;

			/* Read the RT CSR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_RT_CSR, &data, 1);

			/* Get the SRT address (bits 16-20) */
			sRtAddr = (data & 0x001F0000) >> 16;

			/* Verify that selected RT address matches the Single RT Address */
			if (rtAddr != sRtAddr) result = ADT_ERR_BAD_INPUT;
		}

		if (result == ADT_SUCCESS) {
			/* Get the RT Filter Table Pointer */
			temp = rtCbOffset + ADT_L1_1553_RT_CB_FTPTR;
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &rtFtPtr, 1);

			/* Get the RT MC Control Block Pointer */
			temp = rtFtPtr + (64 * 4) + (tr * 32 * 4) + (modeCode * 4);
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &rtSaCbPtr, 1);

			/* Check that the selected CDP number does not exceed the actual number of CDPs */
			temp = rtSaCbPtr + ADT_L1_1553_RT_SA_CB_APINUMCDP;
			result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &i, 1);
			if (cdpNum >= i) {
				result = ADT_ERR_BAD_INPUT;
			}

			if (result == ADT_SUCCESS) {
				/* Read the API starting CDP address */
				temp = rtSaCbPtr + ADT_L1_1553_RT_SA_CB_APISTARTCDP;
				result = ADT_L0_ReadMem32(devID, channelRegOffset + temp, &startCdp, 1);

				/* Follow NEXT pointers to find the selected CDP */
				i = 0;
				selectedCdp = startCdp;
				while (i < cdpNum) {
					result = ADT_L0_ReadMem32(devID, channelRegOffset + selectedCdp, &temp, 1);
					selectedCdp = temp;
					i++;
				}

				if (result == ADT_SUCCESS) {
					*pAddr = selectedCdp;
				}
			}
		}
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}


#ifdef __cplusplus
}
#endif
