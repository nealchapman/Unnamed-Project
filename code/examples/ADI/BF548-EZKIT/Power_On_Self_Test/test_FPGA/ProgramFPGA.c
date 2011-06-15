/*******************************************************************
Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software
you agree to the terms of the associated Analog Devices License Agreement.  

Project Name:  	Power_On_Self_Test

Hardware:		ADSP-BF548 EZ-KIT Lite

Description:	This file programs the FPGA on the FPGA EZ-Extender
				board
*******************************************************************/

/*******************************************************************
*  include files
*******************************************************************/
#include <cdefBF548.h>
#include <ccblkfn.h>
#include <stdlib.h>				// malloc includes
#include <drivers\flash\util.h>			// EZ-Kit includes
#include <drivers\fpga\adi_spartan3.h>

#include "adi_ssl_Init.h"

ADI_DEV_DEVICE_HANDLE DevHandleSpartan3;

section ("fpga")
#include "fpga_image.h"


/*******************************************************************
*  function prototypes
*******************************************************************/
void SetupDevice(void);
int WriteData( unsigned long ulStart, long lCount, long lStride, int *pnData );
static void spartan3_Callback(void *ClientHandle, u32 Event, void *pArg);
int PROGRAM_FPGA(void);

/*******************************************************************
*   Function:    spartan3_Callback
*   Description: Invoked when the driver needs to notify the application
*	             of something
*******************************************************************/
static void spartan3_Callback(void *ClientHandle, u32 Event, void *pArg) 
{
	// just return
}


/*******************************************************************
*   Function:    PROGRAM_FPGA
*   Description: Main routine that will get launched from the POST 
*				 framework.
*******************************************************************/
int PROGRAM_FPGA(void)
{
		
	int nOffset = 0;
	int nCount = 0x625F8;
	int nStride = 1;
	int i = 0;
	int index = 0;
	unsigned int iResult;

	iResult = adi_dev_Open(	adi_dev_ManagerHandle,			// DevMgr handle
								&ADISPARTAN3EntryPoint,			// pdd entry point
								0,								// device instance
								NULL,							// client handle callback identifier
								&DevHandleSpartan3,				// DevMgr handle for this device
								ADI_DEV_DIRECTION_BIDIRECTIONAL,// data direction for this device
								NULL,							// handle to DmaMgr for this device
								NULL,							// handle to deferred callback service
								spartan3_Callback);							// client's callback function

	if (iResult != ADI_DEV_RESULT_SUCCESS)
		return 0;
			
	// setup the fpga so the DSP can access it
	SetupDevice();
	
	iResult = WriteData( nOffset, sizeof(fpga_image), nStride, (int *)fpga_image );
	
	adi_dev_Close( DevHandleSpartan3 );
	
	return iResult;
}

/*******************************************************************
*   Function:    SetupDevice
*   Description: This function performs the necessary setup for the
*                processor to talk to the device such as external 
*                memory interface registers, etc
*******************************************************************/
void SetupDevice(void)
{
	*pEBIU_AMGCTL = 0x000F;
}

/*******************************************************************
*   Function:    WriteData
*   Description: This function writes a buffer to fpga
*******************************************************************/
int WriteData( unsigned long ulStart, long lCount, long lStride, int *pnData )
{
	int iResult = 0;
	ADI_DEV_1D_BUFFER		WriteBuff;	// buffer pointer


	// Set Dataflow method
	iResult = adi_dev_Control(DevHandleSpartan3, ADI_DEV_CMD_SET_DATAFLOW_METHOD, (void *)ADI_DEV_MODE_CHAINED );
    if(iResult != ADI_DEV_RESULT_SUCCESS)    
    {
        return 0;
    }
    
	WriteBuff.ElementCount = lCount;
	WriteBuff.Data 	= (void *)pnData;
	WriteBuff.pNext = NULL;
	WriteBuff.pAdditionalInfo 	= (void *)&ulStart;

	iResult = (int) adi_dev_Write( DevHandleSpartan3, ADI_DEV_1D, (ADI_DEV_BUFFER *)&WriteBuff);

	// return the appropriate error code
	return iResult;
}
