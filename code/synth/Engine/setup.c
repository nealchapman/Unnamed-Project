
#include "display.h"
#include "interface.h"
#include "audio.h"
#include <cycle_count.h>

cycle_t cycle_start = 0x0000;
cycle_t cycle_stop = 0x0000;

void initEngine(void){
	//~/Engine/display.h
	initDisplay();
	//~/Engine/interface.h
	initInterface();
	//~/Engine/audio.h
	initAudio();
}

void startEngine(void){
	//~/Engine/audio.h
	enableAudioOut();
}	
