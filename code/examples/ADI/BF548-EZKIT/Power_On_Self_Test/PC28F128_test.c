/*******************************************************************
Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software
you agree to the terms of the associated Analog Devices License Agreement.  

Project Name:  	Power_On_Self_Test

Hardware:		ADSP-BF548 EZ-KIT Lite

Description:	This file tests the burst flash interface on the EZ-KIT
*******************************************************************/

/*******************************************************************
*  include files
*******************************************************************/
#include <services/services.h>
#include <drivers/adi_dev.h>		// device manager includes
#include <stdlib.h>
#include <drivers\flash\util.h>				// library struct includes
#include <drivers\flash\adi_pc28f128k.h>	// flash-PC28F128 includes
#include <drivers\flash\Errors.h>			// error type includes

#include "adi_ssl_Init.h"
#include "PC28F128_common.h"

/*******************************************************************
*   Function:    TEST_BURSTFLASH
*   Description: Main test routine will get launched from the POST 
*				 framework.
*******************************************************************/
int TEST_BURSTFLASH(void)
{
	int i = 0, j = 0;
	unsigned int iPassed = 0;
	unsigned int iResult;				// result
	ADI_DEV_1D_BUFFER FlashBuff1D;		// write buffer pointer
	unsigned short sVal = 0;
	unsigned int nSector = 0;

	COMMAND_STRUCT pCmdBuffer;

	// open the PC28F128K
	iResult = adi_dev_Open(	adi_dev_ManagerHandle,			// DevMgr handle
							&ADIPC28F128KEntryPoint,			// pdd entry point
							0,								// device instance
							NULL,							// client handle callback identifier
							&DevHandlePC28F128K,				// DevMgr handle for this device
							ADI_DEV_DIRECTION_BIDIRECTIONAL,// data direction for this device
							NULL,							// handle to DmaMgr for this device
							NULL,							// handle to deferred callback service
							pc28f128_Callback);							// client's callback function
							
	if (iResult != ADI_DEV_RESULT_SUCCESS)
		return 0;
		
	// Set Dataflow method
	iResult = adi_dev_Control(DevHandlePC28F128K, ADI_DEV_CMD_SET_DATAFLOW_METHOD, (void *)ADI_DEV_MODE_CHAINED );
    if( ADI_DEV_RESULT_SUCCESS != iResult )    
    {
        return 0;
    }
    							
	// setup the external bus control and PORTs
	SetupForFlash();

	// get the manufacturer code and device code
	iResult = GetFlashInfo();
	if(ADI_DEV_RESULT_SUCCESS == iResult)
	{
		// see if the man code is what we expect
		if( AFP_ManCode != 0x89 )
			return 0;
	
		// see if the device code is what we expect
		if( AFP_DevCode != 0x891F )
			return 0;
	}
		
	// erase the last sector
	pCmdBuffer.SEraseSect.nSectorNum = FLASH_SECTOR;
	pCmdBuffer.SEraseSect.ulFlashStartAddr = FLASH_START_ADDR;

	// Erase the flash
	iResult = adi_dev_Control(DevHandlePC28F128K, CNTRL_ERASE_SECT, &pCmdBuffer );
	if(ADI_DEV_RESULT_SUCCESS == iResult)
	{
		// initialize our buffer
		FlashBuff1D.Data = &sVal;

		for( i = 0, j = 0; i < 0x4000; i+=2, j++ )
		{
			unsigned long ulOffset = (FLASH_SECTOR_OFFSET | FLASH_START_ADDR) + i ;
			FlashBuff1D.pAdditionalInfo = (void *)&ulOffset;
			FlashBuff1D.pNext = NULL;
			sVal = j;

			// write to the flash
			iResult = adi_dev_Write(DevHandlePC28F128K, ADI_DEV_1D, (ADI_DEV_BUFFER *)&FlashBuff1D);
			if(ADI_DEV_RESULT_SUCCESS == iResult)
			{
				iPassed = 1;
				sVal = 0;

				// Read back the values from sector
				// set the parameters needed to do a read

				// read from the flash
				iResult = adi_dev_Read(DevHandlePC28F128K, ADI_DEV_1D, (ADI_DEV_BUFFER *)&FlashBuff1D);
				if(ADI_DEV_RESULT_SUCCESS == iResult)
				{
					if( j != sVal )
					{
						iPassed = 0;
						break;
					}
				}
			}//end if(ADI_DEV_RESULT_SUCCESS == iResult)
		}// end for( i = 0; i < 0xFFFF; i+=2 )
	}//end if(ADI_DEV_RESULT_SUCCESS == iResult)

	adi_dev_Close( DevHandlePC28F128K );
	
	return iPassed;
}

/*******************************************************************
*   Function:    main
*   Description: used only if this test is needed as a standalone
*				 test separate from POST
*******************************************************************/
#ifdef _STANDALONE_ // use this to run standalone tests
int main(void)
{
	int iStatus;

	asm("nop;");

	while(1)
	{
		iStatus = TEST_FLASH();
		if( 0 == iStatus )
		{
			asm("emuexcpt;");
		}
	}

}
#endif //#ifdef _STANDALONE_ // use this to run standalone tests
