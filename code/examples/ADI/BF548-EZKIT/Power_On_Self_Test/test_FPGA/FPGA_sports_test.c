/*******************************************************************
Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software
you agree to the terms of the associated Analog Devices License Agreement.  

Project Name:  	Power_On_Self_Test

Hardware:		ADSP-BF548 EZ-KIT Lite

Description:	This file tests the SPORTs on the FPGA EZ-Extender 
				board.
*******************************************************************/

/*******************************************************************
*  include files
*******************************************************************/
#include <sys\exception.h>
#include <cdefBF548.h>
#include <ccblkfn.h>
#include <signal.h>

#include "../Timer_ISR.h"

#define SPORT_BLOCK_SIZE	0x100000  // 4MB

/*******************************************************************
*  function prototypes
*******************************************************************/
EX_INTERRUPT_HANDLER(SPORT2TX_ISR);
EX_INTERRUPT_HANDLER(SPORT3RX_ISR);
int TEST_FPGA_SPORTS(void);

/*******************************************************************
*  global variables and defines
*******************************************************************/
volatile int sp2txCnt = 0;
volatile int sp3rxCnt = 0;
volatile int sp2rxErr = 0;	// 0 = no error, 1 = error
volatile int sp3rxErr = 0;

// these buffers will only hold the last 16 data elements that were transfered
volatile int bufSPORT2RxPrimaryOnly[16];
volatile int bufSPORT2RxPriSec[16];

volatile int bufSPORT3RxPrimaryOnly[16];
volatile int bufSPORT3RxPriSec[16];

volatile int *bufSPORT2Rx;
volatile int *bufSPORT3Rx;

/*******************************************************************
*   Function:    SPORT2TX_ISR
*   Description: Sport2 tx interrupt handler
*******************************************************************/
EX_INTERRUPT_HANDLER(SPORT2TX_ISR)
{
	// don't send unless the rx fifo is empty
	if( !(*pSPORT3_STAT & RXNE) )
	{
		*pSPORT2_TX = sp2txCnt;
		++sp2txCnt;
	}
}

/*******************************************************************
*   Function:    SPORT3RX_ISR
*   Description: Sport3 rx interrupt handler
*******************************************************************/
EX_INTERRUPT_HANDLER(SPORT3RX_ISR)
{
	if (sp3rxCnt < SPORT_BLOCK_SIZE)
	{
		bufSPORT3Rx[sp3rxCnt % 16] = *pSPORT3_RX;
		if (bufSPORT3Rx[sp3rxCnt % 16] != sp3rxCnt)
		{
			sp3rxErr = 1;
			*pSPORT2_TCR1 &= ~TSPEN;
			*pSPORT3_RCR1 &= ~RSPEN;
		}
		else
		{
			++sp3rxCnt;
		}
	}
	else
	{
		*pSPORT2_TCR1 &= ~TSPEN;
		*pSPORT3_RCR1 &= ~RSPEN;
	}

}


/*******************************************************************
*   Function:    TEST_FPGA_SPORTS
*   Description: Main test routine will get launched from the POST 
*				 framework.
*******************************************************************/
int TEST_FPGA_SPORTS(void)
{
	int bError = 1; 	// returning 1 indicates a pass, anything else is a fail

	// about 10 sec @ 120MHz SCLK (1024 is the period of timer0)
	unsigned int index = SetTimeout(10*120000000/TIMEOUT_PERIOD);

	if (index != 0xffffffff)
	{

		// set up PORTA as SPORTs
		*pPORTA_MUX = 0x0000;
	    ssync();;

		// configure as peripheral NOT GPIO
		*pPORTA_FER = 0xFFFF;
		ssync();

		*pILAT |= EVT_IVG8;							// clear pending IVG8 interrupts
		ssync();

		*pILAT |= EVT_IVG9;							// clear pending IVG9 interrupts
		ssync();

		*pSIC_IAR4 = 0x32222120;

		register_handler(ik_ivg8,  SPORT2TX_ISR);
		register_handler(ik_ivg9,  SPORT3RX_ISR);

		// enable SPORT2/1 RX/TX interrupts
		*pSIC_IMASK1 |= IRQ_DMA19 | IRQ_DMA20;

		/////////////////////////////////////
		// setup SPORT2 -> SPORT3 transfer (Primary ONLY)
		/////////////////////////////////////
		sp2txCnt = 0;
		sp3rxCnt = 0;
		bufSPORT3Rx = bufSPORT3RxPrimaryOnly;

		*pSPORT2_TCR1 = 0;	// make sure the port is disabled
		*pSPORT2_TCR1 = LTFS | TFSR | ITFS | ITCLK;
		*pSPORT2_TCR2 = 31;
		*pSPORT2_TFSDIV = 31;

		*pSPORT3_RCR1 = 0;	// make sure the port is disabled
		*pSPORT3_RCR1 = LRFS | RFSR;
		*pSPORT3_RCR2 = 31;
		*pSPORT3_RFSDIV = 31;

		// enable the ports
		*pSPORT3_RCR1 |= RSPEN;
		*pSPORT2_TCR1 |= TSPEN;

		// wait for the data to be received or an error
		while ((sp3rxCnt < SPORT_BLOCK_SIZE) && (sp3rxErr == 0))
		{
			if (IsTimedout(index))
			{
				sp3rxErr = 1;
				bError = 0;
				break;
			}
		}

		// disable the ports
		*pSPORT3_RCR1 &= ~RSPEN;
		*pSPORT2_TCR1 &= ~TSPEN;

		ClearTimeout(index);
		
		if (sp3rxErr)
		{
			bError = 0;
			return bError;
		}

		// set the timeout value again
		index = SetTimeout(10*120000000/TIMEOUT_PERIOD);
		
		/////////////////////////////////////
		// setup SPORT2 -> SPORT3 transfer (Primary and Secondary)
		/////////////////////////////////////
		sp2txCnt = 0;
		sp3rxCnt = 0;
		bufSPORT3Rx = bufSPORT3RxPriSec;

		*pSPORT2_TCR1 = 0;	// make sure the port is disabled
		*pSPORT2_TCR1 = LTFS | TFSR | ITFS | ITCLK;
		*pSPORT2_TCR2 = TXSE | 31;
		*pSPORT2_TFSDIV = 31;
		*pSPORT2_TCLKDIV = 1;

		*pSPORT3_RCR1 = 0;	// make sure the port is disabled
		*pSPORT3_RCR1 = LRFS | RFSR;
		*pSPORT3_RCR2 = RXSE | 31;
		*pSPORT3_RFSDIV = 31;

		// enable the ports
		*pSPORT3_RCR1 |= RSPEN;
		*pSPORT2_TCR1 |= TSPEN;

		// wait for the data to be received or an error
		while ((sp3rxCnt < SPORT_BLOCK_SIZE) && (sp3rxErr == 0))
		{
			if (IsTimedout(index))
			{
				sp3rxErr = 1;
				bError = 0;
				break;
			}
		}

		// disable the ports
		*pSPORT2_TCR1 &= ~TSPEN;
		*pSPORT3_RCR1 &= ~RSPEN;

		if (sp3rxErr)
		{
			bError = 0;
			return bError;
		}

		ClearTimeout(index);
	}
	else
	{
		bError = 0;
	}

	// clean-up, keep TIMER0 enabled though
	interrupt(ik_ivg8, SIG_IGN);
    interrupt(ik_ivg9, SIG_IGN);

	// disable SIC Level Interrupts
	*pSIC_IMASK1 &= (~(IRQ_DMA19 | IRQ_DMA20));
	ssync();


	return bError;	// 0 = error, 1 = pass
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

	bPassed = TEST_FPGA_SPORTS();

	return 0;
}
#endif //#ifdef _STANDALONE_
