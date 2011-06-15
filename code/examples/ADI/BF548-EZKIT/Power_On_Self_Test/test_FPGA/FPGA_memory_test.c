/*******************************************************************
Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software
you agree to the terms of the associated Analog Devices License Agreement.  

Project Name:  	Power_On_Self_Test

Hardware:		ADSP-BF548 EZ-KIT Lite

Description:	This file tests the memory on the FPGA EZ-Extender 
				board.
*******************************************************************/

/*******************************************************************
*  include files
*******************************************************************/
#include <cdefBF548.h>
#include <ccblkfn.h>
#include "../Timer_ISR.h"

/*******************************************************************
*  global variables
*******************************************************************/
#define SRAM_START  0x28000000	// start address of SRAM
#define SRAM_SIZE	0x00002000	// 512k x 32-bits


/*******************************************************************
*  function prototypes
*******************************************************************/
void FPGA_Init_MemTest(void);
int TEST_FPGA_MEMORY(void);


/*******************************************************************
*   Function:    FPGA_Init_MemTest
*   Description: This function configures the EBIU to allow access to
*                the FPGA as a memory mapped peripheral
*******************************************************************/
void FPGA_Init_MemTest(void)
{

    // Initalize EBIU control registers to enable all banks	
	*pEBIU_AMGCTL = 0x000E;
	ssync();

	*pEBIU_AMCBCTL1 = 0xFF02;
	ssync();
	
	// set port H MUX to configure PH8-PH13 as 1st Function (MUX = 00) 
	// (bits 16-27 == 0) - Address signals A4-A9
	*pPORTH_FER = 0x3F00;
	*pPORTH_MUX = 0x0;
	*pPORTH_DIR_SET = 0x3F00;
	
	// configure PI0-PI15 as A10-A21 respectively
	*pPORTI_FER = 0xFFFF;
	*pPORTI_MUX = 0x0;
	*pPORTI_DIR_SET = 0xFFFF;
	
	// PF13 is used as bank select for the SRAM
	// PF13 = 0 accesses lower bank 15:0
	// PF13 = 1 accesses upper bank 31:16
	*pPORTF_FER = 0x0000;		// put PF0 - PF7 in GPIO mode
	// Set the GPIO direction: 0 = input, 1 = output
	*pPORTF_CLEAR		= 0x0000;	// PF13 = 0
	*pPORTF_DIR_SET		= 0x2000;	// PF13 = ouput
}


/*******************************************************************
*   Function:    TEST_FPGA_MEMORY
*   Description: Main test routine will get launched from the POST 
*				 framework.
*******************************************************************/
int TEST_FPGA_MEMORY(void)
{
	
	volatile unsigned int *pDst;
	int nIndex = 0, nValue =0;
	int bError = 1; 	// returning 1 indicates a pass, anything else is a fail
	int nTests;
		
	FPGA_Init_MemTest();	
		
	for(nTests = 0; nTests < 2; ++nTests)
	{
		if (nTests == 0)
			*pPORTF_CLEAR	= 0x2000;	// PF13 = 0 : Test BANK0
		else
			*pPORTF_SET	= 0x2000;		// PF13 = 1 : Test BANK1
	
		// write incrementing values to each SRAM location
		for(nIndex = 0x0, pDst = (unsigned int *)SRAM_START; nIndex  < (SRAM_SIZE); pDst++, nIndex++ )
		{
			*pDst = nIndex;
			ssync();
		}
		
		// verify incrementing values
		for(nIndex = 0x0, pDst = (unsigned int *)SRAM_START ; pDst  < (unsigned int *)(SRAM_START + SRAM_SIZE); pDst++, nIndex++ )
		{
			if( nIndex != *pDst)
			{
   				bError = 0;
   				break;
			}
		}		

		for(nIndex = 0xFFFFFFFF, pDst = (unsigned int *)SRAM_START; pDst < (unsigned int *)(SRAM_START + SRAM_SIZE); pDst++ )
		{
			*pDst = nIndex;
		}
	
		// verify all FFFF's 
		for(nIndex = 0xFFFFFFFF, pDst = (unsigned int *)SRAM_START; pDst < (unsigned int *)(SRAM_START + SRAM_SIZE); pDst++ )
		{
			if( nIndex != *pDst )
			{
				bError = 0;
				break;
			}
		}

		// write all AAAA's 
		for(nIndex = 0xAAAAAAAA, pDst = (unsigned int *)SRAM_START; pDst < (unsigned int *)(SRAM_START + SRAM_SIZE); pDst++ )
		{
			*pDst = nIndex;
		}
		
		// verify all AAAA's 
		for(nIndex = 0xAAAAAAAA, pDst = (unsigned int *)SRAM_START; pDst < (unsigned int *)(SRAM_START + SRAM_SIZE); pDst++ )
		{
			if( nIndex != *pDst )
			{
				bError = 0;
				break;
			}
		}
			
		// write all 5555's 
		for(nIndex = 0x55555555, pDst = (unsigned int *)SRAM_START; pDst < (unsigned int *)(SRAM_START + SRAM_SIZE); pDst++ )
		{
			*pDst = nIndex;
		}
		
		// verify all 5555's 
		for(nIndex = 0x55555555, pDst = (unsigned int *)SRAM_START; pDst < (unsigned int *)(SRAM_START + SRAM_SIZE); pDst++ )
		{
			if( nIndex != *pDst )
			{
				bError = 0;
				break;
			}
		}
	}
	
	int wrBank0 = 0x1234, rdBank0;
	int wrBank1 = 0x5678, rdBank1;
	pDst = (unsigned int *)SRAM_START;

	// write to bank0
	*pPORTF_CLEAR	= 0x2000;	// PF13 = 0 : Test BANK0	
	*pDst = wrBank0;
	Delay(1);
	rdBank0 = *pDst;
	if (rdBank0 != wrBank0)
		bError = 0;
	// write to bank1
	*pPORTF_SET	= 0x2000;	// PF13 = 1 : Test BANK1	
	*pDst = wrBank1;
	Delay(1);
	rdBank1 = *pDst;
	if (rdBank1 != wrBank1)
		bError = 0;
	
	// now check that the bank selection is really working
	// read from bank0 and make sure we don't match the bank1 value
	*pPORTF_CLEAR	= 0x2000;	// PF13 = 0 : Test BANK0	
	rdBank0 = *pDst;
	if (rdBank0 == wrBank1)
		bError = 0;
	// read from bank1 and make sure we don't match the bank0 value
	*pPORTF_SET	= 0x2000;	// PF13 = 1 : Test BANK1	
	rdBank1 = *pDst;
	if (rdBank1 == wrBank0)
		bError = 0;
	    
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
	
	bPassed = TEST_FPGA_MEMORY();
	
	return 0;
}
#endif //#ifdef _STANDALONE_

