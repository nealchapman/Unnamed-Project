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

#include <drivers\usb\usb_core\adi_usb_objects.h>
#include <drivers\usb\usb_core\adi_usb_core.h>
#include <drivers\usb\usb_core\adi_usb_ids.h>

#include <drivers\usb\controller\otg\adi\bf54x\adi_usb_bf54x.h>
#include <drivers\usb\class\peripheral\vendor_specific\adi\bulkadi\adi_usb_bulkadi.h>		// adi bulk driver header file
#include <hostapp.h>

#include "adi_ssl_Init.h"
#include "USB_common.h"

/*******************************************************************
*  global variables and defines
*******************************************************************/
static ADI_DEV_1D_BUFFER UsbcbBuffer;	// one-dimensional buffer for processing usbcb
static ADI_DEV_1D_BUFFER DataBuffer;	// one-dimensional buffer for processing data
static USBCB usbcb;						// USB command block


/*******************************************************************
*   Function:    CheckDeviceDescriptor
*   Description: Compare the product id and vendor id to a list
*				 of known ids for USB memory sticks used in testing
*******************************************************************/
bool CheckDeviceDescriptor(void)
{
	bool bIdsFound = false;
	
	// these VendorIds and ProductIds are those of USB flash memory 
	// sticks that ADI uses for testing
	if( (g_IdVendor == 0x05DC) && (g_IdProduct == 0xA640) )
		bIdsFound = true;			
	else if( (g_IdVendor == 0x1516) && (g_IdProduct == 0x8628) )
		bIdsFound = true;
	else if( (g_IdVendor == 0x0EA0) && (g_IdProduct == 0x2168) )
		bIdsFound = true;
	else if( (g_IdVendor == 0x0781) && (g_IdProduct == 0x5406) )
		bIdsFound = true;
	else if( (g_IdVendor == 0x08ec) && (g_IdProduct == 0x2039) )
		bIdsFound = true;
	
	return bIdsFound;	
}

/*******************************************************************
*   Function:    TEST_USB_DEVICE
*   Description: Main test routine will get launched from the POST 
*				 framework.
*******************************************************************/
int TEST_USB_DEVICE(void)
{
	unsigned int Result;							// result
	unsigned int i;									// index
	bool bKeepRunning = TRUE;						// keep running flag
	u32	ResponseCount;								// response count
	u32 SecondaryCount;								// secondary count
	ADI_DEV_1D_BUFFER  *pBuf;						// 1D buffer ptr
	ADI_DEV_1D_BUFFER UsbcbBuffer;					// 1D buffer for processing usbcb
	USBCB usbcb;									// USB command block
	PDEVICE_DESCRIPTOR pDevDesc;					// device descriptor ptr
	ADI_DEV_PDD_HANDLE PeripheralDevHandle;         // USB controller driver handle
	

	// Preboot code incorrectly sets this
	// clear it to make sure USB will work after booting
	*pUSB_APHY_CNTRL = 0;

	// initialize the buffer
	UsbcbBuffer.Data = &usbcb;
	UsbcbBuffer.ElementCount = sizeof(usbcb);
	UsbcbBuffer.ElementWidth = 1;
	UsbcbBuffer.CallbackParameter = NULL;
	UsbcbBuffer.ProcessedFlag = FALSE;
	UsbcbBuffer.ProcessedElementCount = 0;
	UsbcbBuffer.pNext = NULL;

	/* Initialize USB Core 	*/
    adi_usb_CoreInit((void*)&Result);
    
    /* Open controller driver */
    Result = adi_dev_Open(	adi_dev_ManagerHandle, 			/* DevMgr handle */
							&ADI_USBDRC_Entrypoint,			/* pdd entry point */
							0,                          	/* device instance */
							(void*)1,						/* client handle callback identifier */
							&PeripheralDevHandle,	        /* device handle */
							ADI_DEV_DIRECTION_BIDIRECTIONAL,/* data direction for this device */
							adi_dma_ManagerHandle,			/* handle to DmaMgr for this device */
							NULL,							/* handle to deferred callback service */
							USBClientCallback);				/* client's callback function */
    if (Result != ADI_DEV_RESULT_SUCCESS)
		failure();
    
	/* Open the vendor specific Bulk USB class driver */
	Result = adi_dev_Open(	adi_dev_ManagerHandle,			/* DevMgr handle */
							&ADI_USB_VSBulk_Entrypoint,		/* pdd entry point  */
							0,								/* device instance */
							(void*)0x1,						/* client handle callback identifier */
							&USBDevHandle,					/* DevMgr handle for this device */
							ADI_DEV_DIRECTION_BIDIRECTIONAL,/* data direction for this device */
							adi_dma_ManagerHandle,			/* handle to DmaMgr for this device */
							NULL,							/* handle to deferred callback service */
							USBClientCallback);				/* client's callback function */
	if (Result != ADI_DEV_RESULT_SUCCESS)
		failure();	

	/* get a pointer to the device descriptor which is maintained by the usb core */
	pDevDesc = adi_usb_GetDeviceDescriptor();
	if (!pDevDesc)
		failure();

	/* customize the device descriptor for this device */
	pDevDesc->wIdVendor = USB_VID_ADI_TOOLS;
#if defined(__ADSPBF533__)
	pDevDesc->wIdProduct = USB_PID_BF533KIT_NET2272EXT_BULK;
#elif defined(__ADSPBF537__)
	pDevDesc->wIdProduct = USB_PID_BF537KIT_NET2272EXT_BULK;
#elif defined(__ADSPBF561__)
	pDevDesc->wIdProduct = USB_PID_BF561KIT_NET2272EXT_BULK;
#elif defined(__ADSPBF548__)
	pDevDesc->wIdProduct = USB_PID_BF548KIT_BULK;
#else
#error *** Processor not supported ***
#endif

	// give USB controller driver handle to the class drivers
	Result = adi_dev_Control(USBDevHandle, ADI_USB_CMD_CLASS_SET_CONTROLLER_HANDLE, (void*)PeripheralDevHandle);
	if (Result != ADI_DEV_RESULT_SUCCESS)
		failure();

	// configure the class driver
	Result = adi_dev_Control(USBDevHandle, ADI_USB_CMD_CLASS_CONFIGURE, (void*)0);
	if (Result != ADI_DEV_RESULT_SUCCESS)
		failure();
		
	// configure the NET2272 mode
	Result = adi_dev_Control(USBDevHandle, ADI_DEV_CMD_SET_DATAFLOW_METHOD, (void*)ADI_DEV_MODE_CHAINED);
	if (Result != ADI_DEV_RESULT_SUCCESS)
		failure();

	// enable data flow
	Result = adi_dev_Control(USBDevHandle, ADI_DEV_CMD_SET_DATAFLOW, (void*)TRUE);
	if (Result != ADI_DEV_RESULT_SUCCESS)
		failure();

	// enable USB connection with host
	Result = adi_dev_Control( PeripheralDevHandle, ADI_USB_CMD_ENABLE_USB, (void*)0);
	if (Result != ADI_DEV_RESULT_SUCCESS)
		failure();

	while (bKeepRunning)
	{    
		// wait until the USB is configured by the host before continuing
		while( !g_bUsbConfigured )
			;
		ConfigureEndpoints();

		// wait for a USB command block from the host indicating what function we should perform
		Result = usb_Read(USBDevHandle, ADI_DEV_1D, (ADI_DEV_BUFFER *)&UsbcbBuffer, TRUE);

		if ( ADI_DEV_RESULT_SUCCESS == Result )
		{
			// switch on the command we just received
			switch( usbcb.u32_Command )
		    {
				// host is asking if we support a given command
				case QUERY_SUPPORT:
					Result = QuerySupport( USBDevHandle, usbcb.u32_Data );
		    		break;

		    	// get the firmware version
			    case GET_FW_VERSION:
			    	Result = ReadMemory( USBDevHandle, (u8*)FwVersionInfo, usbcb.u32_Count );
			        break;

				// run loopback
			    case LOOPBACK:
					Result = Loopback( USBDevHandle, usbcb.u32_Count );
			        break;

				// read memory
			    case MEMORY_READ:
					Result = ReadMemory( USBDevHandle, (u8*)usbcb.u32_Data, usbcb.u32_Count );
			        break;

				// write memory
			    case MEMORY_WRITE:
					Result = WriteMemory( USBDevHandle, (u8*)usbcb.u32_Data, usbcb.u32_Count );
			        break;

				// unsupported command
			    default:
			   		failure();
			        break;
	    	}
		}
	}

	// close the device
	Result = adi_dev_Close(USBDevHandle);
	if (Result != ADI_DEV_RESULT_SUCCESS)
		failure();

	return 1;
}

/*******************************************************************
*   Function:    TEST_USB_HOST
*   Description: Main test routine will get launched from the POST 
*				 framework.
*******************************************************************/
int TEST_USB_HOST(void)
{
	unsigned int Result;							// result
	unsigned int i;									// index
	bool bKeepRunning = TRUE;						// keep running flag
	u32	ResponseCount;								// response count
	u32 SecondaryCount;								// secondary count
	ADI_DEV_1D_BUFFER  *pBuf;						// 1D buffer ptr
	ADI_DEV_1D_BUFFER UsbcbBuffer;					// 1D buffer for processing usbcb
	USBCB usbcb;									// USB command block
	PDEVICE_DESCRIPTOR pDevDesc;					// device descriptor ptr
	ADI_DEV_PDD_HANDLE PeripheralDevHandle;         // USB controller driver handle
	

	// Preboot code incorrectly sets this
	// clear it to make sure USB will work after booting
	*pUSB_APHY_CNTRL = 0;

	// initialize the buffer
	UsbcbBuffer.Data = &usbcb;
	UsbcbBuffer.ElementCount = sizeof(usbcb);
	UsbcbBuffer.ElementWidth = 1;
	UsbcbBuffer.CallbackParameter = NULL;
	UsbcbBuffer.ProcessedFlag = FALSE;
	UsbcbBuffer.ProcessedElementCount = 0;
	UsbcbBuffer.pNext = NULL;

	/* Initialize USB Core 	*/
    adi_usb_CoreInit((void*)&Result);
    
    /* Open controller driver */
    Result = adi_dev_Open(	adi_dev_ManagerHandle, 			/* DevMgr handle */
							&ADI_USBDRC_Entrypoint,			/* pdd entry point */
							0,                          	/* device instance */
							(void*)1,						/* client handle callback identifier */
							&PeripheralDevHandle,	        /* device handle */
							ADI_DEV_DIRECTION_BIDIRECTIONAL,/* data direction for this device */
							adi_dma_ManagerHandle,			/* handle to DmaMgr for this device */
							NULL,							/* handle to deferred callback service */
							USBClientCallback);				/* client's callback function */
    if (Result != ADI_DEV_RESULT_SUCCESS)
		failure();
    
	/* Open the vendor specific Bulk USB class driver */
	Result = adi_dev_Open(	adi_dev_ManagerHandle,			/* DevMgr handle */
							&ADI_USB_VSBulk_Entrypoint,		/* pdd entry point  */
							0,								/* device instance */
							(void*)0x1,						/* client handle callback identifier */
							&USBDevHandle,					/* DevMgr handle for this device */
							ADI_DEV_DIRECTION_BIDIRECTIONAL,/* data direction for this device */
							adi_dma_ManagerHandle,			/* handle to DmaMgr for this device */
							NULL,							/* handle to deferred callback service */
							USBClientCallback);				/* client's callback function */
	if (Result != ADI_DEV_RESULT_SUCCESS)
		failure();

	adi_usb_otg_SetEpZeroCallback(1,(ADI_DCB_CALLBACK_FN)USBClientCallback);
		
	// set to HOST mode
	adi_usb_SetDeviceMode(MODE_OTG_HOST);
	
	// give USB controller driver handle to the class drivers
	Result = adi_dev_Control(USBDevHandle, ADI_USB_CMD_CLASS_SET_CONTROLLER_HANDLE, (void*)PeripheralDevHandle);
	if (Result != ADI_DEV_RESULT_SUCCESS)
		failure();

	// configure the class driver
	Result = adi_dev_Control(USBDevHandle, ADI_USB_CMD_CLASS_CONFIGURE, (void*)0);
	if (Result != ADI_DEV_RESULT_SUCCESS)
		failure();
	
	// configure the NET2272 mode
	Result = adi_dev_Control(USBDevHandle, ADI_DEV_CMD_SET_DATAFLOW_METHOD, (void*)ADI_DEV_MODE_CHAINED);
	if (Result != ADI_DEV_RESULT_SUCCESS)
		failure();

	// enable data flow
	Result = adi_dev_Control(USBDevHandle, ADI_DEV_CMD_SET_DATAFLOW, (void*)TRUE);
	if (Result != ADI_DEV_RESULT_SUCCESS)
		failure();

	// enable USB connection with host
	Result = adi_dev_Control( PeripheralDevHandle, ADI_USB_CMD_ENABLE_USB, (void*)0);
	if (Result != ADI_DEV_RESULT_SUCCESS)
		failure();
	
	// wait until the USB is configured by the host before continuing
	while( !g_bOTGEnumComplete )
	{
		asm("nop;");
		asm("nop;");
		asm("nop;");
	}
	
	// see of the ids match the known ids
	bool bGotCorrectDesc = CheckDeviceDescriptor();	
	
	// close the device
	Result = adi_dev_Close(USBDevHandle);
	if (Result != ADI_DEV_RESULT_SUCCESS)
		failure();

	// if there was not a match then fail
	if( !bGotCorrectDesc )
		return 0;

	return 1;
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

	while( bPassed )
	{
		bPassed = TEST_USB();
		asm("nop;");
		asm("nop;");
		asm("nop;");
		asm("nop;");
	}


	return 0;
}
#endif //#ifdef _STANDALONE_
