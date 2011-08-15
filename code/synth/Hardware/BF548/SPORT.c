/* SPORT1 DMA receive buffer */
volatile short Rx0Buffer[8] = {0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000};

/* SPORT1 DMA transmit buffer */
volatile short Tx0Buffer[8] = {0xe000,0x7400,0x9900,0x0000,0x0000,0x0000,0x0000,0x0000};

// SPORT0 word length
#define SLEN_16	0x000f

#define FLOW_AUTO		0x1000		/* Autobuffer Mode							*/
#define MFD_1			0x1000		/* Multichannel Frame Delay = 1					*/
#define PMAP_SPORT0RX	0x0000		/* SPORT0 Receive DMA							*/
#define PMAP_SPORT0TX	0x1000		/* SPORT0 Transmit DMA						*/
#define WDSIZE_16		0x0004		/* Transfer Word Size = 16						*/

void enableSPORT0DMATDMStreams(void)
{
/*	*pSPORT0_MCMC1 = 0x1000;
	*pSPORT0_MCMC2 = MFD_1 | MCMEN | MCDRXPE | MCDTXPE;
	*pSPORT0_MRCS0 = 0x00FF;
	*pSPORT0_MTCS0 = 0x00FF;

	// Enable DMA0 for SPORT0 RX autobuffer transfers 
	*pDMA0_CONFIG	= (*pDMA0_CONFIG | 0x0001);

	// Enable DMA1 for SPORT0 TX autobuffer transfers
	*pDMA1_CONFIG	= (*pDMA1_CONFIG | 0x0001);

	// Set SPORT0 TX (DMA1) interrupt priority to 2 = IVG9
	*pSIC_IAR1 = 0x33322221;

	// Remap the vector table pointer from the default __I9HANDLER
	   to the new "Sport0_TX_ISR()" interrupt service routine 
	// SPORT0 TX ISR -> IVG 9
	*pEVT9 =  (void *)sport0TXISRSetup;		// SPORT0 TX ISR -> IVG 9

	// Unmask peripheral SPORT0 RX interrupt in System Interrupt Mask Register `
	   (SIC_IMASK bit 9 - DMA1/SPORT0 RX 
	*pIMASK |= EVT_IVG9;
	*pSIC_IMASK0	= (*pSIC_IMASK0 | IRQ_DMA1);
	ssync();

	// Enable TSPEN bit 0 to start SPORT0 TDM Mode Transmitter
	*pSPORT0_TCR1 	= (*pSPORT0_TCR1 | TSPEN);
	ssync();

	// Enable RSPEN bit 0 to start SPORT0 TDM Mode Receiver 
	*pSPORT0_RCR1 	= (*pSPORT0_RCR1 | RSPEN);
	ssync();
*/

}

void initSPORT0(void)
{
	
	/*

	//Sets port C to SPORT0
	*pPORTC_MUX = 0x0000;
	//Switches 4 PORTC pins to be SPORT0 instead of GPIO
	//Includes signals DT0PRI, RFS0, DR0PRI, RSCK0
	*pPORTC_FER = (1<<2) | (1<<4) | (1<<6) | (1<<7);
	//Set receive frame sync divider to 255	
	*pSPORT0_RFSDIV = 0x00FF;
	//Set Transmit frame sync divider to 0.
	//AD1980 generates this signal.
	*pSPORT0_TFSDIV = 0x0000;
	//Set Receive/Transmit clock divider to 0
	*pSPORT0_RCLKDIV = 0x0000;
	*pSPORT0_TCLKDIV = 0x0000;

	//Requires receive framesync on each word, Internal Frame Sync Select	
	*pSPORT0_RCR1 = RFSR | IRFS;
	//Serial recieve word length is SLEN_16+1
	*pSPORT0_RCR2 = SLEN_16;

	*pSPORT0_TCR1 = 0x0000;
	*pSPORT0_TCR2 = SLEN_16;

	*/

}
