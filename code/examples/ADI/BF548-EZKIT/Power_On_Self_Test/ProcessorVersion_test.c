/*******************************************************************
Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software
you agree to the terms of the associated Analog Devices License Agreement.  

Project Name:  	Power_On_Self_Test

Hardware:		ADSP-BF548 EZ-KIT Lite

Description:	This file checks the silicon rev
*******************************************************************/

/*******************************************************************
*  include files
*******************************************************************/
#include <cdef_LPBlackfin.h>
#include <ccblkfn.h>

/*******************************************************************
*  function prototypes
*******************************************************************/
int TEST_VERSION(void);

/*******************************************************************
*   Function:    TEST_VERSION
*   Description: This function compares the version of the processor 
* 				 being run on with the version that the software was
*				 built against
*******************************************************************/
int TEST_VERSION(void)
{
	int j = *pDSPID;
	int k = __SILICON_REVISION__;

	if( __SILICON_REVISION__ != ((*pDSPID) & 0xFF) )
	{
		return 0; // failed
	}

	return 1; // passed
}
