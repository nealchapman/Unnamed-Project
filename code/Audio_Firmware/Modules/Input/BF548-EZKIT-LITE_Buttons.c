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

