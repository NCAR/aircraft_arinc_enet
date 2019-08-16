/******************************************************************************
 * FILE:			ADT_L1_A429_INT.c
 *
 * Copyright (c) 2007-2016, Alta Data Technologies LLC (ADT), All Rights Reserved.
 * Use of this software is subject to the ADT Software License (latest
 * revision), US Government or local laws and the ADT Terms and Conditions
 * of Sale (latest revision).
 *
 * DESCRIPTION:
 *	Source file for Layer 1 API.
 *	Contains A429 interrupt functions.
 *
 *****************************************************************************/
/*! \file ADT_L1_A429_INT.c
 *  \brief Source file containing A429 Interrupt functions
 */
#include "ADT_L1.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
  FUNCTION:		ADT_L1_A429_INT_EnableInt
 *****************************************************************************/
/*! \brief Enables interrupts for the device.
 *
 * This function enables interrupts for the device.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @return
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_INT_EnableInt(ADT_L0_UINT32 devID) {
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

	/* Read PE root CSR */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_A429_PE_ROOT_CSR, &data, 1);

	/* Set HW INT bit */
	data |= ADT_L1_A429_PECSR_HWINTON;

	/* Write PE root CSR */
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_PE_ROOT_CSR, &data, 1);

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_A429_INT_DisableInt
 *****************************************************************************/
/*! \brief Disables interrupts the device.
 *
 * This function disables interrupts for the device.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @return
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_INT_DisableInt(ADT_L0_UINT32 devID) {
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

	/* Read PE root CSR */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_A429_PE_ROOT_CSR, &data, 1);

	/* Clear HW INT bit */
	data &= ~ADT_L1_A429_PECSR_HWINTON;

	/* Write PE root CSR */
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_PE_ROOT_CSR, &data, 1);

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_A429_INT_CheckDeviceIntPending
 *****************************************************************************/
/*! \brief Checks to see if an interrupt is pending for a given A429 device.
 *
 * This function checks to see if an interrupt is pending for a given A429 device.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param pIsIntPending is a pointer to store the result (0 = no int pending, 1 = int pending).
 * @return
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_INT_CheckDeviceIntPending(ADT_L0_UINT32 devID, ADT_L0_UINT32 *pIsIntPending) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 data;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Read the PE Status register - if bit 0 is set then there is an interrupt pending */
	data = 0;
	result = ADT_L1_ReadDeviceMem32(devID, ADT_L1_A429_PE_ROOT_STS, &data, 1);

	if (result == ADT_SUCCESS) {
		if (data & ADT_L1_A429_PESTS_INTPND)
			pIsIntPending[0] = 1;
		else
			pIsIntPending[0] = 0;
	}

	return( result );
}



/******************************************************************************
  FUNCTION:       ADT_L1_A429_INT_IQ_ReadEntry
 *****************************************************************************/
/*! \brief Reads one new entry from the interrupt queue.
 *
 * This function reads one new entry from the interrupt queue.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param pType is the pointer to store the IQ type/seqnum.
 * @param pInfo is the pointer to store the API INFO word.
 * @return
      - \ref ADT_SUCCESS - Completed without error
      - \ref ADT_FAILURE - Completed with error
      - \ref ADT_ERR_IQ_NO_NEW_ENTRY - No new entry in the queue
      - \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - not an A429 device
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_INT_IQ_ReadEntry(ADT_L0_UINT32 devID, ADT_L0_UINT32 *pType, ADT_L0_UINT32 *pInfo) {
      ADT_L0_UINT32 result = ADT_SUCCESS;
      ADT_L0_UINT32 channel, channelRegOffset;
      ADT_L0_UINT32 pApiLastIQEntry, pCurrIQEntry, pRXP, pTXP, pTXCB;

      /* Make sure this is an A429 device */
      if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
            return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

      /* Get offset to the channel registers */
      result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
      if (result != ADT_SUCCESS)
            return(result);

      /* Read the Current IQ pointer */
      result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_A429_IQ_CURR_IQP, &pCurrIQEntry, 1);

      /* Read the API Last IQ pointer */
      result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_A429_IQ_RESV_API, &pApiLastIQEntry, 1);

      /* If the two are not equal, then there are new entries in the queue */
      if (pApiLastIQEntry != pCurrIQEntry) {
            /* Read the IQ entry Type/seqnum */
            result = ADT_L0_ReadMem32(devID, channelRegOffset + pApiLastIQEntry + 4, pType, 1);

            /* Assign pInfo based on interrupt type */
            switch (*pType & 0xFFFF0000)
            {
            /* RxP-level interrupts */
            case 0x80000000:  /* RxP Mask Interrupt */
            case 0x40000000:  /* RxP Channel Interrupt */
            case 0x20000000:  /* RxP Multi-Channel Interrupt */
            case 0x00020000:  /* RxP Label CVT Interrupt */
                  /* Read the IQ entry RXP offset */
                  result = ADT_L0_ReadMem32(devID, channelRegOffset + pApiLastIQEntry + 8, &pRXP, 1);
                  /* Read the the Control Word from RXP - This is API Info for Int Processing */
                  result = ADT_L0_ReadMem32(devID, channelRegOffset + pRXP, pInfo, 1);
                  break;

            /* TxP-level interrupts */
            case 0x10000000:  /* TxP Complete Interrupt*/
                  /* Read the IQ entry TXP offset */
                  result = ADT_L0_ReadMem32(devID, channelRegOffset + pApiLastIQEntry + 8, &pTXP, 1);
                  /* Read the the API Info Bit Fields from TXP */
                  result = ADT_L0_ReadMem32(devID, channelRegOffset + pTXP + ADT_L1_A429_TXP_RESERVED, pInfo, 1);
                  break;

            /* Tx & TxCB interrupt */
            case 0x08000000:  /* TX Stop Interrupt */
                  /* Read the IQ entry TxCB offset */
                  result = ADT_L0_ReadMem32(devID, channelRegOffset + pApiLastIQEntry + 8, &pTXCB, 1);

                  /* Assign pInfo based on Aperiodic or TXCB Causing Int */
                  /* Aperiodic returns address value of zero */
                  if (pTXCB == 0)
                        /* If Aperiodic, Then Set API Info to All Ones - This is an impossible value for TxCB address */
                        *pInfo = 0xFFFFFFFF;
                  /* If not Aperiodic, Then Read the the API Message Number from TxCB */
                  else
                        result = ADT_L0_ReadMem32(devID, channelRegOffset + pTXCB + ADT_L1_A429_TXCB_APITXCBNUM, pInfo, 1);
                  break;

            case 0x02000000:  /* TxCB Complete Interrupt */
                  /* Read the IQ entry TXCB offset */
                  result = ADT_L0_ReadMem32(devID, channelRegOffset + pApiLastIQEntry + 8, &pTXCB, 1);
                  /* Read the the API Message Number from TXCB */
                  result = ADT_L0_ReadMem32(devID, channelRegOffset + pTXCB + ADT_L1_A429_TXCB_APITXCBNUM, pInfo, 1);
                  break;

            /* BIT Fail interrupts */
            case 0x00100000:  /* BIT FAIL Interrupt */
                  *pInfo = 0;
                  break;

            /* PB interrupt place holder - not implemented */
            /* case 0x00020000:     PB Interrupt */

            /* SG interrupt */
            case 0x00010000:  /* SG Interrupt - No Need for SGCB Num as Current API supports only one SGCB */
                  *pInfo = 0;
                  break;

            default:
                  break;
            }

            /* Update the API Last IQ pointer to the next entry */
            result = ADT_L0_ReadMem32(devID, channelRegOffset + pApiLastIQEntry, &pApiLastIQEntry, 1);
            result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_IQ_RESV_API, &pApiLastIQEntry, 1);
      }
      else result = ADT_ERR_IQ_NO_NEW_ENTRY;

      return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_A429_INT_IQ_ReadNewEntries
 *****************************************************************************/
/*! \brief Reads all new entries from the interrupt queue
 *
 * This function reads all new entries from the interrupt queue.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param maxNumEntries is the maximum number of entries to read (size of buffer).
 * @param pNumEntries is the pointer to store the number of messages read.
 * @param pType is the pointer to store the IQ type/seqnums (array sized by maxNumEntries).
 * @param pInfo is the pointer to store the API INFO words (array sized by maxNumEntries).
 * @return
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - invalid pointer
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_INT_IQ_ReadNewEntries(ADT_L0_UINT32 devID, ADT_L0_UINT32 maxNumEntries, ADT_L0_UINT32 *pNumEntries,
											ADT_L0_UINT32 *pType, ADT_L0_UINT32 *pInfo) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 status = ADT_SUCCESS;
	ADT_L0_UINT32 count;

	/* Ensure that the pointers passed in are not NULL */
	if ((pNumEntries != 0) && (pType != 0) && (pInfo != 0)) {
		count = 0;

		while ((count < maxNumEntries) && (status == ADT_SUCCESS)) {
			status = ADT_L1_A429_INT_IQ_ReadEntry(devID, &pType[count], &pInfo[count]);
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
  FUNCTION:		ADT_L1_A429_INT_IQ_ReadRawEntry
 *****************************************************************************/
/*! \brief Reads one new entry from the interrupt queue and returns the raw
 *   data from the queue.
 *
 * This function reads one new entry from the interrupt queue.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param pType is the pointer to store the IQ type/seqnum.
 * @param pIntData is the pointer to the data structure that caused the interrupt.
 * @return
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_FAILURE - Completed with error
	- \ref ADT_ERR_IQ_NO_NEW_ENTRY - No new entry in the queue
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_INT_IQ_ReadRawEntry(ADT_L0_UINT32 devID, ADT_L1_A429_INT *int_buffer) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;
	ADT_L0_UINT32 pApiLastIQEntry, pCurrIQEntry;

	/* Make sure this is a A429 channel */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Read the Current IQ pointer */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_A429_IQ_CURR_IQP, &pCurrIQEntry, 1);

	/* Read the API Last IQ pointer */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_A429_IQ_RESV_API, &pApiLastIQEntry, 1);

	/* If the two are not equal, then there are new entries in the queue */
	if (pApiLastIQEntry != pCurrIQEntry) {
		/* Read the IQ entry Type/seqnum */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + pApiLastIQEntry + 0, &int_buffer->NextPtr, 1);
		result = ADT_L0_ReadMem32(devID, channelRegOffset + pApiLastIQEntry + 4, &int_buffer->Type_SeqNum, 1);
		result = ADT_L0_ReadMem32(devID, channelRegOffset + pApiLastIQEntry + 8, &int_buffer->IntData, 1);

		/* Update the API Last IQ pointer to the next entry */
		result = ADT_L0_ReadMem32(devID, channelRegOffset + pApiLastIQEntry, &pApiLastIQEntry, 1);
		result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_IQ_RESV_API, &pApiLastIQEntry, 1);
	}
	else result = ADT_ERR_IQ_NO_NEW_ENTRY;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_A429_INT_IQ_ReadNewRawEntries
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
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_INT_IQ_ReadNewRawEntries(ADT_L0_UINT32 devID, ADT_L0_UINT32 maxNumEntries, ADT_L0_UINT32 *pNumEntries, ADT_L1_A429_INT *int_buffer ) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 status = ADT_SUCCESS;
	ADT_L0_UINT32 count;

	/* Ensure that the pointers passed in are not NULL */
	if ((pNumEntries != 0) && (int_buffer != 0)) {
		count = 0;

		while ((count < maxNumEntries) && (status == ADT_SUCCESS)) {
			status = ADT_L1_A429_INT_IQ_ReadRawEntry(devID, &int_buffer[count]);
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
  FUNCTION:		ADT_L1_A429_INT_SetIntSeqNum
 *****************************************************************************/
/*! \brief Writes the interrupt sequence number for the device.
 *
 * This function writes the interrupt sequence number for the channel.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param seqNum is the sequence number to write.
 * @return
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_INT_SetIntSeqNum(ADT_L0_UINT32 devID, ADT_L0_UINT32 seqNum) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Write the interrupt sequence number */
	result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_A429_IQ_SEQNUM, &seqNum, 1);

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_A429_INT_GetIntSeqNum
 *****************************************************************************/
/*! \brief Reads the interrupt sequence number for the device.
 *
 * This function reads the interrupt sequence number for the device.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param pSeqNum is the pointer to store the sequence number.
 * @return
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - not an A429 device
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_A429_INT_GetIntSeqNum(ADT_L0_UINT32 devID, ADT_L0_UINT32 *pSeqNum) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset;

	/* Make sure this is an A429 device */
	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_A429)
		return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
	if (result != ADT_SUCCESS)
		return(result);

	/* Read the interrupt sequence number */
	result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_A429_IQ_SEQNUM, pSeqNum, 1);

	return( result );
}



#ifdef __cplusplus
}
#endif
