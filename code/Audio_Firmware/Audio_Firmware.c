/*******************************************************************
*  include files
*******************************************************************/
#include <stdint.h>
#include <builtins.h>
#include <math_bf.h>
#include <math.h>
#include <blackfin.h>
#include <cycle_count.h>
#include "Modules/Oscillators/sincos.h"
#include "Engine/Codec/AD1980.h"

/*******************************************************************
*  global variables and defines
*******************************************************************/

volatile int  g_iSampleIndex = 1;

short *g_sInput;

short buttonTest = 0x0000;
short buttonState = 0x0000;
short buttonCount = 0x0000;

cycle_t cycle_start = 0x0000;
cycle_t cycle_stop = 0x0000;

#define DEBOUNCE_COUNT 0XFF
#define DELAY_COUNT 0x888
#define BUFFER_SIZE 1
#define SCLK 133 //in MHz

unsigned short offset = 0;
short sinOut[BUFFER_SIZE];

uint32_t delayCounter = 0;
uint16_t kCounter = 0;
uint16_t firstSend = 1;
uint16_t slot16Mode = 0;

volatile short delayFinished = 1;

/*********************************************************************************/
/***** Variables                                                             *****/
/***** The values in the array sCodecRegs can be modified to set up the      *****/
/***** codec in different configurations according to the AD1980 data sheet.*****/
/*********************************************************************************/
short z = 0;

/*Function Prototypes*/
static void initSPORT0(void);
static void initDMA(void);
static void enableSPORT0DMATDMStreams(void);
__attribute__((interrupt_handler))
static void sport0TXISR(void);
__attribute__((interrupt_handler))
static void sport0TXISRSetup(void);
static int pollButtons(void);
__attribute__((interrupt_handler))
static void delayTimerISR(void);
static void delayus(uint32_t);

/*--------------------*/
/*Function Definitions*/
/*--------------------*/
void initButtons(void)
{
	*pPORTB_FER &= ~0x0F00;
	*pPORTB_INEN = 0x0F00;
}

void initSPORT0(void)
{
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
}

void initDMA(void)
{
	*pDMA0_CONFIG = 0x0000;
	*pDMA1_CONFIG = 0x0000;
	*pDMA0_IRQ_STATUS = 0x0000;
	*pDMA1_IRQ_STATUS = 0x0000;

	*pDMA0_PERIPHERAL_MAP = PMAP_SPORT0RX;
	*pDMA1_PERIPHERAL_MAP = PMAP_SPORT0TX;

	*pDMA0_CONFIG = FLOW_AUTO | DI_EN | WDSIZE_16 | WNR;
	*pDMA0_START_ADDR = (void *)Rx0Buffer;
	*pDMA0_X_COUNT = 8;
	*pDMA0_X_MODIFY = 2;

	*pDMA1_CONFIG = FLOW_AUTO | DI_EN | WDSIZE_16;
	*pDMA1_START_ADDR = (void *)Tx0Buffer;
	*pDMA1_X_COUNT = 8;
	*pDMA1_X_MODIFY = 2;

	//MAGIC!!!
	delayus(2000);	

}

__attribute__((interrupt_handler))
static void delayTimerISR(void)
{
	*pTIMER_STATUS0	|= TIMIL0;
	delayFinished = 1;
}


__attribute__((interrupt_handler))
static void  sport0TXISRSetup(void)
{

	if(slot16Mode)
	{
		if(firstSend)
		{
			Tx0Buffer[COMMAND_ADDRESS_SLOT] = sCodecRegs[kCounter];
			firstSend = 0;
		}
		else
			Tx0Buffer[COMMAND_ADDRESS_SLOT] = (sCodecRegs[kCounter] | 0x8000);

		Tx0Buffer[COMMAND_DATA_SLOT] = sCodecRegs[kCounter+1];

		sCodecRegsReadBack[kCounter] = Rx0Buffer[STATUS_ADDRESS_SLOT];
		sCodecRegsReadBack[kCounter+1] = Rx0Buffer[STATUS_DATA_SLOT];

		if((sCodecRegsReadBack[kCounter]==sCodecRegs[kCounter]))
		{
			kCounter = kCounter + 2;
			firstSend = 1;
		}
			if(kCounter == SIZE_OF_CODEC_REGS)
		{
			Tx0Buffer[TAG_PHASE] = ENABLE_VFbit_SLOT1;
			Tx0Buffer[COMMAND_DATA_SLOT] = 0x0000;
		}
	}
	else
	{
		Tx0Buffer[TAG_PHASE] = ENABLE_VFbit_SLOT1_SLOT2;			// data into TX SLOT '0'
		Tx0Buffer[COMMAND_ADDRESS_SLOT] = SERIAL_CONFIGURATION;  	// data into TX SLOT '1'
		Tx0Buffer[COMMAND_DATA_SLOT] = 0x9000;  					// data into TX SLOT '2'
		if(Rx0Buffer[TAG_PHASE] & 0x8000);
			slot16Mode = 1;
	}

	if(kCounter >= SIZE_OF_CODEC_REGS)
	{
		*pEVT9 = (void *)sport0TXISR;	
	}

	*pDMA1_IRQ_STATUS = 0x0001;
	
}

void enableSPORT0DMATDMStreams(void)
{
	*pSPORT0_MCMC1 = 0x1000;
	*pSPORT0_MCMC2 = MFD_1 | MCMEN | MCDRXPE | MCDTXPE;
	*pSPORT0_MRCS0 = 0x00FF;
	*pSPORT0_MTCS0 = 0x00FF;

	/* Enable DMA0 for SPORT0 RX autobuffer transfers */
	*pDMA0_CONFIG	= (*pDMA0_CONFIG | 0x0001);

	/* Enable DMA1 for SPORT0 TX autobuffer transfers */
	*pDMA1_CONFIG	= (*pDMA1_CONFIG | 0x0001);

	/* Set SPORT0 TX (DMA1) interrupt priority to 2 = IVG9 */
	*pSIC_IAR1 = 0x33322221;

	/* Remap the vector table pointer from the default __I9HANDLER
	   to the new "Sport0_TX_ISR()" interrupt service routine */
	// SPORT0 TX ISR -> IVG 9
	*pEVT9 =  (void *)sport0TXISRSetup;		// SPORT0 TX ISR -> IVG 9

	/* Unmask peripheral SPORT0 RX interrupt in System Interrupt Mask Register `
	   (SIC_IMASK bit 9 - DMA1/SPORT0 RX */
	*pIMASK |= EVT_IVG9;
	*pSIC_IMASK0	= (*pSIC_IMASK0 | IRQ_DMA1);
	ssync();

	/* Enable TSPEN bit 0 to start SPORT0 TDM Mode Transmitter */
	*pSPORT0_TCR1 	= (*pSPORT0_TCR1 | TSPEN);
	ssync();

	/* Enable RSPEN bit 0 to start SPORT0 TDM Mode Receiver */
	*pSPORT0_RCR1 	= (*pSPORT0_RCR1 | RSPEN);
	ssync();
}


__attribute__((interrupt_handler))
static void sport0TXISR()
{

	START_CYCLE_COUNT(cycle_start);

	z = pollButtons();

	if(z & (1<<6))
		dphase = COMPUTE_DPHASE(scale[4],SAMPLE_RATE);
	else if(z & (1<<7))
		dphase = COMPUTE_DPHASE(scale[5],SAMPLE_RATE);
	else if(z & (1<<8))
		dphase = COMPUTE_DPHASE(scale[6],SAMPLE_RATE);
	else if(z & (1<<9))
		dphase = COMPUTE_DPHASE(scale[7],SAMPLE_RATE);
	else dphase = 0;
	// save new slot values in variables
	sAc97Tag 			= Rx0Buffer[TAG_PHASE];
	//fill_buffer(&Sin, &Cos, dphase, sinOut, 1);

	sincos_step(sinOut, &Cos, dphase);

	Tx0Buffer[TAG_PHASE] = sAc97Tag;
	Tx0Buffer[PCM_LEFT] = sinOut[0];
	Tx0Buffer[PCM_RIGHT] = Tx0Buffer[PCM_LEFT];

	STOP_CYCLE_COUNT(cycle_stop,cycle_start);
	
	*pDMA1_IRQ_STATUS = 0x0001;

}

int main(void)
{
	initButtons();
	initDelay();
	initLEDs();
	initSPORT0();
	audioReset();
	initDMA();
	enableSPORT0DMATDMStreams();
	while(1);
	return 0;
}
