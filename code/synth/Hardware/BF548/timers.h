#ifndef __TIMERS_H__
#define __TIMERS_H__

void configureTimer(blah);
void setTimerWidth(uint32_t width);
uint32_t readTimerCounter(uint16_t timer);
void enableTimer(uint16_t timer);
void disableTimer(uint16_t);
uint32_t readTimerStatus(uint16_t timer);

#endif //__TIMERS_H__
