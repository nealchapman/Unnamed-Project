/*******************************************************************
Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software
you agree to the terms of the associated Analog Devices License Agreement.

Project Name:  	Power_On_Self_Test

Hardware:		ADSP-BF548 EZ-KIT Lite

Description:	This file tests if the RTC is working correctly
*******************************************************************/


/*******************************************************************
*  include files
*******************************************************************/
#include <time.h>
#include <services/services.h>		// system service includes
#include <drivers\flash\util.h>				// library struct includes
#include <drivers\flash\adi_pc28f128k.h>	// flash-PC28F128 includes
#include <drivers\flash\Errors.h>			// error type includes
#include "adi_ssl_init.h"
#include "PC28F128_common.h"


/*******************************************************************
*  function prototypes
*******************************************************************/
static void RTC_Callback(void *ClientHandle, u32 Event, void *pArg); // callback function

int ReadDateTimeFromFlash( struct tm *dt);
int WriteDateTimeToFlash( struct tm dt);


/*******************************************************************
*   Function:    RTC_Callback
*   Description: Invoked when the driver needs to notify the application
*	             of something
*******************************************************************/
static void RTC_Callback(void *ClientHandle, u32 Event, void *pArg)
{
   	// just return
}

/*******************************************************************
*   Function:    ReadDateTimeFromFlash
*   Description: Reads Data and Time info from last sector of flash
*******************************************************************/
int ReadDateTimeFromFlash( struct tm *dt)
{
	u32 Result = ADI_DEV_RESULT_SUCCESS;
	COMMAND_STRUCT pCmdBuffer;
	ADI_DEV_1D_BUFFER FlashBuff1D;		// write buffer pointer
	unsigned int iPassed = 1;
	int i = 0, j = 0;
	unsigned short usVal[6];

	// open the PC28F128K
	Result = adi_dev_Open(	adi_dev_ManagerHandle,			// DevMgr handle
							&ADIPC28F128KEntryPoint,			// pdd entry point
							0,								// device instance
							NULL,							// client handle callback identifier
							&DevHandlePC28F128K,				// DevMgr handle for this device
							ADI_DEV_DIRECTION_BIDIRECTIONAL,// data direction for this device
							NULL,							// handle to DmaMgr for this device
							NULL,							// handle to deferred callback service
							pc28f128_Callback);							// client's callback function

	if (Result != ADI_DEV_RESULT_SUCCESS)
		return 0;

	// Set Dataflow method
	Result = adi_dev_Control(DevHandlePC28F128K, ADI_DEV_CMD_SET_DATAFLOW_METHOD, (void *)ADI_DEV_MODE_CHAINED );
    if(Result != ADI_DEV_RESULT_SUCCESS)
    {
    	adi_dev_Close( DevHandlePC28F128K );
        return 0;
    }

    // setup the external bus control and PORTs
	SetupForFlash();

	// get the manufacturer code and device code
	Result = GetFlashInfo();
	if(ADI_DEV_RESULT_SUCCESS == Result)
	{
		// see if the man code is what we expect
		if( AFP_ManCode != 0x89 )
		{
			adi_dev_Close( DevHandlePC28F128K );
			return 0;
		}

		// see if the device code is what we expect
		if( AFP_DevCode != 0x891F )
		{
			adi_dev_Close( DevHandlePC28F128K );
			return 0;
		}
	}


	if(ADI_DEV_RESULT_SUCCESS == Result)
	{
		for( i = 0, j = 0; i < 12; i+=2, j++ )
		{
			unsigned long ulOffset = (FLASH_SECTOR_OFFSET | FLASH_START_ADDR) + i ;
			FlashBuff1D.pAdditionalInfo = (void *)&ulOffset;
			FlashBuff1D.pNext = NULL;
			FlashBuff1D.Data = &usVal[j];

			// write to the flash
			Result = adi_dev_Read(DevHandlePC28F128K, ADI_DEV_1D, (ADI_DEV_BUFFER *)&FlashBuff1D);
			if(ADI_DEV_RESULT_SUCCESS != Result)
			{
				iPassed = 0;
				break;
			}//end if(ADI_DEV_RESULT_SUCCESS == iResult)
		}// end for( i = 0; i < 0xFFFF; i+=2 )
	}

	// put hour, min, sec in our buffer
	dt->tm_hour = usVal[1];
	dt->tm_hour <<= 16;
	dt->tm_hour |= usVal[0];

	dt->tm_min = usVal[3];
	dt->tm_min <<= 16;
	dt->tm_min |= usVal[2];

	dt->tm_sec = usVal[5];
	dt->tm_sec <<= 16;
	dt->tm_sec |= usVal[4];

	// close the device
	adi_dev_Close( DevHandlePC28F128K );

	return iPassed;
}

/*******************************************************************
*   Function:    WriteDateTimeToFlash
*   Description: Writes Data and Time info to last sector of flash
*******************************************************************/
int WriteDateTimeToFlash( struct tm dt)
{
	u32 Result = ADI_DEV_RESULT_SUCCESS;
	COMMAND_STRUCT pCmdBuffer;
	ADI_DEV_1D_BUFFER FlashBuff1D;		// write buffer pointer
	unsigned int iPassed = 1;
	int i = 0, j = 0;
	unsigned short usVal[6];
	unsigned short usRead = 0;

	// open the PC28F128K
	Result = adi_dev_Open(	adi_dev_ManagerHandle,			// DevMgr handle
							&ADIPC28F128KEntryPoint,			// pdd entry point
							0,								// device instance
							NULL,							// client handle callback identifier
							&DevHandlePC28F128K,				// DevMgr handle for this device
							ADI_DEV_DIRECTION_BIDIRECTIONAL,// data direction for this device
							NULL,							// handle to DmaMgr for this device
							NULL,							// handle to deferred callback service
							pc28f128_Callback);							// client's callback function

	if (Result != ADI_DEV_RESULT_SUCCESS)
		return 0;

	// Set Dataflow method
	Result = adi_dev_Control(DevHandlePC28F128K, ADI_DEV_CMD_SET_DATAFLOW_METHOD, (void *)ADI_DEV_MODE_CHAINED );
    if(Result != ADI_DEV_RESULT_SUCCESS)
    {
    	adi_dev_Close( DevHandlePC28F128K );
        return 0;
    }

    // setup the external bus control and PORTs
	SetupForFlash();

	// get the manufacturer code and device code
	Result = GetFlashInfo();
	if(ADI_DEV_RESULT_SUCCESS == Result)
	{
		// see if the man code is what we expect
		if( AFP_ManCode != 0x89 )
		{
			adi_dev_Close( DevHandlePC28F128K );
			return 0;
		}

		// see if the device code is what we expect
		if( AFP_DevCode != 0x891F )
		{
			adi_dev_Close( DevHandlePC28F128K );
			return 0;
		}
	}

	// put hour, min, sec in our buffer
	usVal[0] = (short)dt.tm_hour;
	usVal[1] = (short)(dt.tm_hour >> 16);
	usVal[2] = (short)dt.tm_min;
	usVal[3] = (short)(dt.tm_min >> 16);
	usVal[4] = (short)dt.tm_sec;
	usVal[5] = (short)(dt.tm_sec >> 16);

	// erase the last sector
	pCmdBuffer.SEraseSect.nSectorNum = FLASH_SECTOR;
	pCmdBuffer.SEraseSect.ulFlashStartAddr = FLASH_START_ADDR;

	// Erase the flash
	Result = adi_dev_Control(DevHandlePC28F128K, CNTRL_ERASE_SECT, &pCmdBuffer );
	if(ADI_DEV_RESULT_SUCCESS == Result)
	{
		// write the hour, minutes, secs to flash
		for( i = 0, j = 0; i < 12; i+=2, j++ )
		{
			unsigned long ulOffset = (FLASH_SECTOR_OFFSET | FLASH_START_ADDR) + i ;
			FlashBuff1D.pAdditionalInfo = (void *)&ulOffset;
			FlashBuff1D.pNext = NULL;
			FlashBuff1D.Data = &usVal[j];

			// write to the flash
			Result = adi_dev_Write(DevHandlePC28F128K, ADI_DEV_1D, (ADI_DEV_BUFFER *)&FlashBuff1D);
			if(ADI_DEV_RESULT_SUCCESS == Result)
			{
				unsigned long ulOffset = (FLASH_SECTOR_OFFSET | FLASH_START_ADDR) + i ;
				FlashBuff1D.pAdditionalInfo = (void *)&ulOffset;
				FlashBuff1D.pNext = NULL;
				FlashBuff1D.Data = &usRead;

				iPassed = 1;
				usRead = 0;

				// Read back the values from sector
				// set the parameters needed to do a read

				// read from the flash
				Result = adi_dev_Read(DevHandlePC28F128K, ADI_DEV_1D, (ADI_DEV_BUFFER *)&FlashBuff1D);
				if(ADI_DEV_RESULT_SUCCESS == Result)
				{
					if( usVal[j] != usRead )
					{
						iPassed = 0;
						break;
					}
				}
			}//end if(ADI_DEV_RESULT_SUCCESS == iResult)
			else
			{
				iPassed = 0;
				break;
			}
		}// end for( i = 0, j = 0; i < 12; i+=2, j++ )
	}

	// close the device
	adi_dev_Close( DevHandlePC28F128K );

	// return
	return iPassed;
}

/*******************************************************************
*   Function:    SET_RTC_TIME
*   Description: Main routine that will get launched from the POST
*				 framework to check if the public key is programmed
*				 with the correct value and locked.
*******************************************************************/
int SET_RTC_TIME(void)
{
	u32 Result = ADI_DEV_RESULT_SUCCESS;
	struct tm   DateTime;

	// initialize data and time to 1am
	// on July 25th of 2007
	DateTime.tm_sec     = 00;
    DateTime.tm_min     = 00;
    DateTime.tm_hour    = 1;
    DateTime.tm_mday    = 25;
    DateTime.tm_mon     = 7;
    DateTime.tm_year    = 107;

    // write the data and time to flash first
    Result = WriteDateTimeToFlash(DateTime);
    if( Result == 0 )
    	return 0;

    // initialize the RTC manager, parameters are
   	//      parameter for adi_int_EnterCriticalRegion (always NULL for VDK and standalone systems)
	if ((Result = adi_rtc_Init(ADI_SSL_ENTER_CRITICAL)) != ADI_RTC_RESULT_SUCCESS) 
	{
    	return 0;
    }
    	
    // write data and time to RTC
    Result = adi_rtc_SetDateTime(&DateTime);
    if (Result != ADI_RTC_RESULT_SUCCESS)
    {
    	adi_rtc_Terminate();
    	return 0;
    }

    
    // terminate the flag manager
    if ((Result = adi_rtc_Terminate()) != ADI_RTC_RESULT_SUCCESS) 
    {
		return 0;
	}
	
	    
	return 1;
}

/*******************************************************************
*   Function:    VERIFY_RTC_UPDATE
*   Description: Main routine that will get launched from the POST
*				 framework to check if the public key is programmed
*				 with the correct value and locked.
*******************************************************************/
int VERIFY_RTC_UPDATE(void)
{
	u32 Result = ADI_DEV_RESULT_SUCCESS;
	struct tm   DateTimeInFlash;
	struct tm   CurrentDateTime;
	u16 usTimeStillTheSame = 0;
	u16 usTimeAllFFs = 0;

	// read the date and time that was stored in flash
    Result = ReadDateTimeFromFlash(&DateTimeInFlash);
    if( Result == 0 )
    	return 0;

    // initialize the RTC manager, parameters are
   	//      parameter for adi_int_EnterCriticalRegion (always NULL for VDK and standalone systems)
	if ((Result = adi_rtc_Init(ADI_SSL_ENTER_CRITICAL)) != ADI_RTC_RESULT_SUCCESS) 
	{
    	return 0;
    }
    
   	// now get the RTC date and time currently
    Result = adi_rtc_GetDateTime(&CurrentDateTime);
    if (Result != ADI_RTC_RESULT_SUCCESS)
    {
    	adi_rtc_Terminate();
    	return 0;
    }

    // terminate the flag manager
    if ((Result = adi_rtc_Terminate()) != ADI_RTC_RESULT_SUCCESS) 
    {
		return 0;
	}
	
    // seconds should not be less than what we set it to
    // since we set it to 0 to start
    if( CurrentDateTime.tm_sec < DateTimeInFlash.tm_sec )
    	return 0;

    // minute should not be less than what we set it to
    // since we set it to 0 to start
    if( CurrentDateTime.tm_min < DateTimeInFlash.tm_min )
    	return 0;

    // hour should not be less than what we set it to
    // since we set it to 0 to start
    if( CurrentDateTime.tm_hour < DateTimeInFlash.tm_hour )
    	return 0;

    // make sure that the value has at least changed
    if( CurrentDateTime.tm_sec == DateTimeInFlash.tm_sec )
    	usTimeStillTheSame |= 0x1;

    if( CurrentDateTime.tm_min == DateTimeInFlash.tm_min )
    	usTimeStillTheSame |= 0x2;

    if( CurrentDateTime.tm_hour == DateTimeInFlash.tm_hour )
    	usTimeStillTheSame |= 0x4;

    // if this value is 7 then the time has not updated
    if( usTimeStillTheSame == 0x7 )
    	return 0;

    // make sure that the value in flash is not all
    // FFs which means that the flash is erased
    if( DateTimeInFlash.tm_sec == 0xFFFFFFFF )
    	usTimeAllFFs |= 0x1;

    if( DateTimeInFlash.tm_min == 0xFFFFFFFF )
    	usTimeAllFFs |= 0x2;

    if( DateTimeInFlash.tm_hour == 0xFFFFFFFF )
    	usTimeAllFFs |= 0x4;

    // if this value is 7 then the time
    // was all FFs so something went wrong
    if( usTimeAllFFs == 0x7 )
    	return 0;

	return 1;
}





#ifdef _STANDALONE_

#define VERIFY_RTC	1

/*********************************************************************
	Function:		ExceptionHandler
					HWErrorHandler
	Description:	We should never get an exception or hardware error,
					but just in case we'll catch them and simply turn
					on all the LEDS should one ever occur.
*********************************************************************/
static ADI_INT_HANDLER(ExceptionHandler)	// exception handler
{
		return(ADI_INT_RESULT_PROCESSED);
}


static ADI_INT_HANDLER(HWErrorHandler)		// hardware error handler
{
		return(ADI_INT_RESULT_PROCESSED);
}

/*******************************************************************
*   Function:    main
*   Description: used only if this test is needed as a standalone
*				 test separate from POST
*******************************************************************/
int main(void) {

	u32     i;              // counter
	u32	    Result;         // return code

	// initialize the system services
	Result = adi_ssl_Init();
	if( Result != ADI_DEV_RESULT_SUCCESS )
		return 0;

    // hook exception and hardware errors
    if ((Result = adi_int_CECHook(3, ExceptionHandler, (void *)0, FALSE)) != ADI_INT_RESULT_SUCCESS)
    {
		return 0;
	}
    if ((Result = adi_int_CECHook(5, HWErrorHandler, (void *)0, FALSE)) != ADI_INT_RESULT_SUCCESS)
    {
		return 0;
	}

#ifndef VERIFY_RTC
	Result = SET_RTC_TIME();
	if( Result != 1 )
		return 0;
#else
	// power should be cycled to the board before doing this
	// if the time was already set.
	Result = VERIFY_RTC_UPDATE();
	if( Result != 1 )
		return 0;
#endif

    Result = adi_ssl_Terminate();
    if( Result != ADI_DEV_RESULT_SUCCESS )
		return 0;

	return 1;
}
#endif // _STANDALONE_






