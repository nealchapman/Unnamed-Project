/*******************************************************************
Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software
you agree to the terms of the associated Analog Devices License Agreement.  

Project Name:  	Power_On_Self_Test

Hardware:		ADSP-BF548 EZ-KIT Lite

Description:	This file tests the HDD on the EZ-KIT.
*******************************************************************/

/*******************************************************************
*  include files
*******************************************************************/
#include <drivers/pid/atapi/adi_atapi.h>
#include "adi_ssl_init.h"
#include <stdio.h>

/*******************************************************************
*  global variables and defines
*******************************************************************/

/* the sector number we are going to access for the test */
#define _SECTOR_NUMBER_   2

/*******************************************************************
* command table for the EZATA driver
*******************************************************************/
static ADI_DEV_CMD_VALUE_PAIR HDD_ConfigurationTable [] = {	
		{ ADI_DEV_CMD_SET_DATAFLOW_METHOD,  (void *)ADI_DEV_MODE_CHAINED },
		{ ADI_PID_CMD_SET_NUMBER_DEVICES,   (void *)1                  	 },
		{ ADI_DEV_CMD_END,                  NULL                         },
};

/*******************************************************************
* definition structure for the EZ-LAN ATA driver
*******************************************************************/
static ADI_FSS_DEVICE_DEF HardDiskDef = {
	0,										/* Not Applicable                           */
	&ADI_ATAPI_EntryPoint,		            /* Entry Points for Driver                  */
	HDD_ConfigurationTable,	                /* Command Table to configure EZ-LAN Driver */
	NULL,									/* Critical region data                     */
	ADI_DEV_DIRECTION_BIDIRECTIONAL,		/* Direction (RW media)                     */
	NULL									/* Device Handle                            */
};



ADI_FSS_VOLUME_DEF *pVolumeDef = NULL;
u32 VolumeDetectComplete;
u16 SectorDataOut[0x4000],SectorDataIn[0x4000], i;


/*******************************************************************
*  function prototypes
*******************************************************************/
u32 adi_atapi_POST(void);
static void PIDCallback(void *hArg, u32 event, void *pArg);
static u32 InitPID( ADI_FSS_DEVICE_DEF *pDevice,  ADI_DCB_CALLBACK_FN Callback );
int TEST_HDD(void);

/*******************************************************************
*   Function:    adi_atapi_POST
*   Description: does the bulk of the testing of the atapi interface
*******************************************************************/
u32 adi_atapi_POST(void)
{
	ADI_FSS_VOLUME_DEF VolumeDef;
	u32 sn, FirstUsableSector, LastUsableSector;
	u32 result;

	/* initialize ATAP PID driver */
    InitPID( &HardDiskDef,  PIDCallback );
    
    /* Activate the Device */
	adi_dev_Control( HardDiskDef.DeviceHandle, ADI_PID_CMD_MEDIA_ACTIVATE,       (void *)TRUE );
	
	/* This will result in IDENTIFY being sent */
	VolumeDetectComplete = FALSE;
    adi_dev_Control( HardDiskDef.DeviceHandle, ADI_PID_CMD_POLL_MEDIA_CHANGE,    (void*)NULL  );
    
    /* wait for volume detection to complete */
    while ( !VolumeDetectComplete );
    
    result = adi_dev_Control( HardDiskDef.DeviceHandle, ADI_ATAPI_CMD_RUN_POST,  (void *)0 );
    
    adi_dev_Close( HardDiskDef.DeviceHandle );
   
    return (!result);
}

/*******************************************************************
*   Function:    InitPID
*   Description: Initialize and configure the device driver
*******************************************************************/
static u32 InitPID( ADI_FSS_DEVICE_DEF *pDevice,  ADI_DCB_CALLBACK_FN Callback )
{
	u32 result;
	/* Open the Device Driver */
	result = adi_dev_Open(
						adi_dev_ManagerHandle,
						pDevice->pEntryPoint,
						0,
						pDevice,
						&pDevice->DeviceHandle,
						ADI_DEV_DIRECTION_BIDIRECTIONAL,
						adi_dma_ManagerHandle,
						NULL,
						Callback
						);

	if (result) 
		return result;

	/* Configure the Device Driver */
	if (pDevice->pConfigTable) {
		ADI_DEV_CMD_VALUE_PAIR *pair = &pDevice->pConfigTable[0];
		while (pair->CommandID!=ADI_DEV_CMD_END)
		{
			result = adi_dev_Control(
							pDevice->DeviceHandle,
							pair->CommandID,
							(void *)pair->Value
							);
							
			if (result) 
				return result;
		
			pair++;
		}
	}
	if (result) 
		return result;
		
	return ADI_DEV_RESULT_SUCCESS;

}

/*******************************************************************
*   Function:    PIDCallback
*   Description: Callback function for ATAPI PID events
*******************************************************************/
static void PIDCallback(void *pHandle, u32 Event, void *pArg)
{
    u32 Result;
    ADI_FSS_SUPER_BUFFER *pSuperBuffer = (ADI_FSS_SUPER_BUFFER *)pArg;
    
    /* CASEOF ( Event flag ) */
    switch (Event)
    {
        /* CASE (Device Interrupt Event) */
        case (ADI_PID_EVENT_DEVICE_INTERRUPT):
        /* CASE (Transfer Completion Event) */
        case (ADI_DEV_EVENT_BUFFER_PROCESSED):
            /* Call PID callback function */
            (pSuperBuffer->PIDCallbackFunction)( pSuperBuffer->PIDCallbackHandle, Event, pArg );
            break;

            
    /* The following are greatly simplified, as we do not need to read the MBR for 
     * the ATAPI POST.
    */
    
    /* CASE (Media Insertion event) */
    case (ADI_FSS_EVENT_MEDIA_INSERTED):
        /* normally issue command to detect volumes */
		*((u32 *)pArg) = ADI_FSS_RESULT_SUCCESS;
		
    /* CASE (Media removal event) */
    case (ADI_FSS_EVENT_MEDIA_REMOVED):
        /* normally issue command to detect volumes */
  	
    /* CASE (Media removal event) */
    case (ADI_FSS_EVENT_VOLUME_DETECTED):
        /* but we will drop through to here to tell our POST function we are ready to go */
        VolumeDetectComplete = TRUE;
        break;
    
    /* END CASEOF */
    }
}

/*******************************************************************
*   Function:    TEST_HDD
*   Description: Main test routine will get launched from the POST 
*				 framework.
*******************************************************************/
int TEST_HDD(void)
{     
    if ( adi_atapi_POST() )
    {
    	return 1;
    }
    else
    {
    	return 0;
    }
}

/*******************************************************************
*   Function:    main
*   Description: used only if this test is needed as a standalone
*				 test separate from POST
*******************************************************************/
#ifdef _STANDALONE_ 
int main(void)
{
	int bPassed = 1;
	int count = 0;

	bPassed = TEST_HARDDRIVE();
	
	return 0;
}
#endif //#ifdef _STANDALONE_
