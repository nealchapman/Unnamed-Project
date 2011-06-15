/*******************************************************************
Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software
you agree to the terms of the associated Analog Devices License Agreement.  

Project Name:  	Power_On_Self_Test

Hardware:		ADSP-BF548 EZ-KIT Lite

Description:	This file tests the AD1980 codec on the BF548 EZ-KIT.
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

#define DELAY_COUNT 0x888

// names for AD1980 codec registers, used for iCodecRegs[]
/*			register				address	*/
/*          -------------		    ------- */
#define		RESET_REG				0x0000
#define		MASTER_VOLUME			0x0200
#define		HEADPHONE_VOLUME		0x0400
#define		MASTER_VOLUME_MONO		0x0600
#define		PHONE_VOLUME			0x0C00
#define		MIC_VOLUME				0x0E00
#define		LINE_IN_VOLUME			0x1000
#define		CD_VOLUME				0x1200
#define		AUX_VOLUME				0x1600
#define		PCM_OUT_VOLUME			0x1800
#define		RECORD_SELECT			0x1A00
#define		RECORD_GAIN			0x1C00
#define		GENERAL_PURPOSE			0x2000
#define		AUDIO_INTERRUPT_PAGING	0x2400
#define		POWERDOWN_CTRL_STAT		0x2600
#define		EXTEND_AUDIO_ID			0x2800
#define		EXTEND_AUDIO_CTL		0x2A00
#define		FRONT_DAC_SAMPLE_RATE		0x2C00
#define		SURR_DAC_SAMPLE_RATE		0x2E00
#define		C_LFE_DAC_SAMPLE_RATE		0x3000
#define		ADC_SAMPLE_RATE			0x3200
#define		CENTER_LFE_VOLUME		0x3600
#define		SURROUND_VOLUME			0x3800
#define		SPDIF_CONTROL			0x3A00
#define		EQ_CONTROL				0x6000
#define		EQ_DATA				0x6200
#define	 	JACK_SENSE				0x7200
#define		SERIAL_CONFIGURATION		0x7400
#define		MISC_CONTROL_BITS		0x7600
#define		VENDOR_ID_1			0x7C00
#define		VENDOR_ID_2			0x7E00

// names for slots in ac97 frame
#define SLOT_TAG			0
#define SLOT_COMMAND_ADDRESS	1
#define SLOT_COMMAND_DATA		2
#define SLOT_PCM_LEFT			3
#define SLOT_PCM_RIGHT		4

#define SIZE_OF_CODEC_REGS		60		// size of array iCodecRegs

/* Mask bit selections in Serial Configuration Register for
   accessing registers on any of the 3 codecs */
#define	MASTER_Reg_Mask			0x1000
#define	SLAVE1_Reg_Mask			0x2000
#define	SLAVE2_Reg_Mask			0x4000
#define	MASTER_SLAVE1			0x3000
#define	MASTER_SLAVE2			0x5000
#define	MASTER_SLAVE1_SLAVE2		0x7000

/* Macros for setting Bits 15, 14 and 13 in Slot 0 Tag Phase */
#define	ENABLE_VFbit_SLOT1_SLOT2	0xE000
#define	ENABLE_VFbit_SLOT1		0xC000
#define 	ENABLE_VFbit_STEREO		0x9800

/* AD1981B AC-Link TDM Timeslot Definitions (indexed by 16-bit short words)*/
#define	TAG_PHASE				0
#define	COMMAND_ADDRESS_SLOT		1
#define	COMMAND_DATA_SLOT		2
#define	STATUS_ADDRESS_SLOT		1
#define	STATUS_DATA_SLOT		2
#define	LEFT					3
#define	RIGHT					4
#define	PCM_LEFT				3
#define	PCM_RIGHT				4

/* Left and Right ADC valid bits used for testing of valid audio data in current TDM frame */
#define Left_ADC_Valid_Tag    12
#define Right_ADC_Valid_Tag   11


/* Useful AD1981B Control Definitions */
#define SEL_LINE_INPUTS 	0x0404	/* Line Level Input Source Select */
#define SEL_MIC_INPUT	 	0x0000  /* Microphone In Source Select */
#define Mic_Input_22dB		0x0F0F  /* 22.5 dB gain for microphone in  */
#define Mic_Input_15dB		0x0A0A  /* 15 dB gain for microphone in */
#define Line_LevelIn_0dB	0x0000	/* 0 dB gain for line in */
#define Line_LevelIn_12dB	0x0808  /* 12 dB gain for line in */
#define PCM_OUT_0dB 		0x0808  /* 0 dB gain for line out */
#define PCM_OUT_12dBgain 	0x0000	/* 12 dB gain for line out */
#define PCM_OUT_12dBminus	0x0F0F	/* -12 dB attenuation for line out */

// Selected ADC/DAC Sample Rates
#define Fs_CD_quality		 44100
#define Fs_Speech_quality	 8000
#define Fs_48kHz		 48000
#define Fs_Pro_Audio		 48000
#define Fs_Broadcast		 22000
#define Fs_oddball_1		 17897
#define Fs_oddball_2		 23456
#define Fs_oddball_3		 7893


// If you set this bit on register 76h (MISC_CONTROL_BITS) you enable split L/R mute controls on
// all volume registers. When set, vol bit 15 mutes the LEFT channel and bit 8 the RIGHT channel
#define MUTE_L				0x8000
#define _SPLIT_MUTE			0x2000
#define MUTE_R				0x0080
#define MUTE_LR				(MUTE_L | MUTE_R)


// If you set this bit on register 76h (MISC_CONTROL_BITS) you enable stereo microphone record.
// This bit allows recording a stereo microphone (array or otherwise). Microphone boost control
// and the microphone gain into the analog mixer affect both channels, however, the ADC selector
// gain can control each channel seperatly.

#define _STEREO_MIC			0x0040

// SPORT0 word length
#define SLEN_16	0x000f

#define FLOW_AUTO		0x1000		/* Autobuffer Mode							*/
#define MCMEN			0x0010 		/* Multichannel Frame Mode Enable				*/
#define MFD_1			0x1000		/* Multichannel Frame Delay = 1					*/
#define PMAP_SPORT0RX	0x0000			/* SPORT0 Receive DMA							*/
#define PMAP_SPORT0TX	0x1000			/* SPORT0 Transmit DMA						*/
#define WDSIZE_16		0x0004		/* Transfer Word Size = 16						*/

/* SPORT1 DMA receive buffer */
volatile short Rx0Buffer[8] = {0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000};

/* SPORT1 DMA transmit buffer */
volatile short Tx0Buffer[8] = {0xe000,0x7400,0x9900,0x0000,0x0000,0x0000,0x0000,0x0000};

short 	sAc97Tag = 0x8000;  // variable to save incoming AC97 tag in SPORT0 TX ISR

short 	sLeftChannelIn, sRightChannelIn;		// PCM input data
short 	sLeftChannelOut, sRightChannelOut;		// PCM output data


// Test paramaters
#define MAX_SAMPLES	 				256
#define REQUIRED_SAMPLES			((MAX_SAMPLES) * 250)
#define DESIRED_FREQ 				((float)3000.0)
#define SAMPLE_RATE 				((float)48000.0)
#define AMPLITUDE					((float)32767)
#define PI							((float)3.141592765309)

#define ACCEPTABLE_DEVIATION_PCT	((float)0.015)
#define ACCEPTABLE_DEVIATION		(DESIRED_FREQ  * ACCEPTABLE_DEVIATION_PCT)
#define MAX_DESIRED_FREQ			(DESIRED_FREQ + ACCEPTABLE_DEVIATION)
#define MIN_DESIRED_FREQ			(DESIRED_FREQ - ACCEPTABLE_DEVIATION)
#define MIN_SIGNAL_STRENGTH			(float)35.0
#define MAX_NOISE_THRESHOLD			(float)10.0

volatile int  g_iSampleIndex = 1;
volatile int  g_iSampleCount = 0;
volatile int  g_iIndex = 0;

short *g_fSineWaveIn_Left;
short *g_fSineWaveIn_Right;
short *g_sInput;

/*********************************************************************************/
/***** Variables                                                             *****/
/***** The values in the array sCodecRegs can be modified to set up the      *****/
/***** codec in different configurations according to the AD1980 data sheet.*****/
/*********************************************************************************/
short 	sCodecRegs[SIZE_OF_CODEC_REGS] =						// array for codec registers
{														// names are defined in "ad1980.h"
					SERIAL_CONFIGURATION,	0x9000,				// Set Slot 16 mode a second time to disable CHEN bit with correct 16-bit alignment
					RESET_REG,				0xABCD,			// Soft reset, Any write will do
					MASTER_VOLUME,			0x0000,			// Unmute Master Volume, 0dB
					HEADPHONE_VOLUME,		0x8000,			// Default, Muted Headphone Volume
					MASTER_VOLUME_MONO,		0x8000,			// Default, Muted, 0dB
					PHONE_VOLUME,			0x8008,			// Default, Muted, 0dB
					MIC_VOLUME,			0x8008,//0x001F,//0x8008, 			// Default Muted... NOTE:  This needs adjusted when MIC input selected !!!
					LINE_IN_VOLUME,			0x8F0F, 			// Unmuted with some gain added
					CD_VOLUME,				0x8808,			// Default, Muted, 0dB
					AUX_VOLUME,				0x8808,		// Default, Muted, 0dB
					PCM_OUT_VOLUME,			0x0000,			// Unmuted, 0dB
					RECORD_SELECT,			0x0404,//0x0000,//0x0404, 			// Selected Line In as Record Select... use 0x0000 for MIC input
					RECORD_GAIN,			0x0000, 			// Unmuted Record Gain, 0dB
					GENERAL_PURPOSE,		0x0000, 			// 0x0080,
					AUDIO_INTERRUPT_PAGING,	0x0000,
					EXTEND_AUDIO_ID,		0x0001,			// Enable VRA and DAC slot assignments 1, 2, 3 & 4
					EXTEND_AUDIO_CTL,		0x0000,			// Default
					FRONT_DAC_SAMPLE_RATE,	0xBB80, 			// 48 kHz default
					SURR_DAC_SAMPLE_RATE,	0xBB80, 				// 48 kHz default
					C_LFE_DAC_SAMPLE_RATE,	0xBB80, 			// 48 kHz default
					ADC_SAMPLE_RATE,		0xBB80, 			// 48 kHz default
					CENTER_LFE_VOLUME,		0x0000, 			// unmuted
					SURROUND_VOLUME,		0x0000,			// unmuted
					SPDIF_CONTROL,			0x2000, 			// default state
					EQ_CONTROL,				0x8080, 		// default, EQ muted
					EQ_DATA,				0x0000, 			// default
					MISC_CONTROL_BITS,		0x0040, 			// enable stereo MIC option
					JACK_SENSE,				0x0000,		// default
					VENDOR_ID_1,			0x4144,			// These are read-only, but the read-back will verify them
					VENDOR_ID_2,			0x5370
};

short 	sCodecRegsReadBack[SIZE_OF_CODEC_REGS];


/*******************************************************************
*  function prototypes
*******************************************************************/
static void Audio_Reset(void);
static void Enable_Codec_Slot16_Mode(void);
static void Init_AD1980_Codec(void);
static void Init_Sport0(void);
static void Init_DMA(void);
static void Enable_SPORT0_DMA_TDM_Streams(void);
EX_INTERRUPT_HANDLER(Sport0TxIsr_Dummy);
EX_INTERRUPT_HANDLER(Sport0_TX_ISR);
static void WaitForCodecInit(void);
static int Test_Channel(short* psRealIn);
int TEST_AUDIO(void);


/****************************************************************************
*   Function:    Audio_Reset
*   Description: This function Resets the ADC and DAC.
******************************************************************************/
void Audio_Reset(void)
{
    int iCounter;

    /* PB3 is connected to the codec reset pin (PBx Output) */
	/* Configure PE I/O pin 6 as output to reset AD1980 */

    /* Set AD1980 Reset pin (PB3) for GPIO mode */
    /* PB3 GP functionality should be enabled by default but just in case... */

	*pPORTB_FER = 0x0000;
	*pPORTB_MUX = 0x0000;


    /* Initialize Output PE6 Pin High and Configure PG[6:11] pins LEDs on*/
	*pPORTB_DIR_SET = Px3;
	*pPORTB_SET = Px3;

	/* Hold RESET high for 10 ms initially */
	/* we already set up PB3 as output, we will toggle PGx Port LEDs along with the codec reset */
	/* wait for (at least) 1 us */
	for(iCounter = 0x0000; iCounter < 0xFFFF; iCounter++) iCounter = iCounter;

	/* Now Reset AD1980 for approx minimum 1 ms per the AD1980 datasheet (we will do 10 ms for debug) */
	/* Assert AD1980 /RESET (active low) and turn-off PG[6:11] (LEDs 1-6) */
	/* Set Output PB3 Pin and PGx LED Pins OFF */
	*pPORTB_CLEAR = Px3;

	/* AD1980 /Reset Active low for 1 usec */
	/* wait for (at least) 1 us */
	for(iCounter = 0x0000; iCounter < 0xFFFF; iCounter++) iCounter = iCounter;

	/* De-assert AD1980 /RESET and turn-on PG[6:11] (LEDs 1-6)*/
	*pPORTB_SET = Px3;

	/* delay 10 msec to allow the Codec to recover from Reset */
	for(iCounter = 0x0000; iCounter < 0xffff; iCounter++) iCounter = iCounter;
}

/****************************************************************************
*   Function:    Enable_Codec_Slot16_Mode
*   Description: This function reprograms the codec after SPORT is enabled
*   			 for SLOT16 mode.
******************************************************************************/
void Enable_Codec_Slot16_Mode(void)
{
    int iCounter;
    unsigned short status;

	//  Note that after initial SLOT16 command by writing a 0x9900 to to
	//  Serial Configuration Register (addr 0x74) with initialized data
	//  in tx1_buf[ ] on startup, we sent data shifted by 4-bits due
	//  to AC-97 20-bit slots.  After the codec recongizes this command
	//  it reshifts register data and then puts the part in chained mode
	//  Since we are only a single codec configuration, rx1_buf[ ] initially
	//  shows all "0xFFFFs".  We need to re-program the Serial Config Register
	//  after Slot-16 mode is active to remove the "CHEN" bit by writing a
	//  value of "0x9000", this value written to codec addr 0x74 will
	//  maintain Slot-16 mode.

	/* Clear CHEN bit in AD1980 Serial Configuration Register (addr 0x74) */
	Tx0Buffer[TAG_PHASE] = ENABLE_VFbit_SLOT1_SLOT2;			// data into TX SLOT '0'
	Tx0Buffer[COMMAND_ADDRESS_SLOT] = SERIAL_CONFIGURATION;  	// data into TX SLOT '1'
	Tx0Buffer[COMMAND_DATA_SLOT] = 0x9000;  					// data into TX SLOT '2'

	idle(); ssync();
	idle(); ssync();

	/* wait a while */
	for(iCounter = 0x0000; iCounter < DELAY_COUNT; iCounter++) iCounter = iCounter;	// wait for (at least) 1 us

	do
	{
		status = Rx0Buffer[TAG_PHASE];
	}while( !(status & 0x8000) );
}

/****************************************************************************
*   Function:    Init_AD1980_Codec
*   Description: This function sends all values in the array sCodecRegs[]	
* 				 to the codec. Different configurations can	be set up by 
* 				 modifiying the values in this array.
******************************************************************************/
void Init_AD1980_Codec(void)
{
	int iCounter;
	int jCounter;
	bool bMatch = false;

	//wait for frame valid flag from codec (first bit in receive buffer)
	while((Tx0Buffer[TAG_PHASE] & 0x8000) == 0);

	// configure codec
	for(iCounter = 0; iCounter < SIZE_OF_CODEC_REGS; iCounter = iCounter + 2)
	{
		do
		{								// send complete register set to codec
			Tx0Buffer[COMMAND_ADDRESS_SLOT] = sCodecRegs[iCounter];
			Tx0Buffer[COMMAND_DATA_SLOT] = sCodecRegs[iCounter+1];

			for(jCounter = 0x0000; jCounter < DELAY_COUNT; jCounter++) jCounter= jCounter;

			idle(); ssync();
			idle(); ssync();	// ...wait for 2 TDM frames... 

			asm("nop;");asm("nop;");asm("nop;");asm("nop;");
			asm("nop;");asm("nop;");asm("nop;");asm("nop;");
			asm("nop;");asm("nop;");asm("nop;");asm("nop;");
			asm("nop;");asm("nop;");asm("nop;");asm("nop;");

			Tx0Buffer[COMMAND_ADDRESS_SLOT] = (sCodecRegs[iCounter] | 0x8000);
			idle(); ssync();
			idle(); ssync();

			for(jCounter = 0x0000; jCounter < DELAY_COUNT; jCounter++) jCounter= jCounter;

			sCodecRegsReadBack[iCounter] = Rx0Buffer[STATUS_ADDRESS_SLOT];
			sCodecRegsReadBack[iCounter+1] = Rx0Buffer[STATUS_DATA_SLOT];

			if( sCodecRegsReadBack[iCounter] == sCodecRegs[iCounter] )
				bMatch = true;

		}while( !bMatch );

		bMatch = false;
 	}


 	Tx0Buffer[TAG_PHASE] = ENABLE_VFbit_SLOT1;
 	Tx0Buffer[COMMAND_DATA_SLOT] = 0x0000;

 	for(iCounter = 0; iCounter < SIZE_OF_CODEC_REGS; iCounter = iCounter + 2)
	{
		Tx0Buffer[COMMAND_ADDRESS_SLOT] = (sCodecRegs[iCounter] | 0x8000);
		idle(); ssync();
		idle(); ssync();

		for(jCounter = 0x0000; jCounter < DELAY_COUNT; jCounter++) jCounter= jCounter;

		sCodecRegsReadBack[iCounter] = Rx0Buffer[STATUS_ADDRESS_SLOT];
		sCodecRegsReadBack[iCounter+1] = Rx0Buffer[STATUS_DATA_SLOT];
	}
}


/****************************************************************************
*   Function:    Init_Sport0
*   Description: Configure SPORT0 for TDM mode, to transmit/receive data
*                to/from the AD1980. Configure Sport for ext clock and
*                internal frame sync.
******************************************************************************/
void Init_Sport0(void)
{
    /* BF548 EZ-KIT Lite connects SPORT0 PortC pins to AD1980 */
    /* PC2 = DT0PRI */
    /* PC3 = TSCLK0 */
    /* PC4 = RFS0   */
    /* PC6 = DR0PRI */
    /* PC7 = RSCLK0 */

    /* NOTE: TSCLK0 is connected on EZ-KIT but is not required for Multichannel operation ! */

	/* Configure SPORT0 Primary Path in Port Multiplexer Control Register */
	/* SPORT0 pins should be enabled by default but just in case... */

	*pPORTC_MUX = 0x0000; 	/* MUX[1:0] for PC0:PC7 = '00', for bits 0:14 in PORTC_MUX */

    /* Enable SPORT0 pins in PORC Function Enable Register */
	*pPORTC_FER = Px2 | Px4 | Px6 | Px7;

	/* Reset SPORT0 Registers & DMA -
	   register clear reg instructions can be removed after debug */
	*pSPORT0_TCR1 = 0x0000;
	*pSPORT0_TCR2 = 0x0000;
	*pSPORT0_RCR1 = 0x0000;
	*pSPORT0_RCR1 = 0x0000;
	*pSPORT0_MCMC1 = 0x0000;


	/*****************************************************/
	/* AC-97 Frame consists of 256 SCLKs/per frame       */
	/* Generate a 48 kHz AC-97 Frame Rate                */
	/* SCLK = 12.288 MHz                                 */
	/* SPORT0 RFS Frequency = SCLK / (RFSDIV + 1)        */
	/*                      = 12.288 MHz / 256           */
	/*                      = 48 KHz AC TDM Frame Rate   */
	/* RFSDIV = 255                                      */
	/*****************************************************/

	/* Set SPORT0 receive frame sync divisor register */
	*pSPORT0_RFSDIV = 0x00FF;  				/* AC'97 48Khz Frame Sync with 12.288Mhz Input Clock */

	/* SPORT0 receive clock divider register */
	/* RCLK is externally generated 12.288 MHz SCLK from AD1980 */
	*pSPORT0_RCLKDIV = 0x0000;				/* AC'97 12.288Mhz Serial Clock */

	/* SPORT0 transmit frame sync divisor, clear reg since externally generated TFS */
	*pSPORT0_TFSDIV = 0x0000;

	/* SPORT0 transmit clock divider register */
	/* TCLK is externally generated 12.288 MHz SCLK from AD1980 */
	*pSPORT0_TCLKDIV = 0x0000;				/* AC'97 12.288Mhz Serial Clock */


	/*****************************************************/
	/* Set the SPORT0 Receive Configuration Registers    */
	/* ----------------------------------------------    */
	/* Early Frame Sync, LARFS = 0                       */
	/* Active High RFS, LRFS = 0                         */
	/* Receive Frame Sync Required, RFSR = 1             */
	/* Internally Generated RFS, IRFS = 1                */
	/* Receive MSB First, RLSBIT = 0,                    */
	/* Zero Fill SPORT data, RDTYPE = 00                 */
	/* Externally Generated Serial Clock, IRCLK = 0      */
	/* Keep SPORT RX Disabled for now, RSPEN = 0         */
	/* Serial Word Length is 16-bits/word, SLEN = 15     */
	/* Receive Secondary Channel Disabled, RXSE = 0      */
	/* Left/Right Order not used, RRFST = 0              */
	/* Frame Sync in Normal Pulse Mode, RSFSE = 0        */
	/*****************************************************/

	*pSPORT0_RCR1 = RFSR | IRFS;
	*pSPORT0_RCR2 = SLEN_16;


	/*****************************************************/
	/* Set the SPORT0 Transmit Configuration Registers   */
	/* -----------------------------------------------   */
	/* Early Frame Sync, LATFS = 0                       */
	/* Active High TFS, LTFS = 0                         */
	/* Transmit Frame Sync Not Required, TFSR = 0        */
	/* Externally Generated TFS, ITFS = 0                */
	/* Transmit MSB First, TLSBIT = 0,                   */
	/* Normal SPORT tx data format, TDTYPE = 00          */
	/* Externally Generated Serial Clock, ITCLK = 0      */
	/* Keep SPORT TX Disabled for now, TSPEN = 0         */
	/* Serial Word Length is 16-bits/word, SLEN = 15     */
	/* Transmit Secondary Channel Disabled, TXSE = 0     */
	/* Left/Right Order not used, TRFST = 0              */
	/* Frame Sync in Normal Pulse Mode, TSFSE = 0        */
	/*****************************************************/

	*pSPORT0_TCR1 = 0x0000;
	*pSPORT0_TCR2 = SLEN_16;
}

/****************************************************************************
*   Function:    Init_DMA
*   Description: DMA Controller Programming For SPORT0
*                Sets up DMA0 and DMA1 in autobuffer mode to receive and 
*				 transmit SPORT data
******************************************************************************/
void Init_DMA(void)
{
	/* Disable SPORT0 RX and TX DMA */
	*pDMA0_CONFIG = 0x0000;
	*pDMA1_CONFIG = 0x0000;
	*pDMA0_IRQ_STATUS = 0x0000;
	*pDMA1_IRQ_STATUS = 0x0000;

	/* Set up DMA0 to receive, map DMA0 to SPORT0 RX */
	*pDMA0_PERIPHERAL_MAP = PMAP_SPORT0RX;		/* PMAP[3:0] = 3 for SPORT0 Receive DMA, CTYPE = 0 */
												/* DMA0_PERIPHERAL_MAP = 0x3000 */

	/* Set up DMA1 to transmit, map DMA1 to SPORT0 TX */
	*pDMA1_PERIPHERAL_MAP = PMAP_SPORT0TX;		/* PMAP[3:0] = 4 for SPORT0 Transmit DMA, CTYPE = 0 */
												/* DMA1_PERIPHERAL_MAP = 0x4000 */

	/*****************************************************/
	/* Set up the DMA0 Configuration Register            */
	/* ------------------------------------------------- */
	/* Autobuffer Mode, FLOW[2:0] = 001                  */
	/* Flex Descriptor Size, NDSIZE[3:0] = 0000          */
	/* Data Interrupt Enable, DI_EN = 1                  */
	/* Data Interrupt Timing Select, DI_SEL = 0          */
	/* Retain DMA FIFO Buffer Data, RESTART = 0          */
	/* Two-Dimensional DMA Disabled, DMA2D = 0           */
	/* Transfer Word Size, WDSIZE[1:0] = 01              */
	/* DMA direction is a memory write/dest, WNR = 1     */
	/* DMA is initially disabled, DMA_EN = 0             */
	/*****************************************************/

	*pDMA0_CONFIG = FLOW_AUTO | DI_EN | WDSIZE_16 | WNR;

	/* Start address of DMA0 data buffer */
	*pDMA0_START_ADDR = (void *)Rx0Buffer;

	/* DMA0 inner loop count */
	*pDMA0_X_COUNT = 8;

	/* Inner loop address increment, stride shown in byte increments */
	*pDMA0_X_MODIFY	= 2;


	/*****************************************************/
	/* Set up the DMA1 Configuration Register            */
	/* ------------------------------------------------- */
	/* Autobuffer Mode, FLOW[2:0] = 001                  */
	/* Flex Descriptor Size, NDSIZE[3:0] = 0000          */
	/* Data Interrupt Enable, DI_EN = 1                  */
	/* Data Interrupt Timing Select, DI_SEL = 0          */
	/* Retain DMA FIFO Buffer Data, RESTART = 0          */
	/* Two-Dimensional DMA Disabled, DMA2D = 0           */
	/* Transfer Word Size, WDSIZE[1:0] = 01              */
	/* DMA direction is a memory read/source, WNR = 0    */
	/* DMA is initially disabled, DMA_EN = 0             */
	/*****************************************************/

	*pDMA1_CONFIG = FLOW_AUTO | DI_EN | WDSIZE_16;

	/* Start address of DMA1 data buffer */
	*pDMA1_START_ADDR = (void *)Tx0Buffer;

	/* DMA1 inner loop count */
	*pDMA1_X_COUNT = 8;

	/* Inner loop address increment, stride shown in byte increments */
	*pDMA1_X_MODIFY	= 2;
}

/****************************************************************************
*   Function:    Enable_SPORT0_DMA_TDM_Streams
*   Description: Enable SPORT0 TX/RX in TDM Mode of Operation and Enable 
*				 DMA0 and DMA1
******************************************************************************/
void Enable_SPORT0_DMA_TDM_Streams(void)
{
	/* Enable MCM Transmit and Receive Timeslot/Channels */

	/*****************************************************/
	/* Set SPORT0 Multichannel Configuration Registers   */
	/* -----------------------------------------------   */
	/* Window Size:                                      */
	/* WSIZE - ( (16 Channels/8) - 1) = 1                */
	/*                                                   */
	/* Window Offset:                                    */
	/* WOFF = 0;                                         */
	/*                                                   */
	/* Multichannel Frame Delay, MFD = 1                 */
	/* Normal Frame Sync to Data Relationship, FSDR = 0  */
	/* Multchanannel DMA TX * RX Packing Enabled:        */
	/* MCDRXPE = 1 & MCDTXPE = 1                         */
	/* Bypass Mode, MCCRM[1:0] = 00                      */
	/* Multichannel Fram Mode Enabled, MCMEN = 1         */
	/*****************************************************/

	*pSPORT0_MCMC1 = 0x1000;
	*pSPORT0_MCMC2 = MFD_1 | MCMEN | MCDRXPE | MCDTXPE;


	/*****************************************************/
	/* Set the SPORT0 Multichannel Receive Select Regs   */
	/* -----------------------------------------------   */
	/* MRCS0 = 0000 0000 0001 1111                       */
	/*                                                   */
	/* Single Configuration AD1980 has 5 active channels */
	/* Slot 0 = TAG phase                                */
	/* Slot 1 = Status Address                           */
	/* Slot 2 = Status Data                              */
	/* Slot 3 = Left Channel/Mic 1                       */
	/* Slot 4 = Right Channel/Mic 2                      */
	/*****************************************************/
	/*
		Timeslot #1-5
		+------------------+
		| (1) Valid Rx Tag |
		+------------------+
		| (2) Status Addr  |
		+------------------+
		| (3) Status Data  |
		+------------------+
		| (4) L ADC Channel|
		+------------------+
		| (5) R ADC Channel|
		+------------------+
	*/

	*pSPORT0_MRCS0 = 0x00FF;


	/*****************************************************/
  	/* Set the SPORT0 Multichannel Transmit Select Regs  */
	/* MTCS0 = 0000 0000 0001 1111                       */
	/*                                                   */
	/* Single Configuration AD1980 has 5 active channels */
	/* Slot 0 = TAG phase                                */
	/* Slot 1 = Command Address                          */
	/* Slot 2 = Command Data                             */
	/* Slot 3 = Left Channel/Mic 1                       */
	/* Slot 4 = Right Channel/Mic 2                      */
	/*****************************************************/
	/*
		Timeslot #1-5
		+------------------+
		| (1) Valid Tx Tag |
		+------------------+
		| (2) Command Addr |
		+------------------+
		| (3) Command Data |
		+------------------+
		| (4) L DAC Channel|
		+------------------+
		| (5) R DAC Channel|
		+------------------+
	**/

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
	register_handler(ik_ivg9, Sport0TxIsr_Dummy);		// SPORT0 TX ISR -> IVG 9

	/* Unmask peripheral SPORT0 RX interrupt in System Interrupt Mask Register `
	   (SIC_IMASK bit 9 - DMA1/SPORT0 RX */
	*pSIC_IMASK0	= (*pSIC_IMASK0 | IRQ_DMA1);
	ssync();

	/* Enable TSPEN bit 0 to start SPORT0 TDM Mode Transmitter */
	*pSPORT0_TCR1 	= (*pSPORT0_TCR1 | TSPEN);
	ssync();

	/* Enable RSPEN bit 0 to start SPORT0 TDM Mode Receiver */
	*pSPORT0_RCR1 	= (*pSPORT0_RCR1 | RSPEN);
	ssync();
}

/****************************************************************************
*   Function:    Enable_SPORT0_DMA_TDM_Streams
*   Description: This ISR makes a call to an empty function and is only 
*				 needed to skip idle instructions during codec initialization.
******************************************************************************/
EX_INTERRUPT_HANDLER(Sport0TxIsr_Dummy)
{
	// confirm interrupt handling
	*pDMA1_IRQ_STATUS = 0x0001;

	WaitForCodecInit();
}

/****************************************************************************
*   Function:    WaitForCodecInit
*   Description: This function is called from within the SPORT0 TX ISR and 
*				 is used as a dummy function for the duration of the codec 
*				 initialisation.
******************************************************************************/
void WaitForCodecInit(void)
{
}


/****************************************************************************
*   Function:    Sport0_TX_ISR
*   Description: This ISR is executed after a complete frame of a complete audio
*				 frame has been received. The PCM input samples can be found
*				 in sLeftChannelIn and sRightChannelIn respectively. The processed
*				 output data should be written into the variables sLeftChannelOut
*				 and sRightChannelOut respectively.
******************************************************************************/
EX_INTERRUPT_HANDLER(Sport0_TX_ISR)
{
	// mask interrupts so we can finish all processing
	unsigned int uiTIMASK = cli();

	// confirm interrupt handling
	*pDMA1_IRQ_STATUS = 0x0001;

	short sChannelX;

	// save new slot values in variables
	sAc97Tag 			= Rx0Buffer[TAG_PHASE];
	sLeftChannelIn 		= Rx0Buffer[PCM_LEFT];
	sRightChannelIn 	= Rx0Buffer[PCM_RIGHT];

	// do data processing if input data are marked as valid
	if((sAc97Tag & 0x1800) != 0)
	{
		g_fSineWaveIn_Left[g_iSampleIndex] = sLeftChannelIn<<2;
		g_fSineWaveIn_Right[g_iSampleIndex] = sRightChannelIn<<2;

		sLeftChannelOut 	= g_sInput[g_iIndex]>>2;
		sRightChannelOut 	= g_sInput[g_iIndex++]>>2;

		// reset index
		if( g_iIndex == 256 )
			g_iIndex = 0;

		g_iSampleIndex++;

		if( g_iSampleIndex > MAX_SAMPLES-1 )
			g_iSampleIndex = 0;

		g_iSampleCount++;
	}
	else
	{
		sLeftChannelOut = 0x0000;
		sRightChannelOut = 0x0000;
	}

	// copy data from previous frame into transmit buffer
	Tx0Buffer[TAG_PHASE] = sAc97Tag;
	Tx0Buffer[PCM_LEFT] = sLeftChannelOut;
	Tx0Buffer[PCM_RIGHT] = sRightChannelOut;

	// restore masked values
	sti(uiTIMASK);
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
	    for( i = 1; i <= (MAX_SAMPLES / 2); i++)
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
*   Function:    TEST_AUDIO
*   Description: Main test routine will get launched from the POST 
*				 framework.
*******************************************************************/
int TEST_AUDIO(void)
{
	int nResult = 0;
	int i = 0;

	// this is only needed because we overwrite these values
	// as we are writing other values to the AD1980
	// Since we run this test over and over in a loop we need
	// to initialize these values at the beginning every time
	// so we are not using the values that were left in the buffer
	Tx0Buffer[0] = 0xE000;
	Tx0Buffer[1] = 0x7400;
	Tx0Buffer[2] = 0x9900;
	Tx0Buffer[3] = 0x0000;
	Tx0Buffer[4] = 0x0000;
	Tx0Buffer[5] = 0x0000;
	Tx0Buffer[6] = 0x0000;
	Tx0Buffer[7] = 0x0000;
	
	// allocate storage for our buffers
	g_sInput = malloc(sizeof(short) * MAX_SAMPLES);
	g_fSineWaveIn_Left = malloc(sizeof(short) * MAX_SAMPLES);
	g_fSineWaveIn_Right = malloc(sizeof(short) * MAX_SAMPLES);

	// make sure the buffers were allocated
	if( (g_sInput == NULL) || (g_fSineWaveIn_Left == NULL) || (g_fSineWaveIn_Right == NULL) )
		return 0;

	// clear the buffers that will hold our sine wave data to make
	// sure we are always getting new data
	for( i = 0; i < MAX_SAMPLES; i++ )
	{
	    g_fSineWaveIn_Left[i] = 0;
	    g_fSineWaveIn_Right[i] = 0;
	}

	// create out sine wave
	for( i = 0; i < MAX_SAMPLES; i++ )
	{
	    g_sInput[i] = (int)(AMPLITUDE * sin( (2.0 * PI * DESIRED_FREQ * ( ((float)(i+1)) / SAMPLE_RATE))) );
	}

	// initialize some global variables
	g_iIndex = 0;
	g_iSampleCount = 0;
	g_iSampleIndex = 1;

	Init_Timers();
	Init_Timer_Interrupts();

	Audio_Reset();
	Init_Sport0();
	Init_DMA();
	Enable_SPORT0_DMA_TDM_Streams();
	Enable_Codec_Slot16_Mode();

	Init_AD1980_Codec();

	// Remap the vector table pointer from the default __I9HANDLER
	// to the new "Sport1_TX_ISR()" interrupt service routine
	register_handler(ik_ivg9, Sport0_TX_ISR);	// Remap Sport1 TX ISR -> IVG 9

	unsigned int nTimer = SetTimeout(0x50000);
	if( ((unsigned int)-1) != nTimer )
	{
	    // once the required number of samples has been collected,
	    // process the signal.
		do{
			asm("nop;");
			asm("nop;");
			asm("nop;");
			asm("nop;");
		}while( (g_iSampleCount < REQUIRED_SAMPLES)  && (!IsTimedout(nTimer)) );
	}

	ClearTimeout(nTimer);

    // turn off interrupts so that the data is stable.
    interrupt(ik_ivg9, SIG_IGN);

    
    // disable DMA and SPORT TX/RX
	*pDMA0_CONFIG &= ~0x1;
	*pDMA1_CONFIG &= ~0x1;

	*pSPORT0_TCR1 &= ~0x1;
	*pSPORT0_RCR1 &= ~0x1;

    // test the right channel
    nResult  = Test_Channel(g_fSineWaveIn_Right);

    if( 1 == nResult  ) // Right channel was OK, test left channel
    {
		nResult = Test_Channel(g_fSineWaveIn_Left);
    }

    // free allocated memory
    free(g_sInput);
    free(g_fSineWaveIn_Left);
    free(g_fSineWaveIn_Right);

    // invalidate our buffers
    g_sInput = NULL;
    g_fSineWaveIn_Left = NULL;
    g_fSineWaveIn_Right = NULL;

	// return pass or fail
    return nResult;
}

//--------------------------------------------------------------------------//
// Function:	main														//
//																			//
// Description:	After calling a few initalization routines, main() just 	//
//				waits in a loop forever.  The code to process the incoming  //
//				data can be placed in the function Process_Data() in the 	//
//				file "Process_Data.c".										//
//--------------------------------------------------------------------------//
#ifdef _STANDALONE_ // use this to run standalone tests
void main(void)
{
	int bResult;

 	while(1)
	{
		asm("nop;");
		asm("nop;");
		asm("nop;");
   		bResult = TEST_AUDIO();
   		asm("nop;");
   		asm("nop;");
	}
}
#endif //_STANDALONE_
