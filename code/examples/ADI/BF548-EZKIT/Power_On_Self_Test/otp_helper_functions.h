#include <drivers/adi_dev.h>		// device manager includes
#include "adi_otp.h"	// device driver specific includes

#include "adi_ssl_Init.h"

// define error checking macro to ease readability
#define OTP_ERROR_CATCH(x,y)                 \
           {                                 \
              u32 ret_val;               \
                                             \
              ret_val = (x);                 \
              if (ret_val != ADI_DEV_RESULT_SUCCESS)    \
              {                              \
                return ret_val;              \
              }                              \
           }

// define default page addresses
#define OTP_UNIQUE_CHIP_ID_PAGE				0x004
#define OTP_UNIQUE_CHIP_ID_ECC_FIELD_PAGE	0x0E0
#define OTP_ADI_PUBLIC_KEY_PAGE				0x00D
#define OTP_ADI_PUBLIC_KEY_ECC_FIELD_PAGE	0x0E1
#define OTP_PUBLIC_KEY_PAGE					0x010
#define OTP_PUBLIC_KEY_ECC_FIELD_PAGE		0x0E2

// size defines
#define OTP_UNIQUE_CHIP_ID_SIZE_IN_BYTES		16
#define OTP_UNIQUE_CHIP_ID_SIZE_IN_HALFPAGES	((OTP_UNIQUE_CHIP_ID_SIZE_IN_BYTES+7)/8)
#define OTP_UNIQUE_CHIP_ID_SIZE_IN_PAGES		((OTP_UNIQUE_CHIP_ID_SIZE_IN_HALFPAGES+1)/2)
#define OTP_PUBLIC_KEY_SIZE_IN_BYTES			48
#define OTP_PUBLIC_KEY_SIZE_IN_HALFPAGES		((OTP_PUBLIC_KEY_SIZE_IN_BYTES+7)/8)
#define OTP_PUBLIC_KEY_SIZE_IN_PAGES			((OTP_PUBLIC_KEY_SIZE_IN_HALFPAGES+1)/2)

// Define starting address of ECC field
#define PUBLIC_OTP_START_PAGE				0x000
#define PRIVATE_OTP_START_PAGE				0x100
#define PUBLIC_OTP_ECC_FIELD_START_PAGE		0x0E0
#define PRIVATE_OTP_ECC_FIELD_START_PAGE	0x1E0

// define container for Unique Chip ID
typedef u8  tOTP_unique_chip_id_bytes [OTP_UNIQUE_CHIP_ID_SIZE_IN_BYTES];
typedef u64 tOTP_unique_chip_id_halfpages [OTP_UNIQUE_CHIP_ID_SIZE_IN_HALFPAGES];
typedef union 
{
   tOTP_unique_chip_id_bytes		bytes;
   tOTP_unique_chip_id_halfpages	halfpages;
} tOTP_unique_chip_id;



// define container for Customer Public Key
typedef u8  tOTP_public_key_bytes [OTP_PUBLIC_KEY_SIZE_IN_BYTES];
typedef u64 tOTP_public_key_halfpages [OTP_PUBLIC_KEY_SIZE_IN_HALFPAGES];
typedef union 
{
   tOTP_public_key_bytes		bytes;
   tOTP_public_key_halfpages	halfpages;
} tOTP_public_key;

// Routine that writes the Public Key to OTP pages 0x010 to 0x012
// Pre-requisite: OTP must be open for write (otp_open (OTP_ACCESS_READWRITE))
// Warning: This routine does not lock the Public Key
//          For security reasons, please lock the Public Key by using OTP_lock_PublicKey() and OTP_lock_ecc_PublicKey()
//          Otherwise, the Public Key could be modified
u32 otp_write_PublicKey (tOTP_public_key* pPublicKey);
	
// Routine that locks the Public Key at OTP pages 0x010 to 0x012
// Pre-requisite: OTP must be open for write (otp_open (OTP_ACCESS_READWRITE))
u32 otp_lock_PublicKey (void);

/***************************************************************************************
* public C API
***************************************************************************************/

u32  otp_islocked (u32 page, u32 *result);
u32  otp_isempty (u32 page, u32 flags, u32 *result);
u32  otp_isprogrammed (u32 page, u32 flags, u32 *result);
u32  otp_lock( u32 page );
u32  otp_read_page (u32 page, u32 flags, u64 *page_content);
u32  otp_write_page (u32 page, u32 flags, u64 *page_content);

/***************************************************************************************
* the API for General Purpose access to OTP Memory
***************************************************************************************/

// Routine to read a few halfpages of OTP memory
// Arguments: "page" and "halfpage" specify the first halfpage to be read
//            "data" is a pointer to the memory where the result will be stored
//            "size" is in halfpages
u32 otp_gp_read (u32 page, u32 halfpage, u64* data, u32 size);

// Routine to write a few halfpages to OTP memory
// Arguments: "page" and "halfpage" specify the first halfpage to be written
//            "data" is a pointer to the content that will be written
//            "size" is in halfpages
u32 otp_gp_write (u32 page, u32 halfpage, u64* data, u32 size);

// Routine to lock a few pages of OTP memory
// Warning: It does not lock the corresponding ECC fields
// Arguments: "page" is the first page to be locked
//            "n_pages" is the number of full OTP pages (128bits) to lock
u32 otp_gp_lock (u32 page, u32 n_pages);

// Routine to lock the ECC fields of a few pages of OTP memory
// Warning: Locking the ECC group for a page automatically locks the ECC field of 8 pages.
//          Each group of 8 pages that is locked is 8 page-aligned
// Arguments: "page" is the first page to be locked
//            "n_pages" is the number of full OTP pages (128bits) to lock
u32 otp_gp_lock_ecc (u32 page, u32 n_pages);

// Routine to check if the content of a few pages of OTP memory is 0
// Warning: It does not check if the pages are locked
// Arguments: "page" and "halfpage" specify the first halfpage to be checked
//            "size" is in halfpages
//            "pResult" is a pointer where the result will be stored (TRUE/FALSE)
u32 otp_gp_isempty (u32 page, u32 halfpage, u32 size, u32* pResult);

// Routine to check if a few pages of OTP memory are all locked
// Warning: It does not check if the corresponding ECC fields has been locked
// Arguments: "page" specifies the first halfpage to be checked
//            "n_pages" is the number of pages to be checked
//            "pResult" is a pointer where the result will be stored (TRUE/FALSE)
u32 otp_gp_islocked (u32 page, u32 n_pages, u32* pResult);

// Routine to check if the ECC fields of a few pages of OTP memory are all locked
// Arguments: "page" specifies the first halfpage to be checked
//            "n_pages" is the number of pages to be checked
//            "pResult" is a pointer where the result will be stored (TRUE/FALSE)
u32 otp_gp_islocked_ecc (u32 page, u32 n_pages, u32* pResult);

// Routine to check if a few pages of OTP memory are all unlocked
// Warning: It does not check if the corresponding ECC fields are all unlocked
// Arguments: "page" specifies the first halfpage to be checked
//            "n_pages" is the number of pages to be checked
//            "pResult" is a pointer where the result will be stored (TRUE/FALSE)
u32 otp_gp_isunlocked (u32 page, u32 n_pages, u32* pResult);

// Routine to check if the ECC fields of a few pages of OTP memory are all unlocked
// Arguments: "page" specifies the first halfpage to be checked
//            "n_pages" is the number of pages to be checked
//            "pResult" is a pointer where the result will be stored (TRUE/FALSE)
u32 otp_gp_isunlocked_ecc (u32 page, u32 n_pages, u32* pResult);
