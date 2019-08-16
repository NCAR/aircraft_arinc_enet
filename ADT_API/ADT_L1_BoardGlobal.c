/******************************************************************************
 * FILE:			ADT_L1_BoardGlobal.c
 *
 * Copyright (c) 2007-2016, Alta Data Technologies LLC (ADT), All Rights Reserved.
 * Use of this software is subject to the ADT Software License (latest
 * revision), US Government or local laws and the ADT Terms and Conditions 
 * of Sale (latest revision).
 *
 * DESCRIPTION:
 *	Source file for Layer 1 API.
 *	Contains functions that use the board-level global registers (only usable
 *  by the "GLOBALS" channel type, ADT_DEVID_CHANNELTYPE_GLOBALS).
 *
 *****************************************************************************/
/*! \file ADT_L1_BoardGlobal.c  
 *  \brief Source file containing functions that use the board-level global registers 
 */
#include "ADT_L1.h"

/* Internal Constants */
#define I2C_STS_ACK			0x00800000
#define I2C_STS_BUSY		0x00400000
#define I2C_STS_TIP 		0x00020000

#define SPI_WRITE_ENABLE	0x01000000
#define SPI_VSELECT1		0x00000100
#define I2C_CNTL_START		0x00800000
#define I2C_CNTL_STOP		0x00400000
#define I2C_CNTL_RD			0x00200000
#define I2C_CNTL_WR			0x00100000

#define I2C_CNTL_NACK		0x00080000

#define I2C_CNTL_CMD_WR		0x00000000
#define I2C_CNTL_CMD_RD		0x00000001

#define I2C_DAC_ADDR	    0x00000036
#define I2C_VV_CH1_ADDR		0x00000038
#define I2C_VV_CH2_ADDR		0x0000003A
#define I2C_VV_CH3_ADDR		0x0000003C
#define I2C_VV_CH4_ADDR		0x0000003E

#ifdef __cplusplus
extern "C" {
#endif

/* Prototypes for internal functions */
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_Global_I2C_Read32(ADT_L0_UINT32 devID, 
														ADT_L0_UINT32 value, 
														ADT_L0_UINT32 *return_val);
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_Global_I2C_Write32(ADT_L0_UINT32 devID, 
														ADT_L0_UINT32 value);

/******************************************************************************
  FUNCTION:		ADT_L1_Global_TimeClear
 *****************************************************************************/
/*! \brief Clears the time-tag registers for all channels on the board 
 *
 * This function clears the time-tag registers for all channels on the board.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid device number
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_Global_TimeClear(ADT_L0_UINT32 devID) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 temp = 0;

	/* Only applies to GLOBAL devices */
	if ((devID & 0x0000FF00) == ADT_DEVID_CHANNELTYPE_GLOBALS)
	{
		/* Write to Global CSR to clear all time tags (set bit 0) */
		result = ADT_L1_ReadDeviceMem32(devID, ADT_L1_GLOBAL_CSR, &temp, 1);
		temp |= ADT_L1_GLOBAL_CSR_CLRTT;
		result = ADT_L1_WriteDeviceMem32(devID, ADT_L1_GLOBAL_CSR, &temp, 1);
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_Global_ConfigExtClk
 *****************************************************************************/
/*! \brief Configures the Global CSR external clock settings 
 *
 * This function configures the Global CSR external clock settings.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param clkFreq must be 1, 5, or 10 (1MHz, 5MHz, or 10MHz)
 * @param clkSrcIn is the input clock signal (0 = NONE/disabled, 1 = RS485, 2 = TTL)
 * @param clkSrcOut is the output clock signal (0 = NONE/disabled, 1 = RS485, 2 = TTL)
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid device number or invalid clkFreq
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_Global_ConfigExtClk(ADT_L0_UINT32 devID, ADT_L0_UINT32 clkFreq, ADT_L0_UINT32 clkSrcIn, ADT_L0_UINT32 clkSrcOut) {
	ADT_L0_UINT32 result = ADT_SUCCESS;
	ADT_L0_UINT32 temp = 0;

	/* Only applies to GLOBAL devices */
	if ((devID & 0x0000FF00) == ADT_DEVID_CHANNELTYPE_GLOBALS)
	{
		/* Verify valid frequency */
		if ((clkFreq != 1) && (clkFreq != 5) && (clkFreq != 10))
			result = ADT_ERR_BAD_INPUT;
		else {
			result = ADT_L1_ReadDeviceMem32(devID, ADT_L1_GLOBAL_CSR, &temp, 1);

			/* Clear time bits and apply new settings */
			temp &= ~(ADT_L1_GLOBAL_CSR_EC_FRQOUT_1MHZ | ADT_L1_GLOBAL_CSR_EC_FRQOUT_5MHZ | ADT_L1_GLOBAL_CSR_EC_FRQOUT_10MHZ);
			temp &= ~(ADT_L1_GLOBAL_CSR_EC_SRCIN_485 | ADT_L1_GLOBAL_CSR_EC_SRCIN_TTL);
			temp &= ~(ADT_L1_GLOBAL_CSR_EC_SRCOUT_485 | ADT_L1_GLOBAL_CSR_EC_SRCOUT_TTL);

			switch (clkFreq) {
			case 1:
				temp |= ADT_L1_GLOBAL_CSR_EC_FRQOUT_1MHZ;
				break;
			case 5:
				temp |= ADT_L1_GLOBAL_CSR_EC_FRQOUT_5MHZ;
				break;
			case 10:
				temp |= ADT_L1_GLOBAL_CSR_EC_FRQOUT_10MHZ;
				break;
			}

			switch (clkSrcIn) {
				case 1:
					temp |= ADT_L1_GLOBAL_CSR_EC_SRCIN_485;
					break;
				case 2:
					temp |= ADT_L1_GLOBAL_CSR_EC_SRCIN_TTL;
					break;
			}

			switch (clkSrcOut) {
				case 1:
					temp |= ADT_L1_GLOBAL_CSR_EC_SRCOUT_485;
					break;
				case 2:
					temp |= ADT_L1_GLOBAL_CSR_EC_SRCOUT_TTL;
					break;
			}

			result = ADT_L1_WriteDeviceMem32(devID, ADT_L1_GLOBAL_CSR, &temp, 1);
		}
	}
	else result = ADT_ERR_BAD_INPUT;

	return( result );
}


/******************************************************************************
  FUNCTION:		ADT_L1_Global_I2C_ReadTemp
 *****************************************************************************/
/*! \brief Reads a temperature value from the I2C bus 
 *
 * This function reads a temperature value from the I2C bus.
 * Notes:
 * 1. If the temp data MSB bit D10 = 0 then the temperature is positive and Temp value (°C) = + (Temp data) * 0.125 °C
 * 2. If the temp data MSB bit D10 = 1 then the temperature is negative and Temp value (°C) = – (2’s complement of Temp data) * 0.125 °C
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param temp_addr is the offset of the temp sensor to read (ADT_L1_GLOBAL_I2C_TS_FPGA_ADDR or ADT_L1_GLOBAL_I2C_TS_XCVR_ADDR).
 * @param temp is the pointer to store the value read.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid device number
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_Global_I2C_ReadTemp(ADT_L0_UINT32 devID, ADT_L0_UINT32 temp_addr, ADT_L0_UINT32 *temp)
{
	ADT_L0_UINT32 data, temp_msb, temp_lsb;
	ADT_L0_UINT32 result = ADT_SUCCESS;

	/* Only applies to GLOBAL devices */
	if ((devID & 0x0000FF00) == ADT_DEVID_CHANNELTYPE_GLOBALS)
	{
		/* check if I2C bus busy - stop the I2C controller if busy since it shouldn't be at this point. */
		result = ADT_L0_ReadMem32(devID, ADT_L1_GLOBAL_I2CSTS, &data, 1);
		if(data & I2C_STS_BUSY)
		{
			data = I2C_CNTL_STOP;
			ADT_L0_WriteMem32(devID, ADT_L1_GLOBAL_I2CCTL, &data, 1);
		}

		/* write command to temp sensor address */
		result = ADT_L1_Global_I2C_Write32(devID, temp_addr | I2C_CNTL_CMD_WR | I2C_CNTL_START | I2C_CNTL_WR);

		/* Select Reg0 on device */
		result = ADT_L1_Global_I2C_Write32(devID, I2C_CNTL_WR);

		/* issue read commnad */
		result = ADT_L1_Global_I2C_Write32(devID, temp_addr | I2C_CNTL_CMD_RD | I2C_CNTL_START | I2C_CNTL_WR);

		/* read temp MSB */
		result = ADT_L1_Global_I2C_Read32(devID, I2C_CNTL_RD, &temp_msb);

		/* read temp LSB */
		result = ADT_L1_Global_I2C_Read32(devID, I2C_CNTL_RD | I2C_CNTL_STOP | I2C_CNTL_NACK, &temp_lsb);

		/* shift the MSB by 8 and OR with the LSB. The result is then shifted right
		 * five bits because the lower 5 bits are not used.
		 */
		*temp = ((temp_msb<<8) | temp_lsb) >> 5;

	}
	else result = ADT_ERR_BAD_INPUT;

	return (result);
}




/******************************************************************************
  FUNCTION:		ADT_L1_Global_I2C_SetIrigDac
 *****************************************************************************/
/*! \brief Writes a voltage value to the IRIG DAC on the I2C bus 
 *
 * This function writes a voltage value to the IRIG DAC on the I2C bus.
 * Notes: The LSB of value represents 3.22266mV (3.3/1024).
 *        i.e. to set the dac output to 1.65V, the value should be set to 512 (0x200)
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param value is the value to set the IRIG DAC to.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid device number
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_Global_I2C_SetIrigDac(ADT_L0_UINT32 devID, ADT_L0_UINT32 value)
{
	ADT_L0_UINT32 data;
	ADT_L0_UINT32 result = ADT_SUCCESS;

	/* Only applies to GLOBAL devices */
	if ((devID & 0x0000FF00) == ADT_DEVID_CHANNELTYPE_GLOBALS)
	{
		/* check if I2C bus busy - stop the I2C controller if busy since it shouldn't be at this point. */
		result = ADT_L0_ReadMem32(devID, ADT_L1_GLOBAL_I2CSTS, &data, 1);
		if (data & I2C_STS_BUSY)
		{
			data = I2C_CNTL_STOP;
			result = ADT_L0_WriteMem32(devID, ADT_L1_GLOBAL_I2CCTL, &data, 1);
		}

		/* Short delay */
		ADT_L0_msSleep(1);

		/* write command to IRIG DAC address */
		result = ADT_L1_Global_I2C_Write32(devID, I2C_DAC_ADDR | I2C_CNTL_CMD_WR | I2C_CNTL_START | I2C_CNTL_WR);

		/* for VV, I2C_DAC_ADDR will be replaced with:
		  Channel 1 = 0x38
		  ch 2 = 0x3A
		  ch 3 = 0x3C
		  ch 4 = 0x3E */

		/* write 0 to command register
		 *  - Update on Stop condition only
		 *  - device in standard operating mode
		 *  - Selects the supply as the reference
		 */
		result = ADT_L1_Global_I2C_Write32(devID, I2C_CNTL_WR);

		/* write LSB */
		data = value & 0x000000FF;
		result = ADT_L1_Global_I2C_Write32(devID, data | I2C_CNTL_WR);

		/* write MSB */
		data = (value >> 8) & 0x000000FF;
		result = ADT_L1_Global_I2C_Write32(devID, data | I2C_CNTL_WR | I2C_CNTL_STOP );
	}
	else result = ADT_ERR_BAD_INPUT;

	return (result);
}


/******************************************************************************
  FUNCTION:		ADT_L1_Global_I2C_SetVVDac
 *****************************************************************************/
/*! \brief Writes a voltage value to the 1553 Variable Voltage DAC on the I2C bus 
 *
 * This function writes a voltage value to the VV DAC on the I2C bus.
 * Notes: The LSB of value represents 4.8828125mV (5v/1024).
 *        5v on the DAC sets MAXIMUM voltage on the bus (about 22 volts peak to peak)
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param devID is the 1553 channel number (0, 1, 2, or 3).
 * @param value is the value to set the VV DAC to (0 to 1023).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid device number or value
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_Global_I2C_SetVVDac(ADT_L0_UINT32 devID, ADT_L0_UINT32 channel, ADT_L0_UINT32 value)
{
	ADT_L0_UINT32 data;
	ADT_L0_UINT32 result = ADT_SUCCESS;

	/* Only applies to GLOBAL devices */
	if ((devID & 0x0000FF00) == ADT_DEVID_CHANNELTYPE_GLOBALS)
	{
		if((devID & 0x0FF00000) == ADT_DEVID_BOARDTYPE_ENETA429P)
		{
			// check if I2C bus busy - stop the I2C controller if busy since it shouldn't be at this point.
			result = ADT_L0_ReadMem32(devID, ADT_L1_GLOBAL_I2CSTS, &data, 1);
			if(data & I2C_STS_BUSY)
			{
				data = I2C_CNTL_STOP;
				ADT_L0_WriteMem32(devID, ADT_L1_GLOBAL_I2CCTL, &data, 1);
			}
 
			// check levels
			if(value > 0xFF) value = 0xFF; // max level
			if(value < 0x40) value = 0x40; // min level
 
			// write command to AD5263 address
			// AD5263 slave address byte = 01011, AD1, AD0, R/W-
			// AD1 and AD0 on ENET-A429P are both ground
			// AD5263 slave address = 0101100 (0x2C << 1 = 0x58)
			// I2C Control - Start bit and write.
			result = ADT_L1_Global_I2C_Write32(devID, 0x58 | I2C_CNTL_CMD_WR | I2C_CNTL_START | I2C_CNTL_WR);
 
			// AD5263 Instruction Byte = X, A1, A0, RS, SD, O1, O2, X
			// A1, A0 are wiper address.  RS is Reset to midscale.  O1 and O2 are Digital output pins (unused).
			// I2C Control - Write.
			data = (channel & 0x03) << 5;
			result = ADT_L1_Global_I2C_Write32(devID, data | I2C_CNTL_WR);
 
			// AD5263 Data Byte = D7-D0
			// I2C Control - Stop bit and write.
			result = ADT_L1_Global_I2C_Write32(devID, value | I2C_CNTL_STOP | I2C_CNTL_WR);
 
			/* Short delay */
			ADT_L0_msSleep(1);
 
		}
		else if((devID & 0x0FF00000) == ADT_DEVID_BOARDTYPE_TBOLTMA4)
		{
			/* check input limit*/
			if(value < 1024)
			{
				data = (ADT_L0_UINT32)(value /.101);
				if(data > 0xFF) data = 0xFF; // max level
				if(data < 0x00) data = 0x00; // min level

				data |= SPI_VSELECT1;		/* Set VSEL to 1 to use 0-26 Range on SPI */
				data |= (channel << 16);	/* or in channel number */
				data |= I2C_CNTL_START;		/* or in start bit */
				data |= SPI_WRITE_ENABLE; 	/* Enable SPI write instead of I2C write (1 << 24) or 0x01000000 */

				/* Write voltage */
				result = ADT_L1_Global_I2C_Write32(devID, data);

			}
			else 
				result = ADT_ERR_BAD_INPUT;
		}
		else
		{
			/* check input limit*/
			if(value < 1024)
			{
				/* check if I2C bus busy - stop the I2C controller if busy since it shouldn't be at this point. */
				result = ADT_L0_ReadMem32(devID, ADT_L1_GLOBAL_I2CSTS, &data, 1);
				if (data & I2C_STS_BUSY)
				{
					data = I2C_CNTL_STOP;
					result = ADT_L0_WriteMem32(devID, ADT_L1_GLOBAL_I2CCTL, &data, 1);
				}

				/* Short delay */
				ADT_L0_msSleep(1);

				/* write command to VV DAC address */
				switch (channel) {
					case 0:
						result = ADT_L1_Global_I2C_Write32(devID, I2C_VV_CH1_ADDR | I2C_CNTL_CMD_WR | I2C_CNTL_START | I2C_CNTL_WR);
						break;

					case 1:
						result = ADT_L1_Global_I2C_Write32(devID, I2C_VV_CH2_ADDR | I2C_CNTL_CMD_WR | I2C_CNTL_START | I2C_CNTL_WR);
						break;

					case 2:
						result = ADT_L1_Global_I2C_Write32(devID, I2C_VV_CH3_ADDR | I2C_CNTL_CMD_WR | I2C_CNTL_START | I2C_CNTL_WR);
						break;

					case 3:
					default:
						result = ADT_L1_Global_I2C_Write32(devID, I2C_VV_CH4_ADDR | I2C_CNTL_CMD_WR | I2C_CNTL_START | I2C_CNTL_WR);
						break;

				}

				/* write 0 to command register
				*  - Update on Stop condition only
				*  - device in standard operating mode
				*  - Selects the supply as the reference
				*/
				result = ADT_L1_Global_I2C_Write32(devID, I2C_CNTL_WR);

				/* write LSB */
				data = value & 0x000000FF;
				result = ADT_L1_Global_I2C_Write32(devID, data | I2C_CNTL_WR);

				/* write MSB */
				data = (value >> 8) & 0x000000FF;
				result = ADT_L1_Global_I2C_Write32(devID, data | I2C_CNTL_WR | I2C_CNTL_STOP );
			}
			else result = ADT_ERR_BAD_INPUT;
		}
	}
	else result = ADT_ERR_BAD_INPUT;

	return (result);
}


/******************************************************************************
  FUNCTION:		ADT_L1_Global_ReadIrigTime
 *****************************************************************************/
/*! \brief Reads the IRIG time. 
 *
 * This function reads the IRIG time.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param irigHigh is a pointer to store the high 32-bits of the IRIG time.
 * @param irigLow is a pointer to store the low 32-bits of the IRIG time.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid device number
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_Global_ReadIrigTime(ADT_L0_UINT32 devID, ADT_L0_UINT32 *irigHigh, ADT_L0_UINT32 *irigLow)
{
	ADT_L0_UINT32 data, locked, temp, tempHigh, tempLow;
	ADT_L0_UINT32 seconds, minutes, hours, days, years;
	ADT_L0_UINT32 result = ADT_SUCCESS;

	locked = 0;

	/* Only applies to GLOBAL devices */
	if ((devID & 0x0000FF00) == ADT_DEVID_CHANNELTYPE_GLOBALS)
	{
		/* Check for IRIG lock */
		result = ADT_L0_ReadMem32(devID, ADT_L1_GLOBAL_CSR, &temp, 1);
		if (temp & ADT_L1_GLOBAL_CSR_IRIG_LOCK) locked = 1;
		else locked = 0;

		if (locked) 
		{
			/* Write to Global CSR to latch IRIG time */
			temp |= ADT_L1_GLOBAL_CSR_IRIG_LATCH;
			result = ADT_L0_WriteMem32(devID, ADT_L1_GLOBAL_CSR, &temp, 1);

			/* Read IRIG time high */
			result = ADT_L0_ReadMem32(devID, ADT_L1_GLOBAL_IRIGTIME_HIGH, &data, 1);
			tempHigh = data;

			/* Read IRIG time low */
			result = ADT_L0_ReadMem32(devID, ADT_L1_GLOBAL_IRIGTIME_LOW, &data, 1);
			tempLow = data;

			/* Extract BCD fields to binary seconds, minutes, hours, days, years */
			seconds = ((tempLow & 0x000000F0) >> 4) * 10 + (tempLow & 0x0000000F);
			minutes = ((tempLow & 0x0000F000) >> 12) * 10 + ((tempLow & 0x00000F00) >> 8);
			hours = ((tempLow & 0x00F00000) >> 20) * 10 + ((tempLow & 0x000F0000) >> 16);
			days = ((tempHigh & 0x00000F00) >> 8) * 100 + ((tempHigh & 0x000000F0) >> 4) * 10 + (tempHigh & 0x0000000F);
			years = ((tempHigh & 0x000F0000) >> 16) * 10 + ((tempHigh & 0x0000F000) >> 12);

			/* Add one second to the IRIG time - the time latched by the firmware lags by one second */
			seconds++;
			if (seconds > 59)
			{
				seconds = 0;
				minutes++;
				if (minutes > 59)
				{
					minutes = 0;
					hours++;
					if (hours > 23)
					{
						hours = 0;
						days++;
						if (days > 365) 
						{
							days = 1;
							years++;
						}
					}
				}
			}

			/* Convert binary seconds, minutes, hours, days, years back to BCD */
			tempLow = tempHigh = 0;
			
			tempLow |= seconds - (seconds / 10) * 10;
			tempLow |= (seconds / 10) << 4;

			tempLow |= (minutes - (minutes / 10) * 10) << 8;
			tempLow |= (minutes / 10) << 12;

			tempLow |= (hours - (hours / 10) * 10) << 16;
			tempLow |= (hours / 10) << 20;

			tempHigh |= days - (days / 10) * 10;
			tempHigh |= ((days - (days / 100) * 100) / 10) << 4;
			tempHigh |= (days / 100) << 8;

			tempHigh |= (years - (years / 10) * 10) << 12;
			tempHigh |= (years / 10) << 16;

			/* This is our return value */
			*irigHigh = tempHigh;
			*irigLow = tempLow;
		}
		else
		{
			result = ADT_FAILURE;
		}
	}
	else result = ADT_ERR_BAD_INPUT;

	return (result);
}


/******************************************************************************
  FUNCTION:		ADT_L1_Global_CalibrateIrigDac
 *****************************************************************************/
/*! \brief Calibrates the IRIG DAC for the input IRIG signal. 
 *
 * This function calibrates the IRIG DAC for the input IRIG signal.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid device number
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_Global_CalibrateIrigDac(ADT_L0_UINT32 devID)
{
	ADT_L0_UINT32 data, locked, temp;
	ADT_L0_UINT32 result = ADT_SUCCESS;

	locked = 0;

	/* Only applies to GLOBAL devices */
	if ((devID & 0x0000FF00) == ADT_DEVID_CHANNELTYPE_GLOBALS)
	{
		/* First set typical value - if we get lock, we are done */
		data = 100;
		result = ADT_L1_Global_I2C_SetIrigDac(devID, data);
		ADT_L0_msSleep(1000);

		/* Read Global CSR, check IRIG lock bit */
		result = ADT_L0_ReadMem32(devID, ADT_L1_GLOBAL_CSR, &temp, 1);
		if (temp & ADT_L1_GLOBAL_CSR_IRIG_LOCK) locked = 1;
		else locked = 0;

		/* If we don't get lock, then we have to do a full calibration cycle */
		if (!locked) {
			/* Start with DAC at minimum value of 0 (max is 1023 or 0x3FF) */
			data = 0;
			result = ADT_L1_Global_I2C_SetIrigDac(devID, data);
			ADT_L0_msSleep(1000);

			/* Increase DAC value until we get lock (must wait 1 sec for IRIG sync) - STEPS of 25 */
			while ((!locked) && (data < 1000))
			{
				data += 25;
				result = ADT_L1_Global_I2C_SetIrigDac(devID, data);
				ADT_L0_msSleep(1000);

				/* Read Global CSR, check IRIG lock bit */
				result = ADT_L0_ReadMem32(devID, ADT_L1_GLOBAL_CSR, &temp, 1);
				if (temp & ADT_L1_GLOBAL_CSR_IRIG_LOCK) locked = 1;
				else locked = 0;
			}

			if (!locked) result = ADT_FAILURE;
		}
	}
	else result = ADT_ERR_BAD_INPUT;

	return (result);
}



/******************************************************************************
  FUNCTION:		ADT_L1_Global_CalibrateIrigDacOptions
 *****************************************************************************/
/*! \brief Calibrates the IRIG DAC with User Options for the input IRIG signal. 
 *
 * This function calibrates the IRIG DAC with User Options for the input IRIG signal.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param options is a unit to provide extended options for IRIG Cal.  Included is an 8-bit field for stepping
 *			the DAC calibration
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid device number
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_Global_CalibrateIrigDacOptions(ADT_L0_UINT32 devID, ADT_L0_UINT32 options)
{
	ADT_L0_UINT32 data, locked, data_high, data_low, dacStep, temp;
	ADT_L0_UINT32 result = ADT_SUCCESS;

	locked = 0;
	dacStep = (options & 0xFF000000) >> 24;

	if (dacStep == 0) {
		result = ADT_ERR_BAD_INPUT;
	}
	else {
		/* Only applies to GLOBAL devices */
		if ((devID & 0x0000FF00) == ADT_DEVID_CHANNELTYPE_GLOBALS)
		{
			/* First set typical value - if we get lock, we are done */
			data = 100;
			result = ADT_L1_Global_I2C_SetIrigDac(devID, data);
			ADT_L0_msSleep(1000);

			/* Read Global CSR, check IRIG lock bit */
			result = ADT_L0_ReadMem32(devID, ADT_L1_GLOBAL_CSR, &temp, 1);
			if (temp & ADT_L1_GLOBAL_CSR_IRIG_LOCK) locked = 1;
			else locked = 0;

			/* If we don't get lock, then we have to do a full calibration cycle */
			if (!locked) {
				/* Start with DAC at max value (1023 or 0x3FF) */
				data = 1023;
				result = ADT_L1_Global_I2C_SetIrigDac(devID, data);
				ADT_L0_msSleep(1000);

				/* Decrease DAC value until we get lock (must wait 1 sec for IRIG sync) - now within high amplitude range */
				/* COARSE STEPS of 50 */
				while ((!locked) && (data > 50))
				{
					data -= 50;
					result = ADT_L1_Global_I2C_SetIrigDac(devID, data);
					ADT_L0_msSleep(1000);

					/* Read Global CSR, check IRIG lock bit */
					result = ADT_L0_ReadMem32(devID, ADT_L1_GLOBAL_CSR, &temp, 1);
					if (temp & ADT_L1_GLOBAL_CSR_IRIG_LOCK) locked = 1;
					else locked = 0;
				}

				if (!locked) result = ADT_FAILURE;
				else 
				{
					/* Go back to value before we got lock */
					data += 100;
					locked = 0;
					result = ADT_L1_Global_I2C_SetIrigDac(devID, data);
					ADT_L0_msSleep(1000);

					/* Decrease DAC value until we get lock (must wait 1 sec for IRIG sync) - now within high amplitude range */
					/* FINE STEPS of dacStep */
					while ((!locked) && (data > dacStep))
					{
						data -= dacStep;
						result = ADT_L1_Global_I2C_SetIrigDac(devID, data);
						ADT_L0_msSleep(1000);

						/* Read Global CSR, check IRIG lock bit */
						result = ADT_L0_ReadMem32(devID, ADT_L1_GLOBAL_CSR, &temp, 1);
						if (temp & ADT_L1_GLOBAL_CSR_IRIG_LOCK) locked = 1;
						else locked = 0;
					}

					if (!locked) result = ADT_FAILURE;
					else 
					{
						data_high = data;

						/* Decrease DAC value until we lose lock, coarse steps of 100 - now within low amplitude range */
						while ((locked) && (data > 50))
						{
							data -= 50;
							result = ADT_L1_Global_I2C_SetIrigDac(devID, data);
							ADT_L0_msSleep(1000);

							/* Read Global CSR, check IRIG lock bit */
							result = ADT_L0_ReadMem32(devID, ADT_L1_GLOBAL_CSR, &temp, 1);
							if (temp & ADT_L1_GLOBAL_CSR_IRIG_LOCK) locked = 1;
							else locked = 0;
						}

						/* If we reached data == 0 without losing lock, then this is probably TTL DCLS signal, set to 50% */
						if (locked) 
						{
							/* Set DAC to be 50% below point where we got lock */
							data = data_high / 2;
							result = ADT_L1_Global_I2C_SetIrigDac(devID, data);
						}
						else
						{
							/* Go back to value before we lost lock */
							data += 100;
							locked = 1;
							result = ADT_L1_Global_I2C_SetIrigDac(devID, data);
							ADT_L0_msSleep(1000);

							/* Decrease DAC value until we lose lock, fine steps of dacStep - now within low amplitude range */
							while ((locked) && (data > dacStep))
							{
								data -= dacStep;
								result = ADT_L1_Global_I2C_SetIrigDac(devID, data);
								ADT_L0_msSleep(1000);

								/* Read Global CSR, check IRIG lock bit */
								result = ADT_L0_ReadMem32(devID, ADT_L1_GLOBAL_CSR, &temp, 1);
								if (temp & ADT_L1_GLOBAL_CSR_IRIG_LOCK) locked = 1;
								else locked = 0;
							}

							/* If we reached data == 0 without losing lock, then this is probably TTL DCLS signal, set to 50% */
							if (locked) {
								/* Set DAC to be 50% below point where we got lock */
								data = data_high / 2;
								result = ADT_L1_Global_I2C_SetIrigDac(devID, data);
							}
							/* Otherwise, set to 10% above low amplitude */
							else {
								data_low = data;

								/* Set DAC to be 10% above point where we lost lock */
								data = data_low + (data_high - data_low) / 10;
								result = ADT_L1_Global_I2C_SetIrigDac(devID, data);
							}
						}
					}
				}
			}
		}
		else result = ADT_ERR_BAD_INPUT;
	}

	return (result);
}


/******************************************************************************
  INTERNAL FUNCTION:		ADT_L1_Global_I2C_Read32
 *****************************************************************************/
/*! \brief Reads a 32-bit word from the I2C bus 
 *
 * This function reads a 32-bit word from the I2C bus.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param value is the offset to read.
 * @param return_val is a pointer to store the value read.
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid device number
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_Global_I2C_Read32(ADT_L0_UINT32 devID, ADT_L0_UINT32 value, ADT_L0_UINT32 *return_val)
{
	ADT_L0_UINT32 data;
	ADT_L0_UINT32 result = ADT_SUCCESS;

	/* Only applies to GLOBAL devices */
	if ((devID & 0x0000FF00) == ADT_DEVID_CHANNELTYPE_GLOBALS)
	{
		/* send read command */
		data = value;
		result = ADT_L0_WriteMem32(devID, ADT_L1_GLOBAL_I2CCTL, &data, 1);

		/* wait for transfer to complete */
		do{
			result = ADT_L0_ReadMem32(devID, ADT_L1_GLOBAL_I2CSTS, &data, 1);
			ADT_L0_msSleep(1);
		} while (data & I2C_STS_TIP); /* timeout here? */

		/* get data */
		result = ADT_L0_ReadMem32(devID, ADT_L1_GLOBAL_I2CSTS, &data, 1);

		/* read status/data */
		*return_val = data & 0x000000FF;
	}
	else result = ADT_ERR_BAD_INPUT;

	return (result);
}



/******************************************************************************
  INTERNAL FUNCTION:		ADT_L1_Global_I2C_Write32
 *****************************************************************************/
/*! \brief Writes a 32-bit word from the I2C bus 
 *
 * This function writes a 32-bit word from the I2C bus.
 *
 * @param devID is the device identifier (Backplane, Board Type, Board #, Channel Type, Channel #).
 * @param value is the value to write
 * @return 
	- \ref ADT_SUCCESS - Completed without error
	- \ref ADT_ERR_BAD_INPUT - Invalid device number
	- \ref ADT_FAILURE - Completed with error
*/
ADT_L0_UINT32 ADT_L0_CALL_CONV ADT_L1_Global_I2C_Write32(ADT_L0_UINT32 devID, ADT_L0_UINT32 value)
{
	ADT_L0_UINT32 data, result;

	result = ADT_SUCCESS;

	/* Only applies to GLOBAL devices */
	if ((devID & 0x0000FF00) == ADT_DEVID_CHANNELTYPE_GLOBALS)
	{
		data = value;
		result = ADT_L0_WriteMem32(devID, ADT_L1_GLOBAL_I2CCTL, &data, 1);

		/* wait for transfer to complete */
		do{
			result = ADT_L0_ReadMem32(devID, ADT_L1_GLOBAL_I2CSTS, &data, 1);
			ADT_L0_msSleep(1);
		} while (data & I2C_STS_TIP); /* timeout here? */

		/* check for ack */
		result = ADT_L0_ReadMem32(devID, ADT_L1_GLOBAL_I2CSTS, &data, 1);
		if (data & I2C_STS_ACK)
			result = ADT_FAILURE; /* no command ack */
		else
			result = ADT_SUCCESS;
	}
	else result = ADT_ERR_BAD_INPUT;

	return(result);
}


#ifdef __cplusplus
}
#endif
