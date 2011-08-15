#ifndef __LEDS_H__
#define __LEDS_H__

typedef enum LEDS_tag{
	LED1 = (1<<6),
	LED2 = (1<<7),
	LED3 = (1<<8),
	LED4 = (1<<9),
	LED5 = (1<<10),
	LED6 = (1<<12),
	LAST_LED = (1<<13)}enLED;


void initBF548EZKIT_LITELEDs(void);
void clearSetLED(const enLED led, const int bState);

#endif //__LEDS_H__
