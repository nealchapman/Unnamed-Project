/********************************************************************************
 MXVR Test Mode Define Section
********************************************************************************/
#define   MXVR_MEMTEST_CTL  0xffc028C8   /* MXVR Memory Test Control Register */
#define   MXVR_MEMTEST_DATA 0xffc028CC   /* MXVR Memory Test Data Register */
#define   MXVR_PLL_TEST     0xffc028E4   /* MXVR PLL Test Register */


#define   pMXVR_MEMTEST_CTL ((volatile unsigned short *)MXVR_MEMTEST_CTL)
#define   pMXVR_MEMTEST_DATA ((volatile unsigned short *)MXVR_MEMTEST_DATA)
#define   pMXVR_PLL_TEST ((volatile unsigned short *)MXVR_PLL_TEST)


#define   MOSTMODE  0x00000004

#define   CDRHOGGE  0x00400000

#define   CPTSEL     0x000F
#define   CDRIBTEN   0x0010
#define   CDRCPTEN   0x0020
#define   FMIBTEN    0x0040
#define   FMCPTEN    0x0080
#define   BYPRXCLK   0x0100
#define   BYPTXCLK   0x0200
#define   BYPCLKX3   0x0400
#define   BYPRXEDATA 0x0800
#define   TMRXCLK    0x1000
#define   TMTXCLK    0x2000
#define   TMCLKX3    0x4000
#define   TMRXEDATA  0x8000

#define   SET_CPTSEL(x)   ( (x) & 0xF )


#define   MEMTEST_NUM  0x000F
#define   MEMTEST_WEN  0x0010
#define   MEMTEST_REN  0x0020
#define   MEMTEST_BENX 0x00C0
#define   MEMTEST_BEN0 0x0040
#define   MEMTEST_BEN1 0x0080 
#define   MEMTEST_ADDR 0x7F00
#define   MEMTEST      0x8000 

#define   SET_MEMTEST_NUM(x)   ( (x) & 0xF )
#define   SET_MEMTEST_ADDR(x)  ( ( (x) & 0x7F ) << 8)
