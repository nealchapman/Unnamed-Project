#include "audio.h"
#include "../Hardware/BF548EZ-KIT_LITE/AD1980.h"

void initAudio(void)
{
	initCodec();
	configureAudioOut(1);
}

__attribute__((interrupt_handler))
static void audioISR()
{
/*
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
	
	*pDMA1_IRQ_STATUS = 0x0001;*/
}

void configureAudioOut(int codec)
{
	configureAudioDMA();
	setAudioDMAISR();
}

void initCodec(void)
{
	//~/Hardware/BF548EZ-KIT_LIGHT/AD1980.h
	initAD1980();	
}

void configureAudioDMA(void)
{

}

void setAudioDMAISR(void)
{

}
