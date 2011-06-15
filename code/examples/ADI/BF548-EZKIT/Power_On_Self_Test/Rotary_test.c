/*******************************************************************
Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software
you agree to the terms of the associated Analog Devices License Agreement.  

Project Name:  	Power_On_Self_Test

Hardware:		ADSP-BF548 EZ-KIT Lite

Description:	This file tests the rotary encoder interface on the EZ-KIT
*******************************************************************/

/*******************************************************************
*  include files
*******************************************************************/
#include <cdefBF548.h>
#include <ccblkfn.h>
#include <sys\exception.h>
#include <stdio.h>
#include <signal.h>

#include "Timer_ISR.h"
#include "PBLED_test.h"


/*******************************************************************
*  function prototypes
*******************************************************************/
void Init_Rotary(void);
void Init_Rotary_Interrupts(void);
int TEST_ROTARY(void);
EX_INTERRUPT_HANDLER(CNT_ISR);


/*******************************************************************
*  global variables
*******************************************************************/
int g_BlinkSpeed = BLINK_SLOW;
int nAllRotaryFunctionTested = 0;
bool bLedsAllOff = 0;
bool bLedsAllOn = 0;
bool bMinHit = 0;
bool bMaxHit = 0;
bool bZeroHit = 0;


/*******************************************************************
*   Function:    Init_Keypad
*   Description: This function configures the Rotary counters and 
*				 initializes the PORT pins that are used
*******************************************************************/
void Init_Rotary(void)
{
    // initialize PORTG for alternate function - CNT CZM
    *pPORTG_FER = 0x020;
    *pPORTG_MUX = 0xC00;
    
    /// initialize PORTH for alternate function - CNT CUD & CNT CDG
    *pPORTH_FER = 0x18;
    *pPORTH_MUX = 0x280;
     	
    *pCNT_IMASK = 0x11E;  	// counter zeroed by zero marker, downcount interrupt enable, upcount interrupt enable
    *pCNT_COMMAND = 0x1000; // enable single zero-marker clear
    *pCNT_DEBOUNCE = 0x2;  	// 4x filter time
    *pCNT_CONFIG = 0x001;  	// enable zero counter, enable counter
}

/*******************************************************************
*   Function:    Init_Rotary_Interrupts
*   Description: Initializes Rotary Counter interrupts
*******************************************************************/
void Init_Rotary_Interrupts(void)
{
  	// assign ISRs to interrupt vectors
	register_handler(ik_ivg8, CNT_ISR);
	
	*pILAT |= EVT_IVG8;							// clear pending IVG8 interrupts
	ssync();
	
    // enable Counter (CNT) Interrupt
	*pSIC_IMASK2 |= 0x10;  
}


/*******************************************************************
*   Function:    TEST_ROTARY
*   Description: Main test routine will get launched from the POST 
*				 framework.
*******************************************************************/
int TEST_ROTARY(void)
{
	int nIndex = 0;
	int bSuccess = 1; 	// returning 1 indicates a pass, anything else is a fail
	int n;

	// initialize timers
	Init_Timers();
	Init_Timer_Interrupts();
	
	// intialize rotary register and interrupts
	Init_Rotary();
	Init_Rotary_Interrupts();

	ClearSet_LED_Bank( (-1), 0x0000);
	
		
	while( !bMinHit || !bMaxHit || !bZeroHit )
	{
	    if( bLedsAllOff )
	    {
	        ClearSet_LED_Bank( (LED1|LED2|LED3|LED4|LED5|LED6), 0x0 );
	    }
	    else if( bLedsAllOn )
	    {
	        ClearSet_LED_Bank( (LED1|LED2|LED3|LED4|LED5|LED6), 0x555 );
	    }
	    else
	    {
			LED_Bar(g_BlinkSpeed);
	    }
	}

	interrupt(ik_ivg8, SIG_IGN);

	// disable SIC Level Interrupts
	*pSIC_IMASK2 &= (~(0x10));

	return bSuccess;
}

EX_INTERRUPT_HANDLER(CNT_ISR)
{
    int Status = 0;
    int Counter = 0;
    signed int temp = 0;
    
    Status = *pCNT_STATUS;
    Counter = *pCNT_COUNTER;
     	
    // upcount
	if (Status & 0x2)
	{
	    // check to see if maximum was reached
	    if( (Counter > 200) )
	    {
	        bMaxHit = 1;
	        bLedsAllOn = 0;
	    	bLedsAllOff = 1;
	    }
	    else if( (Counter < 200) && (Counter >= 0) )
	    {
	    	bLedsAllOn = 0;
	    	bLedsAllOff = 0;
			g_BlinkSpeed -= 0x10;
	    }
	}
	// downcount
	else if (Status & 0x4)
	{ 
	    // check to see if minimum reached
	    if (Counter < -200)
	    {
	        bMinHit = 1;
	        bLedsAllOff = 0;
	        bLedsAllOn = 1; 
	    }
	    else if( (Counter > -200) && (Counter < 0) )
	    {
	        bLedsAllOff = 0;
	        bLedsAllOn = 0;
			g_BlinkSpeed += 0x10;
	    }
	}
	else if (Status & 0x100)
	{
	    *pCNT_COMMAND = 0x1000;  // enable single zero-marker clear
	}
	
	if (Status & 0x400)
	{
	    bLedsAllOff = 0;
	    bLedsAllOn = 0;
	    bZeroHit = 1;
	    g_BlinkSpeed = BLINK_SLOW;
	}
		
	*pCNT_STATUS = Status; 
	
	// make sure we never go less than 0
	// because the blink routine will take forever
	if( g_BlinkSpeed < 0 )
		g_BlinkSpeed = BLINK_SLOW;
		 			
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

	bPassed = TEST_ROTARY();
	
	return 0;
}
#endif //#ifdef _STANDALONE_
