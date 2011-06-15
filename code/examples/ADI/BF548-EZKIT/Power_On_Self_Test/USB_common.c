/*******************************************************************
Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software
you agree to the terms of the associated Analog Devices License Agreement.  

Project Name:  	Power_On_Self_Test

Hardware:		ADSP-BF548 EZ-KIT Lite

Description:	This file tests the USB interface on the EZ-KIT
*******************************************************************/


/*******************************************************************
*  include files
*******************************************************************/
#include <services/services.h>		// system service includes
#include <drivers/adi_dev.h>		// device manager includes
#include <stdio.h>

#include <drivers\usb\usb_core\adi_usb_objects.h>
#include <drivers\usb\usb_core\adi_usb_core.h>
#include <drivers\usb\usb_core\adi_usb_ids.h>

#include <drivers\usb\class\peripheral\vendor_specific\adi\bulkadi\adi_usb_bulkadi.h>		// adi bulk driver header file
#include <hostapp.h>

#include "adi_ssl_Init.h"


/*******************************************************************
*  global variables and defines
*******************************************************************/
static ADI_DEV_1D_BUFFER UsbcbBuffer;	// one-dimensional buffer for processing usbcb
static ADI_DEV_1D_BUFFER DataBuffer;	// one-dimensional buffer for processing data
static USBCB usbcb;						// USB command block
static s32 g_ReadEpID;					// read EP ID
static s32 g_WriteEpID;					// write EP ID

#pragma align 4
section ("sdram0")
char USBLoopbackBuffer[MAX_DATA_BYTES_BULKADI];	// loopback buffer in SDRAM


// firmware version string info
#pragma align 4
char FwVersionInfo[NUM_VERSION_STRINGS][MAX_VERSION_STRING_LEN] = {
																	__DATE__,		// build date
																	__TIME__,		// build time
																	"03.00.00",		// version number
#ifdef __ADSPBF533__
																	"ADSP-BF533",	// target processor
#elif defined(__ADSPBF537__)
																	"ADSP-BF537",	// target processor
#elif defined(__ADSPBF561__)
																	"ADSP-BF561",	// target processor
#elif defined(__ADSPBF548__)
																	"ADSP-BF548",	// target processor
#else
#error *** Processor not supported ***
#endif
																	"loopback"};	// application name

volatile bool g_bRxFlag = FALSE;			// rx flag
volatile bool g_bTxFlag = FALSE;			// tx flag
volatile bool g_bUsbConfigured = FALSE;		// USB device configured flag

ADI_DEV_DEVICE_HANDLE USBDevHandle;										// device handle
ADI_DEV_BUFFER usbBuffers[10];

volatile bool g_bOTGEnumComplete = FALSE;

volatile u16 g_IdVendor = 0;
volatile u16 g_IdProduct = 0;

/*******************************************************************
*  function prototypes
*******************************************************************/
int failure(void);
void USBClientCallback( void *AppHandle, u32 Event, void *pArg);
u32 usb_Read(	ADI_DEV_DEVICE_HANDLE DeviceHandle, ADI_DEV_BUFFER_TYPE BufferType,
				ADI_DEV_BUFFER *pBuffer, bool bWaitForCompletion);
u32 usb_Write(	ADI_DEV_DEVICE_HANDLE DeviceHandle, ADI_DEV_BUFFER_TYPE BufferType,
				ADI_DEV_BUFFER *pBuffer, bool bWaitForCompletion);
unsigned int QuerySupport( ADI_DEV_DEVICE_HANDLE devhandle, u32 u32Command );
unsigned int Loopback(ADI_DEV_DEVICE_HANDLE devhandle, u32 u32Count);
unsigned int ReadMemory( ADI_DEV_DEVICE_HANDLE devhandle, u8 *p8Address, u32 u32Count );
unsigned int WriteMemory( ADI_DEV_DEVICE_HANDLE devhandle, u8 *p8Address, u32 u32Count );


/*********************************************************************
*	Function:		failure
*	Description:	In case of failure we toggle LEDs.
*********************************************************************/
int failure(void)
{
	return 0;
}


/*********************************************************************
*	Function:		USBClientCallback
*	Description:	Each type of callback event has it's own unique ID
*					so we can use a single callback function for all
*					callback events.  The switch statement tells us
*					which event has occurred.
*
*					Note that in the device driver model, in order to
*					generate a callback for buffer completion, the
*					CallbackParameter of the buffer must be set to a non-NULL
*					value.  That non-NULL value is then passed to the
*					callback function as the pArg parameter.
*********************************************************************/
static char string_[255];
void USBClientCallback( void *AppHandle, u32 Event, void *pArg)
{
	static unsigned int TxCtr = 0;				// number of buffers processed
	static unsigned int RxCtr = 0;				// number of buffers processed
	static unsigned int SetConfigCtr = 0;		// number of SET_CONFIGs
	static unsigned int RootPortResetCtr = 0;	// number of root port resets
	static unsigned int VbusFalseCtr = 0;		// number of VBUS false detections

	// note, AppHandle will be 0x2272 which was passed into the adi_dev_open() call
	// to indicate the NET2272 driver is making the callback

	switch (Event)
	{
		case ADI_USB_EVENT_DATA_TX:			// data was transmitted

			// pArg holds the address of the data buffer processed

			// set flag and increment counter
			g_bTxFlag = TRUE;
			TxCtr++;
			break;

		case ADI_USB_EVENT_DATA_RX:			// data was received

			// pArg holds the address of the data buffer processed

			// set flag and increment counter
			g_bRxFlag = TRUE;
			RxCtr++;
			break;

		case ADI_USB_EVENT_SET_CONFIG:		// host called set configuration

			// pArg holds the configuration number

			// set flag and increment counter
			if (0x1 == (u32)pArg)
				g_bUsbConfigured = TRUE;
			else
				g_bUsbConfigured = FALSE;

			SetConfigCtr++;

			break;

		case ADI_USB_EVENT_ROOT_PORT_RESET: // reset signaling detected

			// pArg is NULL for this event

			// set flag and increment counter
			g_bUsbConfigured = FALSE;
			RootPortResetCtr++;

			break;

		case ADI_USB_EVENT_VBUS_FALSE: 		// cable unplugged

			// pArg is NULL for this event

			// set flag and increment counter
			g_bUsbConfigured = FALSE;
			VbusFalseCtr++;

			break;

		case ADI_USB_EVENT_RESUME: 			// device resumed

			// pArg is NULL for this event
			break;

		case ADI_USB_EVENT_SUSPEND: 		// device suspended

			// pArg is NULL for this event
			break;
			
		case  ADI_USB_OTG_EVENT_ENUMERATION_COMPLETE:	
		{
			PDEVICE_OBJECT pDevO = (PDEVICE_OBJECT)pArg;
			PDEVICE_DESCRIPTOR pDevD = pDevO->pDeviceDesc;
	
			g_IdVendor = pDevD->wIdVendor;
			g_IdProduct = pDevD->wIdProduct;
			
			adi_usb_otg_SetConfiguration(1);
			
			// set our flag that enumeration is done
            g_bOTGEnumComplete = TRUE;
            
			break;
		}
	}

	// return
}


/*********************************************************************
 *
 * Function: ConfigureEndpoints
 *
 **********************************************************************/
unsigned int ConfigureEndpoints(void)
{
	ADI_ENUM_ENDPOINT_INFO EnumEpInfo;              /* Binded Endpoint information */
	static ADI_USB_APP_EP_INFO g_UsbAppEpInfo[2] = {0};
	unsigned int Result;

	/* Get Read and Write Endpoints */
	EnumEpInfo.pUsbAppEpInfo = &g_UsbAppEpInfo[0];
	EnumEpInfo.dwEpTotalEntries = sizeof(g_UsbAppEpInfo)/sizeof(ADI_USB_APP_EP_INFO);
	EnumEpInfo.dwEpProcessedEntries = 0;

	/* Get the enumerated endpoints */
	Result = adi_dev_Control( USBDevHandle, ADI_USB_CMD_CLASS_ENUMERATE_ENDPOINTS, (void*)&EnumEpInfo);

	if (Result != ADI_DEV_RESULT_SUCCESS)
		failure();

	if(g_UsbAppEpInfo[0].eDir == USB_EP_IN)
	{
		g_WriteEpID = g_UsbAppEpInfo[0].dwEndpointID;
		g_ReadEpID  = g_UsbAppEpInfo[1].dwEndpointID;
	}
	else
	{
		g_ReadEpID = g_UsbAppEpInfo[0].dwEndpointID;
		g_WriteEpID = g_UsbAppEpInfo[1].dwEndpointID;
	}

	return(Result);
}

/*********************************************************************
*	Function:		usb_Read
*	Description:	Wrapper for adi_dev_Read() which lets the user specify if
*					they want to wait for completion or not.
*	Arguments:		ADI_DEV_DEVICE_HANDLE devhandle -	device handle
*					ADI_DEV_BUFFER_TYPE BufferType -	buffer type
*					ADI_DEV_BUFFER *pBuffer -			pointer to buffer
*					bool bWaitForCompletion -			completion flag
*	Return value:	u32 - return status
*********************************************************************/
u32 usb_Read(	ADI_DEV_DEVICE_HANDLE 	DeviceHandle,			// device handle
				ADI_DEV_BUFFER_TYPE		BufferType,				// buffer type
				ADI_DEV_BUFFER 			*pBuffer,				// pointer to buffer
				bool					bWaitForCompletion)		// completion flag
{
    u32 Result = ADI_DEV_RESULT_SUCCESS;
    ADI_DEV_1D_BUFFER			*p1DBuff;

    p1DBuff = (ADI_DEV_1D_BUFFER*)pBuffer;

    /* Place the Read Endpoint ID */
    p1DBuff->Reserved[BUFFER_RSVD_EP_ADDRESS] = g_ReadEpID;

 	// if the user wants to wait until the operation is complete
    if (bWaitForCompletion)
    {
        // clear the rx flag and call read
   		g_bRxFlag = FALSE;
		Result = adi_dev_Read(DeviceHandle, BufferType, pBuffer);

		// wait for the rx flag to be set
		while (!g_bRxFlag)
		{
		    // make sure we are still configured, if not we should fail
			if ( !g_bUsbConfigured )
				return ADI_DEV_RESULT_FAILED;
		}

		return Result;
    }

    // else they do not want to wait for completion
    else
    {
        return (adi_dev_Read(DeviceHandle, BufferType, pBuffer));
    }
}


/*********************************************************************
*	Function:		usb_Write
*	Description:	Wrapper for adi_dev_Write() which lets the user specify if
*					they want to wait for completion or not.
*	Arguments:		ADI_DEV_DEVICE_HANDLE devhandle -	device handle
*					ADI_DEV_BUFFER_TYPE BufferType -	buffer type
*					ADI_DEV_BUFFER *pBuffer -			pointer to buffer
*					bool bWaitForCompletion -			completion flag
*	Return value:	u32 - return status
*********************************************************************/
u32 usb_Write(	ADI_DEV_DEVICE_HANDLE 	DeviceHandle,			// DM handle
				ADI_DEV_BUFFER_TYPE		BufferType,				// buffer type
				ADI_DEV_BUFFER			*pBuffer,				// pointer to buffer
				bool					bWaitForCompletion)		// completion flag
{
    u32 Result = ADI_DEV_RESULT_SUCCESS;
    ADI_DEV_1D_BUFFER			*p1DBuff;
    
    p1DBuff = (ADI_DEV_1D_BUFFER*)pBuffer;

    /* Place the Endpoint ID */
    p1DBuff->Reserved[BUFFER_RSVD_EP_ADDRESS] = g_WriteEpID;

 	// if the user wants to wait until the operation is complete
    if (bWaitForCompletion)
    {
        // clear the tx flag and call read
   		g_bTxFlag = FALSE;
		Result = adi_dev_Write(DeviceHandle, BufferType, pBuffer);

		// wait for the tx flag to be set
		while (!g_bTxFlag)
		{
		   // make sure we are still configured, if not we should fail
			if ( !g_bUsbConfigured )
				return ADI_DEV_RESULT_FAILED;
		}

		return Result;
    }

    // else they do not want to wait for completion
    else
    {
        return (adi_dev_Write(DeviceHandle, BufferType, pBuffer));
    }
}

/*********************************************************************
*	Function:		Loopback
*	Description:	Performs a loopback by receiving data from the host 
*					and sending it back.
*	Arguments:		ADI_DEV_DEVICE_HANDLE devhandle -	device handle
					u32 u32Count -	number of bytes to loop first time
*	Return value:	unsigned int - return status
*********************************************************************/
unsigned int Loopback(ADI_DEV_DEVICE_HANDLE devhandle, u32 u32Count)
{
	unsigned int Result;
	u32 KeepRunning = 0x1;				// keep running loopback flag
	u32 NextCount = 0;					// count for next transfer

	DataBuffer.Data = USBLoopbackBuffer;
	DataBuffer.ElementCount = u32Count;
	DataBuffer.ElementWidth = 1;
	DataBuffer.CallbackParameter = USBLoopbackBuffer;
	DataBuffer.pNext = NULL;

	while (KeepRunning)
	{
		// wait for data from the host (pass in TRUE)
		Result = usb_Read(devhandle, ADI_DEV_1D, (ADI_DEV_BUFFER *)&DataBuffer, TRUE);

		if ( ADI_DEV_RESULT_SUCCESS != Result )
			return Result;

		// process received data here...for loopback we just check the header info

		// send the host the data
		Result = usb_Write(devhandle, ADI_DEV_1D, (ADI_DEV_BUFFER *)&DataBuffer, FALSE);

		if ( ADI_DEV_RESULT_SUCCESS != Result )
			return Result;

		// find out what the next transfer size will be, mask off the upper byte (stop flag)
		NextCount = *(u32*)(DataBuffer.Data);
		NextCount &= 0x00ffffff;

		// upper byte indicates if this is our last transfer (host wants to end loopback)
		KeepRunning = *(u32*)(DataBuffer.Data);
		KeepRunning &= 0xff000000;

		// update count for next loop
		DataBuffer.ElementCount = NextCount;
	}

	return Result;
}

/*********************************************************************
*	Function:		ReadMemory
*	Description:	Reads data from Blackfin memory and sends it back 
*					to the host.
*	Arguments:		ADI_DEV_DEVICE_HANDLE dh -	device handle
*					u8 *p8Address -	Blackfin address to read from
*					u32 u32Count -	number of bytes to read
*	Return value:	unsigned int - return status
*********************************************************************/
unsigned int ReadMemory( ADI_DEV_DEVICE_HANDLE devhandle, u8 *p8Address, u32 u32Count )
{
	DataBuffer.Data = p8Address;
	DataBuffer.ElementCount = u32Count;
	DataBuffer.ElementWidth = 1;
	DataBuffer.CallbackParameter = NULL;
	DataBuffer.ProcessedFlag = FALSE;
	DataBuffer.ProcessedElementCount = 0;
	DataBuffer.pNext = NULL;

	// send the data back to the host
	return usb_Write(devhandle, ADI_DEV_1D, (ADI_DEV_BUFFER *)&DataBuffer, FALSE);
}

/*********************************************************************
*	Function:		WriteMemory
*	Description:	Receives data from the host and writes it to 
*					Blackfin memory.
*	Arguments:		ADI_DEV_DEVICE_HANDLE dh -	device handle
*					u8 *p8Address -	Blackfin address to write to
*					u32 u32Count -	number of bytes to write
*	Return value:	unsigned int - return status
*********************************************************************/
unsigned int WriteMemory( ADI_DEV_DEVICE_HANDLE devhandle, u8 *p8Address, u32 u32Count )
{
	DataBuffer.Data = p8Address;
	DataBuffer.ElementCount = u32Count;
	DataBuffer.ElementWidth = 1;
	DataBuffer.CallbackParameter = NULL;
	DataBuffer.ProcessedFlag = FALSE;
	DataBuffer.ProcessedElementCount = 0;
	DataBuffer.pNext = NULL;

	// get the data from the host
	return usb_Read(devhandle, ADI_DEV_1D, (ADI_DEV_BUFFER *)&DataBuffer, FALSE);
}

/*********************************************************************
*	Function:		QuerySupport
*	Description:	Checks to see if a command is supported by this 
*					firmware.  The host can use this to verify that 
*					a command it wants to execute is supported ahead 
*					of time.
*	Arguments:		ADI_DEV_DEVICE_HANDLE dh -	device handle
*					u32 u32Command - command we are querying on
*	Return value:	unsigned int - return status
*********************************************************************/
unsigned int QuerySupport( ADI_DEV_DEVICE_HANDLE devhandle, u32 u32Command )
{
	int Result;			// result

	// init buffer
	UsbcbBuffer.Data = &usbcb;
	UsbcbBuffer.ElementCount = sizeof(usbcb);
	UsbcbBuffer.ElementWidth = 1;
	UsbcbBuffer.CallbackParameter = NULL;
	UsbcbBuffer.pNext = NULL;

	// check for a supported command
	if ( QUERY_SUPPORT == u32Command || GET_FW_VERSION == u32Command ||
		LOOPBACK == u32Command || MEMORY_READ == u32Command ||
		MEMORY_WRITE == u32Command  || CUSTOM_COMMAND == u32Command )
		usbcb.u32_Data = TRUE;
	else
		usbcb.u32_Data = FALSE;

	// send a USBCB to the host with the result
	usbcb.u32_Count = 0x0;
	usbcb.u32_Command = QUERY_REPLY;
	Result = usb_Write(devhandle, ADI_DEV_1D, (ADI_DEV_BUFFER *)&UsbcbBuffer, FALSE);

	// return success
	return Result;
}
