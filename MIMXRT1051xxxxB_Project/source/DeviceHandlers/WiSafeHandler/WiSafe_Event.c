/*!****************************************************************************
*
* \file WiSafe_Event.c
*
* \author James Green
*
* \brief WiSafe module that handles fault and alarm messages.
*
* \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

#include <stdio.h>
#include <string.h>
//#include <sys/time.h>

#include "LOG_Api.h"
#include "OSAL_Api.h"
#include "LSD_Api.h"

#include "WiSafe_Event.h"
#include "WiSafe_Protocol.h"
#include "WiSafe_RadioCommsBuffer.h"
#include "WiSafe_DAL.h"
#include "WiSafe_Discovery.h"
#include "WiSafe_RadioComms.h"
#include "WiSafe_Main.h"

// Length of the Fault Details bit map
#define FAULT_MAP_LENGTH 16
// Length of the Fault Flags bit map
#define FAULT_FLAGS_LENGTH 8

#define NO_FAULT_CODE 0
#define MAX_FAULT_CODE 99
#define NO_FAULT_DETAIL_TYPES 9

// Length of fault property strings plus null terminator
#define FAULTPROPERTYLENGTH       5 + 1
#define FAULTSTATEPROPERTYLENGTH 11 + 1     // flt90_state
#define FAULTSEQPROPERTYLENGTH    9 + 1     // flt90_seq
#define FAULTTIMEPROPERTYLENGTH  10 + 1     // flt90_time
#define FAULTBATTVPROPERTYLENGTH 11 + 1     // flt90_battv

enum spruefaultstatusbits
{
    FAULT_IS_CALIBRATED    = 0x01,
    FAULT_IS_FAULTY        = 0x02,
    FAULT_IS_ON_BASE       = 0x04,
    FAULT_BATTERY_FAULT    = 0x08,
    FAULT_AC_FAILED        = 0x10,
    FAULT_RM_BATTERY_FAULT = 0x20
};

/*!
 *\brief fault flags received in SPI 0x71 Fault message
 */
enum fault_flags {
    FAULT_FLAG_CALIBRATED        = 0,
    FAULT_FLAG_FAULTY            = 1,
    FAULT_FLAG_ON_BASE           = 2,
    FAULT_FLAG_SD_BATTERY_FAULT  = 3,
    FAULT_FLAG_AC_FAILED         = 4,
    FAULT_FLAG_RM_BATTERY_FAULT  = 5,
    FAULT_FLAG_NOT_USED_1        = 6,
    FAULT_FLAG_NOT_USED_2        = 7
};

/**
 *  Default alert code used when a wisafe fault message is received and we can't
 *  map it to a more specific code. For example, if a request for additional
 *  fault details from the device fails or if an unexpected fault is received.
 */
const uint8_t AlertCodeDefault = 91;


// Alarm State Property values set in the Alarm property
enum alarm_state
{
    ALARM_STATE_INACTIVE                   = 0,
    ALARM_STATE_ACTIVE_DEFAULT             = 1,
    ALARM_STATE_ACTIVE_SILENCED            = 2,
    ALARM_STATE_ACTIVE_CANNOT_BE_SILENCED  = 3,
    ALARM_STATE_ACTIVE_PRIORITY_4          = 4,  // Temperature priority 4 Cool
    ALARM_STATE_ACTIVE_PRIORITY_4_SILENCED = 5,
    ALARM_STATE_ACTIVE_PRIORITY_3          = 6,  // Temperature priority 3 Warm
    ALARM_STATE_ACTIVE_PRIORITY_3_SILENCED = 7,
    ALARM_STATE_ACTIVE_PRIORITY_2          = 8,  // Temperature priority 2 Cold
    ALARM_STATE_ACTIVE_PRIORITY_2_SILENCED = 9,
    ALARM_STATE_ACTIVE_PRIORITY_1          = 10, // Temperature priority 1 Hot
    ALARM_STATE_ACTIVE_PRIORITY_1_SILENCED = 11,
    ALARM_STATE_ACTIVE_PRIORITY_0          = 12, // Temperature priority 0 Very cold
    ALARM_STATE_ACTIVE_PRIORITY_0_SILENCED = 13
};

#define NUM_STATUS_CODES 22
// Map of received Alarm Status to Event state - only some of the wisafe values
// are defined. Undefined values map to the default. These values are defined:
// 0x00 0  = Alarm Active with no additional information
// 0x01 1  = Alarm Active - silenced
// 0x02 2  = Alarm Active - cannot be silenced
// 0x04 4  = Alarm Active - Temperature priority 4 (Cool - low priority)
// 0x05 5  = Alarm Active - Temperature priority 4 silenced
// 0x08 8  = Alarm Active - Temperature priority 3 (Warm)
// 0x09 9  = Alarm Active - Temperature priority 3 silenced
// 0x0C 12 = Alarm Active - Temperature priority 2 (Cold)
// 0x0D 13 = Alarm Active - Temperature priority 2 silenced
// 0x10 16 = Alarm Active - Temperature priority 1 (Hot)
// 0x11 17 = Alarm Active - Temperature priority 1 silenced
// 0x14 20 = Alarm Active - Temperature priority 0 (Very cold - high priority)
// 0x15 21 = Alarm Active - Temperature priority 0 silenced

const uint8_t alarm_status_to_event_state[NUM_STATUS_CODES] =
                                { ALARM_STATE_ACTIVE_DEFAULT,             //  0
                                  ALARM_STATE_ACTIVE_SILENCED,            //  1
                                  ALARM_STATE_ACTIVE_CANNOT_BE_SILENCED,  //  2
                                  ALARM_STATE_ACTIVE_DEFAULT,             //  3
                                  ALARM_STATE_ACTIVE_PRIORITY_4,          //  4
                                  ALARM_STATE_ACTIVE_PRIORITY_4_SILENCED, //  5
                                  ALARM_STATE_ACTIVE_DEFAULT,             //  6
                                  ALARM_STATE_ACTIVE_DEFAULT,             //  7
                                  ALARM_STATE_ACTIVE_PRIORITY_3,          //  8
                                  ALARM_STATE_ACTIVE_PRIORITY_3_SILENCED, //  9
                                  ALARM_STATE_ACTIVE_DEFAULT,             // 10
                                  ALARM_STATE_ACTIVE_DEFAULT,             // 11
                                  ALARM_STATE_ACTIVE_PRIORITY_2,          // 12
                                  ALARM_STATE_ACTIVE_PRIORITY_2_SILENCED, // 13
                                  ALARM_STATE_ACTIVE_DEFAULT,             // 14
                                  ALARM_STATE_ACTIVE_DEFAULT,             // 15
                                  ALARM_STATE_ACTIVE_PRIORITY_1,          // 16
                                  ALARM_STATE_ACTIVE_PRIORITY_1_SILENCED, // 17
                                  ALARM_STATE_ACTIVE_DEFAULT,             // 18
                                  ALARM_STATE_ACTIVE_DEFAULT,             // 19
                                  ALARM_STATE_ACTIVE_PRIORITY_0,          // 20
                                  ALARM_STATE_ACTIVE_PRIORITY_0_SILENCED  // 21
                                };

// Map Fault Detail Flags to Fault Codes
uint8_t faultDetailsMap[NO_FAULT_DETAIL_TYPES][FAULT_MAP_LENGTH] =
{
{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 }, // None
{  2,  4,  6,  8, 10, 14, 16, 18,  0, 19, 22, 25, 30,  0,  0,  0 }, // 1 Smoke
{  2,  4,  6,  8, 10, 14, 16, 18,  0, 19, 22, 25, 30,  0,  0,  0 }, // 2 Heat
{  2,  4,  6,  8, 12, 14, 16, 18, 19, 22, 25, 28, 31, 33, 34, 36 }, // 3 CO
{  2,  4,  6,  8, 12, 14, 16, 18, 19, 22, 25, 28, 31, 33, 34, 36 }, // 4 Cold
{  2,  4,  6,  8, 10, 14, 16, 18,  0, 19, 22, 25, 30, 32, 35, 37 }, // 5 AC Smk
{  2,  4,  6,  8, 10, 14, 16, 18,  0, 19, 22, 25, 30, 32, 35, 37 }, // 6 AC Heat
{  1,  3,  5,  7,  9, 13, 15, 17, 19, 23, 50, 29,  0,  0,  0, 38 }, // 7 Pad Str
{  1,  3,  5,  7,  9, 13, 15, 17, 19, 23, 50, 29,  0,  0,  0, 38 }  // 8 LFS
};

typedef struct
{
    msgFault_t fault;
    Timer_t timeoutRef;
} pendingFault_t;

typedef struct
{
    msgAlarm_t alarm;
    Timer_t timeoutRef;
} pendingAlarm_t;

static radioCommsBuffer_t* pendingFaults = NULL; // A list of pending faults that we are in the middle of processing (probably in the middle of fetching more detail).
static radioCommsBuffer_t* pendingAlarms = NULL; // A list of pending alarms that we are in the middle of processing (probably in the middle of fetching more detail).

static const uint32_t transactionTimeout = (30 * 1000); // Timeout for how long we should wait for futher details before reporting a fault/alarm.

/**
 * Helper to create a new entry in our list of pending faults that are
 * in progress.
 *
 * @param fault The fault details.
 *
 * @return The underlying buffer for the list item, or NULL if no free
 * buffers and unable to add to list.
 */
static radioCommsBuffer_t* CreatePendingFault(msgFault_t* fault)
{
    radioCommsBuffer_t* buffer = WiSafe_RadioCommsBufferGet();

    assert(sizeof(pendingFault_t) <= RADIOCOMMSBUFFER_LENGTH); // We cast comms buffers to pendingFault_t. So check there is enough memory allocated in the data part of the comms buffer to do so.

    if (buffer != NULL)
    {
        /* Copy the payload. */
        pendingFault_t* pendingFault = (pendingFault_t*)buffer;
        pendingFault->fault = *fault;

        /* Create the timeout. */
        Handle_t param = buffer;
        pendingFault->timeoutRef = OSAL_NewTimer(WiSafe_FaultTimerCallback, transactionTimeout, false, param); /* Needs to be short so we don't hold up notifying the platform */

        /* Add to the list. */
        buffer->next = pendingFaults;
        pendingFaults = buffer;
    }
    else
    {
        LOG_Warning("Unable to allocate pending fault buffer.");
    }

    return buffer;
}

/**
 * Helper to create a new entry in our list of pending alarms that are
 * in progress.
 *
 * @param alarm The alarm details.
 *
 * @return The underlying buffer for the list item, or NULL if no free
 * buffers and unable to add to list.
 */
static radioCommsBuffer_t* CreatePendingAlarm(msgAlarm_t* alarm)
{
    radioCommsBuffer_t* buffer = WiSafe_RadioCommsBufferGet();

    assert(sizeof(pendingAlarm_t) <= RADIOCOMMSBUFFER_LENGTH); // We cast comms buffers to pendingAlarm_t. So check there is enough memory allocated in the data part of the comms buffer to do so.

    if (buffer != NULL)
    {
        /* Copy the payload. */
        pendingAlarm_t* pendingAlarm = (pendingAlarm_t*)buffer;
        pendingAlarm->alarm = *alarm;

        /* Create the timeout. */
        Handle_t param = buffer;
        pendingAlarm->timeoutRef = OSAL_NewTimer(WiSafe_AlarmTimerCallback, transactionTimeout, false, param); /* Needs to be short so we don't hold up notifying the platform */

        /* Add to the list. */
        buffer->next = pendingAlarms;
        pendingAlarms = buffer;
    }
    else
    {
        LOG_Warning("Unable to allocate pending alarm buffer.");
    }

    return buffer;
}

/**
 * Delete a fault from our pending list.
 *
 * @param buffer The underlying buffer for the list item.
 * @param cancelTimeout Whether or not to cancel the timeout (normally
 * true, but false if the calling code has already taken care of it).
 */
static void DeletePendingFault(radioCommsBuffer_t* buffer, bool cancelTimeout)
{
    /* First cancel the timer. */
    if (cancelTimeout)
    {
        pendingFault_t* pendingFault = (pendingFault_t*)buffer;
        if (pendingFault->timeoutRef != NULL)
        {
            OSAL_DestroyTimer(pendingFault->timeoutRef);
        }
    }

    /* Now remove from the list. */
    WiSafe_RadioCommsBufferRemove(&pendingFaults, buffer);

    /* Now free the buffer. */
    WiSafe_RadioCommsBufferRelease(buffer);
}

/**
 * Delete an alamr from our pending list.
 *
 * @param buffer The underlying buffer for the list item.
 * @param cancelTimeout Whether or not to cancel the timeout (normally
 * true, but false if the calling code has already taken care of it).
 */
static void DeletePendingAlarm(radioCommsBuffer_t* buffer, bool cancelTimeout)
{
    /* First cancel the timer. */
    if (cancelTimeout)
    {
        pendingAlarm_t* pendingAlarm = (pendingAlarm_t*)buffer;
        if (pendingAlarm->timeoutRef != NULL)
        {
            OSAL_DestroyTimer(pendingAlarm->timeoutRef);
        }
    }

    /* Now remove from the list. */
    WiSafe_RadioCommsBufferRemove(&pendingAlarms, buffer);

    /* Now free the buffer. */
    WiSafe_RadioCommsBufferRelease(buffer);
}

/**
 * Find an entry in the pending fault list, indexed by device ID.
 *
 * @param id The required ID.
 *
 * @return The list item, or NULL if not found.
 */
static pendingFault_t* FindPendingFaultByDevice(deviceId_t id)
{
    radioCommsBuffer_t* current = pendingFaults;
    while (current != NULL)
    {
        /* Does this pending fault match the device ID we're after? */
        pendingFault_t* pendingFault = (pendingFault_t*)current;
        if (pendingFault->fault.id == id)
        {
            /* Found it. */
            return pendingFault;
        }

        /* On to the next one in the list. */
        current = current->next;
    }

    /* Not found. */
    return NULL;
}

/**
 * Find an entry in the pending alarm list, indexed by device ID.
 *
 * @param id The required ID.
 *
 * @return The list item, or NULL if not found.
 */
static pendingAlarm_t* FindPendingAlarmByDevice(deviceId_t id)
{
    radioCommsBuffer_t* current = pendingAlarms;
    while (current != NULL)
    {
        /* Does this pending alarm match the device ID we're after? */
        pendingAlarm_t* pendingAlarm = (pendingAlarm_t*)current;
        if (pendingAlarm->alarm.id == id)
        {
            /* Found it. */
            return pendingAlarm;
        }

        /* On to the next one in the list. */
        current = current->next;
    }

    /* Not found. */
    return NULL;
}

/*!
 *\brief static translate fault details flag number into a fault code
 *\param faultDetailsType Fault details type
 *\param faultNumber Fault details flag number to translate into a fault code
 *\return Fault code
 */
static uint8_t GetFaultDetailsCode(const int32_t faultDetailsType,
                                   const uint8_t faultNumber)
{
    if (faultNumber >= FAULT_MAP_LENGTH)
    {
        LOG_Error("fault number %u out of range", faultNumber);
        return NO_FAULT_CODE;
    }

    if (faultDetailsType >= NO_FAULT_DETAIL_TYPES)
    {
        LOG_Error("fault details type out of range: %i",faultDetailsType);
        return NO_FAULT_CODE;
    }

    return faultDetailsMap[faultDetailsType][faultNumber];
}


/*!
 *\brief static translate fault flag number into a fault code
 *\param faultNumber Fault flag number to translate into a fault code
 *\return Fault code
 */
static uint8_t GetFaultFlagCode(uint8_t faultNumber)
{
    // This array translates the fault flags received in a wisafe Fault message
    // into the code understood by the platform.
    static const uint8_t faultFlagCodes[FAULT_FLAGS_LENGTH] =
                              { 90, 91, 92, 93, 94, 95, 96, 97};

    uint8_t code = 0;
    if ( faultNumber < FAULT_FLAGS_LENGTH)
    {
        code = faultFlagCodes[faultNumber];
    }
    else
    {
        LOG_Error("Unable to decode fault details. fault number = %i",
                  faultNumber);
    }
    return code;
}

/*!
 \brief Send an alarm event

 This function sends an event when a wisafe Alarm message has been
 received.

 \param did the device id that sent the wisafe Alarm
 \param received_state new fault state. true = active, false = inactive
 \param tempPresent - is the rawTemp parameter being used
 \param rawTemp = raw temperature reading
 */
void SendAlarmEvent(const deviceId_t did,
                    const uint8_t received_state,
                    const bool tempPresent,
                    const uint32_t rawTemp)
{
    LOG_Trace("alarm active = %i", received_state);

    // Read the alarm sequence number property, if it doesn't exists yet
    // then it starts at zero.
    EnsoErrorCode_e error;
    EnsoPropertyValue_u sequenceNo = WiSafe_DALGetReportedProperty(
            did,
            DAL_PROPERTY_DEVICE_ALARM_SEQ_IDX,
            &error);

    if (error == eecNoError)
    {
        // We have read the sequence number property which implies it
        // already exists. If the received state is inactive then
        // use the stored sequence number. If the received state is
        // not inactive then increment the sequence number.

        if ( received_state != ALARM_STATE_INACTIVE)
        {
            // Increase the sequence number.
            sequenceNo.uint32Value++;
        }
    }
    else
    {
        // Property doesn't exist yet so default to inactive.
        sequenceNo.uint32Value = 0;
    }

    if (error == eecNoError || error == eecPropertyNotFound)
    {
        // Either
        // - we have read the sequence number property or
        // - the property doesn't exist yet. i.e. its the first time the
        //   alarm has been raised.

        // Create or update the group of properties making this alarm

        // Temperature property is optional - do this first
        if (tempPresent)
        {
            // The raw temperature received from the device needs to
            // be converted into 1/100ths of a degree Celsius

            float tempv = (79.9 - 0.096 * (rawTemp));

            EnsoPropertyValue_u valueTemp = { .int32Value = (int32_t)(tempv * 100) };
            LOG_InfoC(LOG_MAGENTA "Temperature received for device 0x%06x: raw: 0x%04x, actual: %foC, reported: %d",
                      did, rawTemp, tempv, valueTemp.int32Value);

            WiSafe_DALSetReportedProperty(
                                did,
                                DAL_PROPERTY_DEVICE_ALARM_TEMPV_IDX,
                                DAL_PROPERTY_DEVICE_ALARM_TEMPV,
                                evInt32,
                                valueTemp);
        }

        // Create group of properties for the alarm
        EnsoAgentSidePropertyId_t agentSideId[3];
        const char* cloudName[3];
        EnsoValueType_e type[3];
        EnsoPropertyValue_u value[3];

        // Alarm state property
        agentSideId[0] = DAL_PROPERTY_DEVICE_ALARM_STATE_IDX;
        cloudName[0] = DAL_PROPERTY_DEVICE_ALARM_STATE;
        type[0] = evUnsignedInt32;
        value[0].uint32Value = received_state;

        // Alarm sequence property
        agentSideId[1] = DAL_PROPERTY_DEVICE_ALARM_SEQ_IDX;
        cloudName[1] = DAL_PROPERTY_DEVICE_ALARM_SEQ;
        type[1] = evUnsignedInt32;
        value[1] = sequenceNo;

        // Alarm time property
        agentSideId[2] = DAL_PROPERTY_DEVICE_ALARM_TIME_IDX;
        cloudName[2] = DAL_PROPERTY_DEVICE_ALARM_TIME;
        type[2] = evTimestamp;
        value[2] = LSD_GetTimeNow();

        WiSafe_DALSetReportedProperties(did, agentSideId, cloudName, type, value, 3,
                PROPERTY_PUBLIC, true, true);
    }
}

/*!
 \brief Send a fault event

 This function sends an event when a wisafe Fault message has been
 received.

 If the stored state and received state are inactive then no action.

 If the received fault is active or a fault has cleared then an event is
 sent.

 \param did the device id that sent the wisafe Fault
 \param receivedFaultActive received fault: true = active, false = inactive
 \param code = Fault code
 \param voltsPresent - is the mVolts parameter being used
 \param mVolts = battery voltage in milliVolts
 */
void SendFaultEvent(const deviceId_t did,
                    const bool receivedFaultActive,
                    const uint8_t code,
                    const bool voltsPresent,
                    const uint32_t mVolts)
{
    LOG_Trace("received fault active = %u, code = %u",
              receivedFaultActive, code);

    if (code > NO_FAULT_CODE && code <= MAX_FAULT_CODE)
    {
        char faultName[FAULTPROPERTYLENGTH] = {0};
        int printcount = snprintf(faultName, FAULTPROPERTYLENGTH, "%s%02u",
                DAL_PROPERTY_DEVICE_FAULT, code);
        assert(printcount < FAULTPROPERTYLENGTH);
        LOG_Trace("faultName = |%s|",faultName);

        // Read the fault state property, if it doesn't exists yet
        // then it defaults to false (i.e. fault inactive).
        uint32_t propertyId = DAL_PROPERTY_DEVICE_FAULT_STATE_IDX + code;
        LOG_Trace("propertyId = %u",propertyId);
        EnsoErrorCode_e error;
        EnsoPropertyValue_u stored_state = WiSafe_DALGetReportedProperty(
                did,
                propertyId,
                &error);

        bool updateFaultProperties = true;
        if (error == eecNoError || error == eecPropertyNotFound)
        {
            if (error == eecPropertyNotFound)
            {
                // Property doesn't exist yet so default to inactive.
                stored_state.booleanValue = false;
            }
            // We have read the fault state property or the property doesn't
            // exist yet. i.e. its the first time the fault has been raised.
            // Check the stored and received flags
            if ( !stored_state.booleanValue &&
                 !receivedFaultActive)
            {
                // Not a new fault and its not a fault clear. Nothing to do.
                updateFaultProperties = false;
            }
        }

        if (updateFaultProperties)
        {
            // Read the fault sequence number property, if it doesn't exists yet
            // then it starts at zero.
            uint32_t propertyId = DAL_PROPERTY_DEVICE_FAULT_SEQ_IDX + code;
            LOG_Trace("propertyId = %u",propertyId);
            EnsoErrorCode_e error;
            EnsoPropertyValue_u sequenceNo = WiSafe_DALGetReportedProperty(
                    did,
                    propertyId,
                    &error);

            if (error == eecNoError)
            {
                // We have read the sequence number property which implies it
                // already exists. If the stored state was inactive and the new
                // state is active then increase the sequence number.

                if ( !stored_state.booleanValue &&
                     receivedFaultActive )
                {
                    // New fault has been raised. Increase the sequence number.
                    sequenceNo.uint32Value++;
                    LOG_Trace("Increase sequence to %u", sequenceNo.uint32Value);
                }
            }
            else
            {
                // Sequence number property doesn't exist so start at zero.
                sequenceNo.uint32Value = 0;
            }

            if (error == eecNoError || error == eecPropertyNotFound)
            {
                // Either
                // - we have read the sequence number property or
                // - the property doesn't exist yet. i.e. its the first time the
                //   fault has been raised.

                // Create or update the group of properties making this alarm

                // Battery voltage property is optional - do this first
                if (voltsPresent)
                {
                    // The voltage is received in mV and
                    // we convert it to units of 100mV.
                    EnsoPropertyValue_u valueV = { .uint32Value = mVolts / 10};

                    char faultNameBattV[FAULTBATTVPROPERTYLENGTH];
                    printcount = snprintf(faultNameBattV, FAULTBATTVPROPERTYLENGTH,
                            "%s%s", faultName, DAL_PROPERTY_DEVICE_FAULT_BATTV);
                    assert(printcount < FAULTBATTVPROPERTYLENGTH);
                    LOG_Trace("faultNameBattV = |%s|",faultNameBattV);
                    WiSafe_DALSetReportedProperty(
                            did,
                            DAL_PROPERTY_DEVICE_FAULT_BATTV_IDX + code,
                            faultNameBattV,
                            evUnsignedInt32,
                            valueV);
                }

                // Create or update the group of properties making this fault
                EnsoAgentSidePropertyId_t agentSideId[3];
                const char* cloudName[3];
                EnsoValueType_e type[3];
                EnsoPropertyValue_u value[3];

                // Fault state property
                char faultNameState[FAULTSTATEPROPERTYLENGTH];
                int printcount = snprintf(faultNameState, FAULTSTATEPROPERTYLENGTH,
                        "%s%s", faultName, DAL_PROPERTY_DEVICE_FAULT_STATE);
                assert(printcount < FAULTSTATEPROPERTYLENGTH);
                LOG_Trace("faultNameState = |%s|",faultNameState);
                EnsoPropertyValue_u faultState = { .booleanValue = receivedFaultActive };
                agentSideId[0] = DAL_PROPERTY_DEVICE_FAULT_STATE_IDX + code;
                cloudName[0] = faultNameState;
                type[0] = evBoolean;
                value[0] = faultState;

                // Fault sequence property
                char faultNameSeq[FAULTSEQPROPERTYLENGTH];
                printcount = snprintf(faultNameSeq, FAULTSEQPROPERTYLENGTH,
                        "%s%s", faultName, DAL_PROPERTY_DEVICE_FAULT_SEQ);
                assert(printcount < FAULTSEQPROPERTYLENGTH);
                LOG_Trace("faultNameSeq = |%s|",faultNameSeq);
                agentSideId[1] = DAL_PROPERTY_DEVICE_FAULT_SEQ_IDX + code;
                cloudName[1] = faultNameSeq;
                type[1] = evUnsignedInt32;
                value[1] = sequenceNo;

                // Fault time property
                EnsoPropertyValue_u faultTime = LSD_GetTimeNow();
                char faultNameTime[FAULTTIMEPROPERTYLENGTH];
                printcount = snprintf(faultNameTime, FAULTTIMEPROPERTYLENGTH,
                        "%s%s", faultName, DAL_PROPERTY_DEVICE_FAULT_TIME);
                assert(printcount < FAULTTIMEPROPERTYLENGTH);
                LOG_Trace("faultNameTime = |%s|",faultNameTime);
                agentSideId[2] = DAL_PROPERTY_DEVICE_FAULT_TIME_IDX + code;
                cloudName[2] = faultNameTime;
                type[2] = evTimestamp;
                value[2] = faultTime;

                WiSafe_DALSetReportedProperties(did, agentSideId, cloudName, type, value, 3,
                        PROPERTY_PUBLIC, true, true);
            }
        }
    }
}

/**
 * Handle a fault message received over SPI.
 *
 * @param msg The buffer containing the message.
 */
void WiSafe_FaultReceived(msgFault_t* msg)
{
    /* There's a fault, some devices support read extra details about the fault. */
    LOG_InfoC(LOG_MAGENTA "Fault received from device 0x%06x, model 0x%04x, flags 0x%02x, SID %d, seq %d.",
              msg->id, msg->model, msg->flags, msg->sid, msg->seq);

    // Step through the received fault flags
    for (uint8_t fault_flag_no = 0; fault_flag_no < FAULT_FLAGS_LENGTH; fault_flag_no++)
    {
        bool fault_flag = (msg->flags & (1 << fault_flag_no)) ? true : false;
        LOG_Trace("fault flag %i is %i", fault_flag_no, fault_flag);

        switch (fault_flag_no)
        {
            case FAULT_FLAG_CALIBRATED:
            {
                LOG_InfoC(LOG_MAGENTA "Fault flag set for FAULT_FLAG_CALIBRATED.");
                // The received calibrated flag is:
                // 0 = calibration error
                // 1 = calibration okay

                bool received_fault = fault_flag ? false : true;

                // Translate flag to a fault code
                uint8_t code = GetFaultFlagCode(fault_flag_no);
                SendFaultEvent(msg->id, received_fault, code, false, 0);
                break;
            }

            case FAULT_FLAG_FAULTY:
            {
                LOG_InfoC(LOG_MAGENTA "Fault flag set for FAULT_FLAG_FAULTY.");
                // The received is-faulty flag is:
                // 0 = no fault
                // 1 = request fault details

                /* Does the device support fault details? Need to find its priority and
                   then find the value of supports_fault_details for that type of device. */
                int32_t faultDetailsType = WiSafe_DiscoveryFaultDetailsType(msg->id);

                if (faultDetailsType)
                {
                    if (fault_flag)
                    {
                        // Fault is active
                        /* So are we going to read more details, if so create a pending fault, read the fault details, with a timeout in case we don't get a reply to request. */
                        bool notifyImmediately = true;
                        radioCommsBuffer_t* pendingFault = CreatePendingFault(msg);
                        if (pendingFault != NULL)
                        {
                            radioCommsBuffer_t* buffer = WiSafe_EncodeRequestFaultDetails(msg->id, msg->model);
                            if (buffer != NULL)
                            {
                                /* Make the request to read the fault details. */
                                LOG_InfoC(LOG_MAGENTA "Issuing request fault details for device %06x.", msg->id);
                                WiSafe_RadioCommsTx(buffer);
                                notifyImmediately = false;
                            }
                            else
                            {
                                /* No free buffers to do a fault details read, so just notify immediately with no details. */
                                LOG_Warning("No buffers to read fault details, so reporting fault immediately for device %06x.", msg->id);
                                DeletePendingFault(pendingFault, true);
                                notifyImmediately = true;
                            }
                        }
                        else
                        {
                            /* No free buffers to do a pending fault, so just notify immediately with no details. */
                            LOG_Warning("No buffers to do pending fault, so reporting fault immediately for device %06x.", msg->id);
                            notifyImmediately = true;
                        }

                        /* Whatever path was followed through the code above, should we report the fault immediately with no details? */
                        if (notifyImmediately)
                        {
                            /* Just report the fault immediately. */
                            LOG_InfoC(LOG_MAGENTA "Reporting fault for device 0x%06x with no details.", msg->id);
                            // Translate flag to a fault code
                            uint8_t code = GetFaultFlagCode(fault_flag_no);
                            SendFaultEvent(msg->id, fault_flag, code, false, 0);
                        }
                    }
                    else
                    {
                        // The device has no faults in its fault details map.
                        // Change the state of any active faults to inactive.
                        LOG_Info("faulty flag not set - clear active faults");

                        // Step through active faults and reset them
                        for (uint8_t fault_number = 0 ; fault_number < FAULT_MAP_LENGTH; fault_number++)
                        {
                            uint8_t code = GetFaultDetailsCode(faultDetailsType, fault_number);
                            if (code > NO_FAULT_CODE)
                            {
                                // Set the fault state to false, if the fault was previously active.
                                SendFaultEvent(msg->id, false, code, false, 0);
                            }
                        }

                        // Set the default fault state to inactive, if it was previously active
                        SendFaultEvent(msg->id, false, AlertCodeDefault, false, 0);
                    }
                }
                else
                {
                    // This device doesn't have fault details. Send the default event
                    LOG_Info("Device doesn't have additional fault details - send default fault");
                    SendFaultEvent(msg->id, fault_flag, AlertCodeDefault, false, 0);
                }
                break;
            }
            case FAULT_FLAG_ON_BASE:
            {
                LOG_InfoC(LOG_MAGENTA "Fault flag set for FAULT_FLAG_ON_BASE.");
                // The received on-base flag is:
                // 0 = not on base - fault active
                // 1 = on base     - fault inactive

                bool received_fault = fault_flag ? false : true;

                // Translate flag to a fault code
                uint8_t code = GetFaultFlagCode(fault_flag_no);

                SendFaultEvent(msg->id, received_fault, code, false, 0);

                break;
            }
            case FAULT_FLAG_SD_BATTERY_FAULT:
            {
                LOG_InfoC(LOG_MAGENTA "Fault flag set for FAULT_FLAG_SD_BATTERY_FAULT.");
                // The received SD battery fault flag is:
                // 0 = no fault    - fault inactive
                // 1 = fault       - fault active

                bool received_fault = fault_flag;

                /* So are we going to read the volts, or are we just going to report the fault immediately? */
                bool notifyImmediately;
                if ( received_fault )
                {
                    LOG_InfoC(LOG_MAGENTA "Battery Fault received from device 0x%06x, model 0x%04x, flags 0x%02x, SID %d, seq %d.", msg->id, msg->model, msg->flags, msg->sid, msg->seq);

                    // Does the device support battery voltage reads?
                    bool supportsReadVoltsOnFault =  WiSafe_DiscoveryIsFeatureSupported(msg->id, WISAFE_PROFILE_KEY_READ_VOLTS_ON_FAULT);

                    if (supportsReadVoltsOnFault)
                    {
                        /* Need to create a pending fault, read the volts, with a timeout in case we don't get a reply to reading the volts. */
                        radioCommsBuffer_t* pendingFault = CreatePendingFault(msg);
                        if (pendingFault != NULL)
                        {
                            radioCommsBuffer_t* buffer = WiSafe_EncodeReadVolts(msg->id, msg->model);
                            if (buffer != NULL)
                            {
                                /* Make the request to read the voltage. */
                                LOG_InfoC(LOG_MAGENTA "Issuing battery voltage read for device %06x.", msg->id);
                                WiSafe_RadioCommsTx(buffer);
                                notifyImmediately = false;
                            }
                            else
                            {
                                /* No free buffers to do a battery read, so just notify immediately. */
                                LOG_Warning("No buffers to read voltage, so reporting battery fault immediately for device %06x.", msg->id);
                                DeletePendingFault(pendingFault, true);
                                notifyImmediately = true;
                            }
                        }
                        else
                        {
                            /* No free buffers to do a pending fault, so just notify immediately. */
                            LOG_Warning("No buffers to do pending fault, so reporting battery fault immediately for device %06x.", msg->id);
                            notifyImmediately = true;
                        }
                    }
                    else
                    {
                        // Device does not support voltage reads
                        notifyImmediately = true;
                    }
                }
                else
                {
                    // Fault inactive
                    notifyImmediately = true;
                }

                /* Whatever path was followed through the code above, should we report the fault immediately? */
                if (notifyImmediately)
                {
                    // Translate flag to a fault code
                    uint8_t code = GetFaultFlagCode(fault_flag_no);
                    SendFaultEvent(msg->id, received_fault, code, false, 0);
                }

                break;
            }
            case FAULT_FLAG_AC_FAILED:
            {
                LOG_InfoC(LOG_MAGENTA "Fault flag set for FAULT_FLAG_AC_FAILED.");
                // The received AC (mains) fault flag is:
                // 0 = no fault
                // 1 = fault - mains power failed

                bool received_fault = fault_flag;

                // Translate flag to a fault code
                uint8_t code = GetFaultFlagCode(fault_flag_no);
                SendFaultEvent(msg->id, received_fault, code, false, 0);
                break;
            }
            case FAULT_FLAG_RM_BATTERY_FAULT:
            {
                LOG_InfoC(LOG_MAGENTA "Fault flag set for FAULT_FLAG_RM_BATTERY_FAULT.");
                // The received radio module battery fault flag is:
                // 0 = no fault
                // 1 = fault

                bool received_fault = fault_flag;

                // Translate flag to a fault code
                uint8_t code = GetFaultFlagCode(fault_flag_no);
                SendFaultEvent(msg->id, received_fault, code, false, 0);
                break;
            }
            case FAULT_FLAG_NOT_USED_1:
            case FAULT_FLAG_NOT_USED_2:
            default:
            {
                if (fault_flag == true)
                {
                    LOG_Error("device %u unexpected fault flag set: %i",
                              msg->id, fault_flag_no);
                }
                break;
            }
        }
    }
}

/**
 * Handle a timeout while processing a fault. A timeout would occur
 * when we request further information and don't receive it in a
 * timely manner.
 *
 * @param type timer fault or alarm timeout.
 * @param buffer The underlying buffer containing the timeout.
 */
void WiSafe_EventTimeout(timerType_e type, radioCommsBuffer_t* buffer)
{
    /* We didn't get a speedy reply to one of our requests for more
       information. This could have been for a fault (battery voltage)
       or for an alarm (temperature). We need to work out whether it was
       a fault or alarm, and then signal immediately (with no extra
       detail) because we shouldn't delay any longer. */

    /* Does the buffer correspond to a fault or an alarm? */
    if (type == timerType_FaultTimeout)
    {
        pendingFault_t* pendingFault = (pendingFault_t*)buffer;

        /* Handle pending fault timeout. */
        LOG_InfoC(LOG_MAGENTA "Fault timeout for device 0x%06x, so reporting fault anyway.", pendingFault->fault.id);

        // Translate flag to a fault code (only battery faults need
        // additional information).
        uint8_t code = GetFaultFlagCode(FAULT_FLAG_SD_BATTERY_FAULT);

        SendFaultEvent(pendingFault->fault.id, true, code, false, 0);

        DeletePendingFault(buffer, false);
    }
    else if (type == timerType_AlarmTimeout)
    {
        pendingAlarm_t* pendingAlarm = (pendingAlarm_t*)buffer;

        /* Handle pending alarm timeout. */
        LOG_InfoC(LOG_MAGENTA "Alarm timeout for device 0x%06x, so reporting alarm anyway.", pendingAlarm->alarm.id);

        uint8_t event_state;
        if (pendingAlarm->alarm.status < NUM_STATUS_CODES)
        {
            event_state = alarm_status_to_event_state[pendingAlarm->alarm.status];
        }
        else
        {
            // Status out of range. Assume the default.
            LOG_Warning("Alarm status out of range: %u",pendingAlarm->alarm.status);
            event_state = ALARM_STATE_ACTIVE_DEFAULT;
        }
        SendAlarmEvent(pendingAlarm->alarm.id, event_state, false, 0);

        DeletePendingAlarm(buffer, false);
    }
    else
    {
        LOG_Error("Unexpected fault timeout received that doesn't match a pending fault or pending alarm? Ignoring.");
    }
}

/**
 * Handle an Extended Diagnostic Variable Response message that we have
 * received over SPI.
 *
 * @param msg The buffer containing the SPI message.
 */
void WiSafe_ExtDiagVarRespReceived(msgExtDiagVarResp_t* msg)
{
    /* We have received a voltage or temperature reading (presumably for a fault
     * or alarm in our pending list), so find the pending message. */
    pendingFault_t* pendingFault = FindPendingFaultByDevice(msg->id);

    if (pendingFault != NULL)
    {
        LOG_InfoC(LOG_MAGENTA "Battery voltage received for device 0x%06x, voltage %04x, so reporting battery fault.",
                  msg->id, msg->variable);

        // Get the fault code and raise the fault.
        uint8_t code = GetFaultFlagCode(FAULT_FLAG_SD_BATTERY_FAULT);

        SendFaultEvent(pendingFault->fault.id, true, code, true, msg->variable);

        DeletePendingFault((radioCommsBuffer_t*)pendingFault, true);
    }
    else
    {
        pendingAlarm_t* pendingAlarm = FindPendingAlarmByDevice(msg->id);

        if (pendingAlarm != NULL)
        {
            LOG_InfoC(LOG_MAGENTA "Temperature received for device 0x%06x, temperature %04x, so reporting alarm.",
                      msg->id, msg->variable);
            uint8_t event_state;
            if (pendingAlarm->alarm.status < NUM_STATUS_CODES)
            {
                event_state = alarm_status_to_event_state[pendingAlarm->alarm.status];
            }
            else
            {
                // Status out of range. Assume the default.
                LOG_Warning("Alarm status out of range: %u",pendingAlarm->alarm.status);
                event_state = ALARM_STATE_ACTIVE_DEFAULT;
            }
            SendAlarmEvent(pendingAlarm->alarm.id, event_state, true, msg->variable);

            DeletePendingAlarm((radioCommsBuffer_t*)pendingAlarm, true);
        }
        else
        {
            LOG_Warning("Ext Diag Variable Resp received for unknown pending alarm on device 0x%06x, so ignoring.", msg->id);
        }
    }
}

/**
 * Handle a fault detail message that we have received over SPI.
 *
 * @param msg The buffer containing the SPI message.
 */
void WiSafe_FaultDetailReceived(msgFaultDetail_t* msg)
{
    // Find the stored fault message
    pendingFault_t* pendingFault = FindPendingFaultByDevice(msg->id);

    if (pendingFault != NULL)
    {
        /* Depending on the type of device, we interpret the extra detail
         * differently. */
        int32_t faultDetailsType = WiSafe_DiscoveryFaultDetailsType(msg->id);

        if (faultDetailsType)
        {
            LOG_InfoC(LOG_MAGENTA "Fault details received for device %06x, so sending fault.", msg->id);

            for (int fault_number = 0; fault_number < FAULT_MAP_LENGTH; fault_number++)
            {
                uint16_t fault_byte_mask = (1 << fault_number);
                bool state = ( msg->faults & fault_byte_mask);
                uint8_t code = GetFaultDetailsCode(faultDetailsType,
                                                   fault_number);
                SendFaultEvent(msg->id, state, code, false, 0);
            }
        }
        else
        {
            LOG_Error("Fault details received for device %06x, but device doesn't support fault detail. Ignoring.", msg->id);
        }
        DeletePendingFault((radioCommsBuffer_t*)pendingFault, true);
    }
    else
    {
        LOG_Warning("Unexpected fault detail received, possibly due to delay in the network. Ignoring.");
    }
}

/**
 * Handle an alarm message received over SPI.
 *
 * @param msg The buffer containing the SPI message.
 */
void WiSafe_AlarmReceived(msgAlarm_t* msg)
{
    LOG_InfoC(LOG_MAGENTA "Alarm received from device 0x%06x, priority 0x%02x, status 0x%02x, SID %d, seq %d.", msg->id, msg->priority, msg->status, msg->sid, msg->seq);

    /* For certain types of device we need to read further details to supply with the alarm notification. */
    bool notifyImmediately;
    if (WiSafe_DiscoveryIsFeatureSupported(msg->id, WISAFE_PROFILE_KEY_TEMPERATURE_ALARM))
    {
        /* For a temperature alarm we need to read the current temperature. */
        radioCommsBuffer_t* pendingAlarm = CreatePendingAlarm(msg);
        if (pendingAlarm != NULL)
        {
            deviceModel_t model = WiSafe_DiscoveryGetModelForDevice(msg->id);
            radioCommsBuffer_t* buffer = WiSafe_EncodeReadTemp(msg->id, model);
            if (buffer != NULL)
            {
                /* Make the request to read the fault details. */
                LOG_InfoC(LOG_MAGENTA "Issuing request temperature details for device %06x.", msg->id);
                WiSafe_RadioCommsTx(buffer);
                notifyImmediately = false;
            }
            else
            {
                /* No free buffers to do a fault details read, so just notify immediately with no details. */
                LOG_Warning("No buffers to read fault details, so reporting alarm immediately for device %06x.", msg->id);
                DeletePendingAlarm(pendingAlarm, true);
                notifyImmediately = true;
            }
        }
        else
        {
            /* No free buffers to do a pending alarm, so just notify immediately with no details. */
            LOG_Warning("No buffers to do pending alarm, so reporting alarm immediately for device %06x.", msg->id);
            notifyImmediately = true;
        }
    }
    else
    {
        notifyImmediately = true;
    }

    /* Whatever path was followed through the code above, should we report the alarm immediately with no details? */
    if (notifyImmediately)
    {
        /* Just report the alarm immediately. */
        uint8_t event_state;
        if (msg->status < NUM_STATUS_CODES)
        {
            event_state = alarm_status_to_event_state[msg->status];
        }
        else
        {
            // Status out of range. Assume the default.
            LOG_Warning("Alarm status out of range: %u",msg->status);
            event_state = ALARM_STATE_ACTIVE_DEFAULT;
        }
        LOG_InfoC(LOG_MAGENTA "Reporting alarm for device 0x%06x with no details.", msg->id);
        SendAlarmEvent(msg->id, event_state, false, 0);
    }
}

/**
 * Handle an Alarm Stop message received over SPI.
 *
 * @param msg The buffer containing the SPI message.
 */
void WiSafe_FaultAlarmStopReceived(msgAlarmStop_t* msg)
{
    LOG_InfoC(LOG_MAGENTA "Alarm STOP received from device 0x%06x.", msg->id);
    SendAlarmEvent(msg->id, ALARM_STATE_INACTIVE, false, 0);
}

/**
 * Handle an Hush message received over SPI.
 *
 * The hush command is received from a control device such as a LFS, SVP or
 * ACU. This will silence all sensors on the network (if they can be silenced)
 * Therefore if there are 4 sensors on the network that are in alarm mode, we
 * need to create an Alarm (silenced) event for each of these sensors. This
 * will be the same as the event created when an 0x50 Alarm is received with
 * status set to Silenced.
 *
 * @param msg The buffer containing the SPI message.
 */
void WiSafe_FaultHushReceived(msgHush_t* msg)
{
    LOG_InfoC(LOG_MAGENTA "Hush received from device 0x%06x.", msg->id);

    /* Check for any devices in alarm state. */
    deviceId_t buffer[DAL_MAXIMUM_NUMBER_WISAFE_DEVICES];
    uint16_t numDevices = 0;
    EnsoErrorCode_e enumerateError = WiSafe_DALDevicesEnumerate(buffer, DAL_MAXIMUM_NUMBER_WISAFE_DEVICES, &numDevices);
    if (enumerateError == eecNoError)
    {
        for (uint16_t loop = 0; loop < numDevices; loop += 1)
        {
            deviceId_t current = buffer[loop];
            /* Read the alarm state of the device. The alarm state property will
             * not exist for devices that have never been in alarm state so
             * an error is not a problem. */
            LOG_Info("Checking alarm state of device 0x%06x", current);
            EnsoErrorCode_e error;
            EnsoPropertyValue_u alarm_state = WiSafe_DALGetReportedProperty(current, DAL_PROPERTY_DEVICE_ALARM_STATE_IDX, &error);
            if (error == eecNoError)
            {
                LOG_Info("Device 0x%06x is in alarm state %u", current, alarm_state.uint32Value);
                if (alarm_state.uint32Value == ALARM_STATE_ACTIVE_DEFAULT)
                {
                    /* This alarm has been silenced. Send an event. */
                    LOG_Info("Device 0x%06x has been silenced", current);
                    SendAlarmEvent(current, ALARM_STATE_ACTIVE_SILENCED, false, 0);
                }
            }
        }
    }
    else
    {
        LOG_Error("Failed to enumerate devices while looking for devices in alarm.");
    }
}

/**
 * Handle a Locate message received over SPI.
 *
 * The locate command is generated when the button is pressed on a specific
 * sensor. The locate command contains the device id of the sensor. If it is a
 * sensor in alarm and the Originating field is set to 1 (Originating SD is in
 * local alarm) then we need to create an Alarm (silenced) event for that sensor.
 *
 * @param msg The buffer containing the SPI message.
 */
void WiSafe_FaultLocateReceived(msgLocate_t* msg)
{
    LOG_InfoC(LOG_MAGENTA "Locate received from device 0x%06x.", msg->id);

    EnsoErrorCode_e error = eecNoError;
    /* Read the alarm state of the device. The alarm state property will
     * not exist for devices that have never been in alarm state so
     * an error is not a problem. */
    LOG_Info("Checking alarm state of device 0x%06x", msg->id);
    EnsoPropertyValue_u alarm_state = WiSafe_DALGetReportedProperty(msg->id, DAL_PROPERTY_DEVICE_ALARM_STATE_IDX, &error);
    if (error == eecNoError)
    {
        LOG_Info("Device 0x%06x is in alarm state %u", msg->id, alarm_state.uint32Value);
        if (alarm_state.uint32Value == ALARM_STATE_ACTIVE_DEFAULT)
        {
            /* This alarm has been silenced. Send an event. */
            LOG_Info("Device 0x%06x has been silenced", msg->id);
            SendAlarmEvent(msg->id, ALARM_STATE_ACTIVE_SILENCED, false, 0);
        }
    }
}

/**
 * Handle an RM Fault message received over SPI.
 *
 * @param buf The buffer containing the SPI message.
 */
void WiSafe_FaultRMFaultReceived(radioCommsBuffer_t* buf)
{
    msgRMSDFault_t msg = WiSafe_DecodeRMSDFault(buf);
    LOG_InfoC(LOG_MAGENTA "RM SD Fault received from device 0x%06x.", msg.id);

    /* All we need to do is increment the fault count property. */
    WiSafe_DALIncrementReportedProperty(msg.id, DAL_PROPERTY_DEVICE_RM_SD_FAULT_IDX, DAL_PROPERTY_DEVICE_RM_SD_FAULT);
}

/**
 * Handle an Remote Status Report message received over SPI.
 *
 * @param buf The buffer containing the SPI message.
 */
void WiSafe_FaultRemoteStatusReportReceived(radioCommsBuffer_t* buf)
{
    msgRemoteStatusResult_t msg = WiSafe_DecodeRemoteStatusReport(buf);
    LOG_Info("Remote Status Report received from device 0x%06x.", msg.id);

    EnsoPropertyValue_u valueFC = { .uint32Value = msg.radioFaultCount };
    WiSafe_DALSetReportedProperty(msg.id, DAL_PROPERTY_DEVICE_RADIO_FAULT_COUNT_IDX, DAL_PROPERTY_DEVICE_RADIO_FAULT_COUNT, evUnsignedInt32, valueFC);

    EnsoPropertyValue_u valueL = { .uint32Value = msg.lastRSSI };
    WiSafe_DALSetReportedProperty(msg.id, DAL_PROPERTY_DEVICE_RADIO_RSSI_IDX, DAL_PROPERTY_DEVICE_RADIO_RSSI, evUnsignedInt32, valueL);

    /* Make the firmware a string type. */
    char buffer[3 + 1]; // 3 being the maximum length of a byte in decimal.
    sprintf(&(buffer[0]), "%u", msg.firmwareRevision);
    EnsoPropertyValue_u valueUnion;
    strncpy(valueUnion.stringValue, buffer, (LSD_STRING_PROPERTY_MAX_LENGTH - 1));

    WiSafe_DALSetReportedProperty(msg.id, PROP_WISAFE_ID, (propertyName_t)PROP_WISAFE_ID_CLOUD_NAME, evString, valueUnion);
}
