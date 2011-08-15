#include "../BF548/ports.h"

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

void initBF548EZKIT_LITELEDs(void);
{
	setPortDirection(G,0x0FC0,1);
	//*pPORTG_FER &= ~0x0FC0;
	//*pPORTG_MUX &= ~0x0FC0;
	//*pPORTG_DIR_SET = 0x0FC0;
}
