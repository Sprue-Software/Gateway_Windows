/*!****************************************************************************
*
* \file WiSafe_Discovery.c
*
* \author James Green
*
* \brief Handle discovery of other WiSafe devices.
*
* \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

//#include <unistd.h>
//#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/stat.h>
//#include <direct.h>
#include "OSAL_Api.h"
//#include <Windows.h>
#include "WiSafe_Discovery.h"
#include "WiSafe_Timer.h"
#include "WiSafe_Protocol.h"
#include "WiSafe_RadioComms.h"
#include "LOG_Api.h"
#include "WiSafe_DAL.h"
#include "KVP_Api.h"
#include "WiSafe_Main.h"
#include "WiSafe_Settings.h"

/* General constants. */
#define SID_MAP_BIT_COUNT 64 /*
 Number of bit positions in SID map. */
#define WISAFE_DEVICES_FILE "/var/config/wisafe_devices.json"
#define WISAFE_DEVICES_FILE_READ "/var/config/wisafe_devices_read"

#define REAL_TEST_MESSAGE           true       /*ABR added*/
#define SPOOF_TEST_MESSAGE          false      /*ABR added*/
#define MAXIMUM_INTERROGATE_COUNT   5          /*ABR added*/


/* Length of a wisafe_devices.json file. Example:

[ { "did" : 20712, "model" : 888, "priority" : 65, "sid" : 7 }, { "did" : 63713, "model" : 907, "priority" : 255, "sid" : 42 } ]

 * Each device requires 64 characters plus 4 characters for the array
 * square brackets and a null terminator.
*/
#define WISAFEDEVICESLENGTH ((DAL_MAXIMUM_NUMBER_WISAFE_DEVICES * 64) + 4 + 1)

static const uint32_t maxDeviceNameLength = 20;

/* Key/value pair constants. */
const char* WISAFE_PROFILE_KEY_NAME                   = "name";
const char* WISAFE_PROFILE_KEY_READ_VOLTS_ON_FAULT    = "read_volts_on_fault";
const char* WISAFE_PROFILE_KEY_FAULT_DETAILS_TYPE     = "fault_details_type";
const char* WISAFE_PROFILE_KEY_TEMPERATURE_ALARM      = "temperature_alarm";

/* Constants for Learn mode and SID map polling. */
static const uint32_t learnModeNetworkRefreshPeriod = 5; // How often in seconds to poll for new SID map entries when in Learn mode.

/* Timeout duration between interrogating unknown devices */
static const uint32_t interrogationPeriod = 60; /*ABR changed. was 40seconds*/

/* Variables for Learn mode and SID map polling . */
static bool inLearnMode = false; // Whether or not we are currently in Learn Mode.
static Timer_t learnModeTimeoutTimerRef = NULL; // A reference to the current timer instance for learn mode timeout.
static uint8_t currentSidMap[SID_MAP_BIT_COUNT / 8] = {0,}; // The SID map from the most recent WiSafe_DiscoveryProcessSIDMap().
static Timer_t networkRefreshTimerRef = NULL; // A reference to the current timer instance for network refresh.
static Timer_t interrogateTimerRef = NULL; // A reference to the current timer instance for interrogating umknown devices.

/* Constants for Testing for missing nodes. */
uint32_t testMissingNormalPeriod = 0; // How often in seconds to test for missing devices. - configurable via environment variable
static const uint32_t testMissingReplyTimeout = 60; // Timeout in seconds for response to SID update message.
static const uint32_t testMissingMaxDuration = (6 * 60); // Maximum duration of a missing node test - the test will abort regardless of progress after this time.
static const uint32_t testMissingRequiredNumberOfScans = 3; // How many scan replies we need to form a decision about who's in/out.
static const uint32_t testMissingUpdateDeletePeriod = 30; // At the end of a test, the time period between SID map updates to remove missing nodes.
static const uint32_t testMissingUpdateDeleteRequiredCount = 3; // At the end of a test, the count of SID map updates to remove missing nodes.

/* Variables for Testing for missing nodes. */
static bool testingForMissingDevices = false; // Whether or not we currently doing a scan for missing devices. If so we put Learn/refresh() on hold.
static bool testMissingDeleteMissingDevices = false; // Whether or not to delete missing devices at the end of the test.
static uint32_t testMissingRepliesReceived = 0; // In the current test, how many replies we have received.
static uint32_t outstandingMissingNodeReports = 0; // How many missing node reports we've requested, that have not yet been answered.
static uint32_t testMissingLastTimestamp = 0; // Time when we last sent a SID map update message, in seconds since epoch.
static Timer_t testMissingTimerRef = NULL; // A reference to the current timer instance for missing tests.
static uint32_t testMissingStartTimestamp = 0; // Time when we began the current test.
static uint32_t testMissingUpdateDeleteCount = 0; // At the end of a test, the current number of SID update messages that have been sent.
static uint8_t removalSidMap[SID_MAP_BIT_COUNT / 8] = {0,}; // Builds up a bit mask of missing nodes.
static bool testMissingRanOutOfTime = false; // Whether or not the current test ran out of time.
static bool rerunMissingNodeTestImmediately = false; // After finishing the current missing node test, should we start a new one immediately or after the standard period?
static uint32_t isFlushingAll = 0; // Whether or not we are currently in the middle of flushing (deleting) all devices. It is not of bool type because we need to do something 2 times (see code later).

/* Variables for reading EA2 wisafe devices file for existing devices. */
static bool WisafeDevicesFileChecked = false;
static char wisafeDevicesJson[WISAFEDEVICESLENGTH];

/* Variable to keep track of the current device being interrogated */
static uint8_t currentUnknownSid;
static uint8_t interrogateSidCount[SID_MAP_BIT_COUNT] = {0,};    /*ABR added*/

void WiSafe_DiscoveryInterrogateUnknownDevicesContinue(void);

/*
  The code in this module is responsible for keeping track of known
  WiSafe devices, and keeping the knowledge up-to-date over time as
  devices come and go. It works as follows:

  Initialisation:

  - Once at the start of day WiSafe_DiscoveryInit() is called; it does
    some initial set-up and then sets two timers to go off in the
    future.

    One timer triggers after a few seconds, it is designed to let the
    system settle and then do an initial retrieval of the SID map,
    calculates the unknown devices, and determines if we're on a
    network or not. See WiSafe_DiscoveryProcessSIDMap().

    The other timers triggers after a long delay and is responsible
    for periodic checking of devices that have gone away. See
    InitMissingNodeScan().

  New devices:

  - New devices are discovered when WiSafe test messages are received
    from a given device. See WiSafe_DiscoveryProcessDeviceTested().
    When a new device appears it is added to the shadow, and the
    unknown device count is decremented.

  Periodic checking for missing devices:

  - Periodic checking for missing devices are triggered from the timer
    mentioned above and invoke InitMissingNodeScan(), with
    testMissingDeleteMissingDevices set to true. This means that any
    devices identified as missing are deleted from the shadow.

  - InitMissingNodeScan() works by sending out the SID map that it
    thinks is correct (based on the last one it received), and
    examining update messages to see if any devices are flagged as
    missing, see WiSafe_DiscoveryMissingNodeReport().

    By using a timer repeatedly, 3 SID maps are sent out and a device
    is only considered missing if it is absent in all 3 replies. At
    the end of the 3 tests, WiSafe_DiscoveryMissingNodeReport() will
    look at all devices and mark them offline/online as
    appropriate. At this point, it also takes the opportunity to
    request the status of each device (see RequestRemoteStatus()), so
    that it can keep other details up-to-date in the shadow (see
    WiSafe_Fault::WiSafe_FaultRemoteStatusReportReceived).

    The 3 tests are paced such that they can't happen quicker than a
    certain rate. Since performing the test is quite power hungry, the
    test will complete early if any response has no devices missing
    (since a device must be missing in all 3 tests to be considered
    gone).

    WiSafe_DiscoveryMissingNodeReport() is overloaded and is used for
    another phase of the missing node test not just the initial test
    for missing devices, see later.

    When WiSafe_FaultRemoteStatusReportReceived() has performed all 3
    tests, it calls TestMissingUpdateMapAtEnd() to go on to the next
    phase, see below.

  - The next phase is to tell all other WiSafe devices to remove
    missing nodes from their SID maps, to complete removal of the
    devices from the network. This happens in
    TestMissingUpdateMapAtEnd() which is called repeatedly at the end
    of a test. Its first job is to send SID map updates to remove
    missing nodes. Replies come in through
    WiSafe_DiscoveryMissingNodeReport(), which as mentioned above is
    overloaded to cater for this task and an earlier
    task. WiSafe_DiscoveryMissingNodeReport() is aware of the power
    hungriness of the task and if it detects that retries are not
    necessary, will complete early and not do the 3 retries.

  - At the end of the SID map update phase to remove missing devices
    (whether it completed early, or reached the maximum number of
    retries), and if testMissingDeleteMissingDevices is set, it will
    delete missing devices from the shadow, rather than just mark them
    as offline.

  - Finally a new periodic missing device test is scheduled for the
    next time.

  Timeout:

  - The missing device test has a 6 minute limit, at which point it
    will abort immediately and not do any further work. Several of the
    functions contain a check at the beginning to see if the time
    limit has been reached. If so, they set testMissingRanOutOfTime to
    true, and then call TestMissingUpdateMapAtEnd() to tidy up and
    schedule the next missing node check.

  Mutual exclusivity:

  - testingForMissingDevices is used to prevent conflicting tasks such
    as the missing device test and the regular retrieval of the SID
    map.

  PUBLIC API

  WiSafe_DiscoveryTestMode:

  - This method is called via the control API and begins or stops a
    missing device test.

  WiSafe_DiscoveryLearnMode:

  - This function elnables or disables Learn mode. When Learn mode is
    enabled a simulated RM button press message is sent, and then the
    polling rate of the SID map is increased so that we spot more
    readily when we have joined a network. We also spot newly added
    devices more readily.

    We keep looking forever until we either spot we've joined, or the
    platform tells us to stop by calling this function with enabled =
    false. The platform can spot when we have joined a network by
    looking for deltas in the 'in network' property.

  WiSafe_DiscoveryLeave:

  - Ask the gateway to leave its WiSafe network.

  WiSafe_DiscoveryFlushAll:

  - Disband the WiSafe network. All devices leave, including ourself.

  WiSafe_DiscoveryFlush:

  - Remove a specific device from the WiSafe network.

*/

/**
 * Helper to issue a SID map request over SPI - used to poll the map.
 *
 */
static void Refresh(void)
{
    /* Put doing a refresh on hold if we are currently scanning for
     * missing devices - don't want to overwhelm the RM. */
    if (!testingForMissingDevices)
    {
        /* Request the SID map. */
        radioCommsBuffer_t* buffer = WiSafe_EncodeRequestSIDMap();
        if (buffer != NULL)
        {
            WiSafe_RadioCommsTx(buffer);
        }
        else
        {
            LOG_Error("No free buffers to send 'Request SID Map'.");
        }
    }
    else
    {
        LOG_InfoC(LOG_CYAN "Not doing regular 'Request SID Map' because in the middle of a missing scan.");
    }

    /* Set a timer to call us back to do another refresh in a while. */
    Handle_t type = (Handle_t)timerType_DiscoveryRetrieveSidMap;
    if (networkRefreshTimerRef)
    {
        // Delete the existing timer
        OSAL_DestroyTimer(networkRefreshTimerRef);
        networkRefreshTimerRef = NULL;
    }
    if (inLearnMode)
    {
        networkRefreshTimerRef = OSAL_NewTimer(WiSafe_TimerCallback, (learnModeNetworkRefreshPeriod * 1000), false, type);
    }
    else
    {
        networkRefreshTimerRef = OSAL_NewTimer(WiSafe_TimerCallback, (normalNetworkRefreshPeriod * 1000), false, type);
    }

    if (networkRefreshTimerRef == NULL)
    {
        LOG_Error("WiSafe failed to create network refresh timer (1), errno=%s.", strerror(errno));
    }
}

/**
 * Helper to determine whether or not the given bit is set.
 *
 * @param mapData SID map (assumed to contain 8 bytes)
 * @param pos The required bit position.
 *
 * @return True/false depending on whether or not the bit is set.
 */
static bool IsBitSet(uint8_t* mapData, uint32_t pos)
{
    uint32_t bytePos = (pos / 8);
    uint8_t data = mapData[bytePos];
    uint32_t bitPos = (pos % 8);
    return (data & (1 << bitPos));
}

/**
 * Helper to set the bit in the given map.
 *
 * @param mapData The map.
 * @param pos The bit position to set.
 */
static void SetBit(uint8_t* mapData, uint32_t pos)
{
    uint32_t bytePos = (pos / 8);
    uint32_t bitPos = (pos % 8);

    uint8_t data = mapData[bytePos];
    data |= (1 << bitPos);
    mapData[bytePos] = data;
}

/**
 * Format a SID map as a string of 1's and 0's. Returns the data in a
 * common string (to save memory) so is not thread safe.
 *
 * @param mapData SID map (assumed to contain 8 bytes)
 *
 * @return The string representing the SID map.
 */
static char* PrettySIDMap(uint8_t* mapData)
{
    static char result[SID_MAP_BIT_COUNT + 1];

    for (uint32_t loop = 0; loop < SID_MAP_BIT_COUNT; loop += 1)
    {
        result[loop] = IsBitSet(mapData, loop)?'1':'0';
    }

    result[SID_MAP_BIT_COUNT] = 0;
    return result;
}

/**
 * Helper to signal test mode has completed.
 *
 * @param success True is passed, false is failed.
 */
static void EndTestMode(bool success)
{
    EnsoPropertyValue_u valueResult = { .booleanValue = success };
    EnsoPropertyValue_u valueRunning = { .uint32Value = GatewayStateRunning };

    EnsoAgentSidePropertyId_t agentSideId[2] = {DAL_COMMAND_TEST_MODE_RESULT_IDX, PROP_STATE_ID};
    const char* cloudName[2] = {DAL_COMMAND_TEST_MODE_RESULT, PROP_STATE_CLOUD_NAME};
    EnsoValueType_e type[2] = {evBoolean, evUnsignedInt32};
    EnsoPropertyValue_u value[2] = {valueResult, valueRunning};

    WiSafe_DALSetReportedProperties(GATEWAY_DEVICE_ID, agentSideId, cloudName, type, value, 2, PROPERTY_PUBLIC, false, false);
}

/**
 * Find the device type based on wisafe model number.
 * If the model number is not recognised then the wisafe priority of the device
 * is used.
 *
 * @param requestedModel The wisafe model number.
 * @param requestedPriority The wisafe priority value.
 *
 * @return The enso device type
 */
uint32_t WiSafe_DiscoveryFindDeviceType(uint16_t model, uint8_t priority)
{
    LOG_Info("Model: %u Priority: %u", model, priority);
    // For an unknown device the priority is the device type.
    uint32_t deviceType = priority;
    switch (model)
    {
        case  743: deviceType = 134; break; // WSM-1 AC Smoke
        case  785: deviceType = 129; break; // WST-630 Smoke Alarm
        case  888: deviceType =  65; break; // W2-CO-10X CO Alarm
        case  907: deviceType = 255; break; // W2-SVP-630 Pad Strobe
        case  930: deviceType = 250; break; // W2-LFS-630 Low Frequency Sounder
        case 1022: deviceType = 199; break; // IFG100
        case 1052: deviceType = 200; break; // IFG200
        case 1041: deviceType = 130; break; // WHT-630 Heat Alarm
        case 1044: deviceType = 135; break; // WHM-1 AC Heat Alarm
        case 1093: deviceType = 196; break; // W2-TSL ACU
        case 1148: deviceType = 129; break; // ST-630-DE German Smoke Alarm
        case 1157: deviceType =   1; break; // WETA-10X Extreme Temperature Alarm
        case 1219: deviceType = 250; break; // W2-SVP-630 Low Frequency Sounder
        default:
        {
            // Unknown model. Use the priority field to identify the device.
            LOG_Warning("Unknown model: %u priority: %u", model, priority);
            break;
        }
    }
    LOG_Info("Device Type: %u %u", deviceType, deviceType | DAL_PROPERTY_DEVICE_TYPE_WISAFE_MASK);
    return deviceType | DAL_PROPERTY_DEVICE_TYPE_WISAFE_MASK;
}

/**
 * Find a profile (if any) for the given device based on enso device type.
 *
 * @param deviceType The enso device type.
 *
 * @return The profile KVP string, or NULL if no profile found.
 */
char* WiSafe_DiscoveryFindProfile(uint32_t deviceType)
{
    switch (deviceType & ~DAL_PROPERTY_DEVICE_TYPE_WISAFE_MASK)
    {
        case 1:   return "\"type\":1,   \"name\":\"ColdAlarm\",        \"read_volts_on_fault\":false, \"fault_details_type\":4, \"temperature_alarm\":true";
        case 65:  return "\"type\":65,  \"name\":\"COAlarm\",          \"read_volts_on_fault\":false, \"fault_details_type\":3";
        case 129: return "\"type\":129, \"name\":\"SmokeAlarm\",       \"read_volts_on_fault\":true,  \"fault_details_type\":1";
        case 130: return "\"type\":130, \"name\":\"HeatAlarm\",        \"read_volts_on_fault\":false, \"fault_details_type\":2";
        case 134: return "\"type\":134, \"name\":\"ACSmokeAlarm\",     \"read_volts_on_fault\":false, \"fault_details_type\":5";
        case 135: return "\"type\":135, \"name\":\"ACHeatAlarm\",      \"read_volts_on_fault\":false, \"fault_details_type\":6, \"temperature_alarm\":true";
        case 196: return "\"type\":196, \"name\":\"ACU\",              \"read_volts_on_fault\":false, \"fault_details_type\":0";
        case 199: return "\"type\":199, \"name\":\"interfacegateway\", \"read_volts_on_fault\":false, \"fault_details_type\":0"; // IFG100
        case 200: return "\"type\":200, \"name\":\"interfacegateway\", \"read_volts_on_fault\":false, \"fault_details_type\":0"; // IFG200
        case 250: return "\"type\":250, \"name\":\"LowFreqSounder\",   \"read_volts_on_fault\":false, \"fault_details_type\":8";
        case 255: return "\"type\":255, \"name\":\"PadStrobe\",        \"read_volts_on_fault\":false, \"fault_details_type\":7";
        default:
        {
            // Unknown device type.
            LOG_Warning("Unknown model: %u priority: %u / %u", deviceType);
            break;
        }
    }
    return NULL;
}

#if 0
/**
 * Helper to write a profile in to the shadow.
 *
 * @param name The key for the profile entry.
 * @param value The value for the profile.
 */
static void SetProfile(const char* name, const char* value)
{
    EnsoPropertyValue_u valueUnion;
    strncpy(valueUnion.stringValue, value, (LSD_STRING_PROPERTY_MAX_LENGTH - 1));
    WiSafe_DALSetReportedProperty(GATEWAY_DEVICE_ID, (propertyName_t)name, evString, valueUnion);
}

/**
 * Find a profile (if any) for the given device priority.
 *
 * @param requestedPriority The require priority.
 *
 * @return The profile KVP string, or NULL if no profile found.
 */
char* WiSafe_DiscoveryFindProfile(uint8_t requestedPriority)
{
    uint32_t index = 1;
    do
    {
        char profileName[1 + 10 /* number of characters in MAX int */ + 1];
        sprintf(profileName, "p%u", index);

        /* Read the profile. */
        EnsoErrorCode_e error;
        EnsoPropertyValue_u profileValue = WiSafe_DALGetReportedProperty(GATEWAY_DEVICE_ID, (propertyName_t)profileName, &error);
        if (error == eecNoError)
        {
            /* We have the profile string, see if this is for the requested priority. */
            char* profile = (char*)profileValue.stringValue;
            uint8_t priority = (uint8_t)KVP_GetInt((char*)"priority", profile);
            if (priority == requestedPriority)
            {
                return profile;
            }
        }
        else
        {
            /* Stop on error - presuming because we've reached the last profile. */
            break;
        }

        /* On to the next. */
        index += 1;
    } while(true);

    return NULL;
}
#endif

/**
 * Is the given feature (identified by key name) supported on the given device?
 *
 * @param id Device ID
 * @param key The key, ie. one of WISAFE_PROFILE_KEY_...
 *
 * @return True if supported, false in all other cases including device not found.
 */
bool WiSafe_DiscoveryIsFeatureSupported(deviceId_t id, const char* key)
{
    bool result = false;
    EnsoErrorCode_e error;
    EnsoPropertyValue_u type = WiSafe_DALGetReportedProperty(id, PROP_TYPE_ID, &error);
    if (error == eecNoError)
    {
        /* We have the device type, so find its profile. */
        char* profile = WiSafe_DiscoveryFindProfile(type.uint32Value);
        if (profile != NULL)
        {
            /* Now get the value of the key. */
            if (!KVP_GetBool((char*) key, profile, &result))
            {
                LOG_Trace("Feature not found in the profile so return false");
                result = false;
            }
        }
    }

    return result;
}

/**
 * Get the fault details type
 *
 * @param id Device Id
 *
 * @return fault details type or 0 if no fault details type is defined
 */
int32_t WiSafe_DiscoveryFaultDetailsType(deviceId_t id)
{
    int32_t result = 0;

    EnsoErrorCode_e error;
    EnsoPropertyValue_u type = WiSafe_DALGetReportedProperty(id, PROP_TYPE_ID, &error);
    if (error == eecNoError)
    {
            /* We have the device type so find its profile. */
            char* profile = WiSafe_DiscoveryFindProfile(type.uint32Value);
            if (profile != NULL)
            {
                /* Now get the value of the fault details type. */
                if (!KVP_GetInt("fault_details_type", profile, &result))
                {
                    LOG_Error("KVP_GetInt failed");
                }
            }
    }

    return result;
}

/**
 * Helper to signal to the platform that we've finished a flush all
 * operation.
 *
 */
static void FlushAllComplete(void)
{
    isFlushingAll = 0;

    EnsoErrorCode_e error;
    EnsoPropertyValue_u timestamp = WiSafe_DALGetDesiredProperty(GATEWAY_DEVICE_ID, DAL_COMMAND_DELETE_ALL_IDX, &error);
    if (error == eecNoError)
    {
        WiSafe_DALSetReportedProperty(GATEWAY_DEVICE_ID, DAL_COMMAND_DELETE_ALL_IDX, DAL_COMMAND_DELETE_ALL, evUnsignedInt32, timestamp);
    }
    else
    {
        LOG_Error("Unable to read timestamp for Delete All command.");
    }
}

/**
 * Begin a new test for missing nodes.
 *
 */
static void InitMissingNodeScan(void)
{
    /* Don't do anything if we're already in the middle of a scan. */
    if (testingForMissingDevices)
    {
        LOG_Warning("Ignoring request to begin missing node scan, because already in a scan.");
        return;
    }

    /* Initialise scan. First get the current time. */
    uint32_t currentTimestamp = OSAL_GetTimeInSecondsSinceEpoch();

    /* Init our variables. */
    testMissingRepliesReceived = 0;
    outstandingMissingNodeReports = 0;
    testMissingLastTimestamp = 0;
    testMissingUpdateDeleteCount = 0;
    testMissingRanOutOfTime = false;
    memset(&(removalSidMap[0]), 0xff, sizeof(removalSidMap));
    rerunMissingNodeTestImmediately = false;

    /* Don't do anything if we're not actually on the network. */
    EnsoErrorCode_e error;
    EnsoPropertyValue_u inNetwork = WiSafe_DALGetReportedProperty(GATEWAY_DEVICE_ID, DAL_PROPERTY_IN_NETWORK_IDX, &error);
    if ((error == eecNoError) && (inNetwork.booleanValue == true))
    {
        /* Don't do anything if we're not in the running state. */
        EnsoPropertyValue_u currentState = WiSafe_DALGetReportedProperty(GATEWAY_DEVICE_ID, PROP_STATE_ID, &error);
        if ((error == eecNoError) && (currentState.uint32Value == GatewayStateLearning))
        {
            LOG_InfoC(LOG_CYAN "Skipping periodic missing node scan - in learning state.");
        }
        else
        {
            /* Send out the update message over SPI. */
            radioCommsBuffer_t* buffer = WiSafe_EncodeUpdateSIDMap(currentSidMap);
            if (buffer != NULL)
            {
                LOG_InfoC(LOG_CYAN "Beginning missing node scan.");
                testingForMissingDevices = true;
                testMissingStartTimestamp = currentTimestamp;

                /* Update timestamp for when last SID update message was sent. */
                testMissingLastTimestamp = currentTimestamp;

                /* TX over SPI. */
                WiSafe_RadioCommsTx(buffer);
                outstandingMissingNodeReports += 1;

                /* Set a timer to timeout if we don't receive a reply to our message. */
                assert(testMissingReplyTimeout > testMissingMinimumTimeBetweenScans); // Seems a sensible place to check the constants are defined sensibly.
                Handle_t type = (Handle_t)timerType_DiscoveryTestMissingTimeout;
                if (testMissingTimerRef)
                {
                    // Delete the existing timer
                    OSAL_DestroyTimer(testMissingTimerRef);
                    testMissingTimerRef = NULL;
                }
                testMissingTimerRef = OSAL_NewTimer(WiSafe_TimerCallback, (testMissingReplyTimeout * 1000), false, type);

                if (testMissingTimerRef == NULL)
                {
                    LOG_Error("WiSafe failed to create test missing timer (1), errno=%s.", strerror(errno));
                }

                return;
            }
            else
            {
                LOG_Warning("Skipping periodic missing node scan - no free buffers to send 'Update SID Map'.");
            }
        }
    }
    else
    {
        LOG_InfoC(LOG_CYAN "Skipping periodic missing node scan - not in network.");

        /* If this was called as part of a flush all, we need so signal completion. */
        if (isFlushingAll > 0)
        {
            FlushAllComplete();
        }
    }

    /* At this point, either there has been a problem or we are not in a wisafe
       network. If we are not in a network then run the long timer otherwise
       run the short timer to try again soon. */
    uint32_t timeInterval = inNetwork.booleanValue ? testMissingMinimumTimeBetweenScans : testMissingNormalPeriod;
    Handle_t type = (Handle_t)timerType_DiscoveryTestMissingInit;
    if (testMissingTimerRef)
    {
        // Delete the existing timer
        OSAL_DestroyTimer(testMissingTimerRef);
        testMissingTimerRef = NULL;
    }
    testMissingTimerRef = OSAL_NewTimer(WiSafe_TimerCallback, (timeInterval * 1000), false, type);
    // G2-556 Function InitMissingNodeScan could be called with testMissingDeleteMissingDevices set to false (ie network test without delete during device registration) but when a timer of type timerType_DiscoveryTestMissingInit expires, testMissingDeleteMissingDevices is set to true which could result in devices being deleted.

    if (testMissingTimerRef == NULL)
    {
        LOG_Error("WiSafe failed to create test missing timer (2), errno=%s.", strerror(errno));
    }
}

/**
 * Using during the missing node test, this helper ends the current
 * missing node test, and schedules a new one in the future.
 *
 */
static void TestMissingUpdateMapAtEnd(void)
{
    /* Cancel any current timer. */
    if (testMissingTimerRef != NULL)
    {
        OSAL_DestroyTimer(testMissingTimerRef);
        testMissingTimerRef = NULL;
    }

    /* Set a helper indicating whether or not we have completed updates. */
    bool haveCompletedSidUpdates = (testMissingUpdateDeleteCount >= testMissingUpdateDeleteRequiredCount);

    /* Are we at the very end of the test? */
    if (testMissingRanOutOfTime || haveCompletedSidUpdates || !testMissingDeleteMissingDevices)
    {
        /* Were we asked to delete missing devices at the end? */
        if (testMissingDeleteMissingDevices && !testMissingRanOutOfTime)
        {
            /* Find any offline devices and delete them. */
            deviceId_t buffer[DAL_MAXIMUM_NUMBER_WISAFE_DEVICES];
            uint16_t numDevices = 0;
            EnsoErrorCode_e enumerateError = WiSafe_DALDevicesEnumerate(buffer, DAL_MAXIMUM_NUMBER_WISAFE_DEVICES, &numDevices);
            if (enumerateError == eecNoError)
            {
                for (uint16_t loop = 0; loop < numDevices; loop += 1)
                {
                    deviceId_t current = buffer[loop];

                    /* Don't do anything with the gateway itself. */
                    if (current != GATEWAY_DEVICE_ID)
                    {
                        /* Read the online property to determine its status. */
                        EnsoErrorCode_e error;
                        EnsoPropertyValue_u online = WiSafe_DALGetReportedProperty(current, PROP_ONLINE_ID, &error);
                        if (error == eecNoError)
                        {
                            /* See if this device is offline, and delete it if it is. */
                            if (online.uint32Value == ONLINE_STATUS_OFFLINE)
                            {
                                LOG_InfoC(LOG_CYAN "Deleting offline device %06x.", current);
                                WiSafe_DALDeleteDevice(current);
                            }
                        }
                        else
                        {
                            LOG_Error("Failed to read online status propery for device %06x, at end of missing node scan.", current);
                        }
                    }
                }
            }
            else
            {
                LOG_Error("Failed to enumerate devices while deleting offline devices.");
            }
        }

        /* See if this operation was as a result of a 'delete all'
           command from the platform. And if so notify the platform
           that we've finished. We actually go round this code twice,
           hence the use of a counter. The first time round we delete
           online devices, the second time round we delete missing
           devices too. */
        if (isFlushingAll > 0)
        {
            isFlushingAll -= 1;
            if (isFlushingAll == 0)
            {
                /* Get the value of the timestamp that the platform provided and report it back to confirm we've finished. */
                LOG_Info("Completed Delete All command.");
                FlushAllComplete();
            }
        }

        /* Schedule a brand new scan at the required point in the future. */
        LOG_InfoC(LOG_CYAN "End of missing node scan.");

        testMissingUpdateDeleteCount = 0;
        testingForMissingDevices = false;
        testMissingDeleteMissingDevices = false; // This is overriden by the code that handles the timerType_DiscoveryTestMissingInit timer, but feels sensible to set it to false for now.
        uint32_t delay = rerunMissingNodeTestImmediately?1:(testMissingNormalPeriod * 1000);
        Handle_t type = (Handle_t)timerType_DiscoveryTestMissingInit;

        if (testMissingTimerRef)
        {
            // Delete the existing timer
            OSAL_DestroyTimer(testMissingTimerRef);
            testMissingTimerRef = NULL;
        }
        testMissingTimerRef = OSAL_NewTimer(WiSafe_TimerCallback, delay, false, type);

        if (testMissingTimerRef == NULL)
        {
            LOG_Error("WiSafe failed to create test missing timer (3), errno=%s.", strerror(errno));
        }

        /* We have finished the scan so change our state back to running, and set the test result. */
        EndTestMode(true);
    }
    else
    {
        /* We have been instructed to delete offline devices at the end of a test, do so now. */
        assert(testMissingDeleteMissingDevices);

        /* Is this the first time we have got to this point, if so we need to initialise some things. */
        if (testMissingUpdateDeleteCount == 0)
        {
            LOG_InfoC(LOG_CYAN "Now sending updates to remove missing peers.");

            LOG_InfoC(LOG_CYAN "Current SID map: %s", PrettySIDMap(currentSidMap));
            LOG_InfoC(LOG_CYAN "Missing SID map: %s", PrettySIDMap(removalSidMap));

            /* Need to calculate the SID map of valid nodes that should be kept. This is simply the current mask
               minus the ones we have determined as missing. */
            bool anyNodesMissing = false;
            uint32_t nodesRemaining = 0;
            for (uint32_t bytePos = 0; bytePos < (SID_MAP_BIT_COUNT / 8); bytePos += 1)
            {
                uint8_t previousByteValue = currentSidMap[bytePos];
                removalSidMap[bytePos] = (currentSidMap[bytePos] & ~(removalSidMap[bytePos]));
                anyNodesMissing |= (previousByteValue != removalSidMap[bytePos]);

                for (uint32_t bitPos = 0; bitPos < 8; bitPos += 1)
                {
                    if (removalSidMap[bytePos] & (1 << bitPos))
                    {
                        nodesRemaining += 1;
                    }
                }
            }

            /* There is a special case that if after removing missing nodes, we're the only one left, then we should remove ourselves too. */
            if (nodesRemaining == 1)
            {
                LOG_InfoC(LOG_CYAN "Only us left in the SID map, so removing everyone.");
                memset(removalSidMap, 0, sizeof(removalSidMap));
            }

            LOG_InfoC(LOG_CYAN "New SID map    : %s", PrettySIDMap(removalSidMap));

            /* Need to make sure this is zero, so that WiSafe_DiscoveryMissingNodeReport() ignores replies generated by the updates we send. */
            outstandingMissingNodeReports = 0;

            /* If there are no nodes to remove, then skip sending the final SID update messages. */
            if (!anyNodesMissing)
            {
                LOG_InfoC(LOG_CYAN "No missing nodes, so skipping final SID update messages.");
                testMissingUpdateDeleteCount = testMissingUpdateDeleteRequiredCount; // Pretend we've done all the updates.
                TestMissingUpdateMapAtEnd();

                /* If this was called as part of a flush all, we need so signal completion. */
                if (isFlushingAll > 0)
                {
                    FlushAllComplete();
                }

                // G2-556 Should not return from here
                return;
            }
        }

        /* Send out the SID update to remove the missing devices. If
           buffer is NULL, we'll just try again when the timer calls
           back. */
        radioCommsBuffer_t* buffer = WiSafe_EncodeUpdateSIDMap(removalSidMap);
        if (buffer != NULL)
        {
            /* TX over SPI, replies come back through WiSafe_DiscoveryMissingNodeReport(). */
            LOG_InfoC(LOG_CYAN "Issuing updated SID map with node(s) removed: %s", PrettySIDMap(removalSidMap));
            WiSafe_RadioCommsTx(buffer);
            testMissingUpdateDeleteCount += 1;
        }

        /* Set a timer to call us back. */
        Handle_t type = (Handle_t)timerType_DiscoveryTestMissingUpdateMapAtEnd;
        if (testMissingTimerRef)
        {
            // Delete the existing timer
            OSAL_DestroyTimer(testMissingTimerRef);
            testMissingTimerRef = NULL;
        }
        testMissingTimerRef = OSAL_NewTimer(WiSafe_TimerCallback, (testMissingUpdateDeletePeriod * 1000), false, type);

        if (testMissingTimerRef == NULL)
        {
            LOG_Error("WiSafe failed to create test missing timer (4), errno=%s.", strerror(errno));
        }
    }
}

/**
 * Using during the missing node test, this routine is common to both
 * handling timeouts, and continuing in normal operation when we
 * receive replies to SID map updates.
 *
 */
static void TestMissingContinue(void)
{
    /* We didn't receive a reply to our SID update message, so send
       out another one, unless we've used up all our time. */

    /* See if our time is up? */
    uint32_t currentTimestamp = OSAL_GetTimeInSecondsSinceEpoch();
    if ((currentTimestamp - testMissingStartTimestamp) > testMissingMaxDuration)
    {
        /* Timed-out. */
        LOG_InfoC(LOG_CYAN "Missing node test has timed-out after %d seconds, stopping test.", (currentTimestamp - testMissingStartTimestamp));
        testMissingRanOutOfTime = true;
        TestMissingUpdateMapAtEnd();
    }
    else
    {
        /* We still have time for more scans, so issue another one. So
         send out the update message over SPI. */
        radioCommsBuffer_t* buffer = WiSafe_EncodeUpdateSIDMap(currentSidMap);
        Handle_t type = (Handle_t) timerType_DiscoveryTestMissingTimeout;
        if (buffer != NULL)
        {
            /* TX over SPI. */
            LOG_InfoC(LOG_CYAN "Issuing SID map update request.");
            testMissingLastTimestamp = currentTimestamp;
            WiSafe_RadioCommsTx(buffer);
            outstandingMissingNodeReports += 1;

            /* Set a timer to timeout if we don't receive a reply to our message. */
            if (testMissingTimerRef)
            {
                // Delete the existing timer
                OSAL_DestroyTimer(testMissingTimerRef);
                testMissingTimerRef = NULL;
            }
            testMissingTimerRef = OSAL_NewTimer(WiSafe_TimerCallback, (testMissingReplyTimeout * 1000), false, type);
        }
        else
        {
            /* Set a timer to try the failed buffer allocation again in a short while. */
            LOG_InfoC(LOG_CYAN "Busy waiting on free buffer inside SID map update request.");
            if (testMissingTimerRef)
            {
                // Delete the existing timer
                OSAL_DestroyTimer(testMissingTimerRef);
                testMissingTimerRef = NULL;
            }
            testMissingTimerRef = OSAL_NewTimer(WiSafe_TimerCallback, (1 * 1000), false, type);
        }

        if (testMissingTimerRef == NULL)
        {
            LOG_Error("WiSafe failed to create test missing timer (5), errno=%s.", strerror(errno));
        }
    }
}

/**
 * Using during the missing node test, this routine is called when we
 * timeout on a reply to our SID update message.
 *
 */
static void TestMissingTimeout(void)
{
    LOG_InfoC(LOG_CYAN "Missing node test has timed-out on reply to SID update.");
    TestMissingContinue();
}

/**
 * Helper to request the status of the given device.
 *
 * @param sid Device SID (note not ID).
 */
void RequestRemoteStatus(sid_t sid)
{
    radioCommsBuffer_t* statusBuffer = WiSafe_EncodeRequestRemoteStatus(sid);
    if (statusBuffer != NULL)
    {
        LOG_Info("Issuing remote status request to learn additional details of device with SID %d.", sid);
        WiSafe_RadioCommsTx(statusBuffer);
    }
    else
    {
        /* It's not the end of the world if we can't request the remote status, we just won't
           learn about things like radio fault counts etc. With this in mind, we don't bother
           with retries or the like. */
        LOG_Error("No free buffers, so can't request remote status of device with SID %d.", sid);
    }
}

/**
 * Helper to request the details of the given device.
 *
 * @param sid Device SID (note not ID).
 */
void RequestRemoteDetails(sid_t sid)
{
    radioCommsBuffer_t* statusBuffer = WiSafe_EncodeRequestRemoteIDDetails(sid);
    if (statusBuffer != NULL)
    {
        LOG_Info("Issuing remote details request to learn additional details of device with SID %d.", sid);
        WiSafe_RadioCommsTx(statusBuffer);
    }
}

/**
 * Using during the missing node test, process a missing node report
 * with the given map.
 *
 * @param mapData The missing node map.
 */
void WiSafe_DiscoveryMissingNodeReport(uint8_t* mapData)
{
    LOG_InfoC(LOG_CYAN "Received missing node map: %s", PrettySIDMap(mapData));

    /* See if this report/reply corresponds to a request we've issued. */
    if (outstandingMissingNodeReports > 0)
    {
        /* See if our time is up? */
        uint32_t currentTimestamp = OSAL_GetTimeInSecondsSinceEpoch();
        if ((currentTimestamp - testMissingStartTimestamp) > testMissingMaxDuration)
        {
            /* Timed-out. */
            LOG_InfoC(LOG_CYAN "Missing node test has timed-out after %d seconds, stopping test.", (currentTimestamp - testMissingStartTimestamp));
            testMissingRanOutOfTime = true;
            TestMissingUpdateMapAtEnd();
            return;
        }

        /* Update counts. */
        testMissingRepliesReceived += 1;
        outstandingMissingNodeReports -= 1;

        /* Cancel any pending timeouts. */
        if (testMissingTimerRef != NULL)
        {
            OSAL_DestroyTimer(testMissingTimerRef);
            testMissingTimerRef = NULL;
        }

        /* Combine this newly received mask with our ongoing mask
           (nodes are only counted as missing if they are reported in
           all reports that we received - ie. we repeatedly AND mask
           that we receive). */
        bool anyNodesMissing = false;
        for (uint32_t bytePos = 0; bytePos < (SID_MAP_BIT_COUNT / 8); bytePos += 1)
        {
            removalSidMap[bytePos] = (removalSidMap[bytePos] & mapData[bytePos]);
            anyNodesMissing |= (removalSidMap[bytePos] != 0);
        }

        /* See if we've now received enough SID map replies to form a
           decision on whether nodes have gone missing. We can do an
           optimisation here and end the scan early if we have no
           missing nodes. There is no point in requesting further maps
           if the current map is empty (because we AND them
           together). */
        if ((testMissingRepliesReceived >= testMissingRequiredNumberOfScans) || !anyNodesMissing)
        {
            if (!anyNodesMissing)
            {
                LOG_InfoC(LOG_CYAN "Ending missing node scan early as no missing nodes detected already.");
            }

            /* Iterate over all devices, and look for any which
               have been missing in all scans.  Then mark them as
               offline. */
            deviceId_t buffer[DAL_MAXIMUM_NUMBER_WISAFE_DEVICES];
            uint16_t numDevices = 0;
            EnsoErrorCode_e enumerateError = WiSafe_DALDevicesEnumerate(buffer, DAL_MAXIMUM_NUMBER_WISAFE_DEVICES, &numDevices);
            if (enumerateError == eecNoError)
            {
                for (uint16_t loop = 0; loop < numDevices; loop += 1)
                {
                    deviceId_t current = buffer[loop];

                    /* Look at all devices except the gateway itself. */
                    if (current != GATEWAY_DEVICE_ID)
                    {
                        EnsoErrorCode_e error;
                        EnsoPropertyValue_u sid = WiSafe_DALGetReportedProperty(current, DAL_PROPERTY_DEVICE_SID_IDX, &error);
                        if (error == eecNoError)
                        {
                            /* Is the device already marked as missing, maybe due to SID Map Update
                             * being received from an ACU? If so then the device won't show up in the
                             * Missing Node Report. The device missing property may not exist for
                             * this device so continue if there is an error. */
                            EnsoPropertyValue_u missing = WiSafe_DALGetReportedProperty(current, DAL_PROPERTY_DEVICE_MISSING_IDX, &error);
                            if (error != eecNoError)
                            {
                                missing.booleanValue = false;
                            }

                            /* See if this device has either been missing in all the scans
                             * or is already marked as missing. */
                            if (IsBitSet(removalSidMap, sid.uint32Value) ||
                                missing.booleanValue == true)
                            {
                                LOG_InfoC(LOG_CYAN "Scan has determined that device %06x is missing.", current);

                                /* Mark as offline. */
                                EnsoPropertyValue_u value = { .uint32Value = ONLINE_STATUS_OFFLINE };
                                WiSafe_DALSetReportedProperty(current, PROP_ONLINE_ID, (propertyName_t)PROP_ONLINE_CLOUD_NAME, evUnsignedInt32, value);
                            }
                            else
                            {
                                /* The device is not missing so mark it online. It may have been marked offline
                                 * by a previous round of missing node tests, because it was out of range, but it
                                 * has now come back in range. */
                                EnsoPropertyValue_u value = { .uint32Value = ONLINE_STATUS_ONLINE };
                                WiSafe_DALSetReportedProperty(current, PROP_ONLINE_ID, (propertyName_t)PROP_ONLINE_CLOUD_NAME, evUnsignedInt32, value);

                                /* Take this opportunity to request the status of the device. We do this here
                                   because this code only runs periodically and so it is a good place to keep
                                   our stats up-to-date without doing it too often. */
                                // RequestRemoteStatus(sid.uint32Value); // This has been disabled because the original code didn't do it, and there was a concern it might cause problems.
                            }
                        }
                        else
                        {
                            LOG_Error("Failed to get SID map position for device %06x, at end of missing node scan.", current);
                        }
                    }
                }
            }
            else
            {
                LOG_Error("Failed to enumerate devices while looking for missing devices.");
            }

            /* End of test. */
            TestMissingUpdateMapAtEnd();
        }
        else
        {
            /* We have received the reply to our SID update message,
               but still have some more to issue. There is a minimum
               period between updates which we must honour. */
            uint32_t timeSinceIssue = (currentTimestamp - testMissingLastTimestamp);
            if (timeSinceIssue >= testMissingMinimumTimeBetweenScans)
            {
                /* We can do the next one straight away. */
                TestMissingContinue();
            }
            else
            {
                /* Continue when the time is right. */
                Handle_t type = (Handle_t)timerType_DiscoveryTestMissingContinue;
                if (testMissingTimerRef)
                {
                    // Delete the existing timer
                    OSAL_DestroyTimer(testMissingTimerRef);
                    testMissingTimerRef = NULL;
                }
                testMissingTimerRef = OSAL_NewTimer(WiSafe_TimerCallback, ((testMissingMinimumTimeBetweenScans - timeSinceIssue) * 1000), false, type);

                if (testMissingTimerRef == NULL)
                {
                    LOG_Error("WiSafe failed to create test missing timer (6), errno=%s.", strerror(errno));
                }
            }
        }
    }

    /* Are we in the map update phase, ie. the code in TestMissingUpdateMapAtEnd()? If so
       we can use this missing node report to determine whether or not we can finish
       immediately, or whether we need to keep retrying. */
    else if (testMissingUpdateDeleteCount > 0)
    {
        /* See if missing node map is all zeros. If so, end the update early. */
        bool allZeros = true;
        for (uint32_t bytePos = 0; bytePos < (SID_MAP_BIT_COUNT / 8); bytePos += 1)
        {
            if (mapData[bytePos] != 0)
            {
                allZeros = false;
                break;
            }
        }

        if (allZeros)
        {
            /* End the update early by pretending we've done all the retries. */
            LOG_InfoC(LOG_CYAN "Ending SID update as updates have been accepted.");
            testMissingUpdateDeleteCount = testMissingUpdateDeleteRequiredCount;
            TestMissingUpdateMapAtEnd();
        }
    }

    else
    {
        LOG_InfoC(LOG_CYAN "Ignoring missing node report as it doesn't correspond to anything we want, map: %s", PrettySIDMap(mapData));
    }
}

/**
 * Helper to subscribe to property deltas for newly created devices.
 *
 * @param id The device ID.
 */
static void SubscribeToDeviceProperties(deviceId_t id)
{
    EnsoErrorCode_e error = WiSafe_DALSubscribeToDesiredProperty(id, DAL_PROPERTY_DEVICE_MUTE_IDX);
    if (error != eecNoError)
    {
        LOG_Error("Couldn't subscribe to DAL_PROPERTY_DEVICE_MUTE on new device 0x%06x, will not be able to mute sounder from platform.", id);
    }

    error = WiSafe_DALSubscribeToDesiredProperty(id, DAL_COMMAND_DEVICE_FLUSH_IDX);
    if (error != eecNoError)
    {
        LOG_Error("Couldn't subscribe to DAL_COMMAND_DEVICE_FLUSH on new device 0x%06x, will not be able to flush this device from platform.", id);
    }

    error = WiSafe_DALSubscribeToDesiredProperty(id, DAL_COMMAND_IDENTIFY_DEVICE_IDX);
    if (error != eecNoError)
    {
        LOG_Error("Couldn't subscribe to DAL_COMMAND_IDENTIFY_DEVICE_IDX on new device 0x%06x, will not be able to identify this device from platform.", id);
    }
}

/**
 * Open the wisafe devices file and copy the contents into an array
 *
 * @return True if the file was successfully read, otherwise false
 */
bool Wisafe_OpenDevicesFile(void)
{
    bool result = false;
    //Nishi
    int fd = open(WISAFE_DEVICES_FILE, 1);
    if (fd < 0)
    {
        LOG_Error("cannot open %s: %s", WISAFE_DEVICES_FILE, strerror(errno));
        return false;
    }

    ssize_t bytes_read = read(fd, ((char *)wisafeDevicesJson), WISAFEDEVICESLENGTH);

    close(fd);
    LOG_Info("read %i bytes", bytes_read);
    wisafeDevicesJson[bytes_read] = '\0';
    if (bytes_read)
    {
        LOG_Info("wisafe devices file bytes read = %i\n|%s|\n", bytes_read, wisafeDevicesJson);
        result = true;
    }
    return result;
}


/**
 * Initialise this module - must be called before any other function
 * in this module.
 *
 */
void WiSafe_DiscoveryInit(void)
{
    /* Set default gateway properties. */
    EnsoPropertyValue_u value = { .booleanValue = false };
    WiSafe_DALSetReportedProperty(GATEWAY_DEVICE_ID, DAL_PROPERTY_IN_NETWORK_IDX, DAL_PROPERTY_IN_NETWORK, evBoolean, value);

    WiSafe_DALSetReportedProperty(GATEWAY_DEVICE_ID, DAL_PROPERTY_INTERROGATING_IDX, DAL_PROPERTY_INTERROGATING, evBoolean, value);

    // G2-556 : do we need to subscribe to the in network property?

#if 0
    /* Set profiles for known devices (could do this from a config file in the future. */
    SetProfile("p1", "\"priority\":196, \"name\":\"ACU\",            \"read_volts_on_fault\":false, \"supports_fault_details\":false");
    SetProfile("p2", "\"priority\":135, \"name\":\"ACHeatAlarm\",    \"read_volts_on_fault\":false, \"supports_fault_details\":true, \"temperature_alarm\":true");
    SetProfile("p3", "\"priority\":134, \"name\":\"ACSmokeAlarm\",   \"read_volts_on_fault\":false, \"supports_fault_details\":true");
    SetProfile("p4", "\"priority\":250, \"name\":\"LowFreqSounder\", \"read_volts_on_fault\":false, \"supports_fault_details\":true");
    SetProfile("p5", "\"priority\":1,   \"name\":\"ColdAlarm\",      \"read_volts_on_fault\":false, \"supports_fault_details\":true, \"temperature_alarm\":true");
    SetProfile("p6", "\"priority\":65,  \"name\":\"COAlarm\",        \"read_volts_on_fault\":false, \"supports_fault_details\":true");
    SetProfile("p7", "\"priority\":129, \"name\":\"SmokeAlarm\",     \"read_volts_on_fault\":true,  \"supports_fault_details\":true");
    SetProfile("p8", "\"priority\":255, \"name\":\"PadStrobe\",      \"read_volts_on_fault\":false, \"supports_fault_details\":true");
    SetProfile("p9", "\"priority\":130, \"name\":\"HeatAlarm\",      \"read_volts_on_fault\":false, \"supports_fault_details\":true");
#endif

    /* Let the system settle, and then get the SID map after a short delay. */
    {
        Handle_t type = (Handle_t)timerType_DiscoveryRetrieveSidMap;
        networkRefreshTimerRef = OSAL_NewTimer(WiSafe_TimerCallback, (3 * 1000), false, type);

        if (networkRefreshTimerRef == NULL)
        {
            LOG_Error("WiSafe failed to create network refresh timer (2), errno=%s.", strerror(errno));
        }
    }

    /* Look for missing devices at regular intervals. */
    {
        Handle_t type = (Handle_t)timerType_DiscoveryTestMissingInit;
        testMissingTimerRef = OSAL_NewTimer(WiSafe_TimerCallback, (testMissingNormalPeriod * 1000), false, type);

        if (testMissingTimerRef == NULL)
        {
            LOG_Error("WiSafe failed to create test missing timer (7), errno=%s.", strerror(errno));
        }
    }

    /* Go through all devices that have been persisted in the shadow from last time, and subscribe to properties of interest. */
    deviceId_t buffer[DAL_MAXIMUM_NUMBER_WISAFE_DEVICES];
    uint16_t numDevices = 0;
    EnsoErrorCode_e enumerateError = WiSafe_DALDevicesEnumerate(buffer, DAL_MAXIMUM_NUMBER_WISAFE_DEVICES, &numDevices);
    if (enumerateError == eecNoError)
    {
        LOG_Info("%d devices have persisted in the shadow", numDevices);

        /* Set default values for certain properties in the shadow. */
        EnsoPropertyValue_u valueF = { .booleanValue = false };
        EnsoPropertyValue_u value0 = { .uint32Value = 0 };

        for (uint16_t loop = 0; loop < numDevices; loop += 1)
        {
            deviceId_t current = buffer[loop];

            WiSafe_DALSetReportedProperty(current, DAL_PROPERTY_DEVICE_FAULT_IDX, DAL_PROPERTY_DEVICE_FAULT, evBoolean, valueF);
            WiSafe_DALCreateProperty(current, DAL_PROPERTY_DEVICE_MUTE_IDX, DAL_PROPERTY_DEVICE_MUTE, evBoolean, valueF);
            WiSafe_DALSetReportedProperty(current, DAL_COMMAND_DEVICE_FLUSH_IDX, DAL_COMMAND_DEVICE_FLUSH, evBoolean, valueF);
            WiSafe_DALSetReportedProperty(current, DAL_COMMAND_IDENTIFY_DEVICE_IDX, DAL_COMMAND_IDENTIFY_DEVICE, evUnsignedInt32, value0);

            /* Subscribe to properties that platform can change */
            SubscribeToDeviceProperties(current);
        }
    }
    else
    {
        LOG_Error("Failed to enumerate devices while subscribing to existing devices.");
    }
}

/**
 * Called to handle triggered timers for the discovery module.
 *
 * @param type The timer type.
 */
void WiSafe_DiscoveryHandleTimer(timerType_e type)
{
    switch (type)
    {
        case timerType_DiscoveryRetrieveSidMap:
            Refresh();
            break;
        case timerType_DiscoveryTestMissingInit:
            LOG_InfoC(LOG_CYAN "Missing node scan timer triggered.");
            testMissingDeleteMissingDevices = true;
            InitMissingNodeScan();
            break;
        case timerType_DiscoveryTestMissingTimeout:
            TestMissingTimeout();
            break;
        case timerType_DiscoveryTestMissingContinue:
            TestMissingContinue();
            break;
        case timerType_DiscoveryTestMissingUpdateMapAtEnd:
            TestMissingUpdateMapAtEnd();
            break;
        case timerType_DiscoveryLearnModeTimeout:
            LOG_InfoC(LOG_CYAN "Learn-in mode timeout.");
            WiSafe_DiscoveryLearnMode(false, 0);
            break;
        case timerType_DiscoveryInterrogate:
            LOG_InfoC(LOG_CYAN "Interrogation timeout.");
            WiSafe_DiscoveryInterrogateUnknownDevicesContinue();
            break;
        default:
            LOG_Error("Unexpected timer type %d", type);
            assert(false); // Not a timer type we expected.
            break;
    }
}

/**
 * Create devices in local shadow for devices found in the wisafe devices file.
 *
 */
void WiSafe_ProcessWisafeDevicesFile(void)
{
    if (Wisafe_OpenDevicesFile())
    {
        int deviceCount = 0;
        char *string_start = wisafeDevicesJson;
        char * from = NULL;
        char * to   = NULL;
        while ( deviceCount < DAL_MAXIMUM_NUMBER_WISAFE_DEVICES &&
                GetArrayObject(string_start, &from, &to) &&
                from != NULL &&
                to != NULL )
        {
            // Found a json object
            LOG_Info("json object |%s|", from);
            bool fieldMissing = false;

            // Read the SID
            int32_t sid = 0;
            if (KVP_GetInt("sid", from, &sid))
            {
                LOG_Info("sid=%i\n", sid);
            }
            else
            {
                LOG_Error("Couldn't read sid from file: KVP_GetInt failed");
                fieldMissing = true;
            }

            // Read the Device Identity
            int32_t did = 0;
            if (KVP_GetInt("did", from, &did))
            {
                LOG_Info("did=%i\n", did);
            }
            else
            {
                LOG_Error("Couldn't read did from file: KVP_GetInt failed");
                fieldMissing = true;
            }

            // Read the Device Identity
            int32_t model = 0;
            if (KVP_GetInt("model", from, &model))
            {
                LOG_Info("model=%i\n", model);
            }
            else
            {
                LOG_Error("Couldn't read model from file: KVP_GetInt failed");
                fieldMissing = true;
            }

            // Read the Priority
            int32_t priority = 0;
            if (KVP_GetInt("priority", from, &priority))
            {
                LOG_Info("priority=%i\n", priority);
            }
            else
            {
                LOG_Error("Couldn't read priority from file: KVP_GetInt failed");
                fieldMissing = true;
            }

            if (!fieldMissing)
            {
                LOG_Error("NDI  Spoof Device Tested");
                // Spoof a device tested message so the device read from
                // file is created in local shadow.
                msgDeviceTested_t deviceTested =
                    {
                        .id = did,
                        .priority = priority,
                        .status = 0,
                        .model = model,
                        .sid = sid
                    };
                WiSafe_DiscoveryProcessDeviceTested(&deviceTested,
                                                    ONLINE_STATUS_UNKNOWN, REAL_TEST_MESSAGE);/*ABR changed*/
            }

            string_start = to + 1;
            from = NULL;
            to   = NULL;
        }
    }
    else
    {
        LOG_Error("Failed to open wisafe devices file");
    }
}

/**
 * Check if the wisafe devices file need to be processed.
 *
 */
void WiSafe_CheckWisafeDevicesFile(void)
{
    if (!WisafeDevicesFileChecked)
    {
     LOG_Info("Reading WiSafe_CheckWisafeDevicesFile Request");   
        // Request the RM's details
        radioCommsBuffer_t* rcbuffer = WiSafe_EncodeRMDiagnosticRequest();
        if (rcbuffer != NULL)
        {
            LOG_Info("Issuing RM Diagnostic Request");
            WiSafe_RadioCommsTx(rcbuffer);
        }

        LOG_Info("Reading Wisafe Devices File");
        WisafeDevicesFileChecked = true;

        struct stat st;
        int rv = stat(WISAFE_DEVICES_FILE, &st);
        if (rv >= 0)
        {
            // The wisafe devices file exists.
            // Check if it has already been read.

            rv = stat(WISAFE_DEVICES_FILE_READ, &st);
            if (rv < 0)
            {
                // It hasn't been read so process the file
                WiSafe_ProcessWisafeDevicesFile();
                // Now the file has been processed, mark it as read
                LOG_Info("Touching " WISAFE_DEVICES_FILE_READ);
                system("touch " WISAFE_DEVICES_FILE_READ);
            }
        }
    }
}

/**
 * Continue to interrogate any unknown devices.
 *
 */
void WiSafe_DiscoveryInterrogateUnknownDevicesContinue(void)
{
    EnsoErrorCode_e error;
    static int  NDI_cnt=0;
    EnsoPropertyValue_u inProgress = WiSafe_DALGetReportedProperty(GATEWAY_DEVICE_ID,
            DAL_PROPERTY_INTERROGATING_IDX, &error);

    if (error)
    {
        // If we can't read it then assume search not in progress
        LOG_Error("Cannot read interrogation in progress property");
        return;
    }
    if (false == inProgress.booleanValue)
    {
        // Search not currently in progress
        LOG_Info("Search not currently in progress"); // TODO: make a LOG_Trace
        return;
    }

    if (currentUnknownSid >= SID_MAP_BIT_COUNT)
    {
        LOG_Error("SID out of range");
        currentUnknownSid = 0;
    }

    uint8_t startSid = currentUnknownSid;
    bool continueSearch = true;
    uint8_t unknownSid = false;

    while (continueSearch)
    {
        // Search for a used SID in the SID Map
        if (IsBitSet(currentSidMap, currentUnknownSid))
        {
            /* See if this is an "unknown" device. */
            deviceId_t deviceId;
            bool alreadyExists = WiSafe_DALGetDeviceIdForSID(currentUnknownSid, &deviceId);
            // G2-556 It is very inefficient to call WiSafe_DALGetDeviceIdForSID in this loop as each call to WiSafe_DALGetDeviceIdForSID requires a call to the local shadow to get a list of devices which is then searched for a matching sid.
            if ((!alreadyExists) && (currentUnknownSid != gateWaySid))  //ABR bug fix
            {
                //ABR changed
                if(interrogateSidCount[currentUnknownSid]<MAXIMUM_INTERROGATE_COUNT)
                {
                    interrogateSidCount[currentUnknownSid]++;

                    LOG_Error(" NDI gateWaySid %d ",gateWaySid);
                    NDI_cnt++;

                    // Unknown device found - request its details
                    LOG_InfoC(LOG_CYAN " Fire angel Unknown device at SID position %d.", currentUnknownSid);
                    RequestRemoteDetails(currentUnknownSid);
                    // Wait for the reply before searching for more unknown devices
                    continueSearch = false;
                    unknownSid = true;
                    LOG_Error(" NDI NDI_cnt %d ",NDI_cnt);
                    /* Set a timer to await a response */
                    if (interrogateTimerRef != NULL)
                    {
                        OSAL_DestroyTimer(interrogateTimerRef);
                    }
                    Handle_t type = (Handle_t)timerType_DiscoveryInterrogate;
                    interrogateTimerRef = OSAL_NewTimer(WiSafe_TimerCallback, (interrogationPeriod * 1000), false, type);
                    if (interrogateTimerRef == NULL)
                    {
                        LOG_Error("WiSafe failed to create interrogation timeout");
                    }
                }    
            }
        }
        else
        {
            interrogateSidCount[currentUnknownSid] = 0; //ABR added. Counter cleared if node is removed so when new node joins with this SID, it can be interrogated.
        }
        // Try the next SID next time
        currentUnknownSid += 1;
        if (currentUnknownSid >= SID_MAP_BIT_COUNT)
        {
            // Reached highest SID. Start again at 0
            LOG_Error(" NDI tart again at 0");
            currentUnknownSid = 0;
        }
        if (currentUnknownSid == startSid)
        {
            // We've checked all SIDs
            LOG_Error(" NDI tart again at continueSearch false");
            continueSearch = false;
        }
    }
    // If we requested details from a device then set the interrogation in progress property.
    EnsoPropertyValue_u value = { .booleanValue = unknownSid > 0 };
    LOG_Info("Setting %s to %i", DAL_PROPERTY_INTERROGATING, value.booleanValue);
    WiSafe_DALSetReportedProperty(GATEWAY_DEVICE_ID,
                                  DAL_PROPERTY_INTERROGATING_IDX,
                                  DAL_PROPERTY_INTERROGATING,
                                  evBoolean,
                                  value);
}

/**
 * Start to interrogate any unknown devices.
 *
 */
void WiSafe_DiscoveryInterrogateUnknownDevicesCheck(uint32_t unknownDevices)
{
    EnsoErrorCode_e error;
    if (unknownDevices)
    {
        EnsoPropertyValue_u inProgress = WiSafe_DALGetReportedProperty(GATEWAY_DEVICE_ID,
                DAL_PROPERTY_INTERROGATING_IDX, &error);

        if (error)
        {
            // If we can't read it then start a new search
            LOG_Error("Cannot read interrogation in progress property");
        }
        else if (inProgress.booleanValue == true)
        {
            //TODO: should be a LOG_Trace?
            LOG_Info("Interrogation already in progress");
            return;
        }
          //LOG_Info("NDI WiSafe_DiscoveryInterrogateUnknownDevicesCheck progress");
        // Start a search for unknown devices and ask them to identify themselves
        currentUnknownSid = 0;
        // Unknown devices - start searching
        EnsoPropertyValue_u value = { .booleanValue = true };
        LOG_Info("Setting %s to %i", DAL_PROPERTY_INTERROGATING, value.booleanValue);
        error = WiSafe_DALSetReportedProperty(GATEWAY_DEVICE_ID,
                                              DAL_PROPERTY_INTERROGATING_IDX,
                                              DAL_PROPERTY_INTERROGATING,
                                              evBoolean,
                                              value);
        if (error)
        {
            LOG_Error("Could not reset interrogation property");
        }
         LOG_Error("Call WiSafe_DiscoveryInterrogateUnknownDevicesContinue ");
        WiSafe_DiscoveryInterrogateUnknownDevicesContinue();
    }
    else
    {
        LOG_Error("No unknown devices - stop searching");
        // No unknown devices - stop searching
        EnsoPropertyValue_u value = { .booleanValue = false };
        LOG_Info("Setting %s to %i", DAL_PROPERTY_INTERROGATING, value.booleanValue);
        error = WiSafe_DALSetReportedProperty(GATEWAY_DEVICE_ID,
                                              DAL_PROPERTY_INTERROGATING_IDX,
                                              DAL_PROPERTY_INTERROGATING,
                                              evBoolean,
                                              value);
        if (error)
        {
            LOG_Error("Could not reset interrogation property")
        }
    }
}

/**
 * Process SID maps received over SPI.
 *
 * @param mapData
 */
void WiSafe_DiscoveryProcessSIDMap(uint8_t* mapData)
{
    LOG_InfoC(LOG_CYAN "NDI Received SID map: %s", PrettySIDMap(mapData));

    /* Go through the bits seeing which are set. */
    uint32_t bitsSet = 0;
    uint8_t unknownSids = 0;
    for (uint32_t sid = 0; sid < SID_MAP_BIT_COUNT; sid += 1)        
    {
        if (IsBitSet(mapData, sid))
        {
            /* See if this is an "unknown" device. */
            deviceId_t deviceId;
            bool alreadyExists = WiSafe_DALGetDeviceIdForSID((uint8_t)sid, &deviceId);
            // G2-556 It is very inefficient to call WiSafe_DALGetDeviceIdForSID in this loop as each call to WiSafe_DALGetDeviceIdForSID requires a call to the local shadow to get a list of devices which is then searched for a matching sid.
            if (!alreadyExists)
            {
                if ((uint8_t)sid != gateWaySid)
                {
                    LOG_InfoC(LOG_CYAN " NDI Unknown device at SID position %d.", sid);
                }
                unknownSids += 1;
            }

            /* Note that bits were set, so we can later decide if we we're in a network or not. */
            bitsSet += 1;
        }
    }

    /* If any bits were set, we must be in a network. */
    {
      //  LOG_Info("NDI  ABR Fixes");
        EnsoPropertyValue_u value = { .booleanValue = (bitsSet > 1) };
        WiSafe_DALSetReportedProperty(GATEWAY_DEVICE_ID, DAL_PROPERTY_IN_NETWORK_IDX, DAL_PROPERTY_IN_NETWORK, evBoolean, value);
        /* ABR added to send 0xD1 in case missed because of crash */
        // Request the RM's details
        if ((bitsSet > 1) && (64 == gateWaySid))   // Default unlearnt SID
        {    
            LOG_Info("NDI Issuing RM Diagnostic Request. gateWaySid = %d.", gateWaySid);
            radioCommsBuffer_t* rcbuffer = WiSafe_EncodeRMDiagnosticRequest();
            if (rcbuffer != NULL)
            {
                LOG_Info("Issuing RM Diagnostic Request. gateWaySid = %d.", gateWaySid);
                WiSafe_RadioCommsTx(rcbuffer);
            }
        }
    }

    /* Update unknown SIDs property in shadow. */
    uint32_t unknownReported = 0;
    if (unknownSids > 0)
    {
        unknownReported = (unknownSids - 1U /* gateway */);
    }

    {
        EnsoPropertyValue_u value = { .uint32Value = unknownReported };
        WiSafe_DALSetReportedProperty(GATEWAY_DEVICE_ID, DAL_PROPERTY_UNKNOWN_SIDS_IDX, DAL_PROPERTY_UNKNOWN_SIDS, evUnsignedInt32, value);
    }

    /* Remember the map. */
    memcpy(currentSidMap, mapData, sizeof(currentSidMap));

    // Check if there are any known devices in the wisafe devices file
    WiSafe_CheckWisafeDevicesFile();
    
    // Interrogate any unknown devices    
    WiSafe_DiscoveryInterrogateUnknownDevicesCheck(unknownReported);
}

/**
 * Process a 'request remote id results' message.
 *
 * @param msg Buffer containing  message
 */
void WiSafe_DiscoveryRemoteStatusReceived(msgRequestRemoteIdResult_t* msg)
{   
    // Spoof a device tested message so the device is created in local shadow.
    msgDeviceTested_t deviceTested =
    {
        .id = msg->id,
        .priority = msg->priority,
        .status = 0,
        .model = msg->model,
        .sid = msg->sid
    };
    // AS Per Issue: No Need to generate Device tested message: 
    WiSafe_DiscoveryProcessDeviceTested(&deviceTested, ONLINE_STATUS_ONLINE, SPOOF_TEST_MESSAGE);/*ABR changed*/

    // Continue searching for identifying devices
    //WiSafe_DiscoveryInterrogateUnknownDevicesContinue();
}

/**
 * Process a 'device tested' message.
 *
 * @param msg Buffer containing device tested message
 * @param onlineStatus Online Status of the device
 */

#if 1   /*ABR added*/
void WiSafe_DiscoveryProcessDeviceTested(msgDeviceTested_t* msg,
                                         onlineStatus_e onlineStatus, bool realTestMsg)
{
    /* We've received a device tested message. First check that it is
       one that corresponds to something in our SID map. */
    if (msg->sid < SID_MAP_BIT_COUNT)
    {
        /* See if the SID is in our existing map. It might not be in
         * the map if it wasn't there in our last scan, but had just
         * appeared. The platform expects us to report an unknown
         * device in this situation, so we must ignore the test
         * message that we have received. */
        if (!IsBitSet(currentSidMap, msg->sid))
        {
            LOG_Warning("Device tested message received for SID that is not in our map (%d) - ignoring message.", msg->sid);
        }
        else
        {
            /* It is for one of our devices, so remember the details. */
            LOG_InfoC(LOG_CYAN "Device tested message received for SID (%d).", msg->sid);

            /* Is it in the shadow already, if not, it reduces the unknown node count. */
            bool alreadyKnown = WiSafe_DALIsDeviceRegistered(msg->id);
            if (!alreadyKnown)
            {
                /* Decrease unknown device count. */
                EnsoErrorCode_e error;
                EnsoPropertyValue_u unknownCount = WiSafe_DALGetReportedProperty(GATEWAY_DEVICE_ID, DAL_PROPERTY_UNKNOWN_SIDS_IDX, &error);
                if (error == eecNoError)
                {
                    if (unknownCount.uint32Value > 0)
                    {
                        EnsoPropertyValue_u value = { .uint32Value = (unknownCount.uint32Value - 1) };
                        WiSafe_DALSetReportedProperty(GATEWAY_DEVICE_ID, DAL_PROPERTY_UNKNOWN_SIDS_IDX, DAL_PROPERTY_UNKNOWN_SIDS, evUnsignedInt32, value);
                   LOG_Error("NDI WiSafe_DALSetReportedProperty ");
                    }
                    else
                    {
                        LOG_Error("Inconsistent unknown SIDs count, would be reducing it below 0?");
                    }
                }
                else
                {
                    LOG_Error("Failed to update unknown SIDs for received test message.");
                }

                /* First time registration. */
                LOG_InfoC(LOG_CYAN "First time device registration for SID %d, device %06x.", msg->sid, msg->id);

                // Find the device type based on the model or priority
                uint32_t deviceType = WiSafe_DiscoveryFindDeviceType(msg->model, msg->priority);
                LOG_Info("Device type: %u", deviceType);

                /* See if we have a profile for this priority. */
                char* profile = WiSafe_DiscoveryFindProfile(deviceType);
                if (NULL == profile)
                {
                    // Unknown device.
                    LOG_Warning("NDI Unknown device - model: %u priority: %u", msg->model, msg->priority);
                }

                WiSafe_DALRegisterDevice(msg->id, deviceType);

                /* Set default values for certain properties in the shadow. */
                EnsoPropertyValue_u valueF = { .booleanValue = false };
                EnsoPropertyValue_u value0 = { .uint32Value = 0 };
                WiSafe_DALSetReportedProperty(msg->id, DAL_PROPERTY_DEVICE_FAULT_IDX, DAL_PROPERTY_DEVICE_FAULT, evBoolean, valueF);
                WiSafe_DALSetReportedProperty(msg->id, DAL_PROPERTY_DEVICE_MUTE_IDX, DAL_PROPERTY_DEVICE_MUTE, evBoolean, valueF);
                WiSafe_DALSetReportedProperty(msg->id, DAL_COMMAND_DEVICE_FLUSH_IDX, DAL_COMMAND_DEVICE_FLUSH, evBoolean, valueF);
                WiSafe_DALSetReportedProperty(msg->id, DAL_COMMAND_IDENTIFY_DEVICE_IDX, DAL_COMMAND_IDENTIFY_DEVICE, evUnsignedInt32, value0);

                /* Subscribe to properties that the gateway can change. */
                SubscribeToDeviceProperties(msg->id);

                /* Request remote status so that we can discover fault counts etc. */
                // RequestRemoteStatus(msg->sid); // This has been disabled because the original code didn't do it, and there was a concern it might cause problems.

                /* Update shadow. */
                EnsoPropertyValue_u valueMf;
                strncpy(valueMf.stringValue, "Sprue", (LSD_STRING_PROPERTY_MAX_LENGTH - 1));
                WiSafe_DALSetReportedProperty(msg->id, PROP_MANUFACTURER_ID, (propertyName_t)PROP_MANUFACTURER_CLOUD_NAME, evString, valueMf);

                EnsoPropertyValue_u valueM = { .uint32Value = msg->model };
                WiSafe_DALSetReportedProperty(msg->id, DAL_PROPERTY_DEVICE_MODEL_IDX, DAL_PROPERTY_DEVICE_MODEL, evUnsignedInt32, valueM);

                /* String version of the model. */
                EnsoPropertyValue_u valueModel;
                snprintf(valueModel.stringValue, LSD_STRING_PROPERTY_MAX_LENGTH, "%u", msg->model);
                valueModel.stringValue[LSD_STRING_PROPERTY_MAX_LENGTH -1] = '\0'; // Ensure null terminated
                WiSafe_DALSetReportedProperty(msg->id, PROP_MODEL_ID, (propertyName_t)PROP_MODEL_CLOUD_NAME, evString, valueModel);

                EnsoPropertyValue_u valueT  = { .uint32Value =  onlineStatus};
                WiSafe_DALSetReportedProperty(msg->id, PROP_ONLINE_ID, (propertyName_t)PROP_ONLINE_CLOUD_NAME, evUnsignedInt32, valueT);

                EnsoPropertyValue_u valueS = { .uint32Value = msg->sid };
                WiSafe_DALSetReportedProperty(msg->id, DAL_PROPERTY_DEVICE_SID_IDX, DAL_PROPERTY_DEVICE_SID, evUnsignedInt32, valueS);

                uint32_t epocTime = OSAL_GetTimeInSecondsSinceEpoch();
                EnsoPropertyValue_u valueTS = { .uint32Value = epocTime };
                WiSafe_DALSetReportedProperty(msg->id, DAL_PROPERTY_DEVICE_TEST_TIMESTAMP_IDX, DAL_PROPERTY_DEVICE_TEST_TIMESTAMP, evUnsignedInt32, valueTS);
            }
            else
            {
                if(realTestMsg)
                {
                            /* Update shadow. */
                    EnsoPropertyValue_u valueMf;
                    strncpy(valueMf.stringValue, "Sprue", (LSD_STRING_PROPERTY_MAX_LENGTH - 1));
                    WiSafe_DALSetReportedProperty(msg->id, PROP_MANUFACTURER_ID, (propertyName_t)PROP_MANUFACTURER_CLOUD_NAME, evString, valueMf);

                    EnsoPropertyValue_u valueM = { .uint32Value = msg->model };
                    WiSafe_DALSetReportedProperty(msg->id, DAL_PROPERTY_DEVICE_MODEL_IDX, DAL_PROPERTY_DEVICE_MODEL, evUnsignedInt32, valueM);

                    /* String version of the model. */
                    EnsoPropertyValue_u valueModel;
                    snprintf(valueModel.stringValue, LSD_STRING_PROPERTY_MAX_LENGTH, "%u", msg->model);
                    valueModel.stringValue[LSD_STRING_PROPERTY_MAX_LENGTH -1] = '\0'; // Ensure null terminated
                    WiSafe_DALSetReportedProperty(msg->id, PROP_MODEL_ID, (propertyName_t)PROP_MODEL_CLOUD_NAME, evString, valueModel);

                    EnsoPropertyValue_u valueT  = { .uint32Value =  onlineStatus};
                    WiSafe_DALSetReportedProperty(msg->id, PROP_ONLINE_ID, (propertyName_t)PROP_ONLINE_CLOUD_NAME, evUnsignedInt32, valueT);

                    EnsoPropertyValue_u valueS = { .uint32Value = msg->sid };
                    WiSafe_DALSetReportedProperty(msg->id, DAL_PROPERTY_DEVICE_SID_IDX, DAL_PROPERTY_DEVICE_SID, evUnsignedInt32, valueS);

                    uint32_t epocTime = OSAL_GetTimeInSecondsSinceEpoch();
                    EnsoPropertyValue_u valueTS = { .uint32Value = epocTime };
                    WiSafe_DALSetReportedProperty(msg->id, DAL_PROPERTY_DEVICE_TEST_TIMESTAMP_IDX, DAL_PROPERTY_DEVICE_TEST_TIMESTAMP, evUnsignedInt32, valueTS);
                }
            }
        }
    }
    else
    {
        LOG_Error("Device tested message received with bad SID (%d).", msg->sid);
    }
}

#else

void WiSafe_DiscoveryProcessDeviceTested(msgDeviceTested_t* msg,
                                         onlineStatus_e onlineStatus)
{
    /* We've received a device tested message. First check that it is
       one that corresponds to something in our SID map. */
    if (msg->sid < SID_MAP_BIT_COUNT)
    {
        /* See if the SID is in our existing map. It might not be in
         * the map if it wasn't there in our last scan, but had just
         * appeared. The platform expects us to report an unknown
         * device in this situation, so we must ignore the test
         * message that we have received. */
        if (!IsBitSet(currentSidMap, msg->sid))
        {
            LOG_Warning("Device tested message received for SID that is not in our map (%d) - ignoring message.", msg->sid);
        }
        else
        {
            /* It is for one of our devices, so remember the details. */
            LOG_InfoC(LOG_CYAN "Device tested message received for SID (%d).", msg->sid);

            /* Is it in the shadow already, if not, it reduces the unknown node count. */
            bool alreadyKnown = WiSafe_DALIsDeviceRegistered(msg->id);
            if (!alreadyKnown)
            {
                /* Decrease unknown device count. */
                EnsoErrorCode_e error;
                EnsoPropertyValue_u unknownCount = WiSafe_DALGetReportedProperty(GATEWAY_DEVICE_ID, DAL_PROPERTY_UNKNOWN_SIDS_IDX, &error);
                if (error == eecNoError)
                {
                    if (unknownCount.uint32Value > 0)
                    {
                        EnsoPropertyValue_u value = { .uint32Value = (unknownCount.uint32Value - 1) };
                        WiSafe_DALSetReportedProperty(GATEWAY_DEVICE_ID, DAL_PROPERTY_UNKNOWN_SIDS_IDX, DAL_PROPERTY_UNKNOWN_SIDS, evUnsignedInt32, value);
                   LOG_Error("NDI WiSafe_DALSetReportedProperty ");
                    }
                    else
                    {
                        LOG_Error("Inconsistent unknown SIDs count, would be reducing it below 0?");
                    }
                }
                else
                {
                    LOG_Error("Failed to update unknown SIDs for received test message.");
                }

                /* First time registration. */
                LOG_InfoC(LOG_CYAN "First time device registration for SID %d, device %06x.", msg->sid, msg->id);

                // Find the device type based on the model or priority
                uint32_t deviceType = WiSafe_DiscoveryFindDeviceType(msg->model, msg->priority);
                LOG_Info("Device type: %u", deviceType);

                /* See if we have a profile for this priority. */
                char* profile = WiSafe_DiscoveryFindProfile(deviceType);
                if (NULL == profile)
                {
                    // Unknown device.
                    LOG_Warning("NDI Unknown device - model: %u priority: %u", msg->model, msg->priority);
                }

                WiSafe_DALRegisterDevice(msg->id, deviceType);

                /* Set default values for certain properties in the shadow. */
                EnsoPropertyValue_u valueF = { .booleanValue = false };
                EnsoPropertyValue_u value0 = { .uint32Value = 0 };
                WiSafe_DALSetReportedProperty(msg->id, DAL_PROPERTY_DEVICE_FAULT_IDX, DAL_PROPERTY_DEVICE_FAULT, evBoolean, valueF);
                WiSafe_DALSetReportedProperty(msg->id, DAL_PROPERTY_DEVICE_MUTE_IDX, DAL_PROPERTY_DEVICE_MUTE, evBoolean, valueF);
                WiSafe_DALSetReportedProperty(msg->id, DAL_COMMAND_DEVICE_FLUSH_IDX, DAL_COMMAND_DEVICE_FLUSH, evBoolean, valueF);
                WiSafe_DALSetReportedProperty(msg->id, DAL_COMMAND_IDENTIFY_DEVICE_IDX, DAL_COMMAND_IDENTIFY_DEVICE, evUnsignedInt32, value0);

                /* Subscribe to properties that the gateway can change. */
                SubscribeToDeviceProperties(msg->id);

                /* Request remote status so that we can discover fault counts etc. */
                // RequestRemoteStatus(msg->sid); // This has been disabled because the original code didn't do it, and there was a concern it might cause problems.
            }

            /* Update shadow. */
            EnsoPropertyValue_u valueMf;
            strncpy(valueMf.stringValue, "Sprue", (LSD_STRING_PROPERTY_MAX_LENGTH - 1));
            WiSafe_DALSetReportedProperty(msg->id, PROP_MANUFACTURER_ID, (propertyName_t)PROP_MANUFACTURER_CLOUD_NAME, evString, valueMf);

            EnsoPropertyValue_u valueM = { .uint32Value = msg->model };
            WiSafe_DALSetReportedProperty(msg->id, DAL_PROPERTY_DEVICE_MODEL_IDX, DAL_PROPERTY_DEVICE_MODEL, evUnsignedInt32, valueM);

            /* String version of the model. */
            EnsoPropertyValue_u valueModel;
            snprintf(valueModel.stringValue, LSD_STRING_PROPERTY_MAX_LENGTH, "%u", msg->model);
            valueModel.stringValue[LSD_STRING_PROPERTY_MAX_LENGTH -1] = '\0'; // Ensure null terminated
            WiSafe_DALSetReportedProperty(msg->id, PROP_MODEL_ID, (propertyName_t)PROP_MODEL_CLOUD_NAME, evString, valueModel);

            EnsoPropertyValue_u valueT  = { .uint32Value =  onlineStatus};
            WiSafe_DALSetReportedProperty(msg->id, PROP_ONLINE_ID, (propertyName_t)PROP_ONLINE_CLOUD_NAME, evUnsignedInt32, valueT);

            EnsoPropertyValue_u valueS = { .uint32Value = msg->sid };
            WiSafe_DALSetReportedProperty(msg->id, DAL_PROPERTY_DEVICE_SID_IDX, DAL_PROPERTY_DEVICE_SID, evUnsignedInt32, valueS);

            uint32_t epocTime = OSAL_GetTimeInSecondsSinceEpoch();
            EnsoPropertyValue_u valueTS = { .uint32Value = epocTime };
            WiSafe_DALSetReportedProperty(msg->id, DAL_PROPERTY_DEVICE_TEST_TIMESTAMP_IDX, DAL_PROPERTY_DEVICE_TEST_TIMESTAMP, evUnsignedInt32, valueTS);
        }
    }
    else
    {
        LOG_Error("Device tested message received with bad SID (%d).", msg->sid);
    }
}
#endif

/**
 * This function enables or disables Learn mode. When Learn mode is
 * enabled a simulated RM button press message is sent, and then the
 * polling rate of the SID map is increased so that we spot more
 * readily when we have joined a network. We keep looking forever
 * until we either spot we've joined, or the platform tells us to stop
 * by calling this function with enabled = false. The platform can
 * spot when we have joined a network by looking for deltas in the 'in
 * network' property.
 *
 * @param enabled Whether Learn mode is enabled or disabled.
 * @param timeout If enabled, the timeout after which learn mode
 *                should automatically end.
 */
void WiSafe_DiscoveryLearnMode(bool enabled, uint32_t timeout)
{
    /* Ignore any requests which don't change what we're already doing. */
    if (enabled == inLearnMode)
    {
        LOG_Warning("Ignoring Learn mode requested because inLearnMode already equals %s.", inLearnMode?"true":"false");
        return;
    }

    /* See if we're already in a network. */
    EnsoErrorCode_e error;
    EnsoPropertyValue_u inNetwork = WiSafe_DALGetReportedProperty(GATEWAY_DEVICE_ID, DAL_PROPERTY_IN_NETWORK_IDX, &error);
    bool alreadyInNetwork = ((error == eecNoError) && (inNetwork.booleanValue == true));

    /* Are we going into Learn mode, or being asked to stop it? */
    inLearnMode = enabled;
    if (enabled && (timeout != 0))
    {
        LOG_InfoC(LOG_CYAN "Entering Learn-in mode.");

        /* Update the gateway's state. */
        EnsoPropertyValue_u valueS = { .uint32Value = GatewayStateLearning };
        WiSafe_DALSetReportedProperty(GATEWAY_DEVICE_ID, PROP_STATE_ID, (propertyName_t)PROP_STATE_CLOUD_NAME, evUnsignedInt32, valueS);

        /* Simulate the RM button press to begin Learn (but only if
         * not already in a network). If already in a network still
         * increase the rate of polling but just don't simulate the
         * button press. */
        if (!alreadyInNetwork)
        {
            LOG_InfoC(LOG_CYAN "Simulating RM button press for Learn-in.");
            radioCommsBuffer_t* buffer = WiSafe_EncodeRMLearnIn();
            WiSafe_RadioCommsTx(buffer);
        }

        /* Cancel our existing timer. */
        if (networkRefreshTimerRef != NULL)
        {
            OSAL_DestroyTimer(networkRefreshTimerRef);
        }

        /* Start polling at the faster rate for when in Learn mode. */
        Handle_t type = (Handle_t)timerType_DiscoveryRetrieveSidMap;
        networkRefreshTimerRef = OSAL_NewTimer(WiSafe_TimerCallback, (learnModeNetworkRefreshPeriod * 1000), false, type);

        if (networkRefreshTimerRef == NULL)
        {
            LOG_Error("WiSafe failed to create network refresh timer (3), errno=%s.", strerror(errno));
        }

        /* For now, ignore the timeout. If needed in the future, uncomment the lines below. */
        // /* Set our timeout. */
        // type = (Handle_t)timerType_DiscoveryLearnModeTimeout;
        // if (learnModeTimeoutTimerRef)
        // {
        //     // Delete the existing timer
        //     OSAL_DestroyTimer(learnModeTimeoutTimerRef);
        //     learnModeTimeoutTimerRef = NULL;
        // }
        // learnModeTimeoutTimerRef = OSAL_NewTimer(WiSafe_TimerCallback, (timeout * 1000), false, type);
    }
    else
    {
        LOG_InfoC(LOG_CYAN "Leaving Learn-in mode.");

        /* Update the gateway's state. */
        EnsoPropertyValue_u valueS = { .uint32Value = GatewayStateRunning };
        WiSafe_DALSetReportedProperty(GATEWAY_DEVICE_ID, PROP_STATE_ID, (propertyName_t)PROP_STATE_CLOUD_NAME, evUnsignedInt32, valueS);

        /* Cancel our existing timer. */
        if (networkRefreshTimerRef != NULL)
        {
            OSAL_DestroyTimer(networkRefreshTimerRef);
        }

        /* Start polling at the normal rate for when NOT in Learn mode. */
        Handle_t type = (Handle_t)timerType_DiscoveryRetrieveSidMap;
        networkRefreshTimerRef = OSAL_NewTimer(WiSafe_TimerCallback, (normalNetworkRefreshPeriod * 1000), false, type);

        if (networkRefreshTimerRef == NULL)
        {
            LOG_Error("WiSafe failed to create network refresh timer (4), errno=%s.", strerror(errno));
        }

        /* Cancel timeout. */
        if (learnModeTimeoutTimerRef != NULL)
        {
            OSAL_DestroyTimer(learnModeTimeoutTimerRef);
            learnModeTimeoutTimerRef = NULL;
        }
    }
}

/**
 * Helper to abort the currently running missing node test.
 *
 */
static void CancelMissingNodeTest(void)
{
    if (testingForMissingDevices)
    {
        /* Stop the current test by pretending we've run out of time. */
        testMissingRanOutOfTime = true;
        TestMissingUpdateMapAtEnd();
    }
}

/**
 * Used to begin a missing node test, or stop the current test.
 *
 * @param enabled Set true to begin a test, false to stop the current test.
 * @param deleteMissingNodes Whether or not to delete missing nodes at the end of the test.
 */
void WiSafe_DiscoveryTestMode(bool enabled, bool deleteMissingNodes)
{
    /* Set this immediately. If we're currently running a test, it will pick up the desired value in time. */
    testMissingDeleteMissingDevices = deleteMissingNodes;

    /* Does 'enabled' actually change what we're currently doing? */
    if (enabled == testingForMissingDevices)
    {
        LOG_InfoC(LOG_CYAN "'enabled' has not made any difference to what we are currently doing.");
        return;
    }

    /* Right, we have some work to do, are we starting or stopping a test? */
    if (enabled)
    {
        // Stop interrogating unknown devices while network test is in progress
        EnsoPropertyValue_u value = { .booleanValue = false };
        LOG_Info("Setting %s to %i", DAL_PROPERTY_INTERROGATING, value.booleanValue);
        EnsoErrorCode_e error;
        error = WiSafe_DALSetReportedProperty(GATEWAY_DEVICE_ID,
                                              DAL_PROPERTY_INTERROGATING_IDX,
                                              DAL_PROPERTY_INTERROGATING,
                                              evBoolean,
                                              value);
        if (error)
        {
            LOG_Error("Could not reset interrogation property")
        }

        /* Clear out the result of any previous test. */
        WiSafe_DALDeleteProperty(GATEWAY_DEVICE_ID, DAL_COMMAND_TEST_MODE_RESULT_IDX, DAL_COMMAND_TEST_MODE_RESULT);

        /* Are we in a network, no point in doing anything if we're not. */
        EnsoPropertyValue_u inNetwork = WiSafe_DALGetReportedProperty(GATEWAY_DEVICE_ID, DAL_PROPERTY_IN_NETWORK_IDX, &error);
        if ((error == eecNoError) && (inNetwork.booleanValue == false))
        {
            LOG_InfoC(LOG_CYAN "Missing node scan ending immediately - not in a network.");
            EndTestMode(true);
        }
        else
        {
            /* Cancel our existing timer. */
            if (testMissingTimerRef != NULL)
            {
                OSAL_DestroyTimer(testMissingTimerRef);
                testMissingTimerRef = NULL;
            }

            /* Start the test. */
            LOG_InfoC(LOG_CYAN "Missing node scan triggered by command.");
            InitMissingNodeScan();
        }
    }
    else
    {
        CancelMissingNodeTest();
    }
}

/**
 * Get the model for the specified device. If the device is not found,
 * returns zero.
 *
 * @param id The device.
 *
 * @return The model or zero if not found.
 */
deviceModel_t WiSafe_DiscoveryGetModelForDevice(deviceId_t id)
{
    EnsoErrorCode_e error;
    LOG_Error("NDI WiSafe_DiscoveryGetModelForDevice ");
    EnsoPropertyValue_u model = WiSafe_DALGetReportedProperty(id, DAL_PROPERTY_DEVICE_MODEL_IDX, &error);
    if (error == eecNoError)
    {
        return (deviceModel_t)(model.uint32Value);
    }

    return 0;
}

/**
 * Ask the gateway to leave its WiSafe network.
 *
 */
void WiSafe_DiscoveryLeave(void)
{
    /* There is currently no way to remove the hub from a wisafe network
       while keeping the network intact for the other nodes in the
       network. To remove the hub from the network we would have to send
       an Update SID message with the hub's SID cleared. But we don't know
       the hub's SID so we can't do that. */
    LOG_Error("'Leave' command is not supported.");
}

/**
 * Remove all devices from the WiSafe network. Will also remove ourselves,
 * which in turn will mean "in network" is false.
 *
 */
void WiSafe_DiscoveryFlushAll(void)
{
    /* Check we're not already in the middle of a flush. */
    if (isFlushingAll > 0)
    {
        LOG_Error("Ignoring request to Flush All because already doing a flush.");
        return;
    }

    /* Abort any missing node test that may be happening. This is ok because we
       are going to schedule a new one anyway at the end of our flush. */
    CancelMissingNodeTest();

    /* Mark all known devices as offline so that they get deleted in TestMissingUpdateMapAtEnd(). */
    deviceId_t buffer[DAL_MAXIMUM_NUMBER_WISAFE_DEVICES];
    uint16_t numDevices = 0;
    EnsoErrorCode_e enumerateError = WiSafe_DALDevicesEnumerate(buffer, DAL_MAXIMUM_NUMBER_WISAFE_DEVICES, &numDevices);
    if (enumerateError == eecNoError)
    {
        for (uint16_t loop = 0; loop < numDevices; loop += 1)
        {
            deviceId_t current = buffer[loop];

            if (current != GATEWAY_DEVICE_ID)
            {
                EnsoPropertyValue_u valueF = { .uint32Value = ONLINE_STATUS_OFFLINE };
                WiSafe_DALSetReportedProperty(current, PROP_ONLINE_ID, (propertyName_t)PROP_ONLINE_CLOUD_NAME, evUnsignedInt32, valueF);
            }
        }
    }
    else
    {
        LOG_Error("Failed to enumerate devices while flushing all.");
    }

    /* Now we are going to (re)use the functionality at the end of a
       missing node test, to update the SID map with all known devices
       removed. We do this by carefully setting up the state and then
       calling TestMissingUpdateMapAtEnd(). */
    memcpy(removalSidMap, currentSidMap, sizeof(removalSidMap));

    testMissingUpdateDeleteCount = 0;
    testMissingRanOutOfTime = false;
    testMissingDeleteMissingDevices = true;
    testingForMissingDevices = true;
    rerunMissingNodeTestImmediately = true;
    isFlushingAll = 2;

    TestMissingUpdateMapAtEnd();

    /* After TestMissingUpdateMapAtEnd() has done its work it will do
       a new missing node test. The above code only removed known
       devices (ie. ones that have sent us a test message), so the
       missing node test will ensure missing devices are also removed
       (ie. ones that are in the map but haven't announced themselves
       to us with a test message). */
}

/**
 * Remove the given device from the network.
 *
 * @param id The ID of the device to remove.
 */
void WiSafe_DiscoveryFlush(deviceId_t id)
{
    /* First see if we can find the SID for the required device. */
    EnsoErrorCode_e error;
    EnsoPropertyValue_u sid = WiSafe_DALGetReportedProperty(id, DAL_PROPERTY_DEVICE_SID_IDX, &error);
    if (error != eecNoError)
    {
        LOG_Error("Cannot flush device 0x%06x because its SID is not known.", id);
        return;
    }

    /* Now abort any missing node test that may be happening. This is ok because we
       are going to schedule a new one anyway at the end of our flush. */
    CancelMissingNodeTest();

    /* Now mark the device as offline so that it gets deleted in TestMissingUpdateMapAtEnd(). */
    EnsoPropertyValue_u valueF = { .uint32Value = ONLINE_STATUS_OFFLINE };
    WiSafe_DALSetReportedProperty(id, PROP_ONLINE_ID, (propertyName_t)PROP_ONLINE_CLOUD_NAME, evUnsignedInt32, valueF);

    /* Now we are going to (re)use the functionality at the end of a
       missing node test, to update the SID map with the required
       device removed. We do this by carefully setting up the state
       and then calling TestMissingUpdateMapAtEnd(). */
    memset(&(removalSidMap[0]), 0x00, sizeof(removalSidMap));
    SetBit(removalSidMap, sid.uint32Value);

    testMissingUpdateDeleteCount = 0;
    testMissingRanOutOfTime = false;
    testMissingDeleteMissingDevices = true;
    // G2-556 This is actually dangerous. It could flush other offline devices not just the one we have been asked to flush.
    testingForMissingDevices = true;
    rerunMissingNodeTestImmediately = true;

    TestMissingUpdateMapAtEnd();

    /* After TestMissingUpdateMapAtEnd() has done its work it will do
       a new missing node test. This ensures our view of the current
       SID map is up-to-date. */
}

/**
 * Go through all devices in the shadow, and determine what the SID map would
 * look like if they were all in it; excluding ourselves (gateway) as it's
 * unknown.
 *
 * @param  Where to store the result
 * @return An error code
 */
static EnsoErrorCode_e WiSafe_CalculateShadowSIDMap(uint8_t* resultMap)
{
    /* Clear the result map. */
    memset(resultMap, 0x00, (SID_MAP_BIT_COUNT / 8));

    /* Get all devices. */
    uint16_t numDevices = 0;
    deviceId_t buffer[DAL_MAXIMUM_NUMBER_WISAFE_DEVICES];
    EnsoErrorCode_e result = WiSafe_DALDevicesEnumerate(buffer, DAL_MAXIMUM_NUMBER_WISAFE_DEVICES, &numDevices);
    if (result == eecNoError)
    {
        /* Iterate through all devices. */
        for (uint16_t loop = 0; loop < numDevices; loop += 1)
        {
            /* Get the SID for this device, and set the appropriate bit. */
            deviceId_t current = buffer[loop];
            EnsoPropertyValue_u sid = WiSafe_DALGetReportedProperty(current, DAL_PROPERTY_DEVICE_SID_IDX, &result);
            if (result == eecNoError)
            {
                SetBit(resultMap, sid.uint32Value);
            }
            else
            {
                break; // Return the error.
            }
        }
    }

    return result;
}

/**
 * Find a device in the shadow by its SID.
 *
 * @param requiredSid The SID of the required device
 * @param device Where to provide the result
 * @return Error code
 */
static EnsoErrorCode_e FindDeviceBySID(uint32_t requiredSid, deviceId_t* device)
{
    /* Get all devices. */
     LOG_Error("NDI FindDeviceBySID ");
    uint16_t numDevices = 0;
    deviceId_t buffer[DAL_MAXIMUM_NUMBER_WISAFE_DEVICES];
    EnsoErrorCode_e result = WiSafe_DALDevicesEnumerate(buffer, DAL_MAXIMUM_NUMBER_WISAFE_DEVICES, &numDevices);
    if (result == eecNoError)
    {
        /* Iterate through all devices. */
        for (uint16_t loop = 0; loop < numDevices; loop += 1)
        {
            /* Get the SID for this device, and see if it's what we want? */
            deviceId_t current = buffer[loop];
            EnsoPropertyValue_u sid = WiSafe_DALGetReportedProperty(current, DAL_PROPERTY_DEVICE_SID_IDX, &result);
            if (result == eecNoError)
            {
                if (sid.uint32Value == requiredSid)
                {
                    *device = current;
                    return eecNoError;
                }
            }
        }
    }

    return eecEnsoObjectNotFound;
}

/**
 * Process a SID map update - this happens when an ACU has reorganised the network,
 * by holding down the search button for 5 seconds.
 *
 * @param mapData The map.
 */
void WiSafe_DiscoveryProcessSIDMapUpdate(uint8_t* mapData)
{
    LOG_InfoC(LOG_CYAN "Received SID map update: %s", PrettySIDMap(mapData));

    /* Determine the map corresponding to all the WiSafe devices in our local shadow. */
    uint8_t shadowMap[SID_MAP_BIT_COUNT / 8] = {0,};
    EnsoErrorCode_e shadowError = WiSafe_CalculateShadowSIDMap(shadowMap);
    if (shadowError != eecNoError)
    {
        LOG_Error("Unable to calculate map for shadow devices.");
        return;
    }

    /* See if any devices are missing, and mark them as offline. */
    for (uint32_t loop = 0; loop < SID_MAP_BIT_COUNT; loop += 1)
    {
        /* See if the device is in our shadow SID map, but not in the map update? */
        if (IsBitSet(shadowMap, loop) && !IsBitSet(mapData, loop))
        {
            LOG_InfoC(LOG_CYAN "Map update indicates SID %d is gone.", loop);

            /* This device is offline, find it in the shadow using its SID position. */
            deviceId_t device;
            EnsoErrorCode_e error = FindDeviceBySID(loop, &device);
            if (error == eecNoError)
            {
                /* Mark as offline. */
                LOG_InfoC(LOG_CYAN "Marking device %06x as offline.", device);
                EnsoPropertyValue_u value = { .uint32Value = ONLINE_STATUS_OFFLINE };
                WiSafe_DALSetReportedProperty(device, PROP_ONLINE_ID, (propertyName_t)PROP_ONLINE_CLOUD_NAME, evUnsignedInt32, value);

                // Mark the device missing
                EnsoPropertyValue_u valueT = { .booleanValue = true };
                WiSafe_DALSetReportedProperty(device, DAL_PROPERTY_DEVICE_MISSING_IDX, (propertyName_t)DAL_PROPERTY_DEVICE_MISSING, evBoolean, valueT);
            }
            else
            {
                LOG_Error("Could not find device in shadow corresponding to SID %d.", loop);
            }
        }
    }
}

/**
 * Process a Rumour Target Response.
 *
 * A Rumour Target message was sent by the gateway to "prime" a device
 * to sound its siren. Now send a Device Tested to trigger its siren.
 *
 * @param mapData The map.
 */
void WiSafe_DiscoveryProcessRumourTargetResponse(void)
{
     LOG_Error("NDI WiSafe_DiscoveryProcessRumourTargetResponse ");
    radioCommsBuffer_t* buffer = WiSafe_EncodeDeviceTest(0, GATEWAY_DEVICE_MODEL, GATEWAY_DEVICE_PRIORITY);
    if (buffer != NULL)
    {
        WiSafe_RadioCommsTx(buffer);
    }
    else
    {
        LOG_Error("Cannot issue device test because no buffers free.");
    }
}
