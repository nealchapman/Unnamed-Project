/*******************************************************************
Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software
you agree to the terms of the associated Analog Devices License Agreement.  

Project Name:  	Power_On_Self_Test

Hardware:		ADSP-BF548 EZ-KIT Lite

Description:	This file tests the DDR interface on the EZ-KIT.
*******************************************************************/

/*******************************************************************
*  include files
*******************************************************************/
#include <cdefBF548.h>
#include <ccblkfn.h>
#include <sys/exception.h>
#include <signal.h>
#include <stdlib.h>


/*******************************************************************
*  global variables and defines
*******************************************************************/
#define DDR_START  				0x00000000	// start address of DDR
#define BUFFER_SIZE 			512
#define nTransfers 				0x1fff
#define nStride 				4
#define nCoreWrites 			BUFFER_SIZE/nStride
#define WDSIZE_32 				0x0008
#define FLOW_1 					0x1000
#define WDSIZE_16 				0x0004
#define DMA_TRANSFER_LENGTH 	2
#define DMA_XCOUNT 				BUFFER_SIZE*2 //depends of transfer length


unsigned long *dest_buf;
unsigned int *src_buf = NULL;
unsigned int *dest_buf_verify = NULL;
unsigned long curr_ddr_address = 0;
int direction_flag = 0;
int dma_done = 0;
int change_pattern = 0;
int do_random = 0;
static unsigned long srand_val = 0xFFFFFFFF;
bool bFailedVerify = false;

int nNumDMAXfers = 0;

/*******************************************************************
*  function prototypes
*******************************************************************/
void Init_Interrupts(void);
void Enable_MDMA(void);
void Init_MDMA(void);
void Init_MDMA_reverse(void);
void Init_Interrupts_reverse(void);
void Init_DDR(void);
EX_INTERRUPT_HANDLER(MDMA_ISR);
EX_INTERRUPT_HANDLER(MDMA_ISR_reverse);

/*******************************************************************
*   Function:    Init_Interrupts
*   Description: initialize MDMA interrupts
*******************************************************************/
void Init_Interrupts(void)
{
	*pSIC_IMASK0 |= 0x600000;

	// Register Interrupt Handlers and Enable Core Interrupts
	register_handler(ik_ivg13, MDMA_ISR);

}

/*******************************************************************
*   Function:    Enable_MDMA
*   Description: enable MDMA
*******************************************************************/
void Enable_MDMA(void)
{
	*pMDMA_S0_CONFIG |= 0x1;

	*pMDMA_D0_CONFIG |= 0x1;
}

/*******************************************************************
*   Function:    Init_MDMA
*   Description: initialize DDR as destination
*******************************************************************/
void Init_MDMA(void)
{
    dest_buf = (unsigned long *) 0x0;

	*pMDMA_D0_CONFIG = WNR | WDSIZE_16 | DI_EN;

	*pMDMA_D0_START_ADDR = (void *)curr_ddr_address;

	*pMDMA_D0_X_COUNT = DMA_XCOUNT;

	*pMDMA_D0_X_MODIFY = DMA_TRANSFER_LENGTH;

	*pMDMA_S0_CONFIG = WDSIZE_16;

	*pMDMA_S0_START_ADDR = src_buf;

	*pMDMA_S0_X_COUNT = DMA_XCOUNT;

	*pMDMA_S0_X_MODIFY = DMA_TRANSFER_LENGTH;
}

/*******************************************************************
*   Function:    Init_MDMA_reverse
*   Description: initialize DDR as source
*******************************************************************/
void Init_MDMA_reverse(void)
{
	*pMDMA_D0_START_ADDR = (void *)dest_buf_verify;

	*pMDMA_S0_START_ADDR = 0;
}

/*******************************************************************
*   Function:    Init_Interrupts_reverse
*   Description: changes the ISR to do a read from DDR
*******************************************************************/
void Init_Interrupts_reverse(void)
{
	// Register Interrupt Handlers and Enable Core Interrupts
	register_handler(ik_ivg13, MDMA_ISR_reverse);
}

/*******************************************************************
*   Function:    Init_DDR
*   Description: initialize DDR registers
*******************************************************************/
void Init_DDR(void)
{
    // release the DDR controller from reset as per spec
    *pEBIU_RSTCTL |= 0x0001;
    ssync();

	/* setup DDR controller for MT46V32M16 -5B*/	
	*pEBIU_AMGCTL = 0x0009;
	*pEBIU_DDRCTL0 = 0x218A8411L;
	*pEBIU_DDRCTL1 = 0x20022222L;
	*pEBIU_DDRCTL2 = 0x00000021L;

    ssync();

}

/*******************************************************************
*   Function:    TEST_DDR
*   Description: Main test routine will get launched from the POST 
*				 framework.
*******************************************************************/
int TEST_DDR(void)
{
	volatile unsigned int *pSrc32;
	int nIndex = 0;
	int bSuccess = 1; 	// returning 1 indicates a pass, anything else is a fail
	int n;
	unsigned long ulData;
	
	// allocate storage for our buffers
	src_buf = malloc(sizeof(unsigned int) * BUFFER_SIZE);
	dest_buf_verify = malloc(sizeof(unsigned int) * BUFFER_SIZE);
	
	// make sure storage was allocated correctly
	if( (dest_buf_verify == NULL) || (src_buf == NULL) )
		return 0;
	
	// get a pointer to our source buffer	
	pSrc32 = (unsigned int *)src_buf;
	
	// enable asyn banks
	*pEBIU_AMGCTL = 0x000E;
	ssync();
	
	// reset some global regs
	dma_done = 0;
	nNumDMAXfers = 0;
	curr_ddr_address = 0;
	direction_flag = 0;
	
	srand(srand_val);
	
	srand_val++;
	
	// perform initialization
	Init_DDR();
	Init_Interrupts();
	Init_MDMA();

	// fill our src_buf with random data
	for(nIndex = 0; nIndex < BUFFER_SIZE; pSrc32++, nIndex++ )
	{
		*pSrc32 = rand();
		ssync();   	
	}

	// enable MDMA
	Enable_MDMA();

	// wait for the writes to DDR to finish
	while( !direction_flag && !bFailedVerify )
	{
		asm("nop;");
		asm("nop;");
		asm("nop;");
	}

	// if we failed cleanup
	if( bFailedVerify )
	{
		// free allocated memory
		free(src_buf);
		free(dest_buf_verify);
		
		// invalidate buffers
		src_buf = NULL;
		dest_buf_verify = NULL;
		
		return 0;
	}
	
	// now initialize to read DDR	
	Init_MDMA_reverse();
	Init_Interrupts_reverse();

	// reset globals
	nNumDMAXfers = 0;
	curr_ddr_address = 0;

	// enable MDMA for reads
	Enable_MDMA();

	// wait for DMA to finish
	while( !dma_done && !bFailedVerify )
	{
		asm("nop;");
		asm("nop;");
		asm("nop;");
	}
	
	interrupt(ik_ivg13, SIG_IGN);

	// disable SIC Level Interrupts
	*pSIC_IMASK0 &= (~(0x600000));
	ssync();
	
	// if we failed cleanup
	if( bFailedVerify )
	{
		// free allocated memory
		free(src_buf);
		free(dest_buf_verify);
		
		// invalidate them
		src_buf = NULL;
		dest_buf_verify = NULL;
		
		return 0;
	}

	// free allocated memory
	free(src_buf);
	free(dest_buf_verify);
	
	// invalidate them
	src_buf = NULL;
	dest_buf_verify = NULL;
	
	return bSuccess;
}

/*******************************************************************
*   Function:    MDMA_ISR
*   Description: MDMA ISR that performs writes to DDR
*******************************************************************/
EX_INTERRUPT_HANDLER(MDMA_ISR)
{
    volatile unsigned int *pDst32 = (unsigned int *)curr_ddr_address;
	int nIndex = 0;
		
	// acknowledge the DMA interrupt
	*pMDMA_D0_IRQ_STATUS |= 0x1;

	// increment the number of xfers so far
	nNumDMAXfers += 1;

	curr_ddr_address += 0x800;

	// update to the next address
	*pMDMA_D0_START_ADDR = (void *)curr_ddr_address;

	if( nNumDMAXfers < 0x8000 )
	{
	    while( nIndex < 0x200 )
	    {
	        unsigned long temp = pDst32[nIndex];
	        
	        // verify the DMA writes
	        if(temp != src_buf[nIndex])
	        {
	            bFailedVerify = true;
	            break;
	        }
	        
	        nIndex++;
	    }
	    
	    // enable DMA again
		*pMDMA_S0_CONFIG |= 0x1;

		*pMDMA_D0_CONFIG |= 0x1;
	}
	else
	{
		// disable DMA and set done flag
		*pMDMA_D0_CONFIG &= ~(0x1);

		*pMDMA_S0_CONFIG &= ~(0x1);

		direction_flag = 1;
	}

} // end CAN_RCV_HANDLER


/*******************************************************************
*   Function:    MDMA_ISR
*   Description: MDMA ISR that performs reads from DDR
*******************************************************************/
EX_INTERRUPT_HANDLER(MDMA_ISR_reverse)
{
    int nIndex = 0;
    
    // acknowledge the DMA interrupt
	*pMDMA_D0_IRQ_STATUS |= 0x1;

	// increment the number of xfers so far
	nNumDMAXfers += 1;

	curr_ddr_address += 0x800;

	// update to the next address
	*pMDMA_S0_START_ADDR = (void *)curr_ddr_address;

	if( nNumDMAXfers < 0x8000 )
	{
	    while( nIndex < 0x200 )
	    {
	    	// verify the DMA writes
	        if(dest_buf_verify[nIndex] != src_buf[nIndex])
	        {
	            bFailedVerify = true;
	            break;
	        }
	        
	        nIndex++;
	    }
	    
	    // enable DMA again
		*pMDMA_S0_CONFIG |= 0x1;

		*pMDMA_D0_CONFIG |= 0x1;
	}
	else
	{
		// disable DMA and set done flag
	    *pMDMA_D0_CONFIG &= ~(0x1);

		*pMDMA_S0_CONFIG &= ~(0x1);
		
		dma_done = 1;
	}

} // end CAN_RCV_HANDLER

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

	*pPLL_CTL = 0x2000;
	idle();
	
	*pPLL_DIV = 0x3;
	ssync();
	
	*pVR_CTL = 0x40FB;
	ssync();
	
	idle();

	while( bPassed )
	{
		bPassed = TEST_DDR();
		count++;	
	}

	return 0;
}
#endif //#ifdef _STANDALONE_
