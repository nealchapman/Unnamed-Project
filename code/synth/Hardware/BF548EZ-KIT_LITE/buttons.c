int pollButtons(void)
{

	int buttonNow;

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

}

void initButtons(void)
{
	*pPORTB_FER &= ~0x0F00;
	*pPORTB_INEN = 0x0F00;
}

short buttonTest = 0x0000;
short buttonState = 0x0000;
short buttonCount = 0x0000;


#define DEBOUNCE_COUNT 0XFF
