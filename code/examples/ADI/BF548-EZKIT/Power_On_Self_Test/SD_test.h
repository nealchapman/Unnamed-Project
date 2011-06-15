#ifndef _SD_TEST_H_
#define _SD_TEST_H_


//////////////////////////////////////////////////////////////////////////////
//
// COMMON DEFINES
//
//////////////////////////////////////////////////////////////////////////////

#define TIMEOUT_VAL	0xF00000
#define	SD_STUFF_BITS		0x00000000
#define SD_NO_RESPONSE 		0x00000000
#define SD_RCA_NOT_NEEDED 	0
#define SD_RCA_NEEDED 		1
#define SD_BLK_LEN_512		0x00000200


typedef struct _SD_CMD
{
	unsigned char CmdID;			// command ID
	unsigned long CmdArg;			// command argument
	unsigned char CmdResponse;		// command response
	bool UseRCA;					// 0 - does not need RCA, 1 - needs RCA
} SD_CMD;

// SD command IDs
typedef enum
{
	SD_CMD0,
	SD_CMD2,
	SD_CMD3,
	SD_CMD7,
	SD_CMD9,
	SD_CMD13,
	SD_CMD16,
	SD_CMD17,
	SD_CMD24,
	SD_CMD55,

	SD_ACMD6,
	SD_ACMD13,
	SD_ACMD41,
	SD_ACMD42
}enSDCmds;


bool g_bCardDetected = false;
int g_nISR_Count = 0;
int g_nSRC_Index = 0;
int g_nDEST_Index = 0;
int g_nBytesTXed = 0;
int g_nBytesRXed = 0;
unsigned long g_ulCardRCA = 0;


unsigned long *SD_RX_DEST_BUF = NULL;
unsigned long *SD_TX_SRC_BUF = NULL;

typedef struct _SD_CSD_DATA
{
 	unsigned char ucMaxSpeed;
 	unsigned char ucMaxReadBlkLen;
	unsigned short usCardSize;
	unsigned short usSectorSize;
	unsigned char ucMaxWriteBlkLen;
}SD_CSD_DATA;

SD_CSD_DATA CardData;

//////////////////////////////////////////////////////////////////////////////
//
// function prototypes
//
//////////////////////////////////////////////////////////////////////////////
void Init_SD(void);
int TEST_SD(void);
EX_INTERRUPT_HANDLER(SD_ISR);
EX_INTERRUPT_HANDLER(SD_DMA_ISR);
void Init_SD_Interrupts(void);
void Init_DMA(unsigned long *buf_addr, unsigned short config_value);
int SendCommand( SD_CMD *pSD_Cmd, bool bGetStatusReg );
int SD_reset( void );
int SD_ReadCardIdent(void);
int TX_Block(void);
int Set_4BitMode(void);
void Delay_Loop( int nDelay );

#endif //_SD_TEST_H_
