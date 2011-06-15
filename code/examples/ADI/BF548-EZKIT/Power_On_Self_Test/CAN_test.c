/*******************************************************************
Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software
you agree to the terms of the associated Analog Devices License Agreement.  

Project Name:  	Power_On_Self_Test

Hardware:		ADSP-BF548 EZ-KIT Lite

Description:	This file tests the CAN interface on the EZ-KIT.
*******************************************************************/

/*******************************************************************
*  include files
*******************************************************************/
#include <cdefBF549.h>
#include <ccblkfn.h>
#include <sys/exception.h>
#include <signal.h>

#include "Timer_ISR.h"


/*******************************************************************
*  global variables and defines
*******************************************************************/
#define CAN_RX_MB_LO	0x00FF	// Mailboxes 0-7 are RX, 8 - 15 are TX
#define CAN_RX_MB_HI	0x0000	// Mailboxes 16 - 31 are TX
#define CAN_EN_MB_LO	0x00FF	// Mailboxes 0-7 are RX, 8 - 15 are TX
#define CAN_EN_MB_HI	0xFF00	// Mailboxes 16 - 31 are TX
#define BUF_SIZE		1024
#define UC_WDOG			0x0002	// 		Watchdog Mode

// flag for who is sending and receiving
int g_CAN1_SEND=1;
int g_CAN0_RECV=1;

typedef struct g_CAN_STAMP_BUF_TAG
{
	short zero;
	short one;
}g_CAN_STAMP_BUF_type;

short *g_CAN_RX_BUFFER;
g_CAN_STAMP_BUF_type *g_CAN_STAMP_BUF;

volatile unsigned short g_rx_buf_index = 0;
volatile unsigned short g_tx_buf_index = 0;
volatile unsigned short g_TX_Count = 0;
volatile unsigned short g_RX_Count = 0;
volatile unsigned short g_CAN_ERROR_FLAG = 0;	// flag that can error isr sets


/*******************************************************************
*  function prototypes
*******************************************************************/
EX_INTERRUPT_HANDLER(CAN_XMT_RCV_HANDLER);
void Init_CAN_Port(void);
int CAN_Transmit(void);
int Init_CAN_Interrupts(void);
int Start_Transmit(void);


/*******************************************************************
*   Function:    Init_CAN_Port
*   Description: sets up the CAN port for the test
*******************************************************************/
void Init_CAN_Port(void)
{
	unsigned int nTimer;
	short msgID;
	char mbID;
	volatile unsigned short *pCAN_Ptr;


 	// setup PORTD so that we can read CAN Error
	*pPORTG_FER = 0xF000;		// CAN0 and CAN1 TX/RX
	*pPORTG_MUX = 0x0000;

	// setup PORTD so that we can read CAN Error
	*pPORTC_FER 	= 0x1;		// PC0(CAN0 Error)
	*pPORTC_INEN	= 0x1;		// Enable input buffer
	*pPORTC_DIR_SET		&= (~0x1);	// Setup port for inputs


	/// init the CAN timing registers
	// ===================================================
	// BIT TIMING:
	//
	// CAN_CLOCK  : Prescaler (BRP)
	// CAN_TIMING : SJW = 3, TSEG2 = 5, TSEG1 = 2
	//
	// ===================================================
	// Set Bit Configuration Registers ...
	// ===================================================
	*pCAN0_TIMING = 0x0352;
	*pCAN1_TIMING = 0x0352;
	*pCAN0_CLOCK  = 22;		// 500kHz CAN Clock :: tBIT = 2us
	*pCAN1_CLOCK  = 22;		// 500kHz CAN Clock :: tBIT = 2us
	//
	// tBIT = TQ x (1 + (TSEG1 + 1) + (TSEG2 + 1))
	// 2e-6 = TQ x (1 + 3 + 6)
	// TQ = 2e-7
	//
	// TQ = (BRP+1) / SCLK
	// 2e-7 = (BRP+1) / 112.5e6
	// (BRP+1) = 21.6
	// BRP = 21.5 = ~22

	ssync();


	/// init the CAN mail boxes and turn on the can ISR
	g_CAN_STAMP_BUF = malloc( (sizeof(g_CAN_STAMP_BUF_type) * BUF_SIZE * 2) );
	g_CAN_RX_BUFFER = malloc( (sizeof(short) * BUF_SIZE) );

/////////////////////////////////////////////////////////////////////////////////
// Mailboxes 24-31 are TX Only, They'll Transmit To the RX Only Mailboxes 0-7  //
// Mailbox 24 Transmits ID 0x200
	msgID = 0x200;
	for (mbID = 24; mbID<32; mbID++)
	{
		pCAN_Ptr = (unsigned short *)CAN0_MB00_DATA0 + (0x10 * mbID);

		*(pCAN_Ptr + 0)  = msgID; 		// write msgID to DATA0
		*(pCAN_Ptr + 2)  = msgID; 		// write msgID to DATA1
		*(pCAN_Ptr + 4)  = msgID; 		// write msgID to DATA2
		*(pCAN_Ptr + 6)  = msgID; 		// write msgID to DATA3
		*(pCAN_Ptr + 8)  = 8; 			// write 8 to LENGTH
		*(pCAN_Ptr + 10) = 0; 			// write 0 to TIMESTAMP
		*(pCAN_Ptr + 12) = 0; 			// write 0 to ID0
		*(pCAN_Ptr + 14) = (msgID << 2); // write msgID to ID1

		msgID++;							// Bump msgID to match Mailbox
	} // end for (init TX mailbox area -- TX Only mailboxes)


/////////////////////////////////////////////////////////////////////////////////
// Mailboxes 0-7 are RX Only, They'll Receive From the TX Only Mailboxes 24-31 //
// Mailbox 0 Receives ID 0x200
	msgID = 0x200;
	for (mbID = 0; mbID<8; mbID++)
	{
		pCAN_Ptr = (unsigned short *)CAN0_MB00_DATA0 + (0x10 * mbID);

		*(pCAN_Ptr + 0)  = 0; 		 	// write msgID to DATA0
		*(pCAN_Ptr + 2)  = 0; 			// write msgID to DATA1
		*(pCAN_Ptr + 4)  = 0; 			// write msgID to DATA2
		*(pCAN_Ptr + 6)  = 0; 			// write msgID to DATA3
		*(pCAN_Ptr + 8)  = 8; 			// write 8 to LENGTH
		*(pCAN_Ptr + 10) = 0; 			// write 0 to TIMESTAMP
		*(pCAN_Ptr + 12) = 0; 			// write 0 to ID0
		*(pCAN_Ptr + 14) = (msgID << 2); // write msgID to ID1

		msgID ++;							// Bump msgID to match Mailbox
	} // end for (init RX mailbox area -- RX Only mailboxes)


/////////////////////////////////////////////////////////////////////////////////
// Mailboxes 24-31 are TX Only, They'll Transmit To the RX Only Mailboxes 0-7  //
// Mailbox 24 Transmits ID 0x200
	msgID = 0x200;
	for (mbID = 24; mbID<32; mbID++)
	{
		pCAN_Ptr = (unsigned short *)CAN1_MB00_DATA0 + (0x10 * mbID);

		*(pCAN_Ptr + 0)  = msgID; 		// write msgID to DATA0
		*(pCAN_Ptr + 2)  = msgID; 		// write msgID to DATA1
		*(pCAN_Ptr + 4)  = msgID; 		// write msgID to DATA2
		*(pCAN_Ptr + 6)  = msgID; 		// write msgID to DATA3
		*(pCAN_Ptr + 8)  = 8; 			// write 8 to LENGTH
		*(pCAN_Ptr + 10) = 0; 			// write 0 to TIMESTAMP
		*(pCAN_Ptr + 12) = 0; 			// write 0 to ID0
		*(pCAN_Ptr + 14) = (msgID << 2); // write msgID to ID1

		msgID++;							// Bump msgID to match Mailbox
	} // end for (init TX mailbox area -- TX Only mailboxes)


/////////////////////////////////////////////////////////////////////////////////
// Mailboxes 0-7 are RX Only, They'll Receive From the TX Only Mailboxes 24-31 //
// Mailbox 0 Receives ID 0x200
	msgID = 0x200;
	for (mbID = 0; mbID<8; mbID++)
	{
		pCAN_Ptr = (unsigned short *)CAN1_MB00_DATA0 + (0x10 * mbID);

		*(pCAN_Ptr + 0)  = 0; 		 	// write msgID to DATA0
		*(pCAN_Ptr + 2)  = 0; 			// write msgID to DATA1
		*(pCAN_Ptr + 4)  = 0; 			// write msgID to DATA2
		*(pCAN_Ptr + 6)  = 0; 			// write msgID to DATA3
		*(pCAN_Ptr + 8)  = 8; 			// write 8 to LENGTH
		*(pCAN_Ptr + 10) = 0; 			// write 0 to TIMESTAMP
		*(pCAN_Ptr + 12) = 0; 			// write 0 to ID0
		*(pCAN_Ptr + 14) = (msgID << 2); // write msgID to ID1

		msgID ++;							// Bump msgID to match Mailbox
	} // end for (init RX mailbox area -- RX Only mailboxes)
	

	// enable the CAN and the mailboxes
	*pCAN0_MD1 = CAN_RX_MB_LO;
	*pCAN0_MD2 = CAN_RX_MB_HI;
	*pCAN1_MD1 = CAN_RX_MB_LO;
	*pCAN1_MD2 = CAN_RX_MB_HI;

	// Enable All Mailboxes
	*pCAN0_MC1 = CAN_EN_MB_LO; // mailboxes 0-7
	*pCAN0_MC2 = CAN_EN_MB_HI; // mailboxes 24-31
	*pCAN1_MC1 = CAN_EN_MB_LO; // mailboxes 0-7
	*pCAN1_MC2 = CAN_EN_MB_HI; // mailboxes 24-31
	ssync();

	// Exit CAN Configuration Mode (Clear CCR)
	*pCAN0_CONTROL = 0;
	*pCAN1_CONTROL = 0;

	// Wait for CAN Configuration Acknowledge (CCA)
	nTimer = SetTimeout(10000);
	if( ((unsigned int)-1) != nTimer )
	{
		do{
			asm("nop;");
		}while(  (*pCAN0_STATUS & CCA) && (*pCAN1_STATUS & CCA) && (!IsTimedout(nTimer))  );
	}

	ClearTimeout(nTimer);


	*pCAN0_MBIM1 = CAN_EN_MB_LO;	// Enable Interrupt for Mailboxes 00-8
	*pCAN0_MBIM2 = CAN_EN_MB_HI;	// Enable Interrupt for Mailboxes 24-31
	*pCAN1_MBIM1 = CAN_EN_MB_LO;	// Enable Interrupt for Mailboxes 00-8
	*pCAN1_MBIM2 = CAN_EN_MB_HI;	// Enable Interrupt for Mailboxes 24-31
	ssync();

	*pCAN0_UCCNF = UCE|UC_WDOG;	// enable Universal Counter in Watchdog mode
	*pCAN0_UCRC  = 0xABBA;		// unique value for timestamp checking
	*pCAN1_UCCNF = UCE|UC_WDOG;	// enable Universal Counter in Watchdog mode
	*pCAN1_UCRC  = 0xABBA;		// unique value for timestamp checking
} // End Init_Port ()


/*******************************************************************
*   Function:    Init_CAN_Interrupts
*   Description: initialize CAN interrupts
*******************************************************************/
int Init_CAN_Interrupts(void)
{
	
	*pILAT |= EVT_IVG11;							// clear pending IVG8 interrupts
	ssync();
	
	// Configure Interrupt Priorities
	if(g_CAN1_SEND)
	{
	    *pSIC_IMASK1 |= IRQ_CAN0_RX;
	}
	else
	{
		*pSIC_IMASK2 |= IRQ_CAN1_RX;
	}
	

	// Register Interrupt Handlers and Enable Core Interrupts
	register_handler(ik_ivg11, CAN_XMT_RCV_HANDLER);

	// Enable SIC Level Interrupts
	if(g_CAN1_SEND)
	{
	    *pSIC_IMASK2 |= IRQ_CAN1_TX;	
	}
	else
	{
	    *pSIC_IMASK1 |= IRQ_CAN0_TX;
	}	
	

	return 1;
}

/*******************************************************************
*   Function:    CAN_XMT_RCV_HANDLER
*   Description: transmit and receive interrupt handler
*******************************************************************/
EX_INTERRUPT_HANDLER(CAN_XMT_RCV_HANDLER)
{
	unsigned short mbim_status;
	unsigned short bit_pos = 0;
	char mbID;
	volatile unsigned short *pCAN_Ptr;
	unsigned short intr_status_tx;
	unsigned short intr_status_rx;
	
	if(g_CAN1_SEND)
	{
		intr_status_tx = *pCAN1_INTR;
		intr_status_rx = *pCAN0_INTR;
	}
	else
	{
		intr_status_tx = *pCAN0_INTR;
		intr_status_rx = *pCAN1_INTR;
	}
	
	if(intr_status_tx & 0x2)
	{
        //process trasmit first
    	g_TX_Count++;
 
    	if(g_CAN1_SEND)
    	{
    	    mbim_status = *pCAN1_MBTIF2;
    	}
    	else
    	{
    		mbim_status = *pCAN0_MBTIF2;
    	}
    	while (!(mbim_status & 0x8000))
    	{
    		mbim_status <<= 1;
    		bit_pos++;
    	}

    	mbID = (31 - bit_pos);

    	if(g_CAN1_SEND)
    	{
        	pCAN_Ptr = (unsigned short *)CAN1_MB00_DATA0 + (0x10 * mbID);

        	g_CAN_STAMP_BUF[g_rx_buf_index + g_tx_buf_index].zero = ((*pCAN_Ptr + 0x0E) >> 2);
        	g_CAN_STAMP_BUF[g_rx_buf_index + g_tx_buf_index].one = *pCAN1_UCCNT;
        	*pCAN1_MBTIF2 = (1 << (15 - bit_pos));
    	}
    	else
    	{
        	pCAN_Ptr = (unsigned short *)CAN0_MB00_DATA0 + (0x10 * mbID);

        	g_CAN_STAMP_BUF[g_rx_buf_index + g_tx_buf_index].zero = ((*pCAN_Ptr + 0x0E) >> 2);
        	g_CAN_STAMP_BUF[g_rx_buf_index + g_tx_buf_index].one = *pCAN0_UCCNT;
        	*pCAN0_MBTIF2 = (1 << (15 - bit_pos));
		
    	}
    	
    	ssync();

    	if(++g_tx_buf_index == BUF_SIZE)
    		g_tx_buf_index=0;
	}
	else if(intr_status_rx & 0x1)
	{
		// process receive now
    	g_RX_Count++;
	
    	if(g_CAN1_SEND)
    	{
    	    mbim_status = *pCAN0_MBRIF1;
    	}
    	else
    	{
    		mbim_status = *pCAN1_MBRIF1;
    	}
    	while (!(mbim_status & 0x8000))
    	{
    		mbim_status <<= 1;
    		bit_pos++;
    	}

    	mbID = (15 - bit_pos);

    	if(g_CAN1_SEND)
    	{
        	pCAN_Ptr = (unsigned short *)CAN0_MB00_DATA0 + (0x10 * mbID);
		
        	g_CAN_RX_BUFFER[g_rx_buf_index] = *(pCAN_Ptr + 6);
        	g_CAN_STAMP_BUF[g_rx_buf_index + g_tx_buf_index].zero = ((*pCAN_Ptr + 8) >> 2);
        	g_CAN_STAMP_BUF[g_rx_buf_index + g_tx_buf_index].one = bit_pos + 16;
        	*pCAN0_MBRIF1 = (1 << mbID);
    	}
    	else
    	{
        	pCAN_Ptr = (unsigned short *)CAN1_MB00_DATA0 + (0x10 * mbID);
        	g_CAN_RX_BUFFER[g_rx_buf_index] = *(pCAN_Ptr + 6);
        	g_CAN_STAMP_BUF[g_rx_buf_index + g_tx_buf_index].zero = ((*pCAN_Ptr + 8) >> 2);
        	g_CAN_STAMP_BUF[g_rx_buf_index + g_tx_buf_index].one = bit_pos + 16;
        	*pCAN1_MBRIF1 = (1 << mbID);
		
    	}
    	
    	ssync();
    			
    	if(++g_rx_buf_index == BUF_SIZE)
    		g_rx_buf_index=0;		
	}
	else
	{
		
	}

} // end CAN_XMT_RCV_HANDLER


/*******************************************************************
*   Function:    CAN_Transmit
*   Description: Sends out the information in all the TX mail boxes
*******************************************************************/
int CAN_Transmit(void)
{
	int nStatus = 0;
	unsigned int delay;
	unsigned int nTimer;

	nTimer = SetTimeout(0x1f0000);
	if( ((unsigned int)-1) != nTimer )
	{
		do{
			//// Set Transmit Requests for All TX Mailboxes
			if(g_CAN1_SEND)
			{
    			*pCAN1_TRS1 = 0;
    			*pCAN1_TRS2 = 0xFF00;
			}else
			{
    			*pCAN0_TRS1 = 0;
    			*pCAN0_TRS2 = 0xFF00;
				
			
			}
			ssync();

			if(g_TX_Count >= BUF_SIZE)
			{
				nStatus = 1;
				break;
			}
		}while( !IsTimedout(nTimer) );
	}
	ClearTimeout(nTimer);

	return nStatus;
} // End CAN_Transmit()

/*******************************************************************
*   Function:    Start_Transmit
*   Description: initializes global variables and starts transmit
*******************************************************************/
int Start_Transmit(void)
{
	int iStatus = 1;
	int nTimer;

	g_rx_buf_index = 0;
	g_tx_buf_index = 0;
	g_TX_Count = 0;
	g_RX_Count = 0;
	g_CAN_ERROR_FLAG = 0;	// flag that can error isr sets


	iStatus = CAN_Transmit(); 	// transmit all the TX mailboxes

	if( 1 == iStatus )
	{
		nTimer = SetTimeout(0x800);
		if( ((unsigned int)-1) != nTimer )
		{
			do{
				asm("nop;");
			}while( !IsTimedout(nTimer) );
		}
		ClearTimeout(nTimer);

	}

	if( *pPORTC & 0x11 )
	{	// an error occured in the CAN test
		iStatus = 0;
	}

	return iStatus;
}

/*******************************************************************
*   Function:    TEST_CAN
*   Description: Main test routine will get launched from the POST 
*				 framework.
*******************************************************************/
int TEST_CAN(void)
{
	int bPassed = 0;

	Init_Timers();
	Init_Timer_Interrupts();
	Init_CAN_Port();
	
	// CAN1 send, CAN0 receive
    g_CAN1_SEND=1;	
	Init_CAN_Interrupts();
    	
    bPassed = Start_Transmit();
    if(!bPassed)
        return bPassed; //failed
        

	// CAN0 send, CAN1 receive
    g_CAN1_SEND=0;
 
    Init_CAN_Interrupts();

    bPassed = Start_Transmit();
    if(!bPassed)
        return bPassed; //failed
		

	// free the memory allocated
	if( NULL != g_CAN_STAMP_BUF )
	{
		free(g_CAN_STAMP_BUF);
		g_CAN_STAMP_BUF = NULL;
	}

	if( NULL != g_CAN_RX_BUFFER )
	{
		free(g_CAN_RX_BUFFER);
		g_CAN_RX_BUFFER = NULL;
	}
	
	interrupt(ik_ivg11, SIG_IGN);

	// disable SIC Level Interrupts
	*pSIC_IMASK2 &= (~(IRQ_CAN1_TX | IRQ_CAN1_RX));
	ssync();
	
	*pSIC_IMASK1 &= (~(IRQ_CAN0_TX | IRQ_CAN0_RX));
	ssync();
		
	return bPassed;
}

/*******************************************************************
*   Function:    main
*   Description: used only if this test is needed as a standalone
*				 test separate from POST
*******************************************************************/
#ifdef _STANDALONE_ // use this to run standalone tests
main()
{
	
	int bPassed = 1;
	int count = 0;

	*pPLL_DIV = 0x3;
	ssync();

	while( bPassed )
	{
		bPassed = TEST_CAN();
		count++;
	}	


	asm("nop;");
	asm("emuexcpt;");
	asm("nop;");

} // end main

#endif // _STANDALONE_

