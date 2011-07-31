#ifndef __INTERRUPTS_H__
#define INTERRUPTS

void clearIRQ(uint16_t IRQ);
void setISRAddress(uint16_t *ISRAddress);
void enableInterrupt(uint16_t interrupt);
void disableInterrupt(uint16_t interrupt);

#endif //__INTERRUPTS_H_
