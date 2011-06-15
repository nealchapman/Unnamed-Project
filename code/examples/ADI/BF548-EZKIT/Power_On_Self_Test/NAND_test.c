/*******************************************************************
Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software
you agree to the terms of the associated Analog Devices License Agreement.  

Project Name:  	Power_On_Self_Test

Hardware:		ADSP-BF548 EZ-KIT Lite

Description:	This file tests the NAND interface on the EZ-KIT
*******************************************************************/

/*******************************************************************
*  include files
*******************************************************************/
#include <cdefBF548.h>
#include <ccblkfn.h>
#include <sys\exception.h>
#include <signal.h>
#include "Timer_ISR.h"

#define LOOP_DELAY 0xFFFFFF

enum
{
	// read commands
	READ_CYCLE1 = 0x00,
	READ_CYCLE2 = 0x30,
	
	// page program commands
	PAGE_PROG_CYCLE1 = 0x80,
	PAGE_PROG_CYCLE2 = 0x10,
	
	// erase commands
	BLOCK_ERASE_CYCLE1 = 0x60,
	BLOCK_ERASE_CYCLE2 = 0xD0,
};
	
unsigned long *NAND_RX_DEST_BUF = NULL;
unsigned long *NAND_TX_SRC_BUF = NULL;

int done_tx = 0;
int nIRQstat_val = 0;

unsigned long nand_srand_val = 0xFFFFFFFF;

/*******************************************************************
*  function prototypes
*******************************************************************/
void Init_NAND_Interrupts(void);
void Config_DMA(unsigned long *buf_addr, unsigned short config_value);
void config_nfc_wr(void);
void config_nfc_erase(void);
void config_nfc_rd(void);
void rising_edge_detect(void);
void wr_buf_overflow(void);
void wr_buf_empty(void);
void rd_data_ready(void);
void wr_done(void);
EX_INTERRUPT_HANDLER(NFC_ISR);
EX_INTERRUPT_HANDLER(NAND_DMA_ISR);


/*******************************************************************
*   Function:    Init_NAND_Interrupts
*   Description: Initializes NAND interrupts
*******************************************************************/
void Init_NAND_Interrupts(void)
{	
	*pILAT |= EVT_IVG7;							// clear pending IVG8 interrupts
	ssync();
	
    // assign ISRs to interrupt vectors
	register_handler(ik_ivg7, NFC_ISR);
	register_handler(ik_ivg8, NAND_DMA_ISR);
	
    // unmask NFC Error Interrupt
	*pSIC_IMASK1 |= 0x10000000;

	*pPORTJ_FER = 0x6;
	ssync();

	*pPORTJ_MUX = 0x0;
	ssync();
}

/*******************************************************************
*   Function:    Config_DMA
*   Description: Initialize DMA22
*******************************************************************/
void Config_DMA(unsigned long *buf_addr, unsigned short config_value)
{
	// give the NFC control of the DMA channel
	// SDH and NFC share DMA channel 22
	*pDMAC1_PERIMUX = 0x0;

	*pDMA22_X_COUNT = 0x80;
	*pDMA22_X_MODIFY = 0x4;
	*pDMA22_START_ADDR = buf_addr;
	*pDMA22_CONFIG = config_value;

}

/*******************************************************************
*   Function:    config_nfc_wr
*   Description: sets NFC controller up for a write
*******************************************************************/
void config_nfc_wr(void)
{
	int i = 0;
	int nNumAddressCycles = 5;

	// extend AWE and ARE 3 SCLK cycles
	// page size is 512 Bytes
	// NAND data width is 8
	*pNFC_CTL = 0x233;

	// unmask nBUSY interrupt
	*pNFC_IRQMASK = 0x0;

	// reset ECC regs and NFC counters
	*pNFC_RST = 0x1;

	// write( cycle 1 )
	*pNFC_CMD = PAGE_PROG_CYCLE1;

	*pNFC_ADDR = 0x0;
	*pNFC_ADDR = 0x0;

	// wait until the write buf is not full anymore
	while( *pNFC_STAT & 0x2 )
	{
		asm("nop;");
	}

	*pNFC_ADDR = 0x0;
	*pNFC_ADDR = 0x0;
	*pNFC_ADDR = 0x0;

	for( i = 0; i < LOOP_DELAY; i++ )
	{
		asm("nop;");
	}

	
	// start page write
	*pNFC_PGCTL = 0x2;
}

/*******************************************************************
*   Function:    config_nfc_erase
*   Description: sets NFC controller up for a erase
*******************************************************************/
void config_nfc_erase(void)
{
	int i = 0;
	int nNumAddressCycles = 3;
	
	// extend AWE and ARE 3 SCLK cycles
	// page size is 512 Bytes
	// NAND data width is 8
	*pNFC_CTL = 0x233;

	// mask nBUSY interrupt
	*pNFC_IRQMASK = 0x1;

	// reset ECC regs and NFC counters
	*pNFC_RST = 0x1;

	// block erase( cycle 1 )
	*pNFC_CMD = BLOCK_ERASE_CYCLE1;

	*pNFC_ADDR = 0x0;
	*pNFC_ADDR = 0x0;

	// wait until the write buf is not full anymore
	while( *pNFC_STAT & 0x2 )
	{
		asm("nop;");
	}

	*pNFC_ADDR = 0x0;
	
	// block erase( cycle 2 )
	*pNFC_CMD = BLOCK_ERASE_CYCLE2;

	for( i = 0; i < LOOP_DELAY; i++ )
	{
		asm("nop;");
	}

	// start page write
	*pNFC_PGCTL = 0x2;
}


/*******************************************************************
*   Function:    config_nfc_erase
*   Description: sets NFC controller up for a read
*******************************************************************/
void config_nfc_rd(void)
{
	int i = 0;

	// extend AWE and ARE 3 SCLK cycles
	// page size is 512 Bytes
	// NAND data width is 8
	*pNFC_CTL = 0x233;

	// unmask nBUSY interrupt
	*pNFC_IRQMASK = 0x0;

	// reset ECC regs and NFC counters
	*pNFC_RST = 0x1;

	// read( cycle 1 )
	*pNFC_CMD = READ_CYCLE1;

	*pNFC_ADDR = 0x0;
	*pNFC_ADDR = 0x0;

	// wait until the write buf is not full anymore
	while( !(*pNFC_STAT & 0x1) )
	{
		asm("nop;");
	}

	*pNFC_ADDR = 0x0;
	*pNFC_ADDR = 0x0;
	*pNFC_ADDR = 0x0;

	// wait until the write buf is not full anymore
	while( !(*pNFC_STAT & 0x1) )
	{
		asm("nop;");
	}

	// read( cycle 2 )
	*pNFC_CMD = READ_CYCLE2;

	for( i = 0; i < LOOP_DELAY; i++ )
	{
		asm("nop;");
	}

	// start page read
	*pNFC_PGCTL = 0x1;
}

/*******************************************************************
*   Function:    rising_edge_detect
*   Description: does processing once rising edge of nBUSY is detected
*******************************************************************/
void rising_edge_detect(void)
{
	// flag that rising edge was detected
	done_tx = 1;

	// clear rising edge detect bit
	*pNFC_IRQSTAT |= 0x1;

	// Write buffer overflow
	if( nIRQstat_val & 0x2 )
  	{
  		wr_buf_overflow();
  	}
  	// Write Buffer Empty detected
  	else if( nIRQstat_val & 0x4 )
  	{
  		wr_buf_empty();
  	}
  	// Read data in NFC_READ
  	else if( nIRQstat_val & 0x8 )
  	{
  		rd_data_ready();
  	}
  	// Page write is complete
  	else if( nIRQstat_val & 0x10 )
  	{
  		wr_done();
  	}
}

/*******************************************************************
*   Function:    wr_buf_overflow
*   Description: does processing when write buffer overflows
*******************************************************************/
void wr_buf_overflow(void)
{
	// clear write buffer overflow bit
	*pNFC_IRQSTAT |= 0x2;

	// Write Buffer Empty detected
	if( nIRQstat_val & 0x4 )
  	{
  		wr_buf_empty();
  	}
  	// Read data in NFC_READ
  	else if( nIRQstat_val & 0x8 )
  	{
  		rd_data_ready();
  	}
  	// Page write is complete
  	else if( nIRQstat_val & 0x10 )
  	{
  		wr_done();
  	}
}

/*******************************************************************
*   Function:    wr_buf_empty
*   Description: does processing when write buffer is empty
*******************************************************************/
void wr_buf_empty(void)
{
	// clear write buffer empty bit
	*pNFC_IRQSTAT |= 0x4;

	// Read data in NFC_READ
	if( nIRQstat_val & 0x8 )
  	{
  		rd_data_ready();
  	}
  	// Page write is complete
  	else if( nIRQstat_val & 0x10 )
  	{
  		wr_done();
  	}
}

/*******************************************************************
*   Function:    rd_data_ready
*   Description: does processing when data in NFC_READ
*******************************************************************/
void rd_data_ready(void)
{
	// clear Read data bit
	*pNFC_IRQSTAT |= 0x8;

	// Page write is complete
	if( nIRQstat_val & 0x10 )
  	{
  		wr_done();
  	}
}

/*******************************************************************
*   Function:    wr_done
*   Description: does processing when page write is complete
*******************************************************************/
void wr_done(void)
{
	// make sure write buffer is empty
	while( !(*pNFC_IRQSTAT & 0x4) )
  	{
  		asm("nop;");
  	}

  	// page program( cycle 2 )
  	*pNFC_CMD = PAGE_PROG_CYCLE2;

  	// clear page write complete
  	*pNFC_IRQSTAT |= 0x10;
  	ssync();
}

/*******************************************************************
*   Function:    NAND_DMA_ISR
*   Description: DMA ISR
*******************************************************************/
EX_INTERRUPT_HANDLER(NAND_DMA_ISR)
{
  	asm("emuexcpt;");
  	asm("nop;");
  	asm("nop;");
}

/*******************************************************************
*   Function:    NFC_ISR
*   Description: Handles all NFC Interrupts
*******************************************************************/
EX_INTERRUPT_HANDLER(NFC_ISR)
{
	// get NFC Interrupt Status
  	nIRQstat_val = *pNFC_IRQSTAT;

  	// rising edge of nBUSY detected
  	if( nIRQstat_val & 0x1 )
  	{
  		rising_edge_detect();
  	}
  	// Write buffer overflow
  	else if( nIRQstat_val & 0x2 )
  	{
  		wr_buf_overflow();
  	}
	// Write Buffer Empty detected
  	else if( nIRQstat_val & 0x4 )
  	{
  		wr_buf_empty();
  	}
	// Read data in NFC_READ
  	else if( nIRQstat_val & 0x8 )
  	{
  		rd_data_ready();
  	}
  	// Page write is complete
  	else if( nIRQstat_val & 0x10 )
  	{
  		wr_done();
  	}
}

/*******************************************************************
*   Function:    TEST_NAND
*   Description: Main test routine will get launched from the POST 
*				 framework.
*******************************************************************/
int TEST_NAND(void)
{		
	int nIndex = 0;
	int bSuccess = 1; 	// returning 1 indicates a pass, anything else is a fail
	int n;
	int i = 0, j = 0x0;
	int temp = 0;
	unsigned int nTimer;
	
	// allocate storage for out buffers
	NAND_RX_DEST_BUF = malloc(sizeof(unsigned long) * 128);
	NAND_TX_SRC_BUF = malloc(sizeof(unsigned long) * 128);
	
	// make sure memory was allocated
	if( (NAND_TX_SRC_BUF == NULL) || (NAND_RX_DEST_BUF == NULL) )
		return 0;
		
	srand(nand_srand_val);

	nand_srand_val++;
	
	// fill our tx buffer
	for( i = 0; i < 128; i++ )
	{
		NAND_TX_SRC_BUF[i] = rand();
	}

	// initialize interrupts
	Init_Timers();
	Init_NAND_Interrupts();


	// make sure NBUSYIRQ is cleared
	temp = *pNFC_IRQSTAT;
	temp |= 0x1;
	*pNFC_IRQSTAT = temp;
	ssync();

	// setup buffer for the erase
	// this can be any buffer
	Config_DMA(NAND_TX_SRC_BUF, 0x89);

	// start the erase
	config_nfc_erase();
	ssync();


	nTimer = SetTimeout(0x80000);
	
	while( !done_tx && (!IsTimedout(nTimer)) )
	{
		asm("nop;");
	}

	ClearTimeout(nTimer);

	// check if rising edge of nBUSY was detected
	if( !done_tx )
	{
		// free allocated memory
		free(NAND_RX_DEST_BUF);
		free(NAND_TX_SRC_BUF);
		
		// invalidate buffers
		NAND_RX_DEST_BUF = NULL;
		NAND_TX_SRC_BUF = NULL;
		
		return 0;
	}
		
	// clear rising edge flag
	done_tx = 0;

	// make sure NBUSYIRQ is cleared
	temp = *pNFC_IRQSTAT;
	temp |= 0x1;
	*pNFC_IRQSTAT = temp;
	ssync();

	// stop DMA
	*pDMA22_CONFIG = 0;

	// configure DMA to point to our TX buffer
	Config_DMA(NAND_TX_SRC_BUF, 0x89);

	// start the write
	config_nfc_wr();
	ssync();

	nTimer = SetTimeout(0x80000);
	
	while( !done_tx && (!IsTimedout(nTimer)) )
	{
		asm("nop;");
	}

	ClearTimeout(nTimer);

	// check if rising edge of nBUSY was detected
	if( !done_tx )
	{
		// free allocated memory
		free(NAND_RX_DEST_BUF);
		free(NAND_TX_SRC_BUF);
		
		// invalidate buffers
		NAND_RX_DEST_BUF = NULL;
		NAND_TX_SRC_BUF = NULL;
		return 0;
	}
	
	// clear rising edge flag	
	done_tx = 0;

	// make sure NBUSYIRQ is cleared
	temp = *pNFC_IRQSTAT;
	temp |= 0x1;
	*pNFC_IRQSTAT = temp;
	ssync();

	// stop DMA
	*pDMA22_CONFIG = 0;

	// configure DMA with our RX buffer
	Config_DMA(NAND_RX_DEST_BUF, 0x8B);

	// start the read
	config_nfc_rd();
	ssync();

	nTimer = SetTimeout(0x80000);
	
	while( !done_tx && (!IsTimedout(nTimer)) )
	{
		asm("nop;");
	}

	ClearTimeout(nTimer);
	
	// check if rising edge of nBUSY was detected
	if( !done_tx )
	{
		// free allocated memory
		free(NAND_RX_DEST_BUF);
		free(NAND_TX_SRC_BUF);
		
		// invalidate buffers
		NAND_RX_DEST_BUF = NULL;
		NAND_TX_SRC_BUF = NULL;
		
		return 0;
	}
	
	// clear rising edge flag		
	done_tx = 0;

	// wait for the transfer to finish
	for( i = 0; i < LOOP_DELAY; i++ )
	{
			asm("nop;");
	}

	// see if our TX and RX buffer match
    for( i = 0; i < 128; i++ )
    {
        if( NAND_TX_SRC_BUF[i] != NAND_RX_DEST_BUF[i] )
        	bSuccess = 0;
    }

    interrupt(ik_ivg7, SIG_IGN);
    interrupt(ik_ivg8, SIG_IGN);

	// disable SIC Level Interrupts
	*pSIC_IMASK1 &= (~(0x10000000));

	// free allocated memory
	free(NAND_RX_DEST_BUF);
	free(NAND_TX_SRC_BUF);
	
	// invalidate buffers
	NAND_RX_DEST_BUF = NULL;
	NAND_TX_SRC_BUF = NULL;
		
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

	bPassed = TEST_NAND();
	
	if( bPassed == 0 )
	{
		printf("Test failed.\n");
	}
	else
		printf("Test passed.\n");

	return 0;
}
#endif //#ifdef _STANDALONE_
