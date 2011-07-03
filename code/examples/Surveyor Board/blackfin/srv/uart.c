/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  uart.c - uart functions for the SRV-1 Blackfin robot.
 *    Copyright (C) 2005-2009  Surveyor Corporation
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details (www.gnu.org/licenses)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <cdefBF537.h>
#include "config.h"
#include "uart.h"
#include "srv.h"

#define SSYNC    asm("ssync;")
#define SUART_SEND0 *pPORTHIO |= 0x4000
#define SUART_SEND1 *pPORTHIO &= 0xBFFF
#define SUART_RECV  (*pPORTHIO & 0x8000)

static int suart_timebase;

void waituntilNS(unsigned int target) {
    if (target > PERIPHERAL_CLOCK) {
        target -= PERIPHERAL_CLOCK;
        while (*pTIMER4_COUNTER > target)  // wait for timer to wrap-around
            continue;
    }
    while (*pTIMER4_COUNTER < target)
        continue;
}

void suartInit(int baud) {  // use GPIO-H14 and H15 for soft uart.  H14 = TX, H15 = RX
    suart_timebase = 1000000000 / baud;  // define bit period in nanoseconds
    *pPORTHIO_DIR &= 0x3FFF;   // set H14 as output, H15 as input
    SSYNC;
    *pPORTHIO_DIR |= 0x4000;   // enable H14 output
    SSYNC;
    *pPORTHIO_INEN |= 0x8000;  // set H15 as input
    SSYNC;
    SUART_SEND0;  // set H14 high
    SSYNC;
}

void suartPutChar(unsigned char ch)  // soft uart send - transmit on GPIO-H14
{
    int ix;
    unsigned int twait;
    
    twait = *pTIMER4_COUNTER + suart_timebase;
    SUART_SEND1;  // send start bit
    waituntilNS(twait);
    for (ix=0; ix<8; ix++) {
        if (ch & 0x01)
            SUART_SEND0;  // output is inverted
        else
            SUART_SEND1;
        twait += suart_timebase; 
        waituntilNS(twait);
        ch = ch >> 1;
    }
    SUART_SEND0;  // send 2 stop bits
    twait += suart_timebase * 2;
    waituntilNS(twait); 
}

unsigned short suartGetChar(int timeout)  // check for incoming character, wait for "timeout" milliseconds
{
    int t0;
    unsigned short sx, smask;
    unsigned int twait;
    
    t0 = readRTC();
    sx = 0;
    
    while ((readRTC()-t0) < timeout) {  // wait for start bit
        if (SUART_RECV)
            continue;
        twait = *pTIMER4_COUNTER + ((suart_timebase * 4) / 3);
        waituntilNS(twait);  // wait for completion of start bit, then go 30% into next bit
        for (smask=1; smask<256; smask*=2) {
            if (SUART_RECV)
                sx += smask;
            twait += suart_timebase;
            waituntilNS(twait); 
        }
        twait += suart_timebase;  // wait for first stop bit
        waituntilNS(twait); 
        return (sx + 0x8000);  // set high bit to indicate received character
    }
    return 0;
}

void init_uart0(int baudrate)
{
    int uart_divider;

    uart_divider = (((MASTER_CLOCK * VCO_MULTIPLIER) / SCLK_DIVIDER) / 16) / baudrate;
    *pPORTF_FER |= 0x0003;  // enable UART0 pins
    *pUART0_GCTL = UCEN;
    *pUART0_LCR = DLAB;
    *pUART0_DLL = uart_divider;
    *pUART0_DLH = uart_divider >> 8;
    *pUART0_LCR = WLS(8); // 8 bit, no parity, one stop bit

    // dummy reads to clear possible pending errors / irqs
    char dummy = *pUART0_RBR;
    dummy = *pUART0_LSR;
    dummy = *pUART0_IIR;
    SSYNC;
}

void uart0_CTS(int ix)  // set ~CTS signal.  1 = clear to send   0 = not clear to send
{
    if (ix == 0)
        *pPORTHIO |= 0x0040;  // block incoming data 
    else
        *pPORTHIO &= 0xFFBF;  // allow incoming data 
}

void init_uart1(int baudrate)
{
    int uart_divider;

    uart_divider = (((MASTER_CLOCK * VCO_MULTIPLIER) / SCLK_DIVIDER) / 16) / baudrate;
    *pPORTF_FER |= 0x000C;  // enable UART1 pins
    *pUART1_GCTL = UCEN;
    *pUART1_LCR = DLAB;
    *pUART1_DLL = uart_divider;
    *pUART1_DLH = uart_divider >> 8;
    *pUART1_LCR = WLS(8); // 8 bit, no parity, one stop bit

    char dummy = *pUART1_RBR;
    dummy = *pUART1_LSR;
    dummy = *pUART1_IIR;
    SSYNC;
}

void uart0SendChar(unsigned char c)
{
    while (*pPORTHIO & 0x0001)  // hardware serial flow control - 
        continue;               //    S32 pin 17 should be grounded to disable
    while (!(*pUART0_LSR & THRE))
        continue;
    *pUART0_THR = c;
}

void uart0SendString(unsigned char *s)
{
    char a;
    while ((a = *s++)) {
        uart0SendChar(a);
    }
}

void uart0SendChars(unsigned char *buf, unsigned int size)
{
    while (size--) {
        uart0SendChar(*buf++);
    }
}

unsigned char uart0GetCh()
{
    while (!(*pUART0_LSR & DR));
    return *pUART0_RBR;
}

unsigned char uart0GetChar(unsigned char *a)
{
    if (!(*pUART0_LSR & DR))
        return 0;
    *a = *pUART0_RBR;
    return 1;
}

unsigned char uart0Signal()
{
    if (!(*pUART0_LSR & DR))
        return 0;
    return 1;
}

void uart1SendChar(unsigned char c)
{
    while (!(*pUART1_LSR & THRE));
    *pUART1_THR = c;
}

void uart1SendString(unsigned char *s)
{
    char a;
    while ((a = *s++)) {
        uart1SendChar(a);
    }
}

void uart1SendChars(unsigned char *buf, unsigned int size)
{
    while (size--) {
        uart1SendChar(*buf++);
    }
}

unsigned char uart1GetCh()
{
    while (!(*pUART1_LSR & DR));
        return *pUART1_RBR;
}

unsigned char uart1GetChar(unsigned char *a)
{
    if (!(*pUART1_LSR & DR))
        return 0;
    *a = *pUART1_RBR;
    return 1;
}

unsigned char uart1Signal()
{
    if (!(*pUART1_LSR & DR))
        return 0;
    return 1;
}

/*****************************************************************************
 *
 * Description:
 *    Routine for printing integer numbers in various formats. The number is 
 *    printed in the specified 'base' using exactly 'noDigits', using +/- if 
 *    signed flag 'sign' is TRUE, and using the character specified in 'pad' 
 *    to pad extra characters. 
 *
 * Params:
 *    [in] base     - Base to print number in (2-16) 
 *    [in] noDigits - Number of digits to print (max 32) 
 *    [in] sign     - Flag if sign is to be used (TRUE), or not (FALSE) 
 *    [in] pad      - Character to pad any unused positions 
 *    [in] number   - Signed number to print 
 *
 ****************************************************************************/
void
printNumber(unsigned char  base,
            unsigned char  noDigits,
            unsigned char  sign,
            unsigned char  pad,
            int number)
{
  static unsigned char  hexChars[16] = "0123456789ABCDEF";
  unsigned char        *pBuf;
  unsigned char         buf[32];
  unsigned int        numberAbs;
  unsigned int        count;

  // prepare negative number
  if(sign && (number < 0))
    numberAbs = -number;
  else
    numberAbs = number;

  // setup little string buffer
  count = (noDigits - 1) - (sign ? 1 : 0);
  pBuf = buf + sizeof(buf);
  *--pBuf = '\0';

  // force calculation of first digit
  // (to prevent zero from not printing at all!!!)
  *--pBuf = hexChars[(numberAbs % base)];
  numberAbs /= base;

  // calculate remaining digits
  while(count--)
  {
    if(numberAbs != 0)
    {
      //calculate next digit
      *--pBuf = hexChars[(numberAbs % base)];
      numberAbs /= base;
    }
    else
      // no more digits left, pad out to desired length
      *--pBuf = pad;
  }

  // apply signed notation if requested
  if(sign)
  {
    if(number < 0)
      *--pBuf = '-';
    else if(number > 0)
       *--pBuf = '+';
    else
       *--pBuf = ' ';
  }

  // print the string right-justified
  uart0SendString(pBuf);
}


