/*****************************************************************************
 * ADT Example Program:		ADT_L1_A429_ex_APMP_API_Init.c
 *
 * Copyright (c) 2016, Alta Data Technologies LLC (ADT), All Rights Reserved.
 * Use of this software is subject to the ADT Software License (latest
 * revision), US Government or local laws and the ADT Terms and Conditions
 * of Sale (latest revision).
 *
 * Description:
 *		This example demonstrates how to use AltaAPI to configure the
 *      transmission bit rate and APMP enable for the individual channels
 *      in a bank of A429 channels.
 *
 *      Please see the AltaAPI User's Manual and the AltaCore-ARINC manual
 *      sections on ARINC A429 APMP for detailed descriptions of RXPs and
 *      APMP.
 *
 *****************************************************************************/


#include "A429.h"
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <ADT_L1.h>



#define DEVID_GLOBAL	(ADT_PRODUCT_ENETA429 | ADT_DEVID_BOARDNUM_01 | ADT_DEVID_CHANNELTYPE_GLOBALS | ADT_DEVID_CHANNELNUM_01)
#define DEVID		(ADT_PRODUCT_ENETA429 | ADT_DEVID_BOARDNUM_01 | ADT_DEVID_CHANNELTYPE_A429 | ADT_DEVID_BANK_01)


/* Macro: IP Address octets (chars) to ADT_L0_UINT32 - used for eNet-A429 only */
#define ipOctets_to_ADT_L0_UINT32(class1, class2, subnet, hostNum) (ADT_L0_UINT32)((class1 << 24) | (class2 << 16) | (subnet << 8) | hostNum)


A429::A429() : _irigFail(0)
{

}

A429::~A429()
{
  Close();
}


void A429::Open()
{
printf("Open()\n");
  ADT_L0_UINT32	status = ADT_SUCCESS, rootCsr, timeLow, timeHigh;


  /* Set up the eNet device for APMP operation. */
  Setup();
printf("Setup done; isOpen=%d irigFail=%d\n", _isOpen, _irigFail);

  if (_irigFail)
  {
    printf("Disabling APMP UDP broadcast.\n");
    status = ADT_L1_ReadDeviceMem32(DEVID, ADT_L1_A429_PE_ROOT_CSR, &rootCsr, 1);
    rootCsr &= ~(ADT_L1_A429_PECSR_ENET_APMP_ENABLE);
    status = ADT_L1_WriteDeviceMem32(DEVID, ADT_L1_A429_PE_ROOT_CSR, &rootCsr, 1);
    return;
  }

  if (status != ADT_SUCCESS)
  {
    printf("A429_Setup = %d\n", status);
    return;
  }


  /* Read the Device Root CSR. */
  status = ADT_L1_ReadDeviceMem32(DEVID, ADT_L1_A429_PE_ROOT_CSR, &rootCsr, 1);
  if (status != ADT_SUCCESS)
    printf("\nFAILED!  - ADT_L1_ReadDeviceMem32 %d\n", status);
  else
    printf("\nRead CSR =0x%x\n",rootCsr);

  /* OR-in the APMP "on" option and write back. */
  rootCsr |= ADT_L1_A429_PECSR_ENET_APMP_ENABLE;
  //rootCsr |= ADT_L1_A429_PECSR_ENET_APMP_PEIRIG;
  rootCsr |= ADT_L1_A429_PECSR_ENET_APMP_PEINTV;

  status = ADT_L1_WriteDeviceMem32(DEVID, ADT_L1_A429_PE_ROOT_CSR, &rootCsr, 1);
  if (status != ADT_SUCCESS)
    printf("\nFAILED!  - ADT_L1_WriteDeviceMem32 %d\n", status);

  status = ADT_L1_ReadDeviceMem32(DEVID, ADT_L1_A429_PE_ROOT_CSR, &rootCsr, 1);
  if (status != ADT_SUCCESS)
    printf("\nFAILED!  - ADT_L1_ReadDeviceMem32 %d\n", status);
  else
    printf("\nRead CSR =0x%x\n",rootCsr);


// Not necessary.
  status = ADT_L1_Global_ReadIrigTime(DEVID_GLOBAL, &timeHigh, &timeLow);
  printf("%d 0x%08x 0x%08x\n", status, timeHigh, timeLow);

}


/* This function initializes the eNet-A429 device, and configures the RX channels
 * on the device.
 */
void A429::Setup()
{
  ADT_L0_UINT32	status, l0Version, l1Version;
  ADT_L0_UINT16	peVersion;

  char		strIn[100];
  ADT_L0_UINT32	s1 = 192, s2 = 168, s3 = 84, s4 = 12;		/* Server (eNet device) IP Octets */
  ADT_L0_UINT32	c1 = 192, c2 = 168, c3 = 84, c4 = 2;		/* Client (Your computer) IP Octets */


  /* Set IP addresses and Init Devices - Must be done in this sequential order! */
  status = ADT_L1_ENET_SetIpAddr(DEVID, ipOctets_to_ADT_L0_UINT32(s1, s2, s3, s4),
					ipOctets_to_ADT_L0_UINT32(c1, c2, c3, c4));
  if (status != ADT_SUCCESS)
    printf("ADT_L1_ENET_SetIpAddr failed, status = %d\n", status);


  status = ADT_L1_InitDevice(DEVID_GLOBAL, 0);
  if (status != ADT_SUCCESS)
  {
    printf("FAILURE ADT_L1_InitDevice GLOBAL - Error = %d\n", status);
    return;
  }


  /* Init the ENET Device */
  status = ADT_L1_A429_InitDefault(DEVID, 10);
  if (status != ADT_SUCCESS) {
    printf("ADT_L1_A429_InitDefault Net1 failed, status = %d", status); fflush(stdout);
    sleep(3);
    status = ADT_L1_A429_InitDefault_ExtendedOptions(DEVID, 10, ADT_L1_API_DEVICEINIT_FORCEINIT | ADT_L1_API_DEVICEINIT_NOMEMTEST);
    if (status != ADT_SUCCESS) {
      printf("ADT_L1_A429_InitDefault_ExtendedOptions Net1 failed, status = %d", status); fflush(stdout);
      return;
    }
  }


  /* NOTE ON A429 DEVICE INITIALIZATION FUNCTIONS:
   *	The recommended initialization method is ADT_L1_A429_InitDefault.  This function initializes the A429 channel,
   *	performs a memory test (can take a few seconds), allocates interrupt queue, and sets defaults for A429 protocol.
   *
   *  If the application is exited abnormally (and the ADT_L1_CloseDevice was not called), then the next time the
   *  device is initialized you may see status/error 1017 (ADT_ERR_DEVICEINUSE).  This is used to prevent two applications
   *  from initializing the same device.  On an abnormal exit the board can be left in a "IN USE" state.  This
   *  state can be overridden using the ADT_L1_A429_InitDefault_ExtendedOptions function.  This function can be
   *  used to force initialization when the device is in use with the option ADT_L1_API_DEVICEINIT_FORCEINIT.
   *  This function can also be used to skip the initialization memory test with the option ADT_L1_API_DEVICEINIT_NOMEMTEST.
   *  See the AltaAPI Users Manual for more details on this function.  Here is an example of the initialization call
   *  using these options:
   */

  /* Optional Retrieval of Device PE and API Info - mainly for troubleshooting. */
/*
  status = ADT_L1_GetVersionInfo(DEVID, &peVersion, &l0Version, &l1Version);
  if (status == ADT_SUCCESS) {
    printf("Success.\n");
    printf("   PE version = %04X\n", peVersion);
    printf("   L0 API version = %d.%d.%d.%d\n", (l0Version & 0xFF000000) >> 24,
		(l0Version & 0x00FF0000) >> 16, (l0Version & 0x0000FF00) >> 8, l0Version & 0x000000FF);
    printf("   L1 API version = %d.%d.%d.%d\n", (l1Version & 0xFF000000) >> 24,
		(l1Version & 0x00FF0000) >> 16, (l1Version & 0x0000FF00) >> 8, l1Version & 0x000000FF);
  }
  else printf("\nFAILED! ADT_L1_GetVersionInfo = %d", status);
*/


  /* Setup the Multichannel (MC) RX buffer, 100 RXPs.  As long as there is at least one
   * RXP allocated for the MC buffer, APMP can function.  As labels are written
   * to the MC buffer, they are immediately copied to the APMP buffer if APMP is enabled. */

  status = ADT_L1_A429_RXMC_BufferCreate(DEVID, 100);
  if (status != ADT_SUCCESS)
    printf("ADT_L1_A429_RXMC_BufferCreate failed, status=%d\n", status);

  _isOpen = true;
}


void A429::CalibrateIRIG()
{
printf("CalibrateIRIG\n");
  ADT_L0_UINT32 status, globalCSR;
  _irigFail = true;

  status = ADT_L1_Global_CalibrateIrigDac(DEVID_GLOBAL);
  if (status != ADT_SUCCESS)
  {
    printf("ADT_L1_Global_CalibrateIrigDac failed, status = %d\n", status);
    printf("IRIG signal failed to calibrate.");
  }
  else
    _irigFail = false;	// success.
}


void A429::StartChannel(int channel, int speed)
{
printf("StartChannel %d %d\n", channel, speed);
  ADT_L0_UINT32 status;

  status = ADT_L1_A429_RX_Channel_Init(DEVID, channel, speed, 100, ADT_L1_A429_API_RX_MCON);
  if (status != ADT_SUCCESS)
    printf("ADT_L1_A429_RX_Channel_Init(%d) failed, status=%d\n", channel, status);

  /* Once the select channels have been initialized to store their received
   * labels in the multi-channel buffer, this is the last step in the APMP
   * setup.  Then as soon as operation is started on the  RX channels, APMP
   * packets will be broadcast on the Ethernet connection. */

  /* Now Start the Receive Channels. */
  status = ADT_L1_A429_RX_Channel_Start(DEVID, channel);
  if (status != ADT_SUCCESS)
    printf("ADT_L1_A429_RX_Channel_Start(%d) failed, status=%d\n", channel, status);
}


void A429::Status()
{
  ADT_L0_UINT32 status, globalCSR, portnum, transactions, retries, failures;

  // Check IRIG status.
  status = ADT_L1_ReadDeviceMem32(DEVID_GLOBAL, ADT_L1_GLOBAL_CSR, &globalCSR, 1);
  printf("IRIG: Detect=%d, Latch=%d, Lock=%d\n", (globalCSR & ADT_L1_GLOBAL_CSR_IRIG_DETECT), (globalCSR & ADT_L1_GLOBAL_CSR_IRIG_LATCH), (globalCSR & ADT_L1_GLOBAL_CSR_IRIG_LOCK));

  /* For ENET devices - Get and display the ENET ADCP statistics */
  if ((DEVID & 0xF0000000) == ADT_DEVID_BACKPLANETYPE_ENET) {
    status = ADT_L1_ENET_ADCP_GetStatistics(DEVID, &portnum, &transactions, &retries, &failures);
    if (status == ADT_SUCCESS) {
      printf("UDP Port %d:  %d transactions, %d retries, %d failures\n", portnum, transactions, retries, failures);
    }
  }
}


/* This function stops the eNet-A429 RX channels, releases buffer memory, displays
 * Ethernet transaction statistics, and lastly closes down the eNet device.  APMP
 * packet broadcasts will cease as the RX channels are closed down.
 */
void A429::Close()
{
printf("Close()\n");
  if (_isOpen == false)
    return;

  ADT_L0_UINT32 status;

  /* Stop the Receive Channels before deallocating channel memory. */
  for (int i = 0; i < _channelList.size(); ++i)
  {
    status = ADT_L1_A429_RX_Channel_Stop(DEVID, _channelList[i]);
    if (status != ADT_SUCCESS)
      printf("ADT_L1_A429_RX_Channel_Stop(%d) failed, status=%d\n", _channelList[i], status);

    status = ADT_L1_A429_RX_Channel_Close(DEVID, _channelList[i]);
    if (status != ADT_SUCCESS)
      printf("ADT_L1_A429_RX_Channel_Close(%d) failed, status=%d\n", _channelList[i], status);
  }


  status = ADT_L1_A429_RXMC_BufferFree(DEVID);
  if (status != ADT_SUCCESS)
    printf("ADT_L1_A429_RXMC_BufferFree failed, status=%d\n", status);

  status = ADT_L1_CloseDevice(DEVID);
  if (status != ADT_SUCCESS)
    printf("ADT_L1_CloseDevice failed, status=%d\n", status);

  _isOpen = false;
}
