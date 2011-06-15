#ifndef _USB_COMMON_H_
#define _USB_COMMON_H_

#include <services/services.h>		// system service includes
#include <drivers/adi_dev.h>		// device manager includes

extern char USBLoopbackBuffer[MAX_DATA_BYTES_BULKADI];	// loopback buffer in SDRAM


// firmware version string info
extern char FwVersionInfo[];
extern volatile bool g_bUsbConfigured;		// USB device configured flag
extern ADI_DEV_DEVICE_HANDLE USBDevHandle;										// device handle
extern ADI_DEV_BUFFER usbBuffers[10];
extern volatile bool g_bOTGEnumComplete;

extern volatile u16 g_IdVendor;
extern volatile u16 g_IdProduct;

int failure(void);
void USBClientCallback( void *AppHandle, u32 Event, void *pArg);
unsigned int ConfigureEndpoints(void);
u32 usb_Read(	ADI_DEV_DEVICE_HANDLE DeviceHandle, ADI_DEV_BUFFER_TYPE BufferType,
				ADI_DEV_BUFFER *pBuffer, bool bWaitForCompletion);
u32 usb_Write(	ADI_DEV_DEVICE_HANDLE DeviceHandle, ADI_DEV_BUFFER_TYPE BufferType,
				ADI_DEV_BUFFER *pBuffer, bool bWaitForCompletion);
unsigned int QuerySupport( ADI_DEV_DEVICE_HANDLE devhandle, u32 u32Command );
unsigned int Loopback(ADI_DEV_DEVICE_HANDLE devhandle, u32 u32Count);
unsigned int ReadMemory( ADI_DEV_DEVICE_HANDLE devhandle, u8 *p8Address, u32 u32Count );
unsigned int WriteMemory( ADI_DEV_DEVICE_HANDLE devhandle, u8 *p8Address, u32 u32Count );

#endif //#ifndef _USB_COMMON_H_
