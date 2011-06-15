#include <blackfin.h>

#define pPORTG_FER       ((volatile unsigned short *)PORTG_FER)
#define pPORTG_DIR_CLEAR ((volatile unsigned short *)PORTG_DIR_CLEAR)
#define pPORTG_CLEAR     ((volatile unsigned short *)PORTG_CLEAR)
#define pPORTG_SET       ((volatile unsigned short *)PORTG_SET)
#define pPORTG_DIR_SET   ((volatile unsigned short *)PORTG_DIR_SET)
#define pPORTG           ((volatile unsigned short *)PORTG)
#define pPORTG_MUX       ((volatile unsigned int   *)PORTG_MUX)

#define pPORTB_FER	 ((volatile unsigned short *)PORTB_FER)	
#define pPORTB		 ((volatile unsigned short *)PORTB)
#define pPORTB_INEN	 ((volatile unsigned short *)PORTB_INEN)

#define BLINK_FAST      2000
#define BLINK_SLOW      (BLINK_FAST * 2)

typedef enum LEDS_tag{
	LED1 = (1<<6),
	LED2 = (1<<7),
	LED3 = (1<<8),
	LED4 = (1<<9),
	LED5 = (1<<10),
	LED6 = (1<<12),
	LAST_LED = (1<<13)
}enLED;

void Delay(unsigned long ulMs)
{
	unsigned long sleep = ulMs * 5000;
	while (sleep--)
		asm("nop");
}

void Init_LEDs(void)
{
	*pPORTG_FER &= ~0x0FC0;
	*pPORTG_MUX &= ~0x0FC0;
	*pPORTG_DIR_SET = 0x0FC0;
}

void Init_Buttons(void)
{
	*pPORTB_FER &= ~0x0F00;
	*pPORTB_INEN = 0x0F00;
}

void ClearSet_LED(const enLED led, const int bState)
{
	static unsigned short leds = 0;

	if (bState == 0) {
		*pPORTG_CLEAR = led; /* clear */
		leds &= ~led;
	}
	else if (bState == 1) {
		*pPORTG_SET = led; /* set */
		leds |= led;
	}
	else if (leds & led) {
		*pPORTG_CLEAR = led; /* toggle */
		leds &= ~led;
	}
	else {
		*pPORTG_SET = led; /* toggle */
		leds |= led;
	}
}

void ClearSet_LED_Bank(const int enleds, const int iState)
{
	enLED n;
	int nTempState = iState;

	for (n = LED1; n < LAST_LED; (n <<= 1)) {
		if (n & enleds)
			ClearSet_LED(n, (nTempState & 0x3));
		nTempState >>= 2;
	}
}

void LED_Bar(const int iSpeed)
{
	enLED n;
	for (n = LED1; n < LAST_LED; (n <<= 1)) {
		ClearSet_LED(n, 3);
		Delay(iSpeed);
	}
}

int main(void)
{

	static int buttons = 0;
	enLED n;

	n = LED1;

	Init_LEDs();
	Init_Buttons();
	ClearSet_LED_Bank(-1, 0x0000);
	while (1)
	{
		Delay(200);
		buttons^=(*pPORTB & 0x0F00);
		if(buttons != 0x0)
			ClearSet_LED_Bank(buttons>>2,-1);
		Delay(200);
	}
	return 0;
}
