#include <stdint.h>

void initDelay(void)
{
/*
	*pEVT11 =  (void *)delayTimerISR;
	*pIMASK |= EVT_IVG11;
	*pSIC_IMASK2	= (*pSIC_IMASK2 | IRQ_TIMER0);
	ssync();
*/
}

void delayus(uint32_t delayTime)
{
/*
	uint32_t delayCount = 0;
	delayFinished = 0;

	//SCLK = 133e6
	//convert from SCLK counts / second * (delayTime / 1e6)
	delayCount = (SCLK*delayTime);

	//Check to be sure Timer is disabled

	//Configure Timer0
	*pTIMER0_CONFIG |= (PWM_OUT | IRQ_ENA | OUT_DIS);

	//Set interrupt condition
	*pTIMER0_WIDTH = delayCount;

	//start timer
	*pTIMER_ENABLE0 |= TIMEN0;

	//delay until ISR changes value of delayFinished
	while(!delayFinished);

	*pTIMER_DISABLE0 |= TIMDIS0;
*/

}

__attribute__((interrupt_handler))
void delayTimerISR(void)
{
	/*
	*pTIMER_STATUS0	|= TIMIL0;
	delayFinished = 1;
	*/
}
