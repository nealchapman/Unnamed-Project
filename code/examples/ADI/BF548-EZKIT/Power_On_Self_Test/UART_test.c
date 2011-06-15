/*******************************************************************
Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software
you agree to the terms of the associated Analog Devices License Agreement.  

Project Name:  	Power_On_Self_Test

Hardware:		ADSP-BF548 EZ-KIT Lite

Description:	This file tests the UART interface on the EZ-KIT
*******************************************************************/

/*******************************************************************
*  include files
*******************************************************************/
#include <cdefBF548.h>
#include <ccblkfn.h>

#include "Timer_ISR.h"


/*******************************************************************
*  global variables and defines
*******************************************************************/
// Apply formula DL = PERIOD / 16 / baud rate and call uart_init that writes
// the result to the two 8-bit DL registers (DLH:DLL).
#define BAUD_RATE_115200 	(133*1000000)/16/115200
#define MAX_TEST_CHARS		5000


/*******************************************************************
*  function prototypes
*******************************************************************/
void Init_UART(void);
int PutChar(const char c);
int GetChar(char *const c);
int TEST_UART(void);


/*******************************************************************
*   Function:    Init_UART
*   Description: Initialize UART with the appropriate values
*******************************************************************/
void Init_UART(void)
{
	volatile int temp;
	
	*pPORTH_FER = 0x0003;
    ssync();
    
	*pPORTH_MUX = 0;
    ssync();
  

/*****************************************************************************
 *
 *  First of all, enable UART clock.
 *
 ****************************************************************************/
	*pUART1_GCTL = UCEN;

/*****************************************************************************
 *
 *  Read period value and apply formula:  DL = PERIOD / 16 / 8
 *  Write result to the two 8-bit DL registers (DLH:DLL).
 *
 ****************************************************************************/
	*pUART1_LCR = WLS_8;
	*pUART1_DLL = BAUD_RATE_115200;
	*pUART1_DLH = (BAUD_RATE_115200 >> 8);

/*****************************************************************************
 *
 *  Finally enable interrupts inside UART module, by setting proper bits
 *  in the IER register. It is good programming style to clear potential
 *  UART interrupt latches in advance, by reading RBR, LSR and IIR.
 *
 *  Setting the ETBEI bit automatically fires a TX interrupt request.
 *
 ****************************************************************************/
	temp = *pUART1_RBR;
	temp = *pUART1_LSR;

	*pUART1_IER_SET = ETBEI;
}

/*******************************************************************
*   Function:    PutChar
*   Description: transmit a character of data
*******************************************************************/
int PutChar(const char cVal)
{
	int nStatus = 0;
	unsigned int nTimer = SetTimeout(1000);
	if( ((unsigned int)-1) != nTimer )
	{
		do{
			if( (*pUART1_LSR & THRE) )
			{
				*pUART1_THR = cVal;
				nStatus = 1;
				break;
			}
		}while( !IsTimedout(nTimer) );
	}

	ClearTimeout(nTimer);

	return nStatus;
}

/*******************************************************************
*   Function:    GetChar
*   Description: get a character of data
*******************************************************************/
int GetChar(char *const cVal)
{
	int nStatus = 0;
	unsigned int nTimer = SetTimeout(1000);
	if( ((unsigned int)-1) != nTimer )
	{
		do{
			if( DR == (*pUART1_LSR & DR) )
			{
				*cVal = *pUART1_RBR;
				nStatus = 1;
				break;
			}
		}while( !IsTimedout(nTimer) );
	}

	ClearTimeout(nTimer);

	return nStatus;
}

/*******************************************************************
*   Function:    TEST_UART
*   Description: Main test routine will get launched from the POST 
*				 framework.
*******************************************************************/
int TEST_UART(void)
{
	int n;
	char cTxChar;
	char cRxChar;

	Init_UART();

	for(n = 0; n < MAX_TEST_CHARS; n++)
	{
		cTxChar = (n & 0xFF);

		if( 0 == PutChar(cTxChar) )
		{
			return 0;
		}

		if( 0 == GetChar( &cRxChar ) )
		{
			return 0;
		}

		if( cTxChar != cRxChar )
		{
			return 0;
		}
	}

	return 1;
}

/*******************************************************************
*   Function:    main
*   Description: used only if this test is needed as a standalone
*				 test separate from POST
*******************************************************************/
#ifdef _STANDALONE_ // use this to run standalone tests
int main(void)
{
	int bPassed = 0;

	Init_UART();

	while(1)
	{
		bPassed = TEST_UART();

		if( 0 == bPassed )
		{
			break;
		}
	}



	return 0;
}
#endif //#ifdef _STANDALONE_

