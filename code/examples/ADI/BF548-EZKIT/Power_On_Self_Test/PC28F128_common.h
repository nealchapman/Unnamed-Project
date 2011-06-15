#ifndef _PC28F128_COMMON_H_
#define _PC28F128_COMMON_H_

/*******************************************************************
*  global variables and defines
*******************************************************************/
#define FLASH_SECTOR				258
#define FLASH_SECTOR_OFFSET			0xFFC000
#define FLASH_START_ADDR			0x20000000

extern ADI_DEV_DEVICE_HANDLE DevHandlePC28F128K;

extern int 			AFP_ManCode;			// 0x20 	= Intel
extern int 			AFP_DevCode;			// 0x22CB 	= PC28F128K

/*******************************************************************
*  function prototypes
*******************************************************************/
bool SetupForFlash(void);
unsigned int GetFlashInfo(void);
void pc28f128_Callback(void *ClientHandle, u32 Event, void *pArg);

#endif //#ifndef _PC28F128_COMMON_H_
