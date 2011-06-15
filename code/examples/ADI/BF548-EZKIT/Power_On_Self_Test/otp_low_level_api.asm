#include "low_level_api.h"

//#define DISABLE_CHECK_ECC_REGION

#ifdef DISABLE_CHECK_ECC_REGION
#warning CHECK ECC REGION is DISABLED!!!!!!!!
#endif


#ifdef __MOAB__
#define OTP_BASE     		0xFFC04300
//#warning BUILDING low_level_API for MOAB based Platform
#endif
#ifdef __KOOK__
#define OTP_BASE                0xFFC03600
//#warning BUILDING low_level_API for KOOKABURRA based Platform
#endif

#ifdef __BUILD_for_ROM__
#warning BUILDING low_level_API in ROM!
#else
//#warning BUILDING low_level_API in L1 RAM!
#endif


#define UPPER_BEN 0xFF00
#define LOWER_BEN 0x00FF


#define OTP_CONTROL_OFF		0x0
#define OTP_BEN_OFF			0x4
#define OTP_STATUS_OFF   	0x8
#define OTP_TIMING_OFF   	0xC
#define OTP_DATA0_OFF    	0x80
#define OTP_DATA1_OFF    	0x84
#define OTP_DATA2_OFF    	0x88
#define OTP_DATA3_OFF    	0x8C

#define OTP_PUB_PAR_BASE    0xE0
#define OTP_SEC_PAR_BASE	0x1E0

#define OTP_BYTE_INDEX_MASK	0x7
#define OTP_DREG_OFF_MASK	0xc
#define BYTE_OFF_MASK		0x3
#define ECC_SPACE_MASK		0xFF
#define OTP_MAX_ADDR		0x1FF

#define MAX_PROG_STEPS		10
#define BLIND_PROG_STEPS	2

#define OTP_CONTROL  (OTP_BASE + 0x0)
#define OTP_BEN      (OTP_BASE + 0x4)
#define OTP_STATUS   (OTP_BASE + 0x8)
#define OTP_TIMING   (OTP_BASE + 0xc)
#define OTP_DATA0    (OTP_BASE + 0x80)
#define OTP_DATA1    (OTP_BASE + 0x84)
#define OTP_DATA2    (OTP_BASE + 0x88)
#define OTP_DATA3    (OTP_BASE + 0x8c)


#define OTP_WREN     0x8000
#define OTP_RDEN     0x4000
#define OTP_IEN      0x0800
#define OTP_DONE     0x0001
#define OTP_ERROR    0x0002

#define DECODE_SINGLE_BIT_FIXED 8
#define DECODE_DOUBLE_ERROR 4
#define DECODE_INVSYN_ERROR 5
#define DECODE_INVALID_ERROR 6

#define OTP_READ_TIMING_MASK	(0x00007FFF)		// bits [14:0] of OTP TIMING

#define HAMencode 0
#define HAMdecode 1

#define IMM32(reg,val) reg=LO(val); reg##.H=HI(val)


.extern _HammingCode7264;


#ifdef __BUILD_for_ROM__
.section    otp_ecc_code;
#else
.section    L1_code;
#endif

.global     _otp_command;
.align      8;

_otp_command:
	// prologue
    LINK 32;                // Saves Return address and frame pointer
                            // Updates stack pointer to allocate 16 bytes for local vars                           
    [--SP] = (R7:4, P5:3);  // Pushing the "must preserve" registers on stack.
    SP += -12; 
    
    // body
    r7 = 0;					// Init error code register  (will stay always zero in this simple function)

    IMM32(p4, OTP_BASE);
  
otp_command.ceck_for_init:
	r3 = OTP_INIT;
    cc = r3 == r0;
    if !CC jump otp_command.ceck_for_close;
    
    [p4 + OTP_TIMING_OFF] = r1;	// let's do the timing init
    jump _otp_command.end;		// done
    
otp_command.ceck_for_close:
	r3 = OTP_CLOSE;
    cc = r3 == r0;
    if !CC jump _otp_command.end;

    r3 = 0 (z);
	w[p4 + OTP_CONTROL_OFF] = r3;	// clean up OTP registers for security reasons
	[p4 + OTP_DATA0_OFF] = r3;   
	[p4 + OTP_DATA1_OFF] = r3;   
	[p4 + OTP_DATA2_OFF] = r3;   
	[p4 + OTP_DATA3_OFF] = r3;   
    
	jump _otp_command.end;		// done
	
	// reserved for more commands in the future
	
	
    // end
_otp_command.end:
	JUMP otp.epilogue;
	



#ifdef __BUILD_for_ROM__
.section    otp_ecc_code;
#else
.section    L1_code;
#endif

.global     _otp_read;
.align      8;
_otp_read:
/******************** Function Prologue ***************************************/
_otp_read.prologue:
    LINK 32;                // Saves Return address and frame pointer
                            // Updates stack pointer to allocate 16 bytes for local vars                           
    [--SP] = (R7:4, P5:3);  // Pushing the "must preserve" registers on stack.
    SP += -12;              // Allocate space for outgoing arguments.  Don't need it,
                            // but it seems to be good programming practice.                           
    [FP + 8] = r0;          // Push target addres
    [FP + 0xc] = r1;        // Push flags parameter
    [FP + 0x10] = r2;		// Push pointer to data structure

    
   	r7 = 0;					// Init error code register   	

	r3 = OTP_MAX_ADDR;				// check that target addr is less than 0x1FF
	CC = r0 <= r3 (IU);		
	IF !CC JUMP access.violation.error;
   	
	
	IMM32(p4, OTP_BASE);
    
    cc = bittst ( r1, bitpos(OTP_NO_ECC) );		// cc is set if raw read is desired (No ECC)

    r4 = OTP_UPPER_HALF(z);
    r1 = r1 & r4;
    p1 = [FP + 0x10];		// Point to read result data structure

	if cc jump _otp_read.read_content;

   	CALL check.ECC.region;	// In ECC space?

		
_otp_read.read_ECC:
	call otp_read_raw_byte;			// Get ECC code for the requested page
	r3 = r5;						// save ECC code in R3, which is not used by the following routine

_otp_read.read_content:
	r0 = [FP +8];					// Get target page address off stack
	call otp_read_raw_half_page;	// Get raw data for the requested page

		
	// now see if we need to check ECC
	r1 = [FP + 0xc];				// Get flags parameter off stack
    cc = bittst ( r1, bitpos(OTP_NO_ECC) );		// cc is set if raw read is desired (No ECC)
	if cc jump otp.epilogue;
	
_otp_read.compareECC:
	// ECC code in R3, page content in [r5:r6]
		
/******************** Call Decode Function ***********************************/
decode.rd.data:
    r0 = [FP + 0x10];       		// Setup parameter:  addr of data struct
    r1 = HAMdecode;					// Setup parameter:  function is decode
    r2 = r3;						// Setup parameter:  ecc

    CALL _HammingCode7264;

    p1 = [FP + 0x10];       		// save off corrected value in OTP data 0 and 1
    nop;
    nop;
    nop;
	r5= [p1++];
	[p4 + OTP_DATA0_OFF] = r5;
	r5= [p1++];
	[p4 + OTP_DATA1_OFF] = r5;
	r5 = 0;
	[p4 + OTP_DATA2_OFF] = r5;
	[p4 + OTP_DATA3_OFF] = r5;
	
	        
/******************* Set return status, clean house, and return **************/
	CC = r0 == 0;
    if CC JUMP otp.epilogue;
    CC = r0 == 1;
    if CC JUMP decode.single.bit.fixed;
    CC = r0 == 2;
    if CC JUMP decode.double.error;
    CC = r0 == 3;
    if CC JUMP decode.invsyn.error;
    JUMP decode.invalid.error;
    
_otp_read.end: 



#ifdef __BUILD_for_ROM__
.section    otp_ecc_code;
#else
.section    L1_code;
#endif

.global     _otp_write;
.align      8;
_otp_write:
/******************** Function Prologue ***************************************/
_otp_write.prologue:
    LINK 32;                // Saves Return address and frame pointer
                            // Updates stack pointer to allocate 16 bytes for local vars                           
    [--SP] = (R7:4, P5:3);  // Pushing the "must preserve" registers on stack.
    SP += -12;              // Allocate space for outgoing arguments.  Don't need it,
                            // but it seems to be good programming practice.                           
    [FP + 8] = r0;          // Push target addres
    [FP + 0xc] = r1;        // Push flags parameter
    [FP + 0x10] = r2;		// Push pointer to data structure

  
    r7 = 0;					// Init error code register   
	
	IMM32(p4, OTP_BASE);	// Init P4 to OTP base
    
 	r3 = ~OTP_READ_TIMING_MASK;	// check that write timing is meaningful (i.e > 0)
 	r4 = [p4 + OTP_TIMING_OFF];
 	r4 = r4 & r3;
	CC = r4 == 0;		
	IF CC JUMP access.violation.error;

   	
	r3 = OTP_MAX_ADDR;				// check that target addr is less than 0x1FF
	CC = r0 <= r3 (IU);		
	IF !CC JUMP access.violation.error;


	CC = r2 == 0;									// check for NULL pointer as &data
	if CC jump _otp_write.write_protection_bit;		// maybe we need only to write a protection bit

	
	CALL check.ECC.region;				// In ECC space?

   
    cc = bittst ( r1, bitpos(OTP_NO_ECC) );		// cc is set if raw write is desired (No ECC)
	r4 = OTP_UPPER_HALF(z);
    r1 = r1 & r4;
    p3 = [FP + 0x10];		// Point to  data structure to be written

	if cc jump _otp_write.write_content;

_otp_write.check.if.already.written:
    p1 = FP;							// Point to temporary storage (scratch)
    p1 += -0xc;
	CALL otp_read_raw_half_page;		// [r5:r6] contain read data

compare.init.rd.data:					// now compare to zero
	r4 = 0;
	r4.l = ones r5;
	CC = r4 < 1 (iu);
	IF !CC JUMP prev.write.error;
	r4.l = ones r6;
	CC = r4 < 1 (iu);
	IF !CC JUMP prev.write.error;

_otp_write.calculateECC:
	r0 = [FP + 0x10];               // Setup parameter:  data structure addr
    r1 = HAMencode;					// Setup parameter:  function is 'encode'			
    r2 = 0;							// Setup parameter:  Init to zeros, no meaning for encode func
    
    CALL _HammingCode7264;

_otp_write.write_ECC:
	r6 = r0;						// Move ecc to r6
	r0 = [FP +8];				// Get target page address off stack
	r1 = [FP + 0xc];			// Get upper/lower half selection off stack
	CALL otp_write_raw_byte;		// Write ECC code for the requested page
	r3 = r5;						// save ECC code in R3, which is not used by the following routine

_otp_write.write_content:
	r0 = [FP +8];					// Get target page address off stack
	call otp_write_raw_half_page;	// Write raw data for the requested page
	cc = r7 == 0;
	if !cc jump _otp_write.end;

	
_otp_write.write_protection_bit:
			
	// now see if we need to protect a page
	r1 = [FP + 0xc];						// Get flags parameter off stack
    cc = bittst ( r1, bitpos(OTP_LOCK) );	// cc is set if locking the page is desired
	if !cc jump otp.epilogue;				// not set: we are done
	
	
	call otp_write_pageprotect_byte;
	
_otp_write.end: 

	jump otp.epilogue;





        
/******************************************************************************
************* all OTP API functions exit here**********************************
******************************************************************************/
	
	
otp.epilogue:
	CALL master.err.check;
    r0 = r7;				// Move error status to return reg r0
    
    SP += 12;               // Reclaims space on stack used for outgoing args
    p0 = [FP+4];            // Loads the return address into p0
    (R7:4,P5:3)=[SP++];     // Pop the "must preserve" registers before returning.
    r1 = 0; r2 = 0; r3 = 0;	// Clear out registers for security reasons
    p1 = 0; p2 = 0; 	        // Clear out registers for security reasons
    UNLINK;                 // Restores FP and SP
    JUMP (p0);              // Return to caller
    NOP;                    //to avoid one stall if LINK or UNLINK happens to be
                            //the next instruction after RTS in the memory.
otp.epilogue.end:	







/**********************************************************************************************/
/*************************** Handle Error Codes **********************************************/	
/**********************************************************************************************/
ecc.read.error:
read.error:
	CALL clear.error.bit;
	CALL clear.done.bit;
	BITSET(r7, OTP_READ_ERROR);
	JUMP otp.epilogue;
	
decode.single.bit.fixed:
	BITSET(r7,DECODE_SINGLE_BIT_FIXED);
	JUMP otp.epilogue;

decode.double.error:
	BITSET(r7,DECODE_DOUBLE_ERROR);
	JUMP otp.epilogue;
	
decode.invsyn.error:
	BITSET(r7,DECODE_INVSYN_ERROR);
	JUMP otp.epilogue;
	
decode.invalid.error:
	BITSET(r7,DECODE_INVALID_ERROR);
	JUMP otp.epilogue;                          

	
write.error:
	CALL clear.error.bit;
	CALL clear.done.bit;
	BITSET(r7, OTP_WRITE_ERROR);
	JUMP otp.epilogue;
	
access.violation.error:
	BITSET(r7, OTP_ACC_VIO_ERROR);
	JUMP otp.epilogue;
	
prev.write.error:
	BITSET(r7, OTP_PREV_WR_ERROR);
	JUMP otp.epilogue;










    
/**********************************************************************************************/
/****************** Various Subroutines and utilities *****************************************/
/**********************************************************************************************/

clear.done.bit:
	r5 = OTP_DONE;
	w [p4 + OTP_STATUS_OFF] = r5;
	ssync;
	RTS;
clear.done.bit.end:


clear.error.bit:
	r5 = OTP_ERROR;
	w [p4 + OTP_STATUS_OFF] = r5;		// Write 1 to clear sticky error bit
	ssync;
	RTS;
clear.error.bit.end:


#ifdef DISABLE_CHECK_ECC_REGION

check.ECC.region:
	r3 = OTP_PUB_PAR_BASE;			// check that target addr is NOT in ECC space
	CC = ! BITTST(r0, 8);
	IF CC JUMP save.base;		
secret:
	r3 = OTP_SEC_PAR_BASE;			// r3 holds base address for secure OTP parity fields
save.base:
	[FP - 0x4] = r3;				// push OTP parity field base addr onto stack
	RTS;
check.ECC.region.end:		

#else

check.ECC.region:
	r3 = OTP_PUB_PAR_BASE;			// check that target addr is NOT in ECC space
	r4 = ECC_SPACE_MASK;
	r4 = r0 & r4;
	CC = r4 < r3 (IU);
	IF !CC JUMP access.violation.error;

	CC = ! BITTST(r0, 8);
	IF CC JUMP save.base;		
secret:
	r3 = OTP_SEC_PAR_BASE;			// r3 holds base address for secure OTP parity fields
save.base:
	[FP - 0x4] = r3;				// push OTP parity field base addr onto stack
	RTS;
check.ECC.region.end:		

#endif		// #ifdef DISABLE_CHECK_REGION



master.err.check:
	CC = BITTST(r7,1);				// master error = {OR[OR(r7[7:1]), AND(r7[9:8])]}
	IF CC JUMP set.master.err.bit;
	CC = BITTST(r7,2);
	IF CC JUMP set.master.err.bit;
	CC = BITTST(r7,3);
	IF CC JUMP set.master.err.bit;
	CC = BITTST(r7,4);
	IF CC JUMP set.master.err.bit;
	CC = BITTST(r7,5);
	IF CC JUMP set.master.err.bit;
	CC = BITTST(r7,6);
	IF CC JUMP set.master.err.bit;
	CC = BITTST(r7,7);
	IF CC JUMP set.master.err.bit;
	r5 = r7>>8;							// Shift r7 so bits [1:0] are the single bit warning bits
	CC = r5<= 2 (iu);
	IF !CC JUMP set.master.err.bit;		// 2 single bit warnings = error
	JUMP return.to.epilogue;

set.master.err.bit:
	BITSET(r7, OTP_MASTER_ERROR);

return.to.epilogue:
	RTS;
master.err.check.end:




/************ Read 64 bit data from OTP **************************************
* P1 = Point to result data structure
* P4 = Point to OTP base MMR address
* R0 = page address
* R1 (bit 0) = upper/lower half
* not used: R3, R4
******************************************************************************/
otp_read_raw_half_page:
    cc = bittst ( r1, bitpos(OTP_UPPER_HALF) );		// cc is set if upper half
    if cc jump ben.upper.half;

ben.lower.half:
	r5 = LOWER_BEN (z);
	JUMP load.ben;

ben.upper.half:
	r5 = UPPER_BEN (z);
	
load.ben:
	w [p4 + OTP_BEN_OFF] = r5;		// Load FUSE BEN reg
	
rd.data:
	r5 = OTP_RDEN;
	r5 = r5 | r0;						// r0 contains OTP page address
	w[p4 + OTP_CONTROL_OFF] = r5;							// Init read of OTP	
	ssync;

rd.not.done:
	r5 = w[p4+OTP_STATUS_OFF] (z);
	CC = bittst(r5, bitpos(OTP_ERROR));
	if CC jump ecc.read.error;
	CC = bittst(r5, bitpos(OTP_DONE));
	if !CC jump rd.not.done;
    [--SP] = rets;
	CALL clear.done.bit;		
    rets = [SP++];
    cc = bittst ( r1, bitpos(OTP_UPPER_HALF) );		// cc is set if upper half
    if cc jump rd.upper.half;

rd.lower.half:
	r5 = [p4 + OTP_DATA0_OFF];		// Read lower FUSE Data regs
	r6 = [p4 + OTP_DATA1_OFF];
	JUMP place.data;
rd.upper.half:
	r5 = [p4 + OTP_DATA2_OFF];		// Read upper FUSE Data regs
	r6 = [p4 + OTP_DATA3_OFF];

place.data:    
    [p1++] = r5;			// position upper 32 bit data
    [p1++] = r6;			// position lower 32 bit data
    
    rts;
    

/************ Read 8 bit ECC data from OTP ************************************
* P4 = Point to OTP base MMR address
* R0 = page address
* R1 (bit 0) = upper/lower half
* return: byte in r5[7:0]
* not used: none of Rx registers
******************************************************************************/
otp_read_raw_byte:
	r4 = OTP_BYTE_INDEX_MASK (z);
	r6 = r0 & r4;
	r6 <<= 1;
	r6 = r6 | r1;					// r6 is the INDEX [t[2:0], s]

	r4 = OTP_DREG_OFF_MASK (z);
	r5 = r6 & r4;					// Extract OTP_DATA reg index t[2:1]
	BITSET(r5, 7);
	p2 = r5;						// Load p2 with OTP_DATA reg addr offset

	r4 = 1 (z);
	r4 <<= r6;						// Generate BEN word
	
	w [p4 + OTP_BEN_OFF] = r4;		// Load FUSE BEN reg

	r4 = ECC_SPACE_MASK;
	r0 = r0 & r4;			 		// Use only lower 8 bits of target address
	r0 >>= 3;						// Then divide by 8.
	r4 = OTP_RDEN;	
	r3 = [FP - 0x4];         		// Recall parity field base addr from stack
	r3 = r3 + r0;					// Calculate page address for parity field
	r5 = r4 | r3;

	p0 = p4 + p2;					// Load p0 with OTP_DATA reg addr
		
read.ecc:		
	w [p4 + OTP_CONTROL_OFF] = r5;	// Load FUSE CNTRL reg and initiate read
	ssync;

ecc.rd.not.done:
	r5 = w[p4+OTP_STATUS_OFF] (z);
	CC = bittst(r5, bitpos(OTP_ERROR));
	if CC jump ecc.read.error;
	CC = bittst(r5, bitpos(OTP_DONE));
	if !CC jump ecc.rd.not.done;

    [--SP] = rets;
	CALL clear.done.bit;	
    rets = [SP++];

	r5 = [p0];						// Read OTP_DATA reg using p0
	
	r4 = BYTE_OFF_MASK(z);
	r6 = r6 & r4;					// Extract [t[0], s]
	r6 <<= 3;						// Multiply by 8
	r5 >>= r6;						// Right justify the ecc data in r5    
        
    rts;    
    
        

/************ write 64 bit data to OTP **************************************
* P3 = Point to write data structure
* P4 = Point to OTP base MMR address
* R0 = page address
* R1 (bit 0) = upper/lower half
* not used: none of the registers
******************************************************************************/
otp_write_raw_half_page:

 	r5 = ~OTP_READ_TIMING_MASK;	// check that write timing is meaningful (i.e > 0)
 	r6 = [p4 + OTP_TIMING_OFF];
 	r6 = r6 & r5;
	CC = r6 == 0;		
	IF !CC JUMP program.do.it;
	BITSET(r7, OTP_ACC_VIO_ERROR);	// write timing field is 0, so access violation
	JUMP otp_write_raw_half_page_return;

program.do.it:

	r2 = 0;					// Init write counter	
	 
	r3 = [p3 ];			// get data				
    r4 = [p3 + 0x4];
    
load.data.regs:
    cc = bittst ( r1, bitpos(OTP_UPPER_HALF) );		// cc is set if upper half
    IF CC JUMP wr.upper.half;

wr.lower.half:
	[p4 + OTP_DATA0_OFF] = r3;		//Load lower FUSE Data regs
	[p4 + OTP_DATA1_OFF] = r4;
	r5 = LOWER_BEN (z);
	JUMP wr.ben;

wr.upper.half:
	[p4 + OTP_DATA2_OFF] = r3;		// Load upper FUSE Data regs
	[p4 + OTP_DATA3_OFF] = r4;
	r5 = UPPER_BEN (z);
	
wr.ben:
	w [p4 + OTP_BEN_OFF] = r5;		// Load FUSE BEN reg
	r5 = OTP_WREN (z);
	r6 = r5 | r0;

program:
	w [p4 + OTP_CONTROL_OFF] = r6;	// Load FUSE CNRL reg and initiate write
	ssync;

    
    
// Check 64bit data fuse array write status.
// Program each fuse twice.

	r2 += 1;						// Increment write counter

wr.not.done:
	r5 = w[p4 + OTP_STATUS_OFF](z);
	CC = bittst(r5, bitpos(OTP_ERROR));
	if CC jump write.error;
	CC = bittst(r5, bitpos(OTP_DONE));
	if !CC jump wr.not.done;
	
    [--SP] = rets;
	CALL clear.done.bit;	
    rets = [SP++];

	
	CC = r2 < BLIND_PROG_STEPS;
	if CC jump load.data.regs;

// After programming twice, read back to check for success
// If it still isn't programmed, program bad bits ONLY again and again and again...

rdbk.data:
	r5 = OTP_RDEN;
	r5 = r5 | r0;						// r0 contains OTP page address
	w[p4 + OTP_CONTROL_OFF] = r5;		// Init read of OTP	
	ssync;
rdbk.not.done:
	r5 = w[p4+OTP_STATUS_OFF](z);
	CC = bittst(r5, bitpos(OTP_ERROR));
	if CC jump read.error;
	CC = bittst(r5, bitpos(OTP_DONE));
	if !CC jump rdbk.not.done;

	[--SP] = rets;
	CALL clear.done.bit;	
    rets = [SP++];

    cc = bittst ( r1, bitpos(OTP_UPPER_HALF) );		// cc is set if upper half
    IF CC JUMP rdbk.upper.half;
rdbk.lower.half:
	r5 = [p4 + OTP_DATA0_OFF];		// Read lower FUSE Data regs
	r6 = [p4 + OTP_DATA1_OFF];
	JUMP compare.rdbk.data;
rdbk.upper.half:
	r5 = [p4 + OTP_DATA2_OFF];		// Read upper FUSE Data regs
	r6 = [p4 + OTP_DATA3_OFF];
	
compare.rdbk.data:
	r3 = [p3];					
    r4 = [p3 + 0x4];
    r3 = r3 ^ r5;
	CC = AZ;
    r4 = r4 ^ r6;
	CC &= AZ;
	IF CC JUMP otp_write_raw_half_page_return;	// success
	
rdbk.mismatch:
	r6 = MAX_PROG_STEPS;
	r6 = r6 - r2;
	CC = r6 <= 0;
	IF !CC JUMP load.data.regs;

fail.on.mps.wr:						// Max number of programs reached	
	r0 = 0;
	r1 = 0;
	r5 = 0;
	r1.l = ones r3;					
	r5.l = ones r4;
	r0 = r1 + r5;				// Count number of failing bits ERROR needs to check r3 and r4
	CC = r0 == 1;
	IF CC JUMP single.bit.error;
	BITSET(r7, OTP_DATA_MULT_ERROR);
	JUMP otp.epilogue;

single.bit.error:
	BITSET(r7, OTP_DATA_SB_WARN);	// Set the SB warning flag.  Continue.
		
otp_write_raw_half_page_return:
	// return		
    rts;
    
        
    

/************ Write 8 bit ECC data to OTP ************************************
* P4 = Point to OTP base MMR address
* R0 = page address
* R1 (bit 0) = upper/lower half
* R6: ECC byte in r6[7:0]
* not used: none of Rx registers
******************************************************************************/
otp_write_raw_byte:
	r2 = 0 (z);					// Init write counter

	r4 = 1 (z);
	r1 = r1 & r4;
	
	r4 = OTP_BYTE_INDEX_MASK (z);
	r5 = r0 & r4;
	r5 <<= 1;
	r5 = r5 | r1;				// r5 is the INDEX [t[2:0], s]
	
	r4 = OTP_DREG_OFF_MASK (z);
	r4 = r5 & r4;				// Extract OTP_DATA reg index t[2:1]
	BITSET(r4, 7);
	p1 = r4;					// Load p1 with OTP_DATA reg addr offset

	r4 = BYTE_OFF_MASK(z);
	r4 = r5 & r4;				// Extract [t[0], s]
	r4 <<= 3;					// Multiply by 8
	r6 <<= r4;					// Align ecc to proper byte within 32bit data reg

	p0 = p4 + p1;				// Load p0 with OTP_DATA reg addr	

	
	r4 = 1 (z);
	r4 <<= r5;					// Generate BEN word
	
	w [p4 + OTP_BEN_OFF] = r4;		// Load FUSE BEN reg

	r4 = ECC_SPACE_MASK;
	r0 = r0 & r4;			 		// Use only lower 8 bits of target address
	r0 >>= 3;						// Then divide by 8.
	r4 = OTP_WREN (z);
	r3 = [FP - 0x4];         		// Recall parity field base addr from stack

	r3 = r3 + r0;					// Calculate page address for parity field
	r4 = r4 | r3;

load.ecc.data.reg:
	[p0] = r6;						// Write aligned ecc to OTP_DATAx reg

	
program.ecc:		
	w [p4 + OTP_CONTROL_OFF] = r4;	// Load FUSE CNTRL reg and initiate write
	ssync;



// Check otp ecc write status
// Program each fuse BLIND_PROG_STEPS times.

	r2 += 1;						// Increment write counter

ecc.wr.not.done:
	r5 = w[p4 + OTP_STATUS_OFF](z);
	CC = bittst(r5, bitpos(OTP_ERROR));
	if CC jump write.error;
	CC = bittst(r5, bitpos(OTP_DONE));
	if !CC jump ecc.wr.not.done;
	
    [--SP] = rets;
	CALL clear.done.bit;	
    rets = [SP++];
		
	CC = r2 < BLIND_PROG_STEPS;
	if CC jump load.ecc.data.reg;

// After programming twice, read back to check for success
// If it still isn't programmed, program bad bits again and again...

ecc.readback:

	r5 = OTP_RDEN;
	r5 = r5 | r3;
	w[p4 + OTP_CONTROL_OFF] = r5;	// Init read of OTP
	ssync;
	
ecc.rdbk.not.done:
	r5 = w[p4+OTP_STATUS_OFF](z);
	CC = bittst(r5, bitpos(OTP_ERROR));
	if CC jump read.error;
	CC = bittst(r5, bitpos(OTP_DONE));
	if !CC jump ecc.rdbk.not.done;

    [--SP] = rets;
	CALL clear.done.bit;	
    rets = [SP++];

	r5 = [p0];								// Read OTP_DATA reg using p0 - retained OTP_DATA reg addr

compare.ecc.rd.data:
	r6 = r6 ^ r5;							// XOR rdbk data with original aligned ecc data retained in r2
	CC = AZ;							
	IF CC JUMP otp_write_raw_byte_return;	// success
	
ecc.rd.mismatch:
	r5 = MAX_PROG_STEPS;
	r5 = r5 - r2;
	CC = r5 <= 0;
	IF CC JUMP fail.on.ecc.mps.wr;
	[p0] = r6;							// Write updated data to OTP_DATAx reg
	JUMP program.ecc;	

fail.on.ecc.mps.wr:						// Max number of ecc programs reached	
	r0 = 0;
	r0.l = ones r6;						// Count number of failing bits
	CC = r0 == 1;
	IF CC JUMP single.bit.ecc.error;
	BITSET(r7, OTP_ECC_MULT_ERROR);
	JUMP otp.epilogue;					// error: abort
		
single.bit.ecc.error:
	BITSET(r7, OTP_ECC_SB_WARN);		// Set warning flag for SB in ecc	
	
otp_write_raw_byte_return:
	// return
	rts;
	
	

/************ Write 8 bit page protection data to OTP ************************************
* P4 = Point to OTP base MMR address
* R0 = page address
* not used: none of Rx registers
******************************************************************************/
otp_write_pageprotect_byte:
	r2 = 0 (z);					// Init write counter

	r3 = 0x1F(z);				// calculate the data byte ( 2^ pg[4:0])
	r4= r0 & r3;
	r6 = 1(z);
	r6 <<= r4;					// r6 contains byte value (one "1" for the page to protect)

	r3 = 0xF(z);				// calculate BEN word ( 2^ pg[6:3])
	r5 = r0 >> 3;
	r5 = r5 & r3;
	r4 = 1 (z);
	r4 <<= r5;					// Generate BEN word
	
	w [p4 + OTP_BEN_OFF] = r4;		// Load FUSE BEN reg

	
	r3 = 0x3(z);				// calculate OTP data register ( pg[6:5])
	r4 = r0 >> 5;
	r4 = r4 & r3;
	r4 <<= 2;
	BITSET(r4, 7);
	p1 = r4;
	nop;
	nop;
	nop;
	nop;
	p0 = p4 + p1;				// Load p0 with OTP_DATA reg addr	
	
	[p0] = r6;					// Write aligned ecc to OTP_DATAx reg
	

	
	

	r3 = r0 >>7;				// lock bit page = pg[8:7]
	r4 = OTP_WREN (z);
	r4 = r4 | r3;
	
	jump program.ecc;			// program the byte (will return to caller from there)
	
otp_write_pageprotect_byte.end:
			

    
        
    
    
 
