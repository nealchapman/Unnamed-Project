/*******************************************************************
Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software
you agree to the terms of the associated Analog Devices License Agreement.  

Project Name:  	Power_On_Self_Test

Hardware:		ADSP-BF548 EZ-KIT Lite

Description:	This file tests the Ethernet interface on the EZ-KIT.
*******************************************************************/

/*******************************************************************
*  include files
*******************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <services\services.h>
#include <drivers\adi_dev.h>
#include <drivers\ethernet\ADI_ETHER_LAN9218.h>
#include "adi_ssl_Init.h"

#if defined(__ADSPBF533__)
#include <cdefbf533.h>
#elif defined(__ADSPBF561__)
#include <cdefbf561.h>
#elif defined(__ADSPBF538__)
#include <cdefbf538.h>
#elif defined(__ADSPBF548__)
#include <cdefbf548.h>
#endif /* __ADSPBF533__ */


/*******************************************************************
*  global variables and defines
*******************************************************************/

/*****************************************************************************
 *
 *        Configuration Parameters
 **/
#define _ETHERNET_DEBUG_               /* Enables Output Messages */
#define NO_FRAMES        1000            /* Total Number of Test frames */
#define NEGOTIATE                        /* Driver starts in Auto Neg mode */
#define FILL_AND_CHECK                   /* A pattern is filled and checked */
#define NO_RCVES         20              /* Number of receive buffers  */
#define NO_XMITS          1              /* Number of transmit buffers */

//#define SET_LOOPBACK
//#define CHECK_FRAMES
//#define BOARD_TO_BOARD
//#define SENDING

#ifdef _ETHERNET_DEBUG_
#define DEBUG_PRINT(_str) do { \
		printf("%s\n",_str);   \
}while(0)
#else
#define DEBUG_PRINT(_str)
#endif /* _ETHERNET_DEBUG_ */

/****************************************************************************
 *
 *  Structure Declarations
 *
 **/
typedef struct template_info {
	short			id;
	short			lnth;
	unsigned char	pattern;
} TEMPLATE_INFO;

typedef struct stats {
	unsigned int	nbytes;
	unsigned int	nframes;
} STATS;

typedef struct buffer {
	struct buffer			*next;
	ADI_ETHER_BUFFER		ctrl;
	struct buffer			*chain;
	ADI_ETHER_FRAME_BUFFER	hdr;
	int						template_id;
	unsigned char			data[1600];
} BUFFER;

/****************************************************************************
 *
 *  Global Varaibles
 *
 **/
#pragma alignment_region (4)
char dcbqueue_storage[16*ADI_DCB_ENTRY_SIZE];
char BaseMemSize[ADI_ETHER_MEM_LAN9218_BASE_SIZE];
char MemRcve[NO_RCVES*ADI_ETHER_MEM_LAN9218_PER_RECV];
char MemXmit[NO_XMITS*ADI_ETHER_MEM_LAN9218_PER_XMIT];

#pragma alignment_region_end
#ifdef CHECK_FRAMES
volatile bool FrameRcvd,FrameXmit;
int FrameSize=0;
bool SizeOK[2000];
#ifndef BOARD_TO_BOARD
#ifndef SENDING
#define SENDING
#endif
#endif
#endif /* CHECK_FRAMES */

#define NO_BUFFERS  NO_RCVES+NO_XMITS+10
#define MAX_RCVES (NO_RCVES+2)
#define MAX_XMITS (NO_XMITS)

bool g_SystemInitialized= false;
int heap_ndx=-1;
u16 phyregs[32];
int CurXmit  = 0,CurRcve=0;
int NoRcvd   = 0, NoXmit    = 0, NoInts    = 0;
int NoWrites = 0, NoXmitErrs= 0, NoRcvdErrs= 0, NoIgnored= 0;

static TEMPLATE_INFO templates[]={
	{0, 256,0xa1}, // always fails
	{3, 512,0x33}, // always fails
	{1,1496,0x10}, // never
	{2,  60,0x04}, // never
	{4, 128,0x5a}, // never fails
	{5,1024,0x00}, // never fails
	{6,  60,0x43}, // never fails
	{7, 129,0x63}, // never
	{8,1301,0x89}, // never
	{9,  65,0x77}  //never
	};

STATS CurRxStat = {0} ,CurTxStat ={0};
STATS AvgRxStat[10],AvgTxStat[10];
bool TxActive= true;
int CurMinute,DumpStats;
int NoTemplates = sizeof(templates)/sizeof(TEMPLATE_INFO);
int CtTemplates[100];
BUFFER *FreeBuf=NULL; BUFFER *FirstBuffer;

unsigned int MinRecvBufsize = 0, BufferPrefix=0;
int SizeOfAdiEtherBuffer = sizeof(ADI_ETHER_BUFFER[2])/2;
ADI_DEV_DEVICE_HANDLE lan_handle;
void *DriverHandle;

ADI_ETHER_SUPPLY_MEM memtable = {
	MemRcve,sizeof(MemRcve),0,
	MemXmit,sizeof(MemXmit),0,
	BaseMemSize,sizeof(BaseMemSize)};

#ifdef USBLAN
char MacAddr[] = {0x00,0x01,0x02,0x03,0x04,0x05};
#else
#ifdef SENDING
char MacAddr[] = {0x00,0x01,0x02,0x03,0x04,0x06};
#else
char MacAddr[] = {0x00,0x01,0x02,0x03,0x04,0x07};
#endif
#endif

/**
 *    Device I/O Control Command table
 **/
ADI_DEV_CMD_VALUE_PAIR setup[] ={
	{ADI_ETHER_CMD_SET_MAC_ADDR,MacAddr},
	{ADI_ETHER_CMD_GET_BUFFER_PREFIX,&BufferPrefix},
	{ADI_ETHER_CMD_GET_MIN_RECV_BUFSIZE,&MinRecvBufsize},
#ifndef NEGOTIATE
	{ADI_ETHER_CMD_SET_SPEED,(void *)2 /* 100 */},
	{ADI_ETHER_CMD_SET_FULL_DUPLEX,(void *)TRUE},
#else
	{ADI_ETHER_CMD_SET_NEGOTIATE,(void *)TRUE},
#endif
	{ADI_DEV_CMD_SET_DATAFLOW_METHOD,(void *)ADI_DEV_MODE_CHAINED},
	{ADI_DEV_CMD_SET_DATAFLOW,(void *)TRUE},
#ifdef SET_LOOPBACK
	{ADI_ETHER_CMD_SET_LOOPBACK,(void *)TRUE},
#endif
	{ADI_DEV_CMD_END,(void *)0}
};

/************************************************************************
 *
 *      Local Functions
 **/
TEMPLATE_INFO *NextTemplate(void)
{
	int ndx = rand()%(sizeof(templates)/sizeof(templates[0]));
	return templates+ndx;
}

TEMPLATE_INFO *GetTemplate(int id)
{
	int ndx = id%(sizeof(templates)/sizeof(templates[0]));
	return templates+ndx;
}

/*******************************************************************
*  function prototypes
*******************************************************************/
void AllocateBuffers(int heap_ndx);
BUFFER *GetBuffer(void);
void FreeBuffer(BUFFER *buf);
void DumpBuffer(ADI_ETHER_BUFFER *bf, bool tx);
int IncrInt(int *val);
int DecrInt(int *val);
void IssueRead(ADI_DEV_DEVICE_HANDLE lan_handle);
int  IssueWrite(ADI_DEV_DEVICE_HANDLE lan_handle);
void IncrIgnored(void);
void LanCallback (void* client_handle, u32  event, void *parm);
int GetRtcSecs(void);
static int InitDriver(void *arg);
static int InitRealTimeClock(unsigned int inSeconds);


/*******************************************************************
*   Function:    AllocateBuffers
*   Description: Configures the given heap into list of buffers, 
*				 FirstBuffer points to the head of the list.
*******************************************************************/
void AllocateBuffers(int heap_ndx)
{
	BUFFER *nxt;
	int i;

	for (i=0;i<NO_BUFFERS;i++)
	{
		nxt = (BUFFER *)heap_malloc(heap_ndx,sizeof(BUFFER));
		if (nxt == NULL)
		{
			DEBUG_PRINT("Failed to Allocate Buffers");
		}
		nxt->next = FreeBuf;
		nxt->chain = FreeBuf;
		FreeBuf = nxt;
	}
	FirstBuffer = FreeBuf;
}

/*******************************************************************
*   Function:    GetBuffer
*   Description: Returns a free buffer.
*******************************************************************/
BUFFER *GetBuffer(void)
{
	void *xit = adi_int_EnterCriticalRegion(NULL);
	BUFFER *buf;

	buf = FreeBuf;
	if (buf != NULL)
	{
		FreeBuf = buf->next;
	}
	adi_int_ExitCriticalRegion(xit);
	return buf;
}

/*******************************************************************
*   Function:    FreeBuffer
*   Description: Adds buffer to the free list
*******************************************************************/
void FreeBuffer(BUFFER *buf)
{
	void *xit = adi_int_EnterCriticalRegion(NULL);
	buf->next = FreeBuf;
	FreeBuf = buf;
	adi_int_ExitCriticalRegion(xit);
}

/*******************************************************************
*   Function:    DumpBuffer
*   Description: Dumps contents of the buffer.
*******************************************************************/
void DumpBuffer(ADI_ETHER_BUFFER *bf, bool tx)
{
#ifdef DUMP_BUFFER
	unsigned short *elnth=(unsigned short *)bf->Data;
	int ninline=0;
	int nbytes,nodump;
	int cmpl;
	unsigned char *dmp;

	cmpl = (tx?bf->StatusWord&1:bf->StatusWord&0x1000);
	printf("%s %s StatusWord = %8.8x\n", (tx?"TX":"RX"),(cmpl?"Completed":" "),bf->StatusWord);

	nbytes = (tx?bf->StatusWord>>16:bf->StatusWord)&0x7ff;
	printf("Embedded no bytes = %d, lnth from status word = %d\n",*elnth,nbytes);

	nbytes = (tx?bf->StatusWord>>16:bf->StatusWord)&0x7ff;

	if (tx)
	{
		nodump = *elnth+2;
		if (cmpl && (nodump<nbytes+2)) nodump = nbytes+2;
	}
	else
	{
		nodump = (nbytes==0?32:nbytes+2);
	}

	dmp = (unsigned char *)bf->Data;
	ninline = 0;
	while (nodump>0)
	{
		printf("%2.2x ",*dmp++);
		ninline++;
		if (ninline>=16)
		{
			ninline = 0;
			printf("\n");
		}
		nodump--;
	}
	printf("\n-----------------------------------------------------\n");
#endif /* DUMP_BUFFER */
}

/*******************************************************************
*   Function:    IncrInt
*   Description: Atomic increment / Decrement Routines
*******************************************************************/
int IncrInt(int *val)
{
	void *xit = adi_int_EnterCriticalRegion(NULL);
	int vl = *val + 1;
	*val = vl;
	adi_int_ExitCriticalRegion(xit);
	return vl;
}

/*******************************************************************
*   Function:    DecrInt
*   Description: Atomic increment / Decrement Routines
*******************************************************************/
int DecrInt(int *val)
{
	void *xit = adi_int_EnterCriticalRegion(NULL);
	int vl = *val - 1;
	*val = vl;
	adi_int_ExitCriticalRegion(xit);
	return vl;
}

/*******************************************************************
*   Function:    IssueRead
*   Description: Lends a free buffer to the driver. Buffer 
*				 parameters are initialized.
*******************************************************************/
void IssueRead(ADI_DEV_DEVICE_HANDLE lan_handle)
{
	void *xit = adi_int_EnterCriticalRegion(NULL);
	BUFFER *buf = GetBuffer();
	int result;

	if (buf != NULL)
	{
		// build the structure
		memset(&buf->ctrl,0,SizeOfAdiEtherBuffer);
		buf->ctrl.Data = &buf->hdr;	// start of the data buffer
		buf->ctrl.ElementCount = sizeof(BUFFER)-sizeof(void *)-sizeof(ADI_ETHER_BUFFER);
		buf->ctrl.ElementWidth = 1;
		buf->ctrl.CallbackParameter = buf;
		result = adi_dev_Read(lan_handle,ADI_DEV_1D,(ADI_DEV_BUFFER *)&buf->ctrl);
		if (result != ADI_DEV_RESULT_SUCCESS)
		{
			DEBUG_PRINT("adi_dev_Read Failed");
		}
		else
		{
			IncrInt(&CurRcve);
		}
	}
	adi_int_ExitCriticalRegion(xit);
}

/*******************************************************************
*   Function:    IssueWrite
*   Description: Gets a free buffers, sets the destination address to 
*				 be Broadcast address fills the frame with a pattern 
*				 and send it across.
*******************************************************************/
int  IssueWrite(ADI_DEV_DEVICE_HANDLE lan_handle)
{
	void *xit = adi_int_EnterCriticalRegion(NULL);
	ADI_ETHER_FRAME_BUFFER *frm;
	int result;
	BUFFER *buf;
	TEMPLATE_INFO *tm = NextTemplate();
	int i,*d;
	int ok=1;
	unsigned short int lt;

	if (TxActive)
	{
		buf = GetBuffer();
		if (buf != NULL)
		{
			CtTemplates[tm->id]++;
			// build the structure
			memset(&buf->ctrl,0,SizeOfAdiEtherBuffer);

			// setup ethernet header
			frm = (ADI_ETHER_FRAME_BUFFER *)&buf->hdr;
#ifdef SET_LOOPBACK
			memset(frm->Dest,0xff,6);	// broadcast
#else
			memcpy(frm->Dest,MacAddr,6);
#endif
			memcpy(frm->Srce,MacAddr,6);
#ifdef CHECK_FRAMES
			lt = sizeof(int)+FrameSize;
#else
			lt = sizeof(int)+tm->lnth;
#endif
			frm->LTfield[0] = (lt>>8)&0xff;
			frm->LTfield[1] = lt&0xff;
			frm->NoBytes = lt + sizeof(ADI_ETHER_FRAME_BUFFER)-2;

#ifdef FILL_AND_CHECK
			// layout the frame contents
            #ifdef CHECK_FRAMES
		 	buf->template_id = FrameSize;
			memset(buf->data,0x5a,FrameSize);
            #else
			buf->template_id = tm->id;
			memset(buf->data,tm->pattern,tm->lnth);
            #endif
#endif /* FILL_AND_CHECK */

			// layout the buffer control structure
			buf->ctrl.Data = &buf->hdr;	// start of the data buffer
			buf->ctrl.ElementCount = frm->NoBytes+2;
			buf->ctrl.ElementWidth = 1;
			buf->ctrl.CallbackParameter = buf;
#if 0
			if ((rand()&0x3)==1)
			{
				buf->ctrl.ElementCount = 32;
				buf->ctrl.PayLoad = ((char *)buf->ctrl.Data) +buf->ctrl.ElementCount;

			}
#endif
			DumpBuffer(&buf->ctrl,1);
			result = adi_dev_Write(lan_handle,ADI_DEV_1D,(ADI_DEV_BUFFER *)&buf->ctrl);
			if (result != ADI_DEV_RESULT_SUCCESS)
			{
				DEBUG_PRINT("adi_dev_Write Failed");
			}
			else
			{
				IncrInt(&NoWrites);
#ifndef CHECK_FRAMES
				if (NoWrites>=NO_FRAMES)
				{
					TxActive =  false;
				}
#endif
				IncrInt(&CurXmit);
			}
		}
		else
		{
			ok =0;
		}
	}
	// RX_OVER_RUN errors FIXME:
	// The driver is not fast enough in loopback mode
	// Check the driver if anywhere we can optimize.
	i=0;
	for (i=0;i<10000;i++)
		;

	adi_int_ExitCriticalRegion(xit);
	return ok;
}

/*******************************************************************
*   Function:    IncrIgnored
*   Description: Ignore the increment
*******************************************************************/
void IncrIgnored(void)
{
	IncrInt(&NoIgnored);
}

/*******************************************************************
*   Function:    LanCallback
*   Description: Callback that gets called when Buffer has been 
*				 transmitted or a buffer is received on the Ethernet.
*******************************************************************/
void LanCallback (void* client_handle, u32  event, void *parm)
{
	BUFFER *nbuf = (BUFFER *)parm, *nxt_buffer;
	ADI_ETHER_BUFFER *buf, *nxt_ether_buffer;
	ADI_ETHER_FRAME_BUFFER *frm;
	int result;
	ADI_DEV_DEVICE_HANDLE handle = *((ADI_DEV_DEVICE_HANDLE *)client_handle);
	unsigned char *ch;
	int i,ok,reuse,nx;

	void *xit = adi_int_EnterCriticalRegion(NULL);

	if (handle != DriverHandle)
	{
		int k=4;
	}
	if (event != ADI_ETHER_EVENT_INTERRUPT)
	{
		nbuf = (BUFFER *)parm;
		while (nbuf != NULL)
		{
			buf = &nbuf->ctrl;
			nxt_ether_buffer = buf->pNext;
			if (nxt_ether_buffer != NULL)
			{
				nxt_buffer = (BUFFER *)(((char *)nxt_ether_buffer) - sizeof(struct buffer*));
			}
			else
			{
				nxt_buffer = NULL;
			}
			buf->pNext = NULL;
			if (event == ADI_ETHER_EVENT_FRAME_RCVD)
			{
				DecrInt(&CurRcve);
				DumpBuffer(buf,0);
				reuse = 1;
				// get the status word and see if the reported received length is correct
				int Status = buf->StatusWord;

				if ((Status&0x3000) == 0x3000)
				{
					// valid frame received
					int nobytes;
					unsigned char lst,nxt;

					IncrInt(&NoRcvd);
					frm = (ADI_ETHER_FRAME_BUFFER *)buf->Data;
					nobytes = frm->NoBytes-4; // 4 bytes of CRC
					if (nobytes>128) reuse=0;
					CurRxStat.nframes++;
					CurRxStat.nbytes += nobytes;
	#ifdef FILL_AND_CHECK
					if (nobytes==60)
					{
						// short frame padded out
						unsigned short int lt;
						lt = (frm->LTfield[0]<<8)+frm->LTfield[1];
						nobytes = lt+sizeof(ADI_ETHER_FRAME_BUFFER)-2;
					}

#ifdef CHECK_FRAMES
					if (1)
					{
						TEMPLATE_INFO xx;
						TEMPLATE_INFO *tm = &xx;
						int *fsize = (int *)frm->Data;
						xx.lnth = FrameSize;
						xx.pattern = 0x5a;
#ifdef BOARD_TO_BOARD
						SizeOK[*fsize] = true;
#endif

#else
					if ((0<=nbuf->template_id) && (nbuf->template_id<=9))
					{
						TEMPLATE_INFO *tm = GetTemplate(nbuf->template_id);

#endif

						if (nobytes == sizeof(ADI_ETHER_FRAME_BUFFER)-2+sizeof(int)+tm->lnth)
						{
							ok = 1;
							ch = nbuf->data;
							i = 0;
							while (i<tm->lnth)
							{
								if (*ch++ != tm->pattern)
								{
									ok = 0;
									break;
								}
								i++;
							}
							if (ok ==0)
							{
								IncrIgnored();
							}
						}
						else
						{
							// the no. of bytes is incorrect
							IncrIgnored();

						}
					}
					else
					{
						// ignore the frame
						IncrIgnored();
					}

#endif
				}
				else
				{
					IncrInt(&NoRcvdErrs);
				}
				FreeBuffer(nbuf);
				IssueRead(handle);
#ifdef CHECK_FRAMES
				FrameRcvd=true;
#endif

			}
			else
			{
				if (event == ADI_ETHER_EVENT_FRAME_XMIT)
				{
					// get the status word and see if the reported received length is correct
					int Status = buf->StatusWord;
#ifdef CHECK_FRAMES
					FrameXmit = true;
#endif

					DecrInt(&CurXmit);
					DumpBuffer(buf,1);

					if ((Status&0x3) == 0x3) {
						// valid frame transmitted
						int nobytes = buf->ProcessedElementCount;
						frm = (ADI_ETHER_FRAME_BUFFER *)buf->Data;
						if (frm->NoBytes != nobytes-2)
						{
							// count mismatch
							//printf("xmit count mismatch frm=%d, proc=%d\n",frm->NoBytes,nobytes);
							DEBUG_PRINT("Transmit Count Mismatch");
						}
						else
						{
							NoXmit++;
							CurTxStat.nframes++;
							CurTxStat.nbytes += nobytes;
						}
					}
					else
					{
						NoXmitErrs++;
					}
					FreeBuffer(nbuf);
#ifndef CHECK_FRAMES
					IssueWrite(handle);
#endif
				}
				else
				{
					// it must be an interrupt event
					IncrInt(&NoInts);

				}
			}
			nbuf = nxt_buffer;
		}
	}
	else
	{
		// ethernet event
		;
	}
	adi_int_ExitCriticalRegion(xit);
}

/*******************************************************************
*   Function:    GetRtcSecs
*   Description: Get seconds from the RTC
*******************************************************************/
int GetRtcSecs(void)
{
	unsigned int clk = *pRTC_STAT;
	return (((clk>>12)&0xf)*3600) + (((clk>>6)&0x3f)*60) + (clk&0x3f);
}

/************************************************************************
 *
 * Setup BF533 with best possible SCLK,CCLK settings.
 * Note: One has to be careful moving this code to other processors as
 * it my break the part if configured wrongly.
 *
 **/
#ifdef __ADSPBF533__
void setup_bf533_using_ssl(void)
{
	// initialize power management module
	ADI_PWR_COMMAND_PAIR ezkit[] =
	{
		{ ADI_PWR_CMD_SET_EZKIT, (void*)ADI_PWR_EZKIT_BF533_600MHZ },
		//{ ADI_PWR_CMD_SET_PC133_COMPLIANCE, FALSE},
		{ ADI_PWR_CMD_END, 0}
	};
	adi_pwr_Init(ezkit);

	// initialize EBIU module
	//adi_ebiu_Init(sdram_config,0);

	// set max speed
	adi_pwr_SetFreq(0,0,ADI_PWR_DF_NONE);
	u32 fcclk,fsclk,fvco;
	adi_pwr_GetFreq(&fcclk,&fsclk,&fvco);
	printf("BF533 board running at %dMHz CCLK and %dMHz SCLK\n",fcclk,fsclk);
}
#endif /* __ADSPBF533__ */

/*******************************************************************
*   Function:    InitDriver
*   Description: Initialize the System Services Libraries and LAN91c111 
*				 driver. Done only once.
*******************************************************************/
static int InitDriver(void *arg)
{
	ADI_DCB_HANDLE	dcbmgr_handle;
	unsigned int result,mask;
	u32 response_count;
	int i,recv_data=0;
	ADI_ETHER_MEM_SIZES msizes;
	ADI_ETHER_BUFFER *buf;
	long long int last_irl=0;
	static int critical_handle;
	int linkUp=false;
	int checkCount = 0;

   	if(g_SystemInitialized)
   	     return 0;
   	     
   	// allocate memory for our buffers
	heap_ndx = heap_install((void *)16,0x20000,127);

	AllocateBuffers(heap_ndx);
	
   	result = adi_dcb_Open(ik_ivg14, dcbqueue_storage, sizeof(dcbqueue_storage), &response_count, &dcbmgr_handle);
   	
 	// open lan-device
	//
	result = adi_dev_Open(  adi_dev_ManagerHandle,
							&ADI_ETHER_LAN9218_Entrypoint,
							0,
							&lan_handle,
							&lan_handle,
							ADI_DEV_DIRECTION_BIDIRECTIONAL,
							NULL,
							dcbmgr_handle,
							LanCallback);

	if(result != ADI_DEV_RESULT_SUCCESS)
	{
		DEBUG_PRINT("Failed to open the landevice \n");
		return -1;
	}
	// get memory siea
	mask = ADI_ETHER_CMD_MEM_SIZES;
	result = adi_dev_Control(lan_handle,ADI_ETHER_CMD_MEM_SIZES,&msizes);
	if (result == ADI_DEV_RESULT_SUCCESS)
	{

		result = adi_dev_Control(lan_handle,ADI_ETHER_CMD_SUPPLY_MEM,&memtable);
		if (result == ADI_DEV_RESULT_SUCCESS)
		{
			DriverHandle = lan_handle;
			result = adi_dev_Control(lan_handle,ADI_DEV_CMD_TABLE,&setup);
			if (result == ADI_DEV_RESULT_SUCCESS)
			{
				// provide the driver with some buffers before starting the device
				while ((FreeBuf) && (FreeBuf->next))
				{
					IssueRead(lan_handle);
				}
				// now we start the controller
				result = adi_dev_Control(lan_handle,ADI_ETHER_CMD_START,NULL);
				if (result == ADI_DEV_RESULT_SUCCESS)
				{
					adi_dev_Control(lan_handle,ADI_ETHER_CMD_GET_PHY_REGS,phyregs);
					do
					{

					   // check if link is already up
					    if (((phyregs[1] & 0x4)==0x4))
					    {
                            linkUp = true;
					        break;
					    }
						// read the PHY registers again
					   adi_dev_Control(lan_handle,ADI_ETHER_CMD_GET_PHY_REGS,phyregs);
					checkCount++;
					}while(checkCount < 10);

				     if(linkUp == true)
				     {
				        DEBUG_PRINT("Link Established");
				     }
				     else
				     {
				   	  	return -1;
				     }
				}
				else
				{
					DEBUG_PRINT("adi_dev_Control for CMD_START returned Error");
				}
			}
			else
			{
				DEBUG_PRINT("adi_dev_Control for CMD_TABLE returned Error");
			}
		}
	}
	else
	{
		DEBUG_PRINT("adi_dev_Control returned Error");
	}

 g_SystemInitialized = true;
 return 1;
}

/*******************************************************************
*   Function:    InitRealTimeClock
*   Description: Initialize Real Time Clock (RTC). RTC peripheral 
*				 is used to keep track of time.
*******************************************************************/
static int InitRealTimeClock(unsigned int inSeconds)
{
 unsigned long clock,min;
 int StartMin,EndMin;

		clock = *pRTC_STAT;
		StartMin = (clock>>6)&0x3ff;
		EndMin = StartMin+5;

		*pRTC_PREN = 1;	/* enable prescaler and the 1Khz clock */
		while ((*pRTC_ISTAT&0x8000)==0);	/* wait for writes to complete */

		while ((*pRTC_ISTAT&4)==0);  // wait for seconds event
		*pRTC_STAT = 0;
		while ((*pRTC_ISTAT&0x8000)==0);	/* wait for writes to complete */

		clock = *pRTC_STAT;
		StartMin = (clock>>6)&0x3ff;
		EndMin = StartMin+5;
		return 1;
}

/*******************************************************************
*   Function:    TEST_ETHERNET
*   Description: Main test routine will get launched from the POST 
*				 framework.
*******************************************************************/
int TEST_ETHERNET(void)
{
	int rtcsecs;
	unsigned long clock,min;
	int StartMin,EndMin;
	ADI_ETHER_STATISTICS_COUNTS cts;
	bool ret=false;

	if((InitDriver(0)) == -1)
	{
	    DEBUG_PRINT("Unable to Initialize System. Check System Settings");
	    return 0;
	}
	// check if link is up  or not.
	else
	{
	  	adi_dev_Control(lan_handle,ADI_ETHER_CMD_GET_PHY_REGS,phyregs);
		if (!((phyregs[1] & 0x4)==0x4))
		{
		    // User might have pulled out the plug orsomething happend
		    // we can not proceed.
			DEBUG_PRINT("System Settings Changed, unable to proceed.");
			return 0;
		}
	}

	InitRealTimeClock(30);

#ifdef CHECK_FRAMES
#ifdef SENDING
	for (FrameSize=1;FrameSize<=1496; FrameSize++)
	{
		FrameRcvd = false;
		FrameXmit = false;
		while (IssueWrite(lan_handle)==0);
		while (!FrameXmit);	// wait for the frame to hav egone
#ifndef BOARD_TO_BOARD
		SizeOK[FrameSize] = true;
		while (!FrameRcvd)
		{
			adi_dev_Control(lan_handle,ADI_ETHER_CMD_GET_STATISTICS,&cts);
			if (cts.cEMAC_RX_CNT_IRL>last_irl)
			{
				last_irl = cts.cEMAC_RX_CNT_IRL;
				SizeOK[FrameSize] = false;
				break;
			}
			if (cts.cEMAC_RX_CNT_ALLF>=cts.cEMAC_TX_CNT_OK) break;
		}
#endif
	}
	for (FrameSize=1;FrameSize<=1496; FrameSize++)
	{
		if (!SizeOK[FrameSize])
		{
			//printf("Size %d failed\n",FrameSize);
		}
	}
#else
	// receiving board to board
	adi_dev_Control(lan_handle,ADI_ETHER_CMD_GET_STATISTICS,&cts);
	while (cts.cEMAC_RX_CNT_ALLF < 1496)
	{
		adi_dev_Control(lan_handle,ADI_ETHER_CMD_GET_STATISTICS,&cts);
	}
	for (FrameSize=1;FrameSize<=1496; FrameSize++)
	{
		if (!SizeOK[FrameSize])
		{
			//printf("Size %d failed\n",FrameSize);
		}
	}
#endif
	{
		unsigned long  int *c = (unsigned  long int *)&cts;

		adi_dev_Control(lan_handle,ADI_ETHER_CMD_GET_STATISTICS,&cts);
		for (i=0; i<sizeof(cts)/sizeof(long long int);i++)
		{
			printf("0x%8.8x%8.8x  %s\n",(unsigned int)c[2*i+1],c[2*i],CtDesc[i] );
		}
	}
#else
	// provide the driver with some frames to transmit
	IssueWrite(lan_handle);

	rtcsecs= GetRtcSecs();
	while ((NoRcvd<NO_FRAMES) || (rtcsecs<30))
	{
		clock = *pRTC_STAT;
		min = (clock>>6)&0x3f;
		StartMin = (clock>>6)&0x3ff;
		if (NoRcvd>=NO_FRAMES) break;
		if (min != CurMinute)
		{
			// update the stats
			int ndx = CurMinute % 10;

			CurMinute = min;
			AvgRxStat[ndx].nbytes = CurRxStat.nbytes/60;
			AvgRxStat[ndx].nframes = CurRxStat.nframes/60;

			AvgTxStat[ndx].nbytes = CurTxStat.nbytes/60;
			AvgTxStat[ndx].nframes = CurTxStat.nframes/60;

			CurMinute = min;

			if (DumpStats)
			{
				int i;
				printf("     Rx kfr/sec  Rx kbytes/sec     Tx kfr/sec  Tx kbytes/sec\n");
				for (i=0;i<10;i++)
				{
					printf("%15u%15u%15u%15u\n",
					AvgRxStat[i].nframes/1024,AvgRxStat[i].nbytes/1024,
					AvgTxStat[i].nframes/1024,AvgTxStat[i].nbytes/1024);
				}
				printf("\n\n");
				DumpStats = 0;

			}


		}
		IssueRead(lan_handle);
		adi_dev_Control(lan_handle,ADI_ETHER_CMD_GET_STATISTICS,&cts);
		adi_dev_Control(lan_handle,ADI_ETHER_CMD_GET_PHY_REGS,phyregs);

		rtcsecs= GetRtcSecs();

	}
	rtcsecs= GetRtcSecs();
	DumpStats = 0;

	// check on success
	if ((NoXmit<NO_FRAMES/2) || (NoRcvd<NO_FRAMES/2))
	{
		DEBUG_PRINT("FAIL");
		ret = false;
	}
	else
	{
		unsigned long  int *c = (unsigned  long int *)&cts;

		//printf("OK  Sent=%u (0x%4.4x), Rcvd=%u (0x%4.4x)\n",NoXmit,NoXmit,NoRcvd,NoRcvd);
		DEBUG_PRINT("PASS");
		adi_dev_Control(lan_handle,ADI_ETHER_CMD_GET_STATISTICS,&cts);
		ret = true;
	}
#endif
	
	adi_dev_Close(lan_handle);
	
	return ret;
}
