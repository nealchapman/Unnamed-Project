/*********************************************************************************

Copyright(c) 2005 Analog Devices, Inc. All Rights Reserved. 

This software is proprietary and confidential.  By using this software you agree
to the terms of the associated Analog Devices License Agreement.  
			
*********************************************************************************/


/*********************************************************************

Include files

*********************************************************************/

#include <services/services.h>		// system service includes
#include <drivers/adi_dev.h>        // device driver includes

/*********************************************************************

Definitions/Sizings

The user should modify the values in parenthesis as needed by their
application.

*********************************************************************/

#define ADI_SSL_INT_NUM_SECONDARY_HANDLERS  (6) // number of secondary interrupt handlers
#define ADI_SSL_DCB_NUM_SERVERS             (1) // number of DCB servers
#define ADI_SSL_DMA_NUM_CHANNELS            (2) // number of DMA channels
#define ADI_SSL_FLAG_NUM_CALLBACKS          (2) // number of flag callbacks
#define ADI_SSL_SEM_NUM_SEMAPHORES          (3) // number of semaphores

#define ADI_SSL_DEV_NUM_DEVICES             (4) // number of device drivers



/*********************************************************************

Handles to Services

Declare the handles to the various services and device manager.  These
handles can be used after the adi_ssl_Init() call.  

DO NOT MODIFY THIS SECTION

*********************************************************************/

extern ADI_DMA_MANAGER_HANDLE adi_dma_ManagerHandle;   // handle to the DMA manager
extern ADI_DEV_MANAGER_HANDLE adi_dev_ManagerHandle;   // handle to the device manager


/*********************************************************************

Critical Region Parameter

This is the value that is pased to the adi_int_EnterCriticalRegion()
function call.  This value is NULL for all current implementations.

DO NOT MODIFY THIS SECTION

*********************************************************************/

#define ADI_SSL_ENTER_CRITICAL (NULL)




/*********************************************************************

Function Declarations

Declare the intialization and termination functions.

DO NOT MODIFY THIS SECTION

*********************************************************************/

u32 adi_ssl_Init(void);
u32 adi_ssl_Terminate(void);

