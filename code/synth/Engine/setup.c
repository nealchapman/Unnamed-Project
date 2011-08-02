typedef enum codecList{
	AD1980 = 1
}supportedCodec;

void initDMA(void)
{
		

}

cycle_t cycle_start = 0x0000;
cycle_t cycle_stop = 0x0000;

void initDSP(void){
	configureAudioOut(AD1980);
	enableAudioOut();
}

void initBoard(void){
	//~/Hardware/BF548EZ-KIT_LITE
	initButtons();
	initLEDs();
	initAD1980();	
}

void configureAudioOut(const supportedCodecs codec){
	//~/Hardware/BF548
	configureDMA();
	setAudioDMAISR();
}

void enableAudioOut(void){
	enableDMAISR();
}
