void initDMA(void)
{
	*pDMA0_CONFIG = 0x0000;
	*pDMA1_CONFIG = 0x0000;
	*pDMA0_IRQ_STATUS = 0x0000;
	*pDMA1_IRQ_STATUS = 0x0000;

	*pDMA0_PERIPHERAL_MAP = PMAP_SPORT0RX;
	*pDMA1_PERIPHERAL_MAP = PMAP_SPORT0TX;

	*pDMA0_CONFIG = FLOW_AUTO | DI_EN | WDSIZE_16 | WNR;
	*pDMA0_START_ADDR = (void *)Rx0Buffer;
	*pDMA0_X_COUNT = 8;
	*pDMA0_X_MODIFY = 2;

	*pDMA1_CONFIG = FLOW_AUTO | DI_EN | WDSIZE_16;
	*pDMA1_START_ADDR = (void *)Tx0Buffer;
	*pDMA1_X_COUNT = 8;
	*pDMA1_X_MODIFY = 2;

	//MAGIC!!!
	delayus(2000);	

}

cycle_t cycle_start = 0x0000;
cycle_t cycle_stop = 0x0000;


