/******************************************************************************
 * FILE:			ADT_L1_MemMgmt.c
 *
 * Copyright (c) 2007-2016, Alta Data Technologies LLC (ADT), All Rights Reserved.
 * Use of this software is subject to the ADT Software License (latest
 * revision), US Government or local laws and the ADT Terms and Conditions 
 * of Sale (latest revision).
 *
 * DESCRIPTION:
 *	Source file for Layer 1 API.
 *	Contains MEMORY MANAGEMENT functions.
 *
 *****************************************************************************/
/*! \file ADT_L1_MemMgmt.c 
 *  \brief Source file containing memory management functions 
 */
#include "ADT_L1.h"

#ifdef __cplusplus
extern "C" {
#endif

/* NOTE:
 *   We changed the memory management from using a single linked-list of 
 *   "device mem manager" nodes to an array of DMM nodes in v1.2.1.0.
 *   The linked-list approach was more memory-efficient, but was NOT
 *   thread-safe.  We saw problems in AltaConfig when running threads for
 *   each board under test where the DMM nodes were lost.  The new 
 *   approach with an array indexed by device ID solves this problem.
 */


/******* Types for Memory Management *******/
typedef struct free_mem_node {
	ADT_L0_UINT32 Start;
	ADT_L0_UINT32 Size;
	struct free_mem_node *pNext;
} FREE_MEM_NODE;

typedef struct dev_mem_manager {
	ADT_L0_UINT32 devID;
	FREE_MEM_NODE *pFreeMem;
	struct dev_mem_manager *pNext;
} DEV_MEM_MANAGER;

/******* Globals for Memory Management ******/
/* DEV_MEM_MANAGER *pMemMgrRoot = NULL; */

/* dmmArray by backplane, board type, board num, channel type, channel num */
/*          Sim/PCI/ENET    0-0x31         0-15   0x01,0x10,0x20    0-5         */
/* May need to increase size as more products are added.                   */
DEV_MEM_MANAGER *dmmArray[ADT_L1_MEMMGT_NUM_BACKPLANE][ADT_L1_MEMMGT_NUM_BOARDTYPE][ADT_L1_MEMMGT_NUM_BOARDNUM][ADT_L1_MEMMGT_NUM_CHANTYPE][ADT_L1_MEMMGT_NUM_CHANNUM];
ADT_L0_UINT32 firstTime = 1;

/* MORE ON THREAD-SAFETY:
 * In general, the safe approach is to use a single thread for each device.
 * If you have multiple threads for a single device, then any code that
 * allocates or frees board memory is not thread-safe.  In cases like this,
 * perform all memory allocation of BC messages, RT buffers, etc. in the
 * main thread before spawning threads to service buffers.  Do not allocate
 * or free memory in sub-threads.
 */

/******************************************************************************
  FUNCTION:		ADT_L1_InitMemMgmt
 *****************************************************************************/
/*! \brief Initializes memory management for a device 
 *
 * This function initializes a memory management for a device - initializes
 * the internal data structures for memory management, determines the size
 * of user memory by product type, and performs a memory test on user memory.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_MEM_MGT_INIT - Mem management already initialized for the devID
	- \ref ADT_ERR_BAD_DEV_TYPE - Unknown device type in devID
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_InitMemMgmt(ADT_L0_UINT32 devID, ADT_L0_UINT32 startupOptions) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 memStart, memSize, memTop;  /* in BYTES */
	DEV_MEM_MANAGER *pDmmNode, *pDmmNewNode;
	FREE_MEM_NODE *pFreeMemNode;
	ADT_L0_UINT32 addr, exp, act, i, j, k, l, m;
	ADT_L0_UINT32 backplaneType, boardType, boardNum, channelType, channelNum;

	/* Break out the fields of the Device ID */
	backplaneType = devID & 0xF0000000;
	boardType =		devID & 0x0FF00000;
	boardNum =		devID & 0x000F0000;
	channelType =	devID & 0x0000FF00;
	channelNum =	devID & 0x000000FF;

	/* Look for a DEV_MEM_MANAGER node for this devID */
	/*
	found = 0;
	pDmmLastNode = NULL;
	pDmmNode = pMemMgrRoot;
	while ((pDmmNode != NULL) && (!found)) {
		if (pDmmNode->devID == devID) {
			found = 1;
		}
		else {
			pDmmLastNode = pDmmNode;
			pDmmNode = pDmmNode->pNext;
		}
	}
	*/

	if (firstTime) 
	{
		for (i = 0; i<ADT_L1_MEMMGT_NUM_BACKPLANE; i++) 
		{
			for (j=0; j<ADT_L1_MEMMGT_NUM_BOARDTYPE; j++)
			{
				for (k=0; k<ADT_L1_MEMMGT_NUM_BOARDNUM; k++)
				{
					for (l=0; l<ADT_L1_MEMMGT_NUM_CHANTYPE; l++) 
					{
						for (m=0; m<ADT_L1_MEMMGT_NUM_CHANNUM; m++)
						{
							dmmArray[i][j][k][l][m] = NULL;
						}
					}
				}
			}
		}
		firstTime = 0;
	}

	pDmmNode = dmmArray[backplaneType >> 28][boardType >> 20][boardNum >> 16][channelType >> 8][channelNum];

	/* If not found, then add a node for this devID */
	if (pDmmNode == NULL) {
		/* Determine memStart and memSize based on devID */
		switch (boardType) {
			case ADT_DEVID_BOARDTYPE_SIM1553:		/* SIM-1553 Board */
				memStart = ADT_L1_1553_USER_MEM_START;
				memTop = ADT_L1_SIM1553_CHAN_SIZE;
				memSize = memTop - ADT_L1_1553_USER_MEM_START;  /* 512K bytes per channel minus channel register area */
				break;

			case ADT_DEVID_BOARDTYPE_TEST1553:		/* TEST-1553 Board */
				memStart = ADT_L1_1553_USER_MEM_START;
				memTop = ADT_L1_TEST1553_CHAN_SIZE;
				memSize = memTop - ADT_L1_1553_USER_MEM_START;  /* 512K bytes per channel minus channel register area */
				break;

			case ADT_DEVID_BOARDTYPE_PMC1553:		/* PMC-1553 Board */
				memStart = ADT_L1_1553_USER_MEM_START;
				memTop = ADT_L1_PMC1553_CHAN_SIZE;
				memSize = memTop - ADT_L1_1553_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				break;

			case ADT_DEVID_BOARDTYPE_PCI1553:		/* PCI-1553 Board */
				memStart = ADT_L1_1553_USER_MEM_START;
				memTop = ADT_L1_PCI1553_CHAN_SIZE;
				memSize = memTop - ADT_L1_1553_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				break;

			case ADT_DEVID_BOARDTYPE_PC104P1553:		/* PC104P-1553 Board */
				memStart = ADT_L1_1553_USER_MEM_START;
				memTop = ADT_L1_PC104P1553_CHAN_SIZE;
				memSize = memTop - ADT_L1_1553_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				break;

			case ADT_DEVID_BOARDTYPE_PCCD1553:		/* PCCD-1553 Board */
				memStart = ADT_L1_1553_USER_MEM_START;
				memTop = ADT_L1_PCCD1553_CHAN_SIZE;
				memSize = memTop - ADT_L1_1553_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				break;

			case ADT_DEVID_BOARDTYPE_PCI104E1553:		/* PCI104E-1553 Board */
				memStart = ADT_L1_1553_USER_MEM_START;
				memTop = ADT_L1_PCI104E1553_CHAN_SIZE;
				memSize = memTop - ADT_L1_1553_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				break;

			case ADT_DEVID_BOARDTYPE_XMC1553:		/* XMC-1553 Board */
				memStart = ADT_L1_1553_USER_MEM_START;
				memTop = ADT_L1_XMCE4L1553_CHAN_SIZE;
				memSize = memTop - ADT_L1_1553_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				break;

			case ADT_DEVID_BOARDTYPE_XMCMW:		/* XMC-MW Board */
				memStart = ADT_L1_1553_USER_MEM_START;
				memTop = ADT_L1_XMCMW_CHAN_SIZE;
				memSize = memTop - ADT_L1_1553_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				break;

			case ADT_DEVID_BOARDTYPE_ECD54_1553:		/* ECD54-1553 Board */
				memStart = ADT_L1_1553_USER_MEM_START;
				memTop = ADT_L1_ECD54_1553_CHAN_SIZE;
				memSize = memTop - ADT_L1_1553_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				break;

			case ADT_DEVID_BOARDTYPE_PCIE4L1553:		/* PCIE4L-1553 Board */
				memStart = ADT_L1_1553_USER_MEM_START;
				memTop = ADT_L1_PCIE4L1553_CHAN_SIZE;
				memSize = memTop - ADT_L1_1553_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				break;

			case ADT_DEVID_BOARDTYPE_PCIE1L1553:		/* PCIE1L-1553 Board */
				memStart = ADT_L1_1553_USER_MEM_START;
				memTop = ADT_L1_PCIE4L1553_CHAN_SIZE;
				memSize = memTop - ADT_L1_1553_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				break;

			case ADT_DEVID_BOARDTYPE_MPCIE1553:		/* MPCIE-1553 Board */
				memStart = ADT_L1_1553_USER_MEM_START;
				memTop = ADT_L1_MPCIE1553_CHAN_SIZE;
				memSize = memTop - ADT_L1_1553_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				break;

			case ADT_DEVID_BOARDTYPE_MPCIE21553:		/* MPCIE2-1553 Board */
				memStart = ADT_L1_1553_USER_MEM_START;
				memTop = ADT_L1_MPCIE21553_CHAN_SIZE;
				memSize = memTop - ADT_L1_1553_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				break;

			case ADT_DEVID_BOARDTYPE_SIMA429:		/* SIM-A429 Board */
				memStart = ADT_L1_A429_USER_MEM_START;
				memTop = ADT_L1_SIMA429_CHAN_SIZE;
				memSize = memTop - ADT_L1_A429_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				break;

			case ADT_DEVID_BOARDTYPE_TESTA429:		/* TEST-A429 Board */
				memStart = ADT_L1_A429_USER_MEM_START;
				memTop = ADT_L1_TESTA429_CHAN_SIZE;
				memSize = memTop - ADT_L1_A429_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				break;

			case ADT_DEVID_BOARDTYPE_PMCA429:		/* PMC-A429 Board */
				memStart = ADT_L1_A429_USER_MEM_START;
				memTop = ADT_L1_PMCA429_CHAN_SIZE;
				memSize = memTop - ADT_L1_A429_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				break;

			case ADT_DEVID_BOARDTYPE_PMCA429HD:		/* PMC-A429HD Board */
				memStart = ADT_L1_A429_USER_MEM_START;
				memTop = ADT_L1_PMCA429HD_CHAN_SIZE;
				memSize = memTop - ADT_L1_A429_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				break;

			case ADT_DEVID_BOARDTYPE_PCIA429:		/* PCI-A429 Board */
				memStart = ADT_L1_A429_USER_MEM_START;
				memTop = ADT_L1_PCIA429_CHAN_SIZE;
				memSize = memTop - ADT_L1_A429_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				break;

			case ADT_DEVID_BOARDTYPE_PC104PA429:		/* PC104P-A429 Board */
				memStart = ADT_L1_A429_USER_MEM_START;
				memTop = ADT_L1_PC104PA429_CHAN_SIZE;
				memSize = memTop - ADT_L1_A429_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				break;

			case ADT_DEVID_BOARDTYPE_PC104PA429LTV:		/* PC104P-A429-LTV Board */
				memStart = ADT_L1_A429_USER_MEM_START;
				memTop = ADT_L1_PC104PA429LTV_CHAN_SIZE;
				memSize = memTop - ADT_L1_A429_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				break;

			case ADT_DEVID_BOARDTYPE_PCCDA429:		/* PCCD-A429 Board */
				memStart = ADT_L1_A429_USER_MEM_START;
				memTop = ADT_L1_PCCDA429_CHAN_SIZE;
				memSize = memTop - ADT_L1_A429_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				break;

			case ADT_DEVID_BOARDTYPE_PCI104EA429:		/* PCI104E-A429 Board */
				memStart = ADT_L1_A429_USER_MEM_START;
				memTop = ADT_L1_PCI104EA429_CHAN_SIZE;
				memSize = memTop - ADT_L1_A429_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				break;

			case ADT_DEVID_BOARDTYPE_XMCA429:		/* XMC-A429 Board */
				memStart = ADT_L1_A429_USER_MEM_START;
				memTop = ADT_L1_XMCE4LA429_CHAN_SIZE;
				memSize = memTop - ADT_L1_A429_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				break;

			case ADT_DEVID_BOARDTYPE_ECD54_A429:		/* ECD54-A429 Board */
				memStart = ADT_L1_A429_USER_MEM_START;
				memTop = ADT_L1_ECD54_A429_CHAN_SIZE;
				memSize = memTop - ADT_L1_A429_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				break;

			case ADT_DEVID_BOARDTYPE_PCIE4LA429:		/* PCIE4L-A429 Board */
				memStart = ADT_L1_A429_USER_MEM_START;
				memTop = ADT_L1_PCIE4LA429_CHAN_SIZE;
				memSize = memTop - ADT_L1_A429_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				break;

			case ADT_DEVID_BOARDTYPE_PCIE1LA429:		/* PCIE4L-A429 Board */
				memStart = ADT_L1_A429_USER_MEM_START;
				memTop = ADT_L1_PCIE1LA429_CHAN_SIZE;
				memSize = memTop - ADT_L1_A429_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				break;

			case ADT_DEVID_BOARDTYPE_MPCIEA429:		/* PCIE4L-A429 Board */
				memStart = ADT_L1_A429_USER_MEM_START;
				memTop = ADT_L1_MPCIEA429_CHAN_SIZE;
				memSize = memTop - ADT_L1_A429_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				break;

			case ADT_DEVID_BOARDTYPE_PMCMA4:		/* PMC-MA4 Board */
				if (channelType == ADT_DEVID_CHANNELTYPE_1553)
				{
					memStart = ADT_L1_1553_USER_MEM_START;
					memTop = ADT_L1_PMCMA4_CHAN_SIZE;
					memSize = memTop - ADT_L1_1553_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				}
				else if (channelType == ADT_DEVID_CHANNELTYPE_A429)
				{
					memStart = ADT_L1_A429_USER_MEM_START;
					memTop = ADT_L1_PMCMA4_CHAN_SIZE;
					memSize = memTop - ADT_L1_A429_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				}
				else result = ADT_ERR_BAD_DEV_TYPE;
				break;

			case ADT_DEVID_BOARDTYPE_PC104PMA4:		/* PC104P-MA4 Board */
				if (channelType == ADT_DEVID_CHANNELTYPE_1553)
				{
					memStart = ADT_L1_1553_USER_MEM_START;
					memTop = ADT_L1_PC104PMA4_CHAN_SIZE;
					memSize = memTop - ADT_L1_1553_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				}
				else if (channelType == ADT_DEVID_CHANNELTYPE_A429)
				{
					memStart = ADT_L1_A429_USER_MEM_START;
					memTop = ADT_L1_PC104PMA4_CHAN_SIZE;
					memSize = memTop - ADT_L1_A429_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				}
				else result = ADT_ERR_BAD_DEV_TYPE;
				break;

			case ADT_DEVID_BOARDTYPE_PC104EMA4:		/* PC104E-MA4 Board */
				if (channelType == ADT_DEVID_CHANNELTYPE_1553)
				{
					memStart = ADT_L1_1553_USER_MEM_START;
					memTop = ADT_L1_PC104EMA4_CHAN_SIZE;
					memSize = memTop - ADT_L1_1553_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				}
				else if (channelType == ADT_DEVID_CHANNELTYPE_A429)
				{
					memStart = ADT_L1_A429_USER_MEM_START;
					memTop = ADT_L1_PC104EMA4_CHAN_SIZE;
					memSize = memTop - ADT_L1_A429_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				}
				else result = ADT_ERR_BAD_DEV_TYPE;
				break;

			case ADT_DEVID_BOARDTYPE_TBOLTMA4:		/* TBOLT-MA4 Board */
				if (channelType == ADT_DEVID_CHANNELTYPE_1553)
				{
					memStart = ADT_L1_1553_USER_MEM_START;
					memTop = ADT_L1_TBOLTMA4_CHAN_SIZE;
					memSize = memTop - ADT_L1_1553_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				}
				else if (channelType == ADT_DEVID_CHANNELTYPE_A429)
				{
					memStart = ADT_L1_A429_USER_MEM_START;
					memTop = ADT_L1_TBOLTMA4_CHAN_SIZE;
					memSize = memTop - ADT_L1_A429_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				}
				else result = ADT_ERR_BAD_DEV_TYPE;
				break;

			case ADT_DEVID_BOARDTYPE_XMCMA4:		/* XMC-MA4 Board */
				if (channelType == ADT_DEVID_CHANNELTYPE_1553)
				{
					memStart = ADT_L1_1553_USER_MEM_START;
					memTop = ADT_L1_XMCMA4_CHAN_SIZE;
					memSize = memTop - ADT_L1_1553_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				}
				else if (channelType == ADT_DEVID_CHANNELTYPE_A429)
				{
					memStart = ADT_L1_A429_USER_MEM_START;
					memTop = ADT_L1_XMCMA4_CHAN_SIZE;
					memSize = memTop - ADT_L1_A429_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				}
				else result = ADT_ERR_BAD_DEV_TYPE;
				break;

			case ADT_DEVID_BOARDTYPE_ENET_MA4:		/* ENET-MA4 Board */
				if (channelType == ADT_DEVID_CHANNELTYPE_1553)
				{
					memStart = ADT_L1_1553_USER_MEM_START;
					memTop = ADT_L1_ENET_MA4_CHAN_SIZE;
					memSize = memTop - ADT_L1_1553_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				}
				else if (channelType == ADT_DEVID_CHANNELTYPE_A429)
				{
					memStart = ADT_L1_A429_USER_MEM_START;
					memTop = ADT_L1_ENET_MA4_CHAN_SIZE;
					memSize = memTop - ADT_L1_A429_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				}
				else result = ADT_ERR_BAD_DEV_TYPE;
				break;

			case ADT_DEVID_BOARDTYPE_ENETX_MA4:		/* ENETX-MA4 Board */
				if (channelType == ADT_DEVID_CHANNELTYPE_1553)
				{
					memStart = ADT_L1_1553_USER_MEM_START;
					memTop = ADT_L1_ENETX_MA4_CHAN_SIZE;
					memSize = memTop - ADT_L1_1553_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				}
				else if (channelType == ADT_DEVID_CHANNELTYPE_A429)
				{
					memStart = ADT_L1_A429_USER_MEM_START;
					memTop = ADT_L1_ENETX_MA4_CHAN_SIZE;
					memSize = memTop - ADT_L1_A429_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				}
				else result = ADT_ERR_BAD_DEV_TYPE;
				break;

			case ADT_DEVID_BOARDTYPE_ENET1553:		/* ENET-1553 Board */
				memStart = ADT_L1_1553_USER_MEM_START;
				memTop = ADT_L1_ENET1553_CHAN_SIZE;
				memSize = memTop - ADT_L1_1553_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				break;

			case ADT_DEVID_BOARDTYPE_ENET2_1553:		/* ENET2-1553 Board */
				memStart = ADT_L1_1553_USER_MEM_START;
				memTop = ADT_L1_ENET2_1553_CHAN_SIZE;
				memSize = memTop - ADT_L1_1553_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				break;

			case ADT_DEVID_BOARDTYPE_ENETX_1553:		/* ENETX-1553 Board */
				memStart = ADT_L1_1553_USER_MEM_START;
				memTop = ADT_L1_ENETX_1553_CHAN_SIZE;
				memSize = memTop - ADT_L1_1553_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				break;

			case ADT_DEVID_BOARDTYPE_PMCE1553:		/* PMCE-1553 Board */
				memStart = ADT_L1_1553_USER_MEM_START;
				memTop = ADT_L1_PMCE1553_CHAN_SIZE;
				memSize = memTop - ADT_L1_1553_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				break;

			case ADT_DEVID_BOARDTYPE_ENETA429:		/* ENET-A429 Board */
				memStart = ADT_L1_A429_USER_MEM_START;
				memTop = ADT_L1_ENETA429_CHAN_SIZE;
				memSize = memTop - ADT_L1_A429_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				break;

			case ADT_DEVID_BOARDTYPE_ENETA429P:		/* ENET-A429P Board */
				memStart = ADT_L1_A429_USER_MEM_START;
				memTop = ADT_L1_ENETA429P_CHAN_SIZE;
				memSize = memTop - ADT_L1_A429_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				break;

			case ADT_DEVID_BOARDTYPE_ENET1A1553:		/* ENET1A-1553 Board */
				memStart = ADT_L1_1553_USER_MEM_START;
				memTop = ADT_L1_ENET1A1553_CHAN_SIZE;
				memSize = memTop - ADT_L1_1553_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				break;

			case ADT_DEVID_BOARDTYPE_ENET485:		/* ENET-485 Board */
				memStart = ADT_L1_1553_USER_MEM_START;
				memTop = ADT_L1_ENET485_CHAN_SIZE;
				memSize = memTop - ADT_L1_1553_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				break;

			case ADT_DEVID_BOARDTYPE_PMCWMUX:		/* PMC-WMUX Board */
				memStart = ADT_L1_1553_USER_MEM_START;
				memTop = ADT_L1_PMCWMUX_CHAN_SIZE;
				memSize = memTop - ADT_L1_1553_USER_MEM_START;  /* 1M bytes per channel minus channel register area */
				break;

			default:	/* Unknown device type */
				result = ADT_ERR_BAD_DEV_TYPE;
				break;
		}

		if (result == ADT_SUCCESS) {
			/* Test the memory range if selected */
			if (!(startupOptions & ADT_L1_API_DEVICEINIT_NOMEMTEST)) {
				result = ADT_L1_BIT_MemoryTest(devID, memStart, memTop, &addr, &exp, &act);
			}


			if (result == ADT_SUCCESS) {
				/* Create a free memory node */
				pFreeMemNode = (FREE_MEM_NODE *) malloc( sizeof(FREE_MEM_NODE) );
				pFreeMemNode->Start = memStart;
				pFreeMemNode->Size = memSize;
				pFreeMemNode->pNext = NULL;

				/* Create the new device mem manager node */
				pDmmNewNode = (DEV_MEM_MANAGER *) malloc( sizeof(DEV_MEM_MANAGER) );
				pDmmNewNode->devID = devID;
				pDmmNewNode->pFreeMem = pFreeMemNode;
				pDmmNewNode->pNext = NULL;

				/*
				if (pDmmLastNode == NULL)
					pMemMgrRoot = pDmmNewNode;
				else
					pDmmLastNode->pNext = pDmmNewNode;
				*/
				dmmArray[backplaneType >> 28][boardType >> 20][boardNum >> 16][channelType >> 8][channelNum] = pDmmNewNode;
			}
		}
	}

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_CloseMemMgmt
 *****************************************************************************/
/*! \brief Closes memory management for a device 
 *
 * This function frees resources and closes a memory management for a device.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_MEM_MGT_NO_INIT - Memory management has not been initialized for the device ID
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_CloseMemMgmt(ADT_L0_UINT32 devID) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	DEV_MEM_MANAGER *pDmmNode;
	FREE_MEM_NODE *pFreeMemNode;
	ADT_L0_UINT32 backplaneType, boardType, boardNum, channelType, channelNum;

	/* Break out the fields of the Device ID */
	backplaneType = devID & 0xF0000000;
	boardType =		devID & 0x0FF00000;
	boardNum =		devID & 0x000F0000;
	channelType =	devID & 0x0000FF00;
	channelNum =	devID & 0x000000FF;


	/* Look for a DEV_MEM_MANAGER node for this devID */
	/*
	found = 0;
	pDmmLastNode = NULL;
	pDmmNode = pMemMgrRoot;
	while ((pDmmNode != NULL) && (!found)) {
		if (pDmmNode->devID == devID) {
			found = 1;
		}
		else {
			pDmmLastNode = pDmmNode;
			pDmmNode = pDmmNode->pNext;
		}
	}
    */
	pDmmNode = dmmArray[backplaneType >> 28][boardType >> 20][boardNum >> 16][channelType >> 8][channelNum];

	/* If found, then . . . */
	if (pDmmNode != NULL) {
		/* Remove the dev mem mgmt node from the list */
		/*
		if (pDmmLastNode == NULL) {
			pMemMgrRoot = pDmmNode->pNext;
		}
		else {
			pDmmLastNode->pNext = pDmmNode->pNext;
		}
		*/

		/* Free all free mem nodes associated with this dev mem mgmt node */
		while (pDmmNode->pFreeMem != NULL) {
			pFreeMemNode = pDmmNode->pFreeMem;
			pDmmNode->pFreeMem = pFreeMemNode->pNext;
			free(pFreeMemNode);
		}

		/* Free the dev mem mgmt node */
		free(pDmmNode);

	    dmmArray[backplaneType >> 28][boardType >> 20][boardNum >> 16][channelType >> 8][channelNum] = NULL;
	}

	/* If not found, then memory management has not been initialized for this devID */
	else result = ADT_ERR_MEM_MGT_NO_INIT;

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_MemoryAlloc
 *****************************************************************************/
/*! \brief Allocates memory 
 *
 * This function allocates memory from the data structure memory area.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param memSize is the size in BYTES of memory requested.
 * @param pMemOffset is the pointer to store the starting BYTE offset of the allocated block of memory.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_MEM_MGT_NO_MEM - Requested memory is not available
	- \ref ADT_ERR_MEM_MGT_NO_INIT - Memory management has not been initialized for the device ID
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_MemoryAlloc(ADT_L0_UINT32 devID, ADT_L0_UINT32 memSize, ADT_L0_UINT32 *pMemOffset) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	DEV_MEM_MANAGER *pDmmNode;
	FREE_MEM_NODE *pFreeMemNode;
	int found;
	ADT_L0_UINT32 backplaneType, boardType, boardNum, channelType, channelNum; 
	ADT_L0_UINT32 datablock[ADT_RW_MEM_MAX_SIZE];
	ADT_L0_UINT32 tempSize, blockPtr;

	/* Break out the fields of the Device ID */
	backplaneType = devID & 0xF0000000;
	boardType =		devID & 0x0FF00000;
	boardNum =		devID & 0x000F0000;
	channelType =	devID & 0x0000FF00;
	channelNum =	devID & 0x000000FF;


	/* Look for a DEV_MEM_MANAGER node for this devID */
	/*
	found = 0;
	pDmmNode = pMemMgrRoot;
	while ((pDmmNode != NULL) && (!found)) {
		if (pDmmNode->devID == devID) {
			found = 1;
		}
		else {
			pDmmNode = pDmmNode->pNext;
		}
	}
	*/
	pDmmNode = dmmArray[backplaneType >> 28][boardType >> 20][boardNum >> 16][channelType >> 8][channelNum];


	/* If we found one, then . . . */
	if (pDmmNode != NULL) {
		/* Look for a block of free memory big enough for the requested size */
		found = 0;
		pFreeMemNode = pDmmNode->pFreeMem;
		while ((pFreeMemNode != NULL) && (!found)) {
			if (pFreeMemNode->Size >= memSize) {
				found = 1;
			}
			else {
				pFreeMemNode = pFreeMemNode->pNext;
			}
		}

		/* If we found one, then allocate the memory */
		if ((found) && (pFreeMemNode != NULL)) {
			*pMemOffset = pFreeMemNode->Start;

			/* Clear the allocated block of memory */

			/***** Old approach (before v2.5.7.0) *****
			if (memSize/4 <= ADT_RW_MEM_MAX_SIZE) {
				memset(datablock, 0, sizeof(datablock));
				(void) ADT_L1_WriteDeviceMem32(devID, pFreeMemNode->Start, datablock, memSize/4);
			}
			else {
				for (i=pFreeMemNode->Start; i<pFreeMemNode->Start + memSize; i+=4)
					(void) ADT_L1_WriteDeviceMem32(devID, i, &temp_value, 1);
			}
			*****/

			/* New approach - use block writes to clear memory - v2.5.7.0
			 * This is MUCH more efficient for ENET.
			 */
			memset(datablock, 0, sizeof(datablock));
			tempSize = memSize;
			blockPtr = pFreeMemNode->Start;
			while (tempSize/4 >= ADT_RW_MEM_MAX_SIZE) 
			{
				(void) ADT_L1_WriteDeviceMem32(devID, blockPtr, datablock, ADT_RW_MEM_MAX_SIZE);
				blockPtr += (ADT_RW_MEM_MAX_SIZE * 4);
				tempSize -= (ADT_RW_MEM_MAX_SIZE * 4);
			}
			if (tempSize/4 > 0)
			{
				(void) ADT_L1_WriteDeviceMem32(devID, blockPtr, datablock, tempSize/4);
			}
			/*****/

			pFreeMemNode->Start += memSize;
			pFreeMemNode->Size  -= memSize;
		}

		/* Not enough memory is available */
		else result = ADT_ERR_MEM_MGT_NO_MEM;

	}

	/* Memory management has not been initialized for this devID */
	else result = ADT_ERR_MEM_MGT_NO_INIT;


	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_MemoryFree
 *****************************************************************************/
/*! \brief Frees previously allocated memory 
 *
 * This function frees previously allocated memory in the data structure memory area.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param memStart is the starting BYTE offset of the memory block.
 * @param memSize is the size in BYTES of the memory block.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_MEM_MGT_NO_INIT - Memory management has not been initialized for the device ID
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_MemoryFree(ADT_L0_UINT32 devID, ADT_L0_UINT32 memStart, ADT_L0_UINT32 memSize) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	DEV_MEM_MANAGER *pDmmNode;
	FREE_MEM_NODE *pFreeMemNode, *pNewFreeMemNode, *pLastFreeMemNode;
	int found;
	ADT_L0_UINT32 NewMemStart, CurMemStart, NewMemEnd, CurMemEnd;
	ADT_L0_UINT32 backplaneType, boardType, boardNum, channelType, channelNum;

	/* Break out the fields of the Device ID */
	backplaneType = devID & 0xF0000000;
	boardType =		devID & 0x0FF00000;
	boardNum =		devID & 0x000F0000;
	channelType =	devID & 0x0000FF00;
	channelNum =	devID & 0x000000FF;


	/* Look for a DEV_MEM_MANAGER node for this devID */
	/*
	found = 0;
	pDmmNode = pMemMgrRoot;
	while ((pDmmNode != NULL) && (!found)) {
		if (pDmmNode->devID == devID) {
			found = 1;
		}
		else {
			pDmmNode = pDmmNode->pNext;
		}
	}
	*/
	pDmmNode = dmmArray[backplaneType >> 28][boardType >> 20][boardNum >> 16][channelType >> 8][channelNum];

	/* If we found one, then . . . */
	if (pDmmNode != NULL) {
		/* Create a new free memory node for the memory being freed */
		pNewFreeMemNode = (FREE_MEM_NODE *) malloc( sizeof(FREE_MEM_NODE) );
		pNewFreeMemNode->Start = memStart;
		pNewFreeMemNode->Size = memSize;
		pNewFreeMemNode->pNext = NULL;

		/* Add it to the free mem node list for this devID */
		pFreeMemNode = pDmmNode->pFreeMem;
		while (pFreeMemNode->pNext != NULL)
			pFreeMemNode = pFreeMemNode->pNext;
		pFreeMemNode->pNext = pNewFreeMemNode;

		/* Consolidate any contiguous blocks of memory */
		pFreeMemNode = pDmmNode->pFreeMem;
		while (pFreeMemNode != NULL) {

			pNewFreeMemNode = pDmmNode->pFreeMem;
			pLastFreeMemNode = NULL;
			while (pNewFreeMemNode != NULL) {

				found = 0;
				if (pNewFreeMemNode != pFreeMemNode) {
					CurMemStart = pFreeMemNode->Start;
					CurMemEnd = pFreeMemNode->Start + pFreeMemNode->Size;
					NewMemStart = pNewFreeMemNode->Start;
					NewMemEnd = pNewFreeMemNode->Start + pNewFreeMemNode->Size;

					if (CurMemEnd == NewMemStart) {
						/* Add new mem to end of curr mem */
						pFreeMemNode->Size += pNewFreeMemNode->Size;
						found = 1;
					}
					else if (NewMemEnd == CurMemStart) {
						/* Add new mem to start of curr mem */
						pFreeMemNode->Start = pNewFreeMemNode->Start;
						pFreeMemNode->Size += pNewFreeMemNode->Size;
						found = 1;
					}

					if (found) {
						/* Remove the new node - it has been consolidated */
						if (pLastFreeMemNode != NULL)
							pLastFreeMemNode->pNext = pNewFreeMemNode->pNext;
						else
							pDmmNode->pFreeMem = pNewFreeMemNode->pNext;

						free(pNewFreeMemNode);
					}

				}

				/* If we found, consolidated, and removed a block, then the 
				 * pLastFreeMemNode does not change and we set pNewFreeMemNode
				 * to the next block.
				 */
				if (found) {
					if (pLastFreeMemNode != NULL)
						pNewFreeMemNode = pLastFreeMemNode->pNext;
					else
						pNewFreeMemNode = pDmmNode->pFreeMem;
				}
				else {
					pLastFreeMemNode = pNewFreeMemNode;
					pNewFreeMemNode = pNewFreeMemNode->pNext;
				}
			}

			pFreeMemNode = pFreeMemNode->pNext;
		}

	}

	/* Memory management has not been initialized for this devID */
	else result = ADT_ERR_MEM_MGT_NO_INIT;

	return( result );
}



/******************************************************************************
  FUNCTION:		ADT_L1_GetMemoryAvailable
 *****************************************************************************/
/*! \brief Gets the amount of memory available 
 *
 * This function gets the amount of memory available (in bytes) in the 
 * data structure memory area.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param memAvailable is a pointer to store the number of BYTES available.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_MEM_MGT_NO_INIT - Memory management has not been initialized for the device ID
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_GetMemoryAvailable(ADT_L0_UINT32 devID, ADT_L0_UINT32 *memAvailable) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 temp;
	DEV_MEM_MANAGER *pDmmNode;
	FREE_MEM_NODE *pFreeMemNode;
	ADT_L0_UINT32 backplaneType, boardType, boardNum, channelType, channelNum;

	/* Break out the fields of the Device ID */
	backplaneType = devID & 0xF0000000;
	boardType =		devID & 0x0FF00000;
	boardNum =		devID & 0x000F0000;
	channelType =	devID & 0x0000FF00;
	channelNum =	devID & 0x000000FF;


	/* Look for a DEV_MEM_MANAGER node for this devID */
	/*
	found = 0;
	pDmmNode = pMemMgrRoot;
	while ((pDmmNode != NULL) && (!found)) {
		if (pDmmNode->devID == devID) {
			found = 1;
		}
		else {
			pDmmNode = pDmmNode->pNext;
		}
	}
	*/
	pDmmNode = dmmArray[backplaneType >> 28][boardType >> 20][boardNum >> 16][channelType >> 8][channelNum];

	/* If we found one, then . . . */
	if (pDmmNode != NULL) {
		/* Traverse free memory nodes, add up available memory */
		temp = 0;
		pFreeMemNode = pDmmNode->pFreeMem;
		while (pFreeMemNode != NULL) {
			temp += pFreeMemNode->Size;
			pFreeMemNode = pFreeMemNode->pNext;
		}
		*memAvailable = temp;
	}

	/* Memory management has not been initialized for this devID */
	else {
		result = ADT_ERR_MEM_MGT_NO_INIT;
		*memAvailable = 0;
	}

	return result;
}



#ifdef __cplusplus
}
#endif

