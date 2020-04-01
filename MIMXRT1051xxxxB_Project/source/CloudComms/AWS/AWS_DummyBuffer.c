
/*!****************************************************************************
*
* \file AWS_DummyBuffer.c
*
* \author Gavin Dolling
*
* \brief A stub for the faults buffering code
*
* \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

#include "AWS_CommsHandler.h"
#include "AWS_FaultBuffer.h"


/*!****************************************************************************
 * Constants
 *****************************************************************************/

/******************************************************************************
 * Type Definitions
 *****************************************************************************/

/*!****************************************************************************
 * Private Variables
 *****************************************************************************/

/*!****************************************************************************
 * Private Function Prototypes
 *****************************************************************************/

/*!****************************************************************************
 * Public Functions
 *****************************************************************************/


/*
 * \brief Initialise function for the fault buffer, empty in this version
 *
 * \return                   Error code
 */
EnsoErrorCode_e AWS_FaultBufferInit(void)
{
    return eecNoError;
}

/*
 * \brief  Stops Fault Buffer
 */
void AWS_FaultBufferStop(void)
{

}

/*
 * \brief Handler function for processing property change deltas
 *
 * \param  subscriberId      The destination id to receive the notification
 *
 * \param  deviceId          The thing that changed so is publishing the
 *                           update to its subscriber list.
 *
 * \param  propertyGroup     The group to which the properties belong (reported
 *                           or desired)
 *
 * \param  numProperties     The number of properties in the delta
 *
 * \param  deltasBuffer      The list of properties that have been changed
 *
 * \return                   Error code
 */
EnsoErrorCode_e AWS_FaultBuffer(
        const HandlerId_e subscriberId,
        const EnsoDeviceId_t deviceId,
        const PropertyGroup_e propertyGroup,
        const uint16_t numProperties,
        const EnsoPropertyDelta_t* deltasBuffer)
{
    // We have no fault buffer, just pass it straight back
    return AWS_OnCommsHandler(subscriberId, deviceId, propertyGroup, numProperties,
                deltasBuffer, NULL);
}



/*
 * \brief  Fault buffer made it to the cloud successfully
 *
 * \param  ptr     Pointer to the buffer that contains the fault
 */
void AWS_FinishedWithFaultBuffer(void* ptr)
{
}


/*
 * \brief  Fault buffer did not make it to the cloud successfully
 *
 * \param  ptr     Pointer to the buffer that contains the fault
 */
void AWS_SendFailedForFaultBuffer(void)
{
}


void AWS_ConnectionStateCB(bool connected)
{
    (void)connected;
}


/*
 * \brief  Do we have any faults waiting to be sent to the cloud?
 *
 * \return true if we do, false otherwise
 */
bool AWS_FaultBufferEmpty(void)
{
    return true;
}


/**
 * \name   AWS_SetTimerForPeriodicWorker
 * \brief  Start timer to call periodic to process messages
 */
void AWS_SetTimerForPeriodicWorker(void)
{
}


/**
 * \name   AWS_ForceBackoffDuringDiscovery
 * \brief  Used to request a pause in sending properties to the cloud
 */
void AWS_ForceBackoffDuringDiscovery(void)
{
}

void AWS_SendWaitingDelta(void)
{
}
