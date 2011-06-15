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
#include <drivers/lcd/sharp/adi_lq043t1dg01.h>  /* SHARP LQ043T1DG01 LCD driver include */
#include <drivers/eppi/adi_eppi.h>			    /* EPPI driver include                  */
#include <string.h>

#include "LCD.h"
#include "PBLED_test.h"
#include "adi_ssl_init.h"					

#define     LCD_NUM_FRAMES_DISPLAY 		    120  	/* switch pic every # frames              */

/* handle to the LCD driver */
static ADI_DEV_DEVICE_HANDLE LCDDriverHandle;

bool bUsingPingBuffer = true;

section("RGB_Frame0") u8 Frame0[LCD_PIXELS_PER_LINE * LCD_DATA_PER_PIXEL * LCD_LINES_PER_FRAME]
	= { 
       #include "image1.dat"
      };
section("RGB_Frame1") u8 Frame1[LCD_PIXELS_PER_LINE * LCD_DATA_PER_PIXEL * LCD_LINES_PER_FRAME]
	= { 
       #include "image2.dat"
      };

section("RGB_Frame2") u8 Frame2[LCD_PIXELS_PER_LINE * LCD_DATA_PER_PIXEL * LCD_LINES_PER_FRAME]
	= { 
       #include "image3.dat"
      };

/* Create two buffer chains */
ADI_DEV_2D_BUFFER PingBuffer[LCD_NUM_FRAMES_DISPLAY];
ADI_DEV_2D_BUFFER PongBuffer[LCD_NUM_FRAMES_DISPLAY];

u32 FrameIndex = 2;
u8 *FramePtr[] = { Frame0, Frame1, Frame2 };

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
            
            memcpy(pBuffer->Data, FramePtr[FrameIndex], (LCD_PIXELS_PER_LINE * LCD_DATA_PER_PIXEL * LCD_LINES_PER_FRAME) );
            
            FrameIndex++;
            
            if( FrameIndex == 3 )
            	FrameIndex = 0;
            	
            
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
*   Function:    TEST_TOUCHLCD
*   Description: Main test routine will get launched from the POST 
*				 framework.
*******************************************************************/
int SHOW_IMAGES (void) 
{    
    u32     ResponseCount;
    u32     i;		
    u32     Result;

    ADI_LQ043T1DG01_TIMER_FLAG  Disp;
	
    bKeepRunning = true;
    
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
    for (i = 0; i < LCD_NUM_FRAMES_DISPLAY; i++) 
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
    PingBuffer[LCD_NUM_FRAMES_DISPLAY - 1].CallbackParameter = &PingBuffer[0];
    /* Terminate this buffer chain */
    PingBuffer[LCD_NUM_FRAMES_DISPLAY - 1].pNext = NULL;

    /* now do the same for the Pong buffers */
    /* FOR (Number of output 2D buffers) */
    for (i = 0; i < LCD_NUM_FRAMES_DISPLAY; i++) 
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
    PongBuffer[LCD_NUM_FRAMES_DISPLAY - 1].CallbackParameter = &PongBuffer[0];  
    /* Terminate this buffer chain */
    PongBuffer[LCD_NUM_FRAMES_DISPLAY - 1].pNext = NULL;
    
    memcpy(PingFrame, FramePtr[0], (LCD_PIXELS_PER_LINE * LCD_DATA_PER_PIXEL * LCD_LINES_PER_FRAME) );
    memcpy(PongFrame, FramePtr[1], (LCD_PIXELS_PER_LINE * LCD_DATA_PER_PIXEL * LCD_LINES_PER_FRAME) );
    
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
      
   	while( bKeepRunning )
   	{
   		LED_Bar(BLINK_SLOW);
   	}
   		
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
    
    adi_dev_Close(LCDDriverHandle);
    
   	return 1;
    
}


