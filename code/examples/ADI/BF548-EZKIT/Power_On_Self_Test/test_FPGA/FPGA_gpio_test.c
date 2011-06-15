/*******************************************************************
Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software
you agree to the terms of the associated Analog Devices License Agreement.  

Project Name:  	Power_On_Self_Test

Hardware:		ADSP-BF548 EZ-KIT Lite

Description:	This file tests the gpio on the FPGA EZ-Extender board.
*******************************************************************/

/*******************************************************************
*  include files
*******************************************************************/
#include <cdefBF548.h>
#include <ccblkfn.h>

#include "../post_common.h"
#include "../Timer_ISR.h"


/*******************************************************************
*  function prototypes
*******************************************************************/
void FPGA_Init_GPIO(void);
void FPGA_Disable_GPIO(void);
int TEST_FPGA_GPIO(void);


/*******************************************************************
*   Function:    FPGA_Init_GPIO
*   Description: This function onfigures the programmable flags 
*                for testing
*******************************************************************/
void FPGA_Init_GPIO(void)
{
	
    *pPORTF_FER = 0x0000;	// set PORTF to GPIO mode
    *pPORTD_FER = 0x0000;   // set PORTD to GPIO mode
    
    // Set the GPIO direction: 0 = input, 1 = output
    // PORTF: we will only test PF0-7 as the rest are tested in the SPORT test
    *pPORTF_DIR_SET		= 0x000F;	// PG7-4 as input, PG3-0 as output
	*pPORTF_INEN     	= 0x00F0;   // enable PG7-4 as inputs
	*pPORTF_CLEAR		= 0xFFFF;	// clear the port
	
	// PORTD: we will only test PD4-9, The other are used by Ethernet on the EZ-KIT
	*pPORTD_DIR_SET		= 0x00B0;	// PD 4,5,7 as output
	*pPORTD_INEN        = 0x0340;   // enable PD 6,9,8 as inputs
	*pPORTD_CLEAR		= 0xFFFF;	// clear the port
	ssync();		
}


/*******************************************************************
*   Function:    FPGA_Disable_GPIO
*   Description: This function disables the GPIO programmable flags
*******************************************************************/
void FPGA_Disable_GPIO(void)
{
	*pPORTF_DIR_SET		= 0x0000;
	*pPORTF_INEN      	= 0x0000;
	*pPORTF_CLEAR      	= 0xFFFF;
	*pPORTD_DIR_SET		= 0x0000;
	*pPORTD_INEN      	= 0x0000;
}


/*******************************************************************
*   Function:    TEST_FPGA_GPIO
*   Description: Main test routine will get launched from the POST 
*				 framework.
*******************************************************************/
int TEST_FPGA_GPIO(void)
{
	int bError = 1; 	// returning 1 indicates a pass, anything else is a fail
	
	FPGA_Init_GPIO();
		
	// test PORTF
	Delay(1);
	if (*pPORTF != 0x0000)
		bError = 0;
	
	*pPORTF_SET = 0x000F;
	Delay(1);
	if (*pPORTF != 0x00FF)
		bError = 0;
		
	*pPORTF_CLEAR = 0x000F;
	Delay(1);
	if (*pPORTF != 0x0000)
		bError = 0;
	
	*pPORTF_SET = 0x0005;
	Delay(1);
	if (*pPORTF != 0x0055)
		bError = 0;
	
	*pPORTF_CLEAR = 0x0005;
	Delay(1);
	if (*pPORTF != 0x0000)
		bError = 0;
	
	*pPORTF_SET = 0x000A;
	Delay(1);
	if (*pPORTF != 0x00AA)
		bError = 0;
	
	*pPORTF_CLEAR = 0x000A;
	Delay(1);
	if (*pPORTF != 0x0000)
		bError = 0;
		
	// test PORTD
	if (*pPORTD != 0x0000)
		bError = 0;
	
	*pPORTD_SET = 0x00B0;
	Delay(1);
	if ( ((*pPORTD) & 0x3F0) != 0x03F0 )
		bError = 0;
	
	*pPORTD_CLEAR = 0x00B0;
	Delay(1);
	if (*pPORTD != 0x0000)
		bError = 0;
	
	*pPORTD_SET = 0x00A0;
	Delay(1);
	if ( ((*pPORTD) & 0x1E0) != 0x01E0 )
		bError = 0;
	
	*pPORTD_CLEAR = 0x00A0;
	Delay(1);
	if (*pPORTD != 0x0000)
		bError = 0;
	
	*pPORTD_SET = 0x0010;
	Delay(1);
	if ( ((*pPORTD) & 0x210) != 0x0210 )
		bError = 0;
	
	*pPORTD_CLEAR = 0x0010;
	Delay(1);
	if (*pPORTD != 0x0000)
		bError = 0;
		
	FPGA_Disable_GPIO();
		
	return bError;
}

/*******************************************************************
*   Function:    main
*   Description: used only if this test is needed as a standalone
*				 test separate from POST
*******************************************************************/
#ifdef _STANDALONE_
int main(void)
{
	int bPassed = 0;
	
	InitPLL();
	
	bPassed = TEST_FPGA_GPIO();
		
	return 0;
}
#endif //#ifdef _STANDALONE_
