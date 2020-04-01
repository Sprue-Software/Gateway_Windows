
/*!****************************************************************************
*
* \file AWS_FaultBuffer.c
*
* \author Gavin Dolling
*
* \brief A buffer to ensure all faults make it to AWS eventually.
*
* \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

#include <assert.h>
#include <string.h>

#include "AWS_CommsHandler.h"
#include "AWS_FaultBuffer.h"
#include "AWS_Timestamps.h"
#include "ECOM_Api.h"
#include "ECOM_Messages.h"
#include "APP_Types.h"
#include "AWS_FaultTypes.h"
#include "LOG_Api.h"
#include "OSAL_Api.h"
#include "LSD_Api.h"
#include "LSD_Types.h"


/*!****************************************************************************
 * Constants
 *****************************************************************************/

// Space for at least 100 ECOM Deltas with 4 properties
// We need 101 buffers as in the code, when we generate a free buffer we allow
// space for a maximum sized buffer of 6 deltas (to cater for the worst case)
// Strictly speaking we need 99 of size 4 and one of size 6, but just for ease
// specify 101 buffers
// 32 is the size of the delta header
// 4 is the number of properties in a fault report
#define AWS_BUFFER_SIZE (101 * (32 + (4 * sizeof(EnsoPropertyDelta_t))))
#define AWS_INITIAL_BACKOFF_TIME_IN_SECS 30	//////// [RE:workaround] Increased this from 5
#define AWS_PAUSE_MS 1000
#define AWS_MESSAGE_IN_FLIGHT_TIMEOUT_S 30

/******************************************************************************
 * Type Definitions
 *****************************************************************************/

/*!****************************************************************************
 * Private Variables
 *****************************************************************************/

// Pointers for circular buffer
static Delta_t* _startBuffer; // First delta in list
static Delta_t* _endBuffer;  // Next unused position at end of the buffer
static int _bufferCount;   // Needed as when start == end buffer can be full or empty

// Have we sent a message for sending
static bool _messageInFlight;
static int _messageInFlightTimeout;

// Time to wait before retrying
static uint32_t _propertyBackOffTimeInSeconds;
static uint32_t _discoveryBackOffTimeInSeconds;
static uint8_t _faultBuffer[AWS_BUFFER_SIZE];

// Mutex for accessing circular buffer
static Mutex_t _bufferMutex;

// Mutex for setting timer
static Mutex_t _timerMutex;

static bool _bAWSConnected = false;

static bool _bAWSStopped = false;

static Timer_t _timer = NULL;

static Thread_t periodicWorkerThread;

static Semaphore_t _periodicWorkerSemaphore;

/*!****************************************************************************
 * Private Function Prototypes
 *****************************************************************************/

// Some are not static so they can be unit tested, do not use externally
Delta_t* _FindFreeBuffer(int forNumProperties);
static void _PeriodicWorkerCB(void* p);
static void _PeriodicWorker(void* p);
static Delta_t* _NextBuffer(Delta_t* current, int properties);
static void _returnBuffer(Delta_t* buffer);
void _CircularBufferInit(void);
bool _AWS_SendOutOfSyncProperties(void);
extern bool AWS_RegisterNewDeltaRequest(void);


/*!****************************************************************************
 * Public Functions
 *****************************************************************************/


/*
 * \brief  Fault buffer initialisation code
 * \return Error code
 */
EnsoErrorCode_e AWS_FaultBufferInit(void)
{
    OSAL_InitMutex(&_timerMutex, NULL);

    _CircularBufferInit();

    _propertyBackOffTimeInSeconds = 0;
    _discoveryBackOffTimeInSeconds = 0;

    _messageInFlight = false;
    _messageInFlightTimeout = 0;

    if (OSAL_InitBinarySemaphore(&_periodicWorkerSemaphore) != 0)
    {
        LOG_Error("OSAL_InitBinarySemaphore() failed for AWS Fault Buffer periodic worker semaphore");
        return eecInternalError;
    }
    periodicWorkerThread = OSAL_NewThread(_PeriodicWorker, NULL);
    if (periodicWorkerThread == 0)
    {
        LOG_Error("OSAL_NewThread failed for AWS Fault Buffer periodic worker thread");
        return eecInternalError;
    }

    return eecNoError;
}

/*
 * \brief  Stops Fault Buffer
 */
void AWS_FaultBufferStop(void)
{
    OSAL_LockMutex(&_timerMutex);

    _bAWSStopped = true;

    if (_timer != NULL)
    {
        OSAL_DestroyTimer(_timer);
        _timer = NULL;
    }

    OSAL_UnLockMutex(&_timerMutex);
}

/*
 * \brief Handler function for processing property change deltas
 * \param  subscriberId      The destination id to receive the notification
 * \param  deviceId          The thing that changed so is publishing the
 * \param  propertyGroup     The group to which the properties belong (reported
 *                           or desired)
 * \param  numProperties     The number of properties in the delta
 * \param  deltasBuffer      The list of properties that have been changed
 * \return                   Error code
 */
EnsoErrorCode_e AWS_FaultBuffer(
        const HandlerId_e subscriberId,
        const EnsoDeviceId_t deviceId,
        const PropertyGroup_e propertyGroup,
        const uint16_t numProperties,
        const EnsoPropertyDelta_t* deltasBuffer)
{
    EnsoErrorCode_e result = eecNoError;

    // Get free fault buffer
    Delta_t* buffer = _FindFreeBuffer(numProperties);

    if (buffer != NULL)
    {
        buffer->subscriberId = subscriberId;
        buffer->deviceId = deviceId;
        buffer->propertyGroup = propertyGroup;
        buffer->numProperties = numProperties;
        memcpy(buffer->buffer, deltasBuffer, numProperties * sizeof(EnsoPropertyDelta_t));
        buffer->readyToSend = true;

        // If buffer was empty before, start worker thread
        if (_bufferCount == 1)
        {
            AWS_SetTimerForPeriodicWorker();
        }
    }
    else
    {
        // This should never occur as we can always make space by throwing away
        // the oldest entry, but warn if we see it
        LOG_Warning("We have no free fault buffers to process this delta");
        result = eecPoolFull;
    }

    return result;
}

/*
 * \brief  Fault buffer made it to the cloud successfully
 * \param  ptr     Pointer to the buffer that contains the fault
 */
void AWS_FinishedWithFaultBuffer(void* ptr)
{
    _messageInFlight = false;

    // All is well
    _propertyBackOffTimeInSeconds = 0;
    _discoveryBackOffTimeInSeconds = 0;

    _returnBuffer(ptr);
}



/*
 * \brief  Fault buffer did not make it to the cloud successfully
 */
void AWS_SendFailedForFaultBuffer(void)
{
    _messageInFlight = false;

    if (_propertyBackOffTimeInSeconds == 0)
    {
        // On first failure back off for a preset amount
        _propertyBackOffTimeInSeconds = AWS_INITIAL_BACKOFF_TIME_IN_SECS;
    }
    else
    {
        // Then increase increase back off time, up to a limit
        if (_propertyBackOffTimeInSeconds < 60)
        {
            _propertyBackOffTimeInSeconds++;
        }
    }
}

/*
 * \brief  Callback to get AWS Connection State
 * \param  connected AWS connection state. True if connected, otherwise False
 */
void AWS_ConnectionStateCB(bool connected)
{
    _bAWSConnected = connected;

    if (!_bAWSConnected)
    {
        AWS_SendFailedForFaultBuffer();
    }
}


/**
 * \name   AWS_SetTimerForPeriodicWorker
 * \brief  Start timer to call periodic to process messages
 */
void AWS_SetTimerForPeriodicWorker(void)
{
    OSAL_LockMutex(&_timerMutex);

    /* Do not allow to set time if AWS Stopped */
    if (_bAWSStopped)
    {
        OSAL_UnLockMutex(&_timerMutex);
        return;
    }

    ////////// [RE:fixme] Is it safe to delete ourselves blocking-ly and while there's still code to run?
    if (_timer != NULL)
    {
        OSAL_DestroyTimer(_timer);
    }
    _timer = OSAL_NewTimer(_PeriodicWorkerCB, AWS_PAUSE_MS, false, NULL);

    OSAL_UnLockMutex(&_timerMutex);
}


/**
 * \name   AWS_ForceBackoffDuringDiscovery
 * \brief  Used to request a pause in sending properties to the cloud
 */
void AWS_ForceBackoffDuringDiscovery(void)
{
    _discoveryBackOffTimeInSeconds = AWS_INITIAL_BACKOFF_TIME_IN_SECS;
    AWS_SetTimerForPeriodicWorker();
}

/*
 * \brief This function sends waiting delta in Fault Buffer Queue.
 *
 */
void AWS_SendWaitingDelta(void)
{
    OSAL_LockMutex(&_bufferMutex);

    if (_bufferCount > 0)
    {
        _messageInFlight = true;
        _messageInFlightTimeout = 0;

        LOG_Trace("subscriberId %d Start buffer = %X, count = %i, num properties = %i",
                  _startBuffer->subscriberId, _startBuffer, _bufferCount, _startBuffer->numProperties);
        // Try again with first item in the FIFO
        AWS_OnCommsHandler(
            _startBuffer->subscriberId,
            _startBuffer->deviceId,
            _startBuffer->propertyGroup,
            _startBuffer->numProperties,
            _startBuffer->buffer,
            (void*)_startBuffer);
    }

    OSAL_UnLockMutex(&_bufferMutex);
}

/*!****************************************************************************
 * Private Functions
 *****************************************************************************/

/*
 * \brief  Fault buffer initialisation code
 * \return Error code
 */
void _CircularBufferInit(void)
{
    // Set up circular buffer
    _startBuffer = _endBuffer = (Delta_t*) _faultBuffer;
    _bufferCount = 0;

    OSAL_InitMutex(&_bufferMutex, NULL);
}


/*
 * \brief  Find next free buffer and if there isn't one then make one
 * \param  forNumProperties  - How big does this buffer need to be?
 * \return pointer to next from buffer
 */
Delta_t* _FindFreeBuffer(int forNumProperties)
{
    // Circular buffer to give a FIFO queue of deltas to process
    OSAL_LockMutex(&_bufferMutex);

    // Is there space in the buffer for this request?
    uint8_t* newEndOfBuffer = (uint8_t*)&_endBuffer->buffer[forNumProperties];

    // Will endOfBuffer overtake the start pointer?
    while((_bufferCount > 0) &&  // If buffer is empty then we are ok
          (_endBuffer <= _startBuffer) && // end pointer not in front of start pointer
          (newEndOfBuffer > (uint8_t*)_startBuffer)) // and then overtake
    {
        // Buffer will overrun when we return this one so ...
        LOG_Warning("Fault buffer is full, throwing away a set of deltas of size %i",
                _startBuffer->numProperties);

        _bufferCount--;

        // Buffer is now full. Throw away oldest one and try again
        _startBuffer->inUse = false;
        _startBuffer = _NextBuffer(_startBuffer, _startBuffer->numProperties);
    }

    Delta_t* freeBuffer = _endBuffer;

    // Move endBuffer on
    _endBuffer->inUse = true;
    _endBuffer->readyToSend = false;
    _endBuffer->numProperties = forNumProperties;
    _endBuffer = _NextBuffer(_endBuffer, forNumProperties);
    _bufferCount++;

    OSAL_UnLockMutex(&_bufferMutex);

    return freeBuffer;
}


/*
 * \brief  Return buffer to the circular buffer to be re-used
 * \param  index  index of buffer to return
 */
static void _returnBuffer(Delta_t* buffer)
{
    OSAL_LockMutex(&_bufferMutex);

    if (buffer->inUse)
    {
        buffer->inUse = false;
    }
    else
    {
        // This is ok if we had to throw a buffer away while it was in transit
        LOG_Error("Why did we get back a buffer that wasn't in use?");
    }

    // Don't assume the buffer returned is the one at the start,
    // we may have multiple channels in use and so be processing
    // more than one delta in parallel
    while ((_bufferCount > 0) && (!_startBuffer->inUse))
    {
        _startBuffer = _NextBuffer(_startBuffer, _startBuffer->numProperties);
        _bufferCount--;
    }

    OSAL_UnLockMutex(&_bufferMutex);
}


/**
 * \name   _NextBuffer
 * \brief  Helper function to get 'next' fault buffer in circular buffer
 * \param  current  buffer index to increment
 * \param  num properties that are (or will be) in this buffer
 */
static Delta_t* _NextBuffer(Delta_t* current, int numProperties)
{
    Delta_t* next = (Delta_t*) &current->buffer[numProperties];

    // Will this next buffer fit in remaining space?
    uint8_t* end = (uint8_t*) &next->buffer[ECOM_MAX_DELTAS];
    if ((end -_faultBuffer) > AWS_BUFFER_SIZE)
    {
        // This will overhang the end of the buffer, back to beginning
        // It might fit of course, but we have to allow for maximum size
        next = (Delta_t*)_faultBuffer;
    }

    return next;
}

/**
 * \name   _PeriodicCallback
 * \brief  Called to do fault buffer processing
 * \param  p     not used
 */
static void _PeriodicWorkerCB(void* p)
{
    OSAL_GiveBinarySemaphore(&_periodicWorkerSemaphore);
}

/**
 * \name   _PeriodicCallback
 * \brief  Called to do fault buffer processing
 * \param  p     not used
 */
static void _PeriodicWorker(void* p)
{
    static int counter = 0;
    bool finished = false;
    EnsoDeviceId_t unused;

    // For ever loop
    for ( ; ; )
    {
        OSAL_TakeBinarySemaphore(&_periodicWorkerSemaphore);
        finished = false;

        if (!_bAWSConnected)
        {
            // No connection! Do not do anything
        }
        else  if (!AWS_IsTimeValid())
        {
            // No comms until time is valid
        }
        else if (AWS_GetHeadOfDeleteQueue(&unused))
        {
            // Send device deletes first before any deltas
        }
        else if(_discoveryBackOffTimeInSeconds)
        {
            if (_discoveryBackOffTimeInSeconds == AWS_INITIAL_BACKOFF_TIME_IN_SECS)
            {
                LOG_Info("Fault buffer in back off mode for discovery, secs left %u",
                    _discoveryBackOffTimeInSeconds);
            }
            _discoveryBackOffTimeInSeconds--;

        }
        else if (++counter < _propertyBackOffTimeInSeconds)
        {
            if (counter == 1)
            {
                LOG_Info("Fault buffer in back off mode for send failure, secs left %u",
                    (_propertyBackOffTimeInSeconds - counter));
            }
        }
        else if (!_messageInFlight)
        {
            counter = 0;

            OSAL_LockMutex(&_bufferMutex);

            // _startBuffer is moved on in callback when send succeeds
            if (_bufferCount == 0)
            {
                if (_startBuffer != _endBuffer)
                {
                    // Should never happen
                    LOG_Error("Buffer count is empty, but start and end pointers don't match?");
                    _endBuffer = _startBuffer = (Delta_t*)_faultBuffer; // Try and recover
                }

                if(!_AWS_SendOutOfSyncProperties())
                {
                    LOG_Info("Fault buffer empty, nothing out of sync - fault processing stopping");
                    finished = true;
                }
            }
            else
            {
                // Check if comms handler is busy
                if (!AWS_OutOfSyncTimerEnabled())
                {
                    if (_startBuffer->readyToSend)
                    {
                        /* Just inform AWS about new delta */
                        AWS_RegisterNewDeltaRequest();
                    }
                    else
                    {
                        // It will be ready next time
                        LOG_Info("Fault buffer not ready to send")
                    }
                }
                else
                {
                    LOG_Info("Comms handler busy, wait for sending")
                }
            }
            OSAL_UnLockMutex(&_bufferMutex);
        }
        else
        {
            // Called every AWS_PAUSE_MS
            _messageInFlightTimeout++;
            int waitingTimeInSecs = (AWS_PAUSE_MS * _messageInFlightTimeout) / 1000;
            if (waitingTimeInSecs > 2)
            {
                LOG_Info("Message is in flight, waited for %i secs", waitingTimeInSecs);
            }

            /* The callback for message status is often not received if an unexpected network
             * event has occurred and so we need to timeout and try again. If we are executing
             * this code then comms handler is up and reporting that it is healthy
             */
            if (waitingTimeInSecs >= AWS_MESSAGE_IN_FLIGHT_TIMEOUT_S)
            {
                LOG_Error("We have timed out waiting for callback for message in flight");
                AWS_SendFailedForFaultBuffer();
            }
        }

        if (!finished)
        {
            AWS_SetTimerForPeriodicWorker();
        }
    }
}

/*
 * \brief  Do we have any faults waiting to be sent to the cloud?
 *
 * \return true if we do, false otherwise
 */
bool AWS_FaultBufferEmpty(void)
{
    return _bufferCount == 0;
}



/*
 * \brief  Trigger a search for for any out of sync properties
 *
 * \return true if we do, false otherwise
 */
bool _AWS_SendOutOfSyncProperties(void)
{
    bool propertiesOutOfSync = true;
    uint16_t sentMessages;

    // Get the message queue for this thing
    MessageQueue_t destQueue = ECOM_GetMessageQueue(COMMS_HANDLER);

    if (destQueue)
    {
        int queueSize = OSAL_GetMessageQueueSize(destQueue);
        int currentSize = OSAL_GetMessageQueueNumCurrentMessages(destQueue);
        int space = queueSize - currentSize;

        if (space <= 0)
        {
            LOG_Error("Out of space for sending out of sync messages");
            space = 0;
        }
        else if (space < 2)
        {
            LOG_Warning("Nearly out of space for sending out of sync messages (space=%i)", space);
        }
        else
        {
            // Space looks ok, but give ourselves some leeway
            space -= 2;
        }

        if (space > 0)
        {
            /* Send out-of-sync properties of Reported Group */
            EnsoErrorCode_e retVal = LSD_SyncLocalShadow(COMMS_HANDLER,
                                                         REPORTED_GROUP,
                                                         space,
                                                         &sentMessages);

            if (eecNoError != retVal)
            {
                LOG_Error("Reported shadow sync has failed");
            }

            /* To avoid flood, let's finish REPORTED_GROUP properties first */
            if (eecNoError == retVal && sentMessages == 0)
            {
                /* Send out-of-sync properties of Desired Group */
                retVal = LSD_SyncLocalShadow(COMMS_HANDLER,
                                             DESIRED_GROUP,
                                             space,
                                             &sentMessages);

                if (eecNoError != retVal)
                {
                    LOG_Error("Desired shadow sync has failed");
                }
                else if (sentMessages == 0)
                {
                    propertiesOutOfSync = false;
                }
            }
        }
    }
    else
    {
        LOG_Error("Unable to find comms handler message queue");
    }

    return propertiesOutOfSync;
}



/******************************************************************************
 * Unit test code only, do not use outside of unit tests
 *****************************************************************************/

/**
 * \name   _getBufferCount
 * \brief  Used by unit test to ensure we can store minimum faults
 */
int _getBufferCount()
{
    return _bufferCount;
}

/**
 * \name   _getStartBuffer
 * \brief  Used by unit test to ensure we can store minimum faults
 */
Delta_t* _getStartBuffer()
{
    return _startBuffer;
}
