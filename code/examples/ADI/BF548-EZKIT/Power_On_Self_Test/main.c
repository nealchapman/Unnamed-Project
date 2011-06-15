/*******************************************************************
Analog Devices, Inc. All Rights Reserved.

This software is proprietary and confidential.  By using this software
you agree to the terms of the associated Analog Devices License Agreement.

Project Name:  	Power_On_Self_Test

Hardware:		ADSP-BF548 EZ-KIT Lite

Description:	This examples performs POST on the BF548 EZ-Kit Lite

NOTE: 			The Power_On_Self_Test is too large of an example to work 
				with a DEMO license and can only be run in it's entirety 
				using a full VDSP++ license.

WARNING: 		Because the hard drive test currently writes and verifies raw 
				data, the hard drive will need to be reformatted after running 
				the test.

BF548 EZ-KIT board test indicators 	(version 1.00.0 11 May 2006)
----------------------------------

NOTE:	The switch settings for the built in self test could differ slightly from the
		default settings shipped. See the users manual for default switch settings.

"Common jumper settings for all tests:"

	-  JP2: NOT installed
	-  JP3: NOT installed
	-  JP4: installed
	-  JP5: installed
	-  JP6: installed
	-  JP7: NOT installed
	-  JP8: NOT installed


I. Enter BF548 EZ-KIT POST test by pressing PB1.
(All other LEDs not shown below do not matter and may be on or off.)

Test indicators are as follows:
-------------------------------
				LED5	LED4	LED3	LED2	LED1
VERSION			0		0   	0   	0   	1
PBLED			0		0		0		1		0
ROTARY			0		0		0		1		1
KEYPAD			0		0		1		0		0
TOUCHLCD		0		0		1		0		1
SD				0		0		1		1		0
USB_DEV			0		0		1		1		1
USB_HOST		0		1		0		0		0
MACREADWRITE	0		1		0		0		1
PROGRAM_PUB_KEY	0		1		0		1		0
SET_RTC			0		1		0		1		1
VERIFY_RTC		0		1		1		0		0
NAND			0		1		1		0		1
DDR				0		1		1		1		0
UART			0		1		1		1		1
SPIFLASH		1		0		0		0		0
BURSTFLASH		1		0		0		0		1
ETHERNET		1		0		0		1		0
HDD				1		0		0		1		1
CHECK_PUB_KEY	1		0		1		0		0
AUDIO			1		0		1		0		1
CHECK_MAC		1		0		1		1		0

AV_VIDEO		1		0		1		1		1
AV_AUDIO		1		1		0		0		0

FPGA_MEMORY		1		1		0		0		1
FPGA_GPIO		1		1		0		1		0
FPGA_SPORTS		1		1		0		1		1

CAN				1		1		1		0		0
MXVR			1		1		1		0		1

"Switch settings:"

	- SW2		1 = ON,  2 = ON,  3 = ON,  4 = ON  5 = ON, 6 = ON, 7 = ON,  8 = ON.
	- SW4		1 = ON,  2 = ON,  3 = ON,  4 = OFF.
	- SW5		1 = ON,  2 = ON,  3 = ON,  4 = ON.
	- SW6		1 = OFF, 2 = OFF, 3 = ON,  4 = ON.
	- SW7		1 = ON,  2 = ON,  3 = ON,  4 = OFF.
	- SW8		1 = ON,  2 = ON,  3 = ON,  4 = ON, 5 = ON, 6 = ON.
	- SW14		1 = ON,  2 = ON,  3 = ON,  4 = ON.
	- SW15		1 = OFF, 2 = OFF, 3 = ON,  4 = ON.
	- SW16		1 = ON,  2 = ON,  3 = ON,  4 = ON.
	- SW17		1 = ON,  2 = OFF, 3 = ON,  4 = OFF.


"Jumper settings:"

use common settings with the following additions
	- JP1	ON
	- JP11 	1&2 installed, 3&4 installed( right to left with SPORT 2 connector below )

*******************************************************************/

/*******************************************************************
*  include files
*******************************************************************/
#include <string.h>
#include "Timer_ISR.h"
#include "post_common.h"
#include "PBLED_test.h"
#include "adi_ssl_Init.h"


/*******************************************************************
*  function prototypes
*******************************************************************/
int TEST_VERSION(void);
int TEST_PBLED(void);
int TEST_ROTARY(void);
int TEST_KEYPAD(void);
int TEST_TOUCHLCD(void);
int TEST_SD(void);
int TEST_USB_DEVICE(void);
int TEST_USB_HOST(void);
int MAC_READ_WRITE(void);
int PROGRAM_PUBLIC_KEY(void);
int SET_RTC_TIME(void);
int VERIFY_RTC_UPDATE(void);
int TEST_NAND(void);
int TEST_DDR(void);
int TEST_UART(void);
int TEST_SPIFLASH(void);
int TEST_BURSTFLASH(void);
int TEST_ETHERNET(void);
int TEST_HDD(void);
int IS_PUBLIC_KEY_LOCKED_AND_PROGRAMMED(void);
int TEST_AUDIO(void);
int CHECK_FOR_MAC_ADDRESS(void);
int SHOW_IMAGES(void);


// POST test for the AV EZ-Extender board
int TEST_AV_VIDEO(void);
int TEST_AV_AUDIO(void);


// POST tests for the FPGA EZ-Extender board
int TEST_FPGA_MEMORY(void);
int TEST_FPGA_GPIO(void);
int TEST_FPGA_SPORTS(void);
int PROGRAM_FPGA(void);

// programming public key via OTP
int PROGRAM_PUBLIC_KEY(void);

// BF549 tests
int TEST_CAN(void);
int TEST_MXVR(void);

int TEST_DUMMY(void) {return 1;}// a dummy test for targets which do not have a specific piece of hardware


/*******************************************************************
*  global variables
*******************************************************************/
static int g_loops = 0;
char g_szVersion[64] = "1.00.0";
char g_szBuildDate[64];
char g_szBuildTime[64];

typedef enum TEST_INDEX_tag{
	VERSION,			// 00001
	PBLED,				// 00010
	ROTARY,				// 00011
	KEYPAD,				// 00100
	TOUCHLCD,			// 00101
	SD,					// 00110
	USB_DEV,			// 00111
	USB_HOST,			// 01000
	MACREADWRITE,		// 01001
	PROG_PUB_KEY,		// 01010
	SET_RTC,			// 01011
	VERIFY_RTC,			// 01100
	NAND,				// 01101
	DDR,				// 01110
	UART,				// 01111
	SPIFLASH,			// 10000
	BURSTFLASH,			// 10001
	ETHERNET,			// 10010
	HDD,				// 10011
	CHECK_PUB_KEY,		// 10100
	AUDIO,				// 10101
	CHEC_MAC,			// 10110

	NUM_TESTS_IMPLEMENTED,	// this is the end of repeating tests

	FIRST_AV_TEST = NUM_TESTS_IMPLEMENTED,
	AV_VIDEO = FIRST_AV_TEST,	// 10111
	AV_AUDIO,					// 11000
	LAST_AV_TEST,



	FIRST_FPGA_TEST = LAST_AV_TEST,
	FPGA_MEMORY = FIRST_FPGA_TEST,	// 11001
	FPGA_GPIO,						// 11010
	FPGA_SPORTS,					// 11011
	LAST_FPGA_TEST,

	FIRST_BF549_TEST = LAST_FPGA_TEST,
	CAN,					// 11100
	MXVR,					// 11101
	LAST_BF549_TEST,

	NUM_TESTS = LAST_BF549_TEST


}enTEST_INDEX;


typedef int (*pfnTests)(void);

typedef struct stTestParams_TAG
{
	enTESTS m_nTest;
	enTEST_STATES m_nTestState;
	pfnTests m_pTestFunctions;

}stTestParamaters;


stTestParamaters g_Tests[NUM_TESTS] = {
	{TEST_1, TEST_1_SET, TEST_VERSION},
 	{TEST_2, TEST_2_SET, TEST_PBLED},
 	{TEST_3, TEST_3_SET, TEST_ROTARY},
 	{TEST_4, TEST_4_SET, TEST_KEYPAD},
 	{TEST_5, TEST_5_SET, TEST_TOUCHLCD},
 	{TEST_6, TEST_6_SET, TEST_SD},
 	{TEST_7, TEST_7_SET, TEST_USB_DEVICE},
 	{TEST_8, TEST_8_SET, TEST_USB_HOST},
 	{TEST_9, TEST_9_SET, MAC_READ_WRITE},
 	{TEST_10, TEST_10_SET, PROGRAM_PUBLIC_KEY},
 	{TEST_11, TEST_11_SET, SET_RTC_TIME},
 	{TEST_12, TEST_12_SET, VERIFY_RTC_UPDATE},
 	
 	{TEST_13, TEST_13_SET, TEST_NAND},
 	{TEST_14, TEST_14_SET, TEST_DDR},
 	{TEST_15, TEST_15_SET, TEST_UART},
 	{TEST_16, TEST_16_SET, TEST_SPIFLASH},
 	{TEST_17, TEST_17_SET, TEST_BURSTFLASH},
 	{TEST_18, TEST_18_SET, TEST_ETHERNET},
 	{TEST_19, TEST_19_SET, TEST_HDD},
 	{TEST_20, TEST_20_SET, IS_PUBLIC_KEY_LOCKED_AND_PROGRAMMED},
 	{TEST_21, TEST_21_SET, TEST_AUDIO},
 	{TEST_22, TEST_22_SET, CHECK_FOR_MAC_ADDRESS},

 	// AV EZ-Extender tests
	{TEST_23,TEST_23_SET,TEST_AV_VIDEO},
	{TEST_24,TEST_24_SET,TEST_AV_AUDIO},

	// FPGA EZ-Extender tests
 	{TEST_25,TEST_25_SET,TEST_FPGA_MEMORY},
 	{TEST_26,TEST_26_SET,TEST_FPGA_GPIO},
 	{TEST_27,TEST_27_SET,TEST_FPGA_SPORTS},

 	// BF549 tests
 	{TEST_28, TEST_28_SET, TEST_CAN},
 	{TEST_29, TEST_29_SET, TEST_MXVR},
};


static ADI_INT_HANDLER(ExceptionHandler)	// exception handler
{
	/* Set break point here to debug */
	return(ADI_INT_RESULT_PROCESSED);
}

/*******************************************************************
*   Function:    PerformTest
*   Description: This is where the test parameters are figured out
*				 and the test is performed
*******************************************************************/
int PerformTest( const stTestParamaters Test, const int nIgnoreResult )
{
	int nResult = 0;

	Delay(BLINK_SLOW * 25);
	ClearSet_LED_Bank( (-1), 0x0000);
	ClearSet_LED_Bank( Test.m_nTest, Test.m_nTestState); // change the state of the led

	nResult = Test.m_pTestFunctions();
	if( 0 == nResult )
	{	// test failed
		if( 0 == nIgnoreResult )
		{
			Blink_LED( Test.m_nTest, BLINK_FAST );
		}
		else if( 1 == nIgnoreResult )
		{
		    int n;
		    for( n = 0; n < 30; n++ )
			{
		    	Delay(BLINK_SLOW);
				ClearSet_LED_Bank( (-1), 0x0000);
				Delay(BLINK_SLOW);
				ClearSet_LED_Bank( Test.m_nTest, Test.m_nTestState); // change the state of the led
			}

		}
	}


	return nResult;
}


/*******************************************************************
*   Function:    main
*   Description: This is the main entry point where everything starts
*******************************************************************/
void main(void)
{

	int bPassed;
	int nDelay;
 	volatile int bTrue = 1;
	u32 Result;							// result
	u32	ResponseCount;								// response count

	strcpy(g_szBuildDate, __DATE__);
	strcpy(g_szBuildTime, __TIME__);

	/* initialise System Services */
    adi_ssl_Init();
	
    /* Hook exception handler */
    adi_int_CECHook(3, ExceptionHandler, NULL, FALSE);

	// Set CCLK = 400 MHz, SCLK = 133 MHz
	adi_pwr_SetFreq(400000000,133333333, ADI_PWR_DF_NONE);
	
	// initialize the LEDs and push buttons
	Init_LEDs();
	Init_PushButtons();

	// initialize the timers mostly to be used so that we can
	// timeout in the case of a failure
	Init_Timers();
	Init_Timer_Interrupts();
	
	int temp = *pPORTB;
			
	asm("nop;");
	asm("nop;");
	asm("nop;");
	asm("nop;");
	
	////////////////////////////////////////////////////////////////////////////////
	// This section describes which PB to press to enter
	// certain tests

	// PB1 			- Normal tests
	// PB1 and PB2 	- Normal tests plus CAN and MXVR(For the BF549)
	// PB1 and PB3	- Set the RTC time
	// PB1 and PB4	- Verify the RTC time has updated

	// PB2			- USB as a device test
	// PB2 and PB3	- Program the Public Key
	// PB2 and PB4	- USB as a host test

	// PB3			- FPGA EZ-Extender tests
	// PB3 and PB4	- Writing or reading the MAC address

	// PB4			- AV EZ-Extender tests
	////////////////////////////////////////////////////////////////////////////////

	// If PB1 was pressed we will test the BF548 EZ-KIT Lite
	if( 0x100 == (temp & 0x100) )
	{	
		// if PB1 and PB3 set the rtc time
		if( 0x400 == (temp & 0x400) )
		{
			PerformTest( g_Tests[SET_RTC], 0 );

			// this is just normal blink mode.
			ClearSet_LED_Bank( (-1), 0x0000);
			while(1)
			{
				LED_Bar(BLINK_SLOW);
			}
		}
		// if PB1 and PB4 verify that the rtc time
		// has updated
		else if( 0x800 == (temp & 0x800) )
		{
			while( 1 )
			{
				PerformTest( g_Tests[VERIFY_RTC], 0 );

				Delay(BLINK_FAST * 20);
				
				// this is just normal blink mode.
				ClearSet_LED_Bank( (-1), 0x0000);
			
				LED_Bar(BLINK_SLOW);
			}
		}
		else
		{
			// SILICON VERSION TEST
			PerformTest( g_Tests[VERSION], 1 );

	   	   	// PUSH BUTTON & LED TEST
			PerformTest( g_Tests[PBLED], 0 );

			// see if the KEYPAD switch works
			PerformTest( g_Tests[KEYPAD], 0 );

			// see if the ROTARY works
			PerformTest( g_Tests[ROTARY], 0 );

			// test the touchscreen and LCD
			PerformTest( g_Tests[TOUCHLCD], 0);

			// test the SD memory
			PerformTest( g_Tests[SD], 0 );

			// stay in the loop and test everything else
			while(1)
			{
				int nTestIndex;

				PerformTest( g_Tests[VERSION], 1 );

				for( nTestIndex = NAND; nTestIndex < NUM_TESTS_IMPLEMENTED; nTestIndex++ )
				{
					PerformTest( g_Tests[nTestIndex], 0 );
					Delay(BLINK_FAST * 20);
				}

				// if PB1 and PB2 are pushed
				// do the additional BF549 tests
				if( 0x200 == (temp & 0x200) )
				{
					for( nTestIndex = CAN; nTestIndex < LAST_BF549_TEST; nTestIndex++ )
					{
						PerformTest( g_Tests[nTestIndex], 0 );
						Delay(BLINK_FAST * 20);
					}

				}

				g_loops++;

				// indicate everything passed
				ClearSet_LED_Bank( (-1), 0x0000);
				LED_Bar( BLINK_SLOW);
			}
		}
	}
	else if( 0x200 == (temp & 0x200) )
	{
		// if PB2 and PB3 are pushed
		if( 0x400 == (temp & 0x400) )
		{
			PerformTest( g_Tests[PROG_PUB_KEY], 0 );

			// WAIT A BIT
			Delay(BLINK_SLOW * 25);
			
			// this is just normal blink mode.
			ClearSet_LED_Bank( (-1), 0x0000);
			
			while(1)
			{
				LED_Bar(BLINK_SLOW);
			}
		}
		// if PB2 and PB4 are pushed
		else if( 0x800 == (temp & 0x800) )
		{

	   		// test the USB
			PerformTest( g_Tests[USB_HOST], 0 );

			// WAIT A BIT, THEN RESET THE LED's
			Delay(BLINK_SLOW * 25);

			// indicate everything passed
			ClearSet_LED_Bank( (-1), 0x0000);

			while(1)
			{
				// do a slower blink to indicate the transition
				// back to the first test
				LED_Bar( BLINK_SLOW );
			}

		}
		// just PB2 was pressed
		else
		{
			while( 1 )
			{
		   		// test the USB
				PerformTest( g_Tests[USB_DEV], 0 );

				// WAIT A BIT, THEN RESET THE LED's
				Delay(BLINK_SLOW * 25);

				// indicate everything passed
				ClearSet_LED_Bank( (-1), 0x0000);

				// do a slower blink to indicate the transition
				// back to the first test
				LED_Bar( BLINK_SLOW );
			}
		}

	}
	else if ( 0x400 == (temp & 0x400) )
	{
		// if PB3 and PB4 are pushed
		if( 0x800 == (temp & 0x800) )
		{
			while( 1 )
			{
				PerformTest( g_Tests[MACREADWRITE], 0 );
			}
		}
		else
		{
			// load the fpga image
			PROGRAM_FPGA();

			while(1) // do the POST tests forever
			{
				int nTestIndex;

				for( nTestIndex = FIRST_FPGA_TEST; nTestIndex < LAST_FPGA_TEST; nTestIndex++ )
				{
					PerformTest( g_Tests[nTestIndex], 0 );
				}

				// done with all tests
				g_loops++;

				// WAIT A BIT, THEN RESET THE LED's
				Delay(BLINK_SLOW * 25);

				// indicate everything passed
				ClearSet_LED_Bank( (-1), 0x0000);

				// do a slower blink to indicate the transition
				// back to the first test
				LED_Bar( BLINK_SLOW );

			}// end while(1)
		}

	}
	else if ( 0x800 == (temp & 0x800) )
	{
		while( 1 )
		{
			int nTestIndex;

	   		for( nTestIndex = FIRST_AV_TEST; nTestIndex < LAST_AV_TEST; nTestIndex++ )
			{
				PerformTest( g_Tests[nTestIndex], 0 );
				Delay(BLINK_FAST * 20);
			}

			// indicate everything passed
			ClearSet_LED_Bank( (-1), 0x0000);

			// do a slower blink to indicate the transition
			// back to the first test
			LED_Bar( BLINK_SLOW);
		}
	}
	else
	{
		SHOW_IMAGES();
	}

}


