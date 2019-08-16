/******************************************************************************
 * FILE:       ADT_L1_General.c
 *
 * Copyright (c) 2007-2016, Alta Data Technologies LLC (ADT), All Rights Reserved.
 * Use of this software is subject to the ADT Software License (latest
 * revision), US Government or local laws and the ADT Terms and Conditions
 * of Sale (latest revision).
 *
 * DESCRIPTION:
 * Source file for Layer 1 API.
 * Contains GENERAL functions.
 *
 *****************************************************************************/
/*! \file ADT_L1_General.c
 *  \brief Source file containing General functions
 */
#include "ADT_L1.h"
#include <stdio.h>

/* Padding words for programming flash with firmware load - added in v2.6.0.5 */
/* Padding words no longer needed as entire .BIT file is now loaded - added in v3.1.0.3 */
//#define PADDING_WORDS 16

#ifdef __cplusplus
extern "C" {
#endif

/* Prototypes for Internal Functions */
ADT_L0_UINT32 ADT_L0_CALL_CONV Internal_GetChannelRegOffset(ADT_L0_UINT32 devID, ADT_L0_UINT32 *pChannel, ADT_L0_UINT32 *pChannelRegOffset);
ADT_L0_UINT32 ADT_L0_CALL_CONV Internal_ProgramEcdFlash(ADT_L0_UINT32 devID, ADT_L0_UINT32 numBytes, ADT_L0_UINT8 * fpga_load_bytes); /* Jake - added */
ADT_L0_UINT32 ADT_L0_CALL_CONV Internal_ProgramEnetFlash(ADT_L0_UINT32 devID, ADT_L0_UINT32 numBytes, ADT_L0_UINT8 * fpga_load_bytes); /* Jake - added */
ADT_L0_UINT32 ADT_L0_CALL_CONV Internal_ecd_flash_wait_complete(ADT_L0_UINT32 devID);
ADT_L0_UINT32 ADT_L0_CALL_CONV Internal_ecd_flash_check_writen(ADT_L0_UINT32 devID);
ADT_L0_UINT32 ADT_L0_CALL_CONV Internal_verify_flash(ADT_L0_UINT32 devID, ADT_L0_UINT32 numBytes, ADT_L0_UINT8 * fpga_load_bytes);

ADT_L0_UINT32 ADT_L0_CALL_CONV Internal_read_flash_CFI(ADT_L0_UINT32 devID, ADT_L0_UINT16 *cmd_set, ADT_L0_UINT32 *geometry); /* Michael - added */

ADT_L0_UINT32 ADT_L0_CALL_CONV Internal_write_flash_word_AMD_Std(ADT_L0_UINT32 devID, ADT_L0_UINT32 offset, ADT_L0_UINT16 value); /* Michael - added */
ADT_L0_UINT32 ADT_L0_CALL_CONV Internal_erase_flash_block_AMD_Std(ADT_L0_UINT32 devID, ADT_L0_UINT32 block_addr); /* Michael - added */

ADT_L0_UINT32 ADT_L0_CALL_CONV Internal_write_flash_word_Intel_Std(ADT_L0_UINT32 devID, ADT_L0_UINT32 offset, ADT_L0_UINT16 value);
ADT_L0_UINT32 ADT_L0_CALL_CONV Internal_unlock_flash_block_Intel_Std(ADT_L0_UINT32 devID, ADT_L0_UINT32 block_addr);
ADT_L0_UINT32 ADT_L0_CALL_CONV Internal_erase_flash_block_Intel_Std(ADT_L0_UINT32 devID, ADT_L0_UINT32 block_addr);
ADT_L0_UINT32 ADT_L0_CALL_CONV Internal_lock_flash_block_Intel_Std(ADT_L0_UINT32 devID, ADT_L0_UINT32 block_addr);

/* Internal array to relate DEVID to IP ADDRESS for ENET devices */
/* (ENET backplane type), board type, board # */
ADT_L0_UINT32 eNET_ServerIPaddr[256][16];
ADT_L0_UINT32 eNET_ClientIPaddr[256][16];


/******************************************************************************
  FUNCTION:    ADT_L1_Error_to_String
 *****************************************************************************/
/*! \brief Converts an error/status code to a string.
 *
 * This function converts an error/status code to a string.
 *
 * @param err_status is the error/status code to convert.
 * @return
   - \ref String corresponding to the error/status code.
*/
char * ADT_L0_CALL_CONV ADT_L1_Error_to_String(ADT_L0_UINT32 err_status) {
   switch (err_status)
   {
   case ADT_SUCCESS:
      return("Function call completed without error");
      break;
   case ADT_FAILURE:
      return("Function call completed with error");
      break;
   case ADT_ERR_MEM_MAP_SIZE:
      return("Layer 0 - Invalid memory map size");
      break;
   case ADT_ERR_NO_DEVICE:
      return("Layer 0 - Device not found");
      break;
   case ADT_ERR_CANT_OPEN_DEV:
      return("Layer 0 - Can't open device");
      break;
   case ADT_ERR_DEV_NOT_INITED:
      return("Layer 0 - Device not initialized");
      break;
   case ADT_ERR_DEV_ALREADY_OPEN:
      return("Layer 0 - Device already open");
      break;
   case ADT_ERR_UNSUPPORTED_BACKPLANE:
      return("Layer 0 - Unsupported backplane in DevID");
      break;
   case ADT_ERR_UNSUPPORTED_BOARDTYPE:
      return("Layer 0 - Unsupported board type in DevID");
      break;
   case ADT_ERR_UNSUPPORTED_CHANNELTYPE:
      return("Layer 0 - Unsupported channel type in DevID");
      break;
   case ADT_ERR_CANT_OPEN_DRIVER:
      return("Layer 0 - Can't open driver");
      break;
   case ADT_ERR_CANT_SET_DRV_OPTIONS:
      return("Layer 0 - Can't set driver options");
      break;
   case ADT_ERR_CANT_GET_DEV_INFO:
      return("Layer 0 - Can't get device info");
      break;
   case ADT_ERR_INVALID_BOARD_NUM:
      return("Layer 0 - Invalid board number");
      break;
   case ADT_ERR_INVALID_CHANNEL_NUM:
      return("Layer 0 - Invalid channel number");
      break;
   case ADT_ERR_DRIVER_READ_FAIL:
      return("Layer 0 - Driver read memory failure");
      break;
   case ADT_ERR_DRIVER_WRITE_FAIL:
      return("Layer 0 - Driver write memory failure");
      break;
   case ADT_ERR_DEVICE_CLOSE_FAIL:
      return("Layer 0 - Device close failure");
      break;
   case ADT_ERR_DRIVER_CLOSE_FAIL:
      return("Layer 0 - Driver close failure");
      break;
   case ADT_ERR_KP_OPEN_FAIL:
      return("Layer 0 - Kernel Plug-In Open failure");
      break;
   case ADT_ERR_ENET_NO_PORT_AVAILABLE:
      return("Layer 0 - No UDP port available");
      break;
   case ADT_ERR_ENET_READ_FAIL:
      return("Layer 0 - ENET Read failure");
      break;
   case ADT_ERR_ENET_WRITE_FAIL:
      return("Layer 0 - ENET Write failure");
      break;
   case ADT_ERR_ENET_NOTRUNNING:
      return("Layer 0 - ENET Sockets not running");
      break;
   case ADT_ERR_ENET_INVALID_SIZE:
      return("Layer 0 - ENET Invalid payload size");
      break;
   case ADT_ERR_ENET_SENDFAIL:
      return("Layer 0 - ENET Send failure");
      break;
   case ADT_ERR_ENET_SELECTFAIL:
      return("Layer 0 - ENET Select failure");
      break;
   case ADT_ERR_ENET_SELECTTIMEOUT:
      return("Layer 0 - ENET Select timeout");
      break;
   case ADT_ERR_ENET_BADSEQNUM:
      return("Layer 0 - ENET Bad sequence number");
      break;
   case ADT_ERR_ENET_SRVSTSFAIL:
      return("Layer 0 - ENET Server Status Code indicates FAILURE");
      break;
   case ADT_ERR_ENET_BADPRODUCTID:
      return("Layer 0 - ENET Bad Product ID");
      break;
   case ADT_ERR_CACHEDMAFAIL:
      return("Layer 0 - DMA Cache Malloc Fail");
      break;

   case ADT_ERR_BAD_INPUT:
      return("Layer 1 - Bad input parameter");
      break;
   case ADT_ERR_MEM_TEST_FAIL:
      return("Layer 1 - Failed memory test");
      break;
   case ADT_ERR_MEM_MGT_NO_INIT:
      return("Layer 1 - Memory Management not initialized for the device ID");
      break;
   case ADT_ERR_MEM_MGT_INIT:
      return("Layer 1 - Memory Management already initialized for the device ID");
      break;
   case ADT_ERR_MEM_MGT_NO_MEM:
      return("Layer 1 - Not enough memory available");
      break;
   case ADT_ERR_BAD_DEV_TYPE:
      return("Layer 1 - Bad device type in device ID");
      break;
   case ADT_ERR_RT_FT_UNDEF:
      return("Layer 1 - RT Filter Table not defined");
      break;
   case ADT_ERR_RT_SA_UNDEF:
      return("Layer 1 - RT Subaddress not defined");
      break;
   case ADT_ERR_RT_SA_CDP_UNDEF:
      return("Layer 1 - RT SA CDP not defined");
      break;
   case ADT_ERR_IQ_NO_NEW_ENTRY:
      return("Layer 1 - No new entry in interrupt queue");
      break;
   case ADT_ERR_NO_BCCB_TABLE:
      return("Layer 1 - BCCB Table Pointer is zero");
      break;
   case ADT_ERR_BCCB_ALREADY_ALLOCATED:
      return("Layer 1 - BCCB already allocated");
      break;
   case ADT_ERR_BCCB_NOT_ALLOCATED:
      return("Layer 1 - BCCB has not been allocated");
      break;
   case ADT_ERR_BUFFER_FULL:
      return("Layer 1 - 1553-ARINC PB (CDP/PCB or RXP/PXP) buffer is full");
      break;
   case ADT_ERR_TIMEOUT:
      return("Layer 1 - Timeout error");
      break;
   case ADT_ERR_BAD_CHAN_NUM:
      return("Layer 1 - Bad channel number, channel does not exist on this board or is not initialized");
      break;
   case ADT_ERR_BITFAIL:
      return("Layer 1 - Built-In Test failure");
      break;
   case ADT_ERR_DEVICEINUSE:
      return("Layer 1 - Device in use already, or not properly closed");
      break;
   case ADT_ERR_NO_TXCB_TABLE:
      return("Layer 1 - TXCB Table Pointer is zero");
      break;
   case ADT_ERR_TXCB_ALREADY_ALLOCATED:
      return("Layer 1 - TXCB already allocated");
      break;
   case ADT_ERR_TXCB_NOT_ALLOCATED:
      return("Layer 1 - TXCB has not been allocated");
      break;
   case ADT_ERR_PBCB_TOOMANYPXPS:
      return("Layer 1 - PBCB Too Many PXPs For PBCB Allocation");
      break;
   case ADT_ERR_NORXCHCVT_ALLOCATED:
      return("Layer 1 - RX CH - No CVT Option Defined at Init");
      break;
   case ADT_ERR_NO_DATA_AVAILABLE:
      return("Layer 1 - No Data Available");
      break;

   default:
      return("UNKNOWN ERROR/STATUS CODE!");
      break;
   }
}


/******************************************************************************
  FUNCTION:    ADT_L1_ENET_SetIpAddr
 *****************************************************************************/
/*! \brief Associates an IP ADDRESS with a DEVID (for ENET devices).
 *
 * This function associates an IP ADDRESS with a DEVID (for ENET devices).
 * The IP Address applies at the BOARD LEVEL (not channel/bank level).
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param ServerIpAddr is the server IP Address to associate with the DEVID.
 * @param ClientIpAddr is the client IP Address to associate with the DEVID.
 * @return
   - \ref ADT_SUCCESS - Successfully assigned IP address to the DEVID
   - \ref ADT_ERR_BAD_INPUT - DEVID is not for an ENET device.
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_ENET_SetIpAddr(ADT_L0_UINT32 devID, ADT_L0_UINT32 ServerIpAddr, ADT_L0_UINT32 ClientIpAddr) {
   ADT_L0_UINT32 result = ADT_SUCCESS;
   static ADT_L0_UINT32 firstTime = 1;
   ADT_L0_UINT32 backplaneType, boardType, boardNum;

   /* If this is the first time this function has been called, initialize array of IP addresses */
   if (firstTime)
   {
      memset(eNET_ServerIPaddr, 0, sizeof(eNET_ServerIPaddr));
      memset(eNET_ClientIPaddr, 0, sizeof(eNET_ClientIPaddr));
      firstTime = 0;
   }

   /* Break out the fields of the Device ID */
   backplaneType = devID & 0xF0000000;
   boardType =    devID & 0x0FF00000;
   boardNum =     devID & 0x000F0000;

   if (backplaneType == ADT_DEVID_BACKPLANETYPE_ENET)
   {
      eNET_ServerIPaddr[boardType >> 20][boardNum >> 16] = ServerIpAddr;
      eNET_ClientIPaddr[boardType >> 20][boardNum >> 16] = ClientIpAddr;
   }
   else result = ADT_ERR_BAD_INPUT;

   return( result );
}


/******************************************************************************
  FUNCTION:    ADT_L1_ENET_GetIpAddr
 *****************************************************************************/
/*! \brief Gets the IP ADDRESS associated with a DEVID (for ENET devices).
 *
 * This function gets the IP ADDRESS associated with a DEVID (for ENET devices).
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param pServerIpAddr is a pointer to store the server IP Address associated with the DEVID.
 * @param pClientIpAddr is a pointer to store the client IP Address associated with the DEVID.
 * @return
   - \ref ADT_SUCCESS - Successfully assigned IP address to the DEVID
   - \ref ADT_ERR_BAD_INPUT - DEVID is not for an ENET device.
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_ENET_GetIpAddr(ADT_L0_UINT32 devID, ADT_L0_UINT32 *pServerIpAddr, ADT_L0_UINT32 *pClientIpAddr) {
   ADT_L0_UINT32 result = ADT_SUCCESS;
   ADT_L0_UINT32 backplaneType, boardType, boardNum;

   /* Break out the fields of the Device ID */
   backplaneType = devID & 0xF0000000;
   boardType =    devID & 0x0FF00000;
   boardNum =     devID & 0x000F0000;

   if (backplaneType == ADT_DEVID_BACKPLANETYPE_ENET)
   {
      *pServerIpAddr = eNET_ServerIPaddr[boardType >> 20][boardNum >> 16];
      *pClientIpAddr = eNET_ClientIPaddr[boardType >> 20][boardNum >> 16];
   }
   else result = ADT_ERR_BAD_INPUT;

   return( result );
}


/******************************************************************************
  FUNCTION:    ADT_L1_ENET_ADCP_Reset
 *****************************************************************************/
/*! \brief Resets the ENET device (ENET Global Device only).
 *
 * This function resets the ENET device (ENET Global Device only).
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @return
   - \ref ADT_SUCCESS - Completed without error
   - \ref ADT_ERR_UNSUPPORTED_BACKPLANE - Not an ENET device
   - \ref ADT_ERR_UNSUPPORTED_CHANNELTYPE - Not a GLOBAL channel/device
   - \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_ENET_ADCP_Reset(ADT_L0_UINT32 devID) {
   ADT_L0_UINT32 result = ADT_SUCCESS;

   result = ADT_L0_ENET_ADCP_Reset(devID);

   return( result );
}


/******************************************************************************
  FUNCTION:    ADT_L1_ENET_ADCP_GetStatistics
 *****************************************************************************/
/*! \brief Gets the transaction and error counts for an ENET device.
 *
 * This function gets the transaction and error counts for an ENET device.
 *
 * @param devID is the 32-bit Device Identifier.
 * @param pPortNum is a pointer to store the UDP port number.
 * @param pTransactions is a pointer to store the transaction count.
 * @param pRetries is a pointer to store the retry count.
 * @param pFailures is a pointer to store the failure count.
 * @return
   - \ref ADT_SUCCESS - Completed without error
   - \ref ADT_ERR_UNSUPPORTED_BACKPLANE - Not an ENET device
   - \ref ADT_ERR_ENET_NOTRUNNING - No ENET devices have been initialized
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_ENET_ADCP_GetStatistics(ADT_L0_UINT32 devID, ADT_L0_UINT32 *pPortNum, ADT_L0_UINT32 *pTransactions, ADT_L0_UINT32 *pRetries, ADT_L0_UINT32 *pFailures)
{
   ADT_L0_UINT32 result = ADT_SUCCESS;

   result = ADT_L0_ENET_ADCP_GetStatistics(devID, pPortNum, pTransactions, pRetries, pFailures);

   return( result );
}


/******************************************************************************
  FUNCTION:    ADT_L1_ENET_ADCP_ClearStatistics
 *****************************************************************************/
/*! \brief Clears transaction and error counts for an ENET device
 *
 * This function clears the transaction and error counts for an ENET device.
 *
 * @param devID is the 32-bit Device Identifier.
 * @return
   - \ref ADT_SUCCESS - Completed without error
   - \ref ADT_ERR_UNSUPPORTED_BACKPLANE - Not an ENET device
   - \ref ADT_ERR_ENET_NOTRUNNING - No ENET devices have been initialized
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_ENET_ADCP_ClearStatistics(ADT_L0_UINT32 devID)
{
   ADT_L0_UINT32 result = ADT_SUCCESS;

   result = ADT_L0_ENET_ADCP_ClearStatistics(devID);

   return( result );
}



/******************************************************************************
  FUNCTION:    ADT_L1_DevicePresent
 *****************************************************************************/
/*! \brief Determines if a given device is present.
 *
 * This function polls a device - attempts to map memory to check if the device
 * is present in the system.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param config is the pointer to store the value of the configuration register.
 * @param serNum is the pointer to store the serial number.
 * @return
   - \ref ADT_SUCCESS - Device is present
   - \ref ADT_FAILURE - Device is not present
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_DevicePresent(ADT_L0_UINT32 devID, ADT_L0_UINT32 *config, ADT_L0_UINT32 *serNum) {
   ADT_L0_UINT32 result = ADT_SUCCESS;
   ADT_L0_UINT32 backplaneType, boardType, boardNum;
   ADT_L0_UINT32 channelNum, channelMask;
   ADT_L0_UINT32 prodIdRev;

   *config = 0x00000000;
   *serNum = 0x00000000;

   channelNum = 0;
   channelMask = 0;

   prodIdRev = 0;

   /* Break out the fields of the Device ID */
   backplaneType = devID & 0xF0000000;
   boardType =    devID & 0x0FF00000;
   boardNum =     devID & 0x000F0000;

   if (backplaneType == ADT_DEVID_BACKPLANETYPE_ENET)
   {
      /* Map memory */
      result = ADT_L0_MapMemory( devID, 0, eNET_ClientIPaddr[boardType >> 20][boardNum >> 16], eNET_ServerIPaddr[boardType >> 20][boardNum >> 16] );

      /* Check Product ID */
      if (result == ADT_SUCCESS) {
         result = ADT_L0_ReadMem32(devID, ADT_L1_GLOBAL_PRODIDREV, &prodIdRev, 1);
         if (result == ADT_SUCCESS) {
            switch (devID & 0x0FF00000) {
               case ADT_DEVID_BOARDTYPE_ENET1553:
                  if ((prodIdRev >> 16) != ADT_L1_GLOBAL_PRODIDREV_ENET1553)
                     result = ADT_ERR_ENET_BADPRODUCTID;
                  break;

               case ADT_DEVID_BOARDTYPE_PMCE1553:
                  if ((prodIdRev >> 16) != ADT_L1_GLOBAL_PRODIDREV_PMCE1553)
                     result = ADT_ERR_ENET_BADPRODUCTID;
                  break;

               case ADT_DEVID_BOARDTYPE_ENETA429:
                  if ((prodIdRev >> 16) != ADT_L1_GLOBAL_PRODIDREV_ENETA429)
                     result = ADT_ERR_ENET_BADPRODUCTID;
                  break;

               case ADT_DEVID_BOARDTYPE_ENET1A1553:
                  if ((prodIdRev >> 16) != ADT_L1_GLOBAL_PRODIDREV_ENET1A1553)
                     result = ADT_ERR_ENET_BADPRODUCTID;
                  break;

                case ADT_DEVID_BOARDTYPE_ENET485:
                  if ((prodIdRev >> 16) != ADT_L1_GLOBAL_PRODIDREV_ENET485)
                     result = ADT_ERR_ENET_BADPRODUCTID;
                  break;

               case ADT_DEVID_BOARDTYPE_ENET2_1553:
                  if ((prodIdRev >> 16) != ADT_L1_GLOBAL_PRODIDREV_ENET2_1553)
                     result = ADT_ERR_ENET_BADPRODUCTID;
                  break;

               case ADT_DEVID_BOARDTYPE_ENET_MA4:
                  if ((prodIdRev >> 16) != ADT_L1_GLOBAL_PRODIDREV_ENET_MA4)
                     result = ADT_ERR_ENET_BADPRODUCTID;
                  break;

               case ADT_DEVID_BOARDTYPE_ENETX_MA4:
                  if ((prodIdRev >> 16) != ADT_L1_GLOBAL_PRODIDREV_ENETX_MA4)
                     result = ADT_ERR_ENET_BADPRODUCTID;
                  break;

               case ADT_DEVID_BOARDTYPE_ENETA429P:
                  if ((prodIdRev >> 16) != ADT_L1_GLOBAL_PRODIDREV_ENETA429P)
                     result = ADT_ERR_ENET_BADPRODUCTID;
                  break;

              default:
                  result = ADT_ERR_BAD_INPUT;
                  break;
            }
         }

       /* if error unmap memory */
       if (result != ADT_SUCCESS) (void) ADT_L0_UnmapMemory( devID );

      }
   }
   else
   {
      /* Map memory */
      result = ADT_L0_MapMemory( devID, 0, 0, 0 );
   }

   if (result == ADT_SUCCESS)
   {
      /* Read the board serial number */
      result = ADT_L0_ReadMem32(devID, ADT_L1_GLOBAL_SERNUM, serNum, 1);

      /* Read the board config register */
      result = ADT_L0_ReadMem32(devID, ADT_L1_GLOBAL_CAPREG, config, 1);

      /* This only applies to channels other than global registers */
      if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_GLOBALS)
      {

         /* If we successfully mapped memory, then . . . */
         if (result == ADT_SUCCESS) {
            /* Check if channel exists */
            /* For 1553, channel bits in lower 8-bits */
            if ((devID & 0x0000FF00) == ADT_DEVID_CHANNELTYPE_1553)
            {
               channelNum = devID & 0x000000FF;
               switch (channelNum) {
                  case 0:
                     channelMask = 0x00000001;
                     break;
                  case 1:
                     channelMask = 0x00000002;
                     break;
                  case 2:
                     channelMask = 0x00000004;
                     break;
                  case 3:
                     channelMask = 0x00000008;
                     break;
                  case 4:
                     channelMask = 0x00000010;
                     break;
                  case 5:
                     channelMask = 0x00000020;
                     break;
                  case 6:
                     channelMask = 0x00000040;
                     break;
                  case 7:
                     channelMask = 0x00000080;
                     break;
                  case 8:
                     channelMask = 0x00010000;
                     break;
                  case 9:
                     channelMask = 0x00020000;
                     break;
                  default:
                     channelMask = 0x00000000;
                     result = ADT_ERR_BAD_CHAN_NUM;
                     break;
               }
            }

            /* For A429 devices, only 1 or 2 banks/channels, bits 24 and 25 (bits 24-26 for PMCA429HD)*/
            if ((devID & 0x0000FF00) == ADT_DEVID_CHANNELTYPE_A429)
            {
               channelNum = devID & 0x000000FF;
               switch (channelNum) {
                  case 0:
                     channelMask = 0x00000001;
                     break;
                  case 1:
                     channelMask = 0x00000002;
                     break;
				  case 2:
					  if ((devID & 0x0FF00000) == ADT_DEVID_BOARDTYPE_PMCA429HD) {
						 channelMask = 0x00000004;
					  } else {
		                 channelMask = 0x00000000;
			             result = ADT_ERR_BAD_CHAN_NUM;
					  }
					 break;
                  default:
                     channelMask = 0x00000000;
                     result = ADT_ERR_BAD_CHAN_NUM;
                     break;
               }
               channelMask = channelMask << 24;
            }

            /* For WMUX devices, only 1 or 2 channels, bits 0 and 1 */
            if ((devID & 0x0000FF00) == ADT_DEVID_CHANNELTYPE_WMUX)
            {
               channelNum = devID & 0x000000FF;
               switch (channelNum) {
                  case 0:
                     channelMask = 0x00000001;
                     break;
                  case 1:
                     channelMask = 0x00000002;
                     break;
                  default:
                     channelMask = 0x00000000;
                     result = ADT_ERR_BAD_CHAN_NUM;
                     break;
               }
            }

            if ((*config & channelMask) == 0)
            {
               result = ADT_ERR_BAD_CHAN_NUM;
            }
         }
      }
      else  /* is a Global device */
      {
         channelNum = devID & 0x000000FF;

         /* Only one Global device allowed (channelNum 0) */
         if (channelNum != 0)
            result = ADT_ERR_BAD_CHAN_NUM;
      }

      /* unMap memory */
      (void) ADT_L0_UnmapMemory( devID );
   }

   if (result != ADT_SUCCESS) result = ADT_FAILURE;

   return( result );
}



/******************************************************************************
  FUNCTION:    ADT_L1_DevicePresent_pciInfo
 *****************************************************************************/
/*! \brief Determines if a given device is present, and returns PCI info.
 *
 * This function polls a device - attempts to map memory to check if the device
 * is present in the system.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param config is the pointer to store the value of the configuration register.
 * @param serNum is the pointer to store the serial number.
 * @param pciBus is the pointer to store the PCI bus number.
 * @param pciDevice is the pointer to store the PCI device number.
 * @param pciFunc is the pointer to store the PCI function number.
 * @return
   - \ref ADT_SUCCESS - Device is present
   - \ref ADT_FAILURE - Device is not present
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_DevicePresent_pciInfo(ADT_L0_UINT32 devID, ADT_L0_UINT32 *config, ADT_L0_UINT32 *serNum, ADT_L0_UINT32 *pciBus, ADT_L0_UINT32 *pciDevice, ADT_L0_UINT32 *pciFunc) {
   ADT_L0_UINT32 result = ADT_SUCCESS;
   ADT_L0_UINT32 configReg = 0x00000000;
   ADT_L0_UINT32 channelNum, channelMask;

   *config = 0x00000000;
   *serNum = 0x00000000;

   channelNum = 0;
   channelMask = 0;


  /* Map memory */
  result = ADT_L0_MapMemory_pciInfo( devID, 0, pciBus, pciDevice, pciFunc);

   if (result == ADT_SUCCESS)
   {
      /* Read the board serial number */
      result = ADT_L0_ReadMem32(devID, ADT_L1_GLOBAL_SERNUM, serNum, 1);

      /* Read the board config register */
      result = ADT_L0_ReadMem32(devID, ADT_L1_GLOBAL_CAPREG, &configReg, 1);
      *config = configReg;

      /* This only applies to channels other than global registers */
      if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_GLOBALS)
      {

         /* If we successfully mapped memory, then . . . */
         if (result == ADT_SUCCESS) {
            /* Check if channel exists */
            /* For 1553, channel bits in lower 8-bits */
            if ((devID & 0x0000FF00) == ADT_DEVID_CHANNELTYPE_1553)
            {
               channelNum = devID & 0x000000FF;
               switch (channelNum) {
                  case 0:
                     channelMask = 0x00000001;
                     break;
                  case 1:
                     channelMask = 0x00000002;
                     break;
                  case 2:
                     channelMask = 0x00000004;
                     break;
                  case 3:
                     channelMask = 0x00000008;
                     break;
                  case 4:
                     channelMask = 0x00000010;
                     break;
                  case 5:
                     channelMask = 0x00000020;
                     break;
                  case 6:
                     channelMask = 0x00000040;
                     break;
                  case 7:
                     channelMask = 0x00000080;
                     break;
                  case 8:
                     channelMask = 0x00010000;
                     break;
                  case 9:
                     channelMask = 0x00020000;
                     break;
                  default:
                     channelMask = 0x00000000;
                     result = ADT_ERR_BAD_CHAN_NUM;
                     break;
               }
            }

            /* For A429 devices, only 1 or 2 banks/channels, bits 24 and 25 (bits 24-26 for PMCA429HD)*/
            if ((devID & 0x0000FF00) == ADT_DEVID_CHANNELTYPE_A429)
            {
               channelNum = devID & 0x000000FF;
               switch (channelNum) {
                  case 0:
                     channelMask = 0x00000001;
                     break;
                  case 1:
                     channelMask = 0x00000002;
                     break;
				  case 2:
					  if ((devID & 0x0FF00000) == ADT_DEVID_BOARDTYPE_PMCA429HD) {
						 channelMask = 0x00000004;
					  } else {
		                 channelMask = 0x00000000;
			             result = ADT_ERR_BAD_CHAN_NUM;
					  }
					 break;
                  default:
                     channelMask = 0x00000000;
                     result = ADT_ERR_BAD_CHAN_NUM;
                     break;
               }
               channelMask = channelMask << 24;
            }

            /* For WMUX devices, only 1 or 2 channels, bits 0 and 1 */
            if ((devID & 0x0000FF00) == ADT_DEVID_CHANNELTYPE_WMUX)
            {
               channelNum = devID & 0x000000FF;
               switch (channelNum) {
                  case 0:
                     channelMask = 0x00000001;
                     break;
                  case 1:
                     channelMask = 0x00000002;
                     break;
                  default:
                     channelMask = 0x00000000;
                     result = ADT_ERR_BAD_CHAN_NUM;
                     break;
               }
            }

            if ((configReg & channelMask) == 0)
            {
               result = ADT_ERR_BAD_CHAN_NUM;
            }
         }
      }
      else  /* is a Global device */
      {
         channelNum = devID & 0x000000FF;

         /* Only one Global device allowed (channelNum 0) */
         if (channelNum != 0)
            result = ADT_ERR_BAD_CHAN_NUM;
      }

      /* unMap memory */
      (void) ADT_L0_UnmapMemory( devID );
   }

   if (result != ADT_SUCCESS) result = ADT_FAILURE;

   return( result );
}



/******************************************************************************
  FUNCTION:    ADT_L1_InitDevice
 *****************************************************************************/
/*! \brief Initializes a device
 *
 * This function initializes a device - maps memory and initializes
 * memory management.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param startupOptions control the following initialization options:
 * ADT_L1_API_DEVICEINIT_FORCEINIT           0x00000001
 * ADT_L1_API_DEVICEINIT_NOMEMTEST           0x00000002
 * ADT_L1_API_DEVICEINIT_ROOTPERESET         0x00000004
 * @return
   - \ref ADT_SUCCESS - Completed without error
   - \ref ADT_ERR_BAD_CHAN_NUM - Bad channel number or channel doesn't exist on this board
   - \ref ADT_ERR_BITFAIL - Failed BIT
   - \ref ADT_ERR_DEVICEINUSE - Device in use
   - \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_InitDevice(ADT_L0_UINT32 devID, ADT_L0_UINT32 startupOptions) {
   ADT_L0_UINT32 result = ADT_SUCCESS;
   ADT_L0_UINT32 backplaneType, boardType, boardNum, channelType;
   ADT_L0_UINT32 configReg = 0x00000000;
   ADT_L0_UINT32 channelNum = 0;
   ADT_L0_UINT32 channelMask, bitStatus, inUseReg, data;
   ADT_L0_UINT32 channel, channelRegOffset, prodIdRev, counter;

   channelMask = bitStatus = inUseReg = channel = channelRegOffset = prodIdRev = 0;

   /* Get offset to the channel registers */
   result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
   if (result != ADT_SUCCESS) return(result);

   /* Break out the fields of the Device ID */
   backplaneType = devID & 0xF0000000;
   boardType =     devID & 0x0FF00000;
   boardNum =      devID & 0x000F0000;
   channelType =   devID & 0x0000FF00;
   channelNum =	   devID & 0x000000FF;

   /* ADT_L1_MemMgmt.c - ADT_L1_MEMMGMT constants set size of dmmArray - DO NOT EXCEED BOUNDS OF THIS ARRAY */
   if ((backplaneType >> 28) > (ADT_L1_MEMMGT_NUM_BACKPLANE - 1)) return(ADT_ERR_UNSUPPORTED_BACKPLANE);
   if ((boardType >> 20) >     (ADT_L1_MEMMGT_NUM_BOARDTYPE - 1)) return(ADT_ERR_UNSUPPORTED_BOARDTYPE);
   if ((boardNum >> 16) >      (ADT_L1_MEMMGT_NUM_BOARDNUM - 1)) return(ADT_ERR_INVALID_BOARD_NUM);
   if ((channelType >> 8) >    (ADT_L1_MEMMGT_NUM_CHANTYPE - 1)) return(ADT_ERR_UNSUPPORTED_CHANNELTYPE);
   if (channelNum >			   (ADT_L1_MEMMGT_NUM_CHANNUM - 1)) return(ADT_ERR_INVALID_CHANNEL_NUM);

   if (backplaneType == ADT_DEVID_BACKPLANETYPE_ENET)
   {
      /* Map memory */
      result = ADT_L0_MapMemory( devID, startupOptions, eNET_ClientIPaddr[boardType >> 20][boardNum >> 16], eNET_ServerIPaddr[boardType >> 20][boardNum >> 16] );

      /* Check Product ID */
      if (result == ADT_SUCCESS) {
         result = ADT_L0_ReadMem32(devID, ADT_L1_GLOBAL_PRODIDREV, &prodIdRev, 1);
         if (result == ADT_SUCCESS) {
            switch (devID & 0x0FF00000) {
               case ADT_DEVID_BOARDTYPE_ENET1553:
                  if ((prodIdRev >> 16) != ADT_L1_GLOBAL_PRODIDREV_ENET1553)
                     result = ADT_ERR_ENET_BADPRODUCTID;
                  break;

               case ADT_DEVID_BOARDTYPE_PMCE1553:
                  if ((prodIdRev >> 16) != ADT_L1_GLOBAL_PRODIDREV_PMCE1553)
                     result = ADT_ERR_ENET_BADPRODUCTID;
                  break;

               case ADT_DEVID_BOARDTYPE_ENETA429:
                  if ((prodIdRev >> 16) != ADT_L1_GLOBAL_PRODIDREV_ENETA429)
                     result = ADT_ERR_ENET_BADPRODUCTID;
                  break;

               case ADT_DEVID_BOARDTYPE_ENETA429P:
                  if ((prodIdRev >> 16) != ADT_L1_GLOBAL_PRODIDREV_ENETA429P)
                     result = ADT_ERR_ENET_BADPRODUCTID;
                  break;

               case ADT_DEVID_BOARDTYPE_ENET1A1553:
                  if ((prodIdRev >> 16) != ADT_L1_GLOBAL_PRODIDREV_ENET1A1553)
                     result = ADT_ERR_ENET_BADPRODUCTID;
                  break;

                case ADT_DEVID_BOARDTYPE_ENET485:
                  if ((prodIdRev >> 16) != ADT_L1_GLOBAL_PRODIDREV_ENET485)
                     result = ADT_ERR_ENET_BADPRODUCTID;
                  break;

               case ADT_DEVID_BOARDTYPE_ENET2_1553:
                  if ((prodIdRev >> 16) != ADT_L1_GLOBAL_PRODIDREV_ENET2_1553)
                     result = ADT_ERR_ENET_BADPRODUCTID;
                  break;

               case ADT_DEVID_BOARDTYPE_ENET_MA4:
                  if ((prodIdRev >> 16) != ADT_L1_GLOBAL_PRODIDREV_ENET_MA4)
                     result = ADT_ERR_ENET_BADPRODUCTID;
                  break;

               case ADT_DEVID_BOARDTYPE_ENETX_MA4:
                  if ((prodIdRev >> 16) != ADT_L1_GLOBAL_PRODIDREV_ENETX_MA4)
                     result = ADT_ERR_ENET_BADPRODUCTID;
                  break;

              default:
                  result = ADT_ERR_BAD_INPUT;
                  break;
            }
			if (result != ADT_SUCCESS) ADT_L0_UnmapMemory( devID );
         }
      }
   }
   else
   {
      /* Map memory */
      result = ADT_L0_MapMemory( devID, startupOptions, 0, 0 );
   }

   if (result == ADT_SUCCESS)
   {
      /* This only applies to channels other than global registers */
      if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_GLOBALS)
      {
         /* If we successfully mapped memory, then . . . */
         if (result == ADT_SUCCESS) {
            /* Check if channel exists */
            /* For 1553, channel bits in lower 8-bits */
            channelNum = devID & 0x000000FF;
            switch (channelNum) {
               case 0:
                  channelMask = 0x00000001;
                  break;
               case 1:
                  channelMask = 0x00000002;
                  break;
               case 2:
                  channelMask = 0x00000004;
                  break;
               case 3:
                  channelMask = 0x00000008;
                  break;
               case 4:
                  channelMask = 0x00000010;
                  break;
               case 5:
                  channelMask = 0x00000020;
                  break;
               case 6:
                  channelMask = 0x00000040;
                  break;
               case 7:
                  channelMask = 0x00000080;
                  break;
               case 8:
                  channelMask = 0x00010000;
                  break;
               case 9:
                  channelMask = 0x00020000;
                  break;
               default:
                  result = ADT_ERR_BAD_CHAN_NUM;
                  break;
            }

			/* For A429 devices, only 1 or 2 banks/channels, bits 24 and 25 (bits 24-26 for PMCA429HD)*/
            if ((devID & 0x0000FF00) == ADT_DEVID_CHANNELTYPE_A429)
            {
				if ((devID & 0x0FF00000) == ADT_DEVID_BOARDTYPE_PMCA429HD) {
				   channelMask &= 0x00000007;
				} else {
				   channelMask &= 0x00000003;
				}
				channelMask = channelMask << 24;
            }

            if (result == ADT_SUCCESS)
            {
               result = ADT_L0_ReadMem32(devID, ADT_L1_GLOBAL_CAPREG, &configReg, 1);
               if ((configReg & channelMask) == 0)
               {
                  result = ADT_ERR_BAD_CHAN_NUM;
               }
            }
         }

         if (result == ADT_SUCCESS) {
           /* Check IN USE register */
           result = ADT_L0_ReadMem32(devID, channelRegOffset + ADT_L1_PE_INUSE, &inUseReg, 1);
           if ((inUseReg == 0xFFFFFFFF) && (!(startupOptions & ADT_L1_API_DEVICEINIT_FORCEINIT))) {  /* Channel already in use */
              result = ADT_ERR_DEVICEINUSE;
           }
           else {  /* Channel is available */
			  /* Clear BIT STATUS register to make sure it does not start with a non-zero value */
			  bitStatus = 0x00000000;
			  result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_PE_BITSTATUS, &bitStatus, 1);
           }
         }

         /* If channel is good, then . . . */
         if (result == ADT_SUCCESS) {
            /* Check for PE Hard Device Reset Option from User - This is a hard clear of the PE Device Registers */
            if (startupOptions & ADT_L1_API_DEVICEINIT_ROOTPERESET) {
               data = ADT_L1_1553_PECSR_CHAN_RESET;
               result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_1553_PE_ROOT_CSR, &data, 1);
            }

			/* Ensure POST/IBIT/PBIT has finished - this is important for PE RESET */
			counter = 0;
            result = ADT_L1_BIT_PeriodicBIT(devID, &bitStatus);
			while ((result == ADT_SUCCESS) && ((bitStatus & 0xF0000000) != 0x00000000) && (counter++ < 1000)) {
               result = ADT_L1_BIT_PeriodicBIT(devID, &bitStatus);
			}
			if (counter >= 1000) result = ADT_ERR_TIMEOUT;

            /* Check POST/PBIT status */
            if (result == ADT_SUCCESS) {
               bitStatus = 0;
               result = ADT_L1_BIT_PeriodicBIT(devID, &bitStatus);
               if (bitStatus != 0x00000000) {
                  result = ADT_ERR_BITFAIL;
               }
            }
         }

         /* If POST/PBIT passed, then . . . */
         /* If channel is available, then . . . */
         if (result == ADT_SUCCESS) {
		    /* Ensure mem mgmt is closed (for cases where we are forcing init after abnormal exit) */
		    result = ADT_L1_CloseMemMgmt( devID );

		    /* Initialize memory management for the device, also tests memory */
            result = ADT_L1_InitMemMgmt(devID, startupOptions);
         }

		 /* If SUCCESSFUL, set "IN USE" flag */
         if (result == ADT_SUCCESS) {
              inUseReg = 0xFFFFFFFF;
              result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_PE_INUSE, &inUseReg, 1);
		 }

      }
   }

   return( result );
}



/******************************************************************************
  FUNCTION:    ADT_L1_CloseDevice
 *****************************************************************************/
/*! \brief Closes a device
 *
 * This function un-maps memory and closes the device.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @return
   - \ref ADT_SUCCESS - Completed without error
   - \ref ADT_FAILURE - ADT_L0_UnmapMemory failed
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_CloseDevice(ADT_L0_UINT32 devID) {
   ADT_L0_UINT32 result = ADT_SUCCESS;
   ADT_L0_UINT32 channel, channelRegOffset, inUseReg;

   /* This only applies to channels other than global registers */
   if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_GLOBALS)
   {
      result = ADT_L1_CloseMemMgmt( devID );

      result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);

      if (result == ADT_SUCCESS) {
         /* Mark the channel as NOT IN USE */
         inUseReg = 0x00000000;
         result = ADT_L0_WriteMem32(devID, channelRegOffset + ADT_L1_PE_INUSE, &inUseReg, 1);

         result = ADT_L0_UnmapMemory( devID );
      }
   }
   else
      result = ADT_L0_UnmapMemory( devID );

   return( result );
}



/******************************************************************************
  FUNCTION:    ADT_L1_GetVersionInfo
 *****************************************************************************/
/*! \brief Gets version information
 *
 * This function gets PE (firmware version and the Layer 0 and Layer 1 API
 * version numbers.
 *
 * The PE version is a 16-bit unsigned integer interpreted as a hexadecimal
 * version number.
 *
 * For API version numbers, the returned value is a 32-bit unsigned integer.
 * This is interpreted in octets similar to an IP address.
 * For example, a value of 0x12345678 would be interpreted by
 * first separating the octets (or bytes) to 0x12.0x34.0x56.0x78, then converting
 * these octets to decimal values resulting in version 18.52.86.120.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param peVersion is a pointer to store the PE (firmware) version.
 * @param layer0ApiVersion is a pointer to store the L0 API version.
 * @param layer1ApiVersion is a pointer to store the L1 API version.
 * @return
   - \ref ADT_SUCCESS - Completed without error
   - \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_GetVersionInfo(ADT_L0_UINT32 devID,
                           ADT_L0_UINT16 *peVersion,
                           ADT_L0_UINT32 *layer0ApiVersion,
                           ADT_L0_UINT32 *layer1ApiVersion) {
   ADT_L0_UINT32 result = ADT_SUCCESS;
   ADT_L0_UINT32 peVerReg;

   *layer0ApiVersion = ADT_L0_API_VERSION;
   *layer1ApiVersion = ADT_L1_API_VERSION;

   /* This applies to globals */
   if ((devID & 0x0000FF00) == ADT_DEVID_CHANNELTYPE_GLOBALS)
   {
      result = ADT_L1_ReadDeviceMem32(devID, ADT_L1_GLOBAL_PRODIDREV, &peVerReg, 1);
      *peVersion = (ADT_L0_UINT16) (peVerReg & 0x0000FFFF);
   }
   /* This only applies to all devices other than globals */
   else
   {
      result = ADT_L1_ReadDeviceMem32(devID, ADT_L1_PE_ROOT_IDVER, &peVerReg, 1);
      *peVersion = (ADT_L0_UINT16) (peVerReg & 0x0000FFFF);
   }

   return( result );
}


/******************************************************************************
  FUNCTION:    ADT_L1_GetBoardInfo
 *****************************************************************************/
/*! \brief Gets board information
 *
 * This function gets the BOARD-LEVEL information from the global registers on
 * the board.  These are described in detail in the AltaCore Protocol
 * Engine Reference Manual.
 *
 * Product ID and Revision - Product ID in upper 16-bits, revision in lower 16-bits.
 * Capabilities Register - See AltaCore Protocol Engine Reference Manual.
 * Serial Number - This is the board serial number.
 * Align Check Word - This should always be 0x12345678.  This value can be read
 * to check for problems with byte or word swapping when porting to a new OS or
 * platform.
 * Memory Size - This provides the total memory size for the board in bytes.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param prodIDandRev is a pointer to store the product ID and Revision.
 * @param capabilitiesReg is a pointer to store the capabilities register.
 * @param serialNumber is a pointer to store the serial number.
 * @param alignCheck is a pointer to store the align check word.
 * @param memorySize is a pointer to store the memory size.
 * @return
   - \ref ADT_SUCCESS - Completed without error
   - \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_GetBoardInfo(ADT_L0_UINT32 devID,
                           ADT_L0_UINT32 *prodIDandRev,
                           ADT_L0_UINT32 *capabilitiesReg,
                           ADT_L0_UINT32 *serialNumber,
                           ADT_L0_UINT32 *alignCheck,
                           ADT_L0_UINT32 *memorySize) {
   ADT_L0_UINT32 result = ADT_SUCCESS;

	if ((devID & 0x0000FF00) != ADT_DEVID_CHANNELTYPE_GLOBALS)
		result = ADT_ERR_UNSUPPORTED_CHANNELTYPE;
	else
	{
		result = ADT_L0_ReadMem32(devID, ADT_L1_GLOBAL_PRODIDREV, prodIDandRev, 1);
		result |= ADT_L0_ReadMem32(devID, ADT_L1_GLOBAL_CAPREG, capabilitiesReg, 1);
		result |= ADT_L0_ReadMem32(devID, ADT_L1_GLOBAL_SERNUM, serialNumber, 1);
		result |= ADT_L0_ReadMem32(devID, ADT_L1_GLOBAL_ALIGNCHECK, alignCheck, 1);
		result |= ADT_L0_ReadMem32(devID, ADT_L1_GLOBAL_MEMSIZE, memorySize, 1);
	}

	return( result );
}


/******************************************************************************
  FUNCTION:    ADT_L1_ReadDeviceMem32
 *****************************************************************************/
/*! \brief Reads memory from a device in 32-bit words.
 *
 * This function reads memory from a device in 32-bit words.  The API determines
 * the offset to the selected channel internally, so the memory offset to read
 * provided by the caller is an offset in CHANNEL memory rather than BOARD
 * memory.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param offset is the BYTE offset from the start of CHANNEL memory (not an offset from start of board memory).
 * @param pData is a pointer to store the word(s) read.
 * @param count is the number of 32-bit words to read.  FOR ENET DEVICES CANNOT EXCEED 360.
 * @return
   - \ref ADT_SUCCESS - Completed without error
   - \ref ADT_FAILURE - Completed with error
   - \ref ADT_ERR_ENET_INVALID_SIZE - For ENET device, count exceeds 360.
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_ReadDeviceMem32(ADT_L0_UINT32 devID, ADT_L0_UINT32 offset, ADT_L0_UINT32 *pData, ADT_L0_UINT32 count) {
   ADT_L0_UINT32 result = ADT_SUCCESS;
   ADT_L0_UINT32 channel, channelRegOffset;

   /* Get offset to the channel registers */
   result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
   if (result != ADT_SUCCESS)
      return(result);

   result = ADT_L0_ReadMem32(devID, channelRegOffset + offset, pData, count);

   return( result );
}



/******************************************************************************
  FUNCTION:    ADT_L1_WriteDeviceMem32
 *****************************************************************************/
/*! \brief Writes memory to a device in 32-bit words.
 *
 * This function writes memory to a device in 32-bit words.  The API determines
 * the offset to the selected channel internally, so the memory offset to write
 * provided by the caller is an offset in CHANNEL memory rather than BOARD
 * memory.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param offset is the BYTE offset from the start of CHANNEL memory (not an offset from start of board memory).
 * @param pData is a pointer to the word(s) to be written.
 * @param count is the number of 32-bit words to write.  FOR ENET DEVICES CANNOT EXCEED 360.
 * @return
   - \ref ADT_SUCCESS - Completed without error
   - \ref ADT_FAILURE - Completed with error
   - \ref ADT_ERR_ENET_INVALID_SIZE - For ENET device, count exceeds 360.
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_WriteDeviceMem32(ADT_L0_UINT32 devID, ADT_L0_UINT32 offset, ADT_L0_UINT32 *pData, ADT_L0_UINT32 count) {
   ADT_L0_UINT32 result = ADT_SUCCESS;
   ADT_L0_UINT32 channel, channelRegOffset;

   /* Get offset to the channel registers */
   result = Internal_GetChannelRegOffset(devID, &channel, &channelRegOffset);
   if (result != ADT_SUCCESS)
      return(result);

   result = ADT_L0_WriteMem32(devID, channelRegOffset + offset, pData, count);

   return( result );
}



/******************************************************************************
  FUNCTION:    ADT_L1_ProgramBoardFlash
 *****************************************************************************/
/*! \brief Programs the FLASH memory for the BOARD.
 *
 * This function programs the FLASH memory for the BOARD.
 * WARNING - This affects ALL channels on the board.  Do not use this function
 * if other applications are using other channels on the board.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param numBytes is the number of bytes to load.
 * @param fpga_load_bytes is a pointer to the bytes to load.
 * @return
   - \ref ADT_SUCCESS - Completed without error
   - \ref ADT_ERR_BAD_DEV_TYPE - Unsupported board type
   - \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_ProgramBoardFlash(ADT_L0_UINT32 devID, ADT_L0_UINT32 numBytes, ADT_L0_UINT8 * fpga_load_bytes)
{
   ADT_L0_UINT8  data;
   ADT_L0_UINT32 result = ADT_SUCCESS;
   ADT_L0_UINT32 i, j, sync_found;
   ADT_L0_UINT32 sync_data, byte_count, temp32, byteIndex, save_data;
   ADT_L0_UINT16 word_data;
   ADT_L0_UINT32 boardType;
   ADT_L0_UINT32 cpld_csr_offset;
   ADT_L0_UINT32 flash_erase_offset;
   ADT_L0_UINT32 sync_pattern;
   ADT_L0_UINT16 cmd_set;
   ADT_L0_UINT32 geometry[8];
   ADT_L0_UINT32 *prog_count;
   ADT_L0_UINT8  swap_bytes;

   word_data = 0;
   sync_data = 0;
   byte_count = 0;
   sync_found = 0;
	swap_bytes = 0;

   boardType = devID & 0x0FF00000;
	if ((boardType == ADT_DEVID_BOARDTYPE_ECD54_1553)
			|| (boardType == ADT_DEVID_BOARDTYPE_ECD54_A429)
			|| (boardType == ADT_DEVID_BOARDTYPE_MPCIE1553)
			|| (boardType == ADT_DEVID_BOARDTYPE_MPCIE21553)
			|| (boardType == ADT_DEVID_BOARDTYPE_ENET1A1553)
			|| (boardType == ADT_DEVID_BOARDTYPE_MPCIEA429)
			|| (boardType == ADT_DEVID_BOARDTYPE_TBOLTMA4)
		) {
      result = Internal_ProgramEcdFlash(devID, numBytes, fpga_load_bytes);
      return (result);
   }
   else if ((boardType == ADT_DEVID_BOARDTYPE_ENET1553)
			|| (boardType == ADT_DEVID_BOARDTYPE_PMCE1553)
			|| (boardType == ADT_DEVID_BOARDTYPE_ENETA429)
			|| (boardType == ADT_DEVID_BOARDTYPE_ENET485)
			|| (boardType == ADT_DEVID_BOARDTYPE_ENET2_1553)
			|| (boardType == ADT_DEVID_BOARDTYPE_ENET_MA4)
			|| (boardType == ADT_DEVID_BOARDTYPE_ENETX_MA4)
			|| (boardType == ADT_DEVID_BOARDTYPE_ENETA429P)
		) {
      result = Internal_ProgramEnetFlash(devID, numBytes, fpga_load_bytes);
      return (result);
   }
   else if ((boardType == ADT_DEVID_BOARDTYPE_PC104PMA4)
			|| (boardType == ADT_DEVID_BOARDTYPE_PC104PA429LTV)
		) {
      /* the size of the FPGA bit stream file for 6SLX75 is ~2.4Mbyte. */
      sync_pattern = 0x5599AA66;
      cpld_csr_offset = 0x00370000;
   }
   else if ((boardType == ADT_DEVID_BOARDTYPE_PCIE4L1553)
			|| (boardType == ADT_DEVID_BOARDTYPE_PCIE1L1553)
			|| (boardType == ADT_DEVID_BOARDTYPE_XMCMA4)
			|| (boardType == ADT_DEVID_BOARDTYPE_XMCA429)
			|| (boardType == ADT_DEVID_BOARDTYPE_PMCA429HD)
			|| (boardType == ADT_DEVID_BOARDTYPE_PCIE4LA429)
			|| (boardType == ADT_DEVID_BOARDTYPE_PCIE1LA429)
			|| (boardType == ADT_DEVID_BOARDTYPE_PC104EMA4)
      ) {
      /* the size of the FPGA bit stream file for ECP3-70 is ~2.4Mbyte. */
      sync_pattern = 0xFFFFBDCD;
      cpld_csr_offset = 0x00370000;
   }
   else if ((boardType == ADT_DEVID_BOARDTYPE_XMCMW) || (boardType == ADT_DEVID_BOARDTYPE_XMC1553)
		) { 
      /* the size of the FPGA bit stream file for Artix7 is ~9.5Mbyte. */
		swap_bytes = 1;
      sync_pattern = 0x5599AA66;
      cpld_csr_offset = 0x00FF0000;
   }
   else if ((boardType == ADT_DEVID_BOARDTYPE_PMCMA4)
		) {
      /* the size of the FPGA bit stream file for EPC2-70 is ~1.8Mbyte. */
      sync_pattern  = 0xFFFFBDCD;
      cpld_csr_offset = 0x00200000;
   }
   else if ((boardType == ADT_DEVID_BOARDTYPE_PMC1553)
			|| (boardType == ADT_DEVID_BOARDTYPE_PC104P1553)
			|| (boardType == ADT_DEVID_BOARDTYPE_PCI1553)
			|| (boardType == ADT_DEVID_BOARDTYPE_PMCA429)
			|| (boardType == ADT_DEVID_BOARDTYPE_PC104PA429)
			|| (boardType == ADT_DEVID_BOARDTYPE_PCIA429)
			|| (boardType == ADT_DEVID_BOARDTYPE_PMCWMUX)
      ) {
      /* the size of the FPGA bit stream file for spartan3-2000 is ~1Mbyte. */
      sync_pattern = 0x5599AA66;
      cpld_csr_offset = 0x00200000;
   }
   else return(ADT_ERR_UNSUPPORTED_BOARDTYPE);

   /* Set PLX Usero pin low. This is the CPLD select signal */
   temp32 = 0x100C767E & ~0x00010000;
   ADT_L0_WriteSetupMem32(devID, 0x6C, &temp32, 1);

   /* enable flash access in CPLD */
   temp32 = 0x00000004;
   ADT_L0_WriteMem32(devID, cpld_csr_offset, &temp32, 1);

   /* put LB in 16-bit mode  */
   temp32 = 0x42030341;
   ADT_L0_WriteSetupMem32(devID, 0x18, &temp32, 1);

   /* get flash erase sector sizes (geometry) and command set for CFI flash devices */
   result = Internal_read_flash_CFI(devID, &cmd_set, geometry);

   if(result == ADT_FAILURE) {
      /* put flash back into read array mode */
      cmd_set = 0x00FF; /* AMD Reset */
      ADT_L0_WriteMem16(devID, 0x00000000, &cmd_set, 1);
      cmd_set = 0x00F0; /* Intel Read Array */
      ADT_L0_WriteMem16(devID, 0x00000000, &cmd_set, 1);

      /* disable flash access in CPLD */
      temp32 = 0x00000000;
      ADT_L0_WriteMem32(devID, cpld_csr_offset, &temp32, 1);

      /* put LB back in 32-bit mode  */
      temp32 = 0x42030343;
      ADT_L0_WriteSetupMem32(devID, 0x18, &temp32, 1);

      /* Set PLX usero pin. */
      temp32 = 0x100C767E | 0x00010000;
      ADT_L0_WriteSetupMem32(devID, 0x6C, &temp32, 1);

      return (result);
   }

   /* Flash sector size may be different between flash devices.
      Erase the size of the file (header and data) + 4 bytes. */
   flash_erase_offset = numBytes + 4;

   /* zero the first 4 bytes of fpga_load_bytes array. These locations are used to keep track of the
      number of program bytes written to the device. */
   prog_count = (ADT_L0_UINT32*)fpga_load_bytes;
   save_data = prog_count[0];
   prog_count[0] = 0;

   /* erase flash blocks */
   for(i=0, j=0; j<4; j++) {
      for(;i<flash_erase_offset && geometry[j*2]>0;i+=geometry[j*2+1],geometry[j*2]--) {
         if(cmd_set == 2) result = Internal_erase_flash_block_AMD_Std(devID, i);
         if(cmd_set == 3) result = Internal_erase_flash_block_Intel_Std(devID, i);
         prog_count[0] = i;
	      if (result != ADT_SUCCESS) break;
      }
      if (result != ADT_SUCCESS) break;
   }
	if(i > flash_erase_offset) prog_count[0] = numBytes;

	/* give thread time to update % automatically */
	ADT_L0_msSleep(1000);

   /* load fpga data into flash */
   for (byteIndex=0; byteIndex<numBytes+1 && result == ADT_SUCCESS; byteIndex++)
   {
		data = (byteIndex<numBytes) ? fpga_load_bytes[byteIndex] : 0xFF;

		/* swap bits - data byte to fpga is swapped */
		data = ((data & 0x01) << 7) + ((data & 0x02) << 5) + ((data & 0x04) << 3) + ((data & 0x08) << 1) +
				 ((data & 0x80) >> 7) + ((data & 0x40) >> 5) + ((data & 0x20) >> 3) + ((data & 0x10) >> 1);

		if (!sync_found)
      {
         /* Find sync pattern and adjust to an even byte boundary (so whole sync is in 16b word) */
         sync_data = (sync_data << 8) |  data;

         if (sync_data == sync_pattern)
         {
				/* check for even or odd byte */
            sync_found = 1;
				if	(byteIndex%2) byte_count = 1;
				else byte_count = 2;
				byteIndex = 0xFFFFFFFF; /* increments back to zero for file load */
				word_data = 0xFFFF;
				prog_count[0] = save_data;
				continue;
         }
      }
      else if (byte_count%2)
      {
         /* get low byte */
			word_data = data;
			if(swap_bytes) word_data <<= 8;
         ++byte_count;
      }
      else
      {
         /* get high byte */
			if(swap_bytes) word_data = word_data | data;
			else word_data = (data << 8) |  word_data;

			if(cmd_set == 2) result = Internal_write_flash_word_AMD_Std(devID, byte_count, word_data);
         if(cmd_set == 3) result = Internal_write_flash_word_Intel_Std(devID, byte_count, word_data);
         ++byte_count;
      }

      /* increment number of bytes written to device. */
		if(sync_found && byteIndex > 3) prog_count[0] = byteIndex;

      /* if program errors detected, force loop to exit */
      if (result != ADT_SUCCESS) break;
   }

	/* give thread time to update % automatically */
	ADT_L0_msSleep(1000);

	/* put flash back into read array mode */
   if(cmd_set == 2) cmd_set = 0x00F0;
   if(cmd_set == 3) cmd_set = 0x00FF;
   ADT_L0_WriteMem16(devID, 0x00000000, &cmd_set, 1);

   /* disable flash access in CPLD */
   temp32 = 0x00000000;
   ADT_L0_WriteMem32(devID, cpld_csr_offset, &temp32, 1);

   /* put LB back in 32-bit mode  */
   temp32 = 0x42030343;
   ADT_L0_WriteSetupMem32(devID, 0x18, &temp32, 1);

	/* Clear PLX usero pin. */
	temp32 = 0x100C767E | 0x00010000;
	ADT_L0_WriteSetupMem32(devID, 0x6C, &temp32, 1);

	/* Verify the firmware load. */
	prog_count[0] = save_data; /* restore first 4 bytes */
	if(result == ADT_SUCCESS)
		result = Internal_verify_flash(devID, numBytes, fpga_load_bytes);

	/* PCIE BOARDS MUST CYCLE POWER.
      Other (PCI) boards can reload FPGA and continue operation. */
   if (result == ADT_SUCCESS && (
			(boardType == ADT_DEVID_BOARDTYPE_PMC1553)
		|| (boardType == ADT_DEVID_BOARDTYPE_PC104P1553) 
		|| (boardType == ADT_DEVID_BOARDTYPE_PCI1553)
		|| (boardType == ADT_DEVID_BOARDTYPE_PMCA429)    
		|| (boardType == ADT_DEVID_BOARDTYPE_PMCA429HD)    
		|| (boardType == ADT_DEVID_BOARDTYPE_PC104PA429) 
		|| (boardType == ADT_DEVID_BOARDTYPE_PC104PA429LTV) 
		|| (boardType == ADT_DEVID_BOARDTYPE_PCIA429)    
		|| (boardType == ADT_DEVID_BOARDTYPE_PMCMA4)     
		|| (boardType == ADT_DEVID_BOARDTYPE_PC104PMA4) 
		|| (boardType == ADT_DEVID_BOARDTYPE_PMCWMUX)
      ))
   {
		/* Set PLX Usero pin low. This is the CPLD select signal */
		temp32 = 0x100C767E & ~0x00010000;
		ADT_L0_WriteSetupMem32(devID, 0x6C, &temp32, 1);

		/* Reload FPGA if no flash write failures */
		temp32 = 0x00000002;
		ADT_L0_WriteMem32(devID, cpld_csr_offset, &temp32, 1);

		/* wait for FPGA to reload before continuing */
      ADT_L0_msSleep(1);

		/* Clear PLX usero pin. */
		temp32 = 0x100C767E | 0x00010000;
		ADT_L0_WriteSetupMem32(devID, 0x6C, &temp32, 1);
      ADT_L0_msSleep(1000);
   }

   return(result);
}


/******************************************************************************
  FUNCTION:    Internal_ProgramEnetFlash
 *****************************************************************************/
/*! \brief Programs the FLASH memory for the ENET board.
 *
 * This function programs the FLASH memory for the ENET BOARD.
 * This function will attempt to program the ENET board 3 times
 * if an ethernet communication error occurs.
 * WARNING - This affects ALL channels on the board.  Do not use this function
 * if other applications are using other channels on the board.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param numBytes is the number of bytes to load.
 * @param fpga_load_bytes is a pointer to the bytes to load.
 * @return
   - \ref ADT_SUCCESS - Completed without error
   - \ref ADT_ERR_BAD_DEV_TYPE - Unsupported board type
   - \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV Internal_ProgramEnetFlash(ADT_L0_UINT32 devID, ADT_L0_UINT32 numBytes, ADT_L0_UINT8 * fpga_load_bytes)
{
   ADT_L0_UINT8  data;
   ADT_L0_UINT32 result = ADT_SUCCESS;
   ADT_L0_UINT32 i, j, sync_found;
   ADT_L0_UINT32 sync_data, byte_count, temp32, byteIndex, save_data;
   ADT_L0_UINT16 word_data;
   ADT_L0_UINT32 cpld_csr_offset;
   ADT_L0_UINT32 flash_erase_offset;
   ADT_L0_UINT32 sync_pattern;
   ADT_L0_UINT32 prog_errors;
   ADT_L0_UINT32 flash_prog_cnt;
   ADT_L0_UINT16 cmd_set;
   ADT_L0_UINT32 geometry[8];
   ADT_L0_UINT32 *prog_count;
	ADT_L0_UINT32 PortNum, TransactionCnt, RetryCnt, FailureCnt;

	/* Flash sector size may be different between flash devices.
      Erase the size of the file (header and data) + 4 bytes. */
   flash_erase_offset = numBytes + 4;

   sync_pattern = 0xFFFFBDCD;
   cpld_csr_offset = 0x00370000;
	sync_found = 0;

   flash_prog_cnt = 0;

	PortNum = TransactionCnt = RetryCnt = FailureCnt = 0;

   word_data = 0;
   sync_data = 0;
   byte_count = 0;
   sync_found = 0;

   /* zero the first 4 bytes of fpga_load_bytes array. These locations are used to keep track of the
      number of program bytes written to the device. */
   prog_count = (ADT_L0_UINT32*)fpga_load_bytes;
	save_data = prog_count[0];
   prog_count[0] = 0;

   prog_errors = 0;
   flash_prog_cnt++;

   /* Set the CPLD select signal */
   temp32 = 0;
   prog_errors += ADT_L0_WriteSetupMem32(devID, 0x6C, &temp32, 1);

   /* enable flash access in CPLD */
   temp32 = 0x00000004;
   prog_errors += ADT_L0_WriteMem32(devID, cpld_csr_offset, &temp32, 1);

   /* get flash erase sector sizes (geometry) and command set for CFI flash devices */
   result = Internal_read_flash_CFI(devID, &cmd_set, geometry);

   if(result == ADT_FAILURE) {
      /* put flash back into read array mode */
      cmd_set = 0x00FF; /* AMD Reset */
      ADT_L0_WriteMem16(devID, 0x00000000, &cmd_set, 1);
      cmd_set = 0x00F0; /* Intel Read Array */
      ADT_L0_WriteMem16(devID, 0x00000000, &cmd_set, 1);

      /* disable flash access in CPLD */
      temp32 = 0x00000000;
      ADT_L0_WriteMem32(devID, cpld_csr_offset, &temp32, 1);

      /* Clear the CPLD select signal */
      temp32 = 0x00010000;
      ADT_L0_WriteSetupMem32(devID, 0x6C, &temp32, 1);

      return (result);
   }

   /* erase flash blocks */
   for(i=0, j=0; j<4; j++) {
      for(;i<flash_erase_offset && geometry[j*2]>0;i+=geometry[j*2+1],geometry[j*2]--) {
         if(cmd_set == 2) prog_errors += Internal_erase_flash_block_AMD_Std(devID, i);
         if(cmd_set == 3) prog_errors += Internal_erase_flash_block_Intel_Std(devID, i);
         prog_count[0] = i;

			/* check for loss of device connection */
			ADT_L1_ENET_ADCP_GetStatistics(devID, &PortNum, &TransactionCnt, &RetryCnt, &FailureCnt);
			if(FailureCnt) {
				prog_errors += ADT_FAILURE;
				i = flash_erase_offset; /* break out of both for loops */
			}
      }
   }
	if(i > flash_erase_offset) prog_count[0] = numBytes;

	/* give thread time to update % automatically */
	ADT_L0_msSleep(1000);

   /* load fpga data into flash */
	for (byteIndex=0; byteIndex<numBytes+1 && result == ADT_SUCCESS; byteIndex++)
	{
		data = (byteIndex<numBytes) ? fpga_load_bytes[byteIndex] : 0xFF;

		/* swap bits - data byte to fpga is swapped */
		data = ((data & 0x01) << 7) + ((data & 0x02) << 5) + ((data & 0x04) << 3) + ((data & 0x08) << 1) +
				 ((data & 0x80) >> 7) + ((data & 0x40) >> 5) + ((data & 0x20) >> 3) + ((data & 0x10) >> 1);

      if (!sync_found)
      {
			sync_data = (sync_data << 8) |  data;

			if (sync_data == sync_pattern)
			{
				/* check for even or odd byte */
				sync_found = 1;
				if	(byteIndex%2) byte_count = 1;
				else byte_count = 2;
				byteIndex = 0xFFFFFFFF; /* increments back to zero for file load */
				word_data = 0xFFFF;
				prog_count[0] = save_data;
				continue;
			}
      }
      else if (byte_count%2)
      {
         /* get low byte */
         word_data = data;
         ++byte_count;
      }
      else
      {
         /* get high byte */
         word_data = (data << 8) |  word_data;
         if(cmd_set == 2) prog_errors += Internal_write_flash_word_AMD_Std(devID, byte_count, word_data);
         if(cmd_set == 3) prog_errors += Internal_write_flash_word_Intel_Std(devID, byte_count, word_data);
         ++byte_count;
      }

		/* check for loss of device connection */
		ADT_L1_ENET_ADCP_GetStatistics(devID, &PortNum, &TransactionCnt, &RetryCnt, &FailureCnt);
		if(FailureCnt) {
			prog_errors += ADT_FAILURE;
			break; /* break out of for loop */
		}

      /* increment number of bytes written to device. */
      if(sync_found && byteIndex > 3) prog_count[0] = byteIndex;

      /* if program errors detected, force loop to exit */
      if (prog_errors)
         byteIndex = numBytes;
   }

	/* give thread time to update % automatically */
	ADT_L0_msSleep(1000);

   /* put flash back into read array mode */
   if(cmd_set == 2) temp32 = 0x00F0;
   if(cmd_set == 3) temp32 = 0x00FF;
   prog_errors += ADT_L0_WriteMem32(devID, 0x00000000, &temp32, 1);

   /* disable flash access in CPLD */
   temp32 = 0x00000000;
   prog_errors += ADT_L0_WriteMem32(devID, cpld_csr_offset, &temp32, 1);

   /* Clear the CPLD select signal */
   temp32 = 0x00010000;
   prog_errors += ADT_L0_WriteSetupMem32(devID, 0x6C, &temp32, 1);

	prog_count[0] = save_data; /* restore first 4 bytes */
	prog_errors += Internal_verify_flash(devID, numBytes, fpga_load_bytes);

   if (prog_errors)
      result = ADT_FAILURE;

   return(result);
}

/******************************************************************************
  FUNCTION:    ADT_L1_msSleep
 *****************************************************************************/
/*! \brief This function calls the ADT_L0 sleep for program pauses
 *
 * @param milliseconds is the number for delay.
 * @return ADT_SUCCESS
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L0_CALL_CONV ADT_L1_msSleep(ADT_L0_UINT32 mSec)
{
   ADT_L0_UINT32 status = ADT_SUCCESS;
   ADT_L0_msSleep(mSec);
   return(status);
}

/******************************************************************************
  INTERNAL FUNCTION:    Internal_ProgramEcdFlash
 *****************************************************************************/
ADT_L0_UINT32 ADT_L0_CALL_CONV Internal_ProgramEcdFlash(ADT_L0_UINT32 devID, ADT_L0_UINT32 numBytes, ADT_L0_UINT8 * fpga_load_bytes)
{
   ADT_L0_UINT32 result = ADT_SUCCESS;
   ADT_L0_UINT32 i, data, sync_offset,boardType;

    /* enable writes to flash */
    data = 0x06000000;
    ADT_L0_WriteMem32(devID, 0xD0, &data, 1);
    Internal_ecd_flash_check_writen(devID);

   boardType = devID & 0x0FF00000;

   /* zero the first 4 bytes of fpga_load_bytes array. These locations are used to keep track of the
   number of ptrogram bytes written to the device. */
   fpga_load_bytes[0] = 0;
   fpga_load_bytes[1] = 0;
   fpga_load_bytes[2] = 0;
   fpga_load_bytes[3] = 0;

   /* first find sync offset in fpga data */
   if (boardType == ADT_DEVID_BOARDTYPE_ENET1A1553)
   {
      sync_offset = 0;
      for (i=0;i<numBytes;++i)
      {
         if ((fpga_load_bytes[i] == 0xAA) && (fpga_load_bytes[i+1] == 0x99))
         {
            sync_offset = i-16;
            i = numBytes;
         }
      }
   }
   else
   {
      sync_offset = 0;
      for (i=0;i<numBytes;++i)
      {
         if ((fpga_load_bytes[i] == 0xBD) && (fpga_load_bytes[i+1] == 0xB3))
         {
            sync_offset = i-16;
             i = numBytes;
         }
      }
   }

   if (sync_offset == 0)
      return (ADT_ERR_BAD_INPUT);

   /* clear entire contents of flash memory. This will take ~13 seconds */
   data = 0xC7000000;
   ADT_L0_WriteMem32(devID, 0xD0, &data, 1);
    Internal_ecd_flash_wait_complete(devID);

   /* read flash data from memory and write to the board */
   for (i=sync_offset;i<numBytes-3;i=i+4)
   {
      /* enable writes to flash */
      data = 0x06000000;
       ADT_L0_WriteMem32(devID, 0xD0, &data, 1);
      Internal_ecd_flash_check_writen(devID);

      /* assemble data word and write to memory */
      data = fpga_load_bytes[i] << 24;
      data = data + (fpga_load_bytes[i+1] << 16);
      data = data + (fpga_load_bytes[i+2] << 8);
      data = data + fpga_load_bytes[i+3];

      ADT_L0_WriteMem32(devID, 0xD4, &data, 1);
      Internal_ecd_flash_wait_complete(devID);

      /* issue program command to flash */
      data = 0x02000000 + (i-sync_offset);
      ADT_L0_WriteMem32(devID, 0xD0, &data, 1);
      Internal_ecd_flash_wait_complete(devID);

      /* increment number of bytes written to device. */
      fpga_load_bytes[0] = (ADT_L0_UINT8) (i & 0x000000FF);
      fpga_load_bytes[1] = (ADT_L0_UINT8) ((i & 0x0000FF00) >> 8);
      fpga_load_bytes[2] = (ADT_L0_UINT8) ((i & 0x00FF0000) >> 16);
      fpga_load_bytes[3] = (ADT_L0_UINT8) ((i & 0xFF000000) >> 24);

   }

   /* disable writes to flash */
   data = 0x04000000;
    ADT_L0_WriteMem32(devID, 0xD0, &data, 1);

  return (result);
}

/******************************************************************************
  INTERNAL FUNCTION:    Internal_ecd_flash_wait_complete
 *****************************************************************************/
ADT_L0_UINT32 ADT_L0_CALL_CONV Internal_ecd_flash_wait_complete(ADT_L0_UINT32 devID)
{
   ADT_L0_UINT32 result = ADT_SUCCESS;
   ADT_L0_UINT32 counter1, counter2, data, timeout_occured;

   /* dummy read of memory. This read forces ALL other write over PCIe
    * bus to complete before writing command to read flash status register
    */
   ADT_L0_ReadMem32(devID, 0xD4, &data, 1);

    /* wait for flash access to complete */
   timeout_occured = counter2 = 0;
   do
   {
      data = 0x05000000;
      ADT_L0_WriteMem32(devID, 0xD0, &data, 1); /* read status register */
        counter1 = 0;
      do {
            ADT_L0_ReadMem32(devID, 0xD4, &data, 1);
            counter1++;

            if (counter1 == 100)
               timeout_occured = 1;

      }
      while (((data & 0x00010000) != 0) && (counter1 < 100)); /* wait for status register read data to be valid */

      counter2++;
   }
   while((data & 0x00000001) && (counter2 < 0x1000000)  && !timeout_occured); /* now check that we do not have a transaction in-progress */

   return (result);

}

/******************************************************************************
  INTERNAL FUNCTION:    Internal_ecd_flash_check_writen
 *****************************************************************************/
ADT_L0_UINT32 ADT_L0_CALL_CONV Internal_ecd_flash_check_writen(ADT_L0_UINT32 devID)
{
   ADT_L0_UINT32 result = ADT_SUCCESS;
   ADT_L0_UINT32 counter1, counter2, data, timeout_occured;

   /* dummy read of memory. This read forces ALL other write over PCIe
    * bus to complete before writing command to read flash status register
    */
   ADT_L0_ReadMem32(devID, 0xD4, &data, 1);

    /* wait for flash access to complete */
   timeout_occured = counter2 = 0;
   do
   {
      data = 0x05000000;
      ADT_L0_WriteMem32(devID, 0xD0, &data, 1); /* read status register */
        counter1 = 0;
      do {
            ADT_L0_ReadMem32(devID, 0xD4, &data, 1);
            counter1++;

            if (counter1 == 100)
               timeout_occured = 1;
      }
      while (((data & 0x00010000) != 0) && (counter1 < 100)); /* wait for status register read data to be valid */
      counter2++;
   }
   while((!(data & 0x00000002)) && (counter2 < 0x1000000) && !timeout_occured); /* now check the we do not have a transaction in-progress */


   return (result);

}


/******************************************************************************
  INTERNAL FUNCTION:    Internal_verify_flash
 *****************************************************************************/
ADT_L0_UINT32 ADT_L0_CALL_CONV Internal_verify_flash(ADT_L0_UINT32 devID, ADT_L0_UINT32 numBytes, ADT_L0_UINT8 * fpga_load_bytes)
{
   ADT_L0_UINT8  data;
   ADT_L0_UINT32 result = ADT_SUCCESS;
   ADT_L0_UINT32 sync_found, cpld_csr_offset, sync_pattern;
   ADT_L0_UINT32 sync_data, byte_count, temp, byteIndex, boardType;
   ADT_L0_UINT16 read_data, word_data;
   ADT_L0_UINT32 *prog_count, save_data;
	ADT_L0_UINT32 swap_bytes;

   prog_count = (ADT_L0_UINT32*)fpga_load_bytes;
	save_data = prog_count[0];
   prog_count[0] = 0;

	swap_bytes = 0;

   boardType = devID & 0x0FF00000;
	if (		(boardType == ADT_DEVID_BOARDTYPE_ECD54_1553)
			|| (boardType == ADT_DEVID_BOARDTYPE_ECD54_A429)
			|| (boardType == ADT_DEVID_BOARDTYPE_MPCIE1553)
			|| (boardType == ADT_DEVID_BOARDTYPE_MPCIE21553)
			|| (boardType == ADT_DEVID_BOARDTYPE_ENET1A1553)
			|| (boardType == ADT_DEVID_BOARDTYPE_MPCIEA429)
			|| (boardType == ADT_DEVID_BOARDTYPE_TBOLTMA4)
		) {
      return (result);
   }
   else if ((boardType == ADT_DEVID_BOARDTYPE_ENET1553)
			|| (boardType == ADT_DEVID_BOARDTYPE_PMCE1553)
			|| (boardType == ADT_DEVID_BOARDTYPE_ENETA429)
			|| (boardType == ADT_DEVID_BOARDTYPE_ENET485)
			|| (boardType == ADT_DEVID_BOARDTYPE_ENET2_1553)
			|| (boardType == ADT_DEVID_BOARDTYPE_ENET_MA4)
			|| (boardType == ADT_DEVID_BOARDTYPE_ENETX_MA4)
			|| (boardType == ADT_DEVID_BOARDTYPE_ENETA429P)
		) {
      /* the size of the FPGA bit stream file for ECP3-70 is ~2.4Mbyte. */
      sync_pattern = 0xFFFFBDCD;
      cpld_csr_offset = 0x00370000;
   }
   else if ((boardType == ADT_DEVID_BOARDTYPE_PC104PMA4)
			|| (boardType == ADT_DEVID_BOARDTYPE_PC104PA429LTV)
		) {
      /* the size of the FPGA bit stream file for 6SLX75 is ~2.4Mbyte. */
      sync_pattern = 0x5599AA66;
      cpld_csr_offset = 0x00370000;
   }
   else if ((boardType == ADT_DEVID_BOARDTYPE_PCIE4L1553)
			|| (boardType == ADT_DEVID_BOARDTYPE_PCIE1L1553)
			|| (boardType == ADT_DEVID_BOARDTYPE_XMCMA4)
			|| (boardType == ADT_DEVID_BOARDTYPE_XMCA429)
			|| (boardType == ADT_DEVID_BOARDTYPE_PMCA429HD)
			|| (boardType == ADT_DEVID_BOARDTYPE_PCIE4LA429)
			|| (boardType == ADT_DEVID_BOARDTYPE_PCIE1LA429)
			|| (boardType == ADT_DEVID_BOARDTYPE_PC104EMA4)
      ) {
      /* the size of the FPGA bit stream file for ECP3-70 is ~2.4Mbyte. */
      sync_pattern = 0xFFFFBDCD;
      cpld_csr_offset = 0x00370000;
   }
   else if ((boardType == ADT_DEVID_BOARDTYPE_XMCMW) || (boardType == ADT_DEVID_BOARDTYPE_XMC1553)
		) { 
      /* the size of the FPGA bit stream file for Artix7 is ~9.5Mbyte. */
		swap_bytes = 1;
      sync_pattern = 0x5599AA66;
      cpld_csr_offset = 0x00FF0000;
   }
   else if ((boardType == ADT_DEVID_BOARDTYPE_PMCMA4)
		) {
      /* the size of the FPGA bit stream file for EPC2-70 is ~1.8Mbyte. */
      sync_pattern  = 0xFFFFBDCD;
      cpld_csr_offset = 0x00200000;
   }
   else if ((boardType == ADT_DEVID_BOARDTYPE_PMC1553)
			|| (boardType == ADT_DEVID_BOARDTYPE_PC104P1553)
			|| (boardType == ADT_DEVID_BOARDTYPE_PCI1553)
			|| (boardType == ADT_DEVID_BOARDTYPE_PMCA429)
			|| (boardType == ADT_DEVID_BOARDTYPE_PC104PA429)
			|| (boardType == ADT_DEVID_BOARDTYPE_PCIA429)
			|| (boardType == ADT_DEVID_BOARDTYPE_PMCWMUX)
      ) {
      /* the size of the FPGA bit stream file for spartan3-2000 is ~1Mbyte. */
      sync_pattern = 0x5599AA66;
      cpld_csr_offset = 0x00200000;
   }
   else return(result);

   /* Set PLX Usero pin low. This is the CPLD select signal */
   temp = 0x100C767E & ~0x00010000;
   ADT_L0_WriteSetupMem32(devID, 0x6C, &temp, 1);

   /* enable flash access in CPLD */
   temp = 0x00000004;
   ADT_L0_WriteMem32(devID, cpld_csr_offset, &temp, 1);

   /* put LB in 16-bit mode  */
   temp = 0x42030341;
   ADT_L0_WriteSetupMem32(devID, 0x18, &temp, 1);

	sync_found = 0;

	/* verify flash data */
   for (byteIndex=0; byteIndex<numBytes+1 && result!=ADT_FAILURE; byteIndex++)
   {
		data = (byteIndex<numBytes) ? fpga_load_bytes[byteIndex] : 0xFF;

		/* swap bits - data byte to fpga is swapped */
		data = ((data & 0x01) << 7) + ((data & 0x02) << 5) + ((data & 0x04) << 3) + ((data & 0x08) << 1) +
				 ((data & 0x80) >> 7) + ((data & 0x40) >> 5) + ((data & 0x20) >> 3) + ((data & 0x10) >> 1);

      if (!sync_found)
      {
         /* Find sync pattern and adjust to an even byte boundary (so whole sync is in 16b word) */
         sync_data = (sync_data << 8) |  data;

         if (sync_data == sync_pattern)
         {
				/* check for even or odd byte */
            sync_found = 1;
				//byteIndex -= 32; // back up 32 bytes
				if	(byteIndex%2) byte_count = 1;
				else byte_count = 2;
				byteIndex = 0xFFFFFFFF; /* increments back to zero for file load */
				word_data = 0xFFFF;
				prog_count[0] = save_data;
				continue;
         }
      }
      else if (byte_count%2)
      {
         word_data = data;
			if(swap_bytes) word_data <<= 8;
         ++byte_count;
      }
      else
      {
			if(swap_bytes) word_data = word_data | data;
			else word_data = (data << 8) |  word_data;

         ADT_L0_ReadMem16(devID, byte_count, &read_data, 1);
         if(read_data != word_data)
            result = ADT_FAILURE;

         ++byte_count;
      }
      if (sync_found && byteIndex > 3) prog_count[0] = byteIndex;
   }

   /* disable flash access in CPLD */
   temp = 0x00000000;
   ADT_L0_WriteMem32(devID, cpld_csr_offset, &temp, 1);

   temp = 0x42030343;
   ADT_L0_WriteSetupMem32(devID, 0x18, &temp, 1);     /* put LB back in 32-bit mode */

   /* Clear PLX usero pin. */
   temp = 0x100C767E | 0x00010000;
   ADT_L0_WriteSetupMem32(devID, 0x6C, &temp, 1);

   return(result);
}


/******************************************************************************
  INTERNAL FUNCTION:    Internal_read_flash_CFI
 *****************************************************************************/
ADT_L0_UINT32 ADT_L0_CALL_CONV Internal_read_flash_CFI(ADT_L0_UINT32 devID, ADT_L0_UINT16 *cmd_set, ADT_L0_UINT32 *geometry) {
   ADT_L0_UINT16 temp[17];
   ADT_L0_UINT32 error_count, i;

   error_count = 0;

   temp[0] = 0x0098;
   error_count += ADT_L0_WriteMem16(devID, 2*0x00000055, temp, 1);    /* write CFI Query command  */
   /* Note:  ADT_L0_ReadMem16 is called in a for loop for two reasons.  One is the L0 code was incrementing address incorrectly.
             Two is that older eNet firmware (pre PE v010F) was incrementing address incorrectly.  This loop will work with older code. */
   for(i=0;i<5;i++) error_count += ADT_L0_ReadMem16(devID, 0x10*2+2*i, &temp[i], 1);    /* read  QRY string and Primary Algorithm Command Set */
   if(temp[0] == 'Q' && temp[1] == 'R' && temp[2] == 'Y') {
      cmd_set[0] = (temp[3] & 0xFF) | ((temp[4] & 0xFF) << 8);
      for(i=0;i<17;i++) error_count += ADT_L0_ReadMem16(devID, 0x2C*2+2*i, &temp[i], 1);    /* read geometry info (sector numbers and sizes) */
      for (i=0;i<(ADT_L0_UINT32)(temp[0]&0xFF);i++) {
         geometry[i*2+0] = (temp[(i*4)+1] & 0xFF) | ((temp[(i*4)+2] & 0xFF) << 8);
         geometry[i*2+0] += 1;
         geometry[i*2+1] = (temp[(i*4)+3] & 0xFF) | ((temp[(i*4)+4] & 0xFF) << 8);
         geometry[i*2+1] *= 256;
      }
      for (;i<4;i++) { /* zero remaining */
         geometry[i*2+0] = 0;
         geometry[i*2+1] = 0;
      }
      if (cmd_set[0] == 2) temp[0] = 0xF0; /* Reset command */
      else temp[0] = 0xFF; /* read memory array command */
      ADT_L0_WriteMem16(devID, 0x0000, temp, 1);
      ADT_L0_msSleep(1);
   }
   else {
      cmd_set[0] = 0;
      for (i=0;i<8;i++) geometry[i] = 0;
   }

   return ((!cmd_set[0] || error_count) ? ADT_FAILURE : ADT_SUCCESS);
}


/******************************************************************************
  INTERNAL FUNCTION:    Internal_write_flash_word_AMD_Std
 *****************************************************************************/
ADT_L0_UINT32 ADT_L0_CALL_CONV Internal_write_flash_word_AMD_Std(ADT_L0_UINT32 devID, ADT_L0_UINT32 offset, ADT_L0_UINT16 value) {
   ADT_L0_UINT32 result = ADT_SUCCESS;
   ADT_L0_UINT32 counter;
   ADT_L0_UINT16 read_data, temp;
   ADT_L0_UINT32 error_count;

   error_count = 0;

   temp = 0x00AA;
   error_count += ADT_L0_WriteMem16(devID, 2*0x0555, &temp, 1);
   temp = 0x0055;
   error_count += ADT_L0_WriteMem16(devID, 2*0x02AA, &temp, 1);
   temp = 0x00A0;
   error_count += ADT_L0_WriteMem16(devID, 2*0x0555, &temp, 1);

   error_count += ADT_L0_WriteMem16(devID, offset, &value, 1);    /* write value to offset */

   /* wait for operation to complete */
   counter = 0;
   do
   {
      error_count += ADT_L0_ReadMem16(devID, offset, &read_data, 1);   /* read status register */
      counter++;
   }
   while(((read_data != value)) && (counter < 1000000) && !error_count);

   if ( (read_data != value) || error_count)
      result = ADT_FAILURE;

   return(result);
}

/******************************************************************************
  INTERNAL FUNCTION:    Internal_erase_flash_block_AMD_Std
 *****************************************************************************/
ADT_L0_UINT32 ADT_L0_CALL_CONV Internal_erase_flash_block_AMD_Std(ADT_L0_UINT32 devID, ADT_L0_UINT32 block_addr) {
   ADT_L0_UINT32 result = ADT_SUCCESS;
   ADT_L0_UINT32 counter;
   ADT_L0_UINT16 read_data, temp;
   ADT_L0_UINT32 error_count;

   error_count = 0;

   temp = 0x00AA;
   error_count += ADT_L0_WriteMem16(devID, 2*0x0555, &temp, 1);
   temp = 0x0055;
   error_count += ADT_L0_WriteMem16(devID, 2*0x02AA, &temp, 1);
   temp = 0x0080;
   error_count += ADT_L0_WriteMem16(devID, 2*0x0555, &temp, 1);
   temp = 0x00AA;
   error_count += ADT_L0_WriteMem16(devID, 2*0x0555, &temp, 1);
   temp = 0x0055;
   error_count += ADT_L0_WriteMem16(devID, 2*0x02AA, &temp, 1);

   temp = 0x0030;
   error_count += ADT_L0_WriteMem16(devID, block_addr, &temp, 1);    /* write block address to erase */

   ADT_L0_msSleep(1);

   /* wait for operation to complete */
   counter = 0;
   do
   {
      error_count += ADT_L0_ReadMem16(devID, block_addr, &read_data, 1);   /* read status register */
      counter++;
   }
   while((((read_data & 0xFF) != 0xFF)) && (counter < 1000000) && !error_count);

   if ( ((read_data & 0xFF) != 0x00FF) || error_count)
      result = ADT_FAILURE;

   return(result);
}


/******************************************************************************
  INTERNAL FUNCTION:    Internal_write_flash_word_Intel_Std
 *****************************************************************************/
ADT_L0_UINT32 ADT_L0_CALL_CONV Internal_write_flash_word_Intel_Std(ADT_L0_UINT32 devID, ADT_L0_UINT32 offset, ADT_L0_UINT16 value)
{
   ADT_L0_UINT32 result = ADT_SUCCESS;
   ADT_L0_UINT32 counter;
   ADT_L0_UINT16 read_data, temp;
   ADT_L0_UINT32 error_count;

   error_count = 0;

   temp = 0x0040;
   error_count += ADT_L0_WriteMem16(devID, 0x00000000, &temp, 1);    /* write program command */

   error_count += ADT_L0_WriteMem16(devID, offset, &value, 1);    /* write program data */

   temp = 0x0070;
   error_count += ADT_L0_WriteMem16(devID, 0x00000000, &temp, 1);    /* write status register command */

   /* wait for flash busy bit to clear */
   counter = 0;
   do
   {
      error_count += ADT_L0_ReadMem16(devID, 0x00000000, &read_data, 1); /* read status register */
      counter++;
   }
   while((!(read_data & 0x0080)) && (counter < 1000000) && !error_count);

   if ( (read_data != 0x0080) || error_count)
      result = ADT_FAILURE;

   return(result);
}

/******************************************************************************
  INTERNAL FUNCTION:    Internal_unlock_flash_block_Intel_Std
 *****************************************************************************/
ADT_L0_UINT32 ADT_L0_CALL_CONV Internal_unlock_flash_block_Intel_Std(ADT_L0_UINT32 devID, ADT_L0_UINT32 block_addr)
{
   ADT_L0_UINT32 result = ADT_SUCCESS;
   ADT_L0_UINT16 read_data, temp;
   ADT_L0_UINT32 error_count;

   error_count = 0;

   temp = 0x0060;
   error_count += ADT_L0_WriteMem16(devID, 0x00000000, &temp, 1);       /* write block unlock command */
   temp = 0x00D0;
   error_count += ADT_L0_WriteMem16(devID, block_addr, &temp, 1);       /* write block address to unlock */

   temp = 0x0090;
   error_count += ADT_L0_WriteMem16(devID, 0x00000000, &temp, 1);       /* write read identifier command */
   error_count += ADT_L0_ReadMem16(devID, block_addr+4, &read_data, 1); /* read status register */

   if ( ((read_data & 0x0003) != 0x0000) || error_count)
      result = ADT_FAILURE;

   return(result);
}

/******************************************************************************
  INTERNAL FUNCTION:    Internal_erase_flash_block_Intel_Std
 *****************************************************************************/
ADT_L0_UINT32 ADT_L0_CALL_CONV Internal_erase_flash_block_Intel_Std(ADT_L0_UINT32 devID, ADT_L0_UINT32 block_addr)
{
   ADT_L0_UINT32 result = ADT_SUCCESS;
   ADT_L0_UINT32 counter;
   ADT_L0_UINT16 read_data, temp;
   ADT_L0_UINT32 error_count;

   error_count = 0;

   error_count += Internal_unlock_flash_block_Intel_Std(devID, block_addr);      /* unlock block before erasing */

   temp = 0x0050;
   error_count += ADT_L0_WriteMem16(devID, 0x00000000, &temp, 1);    /* clear status register command */

   temp = 0x0020;
   error_count += ADT_L0_WriteMem16(devID, 0x00000000, &temp, 1);    /* write block erase command  */
   temp = 0x00D0;
   error_count += ADT_L0_WriteMem16(devID, block_addr, &temp, 1);    /* write block address to erase */

   ADT_L0_msSleep(1);

   temp = 0x0070;
   error_count +=  ADT_L0_WriteMem16(devID, 0x00000000, &temp, 1);      /* write status register command */

   /* wait for flash busy bit to clear */
   counter = 0;
   do
   {
      error_count += ADT_L0_ReadMem16(devID, 0x00000000, &read_data, 1);   /* read status register */
      counter++;
   }
   while((!(read_data & 0x0080)) && (counter < 1000000) && !error_count);

   if ( (read_data != 0x0080) || error_count)
      result = ADT_FAILURE;

   return(result);
}

/******************************************************************************
  INTERNAL FUNCTION:    Internal_lock_flash_block_Intel_Std
 *****************************************************************************/
ADT_L0_UINT32 ADT_L0_CALL_CONV Internal_lock_flash_block_Intel_Std(ADT_L0_UINT32 devID, ADT_L0_UINT32 block_addr)
{
   ADT_L0_UINT16 temp;
   ADT_L0_UINT32 error_count;

   error_count = 0;

   temp = 0x0060;
   error_count += ADT_L0_WriteMem16(devID, 0x00000000, &temp, 1);    /* write block lock command  */
   temp = 0x0001;
   error_count += ADT_L0_WriteMem16(devID, block_addr, &temp, 1);    /* write block address to lock */

   return (error_count);
}


/******************************************************************************
  INTERNAL FUNCTION:    Internal_GetChannelRegOffset
 *****************************************************************************/
ADT_L0_UINT32 ADT_L0_CALL_CONV Internal_GetChannelRegOffset(ADT_L0_UINT32 devID, ADT_L0_UINT32 *pChannel, ADT_L0_UINT32 *pChannelRegOffset) {
   ADT_L0_UINT32 result = ADT_SUCCESS;

   /* Determine offset to the PE registers for the channel based on board type and channel type */
   *pChannel = devID & 0x000000FF;

   /* ADT_L1_MemMgmt.c - constants set size of dmmArray, we use a size of 6 for channel number */
   if (*pChannel > (ADT_L1_MEMMGT_NUM_CHANNUM - 1))
   {
      return(ADT_ERR_BAD_CHAN_NUM);
   }

   switch (devID & 0x0FF0FF00) {
      /* GLOBALS Channel Type */
      case (ADT_DEVID_BOARDTYPE_SIM1553 | ADT_DEVID_CHANNELTYPE_GLOBALS):
      case (ADT_DEVID_BOARDTYPE_PMC1553 | ADT_DEVID_CHANNELTYPE_GLOBALS):
      case (ADT_DEVID_BOARDTYPE_PCI1553 | ADT_DEVID_CHANNELTYPE_GLOBALS):
      case (ADT_DEVID_BOARDTYPE_PC104P1553 | ADT_DEVID_CHANNELTYPE_GLOBALS):
      case (ADT_DEVID_BOARDTYPE_PCCD1553 | ADT_DEVID_CHANNELTYPE_GLOBALS):
      case (ADT_DEVID_BOARDTYPE_PCI104E1553 | ADT_DEVID_CHANNELTYPE_GLOBALS):
      case (ADT_DEVID_BOARDTYPE_XMC1553 | ADT_DEVID_CHANNELTYPE_GLOBALS):
      case (ADT_DEVID_BOARDTYPE_XMCMW | ADT_DEVID_CHANNELTYPE_GLOBALS):
      case (ADT_DEVID_BOARDTYPE_ECD54_1553 | ADT_DEVID_CHANNELTYPE_GLOBALS):
      case (ADT_DEVID_BOARDTYPE_PCIE4L1553 | ADT_DEVID_CHANNELTYPE_GLOBALS):
      case (ADT_DEVID_BOARDTYPE_PCIE1L1553 | ADT_DEVID_CHANNELTYPE_GLOBALS):
      case (ADT_DEVID_BOARDTYPE_MPCIE1553 | ADT_DEVID_CHANNELTYPE_GLOBALS):
      case (ADT_DEVID_BOARDTYPE_MPCIE21553 | ADT_DEVID_CHANNELTYPE_GLOBALS):
      case (ADT_DEVID_BOARDTYPE_SIMA429 | ADT_DEVID_CHANNELTYPE_GLOBALS):
      case (ADT_DEVID_BOARDTYPE_TESTA429 | ADT_DEVID_CHANNELTYPE_GLOBALS):
      case (ADT_DEVID_BOARDTYPE_PMCA429 | ADT_DEVID_CHANNELTYPE_GLOBALS):
      case (ADT_DEVID_BOARDTYPE_PMCA429HD | ADT_DEVID_CHANNELTYPE_GLOBALS):
      case (ADT_DEVID_BOARDTYPE_PCIA429 | ADT_DEVID_CHANNELTYPE_GLOBALS):
      case (ADT_DEVID_BOARDTYPE_PC104PA429 | ADT_DEVID_CHANNELTYPE_GLOBALS):
      case (ADT_DEVID_BOARDTYPE_PC104PA429LTV | ADT_DEVID_CHANNELTYPE_GLOBALS):
      case (ADT_DEVID_BOARDTYPE_PCCDA429 | ADT_DEVID_CHANNELTYPE_GLOBALS):
      case (ADT_DEVID_BOARDTYPE_PCI104EA429 | ADT_DEVID_CHANNELTYPE_GLOBALS):
      case (ADT_DEVID_BOARDTYPE_XMCA429 | ADT_DEVID_CHANNELTYPE_GLOBALS):
      case (ADT_DEVID_BOARDTYPE_ECD54_A429 | ADT_DEVID_CHANNELTYPE_GLOBALS):
      case (ADT_DEVID_BOARDTYPE_PCIE4LA429 | ADT_DEVID_CHANNELTYPE_GLOBALS):
      case (ADT_DEVID_BOARDTYPE_PCIE1LA429 | ADT_DEVID_CHANNELTYPE_GLOBALS):
      case (ADT_DEVID_BOARDTYPE_MPCIEA429 | ADT_DEVID_CHANNELTYPE_GLOBALS):
      case (ADT_DEVID_BOARDTYPE_PMCMA4 | ADT_DEVID_CHANNELTYPE_GLOBALS):
      case (ADT_DEVID_BOARDTYPE_PC104PMA4 | ADT_DEVID_CHANNELTYPE_GLOBALS):
      case (ADT_DEVID_BOARDTYPE_PC104EMA4 | ADT_DEVID_CHANNELTYPE_GLOBALS):
      case (ADT_DEVID_BOARDTYPE_TBOLTMA4 | ADT_DEVID_CHANNELTYPE_GLOBALS):
      case (ADT_DEVID_BOARDTYPE_XMCMA4 | ADT_DEVID_CHANNELTYPE_GLOBALS):
      case (ADT_DEVID_BOARDTYPE_ENET1553 | ADT_DEVID_CHANNELTYPE_GLOBALS):
      case (ADT_DEVID_BOARDTYPE_PMCE1553 | ADT_DEVID_CHANNELTYPE_GLOBALS):
      case (ADT_DEVID_BOARDTYPE_ENETA429 | ADT_DEVID_CHANNELTYPE_GLOBALS):
      case (ADT_DEVID_BOARDTYPE_ENETA429P | ADT_DEVID_CHANNELTYPE_GLOBALS):
      case (ADT_DEVID_BOARDTYPE_ENET1A1553 | ADT_DEVID_CHANNELTYPE_GLOBALS):
	  case (ADT_DEVID_BOARDTYPE_PMCWMUX | ADT_DEVID_CHANNELTYPE_GLOBALS):
      case (ADT_DEVID_BOARDTYPE_ENET485 | ADT_DEVID_CHANNELTYPE_GLOBALS):
      case (ADT_DEVID_BOARDTYPE_ENET2_1553 | ADT_DEVID_CHANNELTYPE_GLOBALS):
      case (ADT_DEVID_BOARDTYPE_ENET_MA4 | ADT_DEVID_CHANNELTYPE_GLOBALS):
      case (ADT_DEVID_BOARDTYPE_ENETX_MA4 | ADT_DEVID_CHANNELTYPE_GLOBALS):
         *pChannelRegOffset = 0;
         break;

      /* 1553 Channel Type */
      case (ADT_DEVID_BOARDTYPE_SIM1553 | ADT_DEVID_CHANNELTYPE_1553):
         *pChannelRegOffset = ADT_L1_SIM1553_CHAN_SIZE * (*pChannel + 1) + ADT_L1_1553_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_TEST1553 | ADT_DEVID_CHANNELTYPE_1553):
         *pChannelRegOffset = ADT_L1_TEST1553_CHAN_SIZE * (*pChannel + 1) + ADT_L1_1553_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_PMC1553 | ADT_DEVID_CHANNELTYPE_1553):
         *pChannelRegOffset = ADT_L1_PMC1553_CHAN_SIZE * (*pChannel + 1) + ADT_L1_1553_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_PCI1553 | ADT_DEVID_CHANNELTYPE_1553):
         *pChannelRegOffset = ADT_L1_PCI1553_CHAN_SIZE * (*pChannel + 1) + ADT_L1_1553_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_PC104P1553 | ADT_DEVID_CHANNELTYPE_1553):
         *pChannelRegOffset = ADT_L1_PC104P1553_CHAN_SIZE * (*pChannel + 1) + ADT_L1_1553_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_PCCD1553 | ADT_DEVID_CHANNELTYPE_1553):
         *pChannelRegOffset = ADT_L1_PCCD1553_CHAN_SIZE * (*pChannel + 1) + ADT_L1_1553_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_PCI104E1553 | ADT_DEVID_CHANNELTYPE_1553):
         *pChannelRegOffset = ADT_L1_PCI104E1553_CHAN_SIZE * (*pChannel + 1) + ADT_L1_1553_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_XMC1553 | ADT_DEVID_CHANNELTYPE_1553):
         *pChannelRegOffset = ADT_L1_XMCE4L1553_CHAN_SIZE * (*pChannel + 1) + ADT_L1_1553_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_XMCMW | ADT_DEVID_CHANNELTYPE_1553):
         *pChannelRegOffset = ADT_L1_XMCMW_CHAN_SIZE * (*pChannel + 1) + ADT_L1_1553_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_ECD54_1553 | ADT_DEVID_CHANNELTYPE_1553):
         *pChannelRegOffset = ADT_L1_ECD54_1553_CHAN_SIZE * (*pChannel + 1) + ADT_L1_1553_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_PCIE4L1553 | ADT_DEVID_CHANNELTYPE_1553):
         *pChannelRegOffset = ADT_L1_PCIE4L1553_CHAN_SIZE * (*pChannel + 1) + ADT_L1_1553_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_PCIE1L1553 | ADT_DEVID_CHANNELTYPE_1553):
         *pChannelRegOffset = ADT_L1_PCIE1L1553_CHAN_SIZE * (*pChannel + 1) + ADT_L1_1553_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_MPCIE1553 | ADT_DEVID_CHANNELTYPE_1553):
         *pChannelRegOffset = ADT_L1_MPCIE1553_CHAN_SIZE * (*pChannel + 1) + ADT_L1_1553_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_MPCIE21553 | ADT_DEVID_CHANNELTYPE_1553):
         *pChannelRegOffset = ADT_L1_MPCIE21553_CHAN_SIZE * (*pChannel + 1) + ADT_L1_1553_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_PMCMA4 | ADT_DEVID_CHANNELTYPE_1553):
         *pChannelRegOffset = ADT_L1_PMCMA4_CHAN_SIZE * (*pChannel + 1) + ADT_L1_1553_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_PC104PMA4 | ADT_DEVID_CHANNELTYPE_1553):
         *pChannelRegOffset = ADT_L1_PC104PMA4_CHAN_SIZE * (*pChannel + 1) + ADT_L1_1553_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_PC104EMA4 | ADT_DEVID_CHANNELTYPE_1553):
         *pChannelRegOffset = ADT_L1_PC104EMA4_CHAN_SIZE * (*pChannel + 1) + ADT_L1_1553_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_TBOLTMA4 | ADT_DEVID_CHANNELTYPE_1553):
		 *pChannelRegOffset = ADT_L1_TBOLTMA4_CHAN_SIZE * (*pChannel + 1) + ADT_L1_1553_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_XMCMA4 | ADT_DEVID_CHANNELTYPE_1553):
         *pChannelRegOffset = ADT_L1_XMCMA4_CHAN_SIZE * (*pChannel + 1) + ADT_L1_1553_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_ENET1553 | ADT_DEVID_CHANNELTYPE_1553):
         *pChannelRegOffset = ADT_L1_ENET1553_CHAN_SIZE * (*pChannel + 1) + ADT_L1_1553_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_ENET1A1553 | ADT_DEVID_CHANNELTYPE_1553):
         *pChannelRegOffset = ADT_L1_ENET1A1553_CHAN_SIZE * (*pChannel + 1) + ADT_L1_1553_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_PMCE1553 | ADT_DEVID_CHANNELTYPE_1553):
         *pChannelRegOffset = ADT_L1_PMCE1553_CHAN_SIZE * (*pChannel + 1) + ADT_L1_1553_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_ENET485 | ADT_DEVID_CHANNELTYPE_1553):
         *pChannelRegOffset = ADT_L1_ENET485_CHAN_SIZE * (*pChannel + 1) + ADT_L1_1553_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_ENET2_1553 | ADT_DEVID_CHANNELTYPE_1553):
         *pChannelRegOffset = ADT_L1_ENET2_1553_CHAN_SIZE * (*pChannel + 1) + ADT_L1_1553_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_ENET_MA4 | ADT_DEVID_CHANNELTYPE_1553):
         *pChannelRegOffset = ADT_L1_ENET_MA4_CHAN_SIZE * (*pChannel + 1) + ADT_L1_1553_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_ENETX_MA4 | ADT_DEVID_CHANNELTYPE_1553):
         *pChannelRegOffset = ADT_L1_ENETX_MA4_CHAN_SIZE * (*pChannel + 1) + ADT_L1_1553_CHAN_REGS;
         break;



	  /* A429 Channel Type */
      case (ADT_DEVID_BOARDTYPE_ENETA429 | ADT_DEVID_CHANNELTYPE_A429):
         *pChannelRegOffset = ADT_L1_ENETA429_CHAN_SIZE * (*pChannel + 1) + ADT_L1_A429_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_ENETA429P | ADT_DEVID_CHANNELTYPE_A429):
         *pChannelRegOffset = ADT_L1_ENETA429P_CHAN_SIZE * (*pChannel + 1) + ADT_L1_A429_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_SIMA429 | ADT_DEVID_CHANNELTYPE_A429):
         *pChannelRegOffset = ADT_L1_SIMA429_CHAN_SIZE * (*pChannel + 1) + ADT_L1_A429_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_TESTA429 | ADT_DEVID_CHANNELTYPE_A429):
         *pChannelRegOffset = ADT_L1_TESTA429_CHAN_SIZE * (*pChannel + 1) + ADT_L1_A429_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_PMCA429 | ADT_DEVID_CHANNELTYPE_A429):
         *pChannelRegOffset = ADT_L1_PMCA429_CHAN_SIZE * (*pChannel + 1) + ADT_L1_A429_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_PMCA429HD | ADT_DEVID_CHANNELTYPE_A429):
         *pChannelRegOffset = ADT_L1_PMCA429HD_CHAN_SIZE * (*pChannel + 1) + ADT_L1_A429_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_PCIA429 | ADT_DEVID_CHANNELTYPE_A429):
         *pChannelRegOffset = ADT_L1_PCIA429_CHAN_SIZE * (*pChannel + 1) + ADT_L1_A429_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_PC104PA429 | ADT_DEVID_CHANNELTYPE_A429):
         *pChannelRegOffset = ADT_L1_PC104PA429_CHAN_SIZE * (*pChannel + 1) + ADT_L1_A429_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_PC104PA429LTV | ADT_DEVID_CHANNELTYPE_A429):
         *pChannelRegOffset = ADT_L1_PC104PA429LTV_CHAN_SIZE * (*pChannel + 1) + ADT_L1_A429_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_PCCDA429 | ADT_DEVID_CHANNELTYPE_A429):
         *pChannelRegOffset = ADT_L1_PCCDA429_CHAN_SIZE * (*pChannel + 1) + ADT_L1_A429_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_PCI104EA429 | ADT_DEVID_CHANNELTYPE_A429):
         *pChannelRegOffset = ADT_L1_PCI104EA429_CHAN_SIZE * (*pChannel + 1) + ADT_L1_A429_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_XMCA429 | ADT_DEVID_CHANNELTYPE_A429):
         *pChannelRegOffset = ADT_L1_XMCE4LA429_CHAN_SIZE * (*pChannel + 1) + ADT_L1_A429_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_ECD54_A429 | ADT_DEVID_CHANNELTYPE_A429):
         *pChannelRegOffset = ADT_L1_ECD54_A429_CHAN_SIZE * (*pChannel + 1) + ADT_L1_A429_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_PCIE4LA429 | ADT_DEVID_CHANNELTYPE_A429):
         *pChannelRegOffset = ADT_L1_PCIE4LA429_CHAN_SIZE * (*pChannel + 1) + ADT_L1_A429_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_PCIE1LA429 | ADT_DEVID_CHANNELTYPE_A429):
         *pChannelRegOffset = ADT_L1_PCIE1LA429_CHAN_SIZE * (*pChannel + 1) + ADT_L1_A429_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_MPCIEA429 | ADT_DEVID_CHANNELTYPE_A429):
         *pChannelRegOffset = ADT_L1_MPCIEA429_CHAN_SIZE * (*pChannel + 1) + ADT_L1_A429_CHAN_REGS;
         break;

      case (ADT_DEVID_BOARDTYPE_PMCMA4 | ADT_DEVID_CHANNELTYPE_A429):
         *pChannelRegOffset = ADT_L1_PMCMA4_CHAN_SIZE * (*pChannel) + ADT_L1_A429_CHAN_REGS + ADT_L1_MA4_START_A429_CHANNELS;
         break;

      case (ADT_DEVID_BOARDTYPE_PC104PMA4 | ADT_DEVID_CHANNELTYPE_A429):
         *pChannelRegOffset = ADT_L1_PC104PMA4_CHAN_SIZE * (*pChannel) + ADT_L1_A429_CHAN_REGS + ADT_L1_MA4_START_A429_CHANNELS;
         break;

      case (ADT_DEVID_BOARDTYPE_PC104EMA4 | ADT_DEVID_CHANNELTYPE_A429):
         *pChannelRegOffset = ADT_L1_PC104EMA4_CHAN_SIZE * (*pChannel) + ADT_L1_A429_CHAN_REGS + ADT_L1_MA4_START_A429_CHANNELS;
         break;

      case (ADT_DEVID_BOARDTYPE_TBOLTMA4 | ADT_DEVID_CHANNELTYPE_A429):
         *pChannelRegOffset = ADT_L1_TBOLTMA4_CHAN_SIZE * (*pChannel) + ADT_L1_A429_CHAN_REGS + ADT_L1_MA4_START_A429_CHANNELS;
         break;

      case (ADT_DEVID_BOARDTYPE_XMCMA4 | ADT_DEVID_CHANNELTYPE_A429):
         *pChannelRegOffset = ADT_L1_XMCMA4_CHAN_SIZE * (*pChannel) + ADT_L1_A429_CHAN_REGS + ADT_L1_MA4_START_A429_CHANNELS;
         break;

      case (ADT_DEVID_BOARDTYPE_ENET_MA4 | ADT_DEVID_CHANNELTYPE_A429):
         *pChannelRegOffset = ADT_L1_ENET_MA4_CHAN_SIZE * (*pChannel) + ADT_L1_A429_CHAN_REGS + ADT_L1_MA4_START_A429_CHANNELS;
         break;

      case (ADT_DEVID_BOARDTYPE_ENETX_MA4 | ADT_DEVID_CHANNELTYPE_A429):
         *pChannelRegOffset = ADT_L1_ENETX_MA4_CHAN_SIZE * (*pChannel) + ADT_L1_A429_CHAN_REGS + ADT_L1_MA4_START_A429_CHANNELS;
         break;



	  /* WMUX Channel Type */
	  case (ADT_DEVID_BOARDTYPE_PMCWMUX | ADT_DEVID_CHANNELTYPE_WMUX):
         *pChannelRegOffset = ADT_L1_PMCWMUX_CHAN_SIZE * (*pChannel + 1) + ADT_L1_WMUX_CHAN_REGS;
		  break;

	  case (ADT_DEVID_BOARDTYPE_XMCMW | ADT_DEVID_CHANNELTYPE_WMUX):
         *pChannelRegOffset = ADT_L1_XMCMW_CHAN_SIZE * (*pChannel + 1) + ADT_L1_WMUX_CHAN_REGS;
		  break;

      /* Anything else */
      default:
         result = ADT_ERR_UNSUPPORTED_BOARDTYPE;
         break;
   }

   return(result);
}

#ifdef __cplusplus
}
#endif

