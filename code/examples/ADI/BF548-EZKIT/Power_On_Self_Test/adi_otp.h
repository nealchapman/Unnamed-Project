/*********************************************************************************

Copyright(c) 2004 Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.

$RCSfile: adi_otp.h,v $
$Revision: 1.1.2.2 $
$Date: 2007/07/19 19:47:45 $

Description:
			This is a preliminary device driver for OTP access.

*********************************************************************************/

#ifndef __ADI_OTP_H__
#define __ADI_OTP_H__

// we are not using the official define in the adi_dev.h file for
// this because the OTP driver is not 100% complete in this release
#define ADI_OTP_ENUMERATION_START 0x40240000

/*********************************************************************

Extensible enumerations and defines

*********************************************************************/

enum {												// Command IDs
	ADI_OTP_CMD_START=ADI_OTP_ENUMERATION_START,    // 0x400x0000 - starting point
    ADI_OTP_CMD_SET_ACCESS_MODE,                    // 0x0001 - do some command			(value = something relevant to this command)
    ADI_OTP_CMD_SET_TIMING,
    ADI_OTP_CMD_CLOSE,                    			// 0x400x0001 - do some command			(value = something relevant to this command)
    ADI_OTP_CMD_INIT,                    			// 0x400x0001 - do some command			(value = something relevant to this command)
};



enum {												// Return codes
	ADI_OTP_RESULT_START=ADI_OTP_ENUMERATION_START,	// 0x400x0000 - starting point
	ADI_OTP_MASTER_ERROR,				    		// 0x400x0001 - some special return code for this driver
	ADI_OTP_WRITE_ERROR,
	ADI_OTP_READ_ERROR,
	ADI_OTP_ACC_VIO_ERROR,
	ADI_OTP_DATA_MULT_ERROR,
	ADI_OTP_ECC_MULT_ERROR,
	ADI_OTP_PREV_WR_ERROR,
	ADI_OTP_DATA_SB_WARN,
	ADI_OTP_ECC_SB_WARN,
};

typedef struct {
	volatile u32 	page;
	volatile u32 	flags;
}ADI_OTP_INFO;

#define	ADI_OTP_INIT			(0x00000001)
#define	ADI_OTP_CLOSE			(0x00000002)

#define ADI_OTP_LOWER_HALF      (0x00000000)	// select upper/lower 64-bit half (bit 0)
#define ADI_OTP_UPPER_HALF      (0x00000001)
#define ADI_OTP_NO_ECC 			(0x00000010)	// do not use ECC (bit 4)
#define ADI_OTP_LOCK 			(0x00000020)	// sets page protection bit for page (bit 5)

#define ADI_OTP_ACCESS_READ			(0x1000)		// read-only
#define ADI_OTP_ACCESS_READWRITE	(0x2000)		// read/write

/*********************************************************************

Data Structures

*********************************************************************/

extern ADI_DEV_PDD_ENTRY_POINT ADIOTPEntryPoint;		// entry point to the device driver





#endif // __ADI_SPI_H
