/* MANAGED-BY-SYSTEM-BUILDER                                    */

/*
** Power_On_Self_Test_basiccrt.s generated on Aug 24, 2007 at 15:20:53.
**
** Copyright (C) 2000-2007 Analog Devices Inc., All Rights Reserved.
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
**
** This file is generated automatically based upon the options selected
** in the Startup Code Wizard. Changes to the startup configuration
** should be made by changing the appropriate options rather than
** editing this file. Additional user code to be executed before calling
** main can be inserted between the labels .start_of_user_code1 and
** .end_of_user_code1 or .start_of_user_code2 and .end_of_user_code2.
** This code is preserved if the CRT is re-generated.
**
** Configuration:-
**     processor:                    ADSP-BF548
**     si_revision:                  automatic
**     cplb_init:                    false
**     mem_init:                     false
**     device_init:                  false
**     init_regs:                    false
**     zero_return_regs:             false
**     use_profiling:                false
**     using_cplusplus:              false
**     set_clock_and_power:          false
**
*/

/////////////////////////////////////////////////////////////////
// blackfin-edinburgh-core
#include <sys/platform.h>
#include <sys/anomaly_macros_rtl.h>

/////////////////////////////////////////////////////////////////
// standard
#define IVBh (EVT0 >> 16)
#define IVBl (EVT0 & 0xFFFF)
#define UNASSIGNED_VAL 0x8181
#define INTERRUPT_BITS 0x400	// just IVG15
#define SYSCFG_VALUE 0x30

	.section/DOUBLEANY program;
	.file_attr requiredForROMBoot;
	.align 2;

start:

/////////////////////////////////////////////////////////////////
// standard
#if WA_05000229
	// Avoid Anomaly 05-00-0229: DMA5_CONFIG and SPI_CTL not cleared on reset.
	R1 = 0x400;
#if defined(__ADSPBF538__) || defined(__ADSPBF539__)
	P0.L = SPI0_CTL & 0xFFFF;
	P0.H = SPI0_CTL >> 16;
	W[P0] = R1.L;
#else
	P0.L = SPI_CTL & 0xFFFF;
	P0.H = SPI_CTL >> 16;
	W[P0] = R1.L;
#endif
	P0.L = DMA5_CONFIG & 0xFFFF;
	P0.H = DMA5_CONFIG >> 16;
	R1 = 0;
	W[P0] = R1.L;
#endif
	// Clear loop counters to disable hardware loops
	R7 = 0;
	LC0 = R7;
	LC1 = R7;

	// Clear the DAG Length regs, to force linerar addressing
	L0 = R7;
	L1 = R7;
	L2 = R7;
	L3 = R7;

	// Clear ITEST_COMMAND and DTEST_COMMAND registers
	I0.L = (ITEST_COMMAND & 0xFFFF);
	I0.H = (ITEST_COMMAND >> 16);
	I1.L = (DTEST_COMMAND & 0xFFFF);
	I1.H = (DTEST_COMMAND >> 16);
	[I0] = R7;
	[I1] = R7;
	CSYNC;

	// Initialise the Event Vector table.
	P0.H = IVBh;
	P0.L = IVBl;

	// Install __unknown_exception_occurred in EVT so that
	// there is defined behaviour.
	P0 += 2*4;		// Skip Emulation and Reset
	P1 = 13;
	R1.L = __unknown_exception_occurred;
	R1.H = __unknown_exception_occurred;
	LSETUP (.ivt,.ivt) LC0 = P1;
.ivt:	[P0++] = R1;

	// Set IVG15's handler to be the start of the mode-change
	// code. Then, before we return from the Reset back to user
	// mode, we'll raise IVG15. This will mean we stay in supervisor
	// mode, and continue from the mode-change point, but at a
	// much lower priority.
	P1.H = supervisor_mode;
	P1.L = supervisor_mode;
	[P0] = P1;

/////////////////////////////////////////////////////////////////
// standard
	// Initialise the stack.
	// Note: this points just past the end of the section.
	// First write should be with [--SP].
	SP.L=ldf_stack_end;
	SP.H=ldf_stack_end;
	usp = sp;

	// We're still in supervisor mode at the moment, so the FP
	// needs to point to the supervisor stack.
	FP = SP;

	// Make space for incoming "parameters" for functions
	// we call from here:
	SP += -12;

	R0 = INTERRUPT_BITS;
	R0 <<= 5;	// Bits 0-4 not settable.
	CALL.X __install_default_handlers;

	R1 = SYSCFG;
	R4 = R0;		// Save modified list
	BITSET(R1,1);
	SYSCFG = R1;	// Enable the cycle counter


/*$VDSG<insert-code-early-startup>                              */
.start_of_user_code1:
  // Insert additional code to be executed before main here.
  // This code is preserved if the CRT is re-generated.
.end_of_user_code1:
/*$VDSG<insert-code-early-startup>                              */

/////////////////////////////////////////////////////////////////
// standard
	//  Enable interrupts
	STI R4;		// Using the mask from default handlers
	RAISE 15;

	// Move the processor into user mode.
	P0.L=still_interrupt_in_ipend;
	P0.H=still_interrupt_in_ipend;
	RETI=P0;
	NOP;		// Purely to prevent a stall warning

still_interrupt_in_ipend:
	// execute RTI until we've `finished` servicing all
	// interrupts of priority higher than IVG15. Normally one
	// would expect to only have the reset interrupt in IPEND
	// being serviced, but occasionally when debugging this may
	// not be the case - if restart is hit when servicing an
	// interrupt.
	//
	// When we clear all bits from IPEND, we'll enter user mode,
	// then we'll automatically jump to supervisor_mode to start
	// servicing IVG15 (which we will 'service' for the whole
	// program, so that the program is in supervisor mode.
	// Need to do this to 'finish' servicing the reset interupt.
	RTI;

supervisor_mode:
	[--SP] = RETI;	// re-enables the interrupt system
	R0.L = UNASSIGNED_VAL;
	R0.H = UNASSIGNED_VAL;

	// Push a RETS and Old FP onto the stack, for sanity.
	[--SP]=R0;
	[--SP]=R0;
	// Make sure the FP is sensible.
	FP = SP;
	// Leave space for incoming "parameters"
	SP += -12;


/*$VDSG<insert-code-before-device-initialization>               */
.start_of_user_code2:
  // Insert additional code to be executed before device initialization here.
  // This code is preserved if the CRT is re-generated.
.end_of_user_code2:
/*$VDSG<insert-code-before-device-initialization>               */


/*$VDSG<insert-code-before-main-entry>                          */
.start_of_user_code3:
  // Insert additional code to be executed before main here.
  // This code is preserved if the CRT is re-generated.
.end_of_user_code3:
/*$VDSG<insert-code-before-main-entry>                          */

/////////////////////////////////////////////////////////////////
// get-args
	// Read command-line arguments.
	CALL.X __getargv;
	r1.l=__Argv;
	r1.h=__Argv;

.extern __getargv;
.type __getargv,STT_FUNC;
.extern __Argv;
.type __Argv,STT_OBJECT;

/////////////////////////////////////////////////////////////////
// standard
	// Call the application program.
	CALL.X _main;

/////////////////////////////////////////////////////////////////
// call-exit
	CALL.X _exit;	// passing in main's return value
.extern _exit;
.type _exit,STT_FUNC;

/////////////////////////////////////////////////////////////////
// standard
.start.end:		// Required by the linker to know the size of the routine
                // that is needed for absolute placement.

.global start;
.type start,STT_FUNC;
.global .start.end;
.type .start.end,STT_FUNC;
.extern _main;
.type _main,STT_FUNC;
.extern ldf_stack_end;
.extern __unknown_exception_occurred;
.type __unknown_exception_occurred,STT_FUNC;
.extern __install_default_handlers;
.type __install_default_handlers,STT_FUNC;


/////////////////////////////////////////////////////////////////
// no-device-initialization
	// If File IO support isn't provided, then
	// we provide dummy versions of the device-handling
	// functions, so that the exception handlers don't rely
	// on the file IO library
	.section/DOUBLEANY program;
	.align 2;
_dev_open:
_dev_close:
_dev_write:
_dev_read:
_dev_seek:
_dev_dup:
	R0 = -1;
	RTS;
._dev_open.end:
._dev_close.end:
._dev_write.end:
._dev_read.end:
._dev_seek.end:
._dev_dup.end:

.global _dev_open;
.type _dev_open,STT_FUNC;
.global _dev_close;
.type _dev_close,STT_FUNC;
.global _dev_write;
.type _dev_write,STT_FUNC;
.global _dev_read;
.type _dev_read,STT_FUNC;
.global _dev_seek;
.type _dev_seek,STT_FUNC;
.global _dev_dup;
.type _dev_dup,STT_FUNC;


