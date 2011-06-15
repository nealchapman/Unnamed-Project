
#include  <defBF549.h>
#include  <cdefBF549.h>
#include  <ccblkfn.h>

#include  "mxvr_test.h"



/*******************************************************************
*   Function:    TEST_MXVR
*   Description: Main test routine will get launched from the POST 
*				 framework.
*******************************************************************/
int TEST_MXVR(void)
{
   int				i;
   int 				timeout_cnt;
   unsigned short * pB0;
   unsigned StoredIMask;


   // Set Voltage Regulator for 1.2V
   *pVR_CTL = 0x00db;

   StoredIMask = cli();
   idle();
   sti(StoredIMask);



   // Set MTXONB to be in open drain mode, gate MTX with the MTXONB
   // state, and enable the MFS output.
   *pMXVR_PIN_CTL = MTXONBOD | MTXONBG | MFSOE;

   // Set up MXVR for master mode with MXVR disabled
   // msb=12 mmsm=1, mtxonb=1, mtxen=0
   *pMXVR_CONFIG = SET_MSB(12) | EPARITY | MTXONB | MMSM;



   // Configure Moab Ports for MXVR signals

   // PORTC_1 - MMCLK
   // PORTC_5 - MBCLK
   *pPORTC_MUX = (*pPORTC_MUX & 0xFFFFF3F3) | 0x00000404;
   *pPORTC_FER = (*pPORTC_FER & 0xFFDE) | 0x0022;


   // PORTG_11 - MTXONB
   *pPORTG_MUX = (*pPORTG_MUX & 0xFF3FFFFF) | 0x00400000;
   *pPORTG_FER = (*pPORTG_FER & 0xF7FF) | 0x0800;


   // PORTH_5 - MTX
   // PORTH_6 - MRX
   // PORTH_7 - MRXONB
   *pPORTH_MUX = (*pPORTH_MUX & 0xFFFF03FF) | 0x00000000;
   *pPORTH_FER = (*pPORTH_FER & 0xFF1F) | 0x00E0;




   // Output TXCLK on MFS output pin
   // Output RXCLK on MMCLK output pin
   // Output RXEDATA on MBCLK output pin
   *pMXVR_PLL_TEST = 0xB000;


   // Write the MXVR_CLK_CTL register (Master)
   // mfssync=0, mfssel=01, mfsdiv=0011, mfsen=0,
   // invrx=0, mbclkdiv=0011, mbclken=0,
   // pllsmps=000, mmclkmul=0001, mmclken=0,
   // clkx3sel=1, mxtalmul=10,
   // mostmode=0, mxtalfen=1, mxtalcen=1
   *pMXVR_CLK_CTL = 0x260602A3;



   // Enable the FMPLL state machine
   *pMXVR_FMPLL_CTL = *pMXVR_FMPLL_CTL | FMSMEN;

   // Wait for PFL indicating that the FMPLL is LOCKED.
   timeout_cnt=10000000;
   while ( !(*pMXVR_INT_STAT_0 & PFL) ) 
   {

		//time out
		if(timeout_cnt==0)
		{
		  return 0;
		}

		timeout_cnt--;
   }
     
   // Clear the PFL interrupt
   *pMXVR_INT_STAT_0 = PFL;
  
   
   
   // Set CDRCPSEL to 0xFF
   *pMXVR_CDRPLL_CTL = (*pMXVR_CDRPLL_CTL & ~CDRCPSEL)
                     | SET_CDRCPSEL((unsigned int)0xFF);

   // Enable the CDRPLL State Machine
   *pMXVR_CDRPLL_CTL = *pMXVR_CDRPLL_CTL | CDRSMEN;

   
   // Wait for PFL indicating that the CDRPLL is LOCKED.  
   timeout_cnt=10000000; 
   while ( !(*pMXVR_INT_STAT_0 & PFL) )
   {
    
      //time out
      if(timeout_cnt==0)
      {
		 return 0;
      }

      timeout_cnt--;
   }

   // Clear the PFL interrupt
   *pMXVR_INT_STAT_0 = PFL;
   
   
   
   // Read and write MXVR_CLK_CTL so that clock outputs
   // are setup and captured in the clk_x3 domain
   *pMXVR_CLK_CTL = *pMXVR_CLK_CTL;

   // Enable output clocks
   // mmclken=1, mbclken=1, mfsen=1
   *pMXVR_CLK_CTL = *pMXVR_CLK_CTL | MMCLKEN | MBCLKEN | MFSEN;


   // Setup MXVR

   *pMXVR_ROUTING_0 =  0x03020100;
   *pMXVR_ROUTING_1 =  0x07060504;
   *pMXVR_ROUTING_2 =  0x0B0A0908;
   *pMXVR_ROUTING_3 =  0x0F0E0D0C;
   *pMXVR_ROUTING_4 =  0x13121110;
   *pMXVR_ROUTING_5 =  0x17161514;
   *pMXVR_ROUTING_6 =  0x1B1A1918;
   *pMXVR_ROUTING_7 =  0x1F1E1D1C;
   *pMXVR_ROUTING_8 =  0x23222120;
   *pMXVR_ROUTING_9 =  0x27262524;
   *pMXVR_ROUTING_10 = 0x2B2A2928;
   *pMXVR_ROUTING_11 = 0x2F2E2D2C;
   *pMXVR_ROUTING_12 = 0x33323130;
   *pMXVR_ROUTING_13 = 0x37363534;
   *pMXVR_ROUTING_14 = 0x3B3A3938;

   *pMXVR_LADDR = 0x80000600;

   *(unsigned int*)pMXVR_CMRB_START_ADDR = 0xff805c00;
   *(unsigned int*)pMXVR_CMTB_START_ADDR = 0xff806200;
   *(unsigned int*)pMXVR_RRDB_START_ADDR = 0xff806300;
   *(unsigned int*)pMXVR_APRB_START_ADDR = 0xff800800;
   *(unsigned int*)pMXVR_APTB_START_ADDR = 0xff800700;   

   
   // Activate MXVR
   *pMXVR_CONFIG = (*pMXVR_CONFIG & ~MTXONB)           |
                    LMECH | APRXEN | RWRRXEN | NCMRXEN |
                    MTXEN | ACTIVE | MMSM    | MXVREN  ;


   // Set up synchronous data buffer to transmit
   pB0 = (unsigned short *) 0xff800900;
   for (i=0; i<0x20; i++)
   {
     *pB0 = (short) (((2*i+1)<<8) + 2*i); pB0++;
   }

   // Make logical channel assignments
   *pMXVR_SYNC_LCHAN_0 = 0x00000000;
   *pMXVR_SYNC_LCHAN_1 = 0x00000000;
   *pMXVR_SYNC_LCHAN_2 = 0x00000000;
   *pMXVR_SYNC_LCHAN_3 = 0x00000000;
   *pMXVR_SYNC_LCHAN_4 = 0x00000000;
   *pMXVR_SYNC_LCHAN_5 = 0x00000000;
   *pMXVR_SYNC_LCHAN_6 = 0x00000000;
   *pMXVR_SYNC_LCHAN_7 = 0x00000000;

   // Set up DMA0 to transmit data
   *(unsigned int*)pMXVR_DMA0_START_ADDR = 0xff800900;

   *pMXVR_DMA0_COUNT = 0x0040;
   *pMXVR_DMA0_CONFIG = 0x00001001;



    // Wait for Super Block Lock
     
    timeout_cnt=10000000; 
    while ( !(*pMXVR_STATE_0 & SBLOCK) )
    {
    
      //time out
      if(timeout_cnt==0)
      {
		return 0;
      }

      timeout_cnt--;
    }

	//succeed
   	return 1;
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

	bPassed = TEST_MXVR();
	
	return 0;
}
#endif //#ifdef _STANDALONE_


/******************************** End of File ********************************/
