/*!****************************************************************************
*
* \file WiSafe_Control.cpp
*
* \author James Green
*
* \brief Interface allowing control of the WiSafe Device Handler.
*
* \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

#include "assert.h"

#include "ECOM_Messages.h"
#include "LOG_Api.h"

#include "WiSafe_Control.h"
#include "WiSafe_Main.h"

/**
 * Helper to allocate new control messages.
 *
 * @return The new message, or NULL.
 */
static controlEntry_t* allocateNewControlMessage(void)
{
    radioCommsBuffer_t* messageBuffer = WiSafe_RadioCommsBufferGet();
    controlEntry_t* message = (controlEntry_t*)messageBuffer;

    assert(sizeof(controlEntry_t) <= RADIOCOMMSBUFFER_LENGTH); // We cast comms buffers to controlEntry. So check there is enough memory allocated in the data part of the comms buffer to do so.

    if (message == NULL)
    {
        LOG_Warning("Control API unable to allocate message buffer - command will not execute.");
    }

    return message;
}

/**
 * Submit the control message to the main loop for processing.
 *
 * @param msg The message.
 */
static void submitControlMessage(controlEntry_t* controlMsg)
{
    commandMessage_st msg;
    msg.messageType = ECOM_COMMAND_RX;
    msg.control = controlMsg;

    const MessagePriority_e priority = MessagePriority_high;
    if (OSAL_SendMessage(MainMessageQueue, &msg, sizeof(msg), priority) < 0)
    {
        LOG_Error("SendMessage() failed.");
    }
}

/**
 * Submit a control message to begin a missing node scan.
 *
 * @param deleteMissingDevices Whether or not to delete missing nodes at the end of the test.
 */
void WiSafe_ControlBeginMissingNodeScan(bool deleteMissingDevices)
{
    controlEntry_t* msg = allocateNewControlMessage();
    if (msg != NULL)
    {
        msg->operation = controlOp_beginMissingNodeScan;
        msg->param1 = (uint32_t)deleteMissingDevices;
        submitControlMessage(msg);
    }
}

/**
 * Submit a control message to begin a device test.
 *
 * @param id The device to be tested.
 */
void WiSafe_ControlIssueTest(deviceId_t id)
{
    controlEntry_t* msg = allocateNewControlMessage();
    if (msg != NULL)
    {
        msg->operation = controlOp_issueTest;
        msg->param1 = (uint32_t)id;
        submitControlMessage(msg);
    }
}

/**
 * Submit a control message to enable/disable learn mode.
 *
 * @param enabled Enable/disable learn mode.
 * @param timeout Timeout for maximum learn period.
 */
void WiSafe_ControlLearnMode(bool enabled, uint32_t timeout)
{
    controlEntry_t* msg = allocateNewControlMessage();
    if (msg != NULL)
    {
        msg->operation = controlOp_learn;
        msg->param1 = enabled;
        msg->param2 = timeout;
        submitControlMessage(msg);
    }
}

/**
 * Submit a control message to stop a missing node scan.
 *
 */
void WiSafe_ControlStopMissingNodeScan(void)
{
    controlEntry_t* msg = allocateNewControlMessage();
    if (msg != NULL)
    {
        msg->operation = controlOp_stopMissingNodeScan;
        submitControlMessage(msg);
    }
}

/**
 * Submit a control message to flash all devices.
 *
 */
void WiSafe_ControlFlushAll(void)
{
    controlEntry_t* msg = allocateNewControlMessage();
    if (msg != NULL)
    {
        msg->operation = controlOp_flushAll;
        submitControlMessage(msg);
    }
}

/**
 * Submit a control message to flush the given device.
 *
 * @param id The device ID.
 */
void WiSafe_ControlFlush(deviceId_t id)
{
    controlEntry_t* msg = allocateNewControlMessage();
    if (msg != NULL)
    {
        msg->operation = controlOp_flush;
        msg->param1 = (uint32_t)id;
        submitControlMessage(msg);
    }
}

/**
 * Submit a control message to identify the given device.
 *
 * @param id The device ID.
 */
void WiSafe_ControlIdentifyDevice(deviceId_t id)
{
    controlEntry_t* msg = allocateNewControlMessage();
    if (msg != NULL)
    {
        msg->operation = controlOp_identifyDevice;
        msg->param1 = (uint32_t)id;
        submitControlMessage(msg);
    }
}

/**
 * Submit a control message to leave the current mesh network.
 *
 */
void WiSafe_ControlLeave(void)
{
    controlEntry_t* msg = allocateNewControlMessage();
    if (msg != NULL)
    {
        msg->operation = controlOp_leave;
        submitControlMessage(msg);
    }
}

/**
 * Submit a control message to silence all devices.
 *
 * @param id The device to be tested.
 */
void WiSafe_ControlSilenceAll(void)
{
    controlEntry_t* msg = allocateNewControlMessage();
    if (msg != NULL)
    {
        msg->operation = controlOp_silenceAll;
        submitControlMessage(msg);
    }
}

/**
 * Submit a control message to locate devices in alarm state.
 *
 * @param id The device ID.
 */
void WiSafe_ControlLocate(void)
{
    controlEntry_t* msg = allocateNewControlMessage();
    if (msg != NULL)
    {
        msg->operation = controlOp_locate;
        submitControlMessage(msg);
    }
}
