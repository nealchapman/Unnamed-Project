#ifndef __AD1980_H__
#define __AD1980_H__

#include <stdint.h>

void initAD1980(void);
void AD1980Reset(void);
void AD1980ConfigureDMA(void);
void setAD1980ConfigureISR(uint16_t *ISR);
__attribute__((interrupt_handler))
static void  sport0TXISRSetup(void);

#endif //__AD1980_H__
