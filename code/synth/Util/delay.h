#ifndef __DELAY_H__
#define __DELAY_H__

void initDelay(void);
void delayus(uint32_t delayTime);
__attribute__((interrupt_handler))
void delayTimerISR(void);

#endif //
