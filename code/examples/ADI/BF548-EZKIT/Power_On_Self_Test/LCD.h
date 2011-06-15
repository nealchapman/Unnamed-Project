#ifndef __LCD_H__
#define __LCD_H__

/* Display # test patterns and disable LCD after that. Set it to zero for continous display */
#define		DISPLAY_TEST_CYCLE		0

/*************************
Processor specific Macros
*************************/

/* Processor DMA bus size = 4 bytes */
#define	    DMA_BUS_SIZE			4
/* PPI Device Number - BF548 has 3 EPPI ports */
#define     PPI_DEV_NUMBER         	0   /* use EPPI 0 */

/*******************
LCD specific Macros
*******************/

#define	    LCD_PIXELS_PER_LINE	    480		/* Pixels per line                          */
#define	    LCD_LINES_PER_FRAME	    272		/* Lines per frame                          */
#define	    LCD_DATA_PER_PIXEL     	3		/* 3bytes per pixel (MSB = B, G, LSB = R    */

/* Processor Timer ID used to generate LCD DISP signal  */
#define     LCD_DISP_TIMER_ID       ADI_TMR_GP_TIMER_3
/* Processor Flag ID connected to LCD DISP signal       */
#define     LCD_DISP_FLAG_ID        ADI_FLAG_PE3

/*****************
RGB Color defines
******************/
/* list of colors in RGB888 format */
#define	    RGB888_BLACK  		        0x000000
#define	    RGB888_RED 	    	        0xFF0000
#define     RGB888_GREEN 		        0x00FF00
#define     RGB888_BLUE     	        0x0000FF
#define     RGB888_YELLOW 		        0xFFFF00
#define     RGB888_CYAN 		        0x00FFFF
#define     RGB888_MAGENTA  	        0xFF00FF
#define     RGB888_WHITE 		        0xFFFFFF

/* Define LCD Boundary color */
#define     LCD_BOUNDARY_COLOR			RGB888_BLACK

#define 	FIRST_LINE_LOC			1
#define 	FIRST_PIXEL_LOC			1
#define		HORIZ_INNER_BORDER		32
#define 	VERT_INNER_BORDER		80
#define		VERT_PERIM_BORDER		42
#define 	HORIZ_PERIM_BORDER		18
#define		NUM_BLOCKS_PER_ROW		3

extern u8 Frame0[];
extern u8 Frame1[];
extern u8 Frame2[];
extern u8 PingFrame[];
extern u8 PongFrame[];

extern ADI_DEV_CMD_VALUE_PAIR *pEppiLcdConfig;

/* Create two buffer chains */
extern ADI_DEV_2D_BUFFER PingBuffer[];
extern ADI_DEV_2D_BUFFER PongBuffer[];


/* Globals */
extern volatile bool 	bKeepRunning;
extern volatile u32	PingPongCount;	/* # ping pong frames sent to EPPI */

#endif //_LCD_H_
