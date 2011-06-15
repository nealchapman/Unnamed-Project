/*******************************************************************
Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software
you agree to the terms of the associated Analog Devices License Agreement.  

Project Name:  	Power_On_Self_Test

Hardware:		ADSP-BF548 EZ-KIT Lite

Description:	This file tests if a MAC address is programmed and 
				also allows programming and reading of the MAC address
*******************************************************************/


/*******************************************************************
*  include files
*******************************************************************/
#include <stdio.h>	// sscanf()
#include <string.h>		//memcpy()

#include <services/services.h>		// system service includes
#include <drivers/adi_dev.h>		// device manager includes

#include <drivers\usb\usb_core\adi_usb_objects.h>
#include <drivers\usb\usb_core\adi_usb_core.h>
#include <drivers\usb\usb_core\adi_usb_ids.h>
#include <drivers\usb\class\peripheral\vendor_specific\adi\bulkadi\adi_usb_bulkadi.h>		// adi bulk driver header file
#include <hostapp.h>
#include <lan9218.h>

#include "adi_ssl_Init.h"
#include "USB_common.h"


/*******************************************************************
*  global variables and defines
*******************************************************************/
#define NUM_SUBCOMMANDS			10				// max number of sub-command supported
#define MAC_ADDRESS_LENGTH		12		// MAC address length in byte
#define PARAMETER_LENGTH		128		// buffer size for parameters

static ADI_DEV_1D_BUFFER UsbcbBuffer;	// one-dimensional buffer for processing usbcb
static ADI_DEV_1D_BUFFER DataBuffer;	// one-dimensional buffer for processing data
static USBCB usbcb;						// USB command block

// memory map address for ethernet chip
static unsigned int LAN9218Base	=	SMSC911x_PHYS_ADDR;

// these are the 2 sub commands that we are going to use
enum CustomSubCommand
{
	SET_MAC_ADDRESS,
	GET_MAC_ADDRESS,
};


/*******************************************************************
*  function prototypes
*******************************************************************/
int GetMacAddress(char *pcMacAddr);
int SetMacAddress(char *pcMacAddr);



/****************************************************************************
*   Function:    SscanDottedMAC
*   Description: Format ASCII MAC address into 6 character string.
*	Input:		 char bfr containing ASCII colonated (':') MAC address string
*				 address of where to store 48-bit wide MAC converted value
*	Output: 	 char bfr is overwritten
*				 address of 48-bit wide MAC value is written only on success
*				 SscanDottedMAC returns a value of six (6) to indicate success
******************************************************************************/
static int	SscanDottedMAC( const char *s, char *mac )
{
	int		a0,a1,a2,a3,a4,a5;
	int 	i;
	int		scanned=0;
	char * p = (char*)s;
	char mac_temp[18];

	// insert ":" into every 2 bytes
	for(i=0; i<6;i++)
	{
		strncpy(&mac_temp[i*3], p, 2);
		strcpy(&mac_temp[(i+1)*3-1],":");
		p+=2;	//increment 2 bytes

	}

	/* Scan source to a0 through a5 */
	scanned = sscanf(&mac_temp[0], "%x:%x:%x:%x:%x:%x", &a0,&a1,&a2,&a3,&a4,&a5);
	if (scanned == 6)	//	* success?  Yes--
	{
	//	* Now I shouldn't really have to test for > 255, since
	//	 * the scanf() format string states "%2x", but since it
	//	 * is really so cheap to do so, ...
	//	 *
		if ((a0|a1|a2|a3|a4|a5) > 255)
			return -1;		//* failure!

		*mac++ = (char) a0;	//* write back MAC result
		*mac++ = (char) a1;
		*mac++ = (char) a2;
		*mac++ = (char) a3;
		*mac++ = (char) a4;
		*mac++ = (char) a5;


	}

	return scanned;
}

/****************************************************************************
*   Function:    udelay
*   Description: Delay for a certain time perioid
******************************************************************************/
void udelay(unsigned int uiCount)
{
	int i;
	for(i=0; i<uiCount*10000; i++)
		asm("nop;");

}

/****************************************************************************
*   Function:    LAN9218_SetRegDW
*   Description: Set LAN register
******************************************************************************/
void LAN9218_SetRegDW(const unsigned int dwOffset, const unsigned int dwVal)
{
	(*(volatile unsigned int *)(LAN9218Base + dwOffset)) = dwVal;
}

/****************************************************************************
*   Function:    LAN9218_GetRegDW
*   Description: Get LAN register
******************************************************************************/
unsigned int LAN9218_GetRegDW( const unsigned int dwOffset)
{
	return (unsigned int)(*(volatile unsigned int *)(LAN9218Base + dwOffset));
}

/****************************************************************************
*   Function:    Lan_MacNotBusy
*   Description: Wait until MAC is not busy
******************************************************************************/
static bool Lan_MacNotBusy(void)
{
	int i=0;
	// wait for MAC not busy, w/ timeout
	for(i=0;i<40;i++)
	{
		if((LAN9218_GetRegDW(MAC_CSR_CMD) & MAC_CSR_CMD_CSR_BUSY_)==(0UL)) {
			return TRUE;
		}
	}

	return FALSE;
}

/****************************************************************************
*   Function:    Mac_GetRegDW
*   Description: This function is used to read a Mac Register.
******************************************************************************/
static int Mac_GetRegDW(const int dwOffset)
{
	int	dwRet;

	// wait until not busy, w/ timeout
	if (LAN9218_GetRegDW( MAC_CSR_CMD) & MAC_CSR_CMD_CSR_BUSY_)
	{
		return 0xFFFFFFFFUL;
	}

	// send the MAC Cmd w/ offset
	LAN9218_SetRegDW( MAC_CSR_CMD,
		((dwOffset & 0x000000FFUL) | MAC_CSR_CMD_CSR_BUSY_ | MAC_CSR_CMD_R_NOT_W_));

	// wait for the read to happen, w/ timeout
	if (!Lan_MacNotBusy())
	{
		dwRet = 0xFFFFFFFFUL;
	}
	else
	{
		// finally, return the read data
		dwRet = LAN9218_GetRegDW( MAC_CSR_DATA);
	}

	return dwRet;
}

/****************************************************************************
*   Function:    e2p_busy
*   Description: Ensure that the EPC is NOT busy.
******************************************************************************/
static int e2p_busy( void )
{
	// The main purpose here is to ensure that the EPC is NOT busy

	unsigned int	E2P_CMD_v;
	int		dwTimeOut;

	// make sure EEPROM is ready to program
	for (	E2P_CMD_v=LAN9218_GetRegDW(E2P_CMD), dwTimeOut=50;
		 	E2P_CMD_v & E2P_CMD_EPC_BUSY_ && dwTimeOut > 0;
		 	E2P_CMD_v=LAN9218_GetRegDW(E2P_CMD), dwTimeOut-- )
	{
		udelay(5);						// 50 * 5 us == 250 microsecond poll
	}

	// return E2P_CMD_EPC_BUSY_ bit-31, where 0: NOT busy, else timeout
	return E2P_CMD_v >> 31;				// success! (0), or EPC_BUSY (1)!
}

/****************************************************************************
*   Function:    e2p_eral
*   Description: erase entire EEPROM
******************************************************************************/
static int e2p_eral( void )
{
	// EEPROM erase entire EEPROM
	// Note: must first enable write [e2p_ewen];

	LAN9218_SetRegDW( E2P_CMD, E2P_CMD_EPC_BUSY_ | E2P_CMD_EPC_CMD_ERAL_ );

	return e2p_busy();
}

/****************************************************************************
*   Function:    e2p_erase
*   Description: erase single byte at Offset
******************************************************************************/
static int e2p_erase( int Offset )
{
	// EEPROM erase single byte at Offset
	// Note: must first enable write [e2p_ewen]

	LAN9218_SetRegDW( E2P_CMD, (E2P_CMD_EPC_BUSY_ | E2P_CMD_EPC_CMD_ERASE_|Offset) );

	return e2p_busy();
}

/****************************************************************************
*   Function:    e2p_ewds
*   Description: EEPROM write disable
******************************************************************************/
static int e2p_ewds( void )
{
	// EEPROM write disable
	// Note: called last after write/erase series to write-protect EEPROM

	LAN9218_SetRegDW( E2P_CMD, E2P_CMD_EPC_BUSY_ | E2P_CMD_EPC_CMD_EWDS_ );

	return e2p_busy();
}

/****************************************************************************
*   Function:    e2p_ewen
*   Description: EEPROM write enable
******************************************************************************/
static int e2p_ewen( void )
{
	// EEPROM write enable
	// Note: must be called first before writing to EEPROM

	LAN9218_SetRegDW( E2P_CMD, E2P_CMD_EPC_BUSY_ | E2P_CMD_EPC_CMD_EWEN_ );

	return e2p_busy();
}

/****************************************************************************
*   Function:    e2p_read
*   Description: EEPROM read single byte from Offset
******************************************************************************/
static char e2p_read( int Offset )
{
	char data;
	// EEPROM read single byte from Offset.  Can't return 0/non-0 because data
	// is always expected, so return a non-numeric "?"

	LAN9218_SetRegDW( E2P_CMD, (E2P_CMD_EPC_BUSY_ | E2P_CMD_EPC_CMD_READ_|Offset) );

	return (char) e2p_busy()?
		0:LAN9218_GetRegDW(E2P_DATA) & 0xff;	//success! low 8-bits

}

/****************************************************************************
*   Function:    e2p_reload
*   Description: EEPROM force a re-load of the MAC ADDRH/ADDRL registers
*				 from the EEPROM
******************************************************************************/
static int e2p_reload( void )
{
	// EEPROM force a re-load of the MAC ADDRH/ADDRL registers from the EEPROM

	LAN9218_SetRegDW( E2P_CMD, E2P_CMD_EPC_BUSY_ | E2P_CMD_EPC_CMD_RELOAD_ );
	if (e2p_busy())
		return 1;

	return (LAN9218_GetRegDW(E2P_CMD) & E2P_CMD_MAC_ADDR_LOADED_)? 0 : 1;
}

/****************************************************************************
*   Function:    e2p_wral
*   Description: EEPROM write single byte to entire EEPROM (format character)
******************************************************************************/
static int e2p_wral( char Data )
{
	// EEPROM write single byte to entire EEPROM (format character)
	// Note: must first enable write [e2p_ewen];

	if (e2p_busy())
   		return 1;
	LAN9218_SetRegDW( E2P_DATA, (unsigned int) Data );		// low 8-bits
	LAN9218_SetRegDW( E2P_CMD, E2P_CMD_EPC_BUSY_ | E2P_CMD_EPC_CMD_WRAL_ );

	return e2p_busy();
}

/****************************************************************************
*   Function:    e2p_write
*   Description: EEPROM write single byte to Offset
******************************************************************************/
static int e2p_write( int Offset, char Data )
{
	// EEPROM write single byte to Offset
	// Note: must first enable write [e2p_ewen];

	LAN9218_SetRegDW( E2P_DATA, Data );					// low 8-bits
	if (e2p_busy())
   		return 1;
	LAN9218_SetRegDW( E2P_CMD, (E2P_CMD_EPC_BUSY_ | E2P_CMD_EPC_CMD_WRITE_|Offset) );

	return e2p_busy();
}

/****************************************************************************
*   Function:    eeprom_read
*   Description: re-read the MAC address from the EEPROM
******************************************************************************/
int eeprom_read( char *mac_get )
{
	// re-read the MAC address from the EEPROM (refresh MAC address image)
	// set MAC_Address_Formatted to '1' if format is valid (location 0 == 0xa5)

	int		result=0;					// set result to success!
	int		Offset;


	result += e2p_read(0x0) == 0xa5? 0:1;
	if (result == 0)
	{
		// refresh the display with the contents of the EEPROM MAC address
		for (Offset=1; Offset < 7; Offset++)
			*mac_get++ = e2p_read(Offset);
	}
	else
	{
		// refresh the mac_get location with all "?" signifying an empty EEPROM
		for (Offset=1; Offset < 7; Offset++)
			*mac_get++ = '?';
	}

	return result;						// success! (0), or failure!
}

/****************************************************************************
*   Function:    eeprom_erase
*   Description: Erase the entire EEPROM
******************************************************************************/
int eeprom_erase( char *mac_get )
{
	// Erase the entire EEPROM
	// On success, the format byte at location should read 0xff

	int		result=0;					// set result to success!

	result += e2p_ewen();				// enable write
	if (result == 0)
	{
		result += e2p_eral();			// Erase EEPROM
	}
	result += e2p_ewds();				// disable write

	result += eeprom_read( mac_get );	// refresh MAC address from EEPROM

	return result;						// success! (0), or failure!
}

/****************************************************************************
*   Function:    eeprom_write
*   Description: Take MAC set address value and burn the EEPROM with it.
******************************************************************************/
int eeprom_write( char *mac_set, char *mac_get )
{
	// Take MAC set address value and burn the EEPROM with it.
	// Validate the change by re-loading the MAC ADDRH/ADDRL registers
	// and copying the address to the MAC get address
	char a0;
	int		result=0;						// set result to success!
	char	a1=(*mac_set++), a2=(*mac_set++), a3=(*mac_set++),
			a4=(*mac_set++), a5=(*mac_set++), a6=(*mac_set++);

	// Store the MAC address to the EEPROM and set the format (0xa5)
	result +=		e2p_ewen();				// enable write
	result +=		e2p_write(0x6, a6);		// write MAC address to EEPROM
	result +=		e2p_write(0x5, a5);
	result +=		e2p_write(0x4, a4);
	result +=		e2p_write(0x3, a3);
	result +=		e2p_write(0x2, a2);
	result +=		e2p_write(0x1, a1);
	if (result == 0)
		result +=	e2p_write(0x0, 0xa5);	// write format value
	result +=		e2p_ewds();				// disable write

	// Validate writes by reading back and comparing
	if (result == 0)
	{
		result += ((e2p_read(0x6) ==   a6)? 0:1);
		result += ((e2p_read(0x5) ==   a5)? 0:1);
		result += ((e2p_read(0x4) ==   a4)? 0:1);
		result += ((e2p_read(0x3) ==   a3)? 0:1);
		result += ((e2p_read(0x2) ==   a2)? 0:1);
		result += ((e2p_read(0x1) ==   a1)? 0:1);
		result += ((e2p_read(0x0) == (char)0xa5)? 0:1);	// format byte validates

		result += e2p_reload();				// confirm by re-loading ADDRH/ADDL
		if (result == 0)
		{
			unsigned int	ADDRH_v=Mac_GetRegDW(ADDRH), //update display
					ADDRL_v=Mac_GetRegDW(ADDRL);

			// Synchronize the packet generator src addr to the perfect filter
			a6 = (char) ((ADDRH_v>>8)  & 0xff);
			a5 = (char) ((ADDRH_v>>0)  & 0xff);
			a4 = (char) ((ADDRL_v>>24) & 0xff);
			a3 = (char) ((ADDRL_v>>16) & 0xff);
			a2 = (char) ((ADDRL_v>>8)  & 0xff);
			a1 = (char) ((ADDRL_v>>0)  & 0xff);

			// Write back the contents of the EEPROM as read from a reload
			(*mac_get++)=a1, (*mac_get++)=a2, (*mac_get++)=a3,
			(*mac_get++)=a4, (*mac_get++)=a5, (*mac_get++)=a6;
		}
	}

	return result;						// success! (0), or failure!
}


/****************************************************************************
*   Function:    SetDevicePFConfig
*   Description: Configures the PF/IMASK/IAR settings associated with the driver.
******************************************************************************/
static int  SetDevicePFConfig(void)
{

	// set port H MUX to configure PH8-PH11 as 1st Function (MUX = 00)
	// (bits 16-27 == 0) - Address signals A4-A9

	*pPORTH_FER = 0x3F00;
	*pPORTH_MUX = 0x0;
	*pPORTH_DIR_SET = 0x3F00;

    *pPORTE_FER 		= 0x0000;
    *pPORTE_MUX 		= 0x0000;
 	*pPORTE_INEN		= 0x0100;		// Enable input buffer
	*pPORTE_DIR_CLEAR = 0x0100;


	//initiate extern memory bank 1 for ethernet
   *pEBIU_AMGCTL   |= 0xE;


	 return 1;

}

/****************************************************************************
*   Function:    GetMacRegDW
*   Description: Get a MAC register at a particular offset
******************************************************************************/
unsigned int GetMacRegDW( const unsigned int dwOffset)
{
	unsigned int	dwRet;

	// wait until not busy, w/ timeout
	if (LAN9218_GetRegDW( MAC_CSR_CMD) & MAC_CSR_CMD_CSR_BUSY_)
	{
		return 0xFFFFFFFFUL;
	}

	// send the MAC Cmd w/ offset
	LAN9218_SetRegDW( MAC_CSR_CMD,
		((dwOffset & 0x000000FFUL) | MAC_CSR_CMD_CSR_BUSY_ | MAC_CSR_CMD_R_NOT_W_));

	// wait for the read to happen, w/ timeout
	if (!Lan_MacNotBusy())
	{
		dwRet = 0xFFFFFFFFUL;
	}
	else
	{
		// finally, return the read data
		dwRet = LAN9218_GetRegDW( MAC_CSR_DATA);
	}

	return dwRet;
}

/****************************************************************************
*   Function:    tohex
*   Description: Convert number NIB to a hex digit.
******************************************************************************/
int tohex(int nib)
{
	if(nib < 10)
	{
		return '0' + nib;
	}
    else
	{
        return 'a' + nib - 10;
	}
}

/****************************************************************************
*   Function:    convert_int_to_ascii
*   Description: Converts an integer value to an ascii value.
******************************************************************************/
void convert_int_to_ascii(char *from, char *to, int n)
{
	int nib;
	char ch;
	while(n--)
    {
		ch = *from++;
		nib = ((ch & 0xf0) >> 4) & 0x0f;
		*to++ = tohex(nib);
		nib = ch & 0x0f;
		*to++ = tohex(nib);
    }
	*to++ = 0;
}

/****************************************************************************
*   Function:    GetMacAddress
*   Description: Get MAC address from EEPROM.
******************************************************************************/
int GetMacAddress(char *pcMacAddr)
{
	int i;
	char		mac_addr[6]; 		//buffer for write
	char *pcTemp = pcMacAddr;
	unsigned int dwHigh16;
	unsigned int dwLow32;
	//initiate device
	SetDevicePFConfig();

	// get MAC addr
	dwHigh16 = GetMacRegDW( ADDRH);
	dwLow32 = GetMacRegDW( ADDRL);

	mac_addr[0] = dwLow32 & 0xFF;
	mac_addr[1] = dwLow32 >> 8 & 0xFF;
	mac_addr[2] = dwLow32 >> 16 & 0xFF;
	mac_addr[3] = dwLow32 >> 24 & 0xFF;
	mac_addr[4] = dwHigh16 & 0xFF;
	mac_addr[5] = dwHigh16 >> 8 & 0xFF;

	convert_int_to_ascii(mac_addr, pcMacAddr, 6);


	return 0;

}

/****************************************************************************
*   Function:    SetMacAddress
*   Description: Set MAC address to EEPROM.
******************************************************************************/
int SetMacAddress(char * pcMacAddr)
{
	int Result;			// result
	int dwTemp;
	char *DesiredAsciiMacAddress=0;
	char mac_addr[6]; 		//buffer for write
	char mac_result[]={0,0,0,0,0,0};	//buffer for read
	char *ADI_default_Mac = "00E022FE0000";

	// only the lower 2 bytes change so copy them in here
	strncpy(&ADI_default_Mac[8], pcMacAddr, 4);
	
	SetDevicePFConfig();

	//read chip id, make sure everything works
    dwTemp = LAN9218_GetRegDW(ID_REV);		// 0x118a0000

	if (SscanDottedMAC( ADI_default_Mac, &mac_addr[0]) == 6)
	{
		if (eeprom_write(&mac_addr[0], &mac_result[0]) == 0)
		{
			//make sure MAC read from EEPROM is same as we write to
			if (memcmp(&mac_addr[0], &mac_result[0], sizeof(mac_addr)) == 0)
			{
				return 0;				// success!
			}
		}
	}

	return -1;							// failure!
}

/****************************************************************************
*   Function:    CustomCommand
*   Description: Handles CUSTOM_COMMAND and in turn performs the sub commands
*				 that are specified.
******************************************************************************/
unsigned int CustomCommand( ADI_DEV_DEVICE_HANDLE devhandle, u32 u32_Data, u32 u32Count )
{
	int Result;			// result
	int i=0;
	u32 NextCount=0;
	char SubCommandParameter[NUM_SUBCOMMANDS][PARAMETER_LENGTH];	//string for sub-command


	if(NUM_SUBCOMMANDS < u32Count)
	{
		// parameters are more than buffer can hold
		// if this happens, we have to increase the buffer size
		return ADI_DEV_RESULT_NO_MEMORY;
	}
	
	for( i=0;i < u32Count; i++ )
	{
		
		// init buffer
		UsbcbBuffer.Data = &usbcb;
		UsbcbBuffer.ElementCount = sizeof(usbcb);
		UsbcbBuffer.ElementWidth = 1;
		UsbcbBuffer.CallbackParameter = NULL;
		UsbcbBuffer.pNext = NULL;
		
		// read data length first
		Result = usb_Read(devhandle, ADI_DEV_1D, (ADI_DEV_BUFFER *)&UsbcbBuffer, TRUE);
		if ( ADI_DEV_RESULT_SUCCESS != Result )
			return Result;

		// find out what the next transfer size will be
		NextCount = usbcb.u32_Count;
		
		// read actual data
		DataBuffer.Data = &SubCommandParameter[i];
		DataBuffer.ElementCount = NextCount;
		DataBuffer.ElementWidth = 1;
		DataBuffer.CallbackParameter = &SubCommandParameter[i];
		DataBuffer.pNext = NULL;


		Result = usb_Read(devhandle, ADI_DEV_1D, (ADI_DEV_BUFFER *)&DataBuffer, TRUE);
		if ( ADI_DEV_RESULT_SUCCESS != Result )
			return Result;
				
	}
	
	//check for sub-command
	if(u32_Data == GET_MAC_ADDRESS)
	{
		char mac_address[MAC_ADDRESS_LENGTH];
		//get mac address and sent it back
		GetMacAddress(&mac_address[0]);
		
		//send back next data length first
		// init buffer
		UsbcbBuffer.Data = &usbcb;
		UsbcbBuffer.ElementCount = sizeof(usbcb);
		UsbcbBuffer.ElementWidth = 1;
		UsbcbBuffer.CallbackParameter = NULL;
		UsbcbBuffer.pNext = NULL;
		
		usbcb.u32_Count = MAC_ADDRESS_LENGTH;
		usbcb.u32_Command = CUSTOM_COMMAND;

		Result = usb_Write(devhandle, ADI_DEV_1D, (ADI_DEV_BUFFER *)&UsbcbBuffer, FALSE);
		if ( ADI_DEV_RESULT_SUCCESS != Result )
			return Result;
				
		// send actual data back to host
		DataBuffer.Data = mac_address;
		DataBuffer.ElementCount = MAC_ADDRESS_LENGTH;
		DataBuffer.ElementWidth = 1;
		DataBuffer.CallbackParameter = mac_address;
			
	
		// send the host the data
		Result = usb_Write(devhandle, ADI_DEV_1D, (ADI_DEV_BUFFER *)&DataBuffer, FALSE);

		if ( ADI_DEV_RESULT_SUCCESS != Result )
			return Result;
			
		
	}
	else if(u32_Data== SET_MAC_ADDRESS)
	{
		
		
		//program it to eeprom
		Result = SetMacAddress(SubCommandParameter[0]);
		
		// init buffer
		UsbcbBuffer.Data = &usbcb;
		UsbcbBuffer.ElementCount = sizeof(usbcb);
		UsbcbBuffer.ElementWidth = 1;
		UsbcbBuffer.CallbackParameter = NULL;
		UsbcbBuffer.pNext = NULL;
		
		usbcb.u32_Count = 0x0;
		usbcb.u32_Command = CUSTOM_COMMAND;
		usbcb.u32_Data = Result;
		
		// send the host the data
		Result = usb_Write(devhandle, ADI_DEV_1D, (ADI_DEV_BUFFER *)&UsbcbBuffer, FALSE);

		if ( ADI_DEV_RESULT_SUCCESS != Result )
			return Result;
		
		
	}
	else
	{
		//unknown sub-command	
		return ADI_DEV_RESULT_NOT_SUPPORTED;
	}

		
			
	return 0;
}

/*******************************************************************
*   Function:    CHECK_FOR_MAC_ADDRESS
*   Description: Main test routine will get launched from the POST 
*				 framework.
*******************************************************************/
int CHECK_FOR_MAC_ADDRESS(void)
{
	unsigned int Result;
	int i;
	unsigned char		mac_addr[6]; 		//buffer for write
	unsigned int dwHigh16;
	unsigned int dwLow32;
	
	//initiate device
	SetDevicePFConfig();

	// get MAC addr
	dwHigh16 = GetMacRegDW( ADDRH);
	dwLow32 = GetMacRegDW( ADDRL);

	mac_addr[0] = dwLow32 & 0xFF;
	mac_addr[1] = dwLow32 >> 8 & 0xFF;
	mac_addr[2] = dwLow32 >> 16 & 0xFF;
	mac_addr[3] = dwLow32 >> 24 & 0xFF;
	mac_addr[4] = dwHigh16 & 0xFF;
	mac_addr[5] = dwHigh16 >> 8 & 0xFF;

	const unsigned char ucMAC_TEST_MASK[6] = {0x00, 0xE0, 0x22,0xFF, 0xFF,0xFF };

	// check the first 3 bytes/24 bit of the MAC address indentify the manufacturer. 
	for( i = 0; i < 3; i++ )
	{
		if( ucMAC_TEST_MASK[i] == mac_addr[i] )
		{
			Result = 1;
		}
		else
		{
			Result = 0;
			break;
		}
	}
	
	return Result;
}

/*******************************************************************
*   Function:    MAC_READ_WRITE
*   Description: Main test routine will get launched from the POST 
*				 framework.
*******************************************************************/
int MAC_READ_WRITE(void)
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

	// open the vendor specific bulk USB interface
	Result = adi_dev_Open(	adi_dev_ManagerHandle,				// DevMgr handle
							&ADI_USB_VSBulk_Entrypoint,			// pdd entry point
							0,									// device instance
							(void*)0x1,							// client handle callback identifier
							&USBDevHandle,						// DevMgr handle for this device
							ADI_DEV_DIRECTION_BIDIRECTIONAL,	// data direction for this device
							adi_dma_ManagerHandle,				// handle to DmaMgr for this device
							NULL,								// handle to deferred callback service
							USBClientCallback);					// client's callback function
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



	// configure the NET2272 mode
	Result = adi_dev_Control(USBDevHandle, ADI_DEV_CMD_SET_DATAFLOW_METHOD, (void*)ADI_DEV_MODE_CHAINED);
	if (Result != ADI_DEV_RESULT_SUCCESS)
		failure();

	// enable data flow
	Result = adi_dev_Control(USBDevHandle, ADI_DEV_CMD_SET_DATAFLOW, (void*)TRUE);
	if (Result != ADI_DEV_RESULT_SUCCESS)
		failure();

	// enable USB connection with host
	Result = adi_dev_Control( USBDevHandle, ADI_USB_CMD_ENABLE_USB, (void*)0);
	if (Result != ADI_DEV_RESULT_SUCCESS)
		failure();

	while (bKeepRunning)
	{
	    // wait until the USB is configured by the host before continuing
		while( !g_bUsbConfigured )
			;

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
			        
				case CUSTOM_COMMAND:
			        Result = CustomCommand( USBDevHandle, usbcb.u32_Data, usbcb.u32_Count );
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
