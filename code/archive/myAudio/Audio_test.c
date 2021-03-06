/*******************************************************************
*  include files
*******************************************************************/


#include <stdint.h>
#include <builtins.h>
#include <math_bf.h>
#include <math.h>
#include <blackfin.h>
#include <cycle_count.h>
#include "sincos.h"


/*******************************************************************
*  global variables and defines
*******************************************************************/


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
#define SLOT_TAG				0
#define SLOT_COMMAND_ADDRESS	1
#define SLOT_COMMAND_DATA		2
#define SLOT_PCM_LEFT			3
#define SLOT_PCM_RIGHT			4

#define SIZE_OF_CODEC_REGS		60		// size of array iCodecRegs

/* Mask bit selections in Serial Configuration Register for
   accessing registers on any of the 3 codecs */
#define	MASTER_Reg_Mask			0x1000
#define	SLAVE1_Reg_Mask			0x2000
#define	SLAVE2_Reg_Mask			0x4000
#define	MASTER_SLAVE1			0x3000
#define	MASTER_SLAVE2			0x5000
#define	MASTER_SLAVE1_SLAVE2	0x7000

/* Macros for setting Bits 15, 14 and 13 in Slot 0 Tag Phase */
#define	ENABLE_VFbit_SLOT1_SLOT2	0xE000
#define	ENABLE_VFbit_SLOT1			0xC000
#define ENABLE_VFbit_STEREO			0x9800

/* AD1981B AC-Link TDM Timeslot Definitions (indexed by 16-bit short words)*/
#define	TAG_PHASE				0
#define	COMMAND_ADDRESS_SLOT	1
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
#define Fs_CD_quality		44100
#define Fs_Speech_quality	8000
#define Fs_48kHz		 	48000
#define Fs_Pro_Audio		48000
#define Fs_Broadcast		22000
#define Fs_oddball_1		17897
#define Fs_oddball_2		23456
#define Fs_oddball_3		7893


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
#define MFD_1			0x1000		/* Multichannel Frame Delay = 1					*/
#define PMAP_SPORT0RX	0x0000		/* SPORT0 Receive DMA							*/
#define PMAP_SPORT0TX	0x1000		/* SPORT0 Transmit DMA						*/
#define WDSIZE_16		0x0004		/* Transfer Word Size = 16						*/



/* SPORT1 DMA receive buffer */
volatile short Rx0Buffer[8] = {0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000};

/* SPORT1 DMA transmit buffer */
volatile short Tx0Buffer[8] = {0xe000,0x7400,0x9900,0x0000,0x0000,0x0000,0x0000,0x0000};

short 	sAc97Tag = 0x8000;  // variable to save incoming AC97 tag in SPORT0 TX ISR

short 	sLeftChannelOut, sRightChannelOut;		// PCM output data

// Test paramaters
#define SAMPLE_RATE	((uint32_t)48000)

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

short sCodecRegsReadBack[SIZE_OF_CODEC_REGS];

short z = 0;

typedef enum LEDS_tag{
	LED1 = (1<<6),
	LED2 = (1<<7),
	LED3 = (1<<8),
	LED4 = (1<<9),
	LED5 = (1<<10),
	LED6 = (1<<12),
	LAST_LED = (1<<13)}enLED;

int32_t scale[12] = {
	MAKE_FREQUENCY(261.63),
	MAKE_FREQUENCY(277.18),
	MAKE_FREQUENCY(293.66),
	MAKE_FREQUENCY(311.13),
	MAKE_FREQUENCY(329.63),
	MAKE_FREQUENCY(349.23),
	MAKE_FREQUENCY(369.99),
	MAKE_FREQUENCY(392.0),
	MAKE_FREQUENCY(415.30),
	MAKE_FREQUENCY(440.0),
	MAKE_FREQUENCY(466.16),
	MAKE_FREQUENCY(493.88)
};


int16_t Sin=0, Cos=0x7000, dphase=3361;

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

void audioReset(void)
{
	*pPORTB_FER = 0x0000;
	*pPORTB_MUX = 0x0000;

	*pPORTB_DIR_SET = (1<<3);
	*pPORTB_SET = (1<<3);
	//AD1980 Datasheet says 1us pulse for reset
	//Set reset pulse time to 2us for safety 0x4B0
	
	*pPORTB_CLEAR = (1<<3);
	delayus(20);
	*pPORTB_SET = (1<<3);
	delayus(20);
}

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

uint16_t note2Frequency(int note)
{

	uint32_t frequency = 0;

	if(note >= 0)
		frequency = (1<<(note/12))*scale[note % 12];
	else
		frequency = scale[12 + (note % 12)] / (1<<(note/12));

	return (uint16_t)((frequency/SAMPLE_RATE));
}

static void initDelay(void)
{
	*pEVT11 =  (void *)delayTimerISR;
	*pIMASK |= EVT_IVG11;
	*pSIC_IMASK2	= (*pSIC_IMASK2 | IRQ_TIMER0);
	ssync();
}

static void delayus(uint32_t delayTime)
{

	uint32_t delayCount = 0;
	delayFinished = 0;

	//SCLK = 133e6
	//convert from SCLK counts / second * (delayTime / 1e6)
	delayCount = (SCLK*delayTime);

	//Check to be sure Timer is disabled

	//Configure Timer0
	*pTIMER0_CONFIG |= (PWM_OUT | IRQ_ENA | OUT_DIS);

	//Set interrupt condition
	*pTIMER0_WIDTH = delayCount;

	//start timer
	*pTIMER_ENABLE0 |= TIMEN0;

	//delay until ISR changes value of delayFinished
	while(!delayFinished);

	*pTIMER_DISABLE0 |= TIMDIS0;

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



void clearSetLED(const enLED led, const int bState)
{
	static unsigned short leds = 0;

	if (bState == 0) {
		*pPORTG_CLEAR = led; // clear 
		leds &= ~led;
	}
	else if (bState == 1) {
		*pPORTG_SET = led; // set
		leds |= led;
	}
	else if (leds & led) {
		*pPORTG_CLEAR = led; // toggle 
		leds &= ~led;
	}
	else {
		*pPORTG_SET = led; // toggle 
		leds |= led;
	}
}

void initLEDs(void)
{
	*pPORTG_FER &= ~0x0FC0;
	*pPORTG_MUX &= ~0x0FC0;
	*pPORTG_DIR_SET = 0x0FC0;
}

int pollButtons(void)
{

	int buttonNow;

	buttonNow = (*pPORTB & 0x0F00) >> 2;

	if(buttonCount >= DEBOUNCE_COUNT)
	{
		buttonState = buttonTest;
	}

	if(buttonNow == buttonTest)
	{
		buttonCount++;
	}
	else
	{	
		buttonTest = buttonNow;
		buttonCount = 0;
	}

	return buttonState;

}

int main(void)
{

	initButtons();
	initDelay();

	//NEC APPROVED	
	initLEDs();

	//NEC APPROVED*
	//*Went on faith that the original author knew what the AD1980 wants
	initSPORT0();

	//NEC APPROVED
	//Toggles Reset line for 2us up and down followed by a 2us delay.
	audioReset();
	
	initDMA();
		
	enableSPORT0DMATDMStreams();

	//Rest of program is interrupt driven

	while(1);
	return 0;
}
