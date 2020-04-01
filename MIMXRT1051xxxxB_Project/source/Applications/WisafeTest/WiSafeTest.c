/*!****************************************************************************
 * \file    WisafeTest.c
 *
 * \author  Gavin Dolling
 *
 * \brief   Entry point for Wisafe Unit Tests
 *
 * \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 *
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "LOG_Api.h"
#include "LSD_Api.h"
#include "WiSafe_Main.h"
#include "WiSafe_DAL.h"
#include "C:/gateway/SA3075-P0302_2100_AC_Gateway/Port/Build/x86_64/include/CUnit/Basic.h"
#include "WiSafeTest.h"
#include "HAL.h"
#include "WiSafe_Protocol.h"
#include "WiSafe_DAL.h"
#include "WiSafe_Settings.h"
#include "LSD_Api.h"

/*!****************************************************************************
 * Constants
 *****************************************************************************/
//////// [RE:platform]
////////#ifdef Ameba
#define WISAFETEST_TIME_FOR_PROCESSING          (800)
////////#else
////////#define WISAFETEST_TIME_FOR_PROCESSING          (100)
////////#endif

#define SID_MAP_BIT_COUNT 64 /* Number of bit positions in SID map. */
static const EnsoPropertyValue_u valueTrue = { .booleanValue = true };
static const EnsoPropertyValue_u valueFalse = { .booleanValue = false };
static const EnsoPropertyValue_u value0 = { .uint32Value = 0 };
static const EnsoPropertyValue_u value1 = { .uint32Value = 1 };

/*!****************************************************************************
 * Static variables
 *****************************************************************************/
static uint8_t simulatedSidMap[SID_MAP_BIT_COUNT / 8] = {0,};
static const uint32_t timeForProcessing = WISAFETEST_TIME_FOR_PROCESSING; // How long in milliseconds to allow the WiSafe core engine to process messages.

/*!****************************************************************************
 * Private Functions
 *****************************************************************************/


/*!****************************************************************************
 * Stubs to enable compilation for testing
 *****************************************************************************/

extern propertyName_t WiSafe_ConvertAgentToName(EnsoAgentSidePropertyId_t agentSideId);
extern void WiSafe_InjectPacket(uint8_t* buffer, uint32_t length);
char * LSD_EnsoErrorCode_eToString(EnsoErrorCode_e error);

/**
 * \name    LSD_EnsoErrorCode_eToString
 *
 * \brief   returns the string representation of error
 *
 * \param   error    EnsoErrorCode_e - error code
 *
 * \return  char * string representation of error
 */
char * LSD_EnsoErrorCode_eToString(EnsoErrorCode_e error)
{
    return error == eecPropertyNotFound ? "eecPropertyNotFound" :
           error == eecFunctionalityNotSupported ? "eecFunctionalityNotSupported" :
           error == eecNullPointerSupplied ? "eecNullPointerSupplied" :
           error == eecParameterOutOfRange ? "eecParameterOutOfRange" :
           error == eecPropertyValueNotObjectHandle ? "eecPropertyValueNotObjectHandle" :
           error == eecPropertyObjectHandleNotValue ? "eecPropertyObjectHandleNotValue" :
           error == eecPropertyNotCreatedDuplicateClientId ? "eecPropertyNotCreatedDuplicateClientId" :
           error == eecPropertyNotCreatedDuplicateCloudName ? "eecPropertyNotCreatedDuplicateCloudName" :
           error == eecPropertyWrongType ? "eecPropertyWrongType" :
           error == eecPropertyGroupNotSupported ? "eecPropertyGroupNotSupported" :
           error == eecEnsoObjectNotFound ? "eecEnsoObjectNotFound" :
           error == eecEnsoDeviceNotFound ? "eecEnsoDeviceNotFound" :
           error == eecSubscriberNotFound ? "eecSubscriberNotFound" :
           error == eecEnsoObjectAlreadyRegistered ? "eecEnsoObjectAlreadyRegistered" :
           error == eecBufferTooSmall ? "eecBufferTooSmall" :
           error == eecBufferTooBig ? "eecBufferTooBig" :
           error == eecConversionFailed ? "eecConversionFailed" :
           error == eecPoolFull ? "eecPoolFull" :
           error == eecTimeOut ? "eecTimeOut" :
           error == eecEnsoObjectNotCreated ? "eecEnsoObjectNotCreated" :
           error == eecLocalShadowConnectionFailedToInitialise ? "eecLocalShadowConnectionFailedToInitialise" :
           error == eecLocalShadowConnectionFailedToConnect ? "eecLocalShadowConnectionFailedToConnect" :
           error == eecLocalShadowConnectionFailedToSetAutoConnect ? "eecLocalShadowConnectionFailedToSetAutoConnect" :
           error == eecLocalShadowConnectionFailedToClose ? "eecLocalShadowConnectionFailedToClose" :
           error == eecLocalShadowConnectionPollingFailed ? "eecLocalShadowConnectionPollingFailed" :
           error == eecLocalShadowFailedSubscription ? "eecLocalShadowFailedSubscription" :
           error == eecLocalShadowDocumentFailedToInitialise ? "eecLocalShadowDocumentFailedToInitialise" :
           error == eecLocalShadowDocumentFailedFinalise ? "eecLocalShadowDocumentFailedFinalise" :
           error == eecLocalShadowDocumentFailedToSendDelta ? "eecLocalShadowDocumentFailedToSendDelta" :
           error == eecTestFailed ? "eecTestFailed" :
           error == eecInternalError ? "eecInternalError" :
           error == eecWouldBlock ? "eecWouldBlock" :
           error == eecOpenFailed ? "eecOpenFailed" :
           error == eecCloseFailed ? "eecCloseFailed" :
           error == eecReadFailed ? "eecReadFailed" :
           error == eecWriteFailed ? "eecWriteFailed" :
           error == eecRenameFailed ? "eecRenameFailed" :
           error == eecRemoveFailed ? "eecRemoveFailed" :
           error == eecDeviceNotSupported ? "eecDeviceNotSupported" :
           error == eecNoChange ? "eecNoChange" :
           error == eecNoError ? "eecNoError" : "Undefined";
}



/*
 * \brief Helper function to get time to set value for a timestamp
 *
 * \return     property value to use in a timestamp
 */
EnsoPropertyValue_u LSD_GetTimeNow() {

    EnsoPropertyValue_u newValue;

    newValue.timestamp.seconds = OSAL_GetTimeInSecondsSinceEpoch();

    // Time defaults to Jan 2000 if we don't have actual time yet
    newValue.timestamp.isValid = (newValue.timestamp.seconds > 978307200); // Jan 1, 2001

    return newValue;
}



/**
 * \name LSD_GetPropertyCloudNameFromAgentSideId
 *
 * \brief Retrieves a property name by its agent side identifer.
 *
 * This function finds the property name using the agent side property identifier.
 *
 * \param   deviceID            The device that owns the property in
 *                              question.
 *
 * \param   agentSidePropertyId The property ID as supplied by the agent side
 *
 * \param   rxBufferSize        The size of the buffer that will receive the cloud name
 *
 * \param   cloudName           Buffer to copy name into
 *
 * \return                      EnsoErrorCode_e
 *
 */
EnsoErrorCode_e LSD_GetPropertyCloudNameFromAgentSideId(const EnsoDeviceId_t* deviceId,
        const EnsoAgentSidePropertyId_t agentSidePropertyId, const size_t bufferSize,
        char* cloudName)
{
    bool result = eecPropertyNotFound;

    char* name = (char*) WiSafe_ConvertAgentToName(agentSidePropertyId);

    if (NULL == name)
    {
        return result;
    }

    if ((NULL == deviceId) || (NULL == cloudName))
    {
        return eecNullPointerSupplied;
    }

    if (LSD_PROPERTY_NAME_BUFFER_SIZE > bufferSize)
    {
        result = eecBufferTooSmall;
    }
    else
    {
        strncpy(cloudName, name, LSD_PROPERTY_NAME_BUFFER_SIZE);
        result = eecNoError;
    }

    return result;
}


/**
 * \name LSD_DeviceIdCompare
 *
 * \brief Compare two thing identifiers analogous to strcmp
 *
 * \param leftThing     The thing on the left to compare
 *
 * \param rightThing    The thing on the right to compare
 *
 * \return              positive leftThing >  rightThing
 *                      zero     leftThing == rightThing
 *                      negative leftThing <  rightThing
 *
 */
int LSD_DeviceIdCompare(const EnsoDeviceId_t* leftThing, const EnsoDeviceId_t* rightThing)
{
    assert(leftThing);
    assert(rightThing);

    int comparison = 0;

    if (leftThing->deviceAddress > rightThing->deviceAddress)
    {
        comparison = 1;
    }
    else if (leftThing->deviceAddress < rightThing->deviceAddress)
    {
        comparison = -1;
    }
    else
    {
        if (leftThing->technology > rightThing->technology)
        {
            comparison = 1;
        }
        else if (leftThing->technology < rightThing->technology)
        {
            comparison = -1;
        }
        else
        {
            if (leftThing->childDeviceId > rightThing->childDeviceId)
            {
                comparison = 1;
            }
            else if (leftThing->childDeviceId < rightThing->childDeviceId)
            {
                comparison = -1;
            }
            else
            {
                comparison = 0;
            }
        }
    }

    return comparison;
}



/**
 * \name SYS_IsGateway
 *
 * \brief Returns true if the device passed as an argument is the Gateway
 *
 * \param  deviceId      The device identifier
 *
 * \return True if teh device is the gateway
 */

bool SYS_IsGateway(EnsoDeviceId_t* deviceId)
{
    EnsoDeviceId_t gatewayDeviceId = {
        .childDeviceId = 0,
        .technology = ETHERNET_TECHNOLOGY,
        .deviceAddress = HAL_GetMACAddress(),
        .isChild = false
    };

    return (0 == LSD_DeviceIdCompare(deviceId, &gatewayDeviceId)) ? true : false;
}



/**
 * \brief   Get the list of devices that are managed by the handler specified as
 * an argument
 *
 * \param   handlerId       The identifier of the handler managing the devices
 *
 * \param[out] buffer       Buffer where the list of device identifiers is written

 * \param   bufferElems     Maximum number of elements of the buffer array
 *
 * \param[out] numDevices   Number of device identifiers that have been written.
 *                          If the function returns successfully then this is also
 *                          the number of devices that are managed by the handler
 *
 * \return                  EnsoErrorCode_e
 */
EnsoErrorCode_e LSD_GetDevicesId(
        const HandlerId_e handlerId,
        EnsoDeviceId_t* buffer,
        const uint16_t  bufferElems,
        uint16_t* numDevices)
{
    // Sanity check
    if (!buffer || !numDevices)
    {
        return eecNullPointerSupplied;
    }

    LOG_Warning("LSD_GetDevicesId stub needs implementing depending upon needs");

    return eecDeviceNotSupported;
}

/*!****************************************************************************
 * Public Functions
 *****************************************************************************/

/* The suite initialisation function.
 * Opens the temporary file used by the tests.
 * Returns zero on success, non-zero otherwise.
 */
int init_suite1(void)
{
    /* Set up anything in here that is needed before the tests start */
    EnsoErrorCode_e errorCode = WiSafe_Start();

    return (errorCode != eecNoError);
}

/* The suite cleanup function.
 * Returns zero on success, non-zero otherwise.
 */
int clean_suite1(void)
{
    /* Tidy up after tests have run */
    return 0;
}

/*
 * Helper to clear our simulated SID map.
 */
static void ClearSidMap()
{
    memset(simulatedSidMap, 0, sizeof(simulatedSidMap));
}

/*
 * Helper to set a bit in our simulated SID map.
 */
static void SetSidMapBit(deviceId_t id)
{
    /* For the purposes of this test we just treat the ID as an integer and set the relevant bit. */
    uint32_t pos = (uint32_t)id;
    uint32_t bytePos = (pos / 8);
    uint32_t bitPos = (pos % 8);

    uint8_t data = simulatedSidMap[bytePos];
    data |= (1 << bitPos);
    simulatedSidMap[bytePos] = data;

    /* Also, make sure that bit 0 is set as that is our RM. */
    simulatedSidMap[0] |= 1;
}

/*
 * Helper to clear a bit in our simulated SID map.
 */
static void ClearSidMapBit(deviceId_t id)
{
    /* For the purposes of this test we just treat the ID as an integer and set the relevant bit. */
    uint32_t pos = (uint32_t)id;
    uint32_t bytePos = (pos / 8);
    uint32_t bitPos = (pos % 8);

    uint8_t data = simulatedSidMap[bytePos];
    data &= ~(1 << bitPos);
    simulatedSidMap[bytePos] = data;
}

/*
 * Helper to determine the priority of one of our simulated devices.
 */
static uint8_t DevicePriority(deviceId_t id)
{
    return 129;
}

/*
 * Helper to get the SID of one of our simulated devices.
 */
static uint8_t DeviceSID(deviceId_t id)
{
    return (id & 0xff);
}

/*
 * Helper to get the model string for one of our simulated devices.
 */
static const char* DeviceModelString(deviceId_t id)
{
    static const char* smokeAlarm = "SmokeAlarm.";
    return smokeAlarm;;
}

/*
 * Helper method to simulate (and test) pressing the test button on a sensor.
 */
static void TestDeviceTested(deviceId_t id)
{
    LOG_InfoC(LOG_GREEN "Testing Device Tested for sensor id %08x.", id);

    /* Read the number of unknown devices so that we can check that it goes down. */
    EnsoErrorCode_e error;
    EnsoPropertyValue_u unknownDevices = WiSafe_DALGetReportedProperty(GATEWAY_DEVICE_ID, DAL_PROPERTY_UNKNOWN_SIDS_IDX, &error);
    CU_ASSERT(error == eecNoError);

    /* Now simulate the test button being pressed on the newly attached WiSafe sensor. */
    {
        /* Fake the Device Tested message from the sensor. */
        uint8_t deviceTestedData[] = {0x70, ((id & 0xff) >> 0), ((id & 0xff00) >> 8), ((id & 0xff000) >> 16), DevicePriority(id), 0x00, 0x02, 0x01, DeviceSID(id), 0x00};
        WiSafe_InjectPacket(deviceTestedData, sizeof(deviceTestedData));

        /* Wait for the WiSafe code to process the reply. */
        OSAL_sleep_ms(timeForProcessing);
    }

    /* Now check that this new device has been put in the shadow correctly. */
    {
        CU_ASSERT(CheckReportedProperty(GATEWAY_DEVICE_ID, DAL_PROPERTY_IN_NETWORK_IDX, evBoolean, valueTrue)); // In network
        unknownDevices.uint32Value -= 1;
        CU_ASSERT(CheckReportedProperty(GATEWAY_DEVICE_ID, DAL_PROPERTY_UNKNOWN_SIDS_IDX, evUnsignedInt32, unknownDevices)); // Unknown devices has gone down by 1.

        EnsoPropertyValue_u valueModel;
        strncpy(valueModel.stringValue, DeviceModelString(id), sizeof(valueModel.stringValue));
        CU_ASSERT(CheckReportedProperty(0x000001, PROP_MODEL_ID, evString, valueModel)); // Model of new sensor
    }
}

static void WaitAndRespondToSidMapRequest()
{
    /* Wait for the driver to request the SID map to spot the unknown device. */
    uint8_t requestSidMapData[] = {0xd3, 0x03};
    expectedPacket_t requestSidMap = {sizeof(requestSidMapData), requestSidMapData};
    expectedPacket_t* expectedTX[] = {&requestSidMap, NULL};
    WiSafe_RegisterAndWaitForExpectedTX(10, expectedTX);

    /* Fake the SID map reply from the RM. */
    uint8_t sidMapData[] = {0xd4, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    memcpy(&(sidMapData[2]), simulatedSidMap, sizeof(simulatedSidMap));
    WiSafe_InjectPacket(sidMapData, sizeof(sidMapData));

    /* Wait for the WiSafe code to process the reply. */
    OSAL_sleep_ms(timeForProcessing);
}

/**
 * \name   TestLearnIn
 * \brief  Test learning-in of a new device.
 */
void TestLearnIn(void)
{
    LOG_InfoC(LOG_GREEN "Testing that we're NOT in a network.");

    /* Wait for the driver to request the SID map, and return an empty one to show we are not in a network. */
    {
        ClearSidMap();
        WaitAndRespondToSidMapRequest();
    }

    /* Check that the reply has been processed correctly, and that we are still not in a network. */
    {
        CU_ASSERT(CheckReportedProperty(GATEWAY_DEVICE_ID, DAL_PROPERTY_IN_NETWORK_IDX, evBoolean, valueFalse)); // Not in network
        CU_ASSERT(CheckReportedProperty(GATEWAY_DEVICE_ID, DAL_PROPERTY_UNKNOWN_SIDS_IDX, evUnsignedInt32, value0)); // Unknown devices = 0
    }

    LOG_InfoC(LOG_GREEN "Testing that we can learn-in to a network.");

    /* Simulate learn-in command coming from platform. */
    {
        EnsoPropertyValue_u valueLearning = { .uint32Value = GatewayStateLearning };
        uint8_t rmLearnInData[] = {0xd3, 0x12, 0x01};
        expectedPacket_t rmLearnIn = {sizeof(rmLearnInData), rmLearnInData};
        expectedPacket_t* expectedTX[] = {&rmLearnIn, NULL};
        WiSafe_RegisterExpectedTX(expectedTX);

        /* Set the timeout parameter. */
        WiSafe_DALSetDesiredProperty(GATEWAY_DEVICE_ID, DAL_COMMAND_LEARN_TIMEOUT_IDX, DAL_COMMAND_LEARN_TIMEOUT, evUnsignedInt32, value1);

        /* Now atomically set the trigger and the state. */
        EnsoAgentSidePropertyId_t agentSideId[2] = {DAL_COMMAND_LEARN_IDX, PROP_STATE_ID};
        const char* cloudName[2] = {DAL_COMMAND_LEARN, PROP_STATE_CLOUD_NAME};
        EnsoValueType_e type[2] = {evUnsignedInt32, evUnsignedInt32};
        EnsoPropertyValue_u value[2] = {value1, valueLearning};

        WiSafe_DALSetDesiredProperties(GATEWAY_DEVICE_ID, agentSideId, cloudName, type, value, 2, PROPERTY_PUBLIC, true, false);

        WiSafe_WaitForExpectedTX(2);
    }

    /* Check that we signalled to the platform that we're executing the learn-in command. */
    {
        CU_ASSERT(CheckReportedProperty(GATEWAY_DEVICE_ID, DAL_COMMAND_LEARN_IDX, evUnsignedInt32, value1)); // Check that reported = desired
    }

    /* Reply with a non-empty SID map to show that we've joined a WiSafe network. */
    {
        /* Fake the SID map reply from the RM. */
        SetSidMapBit(0x000001);
        uint8_t sidMapData[] = {0xd4, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        memcpy(&(sidMapData[2]), simulatedSidMap, sizeof(simulatedSidMap));
        WiSafe_InjectPacket(sidMapData, sizeof(sidMapData));

        /* Wait for the WiSafe code to process the reply. */
        OSAL_sleep_ms(timeForProcessing);
    }

    /* Check that the reply has been processed correctly by examining properties. */
    {
        CU_ASSERT(CheckReportedProperty(GATEWAY_DEVICE_ID, DAL_PROPERTY_IN_NETWORK_IDX, evBoolean, valueTrue)); // In network
        CU_ASSERT(CheckReportedProperty(GATEWAY_DEVICE_ID, DAL_PROPERTY_UNKNOWN_SIDS_IDX, evUnsignedInt32, value1)); // Unknown devices = 1
    }

    LOG_InfoC(LOG_GREEN "Testing that we can stop learn-in mode.");

    /* Simulate stopping learn-in. */
    {
        EnsoPropertyValue_u valueRunning = { .uint32Value = GatewayStateRunning };
        WiSafe_DALSetDesiredProperty(GATEWAY_DEVICE_ID, PROP_STATE_ID, PROP_STATE_CLOUD_NAME, evUnsignedInt32, valueRunning);

        /* Wait for the WiSafe code to process the reply. */
        OSAL_sleep_ms(timeForProcessing);

        /* Check the command has been correctly processed. */
        CU_ASSERT(CheckReportedProperty(GATEWAY_DEVICE_ID, PROP_STATE_ID, evUnsignedInt32, valueRunning)); // Back to running state.
    }

    /* Test Device Tested on this first sensor, so that we join the WiSafe network. */
    TestDeviceTested(0x000001);

    /* Now that we've handled the special case above, of learning-in to a network for the first time,
       now go ahead and add another 48 devices, taking the total to 50 (including the gateway). */
    {
        /* First populate the SID map with all the devices, and wait for the next scan to pick them up. */
        for (uint32_t loop = 0; loop < 48; loop += 1)
        {
            SetSidMapBit(loop + 2); // +2 because we did device id 1 above.
        }

        /* For for the next scan. */
        WaitAndRespondToSidMapRequest();

        /* Wait for the WiSafe code to process the reply. */
        OSAL_sleep_ms(timeForProcessing);

        /* Check it picked-up all the devices. */
        static const EnsoPropertyValue_u value48 = { .uint32Value = 48 };
        CU_ASSERT(CheckReportedProperty(GATEWAY_DEVICE_ID, DAL_PROPERTY_IN_NETWORK_IDX, evBoolean, valueTrue)); // In network
        CU_ASSERT(CheckReportedProperty(GATEWAY_DEVICE_ID, DAL_PROPERTY_UNKNOWN_SIDS_IDX, evUnsignedInt32, value48)); // Unknown devices = 48

        /* Now press and test the Test Button on all devices. */
        for (uint32_t loop = 0; loop < 48; loop += 1)
        {
            TestDeviceTested(loop + 2);
        }
    }
}

/**
 * \name   TestTestMode
 * \brief  Test test mode - ie. spotting devices that have disappeared.
 */
void TestTestMode(void)
{
    /* This test assumes that TestDeviceTested() has just been run, ie. there are 50 devices learnt-in to the SID map. */
    LOG_InfoC(LOG_GREEN "Testing test mode with device deletion.");
    static const deviceId_t missingNode = 0x000002;

    /* Prepare the data for a SID map update message - we're going to receive several of these. */
    uint8_t sidMapUpdateData[] = {0xd3, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    memcpy(&(sidMapUpdateData[2]), simulatedSidMap, sizeof(simulatedSidMap));
    expectedPacket_t sidMapUpdate = {sizeof(sidMapUpdateData), sidMapUpdateData};
    expectedPacket_t* expectedSidMapUpdate[] = {&sidMapUpdate, NULL};

    /* Trigger Test Mode, with deleting missing devices enabled. */
    {
        /* Get ready to expect a SID map update message. */
        WiSafe_RegisterExpectedTX(expectedSidMapUpdate);

        /* Trigger the command. */
        WiSafe_DALSetDesiredProperty(GATEWAY_DEVICE_ID, DAL_COMMAND_TEST_MODE_FLUSH_IDX, DAL_COMMAND_TEST_MODE_FLUSH, evBoolean, valueTrue);
        WiSafe_DALSetDesiredProperty(GATEWAY_DEVICE_ID, DAL_COMMAND_TEST_MODE_IDX, DAL_COMMAND_TEST_MODE, evUnsignedInt32, value1);

        /* Wait for the SID map update message. */
        WiSafe_WaitForExpectedTX(2);
    }

    /* Check that we signalled to the platform that we're executing the learn-in command. */
    {
        CU_ASSERT(CheckReportedProperty(GATEWAY_DEVICE_ID, DAL_COMMAND_TEST_MODE_IDX, evUnsignedInt32, value1)); // Check that reported = desired
    }

    /* Inject the 3 missing node reports. */
    for (int loop = 0; loop < 3; loop += 1)
    {
        /* The first time round the loop we don't expect a SID map update message because that was handled above. */
        if (loop > 0)
        {
            /* Wait until we get a SID map update message. */
            WiSafe_RegisterAndWaitForExpectedTX((testMissingMinimumTimeBetweenScans + 1), expectedSidMapUpdate);
        }

        /* Inject the report with one of the devices missing. */
        uint8_t missingNodeData[] = {0xd4, 0x01, 0x01, (1 << missingNode), 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        WiSafe_InjectPacket(missingNodeData, sizeof(missingNodeData));
    }

    /* Now wait for the SID map update with the missing node removed. */
    ClearSidMapBit(missingNode);
    memcpy(&(sidMapUpdateData[2]), simulatedSidMap, sizeof(simulatedSidMap));
    WiSafe_RegisterAndWaitForExpectedTX((testMissingMinimumTimeBetweenScans + 1), expectedSidMapUpdate);

    /* Reply that the node has successfully been removed. */
    {
        uint8_t missingNodeData[] = {0xd4, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        WiSafe_InjectPacket(missingNodeData, sizeof(missingNodeData));
    }

    /* Wait for the WiSafe code to process the reply. */
    OSAL_sleep_ms(timeForProcessing);

    /* Check the result of the test mode command, and check that state has returned to running. */
    CU_ASSERT(CheckReportedProperty(GATEWAY_DEVICE_ID, DAL_COMMAND_TEST_MODE_RESULT_IDX, evBoolean, valueTrue)); // Result = success
    EnsoPropertyValue_u valueRunning = { .uint32Value = GatewayStateRunning };
    CU_ASSERT(CheckReportedProperty(GATEWAY_DEVICE_ID, PROP_STATE_ID, evUnsignedInt32, valueRunning)); // State = Running

    /* Check the device has been deleted. */
    CU_ASSERT(!WiSafe_DALIsDeviceRegistered(missingNode));

    /* Wait for the regular polling code to request the current SID map. */
    WaitAndRespondToSidMapRequest();
}

/**
 * \name   TestRmAll
 * \brief  Test RM all
 */
void TestRmAll(void)
{
    /* This test assumes that TestTestMode() has just been run, ie. there are 49 devices learnt-in to the SID map. */
    LOG_InfoC(LOG_GREEN "Testing RM all command.");

    /* Prepare the data for a SID map update message with everything removed. */
    uint8_t sidMapUpdateData[] = {0xd3, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    expectedPacket_t sidMapUpdate = {sizeof(sidMapUpdateData), sidMapUpdateData};
    expectedPacket_t* expectedSidMapUpdate[] = {&sidMapUpdate, NULL};

    /* Trigger RM all */
    {
        /* Trigger the command. */
        WiSafe_DALSetDesiredProperty(GATEWAY_DEVICE_ID, DAL_COMMAND_DELETE_ALL_IDX, DAL_COMMAND_DELETE_ALL, evUnsignedInt32, value1);
    }

    /* Now wait for the SID map update with the missing node removed. */
    WiSafe_RegisterAndWaitForExpectedTX((testMissingMinimumTimeBetweenScans + 1), expectedSidMapUpdate);

    /* Reply that all nodes have successfully been removed. */
    {
        uint8_t missingNodeData[] = {0xd4, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        WiSafe_InjectPacket(missingNodeData, sizeof(missingNodeData));
    }

    /* The implementation of 'delete all' does another test for missing nodes, so we test that works too. */
    memcpy(&(sidMapUpdateData[2]), simulatedSidMap, sizeof(simulatedSidMap));

    /* Now wait for the SID map update with the missing node removed. */
    WiSafe_RegisterAndWaitForExpectedTX((testMissingMinimumTimeBetweenScans + 1), expectedSidMapUpdate);

    /* Reply that all nodes have been removed. */
    {
        uint8_t missingNodeData[] = {0xd4, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        WiSafe_InjectPacket(missingNodeData, sizeof(missingNodeData));
    }

    /* Wait for the WiSafe code to process the reply. */
    OSAL_sleep_ms(timeForProcessing);

    /* Check the result of the rm all command, and check that state has returned to running. */
    EnsoPropertyValue_u valueRunning = { .uint32Value = GatewayStateRunning };
    CU_ASSERT(CheckReportedProperty(GATEWAY_DEVICE_ID, PROP_STATE_ID, evUnsignedInt32, valueRunning)); // State = Running
    CU_ASSERT(CheckReportedProperty(GATEWAY_DEVICE_ID, DAL_COMMAND_DELETE_ALL_IDX, evUnsignedInt32, value1)); // Reported = desired for rm all triggered.

    /* Wait for the regular polling code to request the current SID map. */
    ClearSidMap();
    WaitAndRespondToSidMapRequest();

    /* Check that we are not in a network. */
    {
        CU_ASSERT(CheckReportedProperty(GATEWAY_DEVICE_ID, DAL_PROPERTY_IN_NETWORK_IDX, evBoolean, valueFalse)); // Not in network
        CU_ASSERT(CheckReportedProperty(GATEWAY_DEVICE_ID, DAL_PROPERTY_UNKNOWN_SIDS_IDX, evUnsignedInt32, value0)); // Unknown devices = 0
    }
}

/**
 * \name   TestRmAll
 * \brief  Test RM all
 */
void TestFaultsAndAlarms(void)
{
    deviceId_t id = 0x000001;

    /* This test assumes that TestRmAll() has just been run, ie. we're not learnt-in to a network.
       So begin by reusing an earlier test to learn us in to a network. */
    TestLearnIn();

    /* Now do the faults and alarms tests. */
    LOG_InfoC(LOG_GREEN "Testing Alarms.");

    /* Simulate an alarm from one of the devices. */
    {
        uint8_t alarmData[] = {0x50, ((id & 0xff) >> 0), ((id & 0xff00) >> 8), ((id & 0xff000) >> 16), DevicePriority(id), 0x00, DeviceSID(id), 0x00};
        WiSafe_InjectPacket(alarmData, sizeof(alarmData));
    }

    /* Wait for the WiSafe code to process the reply. */
    OSAL_sleep_ms(timeForProcessing);

    /* Check the alarm has been set in the shadow. */
    {
        CU_ASSERT(CheckReportedProperty(id, DAL_PROPERTY_DEVICE_ALARM_STATE_IDX, evUnsignedInt32, value1)); // Alarm set.
        CU_ASSERT(CheckReportedProperty(id, DAL_PROPERTY_DEVICE_ALARM_SEQ_IDX, evUnsignedInt32, value0)); // Alarm seq set.
    }

    LOG_InfoC(LOG_GREEN "Testing Faults.");

    /* Now simulate a fault. */
    {
        uint8_t faultData[] = {0x71, ((id & 0xff) >> 0), ((id & 0xff00) >> 8), ((id & 0xff000) >> 16), 0x00, 0x00, 0x00, DeviceSID(id), 0x01};
        WiSafe_InjectPacket(faultData, sizeof(faultData));
    }

    /* Wait for the WiSafe code to process the reply. */
    OSAL_sleep_ms(timeForProcessing);


    /* Check the fault has been set in the shadow. */
    {
        const uint8_t sia = 90;
        CU_ASSERT(CheckReportedProperty(id, (DAL_PROPERTY_DEVICE_FAULT_STATE_IDX + sia), evBoolean, valueTrue)); // Fault set.
        CU_ASSERT(CheckReportedProperty(id, (DAL_PROPERTY_DEVICE_FAULT_SEQ_IDX + sia), evUnsignedInt32, value0)); // Fault seq set.
    }
}

/* The main() function for setting up and running the tests.
 * Returns a CUE_SUCCESS on successful running, another
 * CUnit error code on failure.
 */
//////// [RE:platform]
////////#ifdef Ameba
int EnsoMain(int argc, char* argv[])
////////#else
////////int main(int argc, char *argv[])
////////#endif
{
   CU_pSuite pSuite = NULL;

    //////// [RE:fixed] Need to init LSD
    EnsoErrorCode_e retVal = eecNoError;
    retVal = LSD_Init();
    if (eecNoError != retVal)
    {
        LOG_Error("Error initialising Local Shadow %s", LSD_EnsoErrorCode_eToString(retVal));
        return retVal;
    }

   /* Initialise the DAL. */
   WiSafe_DALInit();

   /* Fake some properties that would normally be created in a non-unit test scenario. */
   EnsoPropertyValue_u valueRunning = { .uint32Value = GatewayStateRunning };
   WiSafe_DALSetReportedProperty(GATEWAY_DEVICE_ID, PROP_STATE_ID, (propertyName_t)PROP_STATE_CLOUD_NAME, evUnsignedInt32, valueRunning);

   /* initialize the CUnit test registry */
   if (CUE_SUCCESS != CU_initialize_registry())
      return CU_get_error();

   /* add a suite to the registry */
   pSuite = CU_add_suite("Suite_1", init_suite1, clean_suite1);
   if (NULL == pSuite) {
      CU_cleanup_registry();
      return CU_get_error();
   }

   /* add the tests to the suite */
   if (NULL == CU_add_test(pSuite, "WiSafe Learning-In", TestLearnIn))
   {
      CU_cleanup_registry();
      return CU_get_error();
   }

   if (NULL == CU_add_test(pSuite, "WiSafe Test Mode", TestTestMode))
   {
      CU_cleanup_registry();
      return CU_get_error();
   }

   if (NULL == CU_add_test(pSuite, "WiSafe RM all", TestRmAll))
   {
       CU_cleanup_registry();
       return CU_get_error();
   }

   if (NULL == CU_add_test(pSuite, "WiSafe Faults and Alarms", TestFaultsAndAlarms))
   {
       CU_cleanup_registry();
       return CU_get_error();
   }

   /* Run all tests using the CUnit Basic interface */
   CU_basic_set_mode(CU_BRM_VERBOSE);
   CU_basic_run_tests();
   CU_cleanup_registry();
   return CU_get_error();
}
