/*******************************************************************
Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software
you agree to the terms of the associated Analog Devices License Agreement.  

Project Name:  	Power_On_Self_Test

Hardware:		ADSP-BF548 EZ-KIT Lite

Description:	This file tests the LCD and touchscreen on the EZ-KIT
*******************************************************************/

/*******************************************************************
*  include files
*******************************************************************/
#include <services/services.h>				    /* system service includes              */
#include <drivers/adi_dev.h>				    /* device manager includes              */
#include <drivers/touchscreen/adi_ad7877.h> 	/* AD7877 driver includes   */
#include <drivers/lcd/sharp/adi_lq043t1dg01.h>  /* SHARP LQ043T1DG01 LCD driver include */
#include <drivers/eppi/adi_eppi.h>			    /* EPPI driver include                  */

#include "adi_ssl_init.h"
#include "LCD.h"					

#define     LCD_NUM_FRAMES_TEST 		    10  	/* switch color every # frames              */

/* Create two VGA frame in DDR which will be sent out to the LCD */      
section("RGB_Frame3") u8 PingFrame[LCD_PIXELS_PER_LINE * LCD_DATA_PER_PIXEL * LCD_LINES_PER_FRAME];
section("RGB_Frame4") u8 PongFrame[LCD_PIXELS_PER_LINE * LCD_DATA_PER_PIXEL * LCD_LINES_PER_FRAME];

/* handle to the LCD driver */
static ADI_DEV_DEVICE_HANDLE LCDDriverHandle;

/* handle to the Touchscreen driver */
static ADI_DEV_DEVICE_HANDLE TouchDriverHandle;

ADI_DEV_CMD_VALUE_PAIR *pEppiLcdConfig;

/* Globals */
volatile bool 	bKeepRunning;
volatile float	Touch_X,Touch_Y;	/* To save Touchscreen X,Y co-ordinates */
volatile u32	PingPongCount = DISPLAY_TEST_CYCLE;	/* # ping pong frames sent to EPPI */

volatile int	HighlightedButtonNum = 1;
volatile int 	TouchCount = 0;
volatile u16	ButtonsPushed = 0;

volatile int Finished = 0;

/*********************************************************************

Function prototypes

*********************************************************************/
static ADI_INT_HANDLER(ExceptionHandler);	/* exception handler */
static ADI_INT_HANDLER(HWErrorHandler);		/* hardware error handler */
static void LQ_Callback(
    void 	*AppHandle, 
    u32  	Event,
    void 	*pArg
);

static void Touch_Callback(
    void 	*AppHandle, 
    u32  	Event,
    void 	*pArg
);

/* LCD Test pattern generator */
static void adi_lcd_TestPattern(
    u8  *pDataBuffer	/* Pointer to RGB888 frame */
);

bool IsBoundary( u32 PixelLoc, u32 LineLoc );
int GetButtonPressed(u32 xPos, u32 yPos);
bool HighlightButton( u32 ButtonNum, u8  *pDataBuffer );


/*******************************************************************
*   Function:    LQ_Callback
*   Description: Each type of callback event has it's own unique ID 
*                so we can use a single callback function for all 
*                callback events.  The switch statement tells us 
*                which event has occurred.
*******************************************************************/
/* place this part of code in L1 */
section ("L1_code")
static void LQ_Callback(
    void 	*AppHandle,
    u32  	Event,
    void 	*pArg
){
    
    ADI_DEV_2D_BUFFER *pBuffer;		/* pointer to the buffer that was processed */
    u8  Color;    
	u32 Result;
	
    /* CASEOF (event type) */
    switch (Event) 
    {    
        /* CASE (buffer processed) */
        case ADI_DEV_EVENT_BUFFER_PROCESSED:

            /* when the buffer chain was created, the CallbackParameter value for the buffer
               that was generating the callback was set to be the address of the first buffer
               in the chain.  So here in the callback that value is passed in as the pArg
               parameter. */
            pBuffer = (ADI_DEV_2D_BUFFER *)pArg;

			/* IF (# Ping/Pong frames displayed exceeds total display cycle */
			if (DISPLAY_TEST_CYCLE)
			{
			    PingPongCount--;
				if (!PingPongCount)
				{
			    	bKeepRunning = FALSE;	/* disable LCD dataflow */
				}
			}
			
            break;

        /* CASE (EPPI Status Error) */
        case ADI_EPPI_EVENT_LUMA_FIFO_ERROR:         
        case ADI_EPPI_EVENT_LINE_TRACK_OVERFLOW_ERROR:         
        case ADI_EPPI_EVENT_LINE_TRACK_UNDERFLOW_ERROR:         
        case ADI_EPPI_EVENT_FRAME_TRACK_OVERFLOW_ERROR:
        case ADI_EPPI_EVENT_FRAME_TRACK_UNDERFLOW_ERROR:
        case ADI_EPPI_EVENT_PREAMBLE_ERROR_NOT_CORRECTED:
        case ADI_EPPI_EVENT_PREAMBLE_ERROR:
					
			break;
			
        /* CASE (DMA Error) */
        case ADI_DEV_EVENT_DMA_ERROR_INTERRUPT:
            
            break;
            
		default:
			break;
    }
    
    /* return */
}

/*******************************************************************
*   Function:    Touch_Callback
*   Description: Each type of callback event has it's own unique ID 
*                so we can use a single callback function for all 
*                callback events.  The switch statement tells us 
*                which event has occurred.
*******************************************************************/
/* place this part of code in L1 */
section ("L1_code")
static void Touch_Callback(
    void 	*AppHandle,
    u32  	Event,
    void 	*pArg
){	
	unsigned int Result;
	int i = 0;
	int ButtonPressed = 0;
	int temp = 1;
		
    /* pointer to AD7877 results location */
    ADI_AD7877_RESULT_REGS	*Results;
        	    
    /* CASEOF (event type) */
    switch (Event) 
    {
        
        /* CASE (PENIRQ occurred) */
        case ADI_AD7877_EVENT_PENIRQ:          
            break;
            
        /* CASE (Single Channel DAV occurred) */
        case ADI_AD7877_EVENT_SINGLE_DAV:
            break;
        
        /* CASE (Sequencer DAV occurred) */
        case ADI_AD7877_EVENT_SEQUENCER_DAV:
    		/* AD7877 passes pointer to the results register structure as argument */
        	Results = (ADI_AD7877_RESULT_REGS *)pArg;
        				
        	/* save the new co-ordinates */
        	Touch_X	= (abs(480-(u32)((Results->X)/8.5)));
        	Touch_Y = (abs(272-(u32)((Results->Y)/15.0)));	
				
        	if ( Results->Z1 )
			{
				if( IsBoundary( (u32)Touch_X, (u32)Touch_Y ))
					break;

				ButtonPressed = GetButtonPressed( (u32)Touch_X, (u32)Touch_Y );

				if( ButtonPressed != 0 )
			{
				HighlightButton( ButtonPressed, PingFrame );
				HighlightButton( ButtonPressed, PongFrame );
				
				temp <<= (ButtonPressed-1);
				ButtonsPushed |= temp;
			}
			else
			{
				// we should not get here
			}
		}	
            break;
            
        /* CASE (ALERT occurred) */
        case ADI_AD7877_EVENT_ALERT:
            break;
            
		default:
			break;
                
    /* ENDCASE */
    }
      
    /* return */
}

/*******************************************************************
*   Function:    adi_lcd_TestPattern
*   Description: Fills RGB888 frame with the LCD Test pattern
*******************************************************************/
void adi_lcd_TestPattern(
    u8  *pDataBuffer        /* Pointer to RGB888 frame      */
){
    u32     Line,Pixel;
    u8      r,g,b;
    u8		*pTemp;
    
    pTemp = pDataBuffer;
    
    /* Fill buffer to black when finished */
	if (Finished)
	{
		/* FOR (each line) */ 
		for(Line = 1; Line <= LCD_LINES_PER_FRAME; Line++)
		{
			/* FOR (each pixel) */ 
			for (Pixel = 1; Pixel <= LCD_PIXELS_PER_LINE; Pixel++ )
			{
		   		*pTemp++ = (u8) (RGB888_BLACK >> 16);
		    	*pTemp++ = (u8) (RGB888_BLACK >> 8);
		    	*pTemp++ = (u8) (RGB888_BLACK);
			}	
		}
	}

	else
	{
		/* Test pattern - Button fill */
	for( Line = 1; Line <= LCD_LINES_PER_FRAME/NUM_BLOCKS_PER_ROW; Line++ )
	{
	    for( Pixel = 1; Pixel <= LCD_PIXELS_PER_LINE; Pixel++ )
	    {
	    	*pDataBuffer++ = (u8) (RGB888_RED >> 16);
        	*pDataBuffer++ = (u8) (RGB888_RED >> 8);
        	*pDataBuffer++ = (u8) (RGB888_RED);
	    }
	}
	
	for( Line = 1; Line <= LCD_LINES_PER_FRAME/NUM_BLOCKS_PER_ROW; Line++ )
	{
	    for( Pixel = 1; Pixel <= LCD_PIXELS_PER_LINE; Pixel++ )
	    {
	    	*pDataBuffer++ = (u8) (RGB888_GREEN >> 16);
        	*pDataBuffer++ = (u8) (RGB888_GREEN >> 8);
        	*pDataBuffer++ = (u8) (RGB888_GREEN);
	    }
	}
	
	for( Line = 1; Line <= LCD_LINES_PER_FRAME/NUM_BLOCKS_PER_ROW; Line++ )
	{
	    for( Pixel = 1; Pixel <= LCD_PIXELS_PER_LINE; Pixel++ )
	    {
		    	*pDataBuffer++ = (u8) (RGB888_BLUE >> 16);
	        	*pDataBuffer++ = (u8) (RGB888_BLUE >> 8);
	        	*pDataBuffer++ = (u8) (RGB888_BLUE);
	    }
	}
	
	/* FOR (each line) */ 
	for(Line = 1; Line <= LCD_LINES_PER_FRAME; Line++)
	{
		/* FOR (each pixel) */ 
		for (Pixel = 1; Pixel <= LCD_PIXELS_PER_LINE; Pixel++ )
		{
		    /* IF (this pixel falls under LCD boundary (B)) */
		    if ( IsBoundary( Pixel, Line ) )
		    {
	            *pTemp++ = (u8)(LCD_BOUNDARY_COLOR >> 16);
	            *pTemp++ = (u8)(LCD_BOUNDARY_COLOR >> 8);
	            *pTemp++ = (u8)(LCD_BOUNDARY_COLOR);
            }
            else
            {
                *pTemp++;
                *pTemp++;
                *pTemp++;
            }
        }
    }
}
}

/*******************************************************************
*   Function:    IsBoundary
*   Description: Figures out if we are within a button or in the
*				 outside boundary area
*******************************************************************/
bool IsBoundary( u32 PixelLoc, u32 LineLoc )
{
	bool bPixelMatch = false;
	bool bLineMatch = false;	
	
	if( (PixelLoc >= FIRST_PIXEL_LOC) && 
		(PixelLoc < (FIRST_PIXEL_LOC + VERT_PERIM_BORDER)) )
	{
		bPixelMatch = true;
	}
	else if((PixelLoc >= (LCD_PIXELS_PER_LINE/NUM_BLOCKS_PER_ROW) - VERT_INNER_BORDER/2) && 
			(PixelLoc <= (LCD_PIXELS_PER_LINE/NUM_BLOCKS_PER_ROW) + VERT_INNER_BORDER/2) )
	{
		bPixelMatch = true;
	}
	else if((PixelLoc >= ((LCD_PIXELS_PER_LINE/NUM_BLOCKS_PER_ROW)*2) - VERT_INNER_BORDER/2) && 
			(PixelLoc <= ((LCD_PIXELS_PER_LINE/NUM_BLOCKS_PER_ROW)*2) + VERT_INNER_BORDER/2) )
	{
		bPixelMatch = true;
	}
	else if((PixelLoc >= LCD_PIXELS_PER_LINE-VERT_PERIM_BORDER) && 
			(PixelLoc <= LCD_PIXELS_PER_LINE) )
	{
		bPixelMatch = true;
	}
	
	
	else if((LineLoc >= FIRST_LINE_LOC) && 
			(LineLoc < (FIRST_LINE_LOC + HORIZ_PERIM_BORDER)) )
	{
		bLineMatch = true;
	}
	else if((LineLoc >= (LCD_LINES_PER_FRAME/NUM_BLOCKS_PER_ROW) - HORIZ_INNER_BORDER/2) && 
			(LineLoc <= (LCD_LINES_PER_FRAME/NUM_BLOCKS_PER_ROW) + HORIZ_INNER_BORDER/2) )
	{
		bLineMatch = true;
	}
	else if((LineLoc >= ((LCD_LINES_PER_FRAME/NUM_BLOCKS_PER_ROW)*2) - HORIZ_INNER_BORDER/2) && 
			(LineLoc <= ((LCD_LINES_PER_FRAME/NUM_BLOCKS_PER_ROW)*2) + HORIZ_INNER_BORDER/2) )
	{
		bLineMatch = true;
	}
	else if((LineLoc >= LCD_LINES_PER_FRAME-HORIZ_PERIM_BORDER) && 
			(LineLoc <= LCD_LINES_PER_FRAME) )
	{
		bLineMatch = true;
	}	
	
	return( bPixelMatch | bLineMatch );		
}

/*******************************************************************
*   Function:    GetButtonPressed
*   Description: Get the button that was pressed by the user
*******************************************************************/
int GetButtonPressed(u32 xPos, u32 yPos)
{
	unsigned char ucButton = 1;
	u32 RowPos = 0;
	u32 ColumnPos = 0;
	bool bButtonFound = false;
	unsigned char ucCurrentButton = 1;
	
	for( RowPos = (LCD_LINES_PER_FRAME/NUM_BLOCKS_PER_ROW); (RowPos <= LCD_LINES_PER_FRAME) && !bButtonFound; RowPos+= (LCD_LINES_PER_FRAME/NUM_BLOCKS_PER_ROW) )
	{
		for( ColumnPos = (LCD_PIXELS_PER_LINE/NUM_BLOCKS_PER_ROW); ColumnPos <= LCD_PIXELS_PER_LINE; ColumnPos +=  (LCD_PIXELS_PER_LINE/NUM_BLOCKS_PER_ROW) )
		{
			if( (xPos < ColumnPos) && (yPos < RowPos) )
			{
				ucButton = ucCurrentButton;
				bButtonFound = true;
				break;
			}
			
			ucCurrentButton++;
		}
	}
	
	if( bButtonFound )
		return (int)ucButton;
	else
		return 0;
}



/*******************************************************************
*   Function:    HighlightButton
*   Description: Change the color of the button
*******************************************************************/
bool HighlightButton( u32 ButtonNum, u8  *pDataBuffer )
{
	unsigned char ucButton = 1;
	u16 RowPos = 0;
	u16 ColumnPos = 0;
	bool bButtonFound = false;
	u32 CurrentButton = 1;
	u32 ButtonStartAddress = 0;
	u32 ButtonLineStartAddress = 0;
	u32     Line,Pixel;
	u8	*pTemp = pDataBuffer;
	u16 CurrentRowPos = 1;
	u16 CurrentColumnPos = 1;
	u32 x,y;
	u32 NumLinesInBlock = 0;
	u32 NumPixelsPerLineInBlock = 0;
	
	for( RowPos = (LCD_LINES_PER_FRAME/NUM_BLOCKS_PER_ROW); (RowPos <= LCD_LINES_PER_FRAME); RowPos+= (LCD_LINES_PER_FRAME/NUM_BLOCKS_PER_ROW) )
	{
		for( ColumnPos = (LCD_PIXELS_PER_LINE/NUM_BLOCKS_PER_ROW); ColumnPos <= LCD_PIXELS_PER_LINE; ColumnPos +=  (LCD_PIXELS_PER_LINE/NUM_BLOCKS_PER_ROW) )
		{
			if( ButtonNum == CurrentButton )
			{
				bButtonFound = true;
				break;	
			}	
			
			CurrentButton++;
			CurrentColumnPos += (LCD_PIXELS_PER_LINE/NUM_BLOCKS_PER_ROW);
		}
		
		if( bButtonFound )
			break;
			
		CurrentRowPos += (LCD_LINES_PER_FRAME/NUM_BLOCKS_PER_ROW);
		CurrentColumnPos = 1;
	}
	
	if( CurrentColumnPos <= (LCD_PIXELS_PER_LINE/NUM_BLOCKS_PER_ROW) )
	{
		CurrentColumnPos += (VERT_PERIM_BORDER);
	}
	else
	{
		CurrentColumnPos += (VERT_INNER_BORDER/2);
	}
	
	if( CurrentRowPos <= (LCD_LINES_PER_FRAME/NUM_BLOCKS_PER_ROW) )
	{
		CurrentRowPos += (HORIZ_PERIM_BORDER);
	}
	else
	{
		CurrentRowPos += (HORIZ_INNER_BORDER/2);
	}
	
	if( CurrentColumnPos <= (LCD_PIXELS_PER_LINE/NUM_BLOCKS_PER_ROW) )
	{
		NumPixelsPerLineInBlock = ((LCD_PIXELS_PER_LINE/NUM_BLOCKS_PER_ROW)-(VERT_INNER_BORDER/2)-VERT_PERIM_BORDER);	
	}
	else if( CurrentColumnPos >= ((LCD_PIXELS_PER_LINE/NUM_BLOCKS_PER_ROW)*2) && CurrentColumnPos <= LCD_PIXELS_PER_LINE)
	{
		NumPixelsPerLineInBlock = ((LCD_PIXELS_PER_LINE/NUM_BLOCKS_PER_ROW)-(VERT_INNER_BORDER/2)-VERT_PERIM_BORDER);
	}
	else
	{
		NumPixelsPerLineInBlock = ((LCD_PIXELS_PER_LINE/NUM_BLOCKS_PER_ROW)-VERT_INNER_BORDER);
	}
	
	if( CurrentRowPos <= (LCD_LINES_PER_FRAME/NUM_BLOCKS_PER_ROW) )
	{
		NumLinesInBlock = ((LCD_LINES_PER_FRAME/NUM_BLOCKS_PER_ROW)-(HORIZ_INNER_BORDER/2)-HORIZ_PERIM_BORDER);
	}
	else if( CurrentRowPos >= ((LCD_LINES_PER_FRAME/NUM_BLOCKS_PER_ROW)*2) && CurrentRowPos <= LCD_PIXELS_PER_LINE)
	{
		NumLinesInBlock = ((LCD_LINES_PER_FRAME/NUM_BLOCKS_PER_ROW)-(HORIZ_INNER_BORDER/2)-HORIZ_PERIM_BORDER);
	}
	else
	{
		NumLinesInBlock = ((LCD_LINES_PER_FRAME/NUM_BLOCKS_PER_ROW)-HORIZ_INNER_BORDER);
	}
	
	// figure out the start address for our Button
	ButtonStartAddress += ( (CurrentRowPos-1) * LCD_PIXELS_PER_LINE );
	ButtonStartAddress += (CurrentColumnPos-1);
	ButtonStartAddress *= 3;
	
	ButtonLineStartAddress = ButtonStartAddress;
	
	pDataBuffer += ButtonStartAddress;
	pTemp = pDataBuffer;
	
	for( x = 1, Line = CurrentRowPos; x <= NumLinesInBlock; Line++, x++ )
	{
	    for( y = 1, Pixel = CurrentColumnPos; y <= NumPixelsPerLineInBlock; Pixel++, y++ )
	    {
	    	if( !(IsBoundary( Pixel, Line )) )
	    	{
	    		*pTemp++ = (u8) (RGB888_WHITE >> 16);
        		*pTemp++ = (u8) (RGB888_WHITE >> 8);
        		*pTemp++ = (u8) (RGB888_WHITE);
	    	}
	    	else
	    	{
	    		pDataBuffer += (LCD_PIXELS_PER_LINE*3);
	    		pTemp = pDataBuffer;
	    	}
	    		
	    }
	}

	return true;	
}



/*******************************************************************
*   Function:    TEST_TOUCHLCD
*   Description: Main test routine will get launched from the POST 
*				 framework.
*******************************************************************/
int TEST_TOUCHLCD (void) 
{    
    u32     ResponseCount;
    u32     i;		
    u32     Result;

    ADI_LQ043T1DG01_TIMER_FLAG  Disp;


	/************** Open AD7877 driver ***************/
    
    /* open the AD7877 driver */
    if((Result = adi_dev_Open(  adi_dev_ManagerHandle,     		/* Dev manager Handle */
                 	            &ADIAD7877EntryPoint,           /* Device Entry point */
                                0,                              /* Device number*/
	                            NULL,                           /* No client handle */
    	                        &TouchDriverHandle,             /* Device manager handle address */
        	                    ADI_DEV_DIRECTION_OUTBOUND,     /* Data Direction */
            	                adi_dma_ManagerHandle,         	/* Handle to DMA Manager */
                	            NULL,               			/* Handle to callback manager */
                    	        Touch_Callback))				/* Callback Function */
		!= ADI_DEV_RESULT_SUCCESS) 
    {
     	return 0;
    }
		
    /**************** AD7877 Driver Configuration Table ****************/

	/* For Moab E-Kit */
    /* Port Info for PENIRQ */
    ADI_AD7877_INTERRUPT_PORT   PenIrqPort;    
    PenIrqPort.FlagId       = ADI_FLAG_PJ12;
    PenIrqPort.FlagIntId    = ADI_INT_PINT2;
    
    /* Port Info for DAV */
    ADI_AD7877_INTERRUPT_PORT   DavIrqPort;
    DavIrqPort.FlagId       = ADI_FLAG_PJ11;
    DavIrqPort.FlagIntId    = ADI_INT_PINT2;
    
    /* Configuration Table to use AD7877 device on BF548 Ez-Kit Lite */
	ADI_DEV_CMD_VALUE_PAIR  AD7877_BF548EzKit[]={		   
        { ADI_AD7877_CMD_SET_SPI_DEVICE_NUMBER, (void *)0           },  /* use SPI device 0         */
        { ADI_AD7877_CMD_SET_SPI_CS,            (void *)2           },  /* SPI CS for AD7877        */
        { ADI_AD877_ENABLE_INTERRUPT_PENIRQ,    (void *)&PenIrqPort },  /* Enable PENIRQ monitoring */
        { ADI_AD877_ENABLE_INTERRUPT_DAV,       (void *)&DavIrqPort },  /* Enable DAV Monitoring    */
		{ ADI_DEV_CMD_END,					    NULL                }
	}; 
	
	/**************** Configure AD7877 Driver ****************/
    if((Result = adi_dev_Control(TouchDriverHandle, ADI_DEV_CMD_TABLE, (void *)AD7877_BF548EzKit))!= ADI_DEV_RESULT_SUCCESS)
    {
		return 0;
    }
    
	/**************** AD7877 device register configuration table for Master Sequencer mode ****************/

    ADI_DEV_ACCESS_REGISTER_FIELD     MasterSequencer[] =
        {   
            { AD7877_CONTROL_REG1,      AD7877_MODE,        3   },  /* ADC in Master sequencer mode             */
            { AD7877_CONTROL_REG1,      AD7877_SER_DFR,     0   },  /* Differential conversion                  */
			{ AD7877_CONTROL_REG2,      AD7877_TMR,         3   },  /* Convert every 8.19ms						*/
		    { AD7877_CONTROL_REG2,      AD7877_REF,         0   },  /* Internal reference                       */
		    { AD7877_CONTROL_REG2,      AD7877_POL,         0   },  /* STOPACQ - Active Low                    */
            { AD7877_CONTROL_REG2,      AD7877_FCD,         2   },  /* First Conversion Delay = 1.024ms         */
            { AD7877_CONTROL_REG2,      AD7877_PM,          2   },  /* ADC & reference powered up continuously  */
		    { AD7877_CONTROL_REG2,      AD7877_ACQ,         2   },  /* ADC acquisition time = 8us               */
		    { AD7877_CONTROL_REG2,      AD7877_AVG,         1   },  /* 4 measurements per channel averaged      */
            { AD7877_SEQUENCER_REG1,    AD7877_YPOS_S,      1   },  /* Enable Y position measurement            */
            { AD7877_SEQUENCER_REG1,    AD7877_XPOS_S,      1   },  /* Enable X position measurement            */
            { AD7877_SEQUENCER_REG1,    AD7877_Z2_S,        0   },  /* Enable Z2 touch pressure measurement     */
		    { AD7877_SEQUENCER_REG1,    AD7877_Z1_S,        1   },  /* Enable Z1 touch pressure measurement     */
            { ADI_DEV_REGEND,           0,                  0   }   /* Terminate this configuration table       */            
        };            
    	
	/**************** Configure AD7877 Driver ****************/
    if((Result = adi_dev_Control(TouchDriverHandle, ADI_DEV_CMD_REGISTER_FIELD_TABLE_WRITE, (void *)MasterSequencer))!= ADI_DEV_RESULT_SUCCESS)
    {
		return 0;
    }
	
    /************** Open LQ043T1DG01 driver (Device 0) ***************/
    
    /* open the EPPI driver for video out */
    if((Result = adi_dev_Open(  adi_dev_ManagerHandle,     		/* Dev manager Handle */
                 	            &ADILQ043T1DG01EntryPoint,      /* Device Entry point */
                                0,                              /* Device number*/
	                            NULL,                           /* No client handle */
    	                        &LCDDriverHandle,               /* Device manager handle address */
        	                    ADI_DEV_DIRECTION_OUTBOUND,     /* Data Direction */
            	                adi_dma_ManagerHandle,         	/* Handle to DMA Manager */
                	            NULL,               			/* Handle to callback manager */
                    	        LQ_Callback))				    /* Callback Function */
		!= ADI_DEV_RESULT_SUCCESS) 
    {
     	return 0;
    }

    /**************** LQ043T1DG01 driver Configuration ****************/
    
    Disp.DispTimerId = LCD_DISP_TIMER_ID;
    Disp.DispFlagId  = LCD_DISP_FLAG_ID;
    
	/* Set DISP Timer & Flag ID */
	if((Result = adi_dev_Control( LCDDriverHandle, ADI_LQ043T1DG01_CMD_SET_DISP_TIMER_FLAG, (void*)&Disp ))!= ADI_DEV_RESULT_SUCCESS)
	{
		return 0;
    }
    
	/* Set EPPI Device number */
	if((Result = adi_dev_Control( LCDDriverHandle, ADI_LQ043T1DG01_CMD_SET_EPPI_DEV_NUMBER, (void*)PPI_DEV_NUMBER ))!= ADI_DEV_RESULT_SUCCESS)
	{
		return 0;
    }
    
    /* Open EPPI Device for LCD out */
	if((Result = adi_dev_Control( LCDDriverHandle, ADI_LQ043T1DG01_CMD_SET_OPEN_EPPI_DEVICE, (void*)TRUE ))!= ADI_DEV_RESULT_SUCCESS)
	{
		return 0;
    }
	
	/**************** EPPI Configuration ****************/

	/* EPPI Control register configuration value for RGB out
    	- EPPI as Output
    	  GP 2 frame sync mode, 
    	  Internal Clock generation disabled, Internal FS generation enabled,
      	  Receives samples on EPPI_CLK raising edge, Transmits samples on EPPI_CLK falling edge,
      	  FS1 & FS2 are active high, 
      	  DLEN = 6 (24 bits for RGB888 out) or 5 (18 bits for RGB666 out)
      	  DMA Unpacking disabled when RGB Formating is enabled, otherwise DMA unpacking enabled
      	  Swapping Disabled, 
      	  One (DMA) Channel Mode,
      	  RGB Formatting Enabled for RGB666 output, disabled for RGB888 output
      	  Regular watermark - when FIFO is 75% full, 
      	  Urgent watermark - when FIFO is 25% full 
	*/

	/* EPPI Configuration table for SHARP LQ10D368 TFT Color LCD connected to Blackfin Av Extender */
	ADI_DEV_CMD_VALUE_PAIR EppiLcdConfig[]={
/* IF (RGB888 output selected) */
		{ ADI_EPPI_CMD_SET_DATA_LENGTH,             (void *)6       },  /* 24 bit out   */
		{ ADI_EPPI_CMD_ENABLE_RGB_FORMATTING,       (void *)FALSE   },  /* Disable RGB formatting */	    
    	{ ADI_EPPI_CMD_SET_PORT_DIRECTION,          (void *)1       },  /* Output mode  */
	    { ADI_EPPI_CMD_SET_TRANSFER_TYPE,           (void *)3       },  /* GP mode      */
    	{ ADI_EPPI_CMD_SET_FRAME_SYNC_CONFIG,       (void *)2       },  /* GP2 mode     */
	    { ADI_EPPI_CMD_SET_ITU_TYPE,                (void *)0       },  /* Interlaced   */
    	{ ADI_EPPI_CMD_ENABLE_INTERNAL_CLOCK_GEN,   (void *)TRUE    },  /* Internally generated Clock  */
	    { ADI_EPPI_CMD_ENABLE_INTERNAL_FS_GEN,      (void *)TRUE    },  /* Internally generated Frame sync  */
    	{ ADI_EPPI_CMD_SET_CLOCK_POLARITY,          (void *)1       },  /* Tx raising edge, Rx falling edge */
    	{ ADI_EPPI_CMD_SET_FRAME_SYNC_POLARITY,     (void *)3       },  /* FS1 & FS2 active low	*/
    	{ ADI_EPPI_CMD_SET_SKIP_ENABLE,             (void *)FALSE   },  /* Disable skipping 	*/
	    { ADI_EPPI_CMD_SET_PACK_UNPACK_ENABLE,      (void *)TRUE    },  /* DMA unpacking enabled*/
    	{ ADI_EPPI_CMD_SET_SWAP_ENABLE,             (void *)FALSE   },  /* Swapping disabled 	*/
    	{ ADI_EPPI_CMD_SET_SPLIT_EVEN_ODD,          (void *)FALSE   },  /* Splitting diabled 	*/
    	{ ADI_EPPI_CMD_SET_FIFO_REGULAR_WATERMARK,  (void *)0      	},  /* Regular watermark  	*/
	    { ADI_EPPI_CMD_SET_FIFO_URGENT_WATERMARK,   (void *)0       },  /* Urgent watermark 	*/
	    { ADI_DEV_CMD_END,                          NULL            }
	}; 
    
	pEppiLcdConfig = EppiLcdConfig;
	
    /* Configure EPPI Control register */
	if((Result = adi_dev_Control( LCDDriverHandle, ADI_DEV_CMD_TABLE, (void*)pEppiLcdConfig ))!= ADI_DEV_RESULT_SUCCESS)
	{
		return 0;
    }

	/****************** Prepare Video Frames *******************/
		
	/* Fill the RGB888 frames with specified pattern */
	adi_lcd_TestPattern(PingFrame);
	adi_lcd_TestPattern(PongFrame);
	
	/******************Buffer preparation **********************/

	/*
	   create a buffer chain that points to the PingFrame.  Each buffer points to the same PingFrame
	   so the PingFrame will be displayed NUM_FRAMES times.  NUM_FRAMES is sized to 
	   keep the display busy for 1 second.  Place a callback on only the last buffer
	   in the chain.  Make the CallbackParameter (the value that gets passed to the callback
	   function as the pArg parameter) point to the first buffer in the chain.  This way, when
	   the callback goes off, the callback function can requeue the whole chain if the loopback
	   mode is off.
	*/

    /* FOR (Number of output 2D buffers) */
    for (i = 0; i < LCD_NUM_FRAMES_TEST; i++) 
    {
        PingBuffer[i].Data              = PingFrame;
        PingBuffer[i].ElementWidth      = DMA_BUS_SIZE;
        PingBuffer[i].XCount            = ((LCD_PIXELS_PER_LINE * LCD_DATA_PER_PIXEL)/DMA_BUS_SIZE);
        PingBuffer[i].XModify           = DMA_BUS_SIZE;
        PingBuffer[i].YCount            = LCD_LINES_PER_FRAME;
        PingBuffer[i].YModify           = DMA_BUS_SIZE;
        PingBuffer[i].CallbackParameter = NULL;
        PingBuffer[i].pNext             = &PingBuffer[i + 1];   /* chain to next buffer in the list */
    }

    /* DMA generates Callback when last buffer in the chain is processed
       address of first buffer in the chain is passed as callback parameter */
    PingBuffer[LCD_NUM_FRAMES_TEST - 1].CallbackParameter = &PingBuffer[0];
    /* Terminate this buffer chain */
    PingBuffer[LCD_NUM_FRAMES_TEST - 1].pNext = NULL;
  
    /* now do the same for the Pong buffers */
    /* FOR (Number of output 2D buffers) */
    for (i = 0; i < LCD_NUM_FRAMES_TEST; i++) 
    {
        PongBuffer[i].Data              = PongFrame;
        PongBuffer[i].ElementWidth      = DMA_BUS_SIZE;
        PongBuffer[i].XCount            = ((LCD_PIXELS_PER_LINE*LCD_DATA_PER_PIXEL)/DMA_BUS_SIZE);
        PongBuffer[i].XModify           = DMA_BUS_SIZE;
        PongBuffer[i].YCount            = LCD_LINES_PER_FRAME;
        PongBuffer[i].YModify           = DMA_BUS_SIZE;
        PongBuffer[i].CallbackParameter = NULL;
        PongBuffer[i].pNext             = &PongBuffer[i + 1];   /* chain to next buffer in the list */
    }
    
    /* DMA generates Callback when last buffer in the chain is processed
       address of first buffer in the chain is passed as callback parameter */
    PongBuffer[LCD_NUM_FRAMES_TEST - 1].CallbackParameter = &PongBuffer[0];  
    /* Terminate this buffer chain */
    PongBuffer[LCD_NUM_FRAMES_TEST - 1].pNext = NULL;

	/* Set Dataflow method */
    if((Result = adi_dev_Control(LCDDriverHandle, ADI_DEV_CMD_SET_DATAFLOW_METHOD, (void *)ADI_DEV_MODE_CHAINED_LOOPBACK ))!= ADI_DEV_RESULT_SUCCESS)    
    {
        return 0;
    }

    /* give the ping-pong buffers to the device */
    if((Result = adi_dev_Write(LCDDriverHandle, ADI_DEV_2D, (ADI_DEV_BUFFER *)PingBuffer))!= ADI_DEV_RESULT_SUCCESS)
    {
    	return 0;
    }
   
    if((Result = adi_dev_Write(LCDDriverHandle, ADI_DEV_2D, (ADI_DEV_BUFFER *)PongBuffer))!= ADI_DEV_RESULT_SUCCESS)
    {	
    	return 0;
    }

    /* Enable LCD video data flow */
    if((Result = adi_dev_Control(LCDDriverHandle, ADI_DEV_CMD_SET_DATAFLOW, (void *)TRUE))!= ADI_DEV_RESULT_SUCCESS)
    {
		return 0;
    }
    
    
    /* continue displaying frames */
   	while(ButtonsPushed != 0x1FF)
   	{

   	}
   	
   	// give a little time to hightlight the last box
   	for(i = 0; i < 0xFFFFF; i++)
   		asm("nop;");
   		
   	Finished = 1;
   		
	adi_lcd_TestPattern(PingFrame);
	adi_lcd_TestPattern(PongFrame);
   		
   	// give a little time to clear the screen
   	for(i = 0; i < 0xFFFFF; i++)
   		asm("nop;");

   	// stop dataflow
   	if((Result = adi_dev_Control(LCDDriverHandle, ADI_DEV_CMD_SET_DATAFLOW, (void *)FALSE))!= ADI_DEV_RESULT_SUCCESS)
    {
		return 0;
    }
   	
    /* Close EPPI Device for LCD out */
	if((Result = adi_dev_Control( LCDDriverHandle, ADI_LQ043T1DG01_CMD_SET_OPEN_EPPI_DEVICE, (void*)FALSE ))!= ADI_DEV_RESULT_SUCCESS)
	{	
		return 0;
    }
 

	i = 0;
	do
	{
		MasterSequencer[i++].Data = 0;
	}while( MasterSequencer[i].Address != ADI_DEV_REGEND );
	 
    
	/**************** Configure AD7877 Driver to turn off controller ****************/
    if((Result = adi_dev_Control(TouchDriverHandle, ADI_DEV_CMD_REGISTER_FIELD_TABLE_WRITE, (void *)MasterSequencer))!= ADI_DEV_RESULT_SUCCESS)
    {
		return 0;
    }
    
    adi_dev_Close(LCDDriverHandle);
    adi_dev_Close(TouchDriverHandle);
    
   	return 1;
    
}


