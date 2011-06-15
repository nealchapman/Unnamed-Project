/*******************************************************************
Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software
you agree to the terms of the associated Analog Devices License Agreement.

Project Name:  	Power_On_Self_Test

Hardware:		ADSP-BF548 EZ-KIT Lite

Description:	This file includes some common PC28F128 flash functions
*******************************************************************/


/*******************************************************************
*  include files
*******************************************************************/
#include <services/services.h>		// system service includes
#include <drivers/adi_dev.h>		// device manager includes
#include <drivers\flash\util.h>				// library struct includes
#include <drivers\flash\adi_pc28f128k.h>	// flash-PC28F128 includes
#include <drivers\flash\Errors.h>			// error type includes

#include "adi_ssl_Init.h"
#include "PC28F128_common.h"


/*******************************************************************
*  global variables and defines
*******************************************************************/
ADI_DEV_DEVICE_HANDLE DevHandlePC28F128K;

int 			AFP_ManCode 		= -1;			// 0x20 	= Intel
int 			AFP_DevCode 		= -1;			// 0x22CB 	= PC28F128K

/*******************************************************************
*   Function:    pc28f128_Callback
*   Description: Invoked when the driver needs to notify the application
*	             of something
*******************************************************************/
void pc28f128_Callback(void *ClientHandle, u32 Event, void *pArg)
{
	// just return
}

/*******************************************************************
*   Function:    SetupForFlash
*   Description: Perform necessary setup for the processor to talk to the
*				 flash such as external memory interface registers, etc.
*******************************************************************/
bool SetupForFlash()
{

	*pEBIU_MODE = 0x1;		// bank 0 async access mode
	*pEBIU_AMGCTL = 0x3;

	// set port H MUX to configure PH8-PH13 as 1st Function (MUX = 00)
	// (bits 16-27 == 0) - Address signals A4-A9
	*pPORTH_FER = 0x3F00;
	*pPORTH_MUX = 0x0;
	*pPORTH_DIR_SET = 0x3F00;

	// configure PI0-PI15 as A10-A21 respectively
	*pPORTI_FER = 0xFFFF;
	*pPORTI_MUX = 0x0;
	*pPORTI_DIR_SET = 0xFFFF;

	return true;
}

/*******************************************************************
*   Function:    GetFlashInfo
*   Description: Get the manufacturer code and device code
*******************************************************************/
unsigned int GetFlashInfo()
{
	unsigned int iResult;				// result
	static GET_CODES_STRUCT  SGetCodes;	//structure for GetCodes
	COMMAND_STRUCT CmdStruct;

	//setup code so that flash programmer can just read memory instead of call GetCodes().
	CmdStruct.SGetCodes.pManCode = (unsigned long *)&AFP_ManCode;
	CmdStruct.SGetCodes.pDevCode = (unsigned long *)&AFP_DevCode;
	CmdStruct.SGetCodes.ulFlashStartAddr = FLASH_START_ADDR;

	iResult = adi_dev_Control( DevHandlePC28F128K, CNTRL_GET_CODES, (void *)&CmdStruct);

	return(iResult);
}
