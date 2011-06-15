/*******************************************************************
Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software
you agree to the terms of the associated Analog Devices License Agreement.

Project Name:  	Power_On_Self_Test

Hardware:		ADSP-BF548 EZ-KIT Lite

Description:	This file programs the public key using OTP and also
				verifies that a public key exists
*******************************************************************/

/*******************************************************************
*  include files
*******************************************************************/
#include <drivers/adi_dev.h>		// device manager includes

#include "adi_otp.h"
#include "adi_ssl_Init.h"
#include "otp_helper_functions.h"


//#define CHANGE_TIME_VAL 	1		// if you wish to use a timing value other than the default, uncomment this
//#define READ_ONLY			1		// if you wish to only read the key, uncomment this

ADI_DEV_DEVICE_HANDLE DevHandleOTP;

tOTP_unique_chip_id Unique_Chip_ID;   // contains the read Unique Chip ID
tOTP_public_key New_Public_Key = {    // contains the customer public key to be programmed in OTP memory
  #include "public_key.dat"
};

u64 Public_Key_Compare[6] =
{
	0xbb1d5da08a9bc0a6,
	0x43193d001e39ce76,
	0x0000000369368af2,
	0x167f6ef7cd2e251b,
	0x41d4f1e1b005e70f,
	0x000000015f7a90c8
};

/*******************************************************************
*  function prototypes
*******************************************************************/
static void OTP_Callback(void *ClientHandle, u32 Event, void *pArg);
int PROGRAM_PUBLIC_KEY(void);
int IS_PUBLIC_KEY_LOCKED_AND_PROGRAMMED(void);

/*******************************************************************
*   Function:    OTP_Callback
*   Description: Invoked when the driver needs to notify the application
*	             of something
*******************************************************************/
static void OTP_Callback(void *ClientHandle, u32 Event, void *pArg)
{
	// just return
}


/*******************************************************************
*   Function:    PROGRAM_PUBLIC_KEY
*   Description: Main routine that will get launched from the POST
*				 framework to program the public key.
*******************************************************************/
int PROGRAM_PUBLIC_KEY(void)
{
	u32 Result = ADI_DEV_RESULT_SUCCESS;
	u32 access_mode = ADI_OTP_ACCESS_READ;
	u32 isempty;
  	u32 islocked;
  	u32 isunlocked;
  	u32 i = 0;
  	u64 store;
  	u32 compareIndex = 0;
  	bool bKeyDoesNotMatch = false;

	// open the OTP driver
	Result = adi_dev_Open(	adi_dev_ManagerHandle,			// DevMgr handle
							&ADIOTPEntryPoint,			// pdd entry point
							0,								// device instance
							NULL,							// client handle callback identifier
							&DevHandleOTP,				// DevMgr handle for this device
							ADI_DEV_DIRECTION_BIDIRECTIONAL,// data direction for this device
							NULL,							// handle to DmaMgr for this device
							NULL,							// handle to deferred callback service
							OTP_Callback);							// client's callback function

	if (Result != ADI_DEV_RESULT_SUCCESS)
		return 0;

	// timing value is set by default to 0x32149485
	// if you wish to change it please uncomment #define CHANGE_TIME_VAL
	// at the top of this file
#ifdef CHANGE_TIME_VAL
	u32 timing_value = 0x32149485;
	Result = adi_dev_Control(DevHandleOTP, ADI_OTP_CMD_SET_TIMING, &timing_value );
    if (Result != ADI_DEV_RESULT_SUCCESS)
    {
    	adi_dev_Close( DevHandleOTP );
    	return 0;
    }
#endif

    access_mode = ADI_OTP_ACCESS_READWRITE;
    Result = adi_dev_Control(DevHandleOTP, ADI_OTP_CMD_SET_ACCESS_MODE, &access_mode );

    if (Result != ADI_DEV_RESULT_SUCCESS)
    {
    	adi_dev_Close( DevHandleOTP );
    	return 0;
    }

    // Set Dataflow method
	Result = adi_dev_Control(DevHandleOTP, ADI_DEV_CMD_SET_DATAFLOW_METHOD, (void *)ADI_DEV_MODE_CHAINED );
	if(Result != ADI_DEV_RESULT_SUCCESS)
	{
		return 0;
    }

    	// Check if the Public Key is already programmed
    otp_gp_isempty (OTP_PUBLIC_KEY_PAGE, ADI_OTP_LOWER_HALF, OTP_PUBLIC_KEY_SIZE_IN_HALFPAGES, &isempty);
    if (Result != ADI_DEV_RESULT_SUCCESS)
    {
    	adi_dev_Close( DevHandleOTP );
    	return 0;
    }

    otp_gp_isunlocked (OTP_PUBLIC_KEY_PAGE, OTP_PUBLIC_KEY_SIZE_IN_PAGES, &isunlocked);
    if (Result != ADI_DEV_RESULT_SUCCESS)
    {
    	adi_dev_Close( DevHandleOTP );
    	return 0;
    }

#ifndef READ_ONLY
    if (!isempty || !isunlocked)
#else
	if ( 0 )
#endif
    {
		if(!isempty && !isunlocked)
		{
			// we are alreay programmed and locked so just return
			adi_dev_Close( DevHandleOTP );
			return 1;
		}
		else
		{
      		// the public key is either programmed or locked so
      		// put up a warning flag because that should not be the case
			adi_dev_Close( DevHandleOTP );
			return 0;
      	}
	}
    else
    {
      	// Program the new Public Key
      	// we are writing so now re-open for readwrite acces
      	access_mode = ADI_OTP_ACCESS_READWRITE;
      	Result = adi_dev_Control(DevHandleOTP, ADI_OTP_CMD_SET_ACCESS_MODE, &access_mode );
      	if (Result != ADI_DEV_RESULT_SUCCESS)
      	{
      		adi_dev_Close( DevHandleOTP );
        	return 0;
      	}

#ifndef READ_ONLY
      	// Write the new Public Key
      	OTP_ERROR_CATCH( otp_write_PublicKey (&New_Public_Key) , "otp_write_PublicKey" );
#endif

      	// read it back
      	for (i= OTP_PUBLIC_KEY_PAGE; (i< (OTP_PUBLIC_KEY_PAGE + OTP_PUBLIC_KEY_SIZE_IN_PAGES)) && !bKeyDoesNotMatch; i++)
      	{
      		if( Result == ADI_DEV_RESULT_SUCCESS )
      		{
        		Result = otp_read_page(i, ADI_OTP_LOWER_HALF, &store);

        		if( store != Public_Key_Compare[compareIndex++] )
      				bKeyDoesNotMatch = true;
      		}

        	if( (Result == ADI_DEV_RESULT_SUCCESS) && !bKeyDoesNotMatch )
        	{
        		Result = otp_read_page(i, ADI_OTP_UPPER_HALF, &store);

        		if( store != Public_Key_Compare[compareIndex++] )
      				bKeyDoesNotMatch = true;
        	}
      	}

      	// we don't want to lock the key if it did not match
      	if( bKeyDoesNotMatch )
      	{
      		adi_dev_Close( DevHandleOTP );
    		return 0;
      	}

#ifndef READ_ONLY
      	// Lock the Public Key
      	OTP_ERROR_CATCH( otp_lock_PublicKey () , "otp_lock_PublicKey" );
#endif

    }

	adi_dev_Close( DevHandleOTP );

	if( Result != ADI_DEV_RESULT_SUCCESS )
		return 0;
	else
		return 1;
}


/*******************************************************************
*   Function:    IS_PUBLIC_KEY_LOCKED_AND_PROGRAMMED
*   Description: Main routine that will get launched from the POST
*				 framework to check if the public key is programmed
*				 with the correct value and locked.
*******************************************************************/
int IS_PUBLIC_KEY_LOCKED_AND_PROGRAMMED(void)
{
	u32 Result = ADI_DEV_RESULT_SUCCESS;
	u32 access_mode = ADI_OTP_ACCESS_READ;
	u32 isempty;
  	u32 islocked;
  	u32 isunlocked;
  	u32 i = 0;
  	u64 store;
  	u32 compareIndex = 0;
  	bool bKeyDoesNotMatch = false;

	// open the OTP driver
	Result = adi_dev_Open(	adi_dev_ManagerHandle,			// DevMgr handle
							&ADIOTPEntryPoint,				// pdd entry point
							0,								// device instance
							NULL,							// client handle callback identifier
							&DevHandleOTP,					// DevMgr handle for this device
							ADI_DEV_DIRECTION_BIDIRECTIONAL,// data direction for this device
							NULL,							// handle to DmaMgr for this device
							NULL,							// handle to deferred callback service
							OTP_Callback);					// client's callback function

	if (Result != ADI_DEV_RESULT_SUCCESS)
		return 0;

	// timing value is set by default to 0x32149485
	// if you wish to change it please uncomment #define CHANGE_TIME_VAL
	// at the top of this file
#ifdef CHANGE_TIME_VAL
	u32 timing_value = 0x32149485;
	Result = adi_dev_Control(DevHandleOTP, ADI_OTP_CMD_SET_TIMING, &timing_value );

    if (Result != ADI_DEV_RESULT_SUCCESS)
    {
    	adi_dev_Close( DevHandleOTP );
    	return 0;
    }
#endif

    access_mode = ADI_OTP_ACCESS_READWRITE;
    Result = adi_dev_Control(DevHandleOTP, ADI_OTP_CMD_SET_ACCESS_MODE, &access_mode );

    if (Result != ADI_DEV_RESULT_SUCCESS)
    {
    	adi_dev_Close( DevHandleOTP );
    	return 0;
    }

    // Set Dataflow method
    Result = adi_dev_Control(DevHandleOTP, ADI_DEV_CMD_SET_DATAFLOW_METHOD, (void *)ADI_DEV_MODE_CHAINED );
    if(Result != ADI_DEV_RESULT_SUCCESS)
    {
        return 0;
    }

    // Check if the Public Key is already programmed
    otp_gp_isempty (OTP_PUBLIC_KEY_PAGE, ADI_OTP_LOWER_HALF, OTP_PUBLIC_KEY_SIZE_IN_HALFPAGES, &isempty);
    if (Result != ADI_DEV_RESULT_SUCCESS)
    {
    	adi_dev_Close( DevHandleOTP );
    	return 0;
    }

    otp_gp_isunlocked (OTP_PUBLIC_KEY_PAGE, OTP_PUBLIC_KEY_SIZE_IN_PAGES, &isunlocked);
    if (Result != ADI_DEV_RESULT_SUCCESS)
    {
    	adi_dev_Close( DevHandleOTP );
    	return 0;
    }


    if (!isempty && !isunlocked)
    {
		// read it back
      	for (i= OTP_PUBLIC_KEY_PAGE; (i< (OTP_PUBLIC_KEY_PAGE + OTP_PUBLIC_KEY_SIZE_IN_PAGES)) && !bKeyDoesNotMatch; i++)
      	{
      		if( Result == ADI_DEV_RESULT_SUCCESS )
      		{
        		Result = otp_read_page(i, ADI_OTP_LOWER_HALF, &store);
        		if( Result != ADI_DEV_RESULT_SUCCESS )
        			break;

        		if( store != Public_Key_Compare[compareIndex++] )
      				bKeyDoesNotMatch = true;
      		}

        	if( (Result == ADI_DEV_RESULT_SUCCESS) && !bKeyDoesNotMatch )
        	{
        		Result = otp_read_page(i, ADI_OTP_UPPER_HALF, &store);
        		if( Result != ADI_DEV_RESULT_SUCCESS )
        			break;

        		if( store != Public_Key_Compare[compareIndex++] )
      				bKeyDoesNotMatch = true;
        	}
      	}

      	// the key is either unlocked or not programmed
    	adi_dev_Close( DevHandleOTP );

    	if( bKeyDoesNotMatch || ( Result != ADI_DEV_RESULT_SUCCESS ) )
    		return 0;
    	else
      		return 1;
	}
    else
    {
    	// the key is either unlocked or not programmed
    	adi_dev_Close( DevHandleOTP );
      	return 0;
    }
}

/*******************************************************************
*   Function:    main
*   Description: used only if this test is needed as a standalone
*				 test separate from POST
*******************************************************************/
#ifdef _STANDALONE_ // use this to run standalone tests

static ADI_INT_HANDLER(ExceptionHandler)	// exception handler
{
	/* Set break point here to debug */
	return(ADI_INT_RESULT_PROCESSED);
}

int main(void)
{
	u32 Result;

	// initialise System Services
    adi_ssl_Init();

    // Hook exception handler
    adi_int_CECHook(3, ExceptionHandler, NULL, FALSE);

	// Set CCLK = 400 MHz, SCLK = 133 MHz
	adi_pwr_SetFreq(400000000,133333333, ADI_PWR_DF_NONE);

	Result = PROGRAM_PUBLIC_KEY();

	if( Result == ADI_DEV_RESULT_SUCCESS )
	{
		Result = IS_PUBLIC_KEY_LOCKED_AND_PROGRAMMED();

		if( Result != ADI_DEV_RESULT_SUCCESS )
			return 0;
		else
			return 1;
	}
	else
		return 1;

}
#endif //#ifdef _STANDALONE_ // use this to run standalone tests
