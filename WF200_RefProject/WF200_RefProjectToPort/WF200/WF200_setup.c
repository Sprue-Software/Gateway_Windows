/*
 * WF200_Setup.c
 *
 *  Created on: 11 Mar 2020
 *      Author: SMurva
 */


/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include "WF200_host_common.h"
#include "fsl_gpio.h"
#include "event_groups.h"
#include "lwip/netifapi.h"


/*******************************************************************************
 * DEFINITIONS
 ******************************************************************************/


/*******************************************************************************
 * VARIABLES
 ******************************************************************************/

// Select the comms method, this
// will be shared with LTE code
eCommsMethod CommsMethod = WiFi;

// Flag to indicate interrupt has
// been received
bool WF200_interrupt_event = false;

// Main WF200 Chip Driver Related Context
static sl_wfx_context_t WF200_Context;

// WF200 Host Control Info
static WF200_ControlInfo_t WF200_ControlInfo;


/*******************************************************************************
 * FUNCTION PROTOTYPES
 ******************************************************************************/

static void WF200_SetupGPIOs(void);
static void WF200_Init_Task(void *arg);


/*******************************************************************************
 * FUNCTION DEFINITIONS
 ******************************************************************************/

/*******************************************************************************
 * @brief       WF200_CommsSetup: Initialise control structure and set up the
 *              Event and Comms Tasks and Init GPIO's
 *
 * @param[in]   None
 *
 * @return      Err Status
 *
 ******************************************************************************/
sl_status_t WF200_CommsSetup(void)
{
    status_t WF200_host_status;
    sl_status_t WF200_DriverStatus = SL_STATUS_FAIL;

	// Setup the GPIO for
    // the WF200 pins
    WF200_SetupGPIOs();

    // Initialise the Control Info
    WF200_ControlInfo.Awake = false;
    WF200_ControlInfo.AP_ModeRequired = false;
    WF200_ControlInfo.STA_ModeRequired = false;
    WF200_ControlInfo.EventGroupHandle = NULL;
    WF200_ControlInfo.TestingModeRequired = false;
    WF200_ControlInfo.STA_CommandedDisconnect = true;
    WF200_ControlInfo.STA_ModeNetworkConnected = false;
    WF200_ControlInfo.AP_LinkStatus = WF200_AP_LINK_STATUS_DOWN;
    WF200_ControlInfo.STA_LinkStatus = WF200_STA_LINK_STATUS_DOWN;
    WF200_ControlInfo.AP_NetIF_Status = WF200_AP_NETIF_NOT_ADDED;
    WF200_ControlInfo.STA_NetIF_Status = WF200_STA_NETIF_NOT_ADDED;

    for(uint8_t numTasks=0; numTasks<WF200_NUM_TASKS; numTasks++)
    {
    	WF200_ControlInfo.WF200_TaskInfo[numTasks].TaskCreated = false;
    	WF200_ControlInfo.WF200_TaskInfo[numTasks].TaskRunning = false;
    	WF200_ControlInfo.WF200_TaskInfo[numTasks].TaskHandle = NULL;
    	WF200_ControlInfo.WF200_TaskInfo[numTasks].QueueHandle = NULL;
    }

    // To use Events within FreeRTOS, the interrupt
    // priority has to be set at the same or below
    // task priorities
    NVIC_SetPriority(GPIO1_Combined_16_31_IRQn, 7);

    // Enable the WF200 SPI Data Ready Interrupt
    WF200_host_status = EnableIRQ(GPIO1_Combined_16_31_IRQn);

    // Log any issues and trap
    if(WF200_host_status != kStatus_Success)
    {
    	// Log the issue (should go in event log)
	    PRINTF("Interrupt Setup Failure: %i\n", WF200_host_status);
	    WF200_DEBUG_TRAP;
    }

    // Carry out setup of the WF200, even if LTE is
    // going to be used, setting up the WF200 and
    // then putting it into Shutdown will, get us
    // into a known state and will show up if there
    // are any issues earlier than setting up later
    // on
    if(WF200_host_status == kStatus_Success)
    {
    	// When running, this task will carry out the
    	// WF200 FW download and init
    	WF200_DriverStatus = WF200_Create_WF200_InitTask();

    	if(WF200_DriverStatus == SL_STATUS_OK)
    	{
    		// Setup the Comms Task (this Task must be
    		// running for initialisation and FW download)
    		WF200_DriverStatus = WF200_CreateCommsTask();

    		// If all is good create the events task
    		if(WF200_DriverStatus == SL_STATUS_OK)
    		{
    			WF200_DriverStatus = WF200_CreateEventTask();
    		}
    	}
    }

    return WF200_DriverStatus;
}


/*******************************************************************************
 * @brief       WF200_Create_WF200_InitTask: Create the Init Task
 *
 * @param[in]   None
 *
 * @return      Err Status
 *
 ******************************************************************************/
sl_status_t WF200_Create_WF200_InitTask(void)
{
  	BaseType_t status;
   	sl_status_t WF200_DriverStatus = SL_STATUS_FAIL;

   	// Should not be trying to create a task that has already been created
   	if(WF200_ControlInfo.WF200_TaskInfo[WF200_INIT_TASK].TaskCreated == false)
   	{
   		status = xTaskCreate(WF200_Init_Task,
   		    	             "WF200_Init_Task",
   			    			 WF200_INIT_TASK_STACK_SIZE,
   			    			 NULL,
   			    			 WF200_INIT_TASK_PRIORITY,
   			    			 &(WF200_ControlInfo.WF200_TaskInfo[WF200_INIT_TASK].TaskHandle));

   	    if (status == pdPASS)
   	    {
   	    	WF200_DriverStatus = SL_STATUS_OK;
   		    WF200_ControlInfo.WF200_TaskInfo[WF200_INIT_TASK].TaskCreated = true;

   		    // Log status
   		    PRINTF("WF200_Create_WF200_InitTask: SUCCESS - Task Created\n");
   	    }
   	    else
   	    {
   		    // Log failure
   		    PRINTF("WF200_Create_WF200_InitTask: FAIL - Task Creation\n");
   		    WF200_DEBUG_TRAP;
   	    }
   	}
   	else
   	{
   		// Log failure
   		PRINTF("WF200_Create_WF200_InitTask: FAIL - Trying to create a task that is already created\n" );
   		WF200_DEBUG_TRAP;
   	}

   	return WF200_DriverStatus;
}


/*******************************************************************************
 * @brief       WF200_Init_Task: Main init task that carries out some
 *              initialisation and WF200 FW download
 *
 * @param[in]
 *
 * @return
 *    None
 ******************************************************************************/
static void WF200_Init_Task(void *arg)
{
	sl_status_t WF200_DriverStatus;

	PRINTF("WF200 INIT TASK RUNNING\r\n");
	WF200_ControlInfo.WF200_TaskInfo[WF200_INIT_TASK].TaskRunning = true;

	// Carry out the WF200 Init and FW
	// Download
	WF200_DriverStatus = WF200_Init();

	if(WF200_DriverStatus == SL_STATUS_OK)
	{
		// There are a few different paths the code can take from here...
		// 1) If WiFi is not being used and LTE is, the WF200 can be SHUTDOWN
		// 2) If Network details are to be input, AP Mode is required
		// 3) Normal STA operation could be required
		// 4) The WF200 Testing mode could be required

		WF200_ControlInfo.Awake = true;

		// Shutdown option
		if((CommsMethod != WiFi) && (WF200_ControlInfo.TestingModeRequired != true))
		{
			// SHUTDOWN the WF200 and the associated code
			WF200_DriverStatus = WF200_ShutDown();
		} // AP Mode Option
		else if((CommsMethod == WiFi) && (WF200_ControlInfo.AP_ModeRequired == true))
		{
			// Log status
			PRINTF("WF200_Init_Task - WF200 AP MODE SELECTED\r\n");
			WF200_DriverStatus = WF200_Setup_AP_Mode();

			if(WF200_DriverStatus != SL_STATUS_OK)
			{
				// Log status
				PRINTF("WF200_Init_Task: ERROR - setup AP MODE fail = %i\n", WF200_DriverStatus);
			}

		} // STA Mode Option
		else if((CommsMethod == WiFi) && (WF200_ControlInfo.STA_ModeRequired == true))
		{
			// Log status
			PRINTF("WF200_Init_Task - WF200 STA MODE SELECTED\r\n");

			WF200_DriverStatus = WF200_Setup_STA_Mode();

			if(WF200_DriverStatus != SL_STATUS_OK)
			{
				// Log status
				PRINTF("WF200_Init_Task: ERROR - setup STA MODE fail = %i\n", WF200_DriverStatus);
			}
		} // Testing Mode Option
		else if(WF200_ControlInfo.TestingModeRequired == true)
		{
			// Log status
			PRINTF("WF200_Init_Task - WF200 TESTING MODE SELECTED\r\n");

			if(WF200_ControlInfo.WF200_TaskInfo[WF200_TEST_MODE_TASK].TaskRunning == false)
			{
				WF200_DriverStatus = WF200_Setup_Testing_Mode();

			    if(WF200_DriverStatus != SL_STATUS_OK)
			    {
				   // Log status
				   PRINTF("WF200_Init_Task: ERROR - setup Testing MODE fail = %i\n", WF200_DriverStatus);
			   }
			}
		}
		else
		{
			// Log failure
			PRINTF("WF200_Init_Task: ERROR - Mode Selection Failure!\r\n");
			WF200_DEBUG_TRAP;
		}
	}
	else
	{
   		// Log failure (should go in event log)
   		PRINTF("WF200_Init_Task: ERROR: WF200 Initialisation fail = %i\n", WF200_DriverStatus);
   		WF200_DEBUG_TRAP;
	}

	PRINTF("WF200 INIT TASK BEING DELETED\r\n");

	// Task is no longer required
	WF200_DriverStatus = WF200_DeleteTask(WF200_INIT_TASK);
}


/*******************************************************************************
 * @brief       WF200_SetupGPIOs: Setsup the GPIO pins
 *
 * @param[in]   None
 *
 * @return      None
 *
 ******************************************************************************/
static void WF200_SetupGPIOs(void)
{
	gpio_pin_config_t WF200_WakeUpPin        = {kGPIO_DigitalOutput, 0, kGPIO_NoIntmode};
	gpio_pin_config_t WF200_ResetPin         = {kGPIO_DigitalOutput, 0, kGPIO_NoIntmode};
	gpio_pin_config_t WF200_ChipSelectPin    = {kGPIO_DigitalOutput, 0, kGPIO_NoIntmode};
	gpio_pin_config_t WF200_SPI_DataReadyInt = {kGPIO_DigitalInput,  0, kGPIO_IntRisingEdge};

	GPIO_PinInit(GPIO1, GPIO1_PIN_03_WF200_CHIP_SELECT_PIN,    &WF200_ChipSelectPin);
	GPIO_PinInit(GPIO1, GPIO1_PIN_28_WF200_SPI_DATA_READY_INT, &WF200_SPI_DataReadyInt);
    GPIO_PinInit(GPIO1, GPIO1_PIN_29_WF200_RESET_PIN,          &WF200_ResetPin);
    GPIO_PinInit(GPIO1, GPIO1_PIN_30_WF200_WAKEUP_PIN,         &WF200_WakeUpPin);
}


/*******************************************************************************
 * @brief       GPIO1_Combined_16_31_DriverIRQHandler: Handler for the data
 *              ready from the WF200
 *
 * @param[in]   None
 *
 * @return      None
 *
 ******************************************************************************/
void GPIO1_Combined_16_31_DriverIRQHandler(void)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* Clear the interrupt status */
    GPIO_PortClearInterruptFlags(GPIO1, 1U << GPIO1_PIN_28_WF200_SPI_DATA_READY_INT);
    WF200_interrupt_event = true;

	vTaskNotifyGiveFromISR(WF200_ControlInfo.WF200_TaskInfo[WF200_COMMS_TASK].TaskHandle,
			               &xHigherPriorityTaskWoken );
    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}


/*******************************************************************************
 * @brief       WF200_Init: Carries out initialisation of the WF200, including
 *              FW download
 *
 * @param[in]   None
 *
 * @return      Error Status
 *
 ******************************************************************************/
sl_status_t WF200_Init(void)
{
	sl_status_t status;

	// The following initialisation will download the WiFi
    // firmware and carry out some initialisation
	// Carry out some logging
    PRINTF("Wait for start (FW Download)....\r\n");
    status = sl_wfx_init(&WF200_Context);

    // Output some WiFi information based on download status
   PRINTF("FMAC Driver version    %s\r\n", FMAC_DRIVER_VERSION_STRING);
   switch(status)
   {
   case SL_STATUS_OK:
	  WF200_Context.state = SL_WFX_STARTED;
      PRINTF("WF200 Firmware version %d.%d.%d\r\n",
    		 WF200_Context.firmware_major,
			 WF200_Context.firmware_minor,
			 WF200_Context.firmware_build);
      PRINTF("WF200 initialization successful\r\n");
      break;
   case SL_STATUS_WIFI_INVALID_KEY:
      PRINTF("Failed to init WF200: Firmware keyset invalid\r\n");
      break;
   case SL_STATUS_WIFI_FIRMWARE_DOWNLOAD_TIMEOUT:
      PRINTF("Failed to init WF200: Firmware download timeout\r\n");
      break;
   case SL_STATUS_TIMEOUT:
      PRINTF("Failed to init WF200: Poll for value timeout\r\n");
      break;
   case SL_STATUS_FAIL:
      PRINTF("Failed to init WF200: Error\r\n");
      break;
   default :
      PRINTF("Failed to init WF200: Unknown error\r\n");
   }

   return status;
}


/***************************************************************************//**
 * @brief       WF200_ShutDown: Shut down WF200 and associated code
 *
 * @param[in]   None
 *
 * @return      Error Status
 *
 ******************************************************************************/
sl_status_t WF200_ShutDown(void)
{
	err_t err;
	uint8_t numTasks;
	struct netif *pSTA_netif;
	struct netif *pAP_netif;
	sl_status_t WF200_DriverStatus;

	WF200_DriverStatus = sl_wfx_deinit();

	if(WF200_DriverStatus == SL_STATUS_OK)
	{
		// Stop the WF200 related tasks that are already
	    // running
	    for(numTasks=0; numTasks<WF200_NUM_TASKS; numTasks++)
	    {
	    	// If in testing mode, don't delete the test task,
	    	// this is because this is a testing mode shutdown
	    	if((WF200_ControlInfo.TestingModeRequired == true) &&
	    	   (numTasks == WF200_TEST_MODE_TASK))
	    	{
	    		// Do nothing
	    	}
	    	else if(WF200_ControlInfo.WF200_TaskInfo[numTasks].TaskCreated == true)
		    {
			    WF200_DriverStatus = WF200_DeleteTask(numTasks);

			    if(WF200_DriverStatus != SL_STATUS_OK)
			    {
			    	// Log failure
			    	PRINTF("WF200_ShutDown: ERROR - Delete Task Fail = %i", WF200_DriverStatus);
			    	WF200_DEBUG_TRAP;
			    	break;
			    }
		    }

	    	// If in testing mode, don't delete the Testing queue
	    	if((WF200_ControlInfo.TestingModeRequired == true) &&
	    	   (numTasks == WF200_TEST_MODE_TASK))
	    	{
	    		// Do nothing
	    	}
	    	else if(WF200_ControlInfo.WF200_TaskInfo[numTasks].QueueHandle != NULL)
		    {
			    WF200_DeleteQueue(numTasks);
		    }
	    }

	    // Delete the Semaphore, if one has been created
	    if(WF200_ControlInfo.ReceiveFrameSemaphoreHandle != NULL)
	    {
		    vSemaphoreDelete(WF200_ControlInfo.ReceiveFrameSemaphoreHandle);
		    WF200_ControlInfo.ReceiveFrameSemaphoreHandle = NULL;
	    }

	    // Deleete the Event Group if one has been created
	    if(WF200_ControlInfo.EventGroupHandle != NULL)
	    {
		    vEventGroupDelete(WF200_ControlInfo.EventGroupHandle);
		    WF200_ControlInfo.EventGroupHandle = NULL;
	    }

	    // Update flags
	    WF200_ControlInfo.Awake = false;
	    WF200_ControlInfo.STA_ModeNetworkConnected = false;

	    // Handle link status related to AP mode
	    if(WF200_ControlInfo.AP_LinkStatus == WF200_AP_LINK_STATUS_UP)
	    {
	    	pAP_netif = WF200_Get_AP_NetIF();
	    	err = netifapi_netif_set_link_down(pAP_netif);

	    	if(err != ERR_OK)
	    	{
	    		// Log failure
	    		PRINTF("WF200_ShutDown: ERROR - netif set link down fail = %i", err);
	    		WF200_DriverStatus = SL_STATUS_FAIL;
	    		WF200_DEBUG_TRAP;
	    	}

	        err = netifapi_netif_set_down(pAP_netif);

	    	if(err != ERR_OK)
	    	{
	    		// Log failure
	    		PRINTF("WF200_ShutDown: ERROR: - netif set down fail = %i", err);
	    		WF200_DriverStatus = SL_STATUS_FAIL;
	    		WF200_DEBUG_TRAP;
	    	}

	    }

	    // Handle link status related to STA mode
	    if(WF200_ControlInfo.STA_LinkStatus == WF200_STA_LINK_STATUS_UP)
	    {
	    	pSTA_netif = WF200_Get_STA_NetIF();
	        err = netifapi_netif_set_link_down(pSTA_netif);

	    	if(err != ERR_OK)
	    	{
	    		// Log failure
	    		PRINTF("WF200_ShutDown: ERROR - netif set sta link down fail = %i", err);
	    		WF200_DriverStatus = SL_STATUS_FAIL;
	    		WF200_DEBUG_TRAP;
	    	}

	    	err = netifapi_netif_set_down(pSTA_netif);

	    	if(err != ERR_OK)
	    	{
	    		// Log failure
	    		PRINTF("WF200_ShutDown: ERROR - netif set down fail = %i", err);
	    		WF200_DriverStatus = SL_STATUS_FAIL;
	    		WF200_DEBUG_TRAP;
	    	}

	    }

	    // Need to remove the AP NetIF if it exists
	    if(WF200_ControlInfo.AP_NetIF_Status == WF200_AP_NETIF_ADDED)
	    {
    	    pAP_netif = WF200_Get_AP_NetIF();
    	    err = netifapi_netif_remove(pAP_netif);

    	    if(err != ERR_OK)
    	    {
    	    	// Log failure
    	    	PRINTF("WF200_ShutDown: ERROR - netif remove fail = %i\n", err);
    	    	WF200_DriverStatus = SL_STATUS_FAIL;
    		    WF200_DEBUG_TRAP;
    	    }
	    }

	    // Need to remove the STA NetIF if it exists
	    if(WF200_ControlInfo.STA_NetIF_Status == WF200_STA_NETIF_ADDED)
	    {
    	    pSTA_netif = WF200_Get_STA_NetIF();
    	    err = netifapi_netif_remove(pSTA_netif);

    	    if(err != ERR_OK)
    	    {
    	    	// Log failure
    	    	PRINTF("WF200_ShutDown: ERROR - netif remove fail = %i\n", err);
    	    	WF200_DriverStatus = SL_STATUS_FAIL;
    		    WF200_DEBUG_TRAP;
    	    }
	    }

	    // Log Status
	    PRINTF("WF200_ShutDown - WF200 BEEN SHUTDOWN\r\n");
	}

	return WF200_DriverStatus;
}


/***************************************************************************//**
 * @brief       WF200_DeleteTask: Deletes the requested Task
 *
 * @param[in]   Task ID
 *
 * @return      Error Status
 *
 ******************************************************************************/
sl_status_t WF200_DeleteTask(const WF200_TaskIds taskId)
{
	TaskHandle_t taskHandle;
	sl_status_t WF200_DriverStatus = SL_STATUS_OK;

	assert(taskId < WF200_NUM_TASKS);

	// Should not be deleting a Task that is already deleted
	if((WF200_ControlInfo.WF200_TaskInfo[taskId].TaskHandle != NULL) &&
	   (WF200_ControlInfo.WF200_TaskInfo[taskId].TaskCreated == true))
	{
		taskHandle = WF200_ControlInfo.WF200_TaskInfo[taskId].TaskHandle;
	    WF200_ControlInfo.WF200_TaskInfo[taskId].TaskHandle = NULL;
	    WF200_ControlInfo.WF200_TaskInfo[taskId].TaskCreated = false;
	    WF200_ControlInfo.WF200_TaskInfo[taskId].TaskRunning = false;

	    PRINTF("WF200_DeleteTask: Task deleted = %i\n", taskId);

	    vTaskDelete(taskHandle);
	}
	else
	{
		WF200_DriverStatus = SL_STATUS_FAIL;
		PRINTF("WF200_DeleteTask: ERROR - Trying to delete a task that is already deleted\n" );
		WF200_DEBUG_TRAP;
	}

	return WF200_DriverStatus;
}


/***************************************************************************//**
 * @brief       WF200_DeleteQueue: Delete message queue
 *
 * @param[in]   Task ID
 *
 * @return      Error Status
 *
 ******************************************************************************/
sl_status_t WF200_DeleteQueue(const WF200_TaskIds taskId)
{
	sl_status_t WF200_DriverStatus = SL_STATUS_OK;

	assert(taskId < WF200_NUM_TASKS);

	vQueueDelete(WF200_ControlInfo.WF200_TaskInfo[taskId].QueueHandle);
	WF200_ControlInfo.WF200_TaskInfo[taskId].QueueHandle = NULL;

	PRINTF("WF200 STA MODE TASK QUEUE BEEN DELETED\n");

	return WF200_DriverStatus;
}


/*******************************************************************************
 * @brief       WF200_GetControlInfoPtr: Returns pointer to the Control Info
 *              structure
 *
 * @param[in]   None
 *
 * @return      Pointer to ControlInfo structure
 *
 ******************************************************************************/
const WF200_ControlInfo_t* WF200_GetControlInfoPtr(void)
{
	return &WF200_ControlInfo;
}


/*******************************************************************************
 * @brief       WF200_SetTaskCreated: Set the Task Created Flag
 *
 * @param[in]  Task ID
 *             Flag
 *
 * @return     None
 *
 ******************************************************************************/
void WF200_SetTaskCreated(const WF200_TaskIds taskId, const bool taskCreated)
{
	assert(taskId < WF200_NUM_TASKS);

	WF200_ControlInfo.WF200_TaskInfo[taskId].TaskCreated = taskCreated;
}


/*******************************************************************************
 * @brief       WF200_SetTaskRunning: Set the Task Running Flag
 *
 * @param[in]   Task ID
 *              Flag
 *
 * @return      None
 *
 ******************************************************************************/
void WF200_SetTaskRunning(const WF200_TaskIds taskId, const bool taskRunning)
{
	assert(taskId < WF200_NUM_TASKS);

	WF200_ControlInfo.WF200_TaskInfo[taskId].TaskRunning = taskRunning;
}


/*******************************************************************************
 * @brief       WF200_SetTaskHandle: Set the Task Handle
 *
 * @param[in]   Task ID
 *              Task Handle
 *
 * @return      None
 *
 ******************************************************************************/
void WF200_SetTaskHandle(const WF200_TaskIds taskId, const TaskHandle_t taskHandle)
{
	assert(taskId < WF200_NUM_TASKS);

	WF200_ControlInfo.WF200_TaskInfo[taskId].TaskHandle = taskHandle;
}


/*******************************************************************************
 * @brief       WF200_SetQueueHandle: Set the Queue Handle
 *
 * @param[in]   Task ID
 *              Queue Handle
 *
 * @return      None
 *
 ******************************************************************************/
void WF200_SetQueueHandle(const WF200_TaskIds taskId, const QueueHandle_t queueHandle)
{
	assert(taskId < WF200_NUM_TASKS);

	WF200_ControlInfo.WF200_TaskInfo[taskId].QueueHandle = queueHandle;
}


/*******************************************************************************
 * @brief       WF200_SetEventGroupHandle: Set the Event Group Handle
 *
 * @param[in]   Event Group Handle
 *
 * @return      None
 *
 ******************************************************************************/
void WF200_SetEventGroupHandle(const EventGroupHandle_t EventGroupHandle)
{
	WF200_ControlInfo.EventGroupHandle = EventGroupHandle;
}


/*******************************************************************************
 * @brief       WF200_SetAwake: Set the Awake Flag
 *
 * @param[in]   Flag
 *
 * @return      None
 *
 ******************************************************************************/
void WF200_SetAwake(const bool awake)
{
	WF200_ControlInfo.Awake = awake;
}


/*******************************************************************************
 * @brief       WF200_Set_AP_ModeRequired: Set Mode Required Flag
 *
 * @param[in]   Flag
 *
 * @return      None
 *
 ******************************************************************************/
void WF200_Set_AP_ModeRequired(const bool AP_ModeRequired)
{
	WF200_ControlInfo.AP_ModeRequired = AP_ModeRequired;
}


/*******************************************************************************
 * @brief       WF200_SetTestingModeRequired: Set Mode Required Flag
 *
 * @param[in]   Flag
 *
 * @return      None
 *
 ******************************************************************************/
void WF200_SetTestingModeRequired(const bool testingModeRequired)
{
	WF200_ControlInfo.TestingModeRequired = testingModeRequired;
}


/*******************************************************************************
 * @brief       WF200_Set_STA_ModeRequired: Set Mode Required Flag
 *
 * @param[in]   Flag
 *
 * @return      None
 *
 ******************************************************************************/
void WF200_Set_STA_ModeRequired(const bool STA_ModeRequired)
{
	WF200_ControlInfo.STA_ModeRequired = STA_ModeRequired;
}


/*******************************************************************************
 * @brief       WF200_SetSemaphoreHandle: Set Semaphore Handle
 *
 * @param[in]   Semaphore Handle
 *
 * @return      None
 *
 ******************************************************************************/
void WF200_SetSemaphoreHandle(const SemaphoreHandle_t receiveFrameSemaphoreHandle)
{
	WF200_ControlInfo.ReceiveFrameSemaphoreHandle = receiveFrameSemaphoreHandle;
}


/*******************************************************************************
 * @brief       WF200_Set_STA_ModeNetworkConnected: Set Network Connected Flag
 *
 * @param[in]   Flag
 *
 * @return      None
 *
 ******************************************************************************/
void WF200_Set_STA_ModeNetworkConnected(const bool STA_ModeNetorkConnected)
{
	WF200_ControlInfo.STA_ModeNetworkConnected = STA_ModeNetorkConnected;
}


/*******************************************************************************
 * @brief       WF200_Set_STA_CommandedDisconnect: Set Commanded Disconnect Flag
 *
 * @param[in]   Flag
 *
 * @return      None
 *
 ******************************************************************************/
void WF200_Set_STA_CommandedDisconnect(const bool STA_CommandedDisconnect)
{
	WF200_ControlInfo.STA_CommandedDisconnect = STA_CommandedDisconnect;
}


/*******************************************************************************
 * @brief       WF200_Get_WF200_Context: Returns a pointer to the WF200 Context
 *
 * @param[in]   None
 *
 * @return      Pointer to the WF200 Context
 *
 ******************************************************************************/
const sl_wfx_context_t* WF200_Get_WF200_Context(void)
{
	return &WF200_Context;
}


/*******************************************************************************
 * @brief       WF200_Set_AP_LinkStatus: Set the AP Link Status
 *
 * @param[in]   LinkStatus
 *
 * @return      None
 *
 ******************************************************************************/
void WF200_Set_AP_LinkStatus(const AP_LinkStatus_t LinkStatus)
{
	WF200_ControlInfo.AP_LinkStatus = LinkStatus;
}


/*******************************************************************************
 * @brief       WF200_Set_STA_LinkStatus: Set the STA Link Status
 *
 * @param[in]   LinkStatus
 *
 * @return      None
 *
 ******************************************************************************/
void WF200_Set_STA_LinkStatus(const STA_LinkStatus_t LinkStatus)
{
	WF200_ControlInfo.STA_LinkStatus = LinkStatus;
}


/*******************************************************************************
 * @brief       WF200_Set_AP_NetIF_Status: Set the AP NETIF Added Status
 *
 * @param[in]   NETIF Status
 *
 * @return      None
 *
 ******************************************************************************/
void WF200_Set_AP_NetIF_Status(const AP_NetIF_Status_t NetIF_Status)
{
	WF200_ControlInfo.AP_NetIF_Status = NetIF_Status;
}


/*******************************************************************************
 * @brief       WF200_Set_AP_LinkStatus: Set the AP Link Status
 *
 * @param[in]   LinkStatus
 *
 * @return      None
 *
 ******************************************************************************/
void WF200_Set_STA_NetIF_Status(const STA_NetIF_Status_t NetIF_Status)
{
	WF200_ControlInfo.STA_NetIF_Status = NetIF_Status;
}
