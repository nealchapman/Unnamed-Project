/*******************************************************************
Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software
you agree to the terms of the associated Analog Devices License Agreement.  

Project Name:  	Power_On_Self_Test

Hardware:		ADSP-BF548 EZ-KIT Lite

Description:	This file tests the SPI flash on the EZ-KIT
*******************************************************************/

/*******************************************************************
*  include files
*******************************************************************/
#include <cdefbf548.h>
#include <defbf548.h>
#include <stdlib.h>
#include <drivers\flash\util.h>			// library struct includes
#include <drivers\flash\adi_m25p16.h>	// flash-M25P16 includes
#include <drivers\flash\Errors.h>		// error type includes
#include "adi_ssl_Init.h"


#define FLASH_START_ADDR 			0x20000000
#define FLASH_SECTOR_OFFSET			0x001F0000

ADI_DEV_DEVICE_HANDLE DevHandleM25P16;

/*******************************************************************
*  function prototypes
*******************************************************************/
int TEST_SPIFLASH(void);
static void m25p16_Callback(void *ClientHandle, u32 Event, void *pArg);

/*******************************************************************
*   Function:    m25p16_Callback
*   Description: Invoked when the driver needs to notify the application
*	             of something
*******************************************************************/
static void m25p16_Callback(void *ClientHandle, u32 Event, void *pArg) 
{
	// just return
}

/*******************************************************************
*   Function:    TEST_SPIFLASH
*   Description: Main test routine will get launched from the POST 
*				 framework.
*******************************************************************/
int TEST_SPIFLASH(void)
{
	int i;
	unsigned int iPassed = 0;
	unsigned int iResult;				// result
	ADI_DEV_1D_BUFFER FlashBuff1D;		// write buffer pointer	
	unsigned short sVal = 0;
	unsigned int nSector = 0;
	int nManCode = 0;
	int nDevCode = 0;
	
	// open the M25P16
	iResult = adi_dev_Open(	adi_dev_ManagerHandle,			// DevMgr handle
							&ADIM25P16EntryPoint,			// pdd entry point
							0,								// device instance
							NULL,							// client handle callback identifier
							&DevHandleM25P16,				// DevMgr handle for this device
							ADI_DEV_DIRECTION_BIDIRECTIONAL,// data direction for this device
							NULL,							// handle to DmaMgr for this device
							NULL,							// handle to deferred callback service
							m25p16_Callback);							// client's callback function


	if (iResult != ADI_DEV_RESULT_SUCCESS)
		return 0;
	
	// Set Dataflow method
	iResult = adi_dev_Control(DevHandleM25P16, ADI_DEV_CMD_SET_DATAFLOW_METHOD, (void *)ADI_DEV_MODE_CHAINED );
    if(iResult != ADI_DEV_RESULT_SUCCESS)    
    {
        return 0;
    }
    		
	COMMAND_STRUCT pCmdBuffer;

	// set the sector number to the last sector so we don't overwrite
	// anything at the beginning
	pCmdBuffer.SEraseSect.nSectorNum = 31;
	pCmdBuffer.SEraseSect.ulFlashStartAddr = FLASH_START_ADDR;
	
	// Erase the flash
	iResult = adi_dev_Control(DevHandleM25P16, CNTRL_ERASE_SECT, &pCmdBuffer );	
	if(ADI_DEV_RESULT_SUCCESS == iResult)
	{
		// initialize our buffer
		FlashBuff1D.Data = &sVal;
	
		for( i = 0; i < 0xFFFF; i++ )
		{
			unsigned long ulOffset = (FLASH_SECTOR_OFFSET | FLASH_START_ADDR) + i;
			FlashBuff1D.pAdditionalInfo = (void *)&ulOffset;
			FlashBuff1D.pNext = NULL;
			sVal = i;
						
			// write to the flash	
			iResult = adi_dev_Write(DevHandleM25P16, ADI_DEV_1D, (ADI_DEV_BUFFER *)&FlashBuff1D);
			if(ADI_DEV_RESULT_SUCCESS == iResult)
			{
				iPassed = 1;
				sVal = 0;
								
				// Read back the values from sector 
				// set the parameters needed to do a read
				iResult = adi_dev_Read(DevHandleM25P16, ADI_DEV_1D, (ADI_DEV_BUFFER *)&FlashBuff1D);
				if(ADI_DEV_RESULT_SUCCESS == iResult)
				{
					if( (i & 0xFF) != sVal )
					{
						iPassed = 0;
						break;
					}
				}
			}//end adi_pdd_Write() -- if(ADI_DEV_RESULT_SUCCESS == iResult)
		}// end for( i = 0; i < 0xFFFF; i+=2 )
	}//end adi_pdd_Control() -- if(ADI_DEV_RESULT_SUCCESS == iResult)
		
	adi_dev_Close( DevHandleM25P16 );
		
	return iPassed;
}

