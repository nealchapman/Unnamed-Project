/*******************************************************************
Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software
you agree to the terms of the associated Analog Devices License Agreement.  

Project Name:  	Power_On_Self_Test

Hardware:		ADSP-BF548 EZ-KIT Lite

Description:	This file tests the keypad interface on the EZ-KIT
*******************************************************************/

/*******************************************************************
*  include files
*******************************************************************/
#include <cdefBF548.h>
#include <ccblkfn.h>
#include <stdio.h>
#include "PBLED_test.h"


/*******************************************************************
*  function prototypes
*******************************************************************/
void Init_Keypad(void);
int TEST_KEYPAD(void);


/*******************************************************************
*   Function:    Init_Keypad
*   Description: This function configures the Keypad and initializes
*				 the PORT pins that are used
*******************************************************************/
void Init_Keypad(void)
{
    
    *pPORTD_FER = 0xFF00;
    *pPORTD_MUX = 0xFFFF0000;
     
   	*pKPAD_PRESCALE = 0x1F; 
	*pKPAD_MSEL = 0xFF0;
    *pKPAD_CTL = 0x6C05;  // enable 0-3 columns, enable 0-3 rows, single and multiple key interrupt
    
}

/*******************************************************************
*   Function:    TEST_KEYPAD
*   Description: Main test routine will get launched from the POST 
*				 framework.
*******************************************************************/
int TEST_KEYPAD(void)
{
	int nIndex = 0;
	int bSuccess = 1; 	// returning 1 indicates a pass, anything else is a fail
	int n;
	int KeyStat, Key = 0;
	int KeysPressed = 0;
	int temp = 0;

	// initialize the keypad register
	Init_Keypad();

	// wait in here until all keys are pressed
	while(KeysPressed != 0xFFFF)
	{
	    KeyStat = *pKPAD_STAT;
	    if ( KeyStat & 0x1)  // test for key pressed
	    {
	    	if ( KeyStat & 0x2)  // test for single key pressed
 			{
	    		Key = *pKPAD_ROWCOL;
	    		
	    		switch (Key)
	    		{
	    		    case 0x0808:
	    		    	// 0 pressed
	    		    	ClearSet_LED_Bank( (LED1|LED2|LED3|LED4|LED5|LED6), 0x0 );
	    		    	temp = 0x1;
	    		    	KeysPressed |= (temp << 0);
	    	    		break;
	    	    	case 0x0804:
	    	    		// 1 pressed
	    	    		ClearSet_LED_Bank( (LED1|LED2|LED3|LED4|LED5|LED6), 0x1 );
	    	    		temp = 0x1;
	    		    	KeysPressed |= (temp << 1);
	    	    		break;
	    	    	case 0x0802:
	    	    		// 2 pressed
	    	    		ClearSet_LED_Bank( (LED1|LED2|LED3|LED4|LED5|LED6), 0x4 );
	    	    		temp = 0x1;
	    		    	KeysPressed |= (temp << 2);
	    	    		break;
	    	    	case 0x0801:
	    	    		// 3 pressed
	    	    		ClearSet_LED_Bank( (LED1|LED2|LED3|LED4|LED5|LED6), 0x5 );
	    	    		temp = 0x1;
	    		    	KeysPressed |= (temp << 3);
	    	    		break;
	    	    	case 0x0408:
	    	    		// 4 pressed
	    	    		ClearSet_LED_Bank( (LED1|LED2|LED3|LED4|LED5|LED6), 0x10 );
	    	    		temp = 0x1;
	    		    	KeysPressed |= (temp << 4);
	    	    		break;
	    	    	case 0x0404:
	    	    		// 5 pressed
	    	    		ClearSet_LED_Bank( (LED1|LED2|LED3|LED4|LED5|LED6), 0x11 );
	    	    		temp = 0x1;
	    		    	KeysPressed |= (temp << 5);
	    	    		break;
	    	    	case 0x0402:
	    	    		// 6 pressed
	    	    		ClearSet_LED_Bank( (LED1|LED2|LED3|LED4|LED5|LED6), 0x14 );
	    	    		temp = 0x1;
	    		    	KeysPressed |= (temp << 6);
	    	    		break;
	    	    	case 0x0401:
	    	    		// 7 pressed
	    	    		ClearSet_LED_Bank( (LED1|LED2|LED3|LED4|LED5|LED6), 0x15 );
	    	    		temp = 0x1;
	    		    	KeysPressed |= (temp << 7);
	    	    		break;
	    	    	case 0x0208:
	    	    		// 8 pressed
	    	    		ClearSet_LED_Bank( (LED1|LED2|LED3|LED4|LED5|LED6), 0x40 );
	    	    		temp = 0x1;
	    		    	KeysPressed |= (temp << 8);
	    	    		break;
	    	    	case 0x0204:
	    	    		// 9 pressed
	    	    		ClearSet_LED_Bank( (LED1|LED2|LED3|LED4|LED5|LED6), 0x41 );
	    	    		temp = 0x1;
	    		    	KeysPressed |= (temp << 9);
	    	    		break;
	    	    	case 0x0202:
	    	    		// A pressed
	    	    		ClearSet_LED_Bank( (LED1|LED2|LED3|LED4|LED5|LED6), 0x44 );
	    	    		temp = 0x1;
	    		    	KeysPressed |= (temp << 10);
	    	    		break;
	    	    	case 0x0201:
	    	    		// B pressed
	    	    		ClearSet_LED_Bank( (LED1|LED2|LED3|LED4|LED5|LED6), 0x45 );
	    	    		temp = 0x1;
	    		    	KeysPressed |= (temp << 11);
	    	    		break;
	    	    	case 0x0108:
	    	    		// C pressed
	    	    		ClearSet_LED_Bank( (LED1|LED2|LED3|LED4|LED5|LED6), 0x50 );
	    	    		temp = 0x1;
	    		    	KeysPressed |= (temp << 12);
	    	    		break;
	    	    	case 0x0104:
	    	    		// D pressed
	    	    		ClearSet_LED_Bank( (LED1|LED2|LED3|LED4|LED5|LED6), 0x51 );
	    	    		temp = 0x1;
	    		    	KeysPressed |= (temp << 13);
	    	    		break;
	    	    	case 0x0102:
	    	    		// E pressed
	    	    		ClearSet_LED_Bank( (LED1|LED2|LED3|LED4|LED5|LED6), 0x54 );
	    	    		temp = 0x1;
	    		    	KeysPressed |= (temp << 14);
	    	    		break;
	    	    	case 0x0101:
	    	    		// F pressed
	    	    		ClearSet_LED_Bank( (LED1|LED2|LED3|LED4|LED5|LED6), 0x55 );
	    	    		temp = 0x1;
	    		    	KeysPressed |= (temp << 15);
	    	    		break;
	    	    	default:
	    	    		// light them all for an error
	    	    		ClearSet_LED_Bank( (LED1|LED2|LED3|LED4|LED5|LED6), 0x555 );
	    	    	break;
	    		}
 			}
 					  		
	    	*pKPAD_ROWCOL = Key;  // clear register
	    	*pKPAD_STAT = KeyStat;  // clear interrupt
	    }   
	}
	
	return bSuccess;
}

/*******************************************************************
*   Function:    main
*   Description: used only if this test is needed as a standalone
*				 test separate from POST
*******************************************************************/
#ifdef _STANDALONE_ 
int main(void)
{
	int bPassed = 1;
	int count = 0;

	bPassed = TEST_KEYPAD();
	
	return 0;
}
#endif //#ifdef _STANDALONE_
