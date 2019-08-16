/******************************************************************************
 * FILE:			ADT_L1_INT.c
 *
 * Copyright (c) 2007-2016, Alta Data Technologies LLC (ADT), All Rights Reserved.
 * Use of this software is subject to the ADT Software License (latest
 * revision), US Government or local laws and the ADT Terms and Conditions 
 * of Sale (latest revision).
 *
 * DESCRIPTION:
 *	Source file for Layer 1 API.
 *	Contains Interrupt functions.
 *
 *****************************************************************************/
/*! \file ADT_L1_INT.c  
 *  \brief Source file containing Interrupt functions 
 */
#include "ADT_L1.h"


#ifdef __cplusplus
extern "C" {
#endif


/******************************************************************************
  FUNCTION:		ADT_L1_INT_HandlerAttach
 *****************************************************************************/
/*! \brief Attaches an interrupt handler function for the device 
 *
 * This function attaches an interrupt handler function for the device.
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param pUserISR is a pointer to the user int handler function.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_INT_HandlerAttach(ADT_L0_UINT32 devID, ADT_L0_PUSERISR pUserISR, void * pUserData) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 channel, channelRegOffset, data;

	/* Get offset to the channel registers */
	result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);

	/* Clear Chan Interrupt Pending Location. Note this is not the interrupt pending bit in the PE status
	   word, this is the location used by the driver plugin module to indicate to a channel that an interrupt
	   is pending. The plugin module clears the interrupt pending bit in the PE Status word and write 0xFFFFFFFF
	   to the Chan Interrupt Pending Location so the channel will know it has an interrupt to process.
   */
	data = 0;
	result = ADT_L0_WriteMem32(devID, channelRegOffset + 0x0000000E0, &data, 1);

	/*Attached the interrupt handler */
	result = ADT_L0_AttachIntHandler(devID, channelRegOffset, pUserISR, pUserData);

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_INT_HandlerDetach
 *****************************************************************************/
/*! \brief Detaches an interrupt handler function for the device 
 *
 * This function attaches an interrupt handler function for the device.
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_INT_HandlerDetach(ADT_L0_UINT32 devID) {
	ADT_L0_UINT32 result = ADT_SUCCESS;

	result = ADT_L0_DetachIntHandler(devID);

	return( result );
}


#ifdef __cplusplus
}
#endif
