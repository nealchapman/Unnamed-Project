/*******************************************************************
Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software
you agree to the terms of the associated Analog Devices License Agreement.

Project Name:  	Power_On_Self_Test

Hardware:		ADSP-BF548 EZ-KIT Lite

Description:	This file controls anything involving the LEDs or PBs
				on the EZ-KIT
*******************************************************************/

/*******************************************************************
*  include files
*******************************************************************/
#include "PBLED_test.h"
#include "Timer_ISR.h"


/*******************************************************************
*  global variables
*******************************************************************/

#define delay 0x400000


/*******************************************************************
*   Function:    Init_LEDs
*   Description: This function configures PORTG as output for LEDs
*******************************************************************/
void Init_LEDs(void)
{
	*pPORTG_FER 		&= ~0x0FC0;		// Setup PG6 - PG11 for LEDs
	*pPORTG_MUX 		&= ~0x0FC0;		// Turn all LEDs on
	*pPORTG_DIR_SET		= 0x0FC0;		// Setup port for output
	*pPORTG_CLEAR		= 0x0FC0;		// Turn all LEDs off
}

/*******************************************************************
*   Function:    Init_PushButtons
*   Description: This function configures PORTB as inputs for push buttons
*******************************************************************/
void Init_PushButtons(void)
{
    *pPORTB_FER 		= 0x0000;
    *pPORTB_MUX 		= 0x0000;
	*pPORTB_INEN		= 0x0F00;		// Enable input buffer
	*pPORTB_DIR_SET		&= (~0x0F00);	// Setup port for inputs
}


/*******************************************************************
*   Function:    LED_Bar
*   Description: Display a blinking LED bar
*******************************************************************/
void LED_Bar(const int iSpeed)
{
	enLED n;
	int j;

	for( n = LED1; n < LAST_LED; (n <<= 1) )
	{
		ClearSet_LED(n, 3);
		Delay(iSpeed);
	}

}

/*******************************************************************
*   Function:    Blink_LED
*   Description: Blink various LED's
*******************************************************************/
void Blink_LED(const int enleds, const int iSpeed)
{
	int j;
	enLED n;

	while( 1 )
	{
		for( n = LED1; n < LAST_LED; (n <<= 1) )
		{
			if( n & enleds )
			{
				ClearSet_LED(n, 3);
			}
		}

		Delay(iSpeed);
	}
}




/*******************************************************************
*   Function:    ClearSet_LED_Bank
*   Description: Clear or set a particular LED or group of LED's
*******************************************************************/
void ClearSet_LED_Bank(const int enleds, const int iState)
{
	enLED n;
	int nTempState = iState;


	for( n = LED1; n < LAST_LED; (n <<= 1) )
	{
		if( n & enleds )
		{
			ClearSet_LED(n, (nTempState & 0x3) );
		}

		nTempState >>= 2;
	}
}


/*******************************************************************
*   Function:    ClearSet_LED
*   Description: Clear or set a particular LED (NOT A GROUP)
*******************************************************************/
void ClearSet_LED(const enLED led, const int bState)
{
   if( 0 == bState )
	{
		int temp = *pPORTG;
		*pPORTG = temp & ~(led);
	}
	else if( 1 == bState )
	{
		*pPORTG |= led ;
	}
	else // toggle the bits
	{
		*pPORTG ^= led ;
	}

}

/*******************************************************************
*   Function:    TEST_PBLED
*   Description: Main test routine will get launched from the POST
*				 framework.
*******************************************************************/
int TEST_PBLED(void)
{
	// enable push button PB1, PB2, PB3, PB4
	*pPORTB_INEN		= 0x0F00;		// Enable input buffer
	*pPORTB_DIR_SET		&= (~0x0F00);	// Setup port for inputs

	// clear all the LED's
	ClearSet_LED_Bank( (-1), 0x0000);

	int nPushed = 0;
	int temp = 0;
	unsigned int nTimer = SetTimeout(0x400000);
	if( ((unsigned int)-1) != nTimer )
	{
		do{
			temp = *pPORTB;
			if( 0x100 == (temp & 0x100) )
			{	// PB1 was pressed.
				nPushed |= 0x100;
				ClearSet_LED_Bank( (LED1|LED2|LED3|LED4|LED5|LED6), 0x15 );
			}
			else if( 0x200 == (temp & 0x200) )
			{	// PB2 was pressed.
				nPushed |= 0x200;
				ClearSet_LED_Bank( (LED1|LED2|LED3|LED4|LED5|LED6), 0x540);
			}
			else if( 0x400 == (temp & 0x400) )
			{	// PB2 was pressed.
				nPushed |= 0x400;
				ClearSet_LED_Bank( (LED1|LED2|LED3|LED4|LED5|LED6), 0x111);
			}
			else if( 0x800 == (temp & 0x800) )
			{	// PB2 was pressed.
				nPushed |= 0x800;
				ClearSet_LED_Bank( (LED1|LED2|LED3|LED4|LED5|LED6), 0x444);
			}
		}while( (0xF00 != nPushed) && (!IsTimedout(nTimer))  );
	}

	ClearTimeout(nTimer);

	// disable  push button PB1, PB2, PB3, PB4
	*pPORTB_INEN		= 0x0000;			// Pushbutton
	*pPORTB_DIR_CLEAR	&= (~0x0000);		// Pushbutton

	return (0xF00 == nPushed);

}

