/*******************************************************************
Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software
you agree to the terms of the associated Analog Devices License Agreement.  

Project Name:  	Power_On_Self_Test

Hardware:		ADSP-BF548 EZ-KIT Lite

Description:	This file tests the video portion of the AV EZ-Extender
				board.
*******************************************************************/

/*******************************************************************
*  include files
*******************************************************************/
#include <cdefBF548.h>
#include <ccblkfn.h>
#include <sys\exception.h>
#include <math.h>
#include <signal.h>
#include <stdlib.h>


/*******************************************************************
*  global variables and defines
*******************************************************************/

// the loopback jumpers are configured incorrectly to do component testing so
// we will still get a color bar but it will be off from what it
// should be.  When Rev 2.1 is available we should be able to remove
// this define
#define WRONG_JUMPER_CONFIG

#define PRESCALE_VALUE 10				//PRESCALE = 100/10 = 10.
#define RESET_TWI 0						//RESET_TWI value for controller
#define CLKDIV_HI 17					//30%DUTY CYCLE 
#define CLKDIV_LO 8
#define WDSIZE_16 0x0004
#define WDSIZE_32 0x0008
#define DLEN_8	0x0
#define PACK_EN 0x200000
#define PORT_EN 0x1

#define PIXEL_PER_LINE 		(720)
#define LINES_PER_FRAME		(480)
#define TOTAL_DDR_USED	(PIXEL_PER_LINE * LINES_PER_FRAME * 2)

// specific defines for this module. Not used anywhere else
#define AV_RESET_Delay			4200000 	// 7ms
#define AV_RESET_RECOVER_Delay	4200000 	// 7ms


#define COLORBAR_START		0x300000		// start address where colorbar is stored

int current_in_Frame = 0;
bool bDMA_Rcv_End = false;

unsigned short EncoderConfig[] = { 	0x0001, 0x0090,
						 			0x0004, 0x0003,
						 			0x0003, 0x0018};

unsigned short DecoderConfig[] = { 	0x0000, 0x0009  };

/*******************************************************************
*  function prototypes
*******************************************************************/
static void InitDMA(void);
static void InitEPPI0(void);
static void InitPorts(void);
static void DisableEPPI0_DMA(void);
static void EnableEPPI0(void);
static void Reset_ADV7179_ADV7183(void );
static void clear_DDR(void);
static void get_color_info( unsigned char *Frame, int x1, int y1, int x2, int y2, int *avg_y, int *avg_u, int *avg_v );
static int Verify_ColorBar(void);
EX_INTERRUPT_HANDLER(EPPI0_RxIsr);


/*******************************************************************
*   Function:    EPPI0_RxIsr
*   Description: ISR for data received from the PPI
*******************************************************************/
EX_INTERRUPT_HANDLER(EPPI0_RxIsr)
{
	int tempReg = 0;

	tempReg = *pDMA12_IRQ_STATUS;

	*pDMA12_IRQ_STATUS |= DMA_DONE;

	if( tempReg & 0x1)
	{
	    DisableEPPI0_DMA();

	    // set dma end flag
	    bDMA_Rcv_End = 1;
	}

}

/*******************************************************************
*   Function:    Init_Ports
*   Description: Configure Port registers needed to run this test
*******************************************************************/
void InitPorts(void)
{
	// set FERF registers
	*pPORTD_FER = 0;
	ssync();

	// video reset
	*pPORTD_DIR_SET = Px11;
	ssync();

	//activate PortE 14,15 for TWI SDA and SCL
	*pPORTE_FER = Px14 | Px15;
	ssync();

	// set FERF for PPI Data 0-7
	*pPORTF_FER = 0x00FF;
	ssync();

	// set FERF for PPI CLK
	*pPORTG_FER = 0x0001;
	ssync();
}

/*******************************************************************
*   Function:    InitInterrupts
*   Description: Initialize interrupts
*******************************************************************/
void InitInterrupts(void)
{
	register_handler(ik_ivg8, EPPI0_RxIsr);		// assign ISR to interrupt vector

	*pILAT |= EVT_IVG8;							// clear pending IVG8 interrupts
	ssync();
	*pSIC_IMASK0 |= 0x100;
	ssync();

}

/*******************************************************************
*   Function:    InitDDR
*   Description: Initialize DDR
*******************************************************************/
void InitDDR(void)
{
  	// release the DDR controller from reset as per spec
    *pEBIU_RSTCTL |= 0x0001;
    ssync();

    *pEBIU_DDRCTL0 = 0x218A8411L;
    *pEBIU_DDRCTL1 = 0x20022222L;
    *pEBIU_DDRCTL2 = 0x00000021L;

    ssync();

}

/*******************************************************************
*   Function:    InitDMA
*   Description: Initialize DMA
*******************************************************************/
void InitDMA(void)
{   
	// Target address of the DMA
	*pDMA12_START_ADDR = (void*)COLORBAR_START;

	// Line_Length 32bit transfers will be executed
	*pDMA12_X_COUNT = PIXEL_PER_LINE;

	// The modifier is set to 4 because of the 32bit transfers
	*pDMA12_X_MODIFY = 0x4;

	// Frame_Length 32bit transfers will be executed
	*pDMA12_Y_COUNT = LINES_PER_FRAME;

	// The modifier is set to 4 because of the 32bit transfers
	*pDMA12_Y_MODIFY = 0x4;

	// DMA Config: Enable DMA | Generate interrupt on completion | Memory write DMA |
	// 32-bit xfers | 2-D DMA | Discard DMA FIFO before start
	*pDMA12_CONFIG = DI_EN | WNR | WDSIZE_32 | DMA2D | SYNC;
	ssync();
	
}//end Init_DMA


/*******************************************************************
*   Function:    InitEPPI0
*   Description: Initialize EPPI 0
*******************************************************************/
void InitEPPI0()
{
    // - EPPI disabled
    // - EPPI in receive mode
    // - ITU-R 656, Active video only
    // - interlaced
    // - sample data on rising edge and sample/drive
    //   syncs on falling edge
    // - DLEN = 8 bits
    // - PACKEN enabled
	*pEPPI0_CONTROL = 0x00101000;
	ssync();
	
	// total lines is 858
    *pEPPI0_LINE = 858;
    ssync();
    
    
    //The PPI is set to receive 525 lines for each frame
	*pEPPI0_FRAME = 525; // this is active plus verticle blanking line total
	ssync();
}

/*******************************************************************
*   Function:    DisableEPPI0_DMA
*   Description: Disable DMA and PPI 0
*******************************************************************/
void DisableEPPI0_DMA(void)
{
	// disable transfers
	*pDMA12_CONFIG = 0;
	ssync();
	*pEPPI0_CONTROL = 0;
	ssync();
}

/*******************************************************************
*   Function:    EnableEPPI0
*   Description: Enable DMA and PPI 0
*******************************************************************/
void EnableEPPI0_DMA(void)
{
    // enable transfers
    *pDMA12_CONFIG |= 0x1;
    ssync();
    
    *pEPPI0_CONTROL |= PORT_EN;
    ssync();
}

/*******************************************************************
*   Function:    Reset_ADV7179_ADV7183
*   Description: Reset the encoder and decoder
*******************************************************************/
void Reset_ADV7179_ADV7183(void )
{
    int i,j;

	// assert AV_RESET
	*pPORTD_CLEAR 	= Px11; 		//0x0800;

	for(i=0; i< AV_RESET_Delay; i++)
		asm("nop;");

	// deassert AV_RESET
	*pPORTD_SET 	= Px11; 		//0x0800;

	for(i=0; i< AV_RESET_RECOVER_Delay; i++)
		asm("nop;");

}

/*******************************************************************
*   Function:    delay_loop
*   Description: Delay for a certain number of cycles
*******************************************************************/
void delay_loop(void)
{
    int i = 0;

    for( i = 0; i < 0xFFF; i++ )
    {
        asm("nop;");
    }
}

/*******************************************************************
*   Function:    Reset_TWI
*   Description: reset the TWI interface
*******************************************************************/
void Reset_TWI(void)
{
     //RESET_TWI CONTROLLER
	*pTWI0_CONTROL = RESET_TWI;
	ssync();

	//CLEAR ALL ERRONOUS CONDITIONS BEFORE ENABLING TWI
	*pTWI0_MASTER_STAT = BUFWRERR | BUFRDERR | LOSTARB | ANAK | DNAK;
	ssync();

	//CLEAR ALL INTERRUPTS BEFORE ENABLING TWI
	*pTWI0_INT_STAT = SINIT | SCOMP | SERR | SOVF | MCOMP | MERR | XMTSERV | RCVSERV;
	ssync();

	//FLUSH THE FIFOs - BOTH TX AND RX.
	*pTWI0_FIFO_CTL = XMTFLUSH | RCVFLUSH;
	ssync();
}

/*******************************************************************
*   Function:    TWI_MasterMode_Write
*   Description: do a master mode write
*******************************************************************/
void TWI_MasterMode_Write(unsigned short DeviceAddr, unsigned short *TWI_Data_Pointer, unsigned short TX_Count, unsigned short TWI_TX_Length)
{
	int i, j;

	//FLUSH THE FIFOs - BOTH TX AND RX.
	*pTWI0_FIFO_CTL = XMTFLUSH | RCVFLUSH;
	ssync();
	
	*pTWI0_MASTER_STAT = BUFWRERR | BUFRDERR | LOSTARB | ANAK | DNAK;
	ssync();
	
    *pTWI0_FIFO_CTL = 0;					// Clear the bit manually
	*pTWI0_CONTROL = TWI_ENA | PRESCALE_VALUE;	// PRESCALE = fsclk/10MHz
	*pTWI0_CLKDIV = ((CLKDIV_HI) << 8) | (CLKDIV_LO);						// For 100KHz SCL speed: CLKDIV = (1/100KHz)/(1/10MHz) = 100 -> SCL symetric: CLKHI = 50, CLKLOW = 50
	*pTWI0_MASTER_ADDR = DeviceAddr;			// Target address (7-bits plus the read/write bit the TWI controls

	for (i = 0; i < TX_Count; i++)
	{
	    // # of configurations to send to the sensor
		*pTWI0_XMT_DATA8 = *TWI_Data_Pointer++;		// Pointer to an array and load a value where a list of data is located

		*pTWI0_MASTER_CTL = (TWI_TX_Length<<6) | MEN | FAST;						// Start transmission

		for (j = 0; j < (TWI_TX_Length-1); j++)
		{
		    // # of transfers before stop condition
			while (*pTWI0_FIFO_STAT == XMTSTAT)					// wait to load the next sample into the TX FIFO
				ssync();

			*pTWI0_XMT_DATA8 = *TWI_Data_Pointer++;		// Load the next sample into the TX FIFO. Pointer to an array where a list of data is located
			ssync();
		}

		while ((*pTWI0_INT_STAT & MCOMP) == 0)				// Wait until transmission complete and MCOMP is set
			ssync();

		*pTWI0_INT_STAT = XMTSERV | MCOMP;					// service TWI for next transmission
	}

	asm("nop;");
	asm("nop;");
	asm("nop;");

}

/*******************************************************************
*   Function:    clear_DDR
*   Description: clear our video buffer
*******************************************************************/
void clear_DDR(void)
{
    int i=0;
    int *pDDR = (int *)COLORBAR_START;
    for(i = 0; i < (TOTAL_DDR_USED / sizeof(int)); i++, pDDR++)
    {
        *pDDR =0;
    }
}

/*******************************************************************
*   Function:    get_color_info
*
*   Parameters:	*Frame 	- frame start address
*				x1 		- starting x coordinate
*				y1		- starting y coordinate
*				x2 		- ending x coordinate
*				y2		- ending y coordinate
*				*avg_y 	- color bar y component
*				*avg_u	- color bar u component
*				*avg_v 	- color bar v component

*   Description: gets YUV components from a shade of the color bar
*******************************************************************/
void get_color_info( unsigned char *Frame, int x1, int y1, int x2, int y2, int *avg_y, int *avg_u, int *avg_v )
{
	// data is in UYVY
	int x, y, i;
	unsigned char *p;
	unsigned char *p_end;
	unsigned char yy1, yy2, u, v;
	long long sum_y = 0;
	long long sum_u = 0;
	long long sum_v = 0;
	int min_y = 255, min_u = 255, min_v = 255;
	int max_y =   0, max_u =   0, max_v =   0;


	long long num_pixels;

	if( ( x1 > x2 ) || ( y1 > y2 ) ) return;

	num_pixels = (y2 - y1) * (x2 - x1);

	for( y = y1; y < y2; y++ )
	{
		for( x = x1; x < x2; x++ )
		{
			if( !(x & 0x01) )
			{
				// U
				i = (unsigned char) Frame[ ( 2 * y * PIXEL_PER_LINE ) + x * 2 ];
				if( i > max_u ) max_u = i;
				if( i < min_u ) min_u = i;
				sum_u += i;


				// Y
				i = (unsigned char) Frame[ ( 2 * y * PIXEL_PER_LINE ) + x * 2 + 1 ];
				if( i > max_y ) max_y = i;
				if( i < min_y ) min_y = i;
				sum_y += i;
			}
			else
			{
				// V
				i = (unsigned char) Frame[ ( 2 * y * PIXEL_PER_LINE ) + x * 2 ];
				if( i > max_v ) max_v = i;
				if( i < min_v ) min_v = i;
				sum_v += i;


				// Y
				i = (unsigned char) Frame[ ( 2 * y * PIXEL_PER_LINE ) + x * 2 + 1 ];
				if( i > max_y ) max_y = i;
				if( i < min_y ) min_y = i;
				sum_y += i;
			}
		}
	}

	// get the avg y:u:v
	*avg_y = sum_y / num_pixels;
	*avg_u = sum_u / ( num_pixels >> 1 );
	*avg_v = sum_v / ( num_pixels >> 1 );
}

/*******************************************************************
*   Function:    Verify_ColorBar
*   Description: see if the color bar matches what we expect it to be
*******************************************************************/
int Verify_ColorBar()
{
    int test_pass = 0;
    int avg_y = 0, avg_u = 0, avg_v = 0;		// yuv components
    int x1 = 6, y1 = 22, x2 = 78, y2 = 240;		// starting coordinates
    int i = 0;									// loop counter
    int column_inc = 90;						// increment to the next column

#ifdef WRONG_JUMPER_CONFIG
   int color_bar[8][3] = { // y   // u  // v
        					{0xF6, 0x80, 0x7f},
    						{0xAA, 0x94, 0x04},
    						{0x89, 0x04, 0xaa},
    						{0x75, 0x19, 0x2d},
    						{0x57, 0xE9, 0xd6},
    						{0x44, 0xFC, 0x56},
    						{0x24, 0x6B, 0xfc},
    						{0x10, 0x80, 0x80}	};		// values that we expect
#else
   int color_bar[8][3] = { // y   // u  // v
        					{0xF6, 0x80, 0x80},
    						{0xAA, 0x03, 0x94},
    						{0x8B, 0xAB, 0x02},
    						{0x75, 0x2E, 0x19},	
    						{0x58, 0xD2, 0xE7},	
    						{0x44, 0x56, 0xFC},
    						{0x24, 0xFC, 0x6B},
    						{0x10, 0x80, 0x80}	};		// values that we expect
#endif


    for( i = 0; i < 8; i++ )
    {
        // get yuv components
    	get_color_info((unsigned char*)COLORBAR_START, x1, y1, x2, y2, &avg_y, &avg_u, &avg_v );

    	x1+=column_inc;
    	x2+=column_inc;

    	// check to see if the values match what we expect +- 9
	    if( ( abs( color_bar[i][0] - avg_y ) < 9 ) &&
	    	( abs( color_bar[i][1] - avg_u ) < 9 ) &&
	    	( abs( color_bar[i][2] - avg_v ) < 9 ) )
		{
			// the color matches
			test_pass++;
		}
		else
		{
			// test failed
		}
    }

    // if all 8 bars were correct then pass
	if( test_pass == 8 )
		return 1;
	else
		return 0;

}

/*******************************************************************
*   Function:    TEST_AV_VIDEO
*   Description: Main test routine will get launched from the POST 
*				 framework.
*******************************************************************/
int TEST_AV_VIDEO()
{
	// after running TEST_VIDEO(), you may view the color bar in VDSP by halting
    // the Blackfin, then open the Image Viewer which is located under the
    // "View->Debug Windows->Image Viewer..." menu item and set the configuration
    // settings as follows:
    //		Source location: DSP Memory
    //		Source format: Raw pixel data
    //		Memory: BLACKFIN Memory
    //		Start address: 0x300000(COLORBAR_START)
    //		Memory stride: 1
    //		Pixel format: UYVY (4:2:2)
    //		Width: 720
    //		Height: 480
	bool Set_PACK32 = true;
	bool Set_Entire_Field = true;
	bool color_bar_verified = false;
	bool bFinished = false;
	int i = 0;

	// initialize appropriate PORT pins
	InitPorts();

	// initialise Video Encoder ADV7179 and ADV7183
	// the reset goes to both
	Reset_ADV7179_ADV7183();
	
	// reset the TWI interface
	Reset_TWI();
	
	// setup encoder to output a color bar
	// setup the DACs for Y:Pb:Pr
	TWI_MasterMode_Write( 0x2A, EncoderConfig, 3, 2);
	
	// setup decoder for component input
	TWI_MasterMode_Write( 0x20, DecoderConfig, 1, 2);
	
	// allow time for frames to sync
	for( i = 0; i < 0xFFFFFFF; i++ )
	{
	    asm("nop;");
	    asm("nop;");
	}
	
	// init sdram
	InitDDR();

	// clear the previous buffer
	clear_DDR();

	 // initialise Interrupts
	InitInterrupts();

	// init DMA
	InitDMA();

	// init PPI
    InitEPPI0();

    // start transfers
	EnableEPPI0_DMA();

	// main loop, just wait for interrupts
	while(!bFinished)
	{
	    if(bDMA_Rcv_End==1)
	 	{
	 	    bDMA_Rcv_End = 0;
	 	    bFinished = true;
			color_bar_verified = Verify_ColorBar();
			break;
	 	}
	}

	return color_bar_verified;
}



/*******************************************************************
*   Function:    main
*   Description: used only if this test is needed as a standalone
*				 test separate from POST
*******************************************************************/
#ifdef _STANDALONE_ 
void main(void)
{
	int nResult;

   	nResult = TEST_AV_VIDEO();
}
#endif //#ifdef _STANDALONE_
