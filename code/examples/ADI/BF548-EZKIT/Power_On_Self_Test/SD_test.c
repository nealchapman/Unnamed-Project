/*******************************************************************
Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software
you agree to the terms of the associated Analog Devices License Agreement.  

Project Name:  	Power_On_Self_Test

Hardware:		ADSP-BF548 EZ-KIT Lite

Description:	This file tests the SD interface on the EZ-KIT
*******************************************************************/

/*******************************************************************
*  include files
*******************************************************************/
#include <cdefBF548.h>
#include <ccblkfn.h>
#include <sys\exception.h>
#include <signal.h>
#include <stdlib.h>

#include "SD_test.h"
#include "Timer_ISR.h"


unsigned long srand_val = 0xFFFFFFFF;

#define SD_4BIT 1
//#define READ_ONLY	1
//#define DETECT_SD_CARD


// command table for the SD commands that are used in this example
SD_CMD SD_CmdTable[]=
{
 // CMD ID	CMD ARG			CMD Response			RCA needed?
	{ 0,	SD_STUFF_BITS,	SD_NO_RESPONSE,			SD_RCA_NOT_NEEDED 	},
	{ 2,	SD_STUFF_BITS,	CMD_L_RSP | CMD_RSP,	SD_RCA_NOT_NEEDED 	},
	{ 3,	SD_STUFF_BITS,	CMD_RSP,				SD_RCA_NOT_NEEDED 	},
	{ 7,	SD_STUFF_BITS,	CMD_RSP,				SD_RCA_NEEDED 		},
	{ 9,	SD_STUFF_BITS,	CMD_L_RSP | CMD_RSP,	SD_RCA_NEEDED 		},
	{ 13,	SD_STUFF_BITS,	CMD_RSP,				SD_RCA_NEEDED 		},
	{ 16,	SD_BLK_LEN_512,	CMD_RSP,				SD_RCA_NOT_NEEDED 	},
	{ 17,	SD_STUFF_BITS,	CMD_RSP,				SD_RCA_NOT_NEEDED 	},
	{ 24,	SD_STUFF_BITS,	CMD_RSP,				SD_RCA_NOT_NEEDED 	},
	{ 55,	SD_STUFF_BITS,	CMD_RSP,				SD_RCA_NEEDED 		},

	{ 6,	0x00000002,		CMD_RSP,				SD_RCA_NOT_NEEDED	},
	{ 13,	SD_STUFF_BITS,	CMD_RSP,				SD_RCA_NOT_NEEDED	},
	{ 41,	0x00FF8000,		CMD_RSP,				SD_RCA_NOT_NEEDED	},
    { 42,	SD_STUFF_BITS,	CMD_RSP,				SD_RCA_NOT_NEEDED	}
};


/*******************************************************************
*   Function:    Init_SD_Interrupts
*   Description: Initialize SD interrupts
*******************************************************************/
void Init_SD_Interrupts(void)
{
    // assign ISRs to interrupt vectors
	register_handler(ik_ivg11, SD_ISR);

	//enable DMA ISR
	register_handler(ik_ivg7, SD_DMA_ISR);

	*pILAT |= EVT_IVG7;							// clear pending IVG8 interrupts
	ssync();

	*pILAT |= EVT_IVG11;							// clear pending IVG8 interrupts
	ssync();

    // unmask SD mask 0
	*pSIC_IMASK2 |= 0x100;
	ssync();

	//unmask SD DMA
	*pSIC_IMASK2 |= 0x8;
	ssync();

	*pPORTC_FER = 0x3F00;
	*pPORTC_MUX = 0x00000000;
	ssync();



}

/*******************************************************************
*   Function:    Init_DMA
*   Description: Initialize DMA22
*******************************************************************/
void Init_DMA(unsigned long *buf_addr, unsigned short config_value)
{
	// give the SDH control of the DMA channel
	// SDH and NFC share DMA channel 22
    *pDMAC1_PERIMUX = 0x1;

	*pDMA22_START_ADDR = buf_addr;
    *pDMA22_X_COUNT = 128;
    *pDMA22_X_MODIFY = 4;
    *pDMA22_CONFIG = config_value;
    ssync();

}

/*******************************************************************
*   Function:    Init_SD
*   Description: Initialize SD registers
*******************************************************************/
void Init_SD(void)
{
    int temp;

    // disable SD card detection interrupt
    *pSDH_E_MASK = 0x0;
    ssync();

    // clear card detect bit
  	*pSDH_E_STATUS = SD_CARD_DET | SDIO_INT_DET;
   	ssync();

   	// Soft reset SDH
    *pSDH_CFG = SD_RST;
    ssync();

    // pull down on DATA3, pull up on DATA2-0, SDH clock enabled
    *pSDH_CFG |= ( CLKS_EN | PUP_SDDAT | PD_SDDAT3 );
    ssync();

    // power on
    *pSDH_PWR_CTL |= PWR_ON;
    ssync();

    // clock enable, SD_CLK should be 200kHz
    // for card initialization and Identification
    *pSDH_CLK_CTL |= ((CLK_E ) | 0xF9);
    ssync();

    // SD Int detected, SD card detected
    *pSDH_E_MASK |= (SCD_MSK | SDIO_MSK);
    ssync();
}

/*******************************************************************
*   Function:    SendCommand
*   Description: Sends a command to the card
*******************************************************************/
int SendCommand( SD_CMD *pSD_Cmd, bool bGetStatusReg )
{
    unsigned long ulStatusPoll = 0;
    int timeout = 0;
    unsigned long ulResponse = 0;

    // need to poll on either Response end
    // or CMD sent depending on whether we
    // are waiting for a response or not
    if( pSD_Cmd->CmdResponse & CMD_RSP )
		ulStatusPoll = CMD_RESP_END;
	else
		ulStatusPoll = CMD_SENT;


	// OR in the RCA if it is required in the argument
	if( pSD_Cmd->UseRCA )
	{
		// clear out the old RCA in case we
		// are doing the test more than 1 time
		pSD_Cmd->CmdArg &= 0xFFFF;
	    pSD_Cmd->CmdArg |= g_ulCardRCA;
	}

	// set the argument
    *pSDH_ARGUMENT = pSD_Cmd->CmdArg;
    ssync();

    // set the command, command response and enable it
	*pSDH_COMMAND = pSD_Cmd->CmdID | pSD_Cmd->CmdResponse | CMD_E;
	ssync();

	// poll to make sure command was sent and
	// a response was received if expected
	while( (!(*pSDH_STATUS & ulStatusPoll)) && (timeout < TIMEOUT_VAL) )
	{
	    timeout++;
	    asm("nop;");
	}

	// clear the Resp end or CMD sent bit
	*pSDH_STATUS_CLR |= ulStatusPoll;

	// if ACMD41, the CRC will fail so we need to clear it
	if( (pSD_Cmd->CmdID & 0x29) && (*pSDH_STATUS & CMD_CRC_FAIL) )
	{
	    *pSDH_STATUS_CLR = CMD_CRC_FAIL_STAT;
	}

	// clear the timeout status bit on a timeout
	if( timeout >= TIMEOUT_VAL )
		*pSDH_STATUS_CLR = CMD_TIMEOUT_STAT;

	if( bGetStatusReg )
	{
	    SendCommand( &SD_CmdTable[SD_CMD13], 0 );
	}

	return 1;
}

/*******************************************************************
*   Function:    SD_reset
*   Description: puts card back to Idle State
*******************************************************************/
int SD_reset( void )
{
    SendCommand( &SD_CmdTable[SD_CMD0], 0 );

    return 1;
}

/*******************************************************************
*   Function:    SD_ReadCardIdent
*   Description: get the CID numbers from the card
*******************************************************************/
int SD_ReadCardIdent(void)
{
	SendCommand( &SD_CmdTable[SD_CMD2], 0 );

	return 1;
}

/*******************************************************************
*   Function:    SD_ReadCardData
*   Description: get the CSD numbers from the card
*******************************************************************/
int SD_ReadCardData(void)
{
    unsigned long ulTemp = 0;
    SD_CSD_DATA CardData;

    SendCommand( &SD_CmdTable[SD_CMD9], 0 );

	CardData.ucMaxSpeed = (unsigned char)*pSDH_RESPONSE0;

	ulTemp = (unsigned long)*pSDH_RESPONSE1;
	CardData.ucMaxReadBlkLen = (unsigned char)( ( ulTemp >> 16 ) & 0xF );

	ulTemp <<= 2;
	ulTemp &= 0xFFF;
	ulTemp |= ( (unsigned long)*pSDH_RESPONSE2 >> 30 );
	CardData.usCardSize = (unsigned short)ulTemp;

	ulTemp = (unsigned long)*pSDH_RESPONSE2;
	ulTemp >>= 7;
	ulTemp &= 0x7F;
	CardData.usSectorSize = (unsigned short)ulTemp;

	ulTemp = (unsigned long)*pSDH_RESPONSE3;
	ulTemp >>= 22;
	CardData.ucMaxWriteBlkLen = ulTemp & 0xF;

	return 1;
}

/*******************************************************************
*   Function:    Set_4BitMode
*   Description: setup to allow 4 bit accesses
*******************************************************************/
int Set_4BitMode(void)
{
    // use WIDE BUS
    *pSDH_CLK_CTL |= WIDE_BUS;
    ssync();

 	SendCommand( &SD_CmdTable[SD_CMD55], 0 );
 	SendCommand( &SD_CmdTable[SD_ACMD42], 0 );

    // Set the bus width to 4 bit
    SendCommand( &SD_CmdTable[SD_CMD55], 0 );
 	SendCommand( &SD_CmdTable[SD_ACMD6], 0 );

    return 1;
}

/*******************************************************************
*   Function:    SD_DMA_ISR
*   Description: DMA ISR
*******************************************************************/
EX_INTERRUPT_HANDLER(SD_DMA_ISR)
{

    *pDMA22_IRQ_STATUS |= 0x1;
    ssync();
}

/*******************************************************************
*   Function:    SD_ISR
*   Description: SD ISR
*******************************************************************/
EX_INTERRUPT_HANDLER(SD_ISR)
{
    int temp = 0;
    g_nISR_Count++;

    // card detected
    if( (*pSDH_E_STATUS & SD_CARD_DET) )
    {
        g_bCardDetected = true;

        // clear card detected bit
        *pSDH_E_STATUS |= SD_CARD_DET;
    }
}

/*******************************************************************
*   Function:    TX_Block
*   Description: transmit a block of data
*******************************************************************/
int TX_Block(void)
{
    // set blk len to 512(SET_BLOCKLEN)
    SendCommand( &SD_CmdTable[SD_CMD16], 0 );

    // send write command(WRITE_SINGLE_BLOCK)
    SendCommand( &SD_CmdTable[SD_CMD24], 0 );

     // tx 512 bytes
    *pSDH_DATA_LGTH = 512;
    ssync();

    // set the timer
    *pSDH_DATA_TIMER = 0xFFFFFFFF;
    ssync();

    // 2^9 = 512 block length, enable write transfer
    *pSDH_DATA_CTL = 0x99;	//use DMA
    ssync();

    // wait until the data has been sent
	while( !(*pSDH_STATUS & DAT_END) )
	{
	    asm("nop;");
	}

	// clear the status bits
	*pSDH_STATUS_CLR = DAT_END_STAT | DAT_BLK_END;

	// stop transfer
	*pSDH_DATA_CTL = 0x0;
    ssync();



	return 1;
}

/*******************************************************************
*   Function:    RX_Block
*   Description: read a block of data
*******************************************************************/
int RX_Block(void)
{
    // set blk len to 512(SET_BLOCKLEN)
    SendCommand( &SD_CmdTable[SD_CMD16], 0 );

    // send read command(READ_SINGLE_BLOCK)
    SendCommand( &SD_CmdTable[SD_CMD17], 0 );

    // rx 512 bytes
    *pSDH_DATA_LGTH = 512;
    ssync();

    // set the timer
    *pSDH_DATA_TIMER = 0xFFFFFFFF;
    ssync();

    // 2^9 = 512 block length, enable read transfer
    *pSDH_DATA_CTL = 0x9b;	//use DMA
    ssync();

    // wait until the data has been sent
	while( !(*pSDH_STATUS & DAT_END) )
	{
	    asm("nop;");
	}

	Delay_Loop( 0xFFFFFF );

	// clear the status bits
	*pSDH_STATUS_CLR = DAT_END_STAT | DAT_BLK_END;

	// stop transfer
	*pSDH_DATA_CTL = 0x0;
    ssync();

	return 1;

}

/*******************************************************************
*   Function:    Delay_Loop
*   Description: delay for n cycles
*******************************************************************/
void Delay_Loop( int nDelay )
{
	int i = 0;

	for( ; i < nDelay; i++ )
	{
		asm("nop;");
	}
}

/*******************************************************************
*   Function:    TEST_SD
*   Description: Main test routine will get launched from the POST 
*				 framework.
*******************************************************************/
int TEST_SD(void)
{
	int nIndex = 0;
	int bSuccess = 1; 	// returning 1 indicates a pass, anything else is a fail
	int n;
	int i = 0, j = 0x0;
	unsigned int nTimer;
	short sClkVal = 0;

	SD_RX_DEST_BUF = malloc(sizeof(unsigned long) * 128);
	SD_TX_SRC_BUF = malloc(sizeof(unsigned long) * 128);
	
	if( (SD_RX_DEST_BUF == NULL) || (SD_TX_SRC_BUF == NULL) )
		return 0;
		
	srand(srand_val);

	srand_val++;

	// fill our tx buffer
	for( i = 0; i < 128; i++ )
	{
		SD_TX_SRC_BUF[i] = rand();
	}

	Init_Timers();			// initialize timers
	Init_SD_Interrupts();	// initialize interrupts
	Init_SD();				// initialize SD registers

#ifdef DETECT_SD_CARD
	// wait until the card is detected
	while( !g_bCardDetected )
    {
        asm("nop;");
    }
#endif

    //
    // start card identification mode
    //

	SD_reset();				// reset the card

	nTimer = SetTimeout(0x80000);

	do
	{

    	SendCommand( &SD_CmdTable[SD_CMD55], 0 );	// next command will be application specific
    	SendCommand( &SD_CmdTable[SD_ACMD41], 0 );	// sends HCS, gets OCR

	} while( !(*pSDH_RESPONSE0 & 0x80000000) && (!IsTimedout(nTimer)) );

	ClearTimeout(nTimer);


	if( !(*pSDH_RESPONSE0 & 0x80000000) )
	{
		// free allocated memory
		free(SD_RX_DEST_BUF);
		free(SD_TX_SRC_BUF);
		
		// invalidate buffers
		SD_RX_DEST_BUF = NULL;
		SD_TX_SRC_BUF = NULL;
		
		return 0;
	}


    SD_ReadCardIdent();		// get CID numbers

    SendCommand( &SD_CmdTable[SD_CMD3], 0 );	// ask the card to publish new RCA

    g_ulCardRCA = *pSDH_RESPONSE0 & 0xFFFF0000;

    SD_ReadCardData();		// get CSD

    //
    // start data transfer mode
    //

    // get clock going at ~22Mhz
    // SCLK is 133MHz
    sClkVal = *pSDH_CLK_CTL;
    sClkVal &= ~(0xFF);
    sClkVal |= 0x3;

    *pSDH_CLK_CTL = sClkVal;
    ssync();

	SendCommand( &SD_CmdTable[SD_CMD7], 0 );	// put the card in transfer state

#ifdef SD_4BIT
	Set_4BitMode();
#endif

	Init_DMA(SD_TX_SRC_BUF, 0x00a9);	//setup DMA

#ifndef READ_ONLY
    TX_Block();
#endif

	*pDMA22_CONFIG = 0x0;
	ssync();

	Init_DMA(SD_RX_DEST_BUF, 0x00ab);
    RX_Block();

    for( i = 0; i < 128; i++ )
    {
        if( SD_TX_SRC_BUF[i] != SD_RX_DEST_BUF[i] )
        	bSuccess = 0;
    }

    g_ulCardRCA = 0x0;

    interrupt(ik_ivg7, SIG_IGN);
    interrupt(ik_ivg11, SIG_IGN);

	// disable SIC Level Interrupts
	*pSIC_IMASK2 &= (~(0x108));
	ssync();

	// free allocated memory
	free(SD_RX_DEST_BUF);
	free(SD_TX_SRC_BUF);
	
	// invalidate buffers
	SD_RX_DEST_BUF = NULL;
	SD_TX_SRC_BUF = NULL;
	
	return bSuccess;
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

	*pPLL_CTL = SET_MSEL(16);	// (25MHz Xtal x (MSEL=16))::CCLK=400MHz
	idle();

	*pPLL_DIV = SET_SSEL(3);	// (400MHz/(SSEL=3))::SCLK=133MHz
	ssync();

	while( bPassed )
	{
		bPassed = TEST_SD();
		asm("nop;");
		asm("nop;");
		asm("nop;");
		asm("nop;");
	}


	return 0;
}
#endif //#ifdef _STANDALONE_
