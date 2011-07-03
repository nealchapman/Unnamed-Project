//
// crt0.asm
//
// This is the startup code to get a supervisor program going.
//
// Support: BF533, BF537, BF561
// 2004, 2005 Martin Strubel <hackfin@section5.ch>
//
// $Id: crt0.asm 792 2009-09-10 15:45:33Z strubi $
//
//

#ifndef LO
#define LO(x) ((x) & 0xffff)
#endif
#ifndef HI
#define HI(x) (((x) >> 16) & 0xffff)
#endif

#define BARE_METAL


.text;

#include <defBF537.h>

////////////////////////////////////////////////////////////////////////////
// core clock dividers -- DO NOT CHANGE!
//#define CCLK_1 0x00
//#define CCLK_2 0x10
//#define CCLK_4 0x20
#//define CCLK_8 0x30

// little macro trick to resolve macros before concatenating:
//#define _GET_CCLK(x) CCLK_##x
//#define GET_CCLK(x) _GET_CCLK(x)

// Short bootstrap

.global start
start:

	sp.h = _supervisor_stack;		//Set up supervisor stack
	sp.l = _supervisor_stack;
	fp = sp;

// Register initialization:
// Currently to prevent some nasty effects within uClibc (memcpy using I0)

	i0 = 0(z);
	m0 = 0(z);
	l0 = 0(z);
	b0 = 0(z);
	i1 = 0(z);
	m1 = 0(z);
	l1 = 0(z);
	b1 = 0(z);



////////////////////////////////////////////////////////////////////////////
// PLL and clock setups
//
//



setupPLL:
	// we have to enter the idle state after changes applied to the
	// VCO clock, because the PLL needs to lock in on the new clocks.


//	p0.l = LO(PLL_CTL);
//	p0.h = HI(PLL_CTL);
//	r1 = w[p0](z);
//	r2 = r1;  
//	r0 = 0(z);
//		
//	r0.l = ~(0x3f << 9);
//	r1 = r1 & r0;
//	r0.l = ((VCO_MULTIPLIER & 0x3f) << 9);
//	r1 = r1 | r0;


// 	p1.l = LO(SIC_IWR);
//	p1.h = HI(SIC_IWR);

//	r0 = [p1];			
//	bitset(r0, 0);	     // Enable PLL wakeup interrupt
//	[p1] = r0;
	
// 	w[p0] = r1;          // Apply PLL_CTL changes.
//	ssync;
 	
//	cli r0;
// 	idle;	// wait for Loop_count expired wake up
//	sti r0;

	// now, set clock dividers:
//	p0.l = LO(PLL_DIV);
//	p0.h = HI(PLL_DIV);

	// SCLK = VCOCLK / SCLK_DIVIDER
//	r0.l = (GET_CCLK(CCLK_DIVIDER) | (SCLK_DIVIDER & 0x000f));

//	w[p0] = r0; // set Core and system clock dividers

//	// not needed in reset routine: sti r1;

////////////////////////////////////////////////////////////////////////////
// install default interrupt handlers

	p0.l = LO(EVT2);
	p0.h = HI(EVT2);

	r0.l = _NHANDLER;
	r0.h = _NHANDLER;  	// NMI Handler (Int2)
    [p0++] = r0;

    r0.l = EXC_HANDLER;
  	r0.h = EXC_HANDLER;  	// Exception Handler (Int3)
    [p0++] = r0;
	
	[p0++] = r0; 		// IVT4 isn't used

    r0.l = _HWHANDLER;
	r0.h = _HWHANDLER; 	// HW Error Handler (Int5)
    [p0++] = r0;
	
    r0.l = _THANDLER;
	r0.h = _THANDLER;  	// Timer Handler (Int6)
	[p0++] = r0;
	
    r0.l = _RTCHANDLER;
	r0.h = _RTCHANDLER; // IVG7 Handler
	[p0++] = r0;
	
    r0.l = _I8HANDLER;
	r0.h = _I8HANDLER; 	// IVG8 Handler
  	[p0++] = r0;
  	
  	r0.l = _I9HANDLER;
	r0.h = _I9HANDLER; 	// IVG9 Handler
 	[p0++] = r0;
 	
    r0.l = _I10HANDLER;
	r0.h = _I10HANDLER;	// IVG10 Handler
 	[p0++] = r0;
 	
    r0.l = _I11HANDLER;
	r0.h = _I11HANDLER;	// IVG11 Handler
  	[p0++] = r0;
  	
    r0.l = _I12HANDLER;
	r0.h = _I12HANDLER;	// IVG12 Handler
  	[p0++] = r0;
  	
    r0.l = _I13HANDLER;
	r0.h = _I13HANDLER;	// IVG13 Handler
    [p0++] = r0;

    r0.l = _I14HANDLER;
	r0.h = _I14HANDLER;	// IVG14 Handler
  	[p0++] = r0;

    r0.l = _I15HANDLER;
	r0.h = _I15HANDLER;	// IVG15 Handler
	[p0++] = r0;


	// we want to run our program in supervisor mode,
	// therefore we need a few tricks:

	//  Enable Interrupt 15 
	p0.l = LO(EVT15);
	p0.h = HI(EVT15);
	r0.l = call_main;  // install isr 15 as caller to main
	r0.h = call_main;
	[p0] = r0;

	r0 = 0x8000(z);    // enable irq 15 only
	sti r0;            // set mask
	raise 15;          // raise sw interrupt
	
	p0.l = wait;
	p0.h = wait;

	reti = p0;
	rti;               // return from reset

wait:
	jump wait;         // wait until irq 15 is being serviced.


call_main:
	[--sp] = reti;  // pushing RETI allows interrupts to occur inside all main routines

	call clear_bss;  // clear BSS

#ifndef BARE_METAL

	// Do some libc initialization:
	call init_newlib;

#endif

	p0.l = _main;
	p0.h = _main;

	r0.l = end;
	r0.h = end;

	rets = r0;      // return address for main()'s RTS

	jump (p0);      // jump into main without argc/argv

end:
	idle;
	jump end;


.global idle_loop
idle_loop:
	idle;
	jump idle_loop;


start.end:

////////////////////////////////////////////////////////////////////////////
// SETUP ROUTINES
//

mem_clr:
	r0 = 0(x);
loop0:
	cc = p1 < p2(iu);
	if !cc jump clr_done (bp);
	[p1++] = r0;
	jump loop0;
clr_done:
	rts

clear_bss:
	link 0x0;
	p1.h = __bss_start;
	p1.l = __bss_start;
	p2.h = __bss_end;
	p2.l = __bss_end;
	
	call mem_clr;

#ifdef CLEAR_SDRAM_BSS
	p1.h = __sdram_bss_start;
	p1.l = __sdram_bss_start;
	p2.h = __sdram_bss_end;
	p2.l = __sdram_bss_end;

	call mem_clr;
#endif

	unlink;
	rts;


#ifndef BARE_METAL
init_newlib:
	link 0x0;

	p0.l = __init;
	p0.h = __init;
	call (p0);

	unlink;
	rts;
#endif

////////////////////////////////////////////////////////////////////////////
// Default handlers:
//


// If we get caught in one of these handlers for some reason, 
// display the IRQ vector on the EZKIT LEDs and enter
// endless loop.

display_fail:
	bitset(r0, 5);    // mark error
#ifdef EXCEPTION_REPORT
	call EXCEPTION_REPORT;
#endif
	jump stall;


_HWHANDLER:           // HW Error Handler 5
	emuexcpt;
rti;

_NHANDLER:
stall:
	jump stall;

EXC_HANDLER:          // exception handler
#ifdef EXCEPTION_REPORT
	r0 = seqstat;
	r1 = retx;
	call EXCEPTION_REPORT;
	cc = r0 == 0;
	if !cc jump cont_program;
#endif
	emuexcpt;
	jump idle_loop;
cont_program:
	rtx;

_THANDLER:            // Timer Handler 6	
	r0.l = 6;
	jump display_fail;

_RTCHANDLER:          // IVG 7 Handler  
	r0.l = 7;
	jump display_fail;

_I8HANDLER:           // IVG 8 Handler
	r0.l = 8;
	jump display_fail;

_I9HANDLER:           // IVG 9 Handler
	r0.l = 9;
	jump display_fail;

_I10HANDLER:          // IVG 10 Handler
	r0.l = 10;
	jump display_fail;

_I11HANDLER:          // IVG 11 Handler
	r0.l = 11;
	jump display_fail;

_I12HANDLER:          // IVG 12 Handler
	r0.l = 12;
	jump display_fail;

_I13HANDLER:		  // IVG 13 Handler
	r0.l = 13;
	jump display_fail;
 
_I14HANDLER:		  // IVG 14 Handler
	r0.l = 14;
	jump display_fail;

_I15HANDLER:		  // IVG 15 Handler
	r0.l = 15;
	jump display_fail;
	
	

