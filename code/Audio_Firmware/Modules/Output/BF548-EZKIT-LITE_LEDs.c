typedef enum LEDS_tag{
	LED1 = (1<<6),
	LED2 = (1<<7),
	LED3 = (1<<8),
	LED4 = (1<<9),
	LED5 = (1<<10),
	LED6 = (1<<12),
	LAST_LED = (1<<13)}enLED;

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
