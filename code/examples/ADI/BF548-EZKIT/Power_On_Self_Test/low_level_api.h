/******************************************************
* prototypes for lowest level OTP interface functions *
******************************************************/

// select target platform (only one allowed)
#define __MOAB__
//#define __KOOK__

// select target memory space (ROM, RAM)
// #define __BUILD_for_ROM__


#if !defined(_LANGUAGE_ASM)

//#include "..\ADI_typedefs.h"


/*******************************************
* OTP generic initialisation and management routine
*******************************************/
u32  otp_command(u32 command, u32 value);

/***************************************************************************************
* these two functions should cover all read and write cases
* the parameters are explained below
***************************************************************************************/

u32  otp_read (u32 page, u32 flags, u64 *page_content);
u32  otp_write(u32 page, u32 flags, u64 *page_content);

#endif //#if !defined(_LANGUAGE_ASM)




/*******************************************
* command parameter 
*******************************************/
#define	OTP_INIT			(0x00000001)
#define	OTP_CLOSE			(0x00000002)

// NOTE: value is a generic value for each command case
// could be a value or a pointer or a pointer to a structure


/*******************************************
* page parameter 
*******************************************/
// page contains value between 0x0 and 0x1ff

/*******************************************
* flag parameter fields for OTP_write()
************************************/
#define OTP_LOWER_HALF      (0x00000000)	// select upper/lower 64-bit half (bit 0)
#define OTP_UPPER_HALF      (0x00000001)
#define OTP_NO_ECC 			(0x00000010)	// do not use ECC (bit 4)
#define OTP_LOCK 			(0x00000020)	// sets page protection bit for page (bit 5)
/*******************************************
* flag parameter fields for OTP_read()
*******************************************/
// only OTP_LOWER_HALF, OTP_UPPER_HALF
// and OTP_NO_ECC are allowed

/*******************************************
* page parameter for OTP_write()
*******************************************/
// page_content is 64bits of data to
// be witten to half page

/*******************************************
* page parameter for OTP_read()
*******************************************/
// page_content is ECC corrected content
// of read half page if OTP_NO_ECC is cleared
// page_content is raw content of read 
// half page if OTP_NO_ECC is set


/*******************************************
* return code contains the following
* error and warning codes (bits)
*******************************************/
#define NO_ERROR			(0x0000)
#define OTP_SUCCESS			(0x0000)
#define OTP_MASTER_ERROR 	(0)
#define OTP_WRITE_ERROR 	(1)
#define OTP_READ_ERROR 		(2)
#define OTP_ACC_VIO_ERROR 	(3)
#define OTP_DATA_MULT_ERROR	(4)
#define OTP_ECC_MULT_ERROR	(5)
#define OTP_PREV_WR_ERROR	(6)
#define OTP_DATA_SB_WARN	(8)
#define OTP_ECC_SB_WARN		(9)
