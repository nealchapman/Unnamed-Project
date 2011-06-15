/*******************************************************************
Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software
you agree to the terms of the associated Analog Devices License Agreement.  

Project Name:  	Power_On_Self_Test

Hardware:		ADSP-BF548 EZ-KIT Lite

Description:	This file controls the timers
*******************************************************************/

/*******************************************************************
*  include files
*******************************************************************/
#include <ccblkfn.h>
#include "Timer_ISR.h"


/*******************************************************************
*  global variables and defines
*******************************************************************/
#define MAX_NUM_COUNTDOWN_TIMERS 5

typedef struct CountDownTimer_TAG
{
	bool m_IsActive;
	unsigned long m_ulTimeoutCounter;
}countdowntimer;

static volatile unsigned long g_ulTickCount;
static countdowntimer sCountDownTimer[MAX_NUM_COUNTDOWN_TIMERS] = { {0,0},{0,0},{0,0},{0,0},{0,0} };


/*******************************************************************
*   Function:    Init_Timers
*   Description: This function initialises Timer0 for PWM mode.
*******************************************************************/
void Init_Timers(void)
{
	// active state, auto reload, generate interrupt
	*pTCNTL = TMPWR | TAUTORLD | TINT;
	*pTPERIOD = TIMEOUT_PERIOD;
	*pTSCALE = TIMEOUT_PERIOD/2;

	// enable the timer
	*pTCNTL |= TMREN;
}

/*******************************************************************
*   Function:    Init_Timer_Interrupts
*   Description: This function initialises the interrupts for Timer0
*******************************************************************/
void Init_Timer_Interrupts(void)
{
	register_handler(ik_timer, Timer_ISR);
}


/*******************************************************************
*   Function:    Delay
*   Description: Delay for a fixed number of Ms, blocks
*******************************************************************/
void Delay(const unsigned long ulMs)
{
	unsigned int uiTIMASK = cli();

	g_ulTickCount = 0;
	unsigned long ulEnd = (g_ulTickCount + ulMs);

	sti(uiTIMASK);

    while( g_ulTickCount < ulEnd )
	{
		asm("nop;");
	}
}


/*******************************************************************
*   Function:    SetTimeout
*   Description: Set a value for a global timeout, return the timer
*******************************************************************/
unsigned int SetTimeout(const unsigned long ulTicks)
{
	unsigned int uiTIMASK = cli();
	unsigned int n;

	// we don't care which countdown timer is used, so search for a free
	// timer structure
	for( n = 0;  n < MAX_NUM_COUNTDOWN_TIMERS; n++ )
	{
		if( false == sCountDownTimer[n].m_IsActive )
		{
			sCountDownTimer[n].m_IsActive = true;
			sCountDownTimer[n].m_ulTimeoutCounter = ulTicks;


			sti(uiTIMASK);
			return n;
		}
	}

	sti(uiTIMASK);
	return ((unsigned int)-1);
}

/*******************************************************************
*   Function:    ClearTimeout
*   Description: Set a value for a global timeout, return the timer
*******************************************************************/
unsigned long ClearTimeout(const unsigned int nIndex)
{
	unsigned int uiTIMASK = cli();
	unsigned long ulTemp = (unsigned int)(-1);

	if( nIndex < MAX_NUM_COUNTDOWN_TIMERS )
	{
		// turn off the timer
		ulTemp = sCountDownTimer[nIndex].m_ulTimeoutCounter;
		sCountDownTimer[nIndex].m_ulTimeoutCounter = 0;
		sCountDownTimer[nIndex].m_IsActive = false;
	}

	sti(uiTIMASK);
	return (ulTemp);
}

/*******************************************************************
*   Function:    IsTimedout
*   Description: Checks to see if the timeout value has expired
*******************************************************************/
bool IsTimedout(const unsigned int nIndex)
{
	unsigned int uiTIMASK = cli();
	if( nIndex < MAX_NUM_COUNTDOWN_TIMERS )
	{
		sti(uiTIMASK);
		return ( 0 == sCountDownTimer[nIndex].m_ulTimeoutCounter );
	}

	sti(uiTIMASK);
	return 0;// an invalid index should cause a hang wherever a timer is being used
}


/*******************************************************************
*   Function:    EX_INTERRUPT_HANDLER(Timer_ISR)
*   Description: Timer ISR
*******************************************************************/
EX_INTERRUPT_HANDLER(Timer_ISR)
{
	unsigned int n;
	// confirm interrupt handling
	*pTIMER_STATUS0 = 0x0001;
	ssync();

	g_ulTickCount++;


	// decrement each counter if it is non-zero
	for( n = 0;  n < MAX_NUM_COUNTDOWN_TIMERS; n++ )
	{
		if( 0 != sCountDownTimer[n].m_ulTimeoutCounter )
		{
			sCountDownTimer[n].m_ulTimeoutCounter--;
		}
	}
}

