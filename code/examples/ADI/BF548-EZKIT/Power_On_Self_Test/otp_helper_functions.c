/*******************************************************************
Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software
you agree to the terms of the associated Analog Devices License Agreement.  

Project Name:  	Power_On_Self_Test

Hardware:		ADSP-BF548 EZ-KIT Lite

Description:	This file includes helper functions for OTP
*******************************************************************/

/*******************************************************************
*  include files
*******************************************************************/
#include "otp_helper_functions.h"


extern ADI_DEV_DEVICE_HANDLE DevHandleOTP;


/*******************************************************************
*   Function:    otp_islocked
*   Description: checks if the OTP page is locked
*******************************************************************/
u32  otp_islocked (u32 page, u32 *result)
{
    u32 RetVal;
    u64 TempData;

    if (page > 0x1FF)
    {
	    RetVal = ADI_OTP_MASTER_ERROR || ADI_OTP_ACC_VIO_ERROR;
    }
    else
    {
		RetVal = otp_read_page( (page>>7) & 0x3 , ADI_OTP_NO_ECC | ((page>>6) & 0x1), &TempData );

		*result = ( TempData >> (page & 0x3F) ) & 0x1;
    }

    return RetVal;
}

/*******************************************************************
*   Function:    otp_isempty
*   Description: checks if the OTP page is empty( not programmed )
*******************************************************************/
u32  otp_isempty (u32 page, u32 flags, u32 *result)
{
    u64 temp_page_content;
    u32 RetVal;

	RetVal  = otp_read_page(page, flags, &temp_page_content);
	*result = ( temp_page_content == 0);

	return RetVal;
}

/*******************************************************************
*   Function:    otp_isprogrammed
*   Description: checks if the OTP page has been programmed
*******************************************************************/
u32  otp_isprogrammed (u32 page, u32 flags, u32 *result)
{
    u64 temp_page_content;
    u32 RetVal;

    RetVal  = otp_read_page(page, flags, &temp_page_content);
	*result = ( temp_page_content != 0);

	return RetVal;
}

/*******************************************************************
*   Function:    otp_read_page
*   Description: reads 1 OTP page
*******************************************************************/
u32  otp_read_page (u32 page, u32 flags, u64 *page_content)
{
	u32 Result = ADI_DEV_RESULT_SUCCESS;
    ADI_OTP_INFO OTP_Params;
    ADI_DEV_1D_BUFFER OTPBuff1D;		// write buffer pointer

    OTP_Params.page = page;
	OTP_Params.flags = flags;

	OTPBuff1D.pAdditionalInfo = (void *)&OTP_Params;
	OTPBuff1D.Data = page_content;
	OTPBuff1D.pNext = NULL;

	Result = adi_dev_Read(DevHandleOTP, ADI_DEV_1D, (ADI_DEV_BUFFER *)&OTPBuff1D);

	return Result;
}

/*******************************************************************
*   Function:    otp_write_page
*   Description: writes 1 OTP page
*******************************************************************/
u32  otp_write_page (u32 page, u32 flags, u64 *page_content)
{

	u32 Result = ADI_DEV_RESULT_SUCCESS;
    ADI_OTP_INFO OTP_Params;
    ADI_DEV_1D_BUFFER OTPBuff1D;		// write buffer pointer

    OTP_Params.page = page;
	OTP_Params.flags = flags;

	OTPBuff1D.pAdditionalInfo = (void *)&OTP_Params;
	OTPBuff1D.Data = page_content;
	OTPBuff1D.pNext = NULL;

	Result = adi_dev_Write(DevHandleOTP, ADI_DEV_1D, (ADI_DEV_BUFFER *)&OTPBuff1D);

	return Result;
}

/*******************************************************************
*   Function:    otp_lock
*   Description: locks an OTP page
*******************************************************************/
u32 otp_lock( u32 page )
{
	u32 Result = ADI_DEV_RESULT_SUCCESS;

	Result = otp_write_page(page, ADI_OTP_LOCK, 0);

	return Result;
}

/*******************************************************************
*   Function:    otp_gp_read
*   Description: Routine to read a few halfpages of OTP memory
*	Arguments:	 page/halfpage - specifies the first halfpage to be read
*				 data - pointer to the memory where the result will be stored
*				 size - size is in halfpages
*******************************************************************/
u32 otp_gp_read (u32 page, u32 halfpage, u64* data, u32 size)
{
    int i;

    for (i=0; i<size; i++)
    {
		OTP_ERROR_CATCH( otp_read_page (page+(halfpage+i)/2, (halfpage+i)%2, &data[i]) , "otp_read_page" );
    }

	return ADI_DEV_RESULT_SUCCESS;
}

/*******************************************************************
*   Function:    otp_gp_write
*   Description: Routine to write a few halfpages to OTP memory
*	Arguments:	 page/halfpage - specifies the first halfpage to be read
*				 data - pointer to the content that will be written
*				 size - size is in halfpages
*******************************************************************/
u32 otp_gp_write (u32 page, u32 halfpage, u64* data, u32 size)
{
    int i;

    for (i=0; i<size; i++)
    {
		OTP_ERROR_CATCH( otp_write_page (page+(halfpage+i)/2, (halfpage+i)%2, &data[i]) , "otp_write_page" );
    }

	return ADI_DEV_RESULT_SUCCESS;
}

/*******************************************************************
*   Function:    otp_gp_lock
*   Description: Routine to lock a few pages of OTP memory
*	Arguments:	 page - first page to be locked
*				 n_pages - is the number of full OTP pages (128bits) to lock
*******************************************************************/
u32 otp_gp_lock (u32 page, u32 n_pages)
{
    int i;
    u32 is_unlocked;

    // Lock the pages
    for (i=0; i<n_pages; i++)
    {
      OTP_ERROR_CATCH( otp_gp_isunlocked ( page+i, 1, &is_unlocked) , "otp_gp_isunlocked" );
      if( 1==is_unlocked )
      {
        OTP_ERROR_CATCH( otp_lock (page+i) , "otp_lock" );
      }
    }

    return ADI_DEV_RESULT_SUCCESS;
}

/*******************************************************************
*   Function:    otp_gp_lock_ecc
*   Description: Routine to lock the ECC fields of a few pages of OTP memory
*	Arguments:	 page - first page to be locked
*				 n_pages - is the number of full OTP pages (128bits) to lock
*
*	Warning: 	 Locking the ECC group for a page automatically locks the 
*				 ECC field of 8 pages. Each group of 8 pages that is locked 
* 				 is 8 page-aligned
*******************************************************************/
u32 otp_gp_lock_ecc (u32 page, u32 n_pages)
{
	u32 ECC_page;
	int i;

    // Lock the page where the ECC code is stored
    for (i=0; i<(n_pages+7)/8; i++)
    {
	    if (page + 8*i < PRIVATE_OTP_START_PAGE)
    	{
    		ECC_page = PUBLIC_OTP_ECC_FIELD_START_PAGE + page/8 + i;
    	}
    	else
    	{
    		ECC_page = PRIVATE_OTP_ECC_FIELD_START_PAGE + page/8 + i;
    	}
		OTP_ERROR_CATCH( otp_lock (ECC_page) , "otp_lock" );
    }

	return ADI_DEV_RESULT_SUCCESS;
}

/*******************************************************************
*   Function:    otp_gp_isempty
*   Description: Routine to check if the content of a few pages of OTP memory is 0
*	Arguments:	 page/halfpage - specifies the first halfpage to be checked
*				 size - size is in halfpages
*				 pResult - pointer where the result will be stored (TRUE/FALSE)
*******************************************************************/
u32 otp_gp_isempty (u32 page, u32 halfpage, u32 size, u32* pResult)
{
    u64 halfpage_data;
    int i;

	*pResult = FALSE;		// In case of error, return false (not empty) (ex: ECC error/warning, etc)
    for (i=0; i<size; i++)
    {
		OTP_ERROR_CATCH( otp_read_page (page+(halfpage+i)/2, (halfpage+i)%2, &halfpage_data) , "otp_read_page" );
		if (halfpage_data != 0)
		{
			*pResult = FALSE;
			return ADI_DEV_RESULT_SUCCESS;
		}
    }

    *pResult = TRUE;
	return ADI_DEV_RESULT_SUCCESS;
}

/*******************************************************************
*   Function:    otp_gp_islocked
*   Description: Routine to check if a few pages of OTP memory are all locked
*	Arguments:	 page - specifies the first halfpage to be checked
*				 n_pages - the number of pages to be checked
*				 pResult - pointer where the result will be stored (TRUE/FALSE)
*
*	Warning: 	 It does not check if the corresponding ECC fields has been locked
*******************************************************************/
u32 otp_gp_islocked (u32 page, u32 n_pages, u32* pResult)
{
	u32 islocked;
	int i;

	*pResult = FALSE;
    for (i=0; i<n_pages; i++)
    {
    	OTP_ERROR_CATCH( otp_islocked (page+i, & islocked) , "otp_islocked" );
    	if (!islocked)
    	{
    		*pResult = FALSE;
    		return ADI_DEV_RESULT_SUCCESS;
    	}
    }

	*pResult = TRUE;
   	return ADI_DEV_RESULT_SUCCESS;
}

/*******************************************************************
*   Function:    otp_gp_islocked_ecc
*   Description: Routine to check if the ECC fields of a few pages of OTP 
*				 memory are all locked
*	Arguments:	 page - specifies the first halfpage to be checked
*				 n_pages - the number of pages to be checked
*				 pResult - pointer where the result will be stored (TRUE/FALSE)
*******************************************************************/
u32 otp_gp_islocked_ecc (u32 page, u32 n_pages, u32* pResult)
{
	u32 islocked_ecc;
	u32 ECC_page;
	int i;

	*pResult = FALSE;
    // Check the lock bit of the pages where the ECC code is stored
    for (i=0; i<(n_pages+7)/8; i++)
    {
	    if (page + 8*i < PRIVATE_OTP_START_PAGE)
    	{
    		ECC_page = PUBLIC_OTP_ECC_FIELD_START_PAGE + page/8 + i;
    	}
    	else
    	{
    		ECC_page = PRIVATE_OTP_ECC_FIELD_START_PAGE + page/8 + i;
    	}
		OTP_ERROR_CATCH( otp_islocked (ECC_page, &islocked_ecc) , "otp_islocked" );
		if (!islocked_ecc)
		{
			*pResult = FALSE;
			return ADI_DEV_RESULT_SUCCESS;
    	}
    }

	*pResult = TRUE;
	return ADI_DEV_RESULT_SUCCESS;
}

/*******************************************************************
*   Function:    otp_gp_isunlocked
*   Description: Routine to check if a few pages of OTP memory are all unlocked
*	Arguments:	 page - specifies the first halfpage to be checked
*				 n_pages - the number of pages to be checked
*				 pResult - pointer where the result will be stored (TRUE/FALSE)
*
**	Warning: 	 It does not check if the corresponding ECC fields has been unlocked
*******************************************************************/
u32 otp_gp_isunlocked (u32 page, u32 n_pages, u32* pResult)
{
	u32 islocked;
	int i;

	*pResult = FALSE;
    for (i=0; i<n_pages; i++)
    {
    	OTP_ERROR_CATCH( otp_islocked (page+i, &islocked) , "otp_islocked" );
    	if (!islocked)
    	{
    		*pResult = TRUE;
    		return ADI_DEV_RESULT_SUCCESS;
    	}
    }

	*pResult = FALSE;
   	return ADI_DEV_RESULT_SUCCESS;
}

/*******************************************************************
*   Function:    otp_gp_isunlocked_ecc
*   Description: Routine to check if the ECC fields of a few pages of OTP 
*				 memory are all unlocked
*	Arguments:	 page - specifies the first halfpage to be checked
*				 n_pages - the number of pages to be checked
*				 pResult - pointer where the result will be stored (TRUE/FALSE)
*******************************************************************/
u32 otp_gp_isunlocked_ecc (u32 page, u32 n_pages, u32* pResult)
{
	u32 islocked_ecc;
	u32 ECC_page;
	int i;

	*pResult = FALSE;
    // Check the lock bit of the pages where the ECC code is stored
    for (i=0; i<(n_pages+7)/8; i++)
    {
	    if (page + 8*i < PRIVATE_OTP_START_PAGE)
    	{
    		ECC_page = PUBLIC_OTP_ECC_FIELD_START_PAGE + page/8 + i;
    	}
    	else
    	{
    		ECC_page = PRIVATE_OTP_ECC_FIELD_START_PAGE + page/8 + i;
    	}
		OTP_ERROR_CATCH( otp_islocked (ECC_page, &islocked_ecc) , "otp_islocked" );
		if (islocked_ecc)
		{
			*pResult = FALSE;
			return ADI_DEV_RESULT_SUCCESS;
    	}
    }

	*pResult = TRUE;
	return ADI_DEV_RESULT_SUCCESS;
}

/*******************************************************************
*   Function:    	otp_write_PublicKey
*   Description: 	Routine that writes the Public Key to OTP pages 0x010 to 0x012
*	Pre-requisite:  OTP must be open for write (otp_open (OTP_ACCESS_READWRITE))
*
*	Warning:	 	This routine does not lock the Public Key
*				 	For security reasons, please lock the Public Key by using
*				 	OTP_lock_PublicKey() and OTP_lock_ecc_PublicKey()
*				 	Otherwise, the Public Key could be modified
*******************************************************************/
u32 otp_write_PublicKey (tOTP_public_key* pPublicKey)
{
    int i;
    u32 is_programmed = 0;
    
    for (i=0; i<OTP_PUBLIC_KEY_SIZE_IN_PAGES; i++)
    {
      OTP_ERROR_CATCH( otp_isprogrammed ((OTP_PUBLIC_KEY_PAGE+i), (ADI_OTP_LOWER_HALF), &is_programmed) , "otp_is_programmed");
      if(0 == is_programmed)
        OTP_ERROR_CATCH( otp_write_page ((OTP_PUBLIC_KEY_PAGE+i), (ADI_OTP_LOWER_HALF), &(pPublicKey->halfpages)[2*i]) , "otp_write_page" );
  		
  		OTP_ERROR_CATCH( otp_isprogrammed ((OTP_PUBLIC_KEY_PAGE+i), (ADI_OTP_UPPER_HALF), &is_programmed) , "otp_is_programmed");
  		if(0 == is_programmed)
  		  OTP_ERROR_CATCH( otp_write_page ((OTP_PUBLIC_KEY_PAGE+i), (ADI_OTP_UPPER_HALF), &(pPublicKey->halfpages)[2*i+1]) , "otp_write_page" );
    }
    
	return ADI_DEV_RESULT_SUCCESS;
}

/*******************************************************************
*   Function:    	otp_lock_PublicKey
*   Description: 	Routine that locks the Public Key at OTP pages 0x010 to 0x012
*				 	memory are all unlocked
*	Pre-requisite: 	OTP must be open for write (otp_open (OTP_ACCESS_READWRITE))
*******************************************************************/
u32 otp_lock_PublicKey (void)
{
    int i;
    u32 is_unlocked = FALSE;
    u32 return_value;
    
    // Lock the pages
    for (i=0; i<OTP_PUBLIC_KEY_SIZE_IN_PAGES; i++)
    {
      OTP_ERROR_CATCH( otp_gp_isunlocked ( (OTP_PUBLIC_KEY_PAGE+i), 1, &is_unlocked) , "otp_gp_isunlocked" );
      if( 1==is_unlocked )
      {
        return_value = otp_lock (OTP_PUBLIC_KEY_PAGE+i);
        /* The otp_lock() routine returns an ECC error or warning.
           Since we don't use ECC for the protection bits, we may safely mask out those error returns */
        return_value &= (!(ADI_OTP_ECC_MULT_ERROR | ADI_OTP_ECC_SB_WARN));
        if ( return_value & (!ADI_OTP_MASTER_ERROR) )
          return return_value;
      }
    }
    
    return ADI_DEV_RESULT_SUCCESS;
}
