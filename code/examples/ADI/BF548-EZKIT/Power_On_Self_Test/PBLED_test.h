#ifndef _PBLED_TEST_H_
#define _PBLED_TEST_H_


//--------------------------------------------------------------------------//
// Header files																//
//--------------------------------------------------------------------------//
#include <sys\exception.h>
#include <cdefBF548.h>
#include <ccblkfn.h>

/////////////////////////        Constants        ////////////////////////////////


typedef enum LEDS_tag{
	LED1 = 0x40,
	LED2 = 0x80,
	LED3 = 0x100,
	LED4 = 0x200,
	LED5 = 0x400,
	LED6 = 0x800,
	LAST_LED = 0x1000
}enLED;


#define BLINK_FAST		2000
#define BLINK_SLOW		(BLINK_FAST * 2)


///////////////////////////   Function Prototypes     ///////////////////////////
void Init_LEDs(void);
void Init_PushButtons(void);

void ClearSet_LED(const enLED led, const int bState);
void LED_Bar(const int iSpeed);
void Blink_LED(const int enleds, const int iSpeed);
void ClearSet_LED_Bank(const int enleds, const int iState);
#endif //_PBLED_TEST_H_
