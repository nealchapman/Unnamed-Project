#ifndef __PORTS_H__
#define __PORTS_H__

void setPinDirection(uint16_t port, uint16_t pin, uint16_t dir);
void setPin(uint16_t port, uint16_t pin);
void readPin(uint16_t port, uint16_t pin);	

#endif // __PORTS_H__
