#include "../BF548/DMA.h"
#include "../BF548/ports.h"

#include "../../Util/delay.h"

#include "AD1980.h"
#include <stdint.h>


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
#define		RECORD_GAIN				0x1C00
#define		GENERAL_PURPOSE			0x2000
#define		AUDIO_INTERRUPT_PAGING	0x2400
#define		POWERDOWN_CTRL_STAT		0x2600
#define		EXTEND_AUDIO_ID			0x2800
#define		EXTEND_AUDIO_CTL		0x2A00
#define		FRONT_DAC_SAMPLE_RATE	0x2C00
#define		SURR_DAC_SAMPLE_RATE	0x2E00
#define		C_LFE_DAC_SAMPLE_RATE	0x3000
#define		ADC_SAMPLE_RATE			0x3200
#define		CENTER_LFE_VOLUME		0x3600
#define		SURROUND_VOLUME			0x3800
#define		SPDIF_CONTROL			0x3A00
#define		EQ_CONTROL				0x6000
#define		EQ_DATA					0x6200
#define	 	JACK_SENSE				0x7200
#define		SERIAL_CONFIGURATION	0x7400
#define		MISC_CONTROL_BITS		0x7600
#define		VENDOR_ID_1				0x7C00
#define		VENDOR_ID_2				0x7E00

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
#define SAMPLE_RATE	((uint32_t)48000)

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

short 	sAc97Tag = 0x8000;  // variable to save incoming AC97 tag in SPORT0 TX ISR
short 	sLeftChannelOut, sRightChannelOut;		// PCM output data

short 	sCodecRegs[SIZE_OF_CODEC_REGS] =						// array for codec registers
{																// names are defined in "ad1980.h"
					SERIAL_CONFIGURATION,	0x9000,	// Set Slot 16 mode a second time to disable CHEN bit with correct 16-bit alignment
					RESET_REG,				0xABCD,	// Soft reset, Any write will do
					MASTER_VOLUME,			0x0000,	// Unmute Master Volume, 0dB
					HEADPHONE_VOLUME,		0x8000,	// Default, Muted Headphone Volume
					MASTER_VOLUME_MONO,		0x8000,	// Default, Muted, 0dB
					PHONE_VOLUME,			0x8008,	// Default, Muted, 0dB
					MIC_VOLUME,			0x8008, 	// Default Muted... NOTE:  This needs adjusted when MIC input selected !!!
					LINE_IN_VOLUME,			0x8F0F, // Unmuted with some gain added
					CD_VOLUME,				0x8808,	// Default, Muted, 0dB
					AUX_VOLUME,				0x8808,	// Default, Muted, 0dB
					PCM_OUT_VOLUME,			0x0000,	// Unmuted, 0dB
					RECORD_SELECT,			0x0404,	// Selected Line In as Record Select... use 0x0000 for MIC input
					RECORD_GAIN,			0x0000,	// Unmuted Record Gain, 0dB
					GENERAL_PURPOSE,		0x0000,	// 0x0080,
					AUDIO_INTERRUPT_PAGING,	0x0000,
					EXTEND_AUDIO_ID,		0x0001,	// Enable VRA and DAC slot assignments 1, 2, 3 & 4
					EXTEND_AUDIO_CTL,		0x0000,	// Default
					FRONT_DAC_SAMPLE_RATE,	0xBB80,	// 48 kHz default
					SURR_DAC_SAMPLE_RATE,	0xBB80,	// 48 kHz default
					C_LFE_DAC_SAMPLE_RATE,	0xBB80,	// 48 kHz default
					ADC_SAMPLE_RATE,		0xBB80,	// 48 kHz default
					CENTER_LFE_VOLUME,		0x0000,	// unmuted
					SURROUND_VOLUME,		0x0000,	// unmuted
					SPDIF_CONTROL,			0x2000,	// default state
					EQ_CONTROL,				0x8080, // default, EQ muted
					EQ_DATA,				0x0000, // default
					MISC_CONTROL_BITS,		0x0040, // enable stereo MIC option
					JACK_SENSE,				0x0000,	// default
					VENDOR_ID_1,			0x4144,	// These are read-only, but the read-back will verify them
					VENDOR_ID_2,			0x5370
};

short sCodecRegsReadBack[SIZE_OF_CODEC_REGS];

void initAD1980(void){
	//AD1980Reset();
	//AD1980ConfigureDMA();
	//setAD1980ConfigureISR();
	//startAD1980Configure();
}

void AD1980Reset(void)
{
//	*pPORTB_FER = 0x0000;
//	*pPORTB_MUX = 0x0000;
//	*pPORTB_DIR_SET = (1<<3);
//	*pPORTB_SET = (1<<3);
///	setPinDirection("B",(1<<3));
///	setPin("B",pin,1);

	//AD1980 Datasheet says 1us pulse for reset
	//Set reset pulse time to 2us for safety 0x4B0
///	setPin("B",(1<<3),0);
//	*pPORTB_CLEAR = (1<<3);
	delayus(20);
//	*pPORTB_SET = (1<<3);
///	setPin("B",(1<<3),1);
	delayus(20);
}

void AD1980ConfigureDMA(void)
{
///	DMAReset(0);
///	DMAReset(1);
//	*pDMA0_CONFIG = 0x0000;
//	*pDMA1_CONFIG = 0x0000;
//	*pDMA0_IRQ_STATUS = 0x0000;
//	*pDMA1_IRQ_STATUS = 0x0000;

///	setDMAPeripheral(0,SPORT0RX);
///	setDMAPeripheral(1,SPORT0TX);
//	*pDMA0_PERIPHERAL_MAP = PMAP_SPORT0RX;
//	*pDMA1_PERIPHERAL_MAP = PMAP_SPORT0TX;

///	DMAConfig(0,blah);
//	*pDMA0_CONFIG = FLOW_AUTO | DI_EN | WDSIZE_16 | WNR;
//	*pDMA0_START_ADDR = (void *)Rx0Buffer;
//	*pDMA0_X_COUNT = 8;
//	*pDMA0_X_MODIFY = 2;

///	DMAConfig(1,blah);
//	*pDMA1_CONFIG = FLOW_AUTO | DI_EN | WDSIZE_16;
//	*pDMA1_START_ADDR = (void *)Tx0Buffer;
//	*pDMA1_X_COUNT = 8;
//	*pDMA1_X_MODIFY = 2;

	uint32_t x = 2000;

	//MAGIC!!!
	delayus(x);
}

void setAD1980ConfigureISR(uint16_t *ISR){
///setDMAISR(0,*ISR);
}

__attribute__((interrupt_handler))
static void  sport0TXISRSetup(void)
{
/*
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
*/
}
