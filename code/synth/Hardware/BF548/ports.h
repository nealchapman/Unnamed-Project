#ifndef __PORTS_H__
#define __PORTS_H__

#include <stdint.h>

void setPinDirection(uint16_t port, uint16_t pinMask, uint16_t dir);
void readPinDirection(uint16_t port, uint16_t pinMask);
void setPin(uint16_t port, uint16_t pinMask);
void readPin(uint16_t port, uint16_t pinMask);	

#endif // __PORTS_H__
