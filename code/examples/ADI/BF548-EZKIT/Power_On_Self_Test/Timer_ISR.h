#ifndef _TIMER_ISR_H_
#define _TIMER_ISR_H_

//--------------------------------------------------------------------------//
// Header files																//
//--------------------------------------------------------------------------//
#include <sys\exception.h>
#include <cdefBF548.h>
#include <ccblkfn.h>

//--------------------------------------------------------------------------//
// Symbolic constants														//
//--------------------------------------------------------------------------//
#define TIMEOUT_PERIOD	0x00002000


//--------------------------------------------------------------------------//
// Global variables															//
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
// Prototypes																//
//--------------------------------------------------------------------------//
void Init_Timers(void);
void Init_Timer_Interrupts(void);
void Delay(const unsigned long ulMs);
unsigned int SetTimeout(const unsigned long ulTicks);
unsigned long ClearTimeout(const unsigned int nIndex);
bool IsTimedout(const unsigned int nIndex);

EX_INTERRUPT_HANDLER(Timer_ISR);


#endif //_TIMER_ISR_H_

