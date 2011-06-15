/*******************************************************************
Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software
you agree to the terms of the associated Analog Devices License Agreement.  

Project Name:  	Power_On_Self_Test

Hardware:		ADSP-BF548 EZ-KIT Lite

Description:	This file tests the audio portion of the AV EZ-Extender
				board.
*******************************************************************/

/*******************************************************************
*  include files
*******************************************************************/
#include <sysreg.h>
#include <ccblkfn.h>
#include <signal.h>
#include <complex.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <filter.h>
#include <vector.h>

#include "post_common.h"
#include "Timer_ISR.h"


/*******************************************************************
*  global variables and defines
*******************************************************************/

// names for codec registers, used for sCodec1836TxRegs[]
#define DAC_CONTROL_1		0x0000
#define DAC_CONTROL_2		0x1000
#define DAC_VOLUME_0		0x2000
#define DAC_VOLUME_1		0x3000
#define DAC_VOLUME_2		0x4000
#define DAC_VOLUME_3		0x5000
#define DAC_VOLUME_4		0x6000
#define DAC_VOLUME_5		0x7000
#define ADC_0_PEAK_LEVEL	0x8000
#define ADC_1_PEAK_LEVEL	0x9000
#define ADC_2_PEAK_LEVEL	0xA000
#define ADC_3_PEAK_LEVEL	0xB000
#define ADC_CONTROL_1		0xC000
#define ADC_CONTROL_2		0xD000
#define ADC_CONTROL_3		0xE000


// names for slots in ad1836 audio frame
#define INTERNAL_ADC_L0			0
#define INTERNAL_ADC_L1			1
#define INTERNAL_ADC_R0			4
#define INTERNAL_ADC_R1			5
#define INTERNAL_DAC_L0			0
#define INTERNAL_DAC_L1			1
#define INTERNAL_DAC_L2			2
#define INTERNAL_DAC_R0			4
#define INTERNAL_DAC_R1			5
#define INTERNAL_DAC_R2			6


// size of array sCodec1836TxRegs
#define CODEC_1836_REGS_LENGTH	11

// SPI transfer mode
#define TIMOD_DMA_TX 0x0003

// SPORT0 word length
#define SLEN_24	0x0017

#define SLEN_16	0x000F

// DMA flow mode
#define FLOW_1	0x1000

#define WDSIZE_16	0x0004  /* Word Size 16 bits */
#define SLEN_32	0x001f

#define MAX_SAMPLES	 				256
#define REQUIRED_SAMPLES			((MAX_SAMPLES) * 250)
#define DESIRED_FREQ 				((float)3000.0)
#define SAMPLE_RATE 				((float)48000.0)
#define AMPLITUDE					((float)8388607)
#define PI							((float)3.141592765309)

#define ACCEPTABLE_DEVIATION_PCT	((float)0.015)
#define ACCEPTABLE_DEVIATION		(DESIRED_FREQ  * ACCEPTABLE_DEVIATION_PCT)
#define MAX_DESIRED_FREQ			(DESIRED_FREQ + ACCEPTABLE_DEVIATION)
#define MIN_DESIRED_FREQ			(DESIRED_FREQ - ACCEPTABLE_DEVIATION)
#define MIN_SIGNAL_STRENGTH			(float)35.0
#define MAX_NOISE_THRESHOLD			(float)10.0

#define MAX_ITER	3

int *g_iInput;		// buffer for the sine wave that is transmitted

volatile unsigned int g_ADC_Channel_PTR;
volatile unsigned int g_DAC_Channel_PTR;

const unsigned int g_ADC_LeftChannel[MAX_ITER] = {
	INTERNAL_ADC_L0,
	INTERNAL_ADC_L1,
	INTERNAL_ADC_L1 };

const unsigned int g_ADC_RightChannel[MAX_ITER] = {
	INTERNAL_ADC_R0,
	INTERNAL_ADC_R1,
	INTERNAL_ADC_R1 };

const unsigned int g_DAC_LeftChannel[MAX_ITER] = {
	INTERNAL_DAC_L0,
	INTERNAL_DAC_L1,
	INTERNAL_DAC_L2 };

const unsigned int g_DAC_RightChannel[MAX_ITER] = {
	INTERNAL_DAC_R0,
	INTERNAL_DAC_R1,
	INTERNAL_DAC_R2 };
	

volatile int iTxBuffer1[8];// SPORT0 DMA transmit buffer
volatile int iRxBuffer1[8];// SPORT0 DMA receive buffer

// left and right input data from ad1836
int iChannelLeftIn, iChannelRightIn;

// array for registers to configure the ad1836
// names are defined in "Talkthrough.h"
volatile short sCodec1836TxRegs[CODEC_1836_REGS_LENGTH] =
{
					DAC_CONTROL_1	| 0x000,
					DAC_CONTROL_2	| 0x000,
					DAC_VOLUME_0	| 0x3ff,
					DAC_VOLUME_1	| 0x3ff,
					DAC_VOLUME_2	| 0x3ff,
					DAC_VOLUME_3	| 0x3ff,
					DAC_VOLUME_4	| 0x3ff,    //0x000,
					DAC_VOLUME_5	| 0x3ff,    //0x000,
					ADC_CONTROL_1	| 0x000,
                    ADC_CONTROL_2	| 0x180,
					ADC_CONTROL_3	| 0x000

};

/*******************************************************************
*  function prototypes
*******************************************************************/
EX_INTERRUPT_HANDLER(Sport2_RX_ISR);
static void Init_Ports(void);
static void Init_1836(void);
static void Init_Sport(void);
static void Init_DMA(void);
static void Init_Interrupts(void);
static void Enable_DMA_Sport(void);
static void Disable_DMA_Sport(void);

/*******************************************************************
*   Function:    Init_Ports
*   Description: Configure Port registers needed to run this test
*******************************************************************/
void Init_Ports(void)
{
	*pPORTD_FER = 0;
	ssync();

    // set PD11 as output for codec reset
    *pPORTD_DIR_SET = Px11;


    // Enable SCK, MISO, MOSI, SPI0SEL3
	*pPORTE_FER = Px0 | Px1 | Px2 | Px6; // PE0,PE1,PE2,PE6, 0x0047;
	ssync();
}

/****************************************************************************
*   Function:    Init_1836
*   Description: This function sets up the SPI port to configure the AD1836.
*				 The content of the array sCodec1836TxRegs is sent to the
*				 codec.
******************************************************************************/
void Init_1836(void)
{
    int i;

    // clear PD11 to reset the AD1836
    *pPORTD_CLEAR 	= Px11;

    asm("nop; nop; nop;"); // assert reset for at least 5 ns

    // set PD11 to deassert reset of the AD1836
    *pPORTD_SET 	= Px11;

   	// wait to recover from reset
	for (i=0; i<200000; i++)
	{
	    asm("nop;");
	}

	// Enable SSEl3
	*pSPI0_FLG = FLS3;
	ssync();
	// Set baud rate SCK = HCLK/(2*SPIBAUD)
	*pSPI0_BAUD = 16;
	// configure spi port
	// SPI DMA write, 16-bit data, MSB first, SPI Master
	*pSPI0_CTL = TIMOD_DMA_TX | SIZE | MSTR ;

	// Configure DMA4 BF548 SPI
	// 16-bit transfers
	*pDMA4_CONFIG = WDSIZE_16;
	// Start address of data buffer
	*pDMA4_START_ADDR = (void*)sCodec1836TxRegs;
	// DMA inner loop count
	*pDMA4_X_COUNT = CODEC_1836_REGS_LENGTH;
	// Inner loop address increment
	*pDMA4_X_MODIFY = 2;
	// Enable DMA
	*pDMA4_CONFIG = (*pDMA7_CONFIG | DMAEN);
	// ENABLE SPI
	*pSPI0_CTL |= SPE;
	// wait until DMA transfers for spi are finished
	for(i=0; i<0xf000;i++)
	{
		asm("nop;");
	}

	// DISABLE SPI
	*pSPI0_CTL = 0x0000;


}

/****************************************************************************
*   Function:    Init_Sport
*   Description: Configure Sport2 for TDM mode, to transmit/receive data 
*				 to/from the ADC/DAC.Configure Sport for external clocks and
*				 frame syncs.
******************************************************************************/
void Init_Sport(void)
{
	// Sport2 receive configuration
	*pPORTA_FER = 0x00ff;
	ssync();

	// External CLK, External Frame sync, MSB first
	// 32-bit data
	*pSPORT2_RCR1 = RFSR;
	*pSPORT2_RCR2 = SLEN_32;
	// Sport2 transmit configuration
	// External CLK, External Frame sync, MSB first
	// 24-bit data
	*pSPORT2_TCR1 = TFSR;
	*pSPORT2_TCR2 = SLEN_32;
	// Enable MCM 8 transmit & receive channels
	*pSPORT2_MTCS0 = 0x000000FF;
	*pSPORT2_MRCS0 = 0x000000FF;
	// Set MCM configuration register and enable MCM mode
	*pSPORT2_MCMC1 = 0;
	*pSPORT2_MCMC2 = 0x101c;
}

/****************************************************************************
*   Function:    Init_DMA
*   Description: Initialize DMA18 in autobuffer mode to receive and DMA19 in
*				 autobuffer mode to transmit
******************************************************************************/
void Init_DMA(void)
{
	// Set up DMA18 to receive
	// 16-bit transfers, Interrupt on completion, Autobuffer mode
	*pDMA18_CONFIG = WNR | WDSIZE_16 | DI_EN | FLOW_1;
	// Start address of data buffer
	*pDMA18_START_ADDR = (void *)iRxBuffer1;
	// DMA inner loop count
	*pDMA18_X_COUNT = 16;
	// Inner loop address increment
	*pDMA18_X_MODIFY = 2;


	// Set up DMA19 to transmit
	// 16-bit transfers, Autobuffer mode
	*pDMA19_CONFIG = WDSIZE_16 | FLOW_1;
	// Start address of data buffer
	*pDMA19_START_ADDR = (void *)iTxBuffer1;
	// DMA inner loop count
	*pDMA19_X_COUNT = 16;
	// Inner loop address increment
	*pDMA19_X_MODIFY = 2;

}

/****************************************************************************
*   Function:    Enable_DMA_Sport
*   Description: Enable DMA18, DMA19, Sport2 TX and Sport2 RX
******************************************************************************/
void Enable_DMA_Sport(void)
{
	// Enable DMA18/19
	*pDMA19_CONFIG |=0x1;
	*pDMA18_CONFIG |= 0x1;

	// ENABLE SPORT2 TX/RX
	*pSPORT2_TCR1 |= 0x1;
	*pSPORT2_RCR1 |= 0x1;
}

/****************************************************************************
*   Function:    Disable_DMA_Sport
*   Description: Disable DMA18, DMA19, Sport2 TX and Sport2 RX
******************************************************************************/
void Disable_DMA_Sport(void)
{
	// Disable DMA
	*pDMA19_CONFIG &= ~0x1;
	*pDMA18_CONFIG &= ~0x1;

	// disable sports
	*pSPORT2_RCR1 &= ~0x1;
	*pSPORT2_TCR1 &= ~0x1;

}

/****************************************************************************
*   Function:    Init_Interrupts
*   Description: Initialize Interrupt for Sport2 RX
******************************************************************************/
void Init_Interrupts(void)
{
	// Unmask peripheral SPORT2 RX interrupt
	*pSIC_IMASK1 = (*pSIC_IMASK1 | IRQ_DMA18);
	ssync();

	// Remap the vector table pointer from the default __I9HANDLER
	// to the new _SPORT2_RX_ISR interrupt service routine
	register_handler(ik_ivg9, Sport2_RX_ISR);
}

/****************************************************************************
*   Function:    Sport2_RX_ISR
*   Description: This ISR is executed after a complete frame of input data
*				has been received. The new samples are stored in
*				iChannelLeftIn and iChannelRightIn.  Then the function
*				puts sine wave data into the DMA transmit buffer.
******************************************************************************/
EX_INTERRUPT_HANDLER(Sport2_RX_ISR)
{
	float fIn;

	// confirm interrupt handling
	*pDMA18_IRQ_STATUS = 0x0001;

	// copy input data from dma input buffer into variables
	iChannelRightIn = iRxBuffer1[g_ADC_RightChannel[g_ADC_Channel_PTR]];
	iChannelLeftIn = iRxBuffer1[g_ADC_LeftChannel[g_ADC_Channel_PTR]];

	g_fSineWaveIn_Right[g_iSampleIndex] = (short)(iChannelRightIn);
	g_fSineWaveIn_Left[g_iSampleIndex]  = (short)(iChannelLeftIn);
	
	// copy processed data from variables into dma output buffer
	iTxBuffer1[g_DAC_RightChannel[g_DAC_Channel_PTR]] = g_iInput[g_iIndex]>>8;
	iTxBuffer1[g_DAC_LeftChannel[g_DAC_Channel_PTR]] = g_iInput[g_iIndex++]>>8;
	
	// reset index
	if( g_iIndex == 256 )
		g_iIndex = 0;
		
	g_iSampleIndex++;	// only increment the index when both channels have been sent.

	if( g_iSampleIndex > MAX_SAMPLES-1 )
		g_iSampleIndex = 0;

	g_iSampleCount++;
}

/****************************************************************************
*   Function:    Test_Channel
*   Description: Takes a buffer of data and determines if the frequency
*				 and amplitude are within acceptable limits
******************************************************************************/
int Test_Channel(short* psRealIn)
{
	short nSampleNumber;
	short nHighestFreqIndex;
	float fSampledFrequency;
	int i = 0;

	// FFT magnitude (includes Nyquist)
	fract16 mag[(MAX_SAMPLES / 2) + 1];

	// twiddle factors
	complex_fract16 w[MAX_SAMPLES];

	complex_fract16 out[MAX_SAMPLES];
	fract16 in[MAX_SAMPLES];
	int BlockExponent;

	// create the input sinewave 3000 hz at 48000 sample rate amplitude is 24-bit's max
	for( nSampleNumber = 0; nSampleNumber < MAX_SAMPLES; nSampleNumber++ )
	{
		in[nSampleNumber] = (short)psRealIn[nSampleNumber];
	}

	// generate twiddle factors
	twidfftrad2_fr16(w, MAX_SAMPLES);

	// perform real fft
	rfft_fr16( in, out, w, 1, MAX_SAMPLES, &BlockExponent, 1 );


	// expect one of the index's of the array to contain a
	// 'spike' or high value such that frequency == index * (SAMPLE_RATE/MAX_SAMPLES) == 3000
	for( nSampleNumber = 0; nSampleNumber <= (MAX_SAMPLES / 2); nSampleNumber++ )
	{
		mag[nSampleNumber] = cabs_fr16(out[nSampleNumber]);
	}
	nHighestFreqIndex = vecmaxloc_fr16(mag,(MAX_SAMPLES / 2) + 1);

	// multiply the index of the array of the highest value with the sample rate value
	fSampledFrequency = nHighestFreqIndex * (SAMPLE_RATE / MAX_SAMPLES);

	// make sure frequency is within acceptable ranges
	if( (fSampledFrequency < MAX_DESIRED_FREQ) && (fSampledFrequency > MIN_DESIRED_FREQ) )
	{
			// check the signal strength
	    float fDB = 10 * log10( (float)mag[nHighestFreqIndex] );
	    if( fDB < MIN_SIGNAL_STRENGTH )
	    {
	    	return 0;	// test failed
	    }

	    float fSigStrength = 0.0;
	    for( i = 1; i < (MAX_SAMPLES / 2); i++)
	    {
	        fSigStrength = 10 * log10( (float)mag[i] );
	        if( (fSigStrength > MAX_NOISE_THRESHOLD) && (i != nHighestFreqIndex) )
	        	return 0; 	// test failed
	    }

		return 1;
	}

	return 0;	// test failed
}

/*******************************************************************
*   Function:    TEST_AV_AUDIO
*   Description: Main test routine will get launched from the POST 
*				 framework.
*******************************************************************/
int TEST_AV_AUDIO(void)
{
	int nResult = 1;
	int i = 0;
	int n = 0;

	g_iInput = malloc(sizeof(int) * MAX_SAMPLES);
	g_fSineWaveIn_Left = malloc(sizeof(short) * MAX_SAMPLES);
	g_fSineWaveIn_Right = malloc(sizeof(short) * MAX_SAMPLES);
	
	if( (g_iInput == NULL) || (g_fSineWaveIn_Left == NULL) || (g_fSineWaveIn_Right == NULL) )
		return 0;
		
	for( i = 0; i < MAX_SAMPLES; i++ )
	{
	    g_iInput[i] = (int)(AMPLITUDE * sin( (2.0 * PI * DESIRED_FREQ * ( ((float)(i+1)) / SAMPLE_RATE))) );
	}
	
	// initialize anything necessary to run the test
	Init_Timers();
	Init_Timer_Interrupts();

	Init_Ports();
	Init_1836();
	Init_Sport();
	Init_DMA();
	

	// test each channel
	for(n = 0; ( (n < MAX_ITER) && nResult ); n++)
	{
		g_iIndex = 0;
		g_iSampleCount = 0;
		g_iSampleIndex = 1;
	
		g_ADC_Channel_PTR = n;
		g_DAC_Channel_PTR = n;
		
		Init_Interrupts();
		Enable_DMA_Sport();
		
		unsigned int nTimer = SetTimeout(0x50000);
		if( ((unsigned int)-1) != nTimer )
		{
		    // once the required number of samples has been collected,
		    // process the signal.
			do{
				asm("nop;");
			}while( (g_iSampleCount < REQUIRED_SAMPLES)  && (!IsTimedout(nTimer)) );
		}

		ClearTimeout(nTimer);

	    // turn off interrupts so that the data is stable.
	    interrupt(ik_ivg9, SIG_IGN);
	    
	    // disable DMA and SPORT
		Disable_DMA_Sport();


    	nResult  = Test_Channel(g_fSineWaveIn_Right);

    	if( 1 == nResult  ) // Right channel was OK, test left channel
    	{
			nResult = Test_Channel(g_fSineWaveIn_Left);
    	}

    	

	}

	// free allocated memory
	free(g_iInput);
    free(g_fSineWaveIn_Left);
    free(g_fSineWaveIn_Right);
    
    // invalidate our buffers
    g_iInput = NULL;
    g_fSineWaveIn_Left = NULL;
    g_fSineWaveIn_Right = NULL;
    	
    return nResult;
}

/****************************************************************************
*   Function:    TEST_AV_AUDIO
*   Description: used to run standalone tests
******************************************************************************/
#ifdef _STANDALONE_ // use this to run standalone tests
void main(void)
{
	int n;

   	TEST_AV_AUDIO();
}
#endif //_STANDALONE_
