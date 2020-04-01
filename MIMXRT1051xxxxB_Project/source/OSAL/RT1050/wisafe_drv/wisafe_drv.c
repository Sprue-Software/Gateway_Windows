/*******************************************************************************
 * Includes
 ******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "LOG_Api.h"

#include "wisafe_drv.h"
#include "wisafe_main.h"



/*******************************************************************************
 * Constants
 ******************************************************************************/

//////// [RE:wisafe] The below was copied from wisafe_main.c
#define QUEUE_ITEM_SIZE 16



/*******************************************************************************
 * Variables
 ******************************************************************************/

uint8_t readBuffer[QUEUE_ITEM_SIZE];
uint8_t writeBuffer[QUEUE_ITEM_SIZE];



/*******************************************************************************
 * Imports
 ******************************************************************************/

extern QueueHandle_t xQueueRMtoWG, xQueueWGtoRM;



/*******************************************************************************
 * Declarations
 ******************************************************************************/

void Wisafe_Main(void);



/*******************************************************************************
 * Code
 ******************************************************************************/

void wisafedrv_init()
{
    LOG_Info("vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv");
    LOG_Info("wisafedrv init");
    Wisafe_Main();
    LOG_Info("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");
}



void wisafedrv_read(uint8_t* bytes, uint32_t* count)
{
    LOG_Info("vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv");
    LOG_Info("wisafedrv read");

    for (;;)
    {
        // Receive a message from xQueueRMtoWG
        if(xQueueReceive(xQueueRMtoWG, &readBuffer, portMAX_DELAY ) != pdPASS)
        {
            // Nothing was received from the queue – even after blocking to wait for data to arrive.
            LOG_Error("<<< No data received from xQueueRMtoWG");
        }
        else
        {
            //////// [RE:wisafe] The below was adapted from SDcomms.c::ProcessIncomingMsgRaw
            uint8_t data, index;
            index = 0;
            do
            {
                data = readBuffer[index];
                bytes[index] = data;
                LOG_Info("<<< %02X", data);
                index += 1;
            } while ((data != 0x7E) && (index < sizeof(readBuffer)));

            if (data == 0x7E)
            {
                *count = index - 1;
            }
            else
            {
                *count = 0;
                LOG_Error("Missing flag at end of message !!!");
            }

            LOG_Info("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");

            return;
        }
    }
}



void wisafedrv_write(uint8_t* bytes, uint32_t count)
{
    LOG_Info("vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv");
    LOG_Info("wisafedrv write");
    LOG_Info("-------------------------------------");
    for (int i = 0; i < count; i++)
    {
        LOG_Info(">>> %02X", bytes[i]);
    }
    LOG_Info("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");

    // Check we don't exceed the queue's message size
    if (count > sizeof(writeBuffer))
    {
        LOG_Error("WiSafe message too large for queue, write aborted");
        return;
    }

    // Prepare the message buffer
    memset(writeBuffer, 0, sizeof(writeBuffer));
    memcpy(writeBuffer, bytes, count);

    // Send the message to xQueueWGtoRM
    if(xQueueSendToBack(xQueueWGtoRM, &writeBuffer, 10) != pdPASS )
    {
        LOG_Error("WiSafe message write failed, queue full ?");
    }
}



void wisafedrv_close()
{
    LOG_Info("vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv");
    LOG_Info("wisafedrv close");
    LOG_Info("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");
}
