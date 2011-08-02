typedef enum codecList{
	AD1980 = 1;
}supportedCodec;

cycle_t cycle_start = 0x0000;
cycle_t cycle_stop = 0x0000;

void initDSP(void){
	configureAudioOut(AD1980);
	enableAudioOut();
}

void initBoard(void){
	//~/Hardware/BF548EZ-KIT_LITE/buttons.h
	initButtons();
	//~/Hardware/BF548EZ-KIT_LITE/LEDs.h
	initLEDs();
	//~/Hardware/BF548EZ-KIT_LITE/AD1980.h
	initAD1980();	
}


