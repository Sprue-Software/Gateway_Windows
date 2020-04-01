/*
 * Copyright (c) 2013 - 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*******************************************************************************
 * \file	wisafe_main.c
 * \brief 	Hardware initialisation and creation of FreeRTOS tasks, queues,
 * 			and semaphores.
 * \note 	Project: 	WG2. Low cost wireless gateway
 * \author  Abdul Ben-Rashed
 ******************************************************************************/

/* FreeRTOS kernel includes. */
#include <timer.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"


#include "wisafe_main.h"
#include "wisafe_drv.h"
#include "clock_config.h"
#include "fsl_gpio.h"

#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "fsl_lpspi.h"
#include "board.h"
#include "radio.h"
#include "messages.h"
#include "logic.h"
#include "SDcomms.h"
#include "filesystem.h"

#include "fsl_common.h"
#include "pin_mux.h"
#if ((defined FSL_FEATURE_SOC_INTMUX_COUNT) && (FSL_FEATURE_SOC_INTMUX_COUNT))
#include "fsl_intmux.h"
#endif

#include "fsl_gpt.h"

#include "LOG_Api.h"

#include "HAL.h"


/*******************************************************************************
* Definitions
******************************************************************************/
// Master related
#define EXAMPLE_LPSPI_MASTER_BASEADDR (LPSPI3)
#define EXAMPLE_LPSPI_MASTER_IRQN (LPSPI3_IRQn)
#define EXAMPLE_LPSPI_MASTER_IRQHandler (LPSPI3_IRQHandler)

#define EXAMPLE_LPSPI_MASTER_PCS_FOR_INIT (kLPSPI_Pcs0)
#define EXAMPLE_LPSPI_MASTER_PCS_FOR_TRANSFER (kLPSPI_MasterPcs0)

// Select USB1 PLL PFD0 (720 MHz) as lpspi clock source
#define EXAMPLE_LPSPI_CLOCK_SOURCE_SELECT (1U)
// Clock divider for master lpspi clock source
#define EXAMPLE_LPSPI_CLOCK_SOURCE_DIVIDER (7U)

#define EXAMPLE_LPSPI_CLOCK_FREQ \
    (CLOCK_GetFreq(kCLOCK_Usb1PllPfd0Clk) / (EXAMPLE_LPSPI_CLOCK_SOURCE_DIVIDER + 1U))

#define EXAMPLE_LPSPI_MASTER_CLOCK_FREQ EXAMPLE_LPSPI_CLOCK_FREQ

#define TRANSFER_BAUDRATE (10000000U) /*! Transfer baud-rate - 2000k */

uint32_t LPSPI_GetInstance(LPSPI_Type *base);



#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
//! @brief Pointers to lpspi clocks for each instance.
static const clock_ip_name_t s_lpspiClocks[] = LPSPI_CLOCKS;

#if defined(LPSPI_PERIPH_CLOCKS)
static const clock_ip_name_t s_LpspiPeriphClocks[] = LPSPI_PERIPH_CLOCKS;
#endif

#endif // FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL


#define PRIORITIE_OFFSET 4 //nishi
// Task priorities.
#define RM_MainLoopHandler_task_PRIORITY 	 	 	tskIDLE_PRIORITY + 5 + PRIORITIE_OFFSET  // priority = 9
#define SW_TIMER_PERIOD_MS 					2900
#define QUEUE_LENGTH 						4
#define QUEUE_ITEM_SIZE 					16

#if LEARN_IN_BUTTON
// ABR
#define UserButtonHandler_task_PRIORITY 	 	 	tskIDLE_PRIORITY + 4 + PRIORITIE_OFFSET // priority = 8
#define UB_TIMER_PERIOD_MS 	 	 	 	 	1000
#endif

/*******************************************************************************
* Prototypes
******************************************************************************/
// LPSPI user callback
//void LPSPI_MasterUserCallback(LPSPI_Type *base, lpspi_master_handle_t *handle, status_t status, void *userData);
void Init_LPSPI(void);



static void RM_MainLoopHandler_task(void *pvParameters);
void vTimerRM_ReceiveCallback( TimerHandle_t xTimer );
static void WGtoRMHandler_task(void *pvParameters);

#if LEARN_IN_BUTTON
// ABR
static void UserButtonHandler_task(void *pvParameters);
void vTimerUserButtonCallback( TimerHandle_t xTimer );
#endif

/*******************************************************************************
* Variables
******************************************************************************/
//uint8_t *keyptr;

TaskHandle_t xHandleRM_MainLoop;
TimerHandle_t xTimerRM_Receive;
SemaphoreHandle_t xSemaphore = NULL;
SemaphoreHandle_t syncMatch = NULL;
QueueHandle_t xQueueRMtoWG, xQueueWGtoRM;

#if LEARN_IN_BUTTON
// ABR
static uint8_t  pushbuttonlongpress, pushbuttonshortpress;
static uint32_t pushbuttonlastendtime;

TaskHandle_t xHandleUserButton;
TimerHandle_t xTimerUserButton;
SemaphoreHandle_t xUserButtonSemaphore = NULL;
#endif

/*******************************************************************************
* Code
******************************************************************************/



/*******************************************************************************
 * \brief	Radio transceiver NIRQ interrupt service.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

void RADIO_CHIP_NIRQ_IRQ_HANDLER(void)
{
    GPIO_PortClearInterruptFlags(RADIO_CHIP_NIRQ_GPIO, 1U << RADIO_CHIP_NIRQ_GPIO_PIN);
    Set_RadioInterruptStatus();
}



/*******************************************************************************
 * \brief	Radio Sync detect interrupt service.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

void SYNC_WORD_DETECT_IRQ_HANDLER(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    Set_SyncDetected();
    if (syncMatch)
    {
        xSemaphoreGiveFromISR(syncMatch,&xHigherPriorityTaskWoken);
    }
    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );

    GPIO_PortClearInterruptFlags(SYNC_WORD_DETECT_GPIO, 1U << SYNC_WORD_DETECT_GPIO_PIN);
}


#if LEARN_IN_BUTTON
/*******************************************************************************
 * \brief   User Button interrupt service.
 *
 * \param   None.
 * \return  None.
 ******************************************************************************/

void USER_BUTTON_IRQ_HANDLER(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    vTaskPrioritySet( xHandleUserButton, RM_MainLoopHandler_task_PRIORITY + 1 );

    xSemaphoreGiveFromISR(xUserButtonSemaphore,&xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );

    GPIO_PortClearInterruptFlags(USER_BUTTON_GPIO, 1U << USER_BUTTON_GPIO_PIN);
}
#endif


/*******************************************************************************
 * \brief	Wisafe Main function.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

void Wisafe_Main(void)
{
    //////// [RE:workaround] We do the below during startup
    // BOARD_ConfigMPU();
    // BOARD_InitPins();
    // BOARD_BootClockRUN();
    // BOARD_InitDebugConsole();

    Init_LPSPI();
    Init_Timer();
    Init_Radio();
    // Init_Uart();     //////// [RE:workaround] This interferes with the stack's use of the UART for debug messages
    Init_FileSystem();
    Init_RM();

    xSemaphore = xSemaphoreCreateBinary();
    if(xSemaphore == NULL)
    {
        LOG_Error("Failed to create semaphore.");
    }

    syncMatch = xSemaphoreCreateBinary();
    if(syncMatch == NULL)
    {
        LOG_Error("Failed to create sync match semaphore.");
    }

    // Queue for received RM Messages passed to WG2
    xQueueRMtoWG = xQueueCreate( QUEUE_LENGTH, QUEUE_ITEM_SIZE );
    if(xQueueRMtoWG == NULL)
    {
        LOG_Error("RMtoWG Queue creation failed!");
        while(1);
    }

    // Queue for messages passed from WG2 to RM to be transmitted
    xQueueWGtoRM = xQueueCreate( QUEUE_LENGTH, QUEUE_ITEM_SIZE );
    if(xQueueWGtoRM == NULL)
    {
        LOG_Error("WGtoRM Queue creation failed!");
        while(1);
    }

    xTimerRM_Receive = xTimerCreate("TimerRM_Receive",pdMS_TO_TICKS(SW_TIMER_PERIOD_MS),pdFALSE,( void * ) 0,vTimerRM_ReceiveCallback);	// one shot
    if( xTimerRM_Receive == NULL )
    {
        LOG_Error("Timer RM_Receive creation failed!");
        while(1);
    }

#if LEARN_IN_BUTTON
    // ABR
    // Create a semaphore to indicate a button press
    xUserButtonSemaphore = xSemaphoreCreateBinary();
    if(xUserButtonSemaphore == NULL)
    {
        LOG_Error("Failed to create semaphore.");
    }

    // Create a timer for use by button press
    xTimerUserButton = xTimerCreate("TimerUserButton",pdMS_TO_TICKS(UB_TIMER_PERIOD_MS),pdFALSE,( void * ) 0,vTimerUserButtonCallback); // one shot
    if( xTimerUserButton == NULL )
    {
        LOG_Error("Timer UB creation failed!");
        while(1);
    }

    // Create a task for handling user button)
    if (xTaskCreate(UserButtonHandler_task, "UserButtonHandler_task", configMINIMAL_STACK_SIZE + 50, NULL, UserButtonHandler_task_PRIORITY, &xHandleUserButton) != pdPASS)
    {
        LOG_Error("User button task creation failed!");
        while(1);
    }
#endif

    // Create a task for handling main Loop)
    if (xTaskCreate(RM_MainLoopHandler_task, "RM_MainLoopHandler_task", configMINIMAL_STACK_SIZE, NULL, RM_MainLoopHandler_task_PRIORITY, &xHandleRM_MainLoop) != pdPASS)
    {
        LOG_Error("RM_MainLoopHandler task creation failed!");
        while(1);
    }

    xSemaphoreGive(xSemaphore);			// so that main starts straight away
}



/*******************************************************************************
 * \brief       Write received message to WG queue.
 *
 * \param       message - message to send
 * \return      None.
 ******************************************************************************/

void writeWGqueue(uint8_t *message)
{
    if (xQueueRMtoWG)
    {
        if(xQueueSendToBack(xQueueRMtoWG, message, 10) != pdPASS )
        {
            LOG_Error("Data could not be sent to queue!");
        }
        else
        {
            //LOG_Info("Data to WGq");
            LOG_Info("xQueueRMtoWG --->success");
        }
    }
}



/*******************************************************************************
 * \brief       Main task handler.
 *
 * \param       *pvParameters. Pointer to task parameters
 * \return      None.
 ******************************************************************************/

static void RM_MainLoopHandler_task(void *pvParameters)
{
#if LEARN_IN_BUTTON
    GPIO_PortEnableInterrupts(USER_BUTTON_GPIO, 1U << USER_BUTTON_GPIO_PIN);
#endif

    GPIO_PortEnableInterrupts(SYNC_WORD_DETECT_GPIO, 1U << SYNC_WORD_DETECT_GPIO_PIN);

    for (;;)
    {
        if(xSemaphoreTake(xSemaphore, 5000) != pdTRUE)
        {
            LOG_Error("Failed to receive main control loop trigger semaphore !!");
        }
        RM_MainLoop();
        xTimerStart(xTimerRM_Receive, 0);
    }
}



/*******************************************************************************
 * \brief       Check for message from WG and initiate processing
 *
 * \param       None
 * \return      None
 ******************************************************************************/

void checkWGtoRMqueue(void)
{
    uint8_t xReceivedWGMessage[QUEUE_ITEM_SIZE];

    if( xQueueReceive( xQueueWGtoRM, &xReceivedWGMessage, 0 ) == pdPASS )
    {
         ProcessIncomingMsgRaw(&xReceivedWGMessage[0]);
    }
}



/*******************************************************************************
 * \brief       Software timer callback function.
 *
 * \param       xTimer. Timer handler.
 * \return      None
 ******************************************************************************/

void vTimerRM_ReceiveCallback(TimerHandle_t xTimer)
{
	xSemaphoreGive(xSemaphore);
}



#if LEARN_IN_BUTTON
static learn_state_type_t learnInButtonStatus = LEARN_INACTIVE;

/*******************************************************************************
 * \brief	Get Wisafe learn-in state
 *
 * \param	None.
 * \return	None.
 ******************************************************************************/

learn_state_type_t getWisafeButtonStatus(void)
{
    return learnInButtonStatus;
}

// ABR
/*******************************************************************************
 * \brief   User button task handler.
 *
 * \param   *pvParameters. Pointer to task parameters
 * \return  None.
 ******************************************************************************/

static void UserButtonHandler_task(void *pvParameters)
{
    for (;;)
    {
        if(xSemaphoreTake(xUserButtonSemaphore, portMAX_DELAY) == pdTRUE)
        {
            int32_t pushbuttonstarttime = GetCurrentTimerCount();
            int32_t timeinterval;

            if(xTimerStop(xTimerUserButton, 0) != pdPASS)
            {
                LOG_Error("The request to stop the timer failed");
            }

            timeinterval = pushbuttonstarttime - pushbuttonlastendtime;

            if(timeinterval > ONE_SECOND)
            {
                pushbuttonshortpress = 0;
                pushbuttonlongpress = 0;
            }
            learnInButtonStatus = LEARN_ACTIVE;
            while(0 == GPIO_PinRead(USER_BUTTON_GPIO, USER_BUTTON_GPIO_PIN))
            {
                OSAL_sleep_ms(100);
            }
            learnInButtonStatus = LEARN_INACTIVE;

            pushbuttonlastendtime = GetCurrentTimerCount();
            timeinterval =  pushbuttonlastendtime - pushbuttonstarttime;

            if(timeinterval < ONE_SECOND)
            {
                pushbuttonshortpress += 1;
                if(pushbuttonlongpress > 0)
                {
                    pushbuttonshortpress = 1;
                    pushbuttonlongpress = 0;
                }
            }
            else if(timeinterval > FIVE_SECOND)
            {
                pushbuttonlongpress += 1;
            }
            else
            {
                pushbuttonshortpress = 0;
                pushbuttonlongpress = 0;
            }

            if(xTimerStart(xTimerUserButton, 0) != pdPASS)
            {
                LOG_Error(" The timer could not be set into the Active state");
            }

            vTaskPrioritySet( xHandleUserButton, UserButtonHandler_task_PRIORITY );
        }
    }
}



/*******************************************************************************
 * \brief   Software timer callback function.
 *
 * \param   xTimer. Timer handler.
 * \return  None.
 ******************************************************************************/

void vTimerUserButtonCallback(TimerHandle_t xTimer)
{
    if( (pushbuttonlongpress == 1) && (pushbuttonshortpress == 1) )
    {
        // Unlearn
        uint8_t xRmMessage[] = {0xD3,0x12,LEARN_BUTTON_PRESS_MASK | CMD_UNLEARN,0x7E};
        if(xQueueSendToFront(xQueueWGtoRM, xRmMessage, 0) != pdPASS )
        {
            LOG_Error("Unpair message NOT sent");
        }
    }
    else if(pushbuttonshortpress == 1)
    {
        // Learn in
        uint8_t xRmMessage[] = {0xD3,0x12,LEARN_BUTTON_PRESS_MASK | CMD_LEARN,0x7E};
        if(xQueueSendToFront(xQueueWGtoRM, xRmMessage, 0) != pdPASS )
        {
            LOG_Error("Pair message NOT sent");
        }
    }
    pushbuttonlongpress = 0;
    pushbuttonshortpress = 0;
}
#endif


/*******************************************************************************
 * \brief	Configure LPSPI as master.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

void LPSPI_MasterInit_New(LPSPI_Type *base, const lpspi_master_config_t *masterConfig, uint32_t srcClock_Hz)
{
    assert(masterConfig);

    uint32_t tcrPrescaleValue = 0;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)

    uint32_t instance = LPSPI_GetInstance(base);
    // Enable LPSPI clock
    CLOCK_EnableClock(s_lpspiClocks[instance]);

#if defined(LPSPI_PERIPH_CLOCKS)
    CLOCK_EnableClock(s_LpspiPeriphClocks[instance]);
#endif

#endif // FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL

    // Reset to known status
    LPSPI_Reset(base);

    // Set LPSPI to master
    LPSPI_SetMasterSlaveMode(base, kLPSPI_Master);

    // Status (SR) register:
    base->SR = 0x00000000;

    // Interrupt Enable (IER) register: Disable all interrupts
    base->IER = 0x00000000;

    // DMA Enable (DER) register: Disable all DMA requests
    base->DER = 0x00000000;

    // Configuration (CFGR0) register: Normal operation (no data matching), disable host request
    base->CFGR0 = 0x00000000;

    //The CFGR1 should only be written when the LPSPI is disabled.
    // Configuration (CFGR1) register: PCS enabled, Retain last o/p value,SIN for input and SOUT for output, Match disabled
    // Chip select pin PCSx active low, Master mode, Sample on SCK edge, Auto PCS disabled, No No-Stall
    base->CFGR1 = 0x00000001;

    // Clock configuration (CCR) register: As in next statement
    // Set baud-rate and delay times
    LPSPI_MasterSetBaudRate(base, masterConfig->baudRate, srcClock_Hz, &tcrPrescaleValue);

    // FIFO Control (FCR) register: TXWATER = 0, RXWATER = 0
    base->FCR = 0x00000000;

    // Set Transmit Command Register
    base->TCR = 0x00000007;

    // CR register: Module enabled, and enabled in debug mode
    base->CR = 0x00000009;

    LPSPI_MasterSetDelayTimes(base, masterConfig->pcsToSckDelayInNanoSec, kLPSPI_PcsToSck, srcClock_Hz);
    LPSPI_MasterSetDelayTimes(base, masterConfig->lastSckToPcsDelayInNanoSec, kLPSPI_LastSckToPcs, srcClock_Hz);
    LPSPI_MasterSetDelayTimes(base, masterConfig->betweenTransferDelayInNanoSec, kLPSPI_BetweenTransfer, srcClock_Hz);

    LPSPI_SetDummyData(base, LPSPI_DUMMY_DATA);
}



/*******************************************************************************
 * \brief   Initialise LPSPI.
 *
 * \param   None.
 * \return  None.
 ******************************************************************************/

void Init_LPSPI(void)
{
    //Set clock source for LPSPI
    CLOCK_SetMux(kCLOCK_LpspiMux, EXAMPLE_LPSPI_CLOCK_SOURCE_SELECT);
    CLOCK_SetDiv(kCLOCK_LpspiDiv, EXAMPLE_LPSPI_CLOCK_SOURCE_DIVIDER);

    //Master configuration
    lpspi_master_config_t masterConfig;

    masterConfig.baudRate = TRANSFER_BAUDRATE;
    masterConfig.bitsPerFrame = 8;
    masterConfig.cpol = kLPSPI_ClockPolarityActiveHigh;
    masterConfig.cpha = kLPSPI_ClockPhaseFirstEdge;
    masterConfig.direction = kLPSPI_MsbFirst;

    masterConfig.pcsToSckDelayInNanoSec = 1000000000 / masterConfig.baudRate;
    masterConfig.lastSckToPcsDelayInNanoSec = 1000000000 / masterConfig.baudRate;
    masterConfig.betweenTransferDelayInNanoSec = 1000000000 / masterConfig.baudRate;

    masterConfig.whichPcs = EXAMPLE_LPSPI_MASTER_PCS_FOR_INIT;
    masterConfig.pcsActiveHighOrLow = kLPSPI_PcsActiveLow;

    masterConfig.pinCfg = kLPSPI_SdiInSdoOut;
    masterConfig.dataOutConfig = kLpspiDataOutRetained;

    LPSPI_MasterInit_New(EXAMPLE_LPSPI_MASTER_BASEADDR, &masterConfig, EXAMPLE_LPSPI_MASTER_CLOCK_FREQ);

    LPSPI_FlushFifo(EXAMPLE_LPSPI_MASTER_BASEADDR, true, true);
    LPSPI_ClearStatusFlags(EXAMPLE_LPSPI_MASTER_BASEADDR, kLPSPI_AllStatusFlag);
    LPSPI_DisableInterrupts(EXAMPLE_LPSPI_MASTER_BASEADDR, kLPSPI_AllInterruptEnable);
}


/*******************************************************************************
 * \brief   wait_syncMatch
 *
 * \param   timeout in ms
 * \return  None.
 ******************************************************************************/
bool wait_syncMatch(uint32_t timeout)
{
    if (xSemaphoreTake(syncMatch,timeout) == pdTRUE)
    {
        return true;
    }
    return false;
}
