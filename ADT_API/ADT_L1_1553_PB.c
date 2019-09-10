/******************************************************************************
 * FILE:			ADT_L1_1553_PB.c
 *
 * Copyright (c) 2007-2016, Alta Data Technologies LLC (ADT), All Rights Reserved.
 * Use of this software is subject to the ADT Software License (latest
 * revision), US Government or local laws and the ADT Terms and Conditions 
 * of Sale (latest revision).
 *
 * DESCRIPTION:
 *	Source file for Layer 1 API.
 *	Contains 1553 Playback functions.
 *
 *****************************************************************************/
/*! \file ADT_L1_1553_PB.c  
 *  \brief Source file containing 1553 Playback functions 
 */
#include <stdio.h>
#include "ADT_L1.h"

#define ADT_USE_BLOCK_RW

#ifdef __cplusplus
extern "C" {
#endif


/******************************************************************************
  FUNCTION:		ADT_L1_1553_PB_SetRtResponse
 *****************************************************************************/
/*! \brief Sets the playback RT response word.
 *
 * This function sets the playback RT response word.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param rtResp is the RT response word - each bit corresponds to an RT, if set then playback status word.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_PB_SetRtResponse(ADT_L0_UINT32 devID, ADT_L0_UINT32 rtResp) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Write the RT Response Word to the board */
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_PB_APIRTRESPWD, &rtResp, 1);

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_PB_GetRtResponse
 *****************************************************************************/
/*! \brief Gets the playback RT response word.
 *
 * This function gets the playback RT response word.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param pRtResp is the pointer to store the RT response word.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_PB_GetRtResponse(ADT_L0_UINT32 devID, ADT_L0_UINT32 *pRtResp) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Write the RT Response Word to the board */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PB_APIRTRESPWD, pRtResp, 1);

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_PB_Allocate
 *****************************************************************************/
/*! \brief Allocates and links playback packets 
 *
 * This function allocates and links playback packets.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param numPkts is the number of playback packets to allocate.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid number of packets
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_PB_Allocate(ADT_L0_UINT32 devID, ADT_L0_UINT32 numPkts) {

/* Optimized to use block R/W, primarily for ENET */
#ifdef ADT_USE_BLOCK_RW
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 startPBptr, prevPBptr, currPBptr, data, pkt;
	ADT_L0_UINT32 packet[ADT_L1_1553_PB_PKT_SIZE];
	ADT_L0_UINT32 memAvailable;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Clear the PB CSR */
	data = 0;
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_PB_CSR, &data, 1);

	if (numPkts != 0) {

		/* Check if there is enough memory to allocate */
		if ( ADT_L1_GetMemoryAvailable(devID, &memAvailable) == ADT_SUCCESS )
		{
			if((numPkts * ADT_L1_1553_PB_PKT_SIZE) > memAvailable)
			{
				result = ADT_ERR_MEM_MGT_NO_MEM;
				return( result);
			}
		}

		/* Allocate the first PB packet */
		result = ADT_L1_MemoryAlloc(devID, ADT_L1_1553_PB_PKT_SIZE, &startPBptr);
		if (result == ADT_SUCCESS) {
			/* Initialize the packet */
			memset(packet, 0, sizeof(packet));
			packet[ADT_L1_1553_PBP_NEXTPTR / 4] = startPBptr;
			result = ADT_L0_WriteMem32(devID, channelRegOffset + startPBptr + ADT_L1_1553_PBP_NEXTPTR, packet, ADT_L1_1553_PB_PKT_SIZE / 4);

			/* Set the STARTING, CURRENT, and TAIL PB PACKET PTRs */
			result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_PB_STARTPBPTR, &startPBptr, 1);
			result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_PB_CURRPBPTR, &startPBptr, 1);
			result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_PB_APITAILPTR, &startPBptr, 1);

			/* Allocate additional PB packets */
			pkt = 1;
			prevPBptr = startPBptr;
			while ((result == ADT_SUCCESS) && (pkt < numPkts)) {
				result = ADT_L1_MemoryAlloc(devID, ADT_L1_1553_PB_PKT_SIZE, &currPBptr);
				if (result == ADT_SUCCESS) {
					/* Initialize the packet */
					memset(packet, 0, sizeof(packet));
					packet[ADT_L1_1553_PBP_NEXTPTR / 4] = startPBptr;
					result = ADT_L0_WriteMem32(devID, channelRegOffset + currPBptr + ADT_L1_1553_PBP_NEXTPTR, packet, ADT_L1_1553_PB_PKT_SIZE / 4);

					/* Link previous packet to the new packet */
					result = ADT_L0_WriteMem32(devID, channelRegOffset + prevPBptr + ADT_L1_1553_PBP_NEXTPTR, &currPBptr, 1);

					/* Increment to next packet */
					pkt++;
					prevPBptr = currPBptr;

				}
			}
		}
	}
	else result = ADT_ERR_BAD_INPUT;
#else
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 startPBptr, prevPBptr, currPBptr, data, i, pkt;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Clear the PB CSR */
	data = 0;
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_PB_CSR, &data, 1);

	if (numPkts != 0) {
		/* Allocate the first PB packet */
		result = ADT_L1_MemoryAlloc(devID, ADT_L1_1553_PB_PKT_SIZE, &startPBptr);
		if (result == ADT_SUCCESS) {
			/* Initialize the packet */
			data = 0;
			result = ADT_L0_WriteMem32(devID, channelRegOffset + startPBptr + ADT_L1_1553_PBP_NEXTPTR, &startPBptr, 1);
			result = ADT_L0_WriteMem32(devID, channelRegOffset + startPBptr + ADT_L1_1553_PBP_CURROFFSET, &data, 1);
			result = ADT_L0_WriteMem32(devID, channelRegOffset + startPBptr + ADT_L1_1553_PBP_CONTROL, &data, 1);
			result = ADT_L0_WriteMem32(devID, channelRegOffset + startPBptr + ADT_L1_1553_PBP_TIME_HIGH, &data, 1);
			result = ADT_L0_WriteMem32(devID, channelRegOffset + startPBptr + ADT_L1_1553_PBP_TIME_LOW, &data, 1);
			for (i=0; i<63; i++)
				result = ADT_L0_WriteMem32(devID, channelRegOffset + startPBptr + ADT_L1_1553_PBP_DATASTART + i*4, &data, 1);

			/* Set the STARTING, CURRENT, and TAIL PB PACKET PTRs */
			result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_PB_STARTPBPTR, &startPBptr, 1);
			result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_PB_CURRPBPTR, &startPBptr, 1);
			result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_PB_APITAILPTR, &startPBptr, 1);

			/* Allocate additional PB packets */
			pkt = 1;
			prevPBptr = startPBptr;
			while ((result == ADT_SUCCESS) && (pkt < numPkts)) {
				result = ADT_L1_MemoryAlloc(devID, ADT_L1_1553_PB_PKT_SIZE, &currPBptr);
				if (result == ADT_SUCCESS) {
					/* Initialize the packet */
					data = 0;
					result = ADT_L0_WriteMem32(devID, channelRegOffset + currPBptr + ADT_L1_1553_PBP_NEXTPTR, &startPBptr, 1);
					result = ADT_L0_WriteMem32(devID, channelRegOffset + currPBptr + ADT_L1_1553_PBP_CURROFFSET, &data, 1);
					result = ADT_L0_WriteMem32(devID, channelRegOffset + currPBptr + ADT_L1_1553_PBP_CONTROL, &data, 1);
					result = ADT_L0_WriteMem32(devID, channelRegOffset + currPBptr + ADT_L1_1553_PBP_TIME_HIGH, &data, 1);
					result = ADT_L0_WriteMem32(devID, channelRegOffset + currPBptr + ADT_L1_1553_PBP_TIME_LOW, &data, 1);
					for (i=0; i<63; i++)
						result = ADT_L0_WriteMem32(devID, channelRegOffset + currPBptr + ADT_L1_1553_PBP_DATASTART + i*4, &data, 1);

					/* Link previous packet to the new packet */
					result = ADT_L0_WriteMem32(devID, channelRegOffset + prevPBptr + ADT_L1_1553_PBP_NEXTPTR, &currPBptr, 1);

					/* Increment to next packet */
					pkt++;
					prevPBptr = currPBptr;

				}
			}
		}
	}
	else result = ADT_ERR_BAD_INPUT;
#endif
	
	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_PB_Free
 *****************************************************************************/
/*! \brief Frees all playback packets for the device
 *
 * This function frees all playback packets for the device.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_PB_Free(ADT_L0_UINT32 devID) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 startPBptr, currPBptr, nextPBptr, data;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Make sure PB is stopped */
	result = ADT_L1_1553_PB_Stop(devID);

	/* Get the starting PB packet pointer */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PB_STARTPBPTR, &startPBptr, 1);
	currPBptr = startPBptr;
	nextPBptr = 1;

	/* Iterate through packets and free them until back to start ptr or zero */
	while ((result == ADT_SUCCESS) && (nextPBptr != startPBptr) && (nextPBptr != 0)) {
		/* Get the next PB packet pointer */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + currPBptr + ADT_L1_1553_PBP_NEXTPTR, &nextPBptr, 1);

		/* Free the current PB packet */
		result = ADT_L1_MemoryFree(devID, currPBptr, ADT_L1_1553_PB_PKT_SIZE);

		/* Move to next PB packet */
		currPBptr = nextPBptr;
	}

	/* Clear the PB registers */
	data = 0;
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_PB_STARTPBPTR, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_PB_CURRPBPTR, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_PB_CSR, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_PB_APITAILPTR, &data, 1);

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_PB_CDPWrite
 *****************************************************************************/
/*! \brief Converts a CDP to a PB packet and writes it to the PB buffer. 
 *
 * This function converts a CDP to a PB packet and writes it to the PB buffer.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param pCdp is a pointer to the CDP to write to the PB packet.
 * @param options are the packet control word options.
 *	- ADT_L1_1553_PBP_CONTROL_STOP		0x00000200
 *	- ADT_L1_1553_PBP_CONTROL_LED		0x00000100
 *	- ADT_L1_1553_PBP_CONTROL_TRGOUT	0x00000080
 *	- ADT_L1_1553_PBP_CONTROL_INT		0x00000040
 *  - ADT_L1_1553_API_PB_CDPWRITE_ATON	0x80000000
 * @param isFirstMsg is 1 for first message in playback or 0 for NOT first message.

 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_PB_BUFFER_FULL - PB buffer is full
	- \ref ADT_ERR_BAD_INPUT - Invalid CDP
	- \ref ADT_FAILURE - Completed with error
 *
 *  !!Change Record!!
 *  081121: RS:	Added Option Bit Selection for controlling Relative or Absolute Timing
 *				Look for ATflag for changes.
 *
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_PB_CDPWrite(ADT_L0_UINT32 devID, ADT_L1_1553_CDP *pCdp, ADT_L0_UINT32 options, ADT_L0_UINT32 isFirstMsg) {

/* Optimized to use block R/W, primarily for ENET */
#ifdef ADT_USE_BLOCK_RW
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, data, ATflag;
	ADT_L0_UINT32 tailPBptr, headPBptr, startPBptr, nextPBptr, wdCnt, msgType, pbpCtl, i, rtResp, rtAddr, rtMask;
	ADT_L0_UINT32 firstTimeHigh, firstTimeLow, timeHigh, timeLow;
	ADT_L0_UINT32 packet[ADT_L1_1553_PB_PKT_SIZE];

	memset(packet, 0, sizeof(packet));

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* For ENET use BLOCK R/W */
	if ((devID & 0xF0000000) == ADT_DEVID_BACKPLANETYPE_ENET)
	{
		/* Read the Root PB Registers - extract needed info */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PB_STARTPBPTR, packet, 16);
		startPBptr = packet[0];		/* Get the PE START PB PACKET PTR */
		headPBptr = packet[1];		/* Get the PE CURRENT PB PACKET PTR */
		rtResp = packet[14];		/* Get the RT Response Word */
		tailPBptr = packet[15];		/* Get the API TAIL PB PACKET PTR */
	}
	/* For non-ENET (PCI/PCIE) use individual R/W */
	else
	{
		/* Get the RT Response Word */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PB_APIRTRESPWD, &rtResp, 1);

		/* Get the API TAIL PB PACKET PTR */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PB_APITAILPTR, &tailPBptr, 1);

		/* Get the PE CURRENT PB PACKET PTR */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PB_CURRPBPTR, &headPBptr, 1);

		/* Get the PE START PB PACKET PTR */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PB_STARTPBPTR, &startPBptr, 1);
	}

	
	/* If this is the FIRST MESSAGE IN PLAYBACK, then it is OK to go past head pointer
	 * (on first msg head, tail, and start ptr will be the same).
	 * If AT is Off, we also want to STORE THE TIMESTAMP on the first message.  This allows us to convert
	 * all timestamps to time relative to start of playback (by subtracting the timestamp
	 * from the first message.
	 *
	 * If AT is on, then we do not adjust time and the playback clock will compare for absolute time values.
	 * The user must have previous set the Playback Control Clock Value to the desired time and possibly
	 * set the PB CSR Bit to Not Reset Clock to Zero.
	 */
	ATflag = options & ADT_L1_1553_API_PB_CDPWRITE_ATON;
	if (isFirstMsg) {
		/* Reset tail and head pointers to equal start pointer - First msg must be at start pointer */
		tailPBptr = startPBptr;
		result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_PB_APITAILPTR, &tailPBptr, 1);
		headPBptr = startPBptr;
		result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_PB_CURRPBPTR, &headPBptr, 1);

		if (!ATflag) {
		/* Save first msg timestamp */
			firstTimeHigh = pCdp->TimeHigh;
			firstTimeLow = pCdp->TimeLow;
			result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_PB_API1STTIMEHI, &pCdp->TimeHigh, 2);

			/* On first msg, add one 20ns tick.  We do this so PB will wait for ext clock on synchronous PB */
			pCdp->TimeLow++;
			if (pCdp->TimeLow < firstTimeLow) pCdp->TimeHigh++;  /* Handle carry */
		}
	}

	/* Else (not the first message in playback) . . .
	 * If the tail ptr equals the head ptr, then the buffer (PBCB Link List) is full - cannot write packet to buffer.
	 * If the buffer is not full, we want to read the first message timestamp so we can adjust the
	 * current message timestamp to be time from start of playback.
	 */
	else {
		if (tailPBptr == headPBptr) 
			result = ADT_ERR_BUFFER_FULL;
		else {
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PB_API1STTIMEHI, packet, 2);
			firstTimeHigh = packet[0];
			firstTimeLow = packet[1];
		}
	}

	if (result == ADT_SUCCESS) {
		/* PBCB Chain is Open, so Write items from the input CDP to the tail PB packet */

		if (!ATflag) {
			/* TIMESTAMP - Need to subtract time of first message in playback to make this time relative to start of playback. */
			/* Time High */
			timeHigh = pCdp->TimeHigh - firstTimeHigh;
			if (firstTimeLow > pCdp->TimeLow) timeHigh -= 1;  /* Borrow/Carry (or whatever we call this) */
			/* Time Low */
			timeLow = pCdp->TimeLow - firstTimeLow;
		}
		else /* Absolute Time - No Adjustment Needed */
		{
			timeHigh = pCdp->TimeHigh;
			timeLow = pCdp->TimeLow;
		}

		memset(packet, 0, sizeof(packet));
		packet[ADT_L1_1553_PBP_TIME_HIGH / 4] = timeHigh;
		packet[ADT_L1_1553_PBP_TIME_LOW / 4] = timeLow;

		msgType = pCdp->CDPStatusWord & 0xFC000000; /* Message type bits */
		wdCnt = pCdp->CDPStatusWord & 0x0000003F;   /* Number of data words */

		/* We need to limit the number of words to transmit so we don't trigger the 
		 * Terminal Fail-Safe Timer, cannot transmit longer than 800us (40 words, including CMD/STS).
		 * We will limit wdCnt to 37 data words.
		 */
		if (wdCnt > 37) wdCnt = 37;

		/* v2.5.4.1, 12 NOV 11, Rich Wade
		 * On good status words, limit response time to a max of 12us to avoid PB delay pushing a good
		 * response over 14us, where it becomes a late response.
		 */
		if ((pCdp->STS1info != 0xFFFFFFFF) && ((pCdp->STS1info & 0x00FF0000) > 0x00640000))
		{
			pCdp->STS1info &= 0xFF00FFFF;
			pCdp->STS1info |= 0x00640000;
		}
		if ((pCdp->STS2info != 0xFFFFFFFF) && ((pCdp->STS2info & 0x00FF0000) > 0x00640000))
		{
			pCdp->STS2info &= 0xFF00FFFF;
			pCdp->STS2info |= 0x00640000;
		}

		switch (msgType) {
			case 0x04000000:	/* Spurious Data */
			case 0x84000000:	/* Broadcast Spurious Data */
				pbpCtl = (options & 0x000003C0) | wdCnt;  /* Control word */
				packet[ADT_L1_1553_PBP_CONTROL / 4] = pbpCtl;

				/* Data words */
				for (i=0; i<wdCnt; i++) {
					if (i<32) data = pCdp->DATAinfo[i] & 0x80FFFFFF;  /* 0xF0FFFFFF; */
					/* We changed above mask because SPURIOUS DATA with TWO-BUS/SYNC error was causing
					 * playback to lose its mind and send the firmware to la-la land.
					 * (changed in v2.5.0.5)
					 */

					else data = 0x0000FFFF;
					if ((((data & 0x00FF0000) == 0x00010000) && (!ATflag)) && (!ATflag)) data &= 0xFF00FFFF;  /* Clear extra tick in gap time */
					packet[ADT_L1_1553_PBP_DATASTART / 4 + i] = data;
				}
				result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_CONTROL, &packet[ADT_L1_1553_PBP_CONTROL / 4],  6 + wdCnt + 4);
				break;

			case 0x08000000:	/* BCRT */
				/* Command word */
				data = pCdp->CMD1info & 0xF0FFFFFF;		
				data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
				packet[ADT_L1_1553_PBP_DATASTART / 4] = data;

				/* Data words */
				for (i=0; i<wdCnt; i++) {
					if (i<32) data = pCdp->DATAinfo[i] & 0xF000FFFF;
					else data = 0x0000FFFF;
					packet[ADT_L1_1553_PBP_DATASTART / 4 + 1 + i] = data;
				}

				/* Status word */
				rtAddr = (pCdp->CMD1info & 0x0000F800) >> 11;
				rtMask = 0x00000001 << rtAddr;
				if ((rtResp & rtMask) && (pCdp->STS1info != 0xFFFFFFFF)) {
					/* Include RT STATUS word */
					pbpCtl = (options & 0x000003C0) | (wdCnt + 2);
					data = pCdp->STS1info & 0xF0FFFFFF;
					data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
					packet[ADT_L1_1553_PBP_DATASTART / 4 + 1 + i] = data;
				}
				else {
					/* Do not include RT STATUS word */ 
					pbpCtl = (options & 0x000003C0) | (wdCnt + 1);
				}
				
				/* Control Word */
				packet[ADT_L1_1553_PBP_CONTROL / 4] = pbpCtl;

				result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_CONTROL, &packet[ADT_L1_1553_PBP_CONTROL / 4],  6 + wdCnt + 4);
				break;

			case 0x10000000:	/* RTBC */
				/* Command word */
				data = pCdp->CMD1info & 0xF0FFFFFF;		
				data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
				packet[ADT_L1_1553_PBP_DATASTART / 4] = data;

				/* Status word */
				rtAddr = (pCdp->CMD1info & 0x0000F800) >> 11;
				rtMask = 0x00000001 << rtAddr;
				if ((rtResp & rtMask) && (pCdp->STS1info != 0xFFFFFFFF)) {
					/* Include RT STATUS word */
					pbpCtl = (options & 0x000003C0) | (wdCnt + 2);
					data = pCdp->STS1info & 0xF0FFFFFF;
					data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
					packet[ADT_L1_1553_PBP_DATASTART / 4 + 1] = data;

					/* DATA words */
					for (i=0; i<wdCnt; i++) {
						if (i<32) data = pCdp->DATAinfo[i] & 0xF000FFFF;
						else data = 0x0000FFFF;
						packet[ADT_L1_1553_PBP_DATASTART / 4 + 2 + i] = data;
					}
				}
				else {
					/* Do not include RT STATUS word or DATA words (only CMD1) */ 
					pbpCtl = (options & 0x000003C0) | 0x00000001;
				}
				
				/* Control Word */
				packet[ADT_L1_1553_PBP_CONTROL / 4] = pbpCtl;

				result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_CONTROL, &packet[ADT_L1_1553_PBP_CONTROL / 4],  6 + wdCnt + 4);
				break;

			case 0x20000000:	/* RTRT */
				/* Command word 1 */
				data = pCdp->CMD1info & 0xF0FFFFFF;		
				data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
				packet[ADT_L1_1553_PBP_DATASTART / 4] = data;

				/* Command word 2 */
				data = pCdp->CMD2info & 0xF0FFFFFF;		
				data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
				if (((data & 0x00FF0000) == 0x00010000) && (!ATflag)) data &= 0xFF00FFFF;  /* Clear extra tick in gap time */
				packet[ADT_L1_1553_PBP_DATASTART / 4 + 1] = data;

				/* Status word 1 */
				rtAddr = (pCdp->CMD1info & 0x0000F800) >> 11;
				rtMask = 0x00000001 << rtAddr;
				if ((rtResp & rtMask) && (pCdp->STS1info != 0xFFFFFFFF)) {
					/* Include RT STATUS word 1 */
					pbpCtl = (options & 0x000003C0) | (wdCnt + 3);
					data = pCdp->STS1info & 0xF0FFFFFF;
					data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
					packet[ADT_L1_1553_PBP_DATASTART / 4 + 2] = data;

					/* DATA words */
					for (i=0; i<wdCnt; i++) {
						if (i<32) data = pCdp->DATAinfo[i] & 0xF000FFFF;
						else data = 0x0000FFFF;
						packet[ADT_L1_1553_PBP_DATASTART / 4 + 3 + i] = data;
					}

					/* Status word 2 */
					rtAddr = (pCdp->CMD2info & 0x0000F800) >> 11;
					rtMask = 0x00000001 << rtAddr;
					if ((rtResp & rtMask) && (pCdp->STS2info != 0xFFFFFFFF)) {
						/* Include RT STATUS word 2 */
						pbpCtl = (options & 0x000003C0) | (wdCnt + 4);
						data = pCdp->STS2info & 0xF0FFFFFF;
						data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
						packet[ADT_L1_1553_PBP_DATASTART / 4 + 3 + i] = data;
					}

				}
				else {
					/* Do not include RT STATUS words or DATA words (only CMD1 and CMD2) */ 
					pbpCtl = (options & 0x000003C0) | 0x00000002;
				}
				
				/* Control Word */
				packet[ADT_L1_1553_PBP_CONTROL / 4] = pbpCtl;

				result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_CONTROL, &packet[ADT_L1_1553_PBP_CONTROL / 4],  6 + wdCnt + 4);
				break;

			case 0x40000000:	/* MODE */
				/* Command word */
				data = pCdp->CMD1info & 0xF0FFFFFF;		
				data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
				packet[ADT_L1_1553_PBP_DATASTART / 4] = data;

				/* If TRANSMIT, we send STATUS (if enabled) and any data words (just like RTBC) */
				if (pCdp->CMD1info & 0x00000400) {
					/* Status word */
					rtAddr = (pCdp->CMD1info & 0x0000F800) >> 11;
					rtMask = 0x00000001 << rtAddr;
					if ((rtResp & rtMask) && (pCdp->STS1info != 0xFFFFFFFF)) {
						/* Include RT STATUS word */
						pbpCtl = (options & 0x000003C0) | (wdCnt + 2);
						data = pCdp->STS1info & 0xF0FFFFFF;
						data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
						packet[ADT_L1_1553_PBP_DATASTART / 4 + 1] = data;

						/* DATA words */
						for (i=0; i<wdCnt; i++) {
							if (i<32) data = pCdp->DATAinfo[i] & 0xF000FFFF;
							else data = 0x0000FFFF;
							packet[ADT_L1_1553_PBP_DATASTART / 4 + 2 + i] = data;
						}
					}
					else {
						/* Do not include RT STATUS word or DATA words (only CMD1) */ 
						pbpCtl = (options & 0x000003C0) | 0x00000001;
					}
				}
				/* If RECEIVE, we send any data words then send STATUS (if enabled) (just like BCRT) */
				else {
					/* Data words */
					for (i=0; i<wdCnt; i++) {
						if (i<32) data = pCdp->DATAinfo[i] & 0xF000FFFF;
						else data = 0x0000FFFF;
						packet[ADT_L1_1553_PBP_DATASTART / 4 + 1 + i] = data;
					}

					/* Status word */
					rtAddr = (pCdp->CMD1info & 0x0000F800) >> 11;
					rtMask = 0x00000001 << rtAddr;
					if ((rtResp & rtMask) && (pCdp->STS1info != 0xFFFFFFFF)) {
						/* Include RT STATUS word */
						pbpCtl = (options & 0x000003C0) | (wdCnt + 2);
						data = pCdp->STS1info & 0xF0FFFFFF;
						data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
						packet[ADT_L1_1553_PBP_DATASTART / 4 + 1 + i] = data;
					}
					else {
						/* Do not include RT STATUS word */ 
						pbpCtl = (options & 0x000003C0) | (wdCnt + 1);
					}
				}

				/* Control Word */
				packet[ADT_L1_1553_PBP_CONTROL / 4] = pbpCtl;

				result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_CONTROL, &packet[ADT_L1_1553_PBP_CONTROL / 4],  6 + wdCnt + 4);
				break;

			case 0x88000000:	/* BROADCAST BCRT */
				/* Command word */
				data = pCdp->CMD1info & 0xF0FFFFFF;		
				data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
				packet[ADT_L1_1553_PBP_DATASTART / 4] = data;

				/* Data words */
				for (i=0; i<wdCnt; i++) {
					if (i<32) data = pCdp->DATAinfo[i] & 0xF000FFFF;
					else data = 0x0000FFFF;
					packet[ADT_L1_1553_PBP_DATASTART / 4 + 1 + i] = data;
				}

				/* Do not include RT STATUS word */ 
				pbpCtl = (options & 0x000003C0) | (wdCnt + 1);
				
				/* Control Word */
				packet[ADT_L1_1553_PBP_CONTROL / 4] = pbpCtl;

				result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_CONTROL, &packet[ADT_L1_1553_PBP_CONTROL / 4],  6 + wdCnt + 4);
				break;

			case 0xA0000000:	/* BROADCAST RTRT */
				/* Command word 1 */
				data = pCdp->CMD1info & 0xF0FFFFFF;		
				data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
				packet[ADT_L1_1553_PBP_DATASTART / 4] = data;

				/* Command word 2 */
				data = pCdp->CMD2info & 0xF0FFFFFF;		
				data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
				if (((data & 0x00FF0000) == 0x00010000) && (!ATflag)) data &= 0xFF00FFFF;  /* Clear extra tick in gap time */
				packet[ADT_L1_1553_PBP_DATASTART / 4 + 1] = data;

				/* Status word 1 */
				rtAddr = (pCdp->CMD1info & 0x0000F800) >> 11;
				rtMask = 0x00000001 << rtAddr;
				if ((rtResp & rtMask) && (pCdp->STS1info != 0xFFFFFFFF)) {
					/* Include RT STATUS word 1 */
					pbpCtl = (options & 0x000003C0) | (wdCnt + 3);
					data = pCdp->STS1info & 0xF0FFFFFF;
					data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
					packet[ADT_L1_1553_PBP_DATASTART / 4 + 2] = data;

					/* DATA words */
					for (i=0; i<wdCnt; i++) {
						if (i<32) data = pCdp->DATAinfo[i] & 0xF000FFFF;
						else data = 0x0000FFFF;
						packet[ADT_L1_1553_PBP_DATASTART / 4 + 3 + i] = data;
					}
				}
				else {
					/* Do not include RT STATUS words or DATA words (only CMD1 and CMD2) */ 
					pbpCtl = (options & 0x000003C0) | 0x00000002;
				}
				
				/* Control Word */
				packet[ADT_L1_1553_PBP_CONTROL / 4] = pbpCtl;

				result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_CONTROL, &packet[ADT_L1_1553_PBP_CONTROL / 4],  6 + wdCnt + 4);
				break;

			case 0xC0000000:	/* BROADCAST MODE */
				/* Command word */
				data = pCdp->CMD1info & 0xF0FFFFFF;		
				data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
				packet[ADT_L1_1553_PBP_DATASTART / 4] = data;

				/* If TRANSMIT, then NO STATUS OR DATA */
				if (pCdp->CMD1info & 0x00000400) {
					/* Do not include RT STATUS word or DATA words (only CMD1) */ 
					pbpCtl = (options & 0x000003C0) | 0x00000001;
				}
				/* If RECEIVE, we send any data words (just like BCRT BROADCAST) */
				else {
					/* Data words */
					for (i=0; i<wdCnt; i++) {
						if (i<32) data = pCdp->DATAinfo[i] & 0xF000FFFF;
						else data = 0x0000FFFF;
						packet[ADT_L1_1553_PBP_DATASTART / 4 + 1 + i] = data;
					}

					/* Do not include RT STATUS word */ 
					pbpCtl = (options & 0x000003C0) | (wdCnt + 1);
				}

				/* Control Word */
				packet[ADT_L1_1553_PBP_CONTROL / 4] = pbpCtl;

				result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_CONTROL, &packet[ADT_L1_1553_PBP_CONTROL / 4],  6 + wdCnt + 4);
				break;

			default:			/* Unexpected Value */
				result = ADT_ERR_BAD_INPUT;
				break;
		}


		if (result == ADT_SUCCESS)
		{
			/* Get the NEXT PB PACKET PTR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_NEXTPTR, &nextPBptr, 1);

			/* Move API TAIL PB PACKET PTR to the NEXT packet */
			result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_PB_APITAILPTR, &nextPBptr, 1);
		}

	}
#else   /* NOT USING BLOCK RW */
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, data, ATflag;
	ADT_L0_UINT32 tailPBptr, headPBptr, startPBptr, nextPBptr, wdCnt, msgType, pbpCtl, i, rtResp, rtAddr, rtMask;
	ADT_L0_UINT32 firstTimeHigh, firstTimeLow, timeHigh, timeLow;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Get the RT Response Word */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PB_APIRTRESPWD, &rtResp, 1);

	/* Get the API TAIL PB PACKET PTR */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PB_APITAILPTR, &tailPBptr, 1);

	/* Get the PE CURRENT PB PACKET PTR */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PB_CURRPBPTR, &headPBptr, 1);

	/* Get the PE START PB PACKET PTR */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PB_STARTPBPTR, &startPBptr, 1);

	/* If this is the FIRST MESSAGE IN PLAYBACK, then it is OK to go past head pointer
	 * (on first msg head, tail, and start ptr will be the same).
	 * If AT is Off, we also want to STORE THE TIMESTAMP on the first message.  This allows us to convert
	 * all timestamps to time relative to start of playback (by subtracting the timestamp
	 * from the first message.
	 *
	 * If AT is on, then we do not adjust time and the playback clock will compare for absolute time values.
	 * The user must have previous set the Playback Control Clock Value to the desired time and possibly
	 * set the PB CSR Bit to Not Reset Clock to Zero.
	 */
	ATflag = options & ADT_L1_1553_API_PB_CDPWRITE_ATON;
	if (isFirstMsg) {
		/* Reset tail and head pointers to equal start pointer - First msg must be at start pointer */
		tailPBptr = startPBptr;
		result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_PB_APITAILPTR, &tailPBptr, 1);
		headPBptr = startPBptr;
		result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_PB_CURRPBPTR, &headPBptr, 1);

		if (!ATflag) {
		/* Save first msg timestamp */
			firstTimeHigh = pCdp->TimeHigh;
			firstTimeLow = pCdp->TimeLow;
			result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_PB_API1STTIMEHI, &firstTimeHigh, 1);
			result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_PB_API1STTIMELO, &firstTimeLow, 1);

			/* On first msg, add one 20ns tick.  We do this so PB will wait for ext clock on synchronous PB */
			pCdp->TimeLow++;
			if (pCdp->TimeLow < firstTimeLow) pCdp->TimeHigh++;  /* Handle carry */
		}
	}

	/* Else (not the first message in playback) . . .
	 * If the tail ptr equals the head ptr, then the buffer (PBCB Link List) is full - cannot write packet to buffer.
	 * If the buffer is not full, we want to read the first message timestamp so we can adjust the
	 * current message timestamp to be time from start of playback.
	 */
	else {
		if (tailPBptr == headPBptr) 
			result = ADT_ERR_BUFFER_FULL;
		else {
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PB_API1STTIMEHI, &firstTimeHigh, 1);
			result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PB_API1STTIMELO, &firstTimeLow, 1);
		}
	}

	if (result == ADT_SUCCESS) {
		/* PBCB Chain is Open, so Write items from the input CDP to the tail PB packet */

		if (!ATflag) {
			/* TIMESTAMP - Need to subtract time of first message in playback to make this time relative to start of playback. */
			/* Time High */
			timeHigh = pCdp->TimeHigh - firstTimeHigh;
			if (firstTimeLow > pCdp->TimeLow) timeHigh -= 1;  /* Borrow/Carry (or whatever we call this) */
			/* Time Low */
			timeLow = pCdp->TimeLow - firstTimeLow;
		}
		else /* Absolute Time - No Adjustment Needed */
		{
			timeHigh = pCdp->TimeHigh;
			timeLow = pCdp->TimeLow;
		}

		result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_TIME_HIGH, &timeHigh, 1);
		result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_TIME_LOW, &timeLow, 1);

		msgType = pCdp->CDPStatusWord & 0xFC000000; /* Message type bits */
		wdCnt = pCdp->CDPStatusWord & 0x0000003F;   /* Number of data words */

		/* We need to limit the number of words to transmit so we don't trigger the 
		 * Terminal Fail-Safe Timer, cannot transmit longer than 800us (40 words, including CMD/STS).
		 * We will limit wdCnt to 37 data words.
		 */
		if (wdCnt > 37) wdCnt = 37;

		/* v2.5.4.1, 12 NOV 11, Rich Wade
		 * On good status words, limit response time to a max of 12us to avoid PB delay pushing a good
		 * response over 14us, where it becomes a late response.
		 */
		if ((pCdp->STS1info != 0xFFFFFFFF) && ((pCdp->STS1info & 0x00FF0000) > 0x00640000))
		{
			pCdp->STS1info &= 0xFF00FFFF;
			pCdp->STS1info |= 0x00640000;
		}
		if ((pCdp->STS2info != 0xFFFFFFFF) && ((pCdp->STS2info & 0x00FF0000) > 0x00640000))
		{
			pCdp->STS2info &= 0xFF00FFFF;
			pCdp->STS2info |= 0x00640000;
		}

		switch (msgType) {
			case 0x04000000:	/* Spurious Data */
			case 0x84000000:	/* Broadcast Spurious Data */
				pbpCtl = (options & 0x000003C0) | wdCnt;  /* Control word */
				result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_CONTROL, &pbpCtl, 1);

				/* Data words */
				for (i=0; i<wdCnt; i++) {
					if (i<32) data = pCdp->DATAinfo[i] & 0x80FFFFFF;  /* 0xF0FFFFFF; */
					/* We changed above mask because SPURIOUS DATA with TWO-BUS/SYNC error was causing
					 * playback to lose its mind and send the firmware to la-la land.
					 * (changed in v2.5.0.5)
					 */

					else data = 0x0000FFFF;
					if ((((data & 0x00FF0000) == 0x00010000) && (!ATflag)) && (!ATflag)) data &= 0xFF00FFFF;  /* Clear extra tick in gap time */
					result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_DATASTART + i*4, &data, 1);
				}
				break;

			case 0x08000000:	/* BCRT */
				/* Command word */
				data = pCdp->CMD1info & 0xF0FFFFFF;		
				data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
				result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_DATASTART, &data, 1);

				/* Data words */
				for (i=0; i<wdCnt; i++) {
					if (i<32) data = pCdp->DATAinfo[i] & 0xF000FFFF;
					else data = 0x0000FFFF;
					result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_DATASTART + 4 + i*4, &data, 1);
				}

				/* Status word */
				rtAddr = (pCdp->CMD1info & 0x0000F800) >> 11;
				rtMask = 0x00000001 << rtAddr;
				if ((rtResp & rtMask) && (pCdp->STS1info != 0xFFFFFFFF)) {
					/* Include RT STATUS word */
					pbpCtl = (options & 0x000003C0) | (wdCnt + 2);
					data = pCdp->STS1info & 0xF0FFFFFF;
					data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
					result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_DATASTART + 4 + i*4, &data, 1);
				}
				else {
					/* Do not include RT STATUS word */ 
					pbpCtl = (options & 0x000003C0) | (wdCnt + 1);
				}
				
				/* Control Word */
				result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_CONTROL, &pbpCtl, 1);
				break;

			case 0x10000000:	/* RTBC */
				/* Command word */
				data = pCdp->CMD1info & 0xF0FFFFFF;		
				data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
				result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_DATASTART, &data, 1);

				/* Status word */
				rtAddr = (pCdp->CMD1info & 0x0000F800) >> 11;
				rtMask = 0x00000001 << rtAddr;
				if ((rtResp & rtMask) && (pCdp->STS1info != 0xFFFFFFFF)) {
					/* Include RT STATUS word */
					pbpCtl = (options & 0x000003C0) | (wdCnt + 2);
					data = pCdp->STS1info & 0xF0FFFFFF;
					data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
					result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_DATASTART + 4, &data, 1);

					/* DATA words */
					for (i=0; i<wdCnt; i++) {
						if (i<32) data = pCdp->DATAinfo[i] & 0xF000FFFF;
						else data = 0x0000FFFF;
						result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_DATASTART + 8 + i*4, &data, 1);
					}
				}
				else {
					/* Do not include RT STATUS word or DATA words (only CMD1) */ 
					pbpCtl = (options & 0x000003C0) | 0x00000001;
				}
				
				/* Control Word */
				result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_CONTROL, &pbpCtl, 1);
				break;

			case 0x20000000:	/* RTRT */
				/* Command word 1 */
				data = pCdp->CMD1info & 0xF0FFFFFF;		
				data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
				result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_DATASTART, &data, 1);

				/* Command word 2 */
				data = pCdp->CMD2info & 0xF0FFFFFF;		
				data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
				if (((data & 0x00FF0000) == 0x00010000) && (!ATflag)) data &= 0xFF00FFFF;  /* Clear extra tick in gap time */
				result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_DATASTART + 4, &data, 1);

				/* Status word 1 */
				rtAddr = (pCdp->CMD1info & 0x0000F800) >> 11;
				rtMask = 0x00000001 << rtAddr;
				if ((rtResp & rtMask) && (pCdp->STS1info != 0xFFFFFFFF)) {
					/* Include RT STATUS word 1 */
					pbpCtl = (options & 0x000003C0) | (wdCnt + 3);
					data = pCdp->STS1info & 0xF0FFFFFF;
					data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
					result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_DATASTART + 8, &data, 1);

					/* DATA words */
					for (i=0; i<wdCnt; i++) {
						if (i<32) data = pCdp->DATAinfo[i] & 0xF000FFFF;
						else data = 0x0000FFFF;
						result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_DATASTART + 12 + i*4, &data, 1);
					}

					/* Status word 2 */
					rtAddr = (pCdp->CMD2info & 0x0000F800) >> 11;
					rtMask = 0x00000001 << rtAddr;
					if ((rtResp & rtMask) && (pCdp->STS2info != 0xFFFFFFFF)) {
						/* Include RT STATUS word 2 */
						pbpCtl = (options & 0x000003C0) | (wdCnt + 4);
						data = pCdp->STS2info & 0xF0FFFFFF;
						data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
						result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_DATASTART + 12 + i*4, &data, 1);
					}

				}
				else {
					/* Do not include RT STATUS words or DATA words (only CMD1 and CMD2) */ 
					pbpCtl = (options & 0x000003C0) | 0x00000002;
				}
				
				/* Control Word */
				result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_CONTROL, &pbpCtl, 1);
				break;

			case 0x40000000:	/* MODE */
				/* Command word */
				data = pCdp->CMD1info & 0xF0FFFFFF;		
				data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
				result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_DATASTART, &data, 1);

				/* If TRANSMIT, we send STATUS (if enabled) and any data words (just like RTBC) */
				if (pCdp->CMD1info & 0x00000400) {
					/* Status word */
					rtAddr = (pCdp->CMD1info & 0x0000F800) >> 11;
					rtMask = 0x00000001 << rtAddr;
					if ((rtResp & rtMask) && (pCdp->STS1info != 0xFFFFFFFF)) {
						/* Include RT STATUS word */
						pbpCtl = (options & 0x000003C0) | (wdCnt + 2);
						data = pCdp->STS1info & 0xF0FFFFFF;
						data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
						result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_DATASTART + 4, &data, 1);

						/* DATA words */
						for (i=0; i<wdCnt; i++) {
							if (i<32) data = pCdp->DATAinfo[i] & 0xF000FFFF;
							else data = 0x0000FFFF;
							result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_DATASTART + 8 + i*4, &data, 1);
						}
					}
					else {
						/* Do not include RT STATUS word or DATA words (only CMD1) */ 
						pbpCtl = (options & 0x000003C0) | 0x00000001;
					}
				}
				/* If RECEIVE, we send any data words then send STATUS (if enabled) (just like BCRT) */
				else {
					/* Data words */
					for (i=0; i<wdCnt; i++) {
						if (i<32) data = pCdp->DATAinfo[i] & 0xF000FFFF;
						else data = 0x0000FFFF;
						result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_DATASTART + 4 + i*4, &data, 1);
					}

					/* Status word */
					rtAddr = (pCdp->CMD1info & 0x0000F800) >> 11;
					rtMask = 0x00000001 << rtAddr;
					if ((rtResp & rtMask) && (pCdp->STS1info != 0xFFFFFFFF)) {
						/* Include RT STATUS word */
						pbpCtl = (options & 0x000003C0) | (wdCnt + 2);
						data = pCdp->STS1info & 0xF0FFFFFF;
						data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
						result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_DATASTART + 4 + i*4, &data, 1);
					}
					else {
						/* Do not include RT STATUS word */ 
						pbpCtl = (options & 0x000003C0) | (wdCnt + 1);
					}
				}

				/* Control Word */
				result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_CONTROL, &pbpCtl, 1);
				break;

			case 0x88000000:	/* BROADCAST BCRT */
				/* Command word */
				data = pCdp->CMD1info & 0xF0FFFFFF;		
				data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
				result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_DATASTART, &data, 1);

				/* Data words */
				for (i=0; i<wdCnt; i++) {
					if (i<32) data = pCdp->DATAinfo[i] & 0xF000FFFF;
					else data = 0x0000FFFF;
					result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_DATASTART + 4 + i*4, &data, 1);
				}

				/* Do not include RT STATUS word */ 
				pbpCtl = (options & 0x000003C0) | (wdCnt + 1);
				
				/* Control Word */
				result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_CONTROL, &pbpCtl, 1);
				break;

			case 0xA0000000:	/* BROADCAST RTRT */
				/* Command word 1 */
				data = pCdp->CMD1info & 0xF0FFFFFF;		
				data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
				result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_DATASTART, &data, 1);

				/* Command word 2 */
				data = pCdp->CMD2info & 0xF0FFFFFF;		
				data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
				if (((data & 0x00FF0000) == 0x00010000) && (!ATflag)) data &= 0xFF00FFFF;  /* Clear extra tick in gap time */
				result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_DATASTART + 4, &data, 1);

				/* Status word 1 */
				rtAddr = (pCdp->CMD1info & 0x0000F800) >> 11;
				rtMask = 0x00000001 << rtAddr;
				if ((rtResp & rtMask) && (pCdp->STS1info != 0xFFFFFFFF)) {
					/* Include RT STATUS word 1 */
					pbpCtl = (options & 0x000003C0) | (wdCnt + 3);
					data = pCdp->STS1info & 0xF0FFFFFF;
					data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
					result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_DATASTART + 8, &data, 1);

					/* DATA words */
					for (i=0; i<wdCnt; i++) {
						if (i<32) data = pCdp->DATAinfo[i] & 0xF000FFFF;
						else data = 0x0000FFFF;
						result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_DATASTART + 12 + i*4, &data, 1);
					}
				}
				else {
					/* Do not include RT STATUS words or DATA words (only CMD1 and CMD2) */ 
					pbpCtl = (options & 0x000003C0) | 0x00000002;
				}
				
				/* Control Word */
				result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_CONTROL, &pbpCtl, 1);
				break;

			case 0xC0000000:	/* BROADCAST MODE */
				/* Command word */
				data = pCdp->CMD1info & 0xF0FFFFFF;		
				data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
				result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_DATASTART, &data, 1);

				/* If TRANSMIT, then NO STATUS OR DATA */
				if (pCdp->CMD1info & 0x00000400) {
					/* Do not include RT STATUS word or DATA words (only CMD1) */ 
					pbpCtl = (options & 0x000003C0) | 0x00000001;
				}
				/* If RECEIVE, we send any data words (just like BCRT BROADCAST) */
				else {
					/* Data words */
					for (i=0; i<wdCnt; i++) {
						if (i<32) data = pCdp->DATAinfo[i] & 0xF000FFFF;
						else data = 0x0000FFFF;
						result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_DATASTART + 4 + i*4, &data, 1);
					}

					/* Do not include RT STATUS word */ 
					pbpCtl = (options & 0x000003C0) | (wdCnt + 1);
				}

				/* Control Word */
				result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_CONTROL, &pbpCtl, 1);
				break;

			default:			/* Unexpected Value */
				result = ADT_ERR_BAD_INPUT;
				break;
		}


		if (result == ADT_SUCCESS)
		{
			/* Get the NEXT PB PACKET PTR */
			result = ADT_L0_ReadMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_NEXTPTR, &nextPBptr, 1);

			/* Move API TAIL PB PACKET PTR to the NEXT packet */
			result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_PB_APITAILPTR, &nextPBptr, 1);
		}

	}

#endif
	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_PB_Start
 *****************************************************************************/
/*! \brief Starts Playback operation for the device 
 *
 * This function starts Playback operation for the device.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_PB_Start(ADT_L0_UINT32 devID) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, pbCsr, data;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Set the CURRENT PB PACKET PTR to match the STARTING PB PACKET PTR */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PB_STARTPBPTR, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_PB_CURRPBPTR, &data, 1);

	/* Set the RUN bit in the PB CSR */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PB_CSR, &pbCsr, 1);
	pbCsr |= ADT_L1_1553_PB_CSR_PBRUN;
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_PB_CSR, &pbCsr, 1);

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_PB_Stop
 *****************************************************************************/
/*! \brief Stops Playback operation for the device 
 *
 * This function stops Playback operation for the device.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_PB_Stop(ADT_L0_UINT32 devID) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, pbCsr, data;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Clear the RUN bit in the PB CSR */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PB_CSR, &pbCsr, 1);
	pbCsr &= ~ADT_L1_1553_PB_CSR_PBRUN;
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_PB_CSR, &pbCsr, 1);

	/* Set the CURRENT PB PACKET PTR and API TAIL PTR to match the STARTING PB PACKET PTR */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PB_STARTPBPTR, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_PB_CURRPBPTR, &data, 1);
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_PB_APITAILPTR, &data, 1);

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_PB_IsRunning
 *****************************************************************************/
/*! \brief Determines if playback is running or stopped 
 *
 * This function determines if playback is running or stopped.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param pIsRunning is a pointer to store the result, 0=NotRunning, 1=Running.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_PB_IsRunning(ADT_L0_UINT32 devID, ADT_L0_UINT32 *pIsRunning) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, pbCsr, data;

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Read the PB CSR */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PB_CSR, &pbCsr, 1);

	/* Check the "PB RUN" bit (bit 0) to determine if user stopped the PB */
	if (pbCsr & ADT_L1_1553_PB_CSR_PBRUN)
		*pIsRunning = 1;
	else
		*pIsRunning = 0;

	/* Check the CURRENT PBP PTR to determine if the firmware stopped the PB */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PB_CURRPBPTR, &data, 1);
	if (data == 0x00000000)
		*pIsRunning = 0;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_1553_PB_CDPWrite_Opt
 *****************************************************************************/
/*! \brief Converts a CDP to a PB packet and writes it to the PB buffer. 
 *
 * This function converts a CDP to a PB packet and writes it to the PB buffer in an 
 * optimized fashion, it assumes this buffer is offline so does not check the 
 * head or tail pointers.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param pCdp is a pointer to the CDP to write to the PB packet.
 * @param options are the packet control word options.
 *	- ADT_L1_1553_PBP_CONTROL_STOP		0x00000200
 *	- ADT_L1_1553_PBP_CONTROL_LED		0x00000100
 *	- ADT_L1_1553_PBP_CONTROL_TRGOUT	0x00000080
 *	- ADT_L1_1553_PBP_CONTROL_INT		0x00000040
 *  - ADT_L1_1553_API_PB_CDPWRITE_ATON	0x80000000
 * @param pbCdpAddr the address of the PB CDP buffer to write this CDP message to.

 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_PB_BUFFER_FULL - PB buffer is full
	- \ref ADT_ERR_BAD_INPUT - Invalid CDP
	- \ref ADT_FAILURE - Completed with error
 *
 *  !!Change Record!!
 *  081121: RS:	Added Option Bit Selection for controlling Relative or Absolute Timing
 *				Look for ATflag for changes.
 *
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_PB_CDPWriteOptX(ADT_L0_UINT32 devID, ADT_L1_1553_CDP *pCdp, ADT_L0_UINT32 options, ADT_L0_UINT32 pbCdpAddr) {

	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, data, ATflag = 0;
	ADT_L0_UINT32 wdCnt, msgType, pbpCtl, i, rtResp = 0xFFFFFFFF, rtAddr, rtMask;
	ADT_L0_UINT32 packet[ADT_L1_1553_PB_PKT_SIZE];

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);
	
	if (result == ADT_SUCCESS) {
		/* PBCB Chain is Open, so Write items from the input CDP to the tail PB packet */

		memset(packet, 0, sizeof(packet));
		packet[ADT_L1_1553_PBP_TIME_HIGH >> 2] = pCdp->TimeHigh;
		packet[ADT_L1_1553_PBP_TIME_LOW >> 2] = pCdp->TimeLow;

		msgType = pCdp->CDPStatusWord & 0xFC000000; /* Message type bits */
		wdCnt = pCdp->CDPStatusWord & 0x0000003F;   /* Number of data words */

		/* We need to limit the number of words to transmit so we don't trigger the 
		 * Terminal Fail-Safe Timer, cannot transmit longer than 800us (40 words, including CMD/STS).
		 * We will limit wdCnt to 37 data words.
		 */
		if (wdCnt > 37) wdCnt = 37;

		/* v2.5.4.1, 12 NOV 11, Rich Wade
		 * On good status words, limit response time to a max of 12us to avoid PB delay pushing a good
		 * response over 14us, where it becomes a late response.
		 */
		if ((pCdp->STS1info != 0xFFFFFFFF) && ((pCdp->STS1info & 0x00FF0000) > 0x00640000))
		{
			pCdp->STS1info &= 0xFF00FFFF;
			pCdp->STS1info |= 0x00640000;
		}
		if ((pCdp->STS2info != 0xFFFFFFFF) && ((pCdp->STS2info & 0x00FF0000) > 0x00640000))
		{
			pCdp->STS2info &= 0xFF00FFFF;
			pCdp->STS2info |= 0x00640000;
		}

		switch (msgType) {
			case 0x04000000:	/* Spurious Data */
			case 0x84000000:	/* Broadcast Spurious Data */
				pbpCtl = (options & 0x000003C0) | wdCnt;  /* Control word */

				/* Data words */
				for (i=0; i<wdCnt; i++) {
					if (i<32) data = pCdp->DATAinfo[i] & 0x80FFFFFF;  /* 0xF0FFFFFF; */
					/* We changed above mask because SPURIOUS DATA with TWO-BUS/SYNC error was causing
					 * playback to lose its mind and send the firmware to la-la land.
					 * (changed in v2.5.0.5)
					 */

					else data = 0x0000FFFF;
					if ((((data & 0x00FF0000) == 0x00010000) && (!ATflag)) && (!ATflag)) data &= 0xFF00FFFF;  /* Clear extra tick in gap time */
					packet[ADT_L1_1553_PBP_DATASTART / 4 + i] = data;
				}
				break;

			case 0x08000000:	/* BCRT */
				/* Command word */
				data = pCdp->CMD1info & 0xF0FFFFFF;		
				data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
				packet[ADT_L1_1553_PBP_DATASTART / 4] = data;

				/* Data words */
				for (i=0; i<wdCnt; i++) {
					if (i<32) data = pCdp->DATAinfo[i] & 0xF000FFFF;
					else data = 0x0000FFFF;
					packet[ADT_L1_1553_PBP_DATASTART / 4 + 1 + i] = data;
				}

				/* Status word */
				rtAddr = (pCdp->CMD1info & 0x0000F800) >> 11;
				rtMask = 0x00000001 << rtAddr;
				if ((rtResp & rtMask) && (pCdp->STS1info != 0xFFFFFFFF)) {
					/* Include RT STATUS word */
					pbpCtl = (options & 0x000003C0) | (wdCnt + 2);
					data = pCdp->STS1info & 0xF0FFFFFF;
					data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
					packet[ADT_L1_1553_PBP_DATASTART / 4 + 1 + i] = data;
				}
				else {
					/* Do not include RT STATUS word */ 
					pbpCtl = (options & 0x000003C0) | (wdCnt + 1);
				}

				break;

			case 0x10000000:	/* RTBC */
				/* Command word */
				data = pCdp->CMD1info & 0xF0FFFFFF;		
				data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
				packet[ADT_L1_1553_PBP_DATASTART / 4] = data;

				/* Status word */
				rtAddr = (pCdp->CMD1info & 0x0000F800) >> 11;
				rtMask = 0x00000001 << rtAddr;
				if ((rtResp & rtMask) && (pCdp->STS1info != 0xFFFFFFFF)) {
					/* Include RT STATUS word */
					pbpCtl = (options & 0x000003C0) | (wdCnt + 2);
					data = pCdp->STS1info & 0xF0FFFFFF;
					data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
					packet[ADT_L1_1553_PBP_DATASTART / 4 + 1] = data;

					/* DATA words */
					for (i=0; i<wdCnt; i++) {
						if (i<32) data = pCdp->DATAinfo[i] & 0xF000FFFF;
						else data = 0x0000FFFF;
						packet[ADT_L1_1553_PBP_DATASTART / 4 + 2 + i] = data;
					}
				}
				else {
					/* Do not include RT STATUS word or DATA words (only CMD1) */ 
					pbpCtl = (options & 0x000003C0) | 0x00000001;
				}
				
				break;

			case 0x20000000:	/* RTRT */
				/* Command word 1 */
				data = pCdp->CMD1info & 0xF0FFFFFF;		
				data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
				packet[ADT_L1_1553_PBP_DATASTART / 4] = data;

				/* Command word 2 */
				data = pCdp->CMD2info & 0xF0FFFFFF;		
				data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
				if (((data & 0x00FF0000) == 0x00010000) && (!ATflag)) data &= 0xFF00FFFF;  /* Clear extra tick in gap time */
				packet[(ADT_L1_1553_PBP_DATASTART >> 2) + 1] = data;

				/* Status word 1 */
				rtAddr = (pCdp->CMD1info & 0x0000F800) >> 11;
				rtMask = 0x00000001 << rtAddr;
				if ((rtResp & rtMask) && (pCdp->STS1info != 0xFFFFFFFF)) {
					/* Include RT STATUS word 1 */
					pbpCtl = (options & 0x000003C0) | (wdCnt + 3);
					data = pCdp->STS1info & 0xF0FFFFFF;
					data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
					packet[ADT_L1_1553_PBP_DATASTART / 4 + 2] = data;

					/* DATA words */
					for (i=0; i<wdCnt; i++) {
						if (i<32) data = pCdp->DATAinfo[i] & 0xF000FFFF;
						else data = 0x0000FFFF;
						packet[ADT_L1_1553_PBP_DATASTART / 4 + 3 + i] = data;
					}

					/* Status word 2 */
					rtAddr = (pCdp->CMD2info & 0x0000F800) >> 11;
					rtMask = 0x00000001 << rtAddr;
					if ((rtResp & rtMask) && (pCdp->STS2info != 0xFFFFFFFF)) {
						/* Include RT STATUS word 2 */
						pbpCtl = (options & 0x000003C0) | (wdCnt + 4);
						data = pCdp->STS2info & 0xF0FFFFFF;
						data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
						packet[ADT_L1_1553_PBP_DATASTART / 4 + 3 + i] = data;
					}

				}
				else {
					/* Do not include RT STATUS words or DATA words (only CMD1 and CMD2) */ 
					pbpCtl = (options & 0x000003C0) | 0x00000002;
				}
				
				break;

			case 0x40000000:	/* MODE */
				/* Command word */
				data = pCdp->CMD1info & 0xF0FFFFFF;		
				data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
				packet[ADT_L1_1553_PBP_DATASTART / 4] = data;

				/* If TRANSMIT, we send STATUS (if enabled) and any data words (just like RTBC) */
				if (pCdp->CMD1info & 0x00000400) {
					/* Status word */
					rtAddr = (pCdp->CMD1info & 0x0000F800) >> 11;
					rtMask = 0x00000001 << rtAddr;
					if ((rtResp & rtMask) && (pCdp->STS1info != 0xFFFFFFFF)) {
						/* Include RT STATUS word */
						pbpCtl = (options & 0x000003C0) | (wdCnt + 2);
						data = pCdp->STS1info & 0xF0FFFFFF;
						data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
						packet[ADT_L1_1553_PBP_DATASTART / 4 + 1] = data;

						/* DATA words */
						for (i=0; i<wdCnt; i++) {
							if (i<32) data = pCdp->DATAinfo[i] & 0xF000FFFF;
							else data = 0x0000FFFF;
							packet[ADT_L1_1553_PBP_DATASTART / 4 + 2 + i] = data;
						}
					}
					else {
						/* Do not include RT STATUS word or DATA words (only CMD1) */ 
						pbpCtl = (options & 0x000003C0) | 0x00000001;
					}
				}
				/* If RECEIVE, we send any data words then send STATUS (if enabled) (just like BCRT) */
				else {
					/* Data words */
					for (i=0; i<wdCnt; i++) {
						if (i<32) data = pCdp->DATAinfo[i] & 0xF000FFFF;
						else data = 0x0000FFFF;
						packet[ADT_L1_1553_PBP_DATASTART / 4 + 1 + i] = data;
					}

					/* Status word */
					rtAddr = (pCdp->CMD1info & 0x0000F800) >> 11;
					rtMask = 0x00000001 << rtAddr;
					if ((rtResp & rtMask) && (pCdp->STS1info != 0xFFFFFFFF)) {
						/* Include RT STATUS word */
						pbpCtl = (options & 0x000003C0) | (wdCnt + 2);
						data = pCdp->STS1info & 0xF0FFFFFF;
						data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
						packet[ADT_L1_1553_PBP_DATASTART / 4 + 1 + i] = data;
					}
					else {
						/* Do not include RT STATUS word */ 
						pbpCtl = (options & 0x000003C0) | (wdCnt + 1);
					}
				}

				break;

			case 0x88000000:	/* BROADCAST BCRT */
				/* Command word */
				data = pCdp->CMD1info & 0xF0FFFFFF;		
				data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
				packet[ADT_L1_1553_PBP_DATASTART / 4] = data;

				/* Data words */
				for (i=0; i<wdCnt; i++) {
					if (i<32) data = pCdp->DATAinfo[i] & 0xF000FFFF;
					else data = 0x0000FFFF;
					packet[ADT_L1_1553_PBP_DATASTART / 4 + 1 + i] = data;
				}

				/* Do not include RT STATUS word */ 
				pbpCtl = (options & 0x000003C0) | (wdCnt + 1);
				
				break;

			case 0xA0000000:	/* BROADCAST RTRT */
				/* Command word 1 */
				data = pCdp->CMD1info & 0xF0FFFFFF;		
				data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
				packet[ADT_L1_1553_PBP_DATASTART / 4] = data;

				/* Command word 2 */
				data = pCdp->CMD2info & 0xF0FFFFFF;		
				data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
				if (((data & 0x00FF0000) == 0x00010000) && (!ATflag)) data &= 0xFF00FFFF;  /* Clear extra tick in gap time */
				packet[ADT_L1_1553_PBP_DATASTART / 4 + 1] = data;

				/* Status word 1 */
				rtAddr = (pCdp->CMD1info & 0x0000F800) >> 11;
				rtMask = 0x00000001 << rtAddr;
				if ((rtResp & rtMask) && (pCdp->STS1info != 0xFFFFFFFF)) {
					/* Include RT STATUS word 1 */
					pbpCtl = (options & 0x000003C0) | (wdCnt + 3);
					data = pCdp->STS1info & 0xF0FFFFFF;
					data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
					packet[ADT_L1_1553_PBP_DATASTART / 4 + 2] = data;

					/* DATA words */
					for (i=0; i<wdCnt; i++) {
						if (i<32) data = pCdp->DATAinfo[i] & 0xF000FFFF;
						else data = 0x0000FFFF;
						packet[ADT_L1_1553_PBP_DATASTART / 4 + 3 + i] = data;
					}
				}
				else {
					/* Do not include RT STATUS words or DATA words (only CMD1 and CMD2) */ 
					pbpCtl = (options & 0x000003C0) | 0x00000002;
				}

				break;

			case 0xC0000000:	/* BROADCAST MODE */
				/* Command word */
				data = pCdp->CMD1info & 0xF0FFFFFF;		
				data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
				packet[ADT_L1_1553_PBP_DATASTART >> 2] = data;

				/* If TRANSMIT, then NO STATUS OR DATA */
				if (pCdp->CMD1info & 0x00000400) {
					/* Do not include RT STATUS word or DATA words (only CMD1) */ 
					pbpCtl = (options & 0x000003C0) | 0x00000001;
				}
				/* If RECEIVE, we send any data words (just like BCRT BROADCAST) */
				else {
					/* Data words */
					for (i=0; i<wdCnt; i++) {
						if (i<32) data = pCdp->DATAinfo[i] & 0xF000FFFF;
						else data = 0x0000FFFF;
						packet[(ADT_L1_1553_PBP_DATASTART >> 2) + 1 + i] = data;
					}

					/* Do not include RT STATUS word */ 
					pbpCtl = (options & 0x000003C0) | (wdCnt + 1);
				}

				break;

			default:			/* Unexpected Value */
				result = ADT_ERR_BAD_INPUT;
				break;
		}

		if (result == ADT_SUCCESS)
		{
			/* Control Word */
			packet[ADT_L1_1553_PBP_CONTROL >> 2] = pbpCtl;
			result = ADT_L0_WriteMem32(devID, channelRegOffset + pbCdpAddr + ADT_L1_1553_PBP_CONTROL, &packet[ADT_L1_1553_PBP_CONTROL >> 2],  6 + wdCnt + 4);
		}

	}

	return result;
}

#ifdef __cplusplus
}
#endif


/******************************************************************************
  FUNCTION:		ADT_L1_1553_PB_CDPWriteOpt
 *****************************************************************************/
/*! \brief Converts a CDP to a PB packet and writes it to the PB buffer. 
 *
 * This function converts a CDP to a PB packet and writes it to the PB buffer.  This is
 * an optimized version which minimized device writes, it does not check for tail/head
 * pointer collision when writing, it is up to the calling code to deal with that.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param pCdp is a pointer to the CDP to write to the PB packet.
 * @param options are the packet control word options.
 *	- ADT_L1_1553_PBP_CONTROL_STOP		0x00000200
 *	- ADT_L1_1553_PBP_CONTROL_LED		0x00000100
 *	- ADT_L1_1553_PBP_CONTROL_TRGOUT	0x00000080
 *	- ADT_L1_1553_PBP_CONTROL_INT		0x00000040
 *  - ADT_L1_1553_API_PB_CDPWRITE_ATON	0x80000000
 * @param isFirstMsg is 1 for first message in playback or 0 for NOT first message.
 * @param pbData is an array of 6 elements used to carry playback information that is usually
 *        stored in card memory but to minimize unneccessary device access will be placed in this array.

 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_PB_BUFFER_FULL - PB buffer is full
	- \ref ADT_ERR_BAD_INPUT - Invalid CDP
	- \ref ADT_FAILURE - Completed with error
 *
 *  !!Change Record!!
 *  081121: RS:	Added Option Bit Selection for controlling Relative or Absolute Timing
 *				Look for ATflag for changes.
 *
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_1553_PB_CDPWriteOpt(ADT_L0_UINT32 devID, ADT_L1_1553_CDP *pCdp, ADT_L0_UINT32 options, ADT_L0_UINT32 isFirstMsg, ADT_L0_UINT32 *pbData) {

	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, data, ATflag;
	ADT_L0_UINT32 tailPBptr, wdCnt, msgType, pbpCtl, i, rtResp = 0xFFFFFFFF, rtAddr, rtMask;
	ADT_L0_UINT32 firstTimeHigh, firstTimeLow, timeHigh, timeLow;
	ADT_L0_UINT32 packet[ADT_L1_1553_PB_PKT_SIZE];
	ADT_L0_UINT32 allocBuffers;

	memset(packet, 0, sizeof(packet));

	/* Make sure this is a 1553 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_1553)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Check if pbData is allocated */
	if (pbData == NULL) {
		return(ADT_ERR_BAD_INPUT);
	}
	
	/* If this is the FIRST MESSAGE IN PLAYBACK, then it is OK to go past head pointer
	 * (on first msg head, tail, and start ptr will be the same).
	 * If AT is Off, we also want to STORE THE TIMESTAMP on the first message.  This allows us to convert
	 * all timestamps to time relative to start of playback (by subtracting the timestamp
	 * from the first message.
	 *
	 * If AT is on, then we do not adjust time and the playback clock will compare for absolute time values.
	 * The user must have previous set the Playback Control Clock Value to the desired time and possibly
	 * set the PB CSR Bit to Not Reset Clock to Zero.
	 */
	ATflag = options & ADT_L1_1553_API_PB_CDPWRITE_ATON;
	if (isFirstMsg) {
		allocBuffers = pbData[0];
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PB_APIRTRESPWD, &pbData[0], 1);
		rtResp = pbData[0];
		result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_1553_PB_STARTPBPTR, &pbData[1], 1);
		pbData[2] = pbData[1];

		/* Reset tail and head pointers to equal start pointer - First msg must be at start pointer */
		tailPBptr = pbData[1];
		result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_PB_APITAILPTR, &tailPBptr, 1);
		result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_PB_CURRPBPTR, &tailPBptr, 1);

		if (!ATflag) {
		/* Save first msg timestamp */
			firstTimeHigh = pCdp->TimeHigh;
			firstTimeLow = pCdp->TimeLow;
			pbData[3] = pCdp->TimeHigh;
			pbData[4] = pCdp->TimeLow;

			/* On first msg, add one 20ns tick.  We do this so PB will wait for ext clock on synchronous PB */
			pCdp->TimeLow++;
			if (pCdp->TimeLow < firstTimeLow) pCdp->TimeHigh++;  /* Handle carry */
		}

		pbData[5] = allocBuffers*ADT_L1_1553_PB_PKT_SIZE + pbData[1]; /* Calculate and set the maximum buffer address */
	}

	/* Else (not the first message in playback) . . .
	 * If the tail ptr equals the head ptr, then the buffer (PBCB Link List) is full - cannot write packet to buffer.
	 * If the buffer is not full, we want to read the first message timestamp so we can adjust the
	 * current message timestamp to be time from start of playback.
	 */
	else {
			rtResp = pbData[0];
			tailPBptr = pbData[2];

			firstTimeHigh = pbData[3];
			firstTimeLow = pbData[4];
	}

	if (result == ADT_SUCCESS) {
		/* PBCB Chain is Open, so Write items from the input CDP to the tail PB packet */

		if (!ATflag) {
			/* TIMESTAMP - Need to subtract time of first message in playback to make this time relative to start of playback. */
			/* Time High */
			timeHigh = pCdp->TimeHigh - firstTimeHigh;
			if (firstTimeLow > pCdp->TimeLow) timeHigh -= 1;  /* Borrow/Carry (or whatever we call this) */
			/* Time Low */
			timeLow = pCdp->TimeLow - firstTimeLow;
		}
		else /* Absolute Time - No Adjustment Needed */
		{
			timeHigh = pCdp->TimeHigh;
			timeLow = pCdp->TimeLow;
		}

		memset(packet, 0, sizeof(packet));
		packet[ADT_L1_1553_PBP_TIME_HIGH / 4] = timeHigh;
		packet[ADT_L1_1553_PBP_TIME_LOW / 4] = timeLow;

		msgType = pCdp->CDPStatusWord & 0xFC000000; /* Message type bits */
		wdCnt = pCdp->CDPStatusWord & 0x0000003F;   /* Number of data words */

		/* We need to limit the number of words to transmit so we don't trigger the 
		 * Terminal Fail-Safe Timer, cannot transmit longer than 800us (40 words, including CMD/STS).
		 * We will limit wdCnt to 37 data words.
		 */
		if (wdCnt > 37) wdCnt = 37;

		/* v2.5.4.1, 12 NOV 11, Rich Wade
		 * On good status words, limit response time to a max of 12us to avoid PB delay pushing a good
		 * response over 14us, where it becomes a late response.
		 */
		if ((pCdp->STS1info != 0xFFFFFFFF) && ((pCdp->STS1info & 0x00FF0000) > 0x00640000))
		{
			pCdp->STS1info &= 0xFF00FFFF;
			pCdp->STS1info |= 0x00640000;
		}
		if ((pCdp->STS2info != 0xFFFFFFFF) && ((pCdp->STS2info & 0x00FF0000) > 0x00640000))
		{
			pCdp->STS2info &= 0xFF00FFFF;
			pCdp->STS2info |= 0x00640000;
		}

		switch (msgType) {
			case 0x04000000:	/* Spurious Data */
			case 0x84000000:	/* Broadcast Spurious Data */
				pbpCtl = (options & 0x000003C0) | wdCnt;  /* Control word */
				packet[ADT_L1_1553_PBP_CONTROL / 4] = pbpCtl;

				/* Data words */
				for (i=0; i<wdCnt; i++) {
					if (i<32) data = pCdp->DATAinfo[i] & 0x80FFFFFF;  /* 0xF0FFFFFF; */
					/* We changed above mask because SPURIOUS DATA with TWO-BUS/SYNC error was causing
					 * playback to lose its mind and send the firmware to la-la land.
					 * (changed in v2.5.0.5)
					 */

					else data = 0x0000FFFF;
					if ((((data & 0x00FF0000) == 0x00010000) && (!ATflag)) && (!ATflag)) data &= 0xFF00FFFF;  /* Clear extra tick in gap time */
					packet[ADT_L1_1553_PBP_DATASTART / 4 + i] = data;
				}
				result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_CONTROL, &packet[ADT_L1_1553_PBP_CONTROL / 4],  6 + wdCnt + 4);
				break;

			case 0x08000000:	/* BCRT */
				/* Command word */
				data = pCdp->CMD1info & 0xF0FFFFFF;		
				data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
				packet[ADT_L1_1553_PBP_DATASTART / 4] = data;

				/* Data words */
				for (i=0; i<wdCnt; i++) {
					if (i<32) data = pCdp->DATAinfo[i] & 0xF000FFFF;
					else data = 0x0000FFFF;
					packet[ADT_L1_1553_PBP_DATASTART / 4 + 1 + i] = data;
				}

				/* Status word */
				rtAddr = (pCdp->CMD1info & 0x0000F800) >> 11;
				rtMask = 0x00000001 << rtAddr;
				if ((rtResp & rtMask) && (pCdp->STS1info != 0xFFFFFFFF)) {
					/* Include RT STATUS word */
					pbpCtl = (options & 0x000003C0) | (wdCnt + 2);
					data = pCdp->STS1info & 0xF0FFFFFF;
					data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
					packet[ADT_L1_1553_PBP_DATASTART / 4 + 1 + i] = data;
				}
				else {
					/* Do not include RT STATUS word */ 
					pbpCtl = (options & 0x000003C0) | (wdCnt + 1);
				}
				
				/* Control Word */
				packet[ADT_L1_1553_PBP_CONTROL / 4] = pbpCtl;

				result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_CONTROL, &packet[ADT_L1_1553_PBP_CONTROL / 4],  6 + wdCnt + 4);
				break;

			case 0x10000000:	/* RTBC */
				/* Command word */
				data = pCdp->CMD1info & 0xF0FFFFFF;		
				data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
				packet[ADT_L1_1553_PBP_DATASTART / 4] = data;

				/* Status word */
				rtAddr = (pCdp->CMD1info & 0x0000F800) >> 11;
				rtMask = 0x00000001 << rtAddr;
				if ((rtResp & rtMask) && (pCdp->STS1info != 0xFFFFFFFF)) {
					/* Include RT STATUS word */
					pbpCtl = (options & 0x000003C0) | (wdCnt + 2);
					data = pCdp->STS1info & 0xF0FFFFFF;
					data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
					packet[ADT_L1_1553_PBP_DATASTART / 4 + 1] = data;

					/* DATA words */
					for (i=0; i<wdCnt; i++) {
						if (i<32) data = pCdp->DATAinfo[i] & 0xF000FFFF;
						else data = 0x0000FFFF;
						packet[ADT_L1_1553_PBP_DATASTART / 4 + 2 + i] = data;
					}
				}
				else {
					/* Do not include RT STATUS word or DATA words (only CMD1) */ 
					pbpCtl = (options & 0x000003C0) | 0x00000001;
				}
				
				/* Control Word */
				packet[ADT_L1_1553_PBP_CONTROL / 4] = pbpCtl;

				result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_CONTROL, &packet[ADT_L1_1553_PBP_CONTROL / 4],  6 + wdCnt + 4);
				break;

			case 0x20000000:	/* RTRT */
				/* Command word 1 */
				data = pCdp->CMD1info & 0xF0FFFFFF;		
				data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
				packet[ADT_L1_1553_PBP_DATASTART / 4] = data;

				/* Command word 2 */
				data = pCdp->CMD2info & 0xF0FFFFFF;		
				data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
				if (((data & 0x00FF0000) == 0x00010000) && (!ATflag)) data &= 0xFF00FFFF;  /* Clear extra tick in gap time */
				packet[ADT_L1_1553_PBP_DATASTART / 4 + 1] = data;

				/* Status word 1 */
				rtAddr = (pCdp->CMD1info & 0x0000F800) >> 11;
				rtMask = 0x00000001 << rtAddr;
				if ((rtResp & rtMask) && (pCdp->STS1info != 0xFFFFFFFF)) {
					/* Include RT STATUS word 1 */
					pbpCtl = (options & 0x000003C0) | (wdCnt + 3);
					data = pCdp->STS1info & 0xF0FFFFFF;
					data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
					packet[ADT_L1_1553_PBP_DATASTART / 4 + 2] = data;

					/* DATA words */
					for (i=0; i<wdCnt; i++) {
						if (i<32) data = pCdp->DATAinfo[i] & 0xF000FFFF;
						else data = 0x0000FFFF;
						packet[ADT_L1_1553_PBP_DATASTART / 4 + 3 + i] = data;
					}

					/* Status word 2 */
					rtAddr = (pCdp->CMD2info & 0x0000F800) >> 11;
					rtMask = 0x00000001 << rtAddr;
					if ((rtResp & rtMask) && (pCdp->STS2info != 0xFFFFFFFF)) {
						/* Include RT STATUS word 2 */
						pbpCtl = (options & 0x000003C0) | (wdCnt + 4);
						data = pCdp->STS2info & 0xF0FFFFFF;
						data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
						packet[ADT_L1_1553_PBP_DATASTART / 4 + 3 + i] = data;
					}

				}
				else {
					/* Do not include RT STATUS words or DATA words (only CMD1 and CMD2) */ 
					pbpCtl = (options & 0x000003C0) | 0x00000002;
				}
				
				/* Control Word */
				packet[ADT_L1_1553_PBP_CONTROL / 4] = pbpCtl;

				result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_CONTROL, &packet[ADT_L1_1553_PBP_CONTROL / 4],  6 + wdCnt + 4);
				break;

			case 0x40000000:	/* MODE */
				/* Command word */
				data = pCdp->CMD1info & 0xF0FFFFFF;		
				data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
				packet[ADT_L1_1553_PBP_DATASTART / 4] = data;

				/* If TRANSMIT, we send STATUS (if enabled) and any data words (just like RTBC) */
				if (pCdp->CMD1info & 0x00000400) {
					/* Status word */
					rtAddr = (pCdp->CMD1info & 0x0000F800) >> 11;
					rtMask = 0x00000001 << rtAddr;
					if ((rtResp & rtMask) && (pCdp->STS1info != 0xFFFFFFFF)) {
						/* Include RT STATUS word */
						pbpCtl = (options & 0x000003C0) | (wdCnt + 2);
						data = pCdp->STS1info & 0xF0FFFFFF;
						data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
						packet[ADT_L1_1553_PBP_DATASTART / 4 + 1] = data;

						/* DATA words */
						for (i=0; i<wdCnt; i++) {
							if (i<32) data = pCdp->DATAinfo[i] & 0xF000FFFF;
							else data = 0x0000FFFF;
							packet[ADT_L1_1553_PBP_DATASTART / 4 + 2 + i] = data;
						}
					}
					else {
						/* Do not include RT STATUS word or DATA words (only CMD1) */ 
						pbpCtl = (options & 0x000003C0) | 0x00000001;
					}
				}
				/* If RECEIVE, we send any data words then send STATUS (if enabled) (just like BCRT) */
				else {
					/* Data words */
					for (i=0; i<wdCnt; i++) {
						if (i<32) data = pCdp->DATAinfo[i] & 0xF000FFFF;
						else data = 0x0000FFFF;
						packet[ADT_L1_1553_PBP_DATASTART / 4 + 1 + i] = data;
					}

					/* Status word */
					rtAddr = (pCdp->CMD1info & 0x0000F800) >> 11;
					rtMask = 0x00000001 << rtAddr;
					if ((rtResp & rtMask) && (pCdp->STS1info != 0xFFFFFFFF)) {
						/* Include RT STATUS word */
						pbpCtl = (options & 0x000003C0) | (wdCnt + 2);
						data = pCdp->STS1info & 0xF0FFFFFF;
						data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
						packet[ADT_L1_1553_PBP_DATASTART / 4 + 1 + i] = data;
					}
					else {
						/* Do not include RT STATUS word */ 
						pbpCtl = (options & 0x000003C0) | (wdCnt + 1);
					}
				}

				/* Control Word */
				packet[ADT_L1_1553_PBP_CONTROL / 4] = pbpCtl;

				result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_CONTROL, &packet[ADT_L1_1553_PBP_CONTROL / 4],  6 + wdCnt + 4);
				break;

			case 0x88000000:	/* BROADCAST BCRT */
				/* Command word */
				data = pCdp->CMD1info & 0xF0FFFFFF;		
				data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
				packet[ADT_L1_1553_PBP_DATASTART / 4] = data;

				/* Data words */
				for (i=0; i<wdCnt; i++) {
					if (i<32) data = pCdp->DATAinfo[i] & 0xF000FFFF;
					else data = 0x0000FFFF;
					packet[ADT_L1_1553_PBP_DATASTART / 4 + 1 + i] = data;
				}

				/* Do not include RT STATUS word */ 
				pbpCtl = (options & 0x000003C0) | (wdCnt + 1);
				
				/* Control Word */
				packet[ADT_L1_1553_PBP_CONTROL / 4] = pbpCtl;

				result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_CONTROL, &packet[ADT_L1_1553_PBP_CONTROL / 4],  6 + wdCnt + 4);
				break;

			case 0xA0000000:	/* BROADCAST RTRT */
				/* Command word 1 */
				data = pCdp->CMD1info & 0xF0FFFFFF;		
				data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
				packet[ADT_L1_1553_PBP_DATASTART / 4] = data;

				/* Command word 2 */
				data = pCdp->CMD2info & 0xF0FFFFFF;		
				data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
				if (((data & 0x00FF0000) == 0x00010000) && (!ATflag)) data &= 0xFF00FFFF;  /* Clear extra tick in gap time */
				packet[ADT_L1_1553_PBP_DATASTART / 4 + 1] = data;

				/* Status word 1 */
				rtAddr = (pCdp->CMD1info & 0x0000F800) >> 11;
				rtMask = 0x00000001 << rtAddr;
				if ((rtResp & rtMask) && (pCdp->STS1info != 0xFFFFFFFF)) {
					/* Include RT STATUS word 1 */
					pbpCtl = (options & 0x000003C0) | (wdCnt + 3);
					data = pCdp->STS1info & 0xF0FFFFFF;
					data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
					packet[ADT_L1_1553_PBP_DATASTART / 4 + 2] = data;

					/* DATA words */
					for (i=0; i<wdCnt; i++) {
						if (i<32) data = pCdp->DATAinfo[i] & 0xF000FFFF;
						else data = 0x0000FFFF;
						packet[ADT_L1_1553_PBP_DATASTART / 4 + 3 + i] = data;
					}
				}
				else {
					/* Do not include RT STATUS words or DATA words (only CMD1 and CMD2) */ 
					pbpCtl = (options & 0x000003C0) | 0x00000002;
				}
				
				/* Control Word */
				packet[ADT_L1_1553_PBP_CONTROL / 4] = pbpCtl;

				result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_CONTROL, &packet[ADT_L1_1553_PBP_CONTROL / 4],  6 + wdCnt + 4);
				break;

			case 0xC0000000:	/* BROADCAST MODE */
				/* Command word */
				data = pCdp->CMD1info & 0xF0FFFFFF;		
				data |= ADT_L1_1553_PBP_DATA_CMDSYNC;	/* Set Command Sync */
				packet[ADT_L1_1553_PBP_DATASTART / 4] = data;

				/* If TRANSMIT, then NO STATUS OR DATA */
				if (pCdp->CMD1info & 0x00000400) {
					/* Do not include RT STATUS word or DATA words (only CMD1) */ 
					pbpCtl = (options & 0x000003C0) | 0x00000001;
				}
				/* If RECEIVE, we send any data words (just like BCRT BROADCAST) */
				else {
					/* Data words */
					for (i=0; i<wdCnt; i++) {
						if (i<32) data = pCdp->DATAinfo[i] & 0xF000FFFF;
						else data = 0x0000FFFF;
						packet[ADT_L1_1553_PBP_DATASTART / 4 + 1 + i] = data;
					}

					/* Do not include RT STATUS word */ 
					pbpCtl = (options & 0x000003C0) | (wdCnt + 1);
				}

				/* Control Word */
				packet[ADT_L1_1553_PBP_CONTROL / 4] = pbpCtl;

				result = ADT_L0_WriteMem32(devID, channelRegOffset + tailPBptr + ADT_L1_1553_PBP_CONTROL, &packet[ADT_L1_1553_PBP_CONTROL / 4],  6 + wdCnt + 4);
				break;

			default:			/* Unexpected Value */
				result = ADT_ERR_BAD_INPUT;
				break;
		}


		if (result == ADT_SUCCESS)
		{
			/* Set the address for the next PBCDP */
			pbData[2] += ADT_L1_1553_PB_PKT_SIZE;
			if (pbData[2] >= pbData[5]) {
				pbData[2] = pbData[1];
			}
		}

	}

	return( result );
}
