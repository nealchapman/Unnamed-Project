short buttonTest = 0x0000;
short buttonState = 0x0000;
short buttonCount = 0x0000;

#define DEBOUNCE_COUNT 0XFF

int pollBF548EZKIT_LITEButtons(void)
{
/*
	int buttonNow;

	readPortState(B,0x0F00);
	buttonNow = (*pPORTB & 0x0F00) >> 2;

	if(buttonCount >= DEBOUNCE_COUNT)
	{
		buttonState = buttonTest;
	}

	if(buttonNow == buttonTest)
	{
		buttonCount++;
	}
	else
	{	
		buttonTest = buttonNow;
		buttonCount = 0;
	}

	return buttonState;
*/
}

void initBF548EZKIT_LITEButtons(void)
{
///	setPinDirection(V,0x0F00);
//	*pPORTB_FER &= ~0x0F00;
//	*pPORTB_INEN = 0x0F00;
}
