/******************************************************************************
 * FILE:			ADT_L1_BIT.c
 *
 * Copyright (c) 2007-2016, Alta Data Technologies LLC (ADT), All Rights Reserved.
 * Use of this software is subject to the ADT Software License (latest
 * revision), US Government or local laws and the ADT Terms and Conditions 
 * of Sale (latest revision).
 *
 * DESCRIPTION:
 *	Source file for Layer 1 API.
 *	Contains Built-In-Test functions.
 *
 *****************************************************************************/
/*! \file ADT_L1_BIT.c  
 *  \brief Source file containing Built-In-Test functions 
 */
#include "ADT_L1.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
  FUNCTION:		ADT_L1_BIT_MemoryTest
 *****************************************************************************/
/*! \brief Tests memory on the device 
 *
 * This function performs a memory test on the selected range of memory.
 * The memory is tested five times with different values (0x55555555, 
 * 0xAAAAAAAA, 0xFFFFFFFF, value=offset, and 0x00000000) where first the
 * value is written to the entire range of memory and then each word is
 * read and checked to see if it matches the expected value.  If a failure
 * occurs, the function provides the memory offset, the expected value,
 * and the actual value read.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param start is the starting offset (BYTE offset)
 * @param end is the ending offset (BYTE offset)
 * @param addr is a pointer to store the memory offset (BYTE offset) on failure
 * @param exp is a pointer to store the expected value on failure
 * @param act is a pointer to store the actual value on failure
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid device number
	- \ref ADT_ERR_MEM_TEST_FAIL - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_BIT_MemoryTest(ADT_L0_UINT32 devID, ADT_L0_UINT32 start, ADT_L0_UINT32 end, ADT_L0_UINT32 *addr, ADT_L0_UINT32 *exp, ADT_L0_UINT32 *act) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 i, exp_value;
	ADT_L0_UINT32 datablock[ADT_RW_MEM_MAX_SIZE];
	ADT_L0_UINT32 j, k;

	if (start > end) {
		/* Bad start/end values */
		result = ADT_ERR_BAD_INPUT;
	}
	else {

		if (result == ADT_SUCCESS) {
			/* Test with 0x55555555 pattern */
			exp_value = 0x55555555;
			memset(datablock, 0, sizeof(datablock));
			j = 0;
			for (i=start; i<end; i+=4) {
				if (j < 100) {
					datablock[j] = exp_value;
					j++;
				}
				else {  /* Write data in blocks of 100 words */
					(void) ADT_L1_WriteDeviceMem32(devID, i - (j * 4), datablock, j);

					j = 0;
					memset(datablock, 0, sizeof(datablock));
					datablock[j] = exp_value;
					j++;
				}
			}
			if (j != 0) {  /* Write last block if total # words is not evenly divisible by 100 */
				(void) ADT_L1_WriteDeviceMem32(devID, i - (j * 4), datablock, j);
				j = 0;
			}

			memset(datablock, 0, sizeof(datablock));
			j = 0;
			for (i=start; i<end; i+=4) {
				if (j < 100) {
					j++;
				}
				else {
					memset(datablock, 0, sizeof(datablock));
					(void) ADT_L1_ReadDeviceMem32(devID, i - (j * 4), datablock, j);
					for (k=0; k<j; k++) {
						if (datablock[k] != exp_value) {
							result = ADT_ERR_MEM_TEST_FAIL;
							*addr = i - (j * 4) + (k * 4);
							*exp = exp_value;
							*act = datablock[k];
							k = j;
							i = end;
						}
					}
					j = 0;
				}
			}
			if (j != 0) {  /* Read last block if total # words is not evenly divisible by 100 */
				memset(datablock, 0, sizeof(datablock));
				(void) ADT_L1_ReadDeviceMem32(devID, i - (j * 4), datablock, j);
					for (k=0; k<j; k++) {
						if (datablock[k] != exp_value) {
							result = ADT_ERR_MEM_TEST_FAIL;
							*addr = i - (j * 4) + (k * 4);
							*exp = exp_value;
							*act = datablock[k];
							k = j;
							i = end;
						}
					}
				j = 0;
			}
		}

		if (result == ADT_SUCCESS) {
			/* Test with 0xAAAAAAAA pattern */
			exp_value = 0xAAAAAAAA;
			memset(datablock, 0, sizeof(datablock));
			j = 0;
			for (i=start; i<end; i+=4) {
				if (j < 100) {
					datablock[j] = exp_value;
					j++;
				}
				else {  /* Write data in blocks of 100 words */
					(void) ADT_L1_WriteDeviceMem32(devID, i - (j * 4), datablock, j);

					j = 0;
					memset(datablock, 0, sizeof(datablock));
					datablock[j] = exp_value;
					j++;
				}
			}
			if (j != 0) {  /* Write last block if total # words is not evenly divisible by 100 */
				(void) ADT_L1_WriteDeviceMem32(devID, i - (j * 4), datablock, j);
				j = 0;
			}

			memset(datablock, 0, sizeof(datablock));
			j = 0;
			for (i=start; i<end; i+=4) {
				if (j < 100) {
					j++;
				}
				else {
					memset(datablock, 0, sizeof(datablock));
					(void) ADT_L1_ReadDeviceMem32(devID, i - (j * 4), datablock, j);
					for (k=0; k<j; k++) {
						if (datablock[k] != exp_value) {
							result = ADT_ERR_MEM_TEST_FAIL;
							*addr = i - (j * 4) + (k * 4);
							*exp = exp_value;
							*act = datablock[k];
							k = j;
							i = end;
						}
					}
					j = 0;
				}
			}
			if (j != 0) {  /* Read last block if total # words is not evenly divisible by 100 */
				memset(datablock, 0, sizeof(datablock));
				(void) ADT_L1_ReadDeviceMem32(devID, i - (j * 4), datablock, j);
					for (k=0; k<j; k++) {
						if (datablock[k] != exp_value) {
							result = ADT_ERR_MEM_TEST_FAIL;
							*addr = i - (j * 4) + (k * 4);
							*exp = exp_value;
							*act = datablock[k];
							k = j;
							i = end;
						}
					}
				j = 0;
			}
		}

		if (result == ADT_SUCCESS) {
			/* Test with 0xFFFFFFFF pattern */
			exp_value = 0xFFFFFFFF;
			memset(datablock, 0, sizeof(datablock));
			j = 0;
			for (i=start; i<end; i+=4) {
				if (j < 100) {
					datablock[j] = exp_value;
					j++;
				}
				else {  /* Write data in blocks of 100 words */
					(void) ADT_L1_WriteDeviceMem32(devID, i - (j * 4), datablock, j);

					j = 0;
					memset(datablock, 0, sizeof(datablock));
					datablock[j] = exp_value;
					j++;
				}
			}
			if (j != 0) {  /* Write last block if total # words is not evenly divisible by 100 */
				(void) ADT_L1_WriteDeviceMem32(devID, i - (j * 4), datablock, j);
				j = 0;
			}

			memset(datablock, 0, sizeof(datablock));
			j = 0;
			for (i=start; i<end; i+=4) {
				if (j < 100) {
					j++;
				}
				else {
					memset(datablock, 0, sizeof(datablock));
					(void) ADT_L1_ReadDeviceMem32(devID, i - (j * 4), datablock, j);
					for (k=0; k<j; k++) {
						if (datablock[k] != exp_value) {
							result = ADT_ERR_MEM_TEST_FAIL;
							*addr = i - (j * 4) + (k * 4);
							*exp = exp_value;
							*act = datablock[k];
							k = j;
							i = end;
						}
					}
					j = 0;
				}
			}
			if (j != 0) {  /* Read last block if total # words is not evenly divisible by 100 */
				memset(datablock, 0, sizeof(datablock));
				(void) ADT_L1_ReadDeviceMem32(devID, i - (j * 4), datablock, j);
					for (k=0; k<j; k++) {
						if (datablock[k] != exp_value) {
							result = ADT_ERR_MEM_TEST_FAIL;
							*addr = i - (j * 4) + (k * 4);
							*exp = exp_value;
							*act = datablock[k];
							k = j;
							i = end;
						}
					}
				j = 0;
			}
		}

		if (result == ADT_SUCCESS) {
			/* Test with Data = Offset pattern */
			memset(datablock, 0, sizeof(datablock));
			j = 0;
			for (i=start; i<end; i+=4) {
				exp_value = i;
				if (j < 100) {
					datablock[j] = exp_value;
					j++;
				}
				else {  /* Write data in blocks of 100 words */
					(void) ADT_L1_WriteDeviceMem32(devID, i - (j * 4), datablock, j);

					j = 0;
					memset(datablock, 0, sizeof(datablock));
					datablock[j] = exp_value;
					j++;
				}
			}
			if (j != 0) {  /* Write last block if total # words is not evenly divisible by 100 */
				(void) ADT_L1_WriteDeviceMem32(devID, i - (j * 4), datablock, j);
				j = 0;
			}

			memset(datablock, 0, sizeof(datablock));
			j = 0;
			for (i=start; i<end; i+=4) {
				if (j < 100) {
					j++;
				}
				else {
					memset(datablock, 0, sizeof(datablock));
					(void) ADT_L1_ReadDeviceMem32(devID, i - (j * 4), datablock, j);
					for (k=0; k<j; k++) {
						exp_value = i - (j * 4) + (k * 4);
						if (datablock[k] != exp_value) {
							result = ADT_ERR_MEM_TEST_FAIL;
							*addr = exp_value;
							*exp = exp_value;
							*act = datablock[k];
							k = j;
							i = end;
						}
					}
					j = 0;
				}
			}
			if (j != 0) {  /* Read last block if total # words is not evenly divisible by 100 */
				memset(datablock, 0, sizeof(datablock));
				(void) ADT_L1_ReadDeviceMem32(devID, i - (j * 4), datablock, j);
				for (k=0; k<j; k++) {
					exp_value = i - (j * 4) + (k * 4);
					if (datablock[k] != exp_value) {
						result = ADT_ERR_MEM_TEST_FAIL;
						*addr = exp_value;
						*exp = exp_value;
						*act = datablock[k];
						k = j;
						i = end;
					}
				}
				j = 0;
			}
		}

		if (result == ADT_SUCCESS) {
			/* Test with 0x00000000 pattern */
			exp_value = 0x00000000;
			memset(datablock, 0, sizeof(datablock));
			j = 0;
			for (i=start; i<end; i+=4) {
				if (j < 100) {
					datablock[j] = exp_value;
					j++;
				}
				else {  /* Write data in blocks of 100 words */
					(void) ADT_L1_WriteDeviceMem32(devID, i - (j * 4), datablock, j);

					j = 0;
					memset(datablock, 0, sizeof(datablock));
					datablock[j] = exp_value;
					j++;
				}
			}
			if (j != 0) {  /* Write last block if total # words is not evenly divisible by 100 */
				(void) ADT_L1_WriteDeviceMem32(devID, i - (j * 4), datablock, j);
				j = 0;
			}

			memset(datablock, 0, sizeof(datablock));
			j = 0;
			for (i=start; i<end; i+=4) {
				if (j < 100) {
					j++;
				}
				else {
					memset(datablock, 0xFF, sizeof(datablock));
					(void) ADT_L1_ReadDeviceMem32(devID, i - (j * 4), datablock, j);
					for (k=0; k<j; k++) {
						if (datablock[k] != exp_value) {
							result = ADT_ERR_MEM_TEST_FAIL;
							*addr = i - (j * 4) + (k * 4);
							*exp = exp_value;
							*act = datablock[k];
							k = j;
							i = end;
						}
					}
					j = 0;
				}
			}
			if (j != 0) {  /* Read last block if total # words is not evenly divisible by 100 */
				memset(datablock, 0xFF, sizeof(datablock));
				(void) ADT_L1_ReadDeviceMem32(devID, i - (j * 4), datablock, j);
					for (k=0; k<j; k++) {
						if (datablock[k] != exp_value) {
							result = ADT_ERR_MEM_TEST_FAIL;
							*addr = i - (j * 4) + (k * 4);
							*exp = exp_value;
							*act = datablock[k];
							k = j;
							i = end;
						}
					}
				j = 0;
			}
		}

	}

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_BIT_InitiatedBIT
 *****************************************************************************/
/*! \brief Runs IBIT on the device 
 *
 * This function performs INITIATED BIT.  This test should only be run when the
 * device is in a non-operational safe state - this test will overwrite data
 * structures in memory.  If this function returns ADT_FAILURE you can examine 
 * the value of the BIT Status Register to see what failed.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param bitStatus is the a pointer to store the value of the BIT Status Register.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid device number
	- \ref ADT_ERR_TIMEOUT - Timeout error
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Unsupported channel type
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_BIT_InitiatedBIT(ADT_L0_UINT32 devID, ADT_L0_UINT32 *bitStatus) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 temp, loopCounter;

	/* Perform IBIT based on device type */
	switch (devID & 0x0000FF00) {
		case ADT_DEVID_CHANNELTYPE_GLOBALS:
			/* GLOBAL device always passes */
			result = ADT_SUCCESS;
			*bitStatus = 0;
			break;

		case ADT_DEVID_CHANNELTYPE_WMUX:
		case ADT_DEVID_CHANNELTYPE_1553:
			/* Start IBIT */
			result = ADT_L1_ReadDeviceMem32(devID, ADT_L1_1553_PE_ROOT_CSR, &temp, 1);
			temp |= ADT_L1_1553_PECSR_RUNIBIT;
			result = ADT_L1_WriteDeviceMem32(devID, ADT_L1_1553_PE_ROOT_CSR, &temp, 1);

			/* Wait for PE to clear the Run IBIT bit (this will timeout on a SIMULATED device) */
			loopCounter = 0;
			while ((temp & ADT_L1_1553_PECSR_RUNIBIT) && (loopCounter < 1000000))
			{
				result = ADT_L1_ReadDeviceMem32(devID, ADT_L1_1553_PE_ROOT_CSR, &temp, 1);
				loopCounter++;
			}
			if (loopCounter >= 1000000) 
			{
				result = ADT_ERR_TIMEOUT;
				*bitStatus = 0xFFFFFFFF;
			}

			if (result == ADT_SUCCESS) 
			{
				/* Short delay to ensure IBIT is started */
				ADT_L1_msSleep(1);

				/* Wait for IBIT to complete */
				loopCounter = 0;
				temp = ADT_L1_1553_BIT_IBITINPROG;
				while ((temp & ADT_L1_1553_BIT_IBITINPROG) && (loopCounter < 1000000)) 
				{
					result = ADT_L1_ReadDeviceMem32(devID, ADT_L1_1553_PE_BITSTATUS, &temp, 1);
					loopCounter++;
				}
				if (loopCounter >= 1000000) 
				{
					result = ADT_ERR_TIMEOUT;
					*bitStatus = 0xFFFFFFFF;
				}

				if (result == ADT_SUCCESS)
				{
					/* Check BIT STATUS for IBIT FAILURE bit */
					result = ADT_L1_ReadDeviceMem32(devID, ADT_L1_1553_PE_BITSTATUS, &temp, 1);
					if (temp & ADT_L1_1553_BIT_IBITFAIL) result = ADT_FAILURE;
					*bitStatus = temp;
				}
			}
			break;

		case ADT_DEVID_CHANNELTYPE_A429:
			/* Start IBIT */
			result = ADT_L1_ReadDeviceMem32(devID, ADT_L1_A429_PE_ROOT_CSR, &temp, 1);
			temp |= ADT_L1_A429_PECSR_RUNIBIT;
			result = ADT_L1_WriteDeviceMem32(devID, ADT_L1_A429_PE_ROOT_CSR, &temp, 1);

			/* Wait for PE to clear the Run IBIT bit (this will timeout on a SIMULATED device) */
			loopCounter = 0;
			while ((temp & ADT_L1_A429_PECSR_RUNIBIT) && (loopCounter < 1000000))
			{
				result = ADT_L1_ReadDeviceMem32(devID, ADT_L1_A429_PE_ROOT_CSR, &temp, 1);
				loopCounter++;
			}
			if (loopCounter >= 1000000) 
			{
				result = ADT_ERR_TIMEOUT;
				*bitStatus = 0xFFFFFFFF;
			}

			if (result == ADT_SUCCESS) 
			{
				/* Short delay to ensure IBIT is started */
				ADT_L1_msSleep(1);

				/* Wait for IBIT to complete */
				loopCounter = 0;
				temp = ADT_L1_A429_BIT_IBITINPROG;
				while ((temp & ADT_L1_A429_BIT_IBITINPROG) && (loopCounter < 1000000)) 
				{
					result = ADT_L1_ReadDeviceMem32(devID, ADT_L1_A429_PE_BITSTATUS, &temp, 1);
					loopCounter++;
				}
				if (loopCounter >= 1000000) 
				{
					result = ADT_ERR_TIMEOUT;
					*bitStatus = 0xFFFFFFFF;
				}

				if (result == ADT_SUCCESS)
				{
					/* Check BIT STATUS for IBIT FAILURE bit */
					result = ADT_L1_ReadDeviceMem32(devID, ADT_L1_A429_PE_BITSTATUS, &temp, 1);
					if (temp & ADT_L1_A429_BIT_IBITFAIL) result = ADT_FAILURE;
					*bitStatus = temp;
				}
			}
			break;

		default:
			result = ADT_ERR_UNSUPPORTED_CHANNELTYPE;
			*bitStatus = 0xFFFFFFFF;
			break;
	}

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_BIT_PeriodicBIT
 *****************************************************************************/
/*! \brief Checks PBIT on the device 
 *
 * This function checks PERIODIC BIT.  This can be safely run during normal
 * operation.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param bitStatus is the a pointer to store the value of the BIT Status Register.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid device number
	- \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Unsupported channel type
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_BIT_PeriodicBIT(ADT_L0_UINT32 devID, ADT_L0_UINT32 *bitStatus) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 temp;

	ADT_L0_msSleep(10);


	/* Perform PBIT based on device type */
	switch (devID & 0x0000FF00) {
		case ADT_DEVID_CHANNELTYPE_GLOBALS:
			/* GLOBAL device always passes */
			result = ADT_SUCCESS;
			*bitStatus = 0;
			break;

		case ADT_DEVID_CHANNELTYPE_WMUX:
		case ADT_DEVID_CHANNELTYPE_1553:
			/* Check BIT STATUS for PBIT FAILURE bit */
			result = ADT_L1_ReadDeviceMem32(devID, ADT_L1_1553_PE_BITSTATUS, &temp, 1);
			if (temp & ADT_L1_1553_BIT_PBITFAIL) result = ADT_FAILURE;
			*bitStatus = temp;
			break;

		case ADT_DEVID_CHANNELTYPE_A429:
			/* Check BIT STATUS for PBIT FAILURE bit */
			result = ADT_L1_ReadDeviceMem32(devID, ADT_L1_A429_PE_BITSTATUS, &temp, 1);
			if (temp & ADT_L1_A429_BIT_PBITFAIL) result = ADT_FAILURE;
			*bitStatus = temp;
			break;

		default:
			result = ADT_ERR_UNSUPPORTED_CHANNELTYPE;
			*bitStatus = 0xFFFFFFFF;
			break;
	}


	return( result );
}


#ifdef __cplusplus
}
#endif
