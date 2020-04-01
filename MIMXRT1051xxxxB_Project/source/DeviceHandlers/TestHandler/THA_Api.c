/*!****************************************************************************
 *
 * \file THA_Api.c
 *
 * \author Gavin Dolling
 *
 * \brief Test handler basic property manipulation
 *
 * \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 *
 *****************************************************************************/

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <DEV_Api.h>
#include "LOG_Api.h"
#include "LSD_Api.h"
#include "LSD_Types.h"
#include "LSD_Subscribe.h"
#include "ECOM_Messages.h"
#include "LSD_Types.h"
#include "KVP_Api.h"
#include "OSAL_Api.h"
#include "APP_Types.h"
#include "SYS_Gateway.h"

/*!****************************************************************************
 * Constants
 *****************************************************************************/

#define MAX_TEMPLATE_SIZE 384

// We have considered using sizeof for these, but sizeof cannot be applied to bitfield
#define MAX_DEVICE_ADDRESS_LEN 16
#define MAX_DEVICE_TYPE_LEN    4
#define MAX_DEVICE_CHILD_LEN   2
#define MAX_DEVICE_ID_LEN (MAX_DEVICE_ADDRESS_LEN+MAX_DEVICE_TYPE_LEN+MAX_DEVICE_CHILD_LEN+3)

// Test Handler Device Properties
const char PROP_TEMPLATE_NAME [LSD_PROPERTY_NAME_BUFFER_SIZE] = "test_tmplt";
const char PROP_DEVICE_NAME   [LSD_PROPERTY_NAME_BUFFER_SIZE] = "test_cntrl";
const char PROP_GATEWAY_NAME  [LSD_PROPERTY_NAME_BUFFER_SIZE] = "test_gtway";

// Properties that the test handler creates for a device
const char PROP_DELETE_NAME   [LSD_PROPERTY_NAME_BUFFER_SIZE] = "delet";
const char PROP_RESPOND_NAME  [LSD_PROPERTY_NAME_BUFFER_SIZE] = "respo";
const char PROP_OFFLINE_NAME  [LSD_PROPERTY_NAME_BUFFER_SIZE] = "offln";

// Keys
const char KEY_DEVICE_ID [9] = "deviceId";
const char KEY_TYPE [5] = "type";
const char KEY_MANUFACTURER [4] = "mfr";
const char KEY_MODEL [6] = "model";

// Default values for property creation
#define BOOL_PROP_DEFAULT   false
#define UINT_PROP_DEFAUT    0
#define INT_PROP_DEFAULT    0
#define FLOAT_PROP_DEFAULT  0


/*!****************************************************************************
 * Type Definitions
 *****************************************************************************/

/*!****************************************************************************
 * Private Variables
 *****************************************************************************/

static EnsoDeviceId_t _gwDeviceId;

/*!****************************************************************************
 * Private Function Prototypes
 *****************************************************************************/
static void _MessageQueueListener(MessageQueue_t mq);

static EnsoErrorCode_e _PropertyUpdate(const HandlerId_e subscriberId,
        const EnsoDeviceId_t publishedDeviceId, const PropertyGroup_e propertyGroup,
        const uint16_t numPropertiesChanged, const EnsoPropertyDelta_t* deltasBuffer);

static EnsoErrorCode_e _CreateBoolProperty(EnsoObject_t* owningObject,
        EnsoDeviceId_t* owningDevice, EnsoAgentSidePropertyId_t agentPropertyId,
        const char* cloudPropertyId, bool initialValue);

static EnsoErrorCode_e _CreateBlobProperty(EnsoObject_t* owningObject,
        EnsoDeviceId_t* owningDevice, EnsoAgentSidePropertyId_t agentPropertyId,
        const char* cloudPropertyId);

static EnsoErrorCode_e _CreateStringProperty(EnsoObject_t* owningObject,
        EnsoDeviceId_t* owningDevice, EnsoAgentSidePropertyId_t agentPropertyId,
        const char* cloudPropertyId, char* initialValue);

static EnsoErrorCode_e _CreateIntProperty(EnsoObject_t* owningObject,
        EnsoDeviceId_t* owningDevice, EnsoAgentSidePropertyId_t agentPropertyId,
        const char* cloudPropertyId, int32_t initialValue);

static EnsoErrorCode_e _CreateUIntProperty(EnsoObject_t* owningObject,
        EnsoDeviceId_t* owningDevice, EnsoAgentSidePropertyId_t agentPropertyId,
        const char* cloudPropertyId, uint32_t initialValue);

static EnsoErrorCode_e _CreateFloatProperty(EnsoObject_t* owningObject,
        EnsoDeviceId_t* owningDevice, EnsoAgentSidePropertyId_t agentPropertyId,
        const char* cloudPropertyId, float32_t initialValue);

static EnsoErrorCode_e _SubscribeToProperty(EnsoObject_t* owningObject,
        EnsoDeviceId_t* owningDevice, EnsoAgentSidePropertyId_t agentPropertyId);

static EnsoErrorCode_e _CreateAreaInLocalShadow(void);

static EnsoErrorCode_e _CreateControlProperty(EnsoObject_t* testHandlerObject,
        const char* cloudId);

static EnsoAgentSidePropertyId_t _GenerateAgentID(void);

static bool _FindDeviceId(char* update, EnsoDeviceId_t* device);

static bool _ScanHexValue(char* hexString, uint64_t* value, int maxWidth, char** nextValue);

static EnsoErrorCode_e _CreateDevice(EnsoDeviceId_t device, char* defaults,
        EnsoObject_t** newObject);

static EnsoErrorCode_e _CreateProperties(EnsoObject_t* object, EnsoDeviceId_t* device,
        char* defaults);

static EnsoErrorCode_e _TestHandlerUpdate(const EnsoDeviceId_t* publishedDeviceId,
        const uint16_t numPropertiesChanged, const EnsoPropertyDelta_t* deltasBuffer);

static EnsoErrorCode_e _DeviceUpdate(const EnsoDeviceId_t* publishedDeviceId,
        const uint16_t numPropertiesChanged, const EnsoPropertyDelta_t* deltasBuffer);

static void _ProcessLocalDeviceUpdate(char* update);

static EnsoErrorCode_e _CreateDefaultProperties(EnsoObject_t* object,
        EnsoDeviceId_t* device, char* defaults);

static bool _InterceptDelete(EnsoProperty_t* property, EnsoPropertyValue_u newValue,
        const EnsoDeviceId_t* device);

static bool _InterceptReservedProperty(EnsoProperty_t* property, const char* propertyName,
        EnsoPropertyValue_u newValue, const EnsoDeviceId_t* device);

static uint32_t _GetRespondValueForDevice(const EnsoDeviceId_t* deviceId);

static bool _GetOfflineValueForDevice(const EnsoDeviceId_t* deviceId);

static void _TimerCallbackUpdateProperty(void* p);


/*!****************************************************************************
 * Public Functions
 *****************************************************************************/

/**
 * \name   THA_Init
 * \brief  Test Handler initialisation
 * \return EnsoErrorCode_e:      0 = success else error code
 */
EnsoErrorCode_e THA_Init(void)
{
    // Create message queue for test handler
    const char * name = "/thaDH";

    MessageQueue_t messageQ = OSAL_NewMessageQueue(name, ECOM_MAX_MESSAGE_QUEUE_DEPTH,
        ECOM_MAX_MESSAGE_SIZE);
    if (messageQ == NULL)
    {
        LOG_Error("OSAL_NewMessageQueue(\"%s\", %d, %d) failed", name, ECOM_MAX_MESSAGE_QUEUE_DEPTH, ECOM_MAX_MESSAGE_SIZE);
        return eecInternalError;
    }
    // Create listening thread
    Thread_t listeningThread = OSAL_NewThread(_MessageQueueListener, messageQ);
    if (listeningThread == NULL)
    {
        LOG_Error("OSAL_NewThread() failed for test handler");
        return eecInternalError;
    }

    ECOM_RegisterMessageQueue(TEST_DEVICE_HANDLER, messageQ);

    // Only used on implementations without messaging
    ECOM_RegisterOnUpdateFunction(TEST_DEVICE_HANDLER, _PropertyUpdate);

    return eecNoError;
}


/**
 * \name   THA_Start
 * \brief  Test Handler entry point, called after init
 * \return EnsoErrorCode_e:      0 = success else error code
 */
EnsoErrorCode_e THA_Start(void)
{
    _gwDeviceId.deviceAddress = 0;
    _gwDeviceId.isChild = false;

    // This has to work for us to proceed
    SYS_GetDeviceId(&_gwDeviceId);
    if (_gwDeviceId.deviceAddress == 0) {
        return eecEnsoDeviceNotFound;
    }

    return _CreateAreaInLocalShadow();
}



/*!****************************************************************************
 * Private Functions
 *****************************************************************************/

/**
 * \name   _CreateBlobProperty
 * \brief  Helper function to create a blob property and subscribe to changes
 * \param  owningObject      The owning object
 * \param  owningDevice      The owning device
 * \param  agentPropertyId   ID of property in the test handler
 * \param  cloudPropertyName descriptive ID of property in the cloud
 * \return EnsoErrorCode_e   0 = success else error code
 */
static EnsoErrorCode_e _CreateBlobProperty(EnsoObject_t* owningObject,
        EnsoDeviceId_t* owningDevice, EnsoAgentSidePropertyId_t agentPropertyId,
        const char* cloudPropertyName)
{
    EnsoPropertyValue_u defaultValues[PROPERTY_GROUP_MAX];

    /*
    defaultValues[DESIRED_GROUP].memoryHandle = "";
    defaultValues[REPORTED_GROUP].memoryHandle = "";
    */
    //////// [RE:workaround] Something is wrong with the above. memoryHandle == void*. Also not sure incoming from AWS supports memoryHandle type.
    defaultValues[DESIRED_GROUP].int32Value = 0;
    defaultValues[REPORTED_GROUP].int32Value = 0;

    EnsoErrorCode_e result = LSD_CreateProperty(owningObject, agentPropertyId,
            cloudPropertyName, evBlobHandle, PROPERTY_PUBLIC, false, false, defaultValues);
    if (result == eecNoError)
    {
        result = _SubscribeToProperty(owningObject, owningDevice, agentPropertyId);
        if (result != eecNoError)
        {
            LOG_Error("Failed to subscribe to string property with cloud id %s",
                    cloudPropertyName);
        }
    }
    else
    {
        LOG_Error("Failed to create blob property with cloud id %s", cloudPropertyName);
    }
    return result;
}



/**
 * \name   _SubscribeToProperty
 * \brief  Helper function to subscribe to newly created properties
 * \param  owningObject     The owning object
 * \param  owningDevice     What device does this property belong to
 * \param  agentPropertyId  ID of property in the test handler
 * \return EnsoErrorCode_e  0 = success else error code
 */
static EnsoErrorCode_e _SubscribeToProperty(EnsoObject_t* owningObject,
        EnsoDeviceId_t* owningDevice, EnsoAgentSidePropertyId_t agentPropertyId)
{
    EnsoErrorCode_e result = LSD_SubscribeToDevicePropertyByAgentSideId(owningDevice,
            agentPropertyId, DESIRED_GROUP, TEST_DEVICE_HANDLER, true);

    if (result == eecNoError)
    {
        result = LSD_SubscribeToDevicePropertyByAgentSideId(owningDevice,
                agentPropertyId, REPORTED_GROUP, TEST_DEVICE_HANDLER, true);

        if (result != eecNoError)
        {
            LOG_Error("Failed to subscribe to reported group?");
        }
    }
    else
    {
        LOG_Error("Failed to subscribe to desired group?");
    }

    return result;
}



/**
 * \name   _CreateBoolProperty
 * \brief  Helper function to create a boolean property and subscribe to changes
 * \param  owningObject     The owning object
 * \param  owningDevice     The owning device
 * \param  agentPropertyId  ID of property in the test handler
 * \param  cloudPropertyId  descriptive ID of property in the cloud
 * \param  initialValue     starting value of property
 * \return EnsoErrorCode_e  0 = success else error code
 */
static EnsoErrorCode_e _CreateBoolProperty(EnsoObject_t* owningObject,
        EnsoDeviceId_t* owningDevice, EnsoAgentSidePropertyId_t agentPropertyId,
        const char* cloudPropertyId, bool initialValue)
{
    EnsoPropertyValue_u defaultValues[PROPERTY_GROUP_MAX];
    defaultValues[DESIRED_GROUP].booleanValue = initialValue;
    defaultValues[REPORTED_GROUP].booleanValue = initialValue;

    EnsoErrorCode_e result = LSD_CreateProperty(owningObject, agentPropertyId,
            cloudPropertyId, evBoolean, PROPERTY_PUBLIC, false, false, defaultValues);

    if (result == eecNoError)
    {
        result = _SubscribeToProperty(owningObject, owningDevice, agentPropertyId);

        if (result != eecNoError)
        {
            LOG_Error("Failed to subscribe to bool property with cloud id %s",
                    cloudPropertyId);
        }
    }
    else
    {
        LOG_Error("Failed to create bool property with cloud id %s", cloudPropertyId);
    }

    return result;
}



/**
 * \name   _CreateIntProperty
 * \brief  Helper function to create an int property and subscribe to changes
 * \param  owningObject     The owning object
 * \param  owningDevice     The owning device
 * \param  agentPropertyId  ID of property in the test handler
 * \param  cloudPropertyId  descriptive ID of property in the cloud
 * \param  initialValue     starting value of property
 * \return EnsoErrorCode_e  0 = success else error code
 */
static EnsoErrorCode_e _CreateIntProperty(EnsoObject_t* owningObject,
        EnsoDeviceId_t* owningDevice, EnsoAgentSidePropertyId_t agentPropertyId,
        const char* cloudPropertyId, int32_t initialValue)
{
    EnsoPropertyValue_u defaultValues[PROPERTY_GROUP_MAX];
    defaultValues[DESIRED_GROUP].int32Value = initialValue;
    defaultValues[REPORTED_GROUP].int32Value = initialValue;

    EnsoErrorCode_e result = LSD_CreateProperty(owningObject, agentPropertyId,
            cloudPropertyId, evInt32, PROPERTY_PUBLIC, false, false, defaultValues);

    if (result == eecNoError)
    {
        result = _SubscribeToProperty(owningObject, owningDevice, agentPropertyId);

        if (result != eecNoError)
        {
            LOG_Error("Failed to subscribe to int property with cloud id %s",
                    cloudPropertyId);
        }
    }
    else
    {
        LOG_Error("Failed to create int property with cloud id %s", cloudPropertyId);
    }

    return result;
}



/**
 * \name   _CreateUIntProperty
 * \brief  Helper function to create a uint property and subscribe to changes
 * \param  owningObject     The owning object
 * \param  owningDevice     The owning device
 * \param  agentPropertyId  ID of property in the test handler
 * \param  cloudPropertyId  descriptive ID of property in the cloud
 * \param  initialValue     starting value of property
 * \return EnsoErrorCode_e  0 = success else error code
 */
static EnsoErrorCode_e _CreateUIntProperty(EnsoObject_t* owningObject,
        EnsoDeviceId_t* owningDevice, EnsoAgentSidePropertyId_t agentPropertyId,
        const char* cloudPropertyId, uint32_t initialValue)
{
    EnsoPropertyValue_u defaultValues[PROPERTY_GROUP_MAX];
    defaultValues[DESIRED_GROUP].uint32Value = initialValue;
    defaultValues[REPORTED_GROUP].uint32Value = initialValue;

    EnsoErrorCode_e result = LSD_CreateProperty(owningObject, agentPropertyId,
            cloudPropertyId, evUnsignedInt32, PROPERTY_PUBLIC, false, false, defaultValues);

    if (result == eecNoError)
    {
        result = _SubscribeToProperty(owningObject, owningDevice, agentPropertyId);

        if (result != eecNoError)
        {
            LOG_Error("Failed to subscribe to uint property with cloud id %s",
                    cloudPropertyId);
        }
    }
    else
    {
        LOG_Error("Failed to create uint property with cloud id %s", cloudPropertyId);
    }

    return result;
}




/**
 * \name   _CreateFloatProperty
 * \brief  Helper function to create a uint property and subscribe to changes
 * \param  owningObject     The owning object
 * \param  owningDevice     The owning device
 * \param  agentPropertyId  ID of property in the test handler
 * \param  cloudPropertyId  descriptive ID of property in the cloud
 * \param  initialValue     starting value of property
 * \return EnsoErrorCode_e  0 = success else error code
 */
static EnsoErrorCode_e _CreateFloatProperty(EnsoObject_t* owningObject,
        EnsoDeviceId_t* owningDevice, EnsoAgentSidePropertyId_t agentPropertyId,
        const char* cloudPropertyId, float32_t initialValue)
{
    EnsoPropertyValue_u defaultValues[PROPERTY_GROUP_MAX];
    defaultValues[DESIRED_GROUP].float32Value = initialValue;
    defaultValues[REPORTED_GROUP].float32Value = initialValue;

    EnsoErrorCode_e result = LSD_CreateProperty(owningObject, agentPropertyId,
            cloudPropertyId, evFloat32, PROPERTY_PUBLIC, false, false, defaultValues);

    if (result == eecNoError)
    {
        result = _SubscribeToProperty(owningObject, owningDevice, agentPropertyId);

        if (result != eecNoError)
        {
            LOG_Error("Failed to subscribe to float property with cloud id %s",
                    cloudPropertyId);
        }
    }
    else
    {
        LOG_Error("Failed to create float property with cloud id %s", cloudPropertyId);
    }

    return result;
}



/**
 * \name   _CreateStringProperty
 * \brief  Helper function to create a string property and subscribe to changes
 * \param  owningObject     The owning object
 * \param  owningDevice     The owning device
 * \param  agentPropertyId  ID of property in the test handler
 * \param  cloudPropertyId  descriptive ID of property in the cloud
 * \param  initialValue     starting string value of property
 * \return EnsoErrorCode_e  0 = success else error code
 */
static EnsoErrorCode_e _CreateStringProperty(EnsoObject_t* owningObject,
        EnsoDeviceId_t* owningDevice, EnsoAgentSidePropertyId_t agentPropertyId,
        const char* cloudPropertyId, char* initialValue)
{
    EnsoErrorCode_e result = eecNoError;

    if (initialValue)
    {
        if (strlen(initialValue) < LSD_STRING_PROPERTY_MAX_LENGTH)
        {
            EnsoPropertyValue_u defaultValues[PROPERTY_GROUP_MAX];

            strncpy(defaultValues[DESIRED_GROUP].stringValue, initialValue,
                    LSD_STRING_PROPERTY_MAX_LENGTH);
            strncpy(defaultValues[REPORTED_GROUP].stringValue, initialValue,
                    LSD_STRING_PROPERTY_MAX_LENGTH);

            result = LSD_CreateProperty(owningObject, agentPropertyId,
                    cloudPropertyId, evString, PROPERTY_PUBLIC, false, false, defaultValues);

            if (result == eecNoError)
            {
                result = _SubscribeToProperty(owningObject, owningDevice, agentPropertyId);

                if (result != eecNoError)
                {
                    LOG_Error("Failed to subscribe to string property with cloud id %s",
                            cloudPropertyId);
                }
            }
            else
            {
                LOG_Error("Failed to create string property with cloud id %s", cloudPropertyId);
            }
        }
        else
        {
            LOG_Error("Initials string for property too long");
            result = eecBufferTooSmall;
        }
    }
    else
    {
        LOG_Error("Null pointer supplied as initial value");
        result = eecNullPointerSupplied;
    }

    return result;
}



/**
 * \name _MessageQueueListener
 * \brief  Consumer thread - receives messages on mq
 * \param  mq Message queue on which to listen
 */
static void _MessageQueueListener(MessageQueue_t mq)
{
    LOG_Info("Starting Test Handler Message Queue Listener");

    // For ever loop
    for (; ; )
    {
        char buffer[ECOM_MAX_MESSAGE_SIZE];
        MessagePriority_e priority;

        int size = OSAL_ReceiveMessage(mq, buffer, sizeof(buffer), &priority);
        if (size < 1)
        {
            LOG_Error("ReceiveMessage() error %d", size);
            continue;
        }

        // Read the message id
        uint8_t messageId = buffer[0];
        switch (messageId)
        {
            case ECOM_DELTA_MSG:
                // Note that in future size may not match the ECOM_DeltaMessage_t
                // though it does right now, comment from Carl (7Feb17)
                if (size != sizeof(ECOM_DeltaMessage_t))
                {
                    LOG_Error("Invalid DELTA message: size is %d, expected %d",
                            size, sizeof(ECOM_DeltaMessage_t));
                }
                else
                {
                    ECOM_DeltaMessage_t* pDeltaMessage =
                            (ECOM_DeltaMessage_t*) buffer;

                    // Call PropertyUpdate function so that we work with message
                    // queue and when function is called directly
                    _PropertyUpdate(pDeltaMessage->destinationId, pDeltaMessage->deviceId,
                            pDeltaMessage-> propertyGroup, pDeltaMessage->numProperties,
                            pDeltaMessage->deltasBuffer);
                }
                break;

            default:
                LOG_Error("Unknown message %d", messageId);
                break;
        }
    }

    LOG_Warning("_MessageQueueListener exit");
}



/**
 * \name   _PropertyUpdate
 * \brief  Handler function for property change deltas on the object
 * \param  subscriberId      The subscriber i.e the destination handler
 * \param  publishedDeviceId The device that changed so is publishing the
 *                           update to its subscriber list.
 * \param  propertyGroup     Which group it effects
 * \param  numProperties     The number of properties changed
 * \param  deltasBuffer      Array of changes
 * \return EnsoErrorCode_e 0 = success else error code
 */
static EnsoErrorCode_e _PropertyUpdate(const HandlerId_e subscriberId,
        const EnsoDeviceId_t publishedDeviceId, const PropertyGroup_e propertyGroup,
        const uint16_t numPropertiesChanged, const EnsoPropertyDelta_t* deltasBuffer)
{
    EnsoErrorCode_e retVal = eecNoError;

    if (propertyGroup == REPORTED_GROUP)
    {
        // We are not interested in changes to reported values
        LOG_Info("REPORTED_GROUP update for handler %d : numProperties %d - ignoring",
                subscriberId, numPropertiesChanged);
    }
    else
    {
        // Check whether the published device is the test handler itself
        if (publishedDeviceId.deviceAddress == _gwDeviceId.deviceAddress)
        {
            // It is an update for the test handler directly
            _TestHandlerUpdate(&publishedDeviceId, numPropertiesChanged, deltasBuffer);
        }
        else
        {
            _DeviceUpdate(&publishedDeviceId, numPropertiesChanged, deltasBuffer);
        }
    }
    return retVal;
}



/**
 * \name   _TestHandlerUpdate
 * \brief  These changes are for the Test handler itself, usually an instruction
 *         to create or delete a device
 *
 * \param  publishedDeviceId The device that changed so is publishing the
 *                           update to its subscriber list.
 * \param  numProperties     The number of properties changed
 * \param  deltasBuffer      Array of changes
 * \return EnsoErrorCode_e 0 = success else error code
 */
static EnsoErrorCode_e _TestHandlerUpdate(const EnsoDeviceId_t* publishedDeviceId,
        const uint16_t numPropertiesChanged, const EnsoPropertyDelta_t* deltasBuffer)
{
    // On entry we know that the delta is in the desired group and it is for the
    // test handler itself
    EnsoErrorCode_e retVal = eecNoError;

    uint16_t index = 0;

    while((retVal == eecNoError) && (index < numPropertiesChanged))
    {
        EnsoPropertyDelta_t delta = deltasBuffer[index];

        EnsoProperty_t* property =
                LSD_GetPropertyByAgentSideId(publishedDeviceId, delta.agentSidePropertyID);

        if (property)
        {
            // Test handler responds to changes in the desired group. We process changes
            // on the 'local device' property all others we just map desired -> reported

            switch (property->type.valueType)
            {
                case evInt32:
                case evUnsignedInt32:
                case evFloat32:
                case evBoolean:
                case evString:
                {
                    LOG_Warning("Test handler should have no simple property types? What are we updating?")

                    // Simple update, we have the value in the delta
                    if (LSD_SetPropertyValueByAgentSideId(TEST_DEVICE_HANDLER, publishedDeviceId,
                            REPORTED_GROUP, delta.agentSidePropertyID, delta.propertyValue)
                            != eecNoError)
                    {
                        LOG_Error("Failed to set REPORTED_GROUP for value in %s", property->cloudName);
                        retVal = eecInternalError;
                    }
                }
                break;

                case evBlobHandle:
                {
                    char desiredValue[MAX_TEMPLATE_SIZE];
                    size_t bytesCopied;

                    if (LSD_GetPropertyBufferByCloudName(publishedDeviceId, DESIRED_GROUP,
                            property->cloudName, MAX_TEMPLATE_SIZE, desiredValue, &bytesCopied) == eecNoError)
                    {
                        if (strncmp(property->cloudName, PROP_DEVICE_NAME, LSD_PROPERTY_NAME_BUFFER_SIZE) == 0)
                        {
                            // Process the local device request, desired value will contain a device
                            // ID and a list of key value pairs for this device as a string
                            _ProcessLocalDeviceUpdate(desiredValue);
                        }
                        else
                        {
                            // No processing required, we just copy desired->reported (see below)
                        }

                        // Copy the desired value to the reported value
                        if (LSD_SetPropertyBufferByCloudName(TEST_DEVICE_HANDLER, publishedDeviceId,
                                 REPORTED_GROUP, property->cloudName, bytesCopied, desiredValue, &bytesCopied)
                                 != eecNoError)
                        {
                            LOG_Error("Failed to set REPORTED_GROUP for %s", property->cloudName);
                            retVal = eecInternalError;
                        }
                    }
                    else
                    {
                        LOG_Error("Failed to find DESIRED_GROUP for template property");
                        retVal = eecEnsoDeviceNotFound;
                    }
                }
                break;

                default:
                {
                    LOG_Error("Unsupported property type?");
                    retVal = eecInternalError;
                }
                break;
            }
        }
        else
        {
            LOG_Error("Didn't find the test handler property that we were notified of?");
            retVal = eecPropertyNotFound;
        }

        index++;
    }

    return retVal;
}



/**
 * \name   _DeviceUpdate
 * \brief  A change to the desired property values of a device that the test
 *         handler is managing and therefore we update the reported properties
 *         Note the a future enhancement (G2-1211) will give greater control on these updates
 *
 * \param  publishedDeviceId The device that changed so is publishing the
 *                           update to its subscriber list.
 * \param  numProperties     The number of properties changed
 * \param  deltasBuffer      Array of changes
 * \return EnsoErrorCode_e 0 = success else error code
 */

static EnsoErrorCode_e _DeviceUpdate(const EnsoDeviceId_t* publishedDeviceId,
        const uint16_t numPropertiesChanged, const EnsoPropertyDelta_t* deltasBuffer)
{
    // On entry we know that the delta is in the desired group and it is for a device
    // that the test handler created, not the test handler itself
    EnsoErrorCode_e retVal = eecNoError;

    uint16_t index = 0;

    while((retVal == eecNoError) && (index < numPropertiesChanged))
    {
        EnsoPropertyDelta_t delta = deltasBuffer[index];

        EnsoProperty_t* property =
                LSD_GetPropertyByAgentSideId(publishedDeviceId, delta.agentSidePropertyID);

        if (property)
        {
            // Check if this is a request to delete the device?
            if (!_InterceptDelete(property, delta.propertyValue, publishedDeviceId) &&
                !_InterceptReservedProperty(property, PROP_RESPOND_NAME, delta.propertyValue, publishedDeviceId) &&
                !_InterceptReservedProperty(property, PROP_OFFLINE_NAME, delta.propertyValue, publishedDeviceId))
            {
                 uint32_t respondValue = _GetRespondValueForDevice(publishedDeviceId);
                 bool offlineValue = _GetOfflineValueForDevice(publishedDeviceId);

                 if (offlineValue)
                 {
                     // The device is offline and will not respond
                 }
                 else if (respondValue >= 0xFFFF)
                 {
                     // This value signifies that we ignore the reported value and simulate
                     // being broken
                 }
                 else
                 {
                     // Respond value is the time in seconds in takes us to respond to the desired message
                     ECOM_DeltaMessage_t* message = malloc(sizeof(ECOM_DeltaMessage_t));

                     if (message)
                     {
                         message->numProperties = 1;
                         message->deviceId = *publishedDeviceId;
                         message->propertyGroup = DESIRED_GROUP;
                         message->deltasBuffer[0] = delta;

                         // Optimise this path if respond is zero
                         if (respondValue == 0)
                         {
                             // We are to process this immediately, so bypass the Timer
                             _TimerCallbackUpdateProperty(message);
                         }
                         else
                         {
                             // Delay update by 'respondValue' seconds
                             Handle_t value = message;
                             OSAL_NewTimer(_TimerCallbackUpdateProperty,
                                     respondValue * 1000, false, value);
                         }
                     }
                     else
                     {
                         LOG_Error("Failed to allocate space for message update");
                         retVal = eecInternalError;
                     }
                 }
            }
        }
        else
        {
            LOG_Error("Didn't find property that we were notified of (agent ID = 0x%x?",
                    delta.agentSidePropertyID);
            retVal = eecPropertyNotFound;
        }

        index++;
    }

    return retVal;
}





/**
 * \name   _CreateAreaInLocalShadow
 * \brief  Create control properties
 * \return EnsoErrorCode_e 0 = success else error code
 */
static EnsoErrorCode_e _CreateAreaInLocalShadow(void)
{
    ////////EnsoErrorCode_e result;
    EnsoErrorCode_e result = eecNoError;    //////// [RE:fixed] Inited this

    EnsoObject_t* testHandlerObject = LSD_FindEnsoObjectByDeviceId(&_gwDeviceId);

    if (testHandlerObject)
    {
        // Test handler only has string properties, so use a generic creation function
        result = _CreateControlProperty(testHandlerObject, PROP_TEMPLATE_NAME);

        if (result == eecNoError)
        {
            result = _CreateControlProperty(testHandlerObject, PROP_DEVICE_NAME);

            if (result == eecNoError)
            {
                result = _CreateControlProperty(testHandlerObject, PROP_GATEWAY_NAME);

                if (result != eecNoError)
                {
                    LOG_Error("Failed to create gateway property");
                }
            }
            else
            {
                LOG_Error("Failed to create device property");
            }
        }
        else
        {
            LOG_Error("Failed to create template property");
        }
    }
    else
    {
        LOG_Error("Failed to create Test Handler Object");
    }

    return result;
}



/**
 * \name   _CreateControlProperty
 * \brief  Create control property
 * \param  object     owning object
 * \param  id         cloud id forRemoved  property
 * \return EnsoErrorCode_e 0 = success else error code
 */
static EnsoErrorCode_e _CreateControlProperty(EnsoObject_t* object,
        const char* cloudId)
{
    EnsoErrorCode_e result = eecNullPointerSupplied;

    if (object && cloudId)
    {
        result = _CreateBlobProperty(object, &_gwDeviceId,
                _GenerateAgentID(), cloudId);

        if (result != eecNoError)
        {
            LOG_Error("Failed to create test handler property : %s", cloudId);
        }
    }

    return result;
}



/**
 * \name   _GenerateAgentID
 * \brief  Generate unique increasing agent ID
 * \return agent ID
 */
static EnsoAgentSidePropertyId_t _GenerateAgentID(void)
{
    // Test handler will use descriptive cloud ID string to find properties
    // agent ID can simply be generated sequentially on demand

    // In the future if we need to set a specific agent ID, then this could be
    // sent to the test handler as a key value pair, but currently this is not
    // thought to be needed and agent IDs are internal to the test handler

    static EnsoAgentSidePropertyId_t current = 0xE0001;

    return current++;
}



/**
 * \name   _ProcessLocalDeviceUpdate
 * \brief  Process a request to change state of a local device
 * \param  update     the update string to parse and process
 */
static void _ProcessLocalDeviceUpdate(char* update)
{
    EnsoDeviceId_t device;

    // We find a device id otherwise there is nothing to be done
    if (update)
    {
        if (_FindDeviceId(update, &device))
        {
            // All is well
            LOG_Info("Address = %lx", device.deviceAddress);
            LOG_Info("Type    = %x", device.technology);
            LOG_Info("Child   = %x", device.childDeviceId);

            // Does device exist already?
            EnsoObject_t* object = LSD_FindEnsoObjectByDeviceId(&device);

            if (object)
            {
                LOG_Warning("Device already exists, nothing to do");
            }
            else
            {
                LOG_Info("Device doesn't exist, create it and its properties");

                ////////EnsoObject_t* newObject;
                EnsoObject_t* newObject = NULL; //////// [RE:fixed] Inited this

                // When creating a device, the update string optionally provides
                // defaults to use in device creation
                if (_CreateDevice(device, update, &newObject) == eecNoError)
                {
                    LOG_Info("Create properties for device");

                    if (_CreateProperties(newObject, &device, update) == eecNoError)
                    {
                        if(_CreateDefaultProperties(newObject, &device,update) != eecNoError)
                        {
                            LOG_Error("Failed to create default properties for new device");
                        }
                    }
                    else
                    {
                        LOG_Error("Failed in property creation for device");
                    }
                }
                else
                {
                    LOG_Error("Failed to create device");
                }
            }
        }
        else
        {
            LOG_Error("Device ID was not found, can't process update");
        }
    }
    else
    {
        LOG_Error("No string data to process for local device update?");
    }
}



/**
 * \name   _FindDeviceId
 * \brief  Extract device id from string
 * \return true if the value parsed correctly, false otherwise
 */
static bool _FindDeviceId(char* update, EnsoDeviceId_t* device)
{
    assert(update);
    assert(device);

    // Note : device id is of the form :
    // xxxxxxxx.yyyy.zz
    // where x is a device address in hex, 8 bytes
    // y is a device type in hex, 2 bytes
    // z is a child id in hex, 1 byte
    bool found = true;

    // We must find deviceId in the update string or KVP is invalid
    char deviceId[KVP_MAX_ELEMENT_LENGTH];

    if(KVP_GetString(KEY_DEVICE_ID, update, deviceId, KVP_MAX_ELEMENT_LENGTH))
    {
        uint64_t value;
        char* nextValue;

        // Address first
        if (_ScanHexValue(deviceId, &value, MAX_DEVICE_ADDRESS_LEN, &nextValue))
        {
            device->deviceAddress = value;

            // Type next
            if (_ScanHexValue(nextValue, &value, MAX_DEVICE_TYPE_LEN, &nextValue))
            {
                device->technology = value;

                // Child finally
                if (_ScanHexValue(nextValue, &value, MAX_DEVICE_CHILD_LEN, &nextValue))
                {
                    device->childDeviceId = value;
                }
                else
                {
                    LOG_Error("Failed to parse child value : %s", deviceId);
                    found = false;
                }
            }
            else
            {
                LOG_Error("Failed to parse device handler value : %s", deviceId);
                found = false;
            }
        }
        else
        {
            LOG_Error("Failed to parse device address value : %s", deviceId);
            found = false;
        }
    }
    else
    {
        LOG_Error("key 'deviceId' was not found in list : %s", update);
        found = false;
    }

    return found;
}




/**
 * \name   _ScanHexValue
 * \brief  Convert hex string to uint, scanf can't do 64 bit values
 * \return true if the value parsed correctly, false otherwise
 */
bool _ScanHexValue(char* hexString, uint64_t* value, int maxWidth, char** nextValue)
{
    bool valid = true;
    int index = 0;
    *value = 0;

    if (!hexString || !value || !nextValue)
    {
        LOG_Error("Passed a null value to hex parser?");
        valid = false;
    }

    // In our case, a '.' is also a terminating value
    while (valid && hexString[index] && hexString[index] != '.')
    {
        char c = hexString[index];
        uint8_t nibble = 0;

        if ((c >= '0') && (c <= '9'))
        {
            nibble = c - '0';
        }
        else if ((c >= 'A') && (c <= 'F'))
        {
            nibble = 10 + c - 'A';
        }
        else if ((c >= 'a') && (c <= 'f'))
        {
            nibble = 10 + c - 'a';
        }
        else
        {
            valid = false;
        }

        *value = (*value * 16) + nibble;

        index++;
    }

    if (index > maxWidth)
    {
        // If we went beyond our max width in hex digits, something is wrong
        LOG_Error("Hex string exceeds max width (%i chars) : %s", maxWidth, hexString);
        valid = false;
    }

    if (index == 0 )
    {
        // If we didn't parse any characters, something is wrong
        LOG_Error("Asked to parse an empty hex string : %s", hexString);
        valid = false;
    }

    if (valid)
    {
        // Point to next value, hopping over current terminating value
        *nextValue = hexString + index + 1;
    }

    return valid;
}



/**
 * \name   _CreateDevice
 * \brief  Create a new device
 * \param  device    The device
 * \param  defaults  defaults to use
 * \param  newObject the new object we will create
 * \return EnsoErrorCode_e 0 = success else error code
 */
static EnsoErrorCode_e _CreateDevice(EnsoDeviceId_t device, char* defaults,
        EnsoObject_t** newObject)
{
    if (!defaults || ! newObject)
    {
        return eecNullPointerSupplied;
    }

    EnsoErrorCode_e result = eecEnsoObjectNotCreated;

    char deviceId[MAX_DEVICE_ID_LEN];
    int32_t type;

    if (KVP_GetString(KEY_DEVICE_ID, defaults, deviceId, MAX_DEVICE_ID_LEN))
    {
        if (KVP_GetInt(KEY_TYPE, defaults, &type))
        {
            const int MaxStringSize = 32;
            char manufacturer[MaxStringSize];

            if (KVP_GetString(KEY_MANUFACTURER, defaults, manufacturer, MaxStringSize))
            {
                char model[MaxStringSize];

                if (KVP_GetString(KEY_MODEL, defaults, model, MaxStringSize))
                {
                    // Have all we need to create the object
                    *newObject = LSD_CreateEnsoObject(device);

                    if (*newObject)
                    {
                        // Create the owner (persistent private) property.
                        EnsoPropertyValue_u handlerValue[PROPERTY_GROUP_MAX];
                        handlerValue[DESIRED_GROUP].uint32Value = TEST_DEVICE_HANDLER;
                        handlerValue[REPORTED_GROUP].uint32Value = TEST_DEVICE_HANDLER;

                        result = LSD_CreateProperty(
                                *newObject,
                                PROP_OWNER_ID,
                                PROP_OWNER_INTERNAL_NAME,
                                evUnsignedInt32,
                                PROPERTY_PRIVATE,
                                false,
                                true,
                                handlerValue);
                        if (result < 0)
                        {
                            LOG_Error("Failed to create owner property %s", LSD_EnsoErrorCode_eToString(result));
                        }

                        // Create the connection (private) property.
                        EnsoPropertyValue_u connValue[PROPERTY_GROUP_MAX];
                        connValue[DESIRED_GROUP].int32Value = -1;
                        connValue[REPORTED_GROUP].int32Value = -1;

                        result = LSD_CreateProperty(
                                *newObject,
                                PROP_CONNECTION_ID,
                                PROP_CONNECTION_ID_CLOUD_NAME,
                                evInt32,
                                PROPERTY_PRIVATE,
                                false,
                                false,
                                connValue);
                        if (result < 0)
                        {
                            LOG_Error("Failed to create connection property %s", LSD_EnsoErrorCode_eToString(result));
                        }

                        // Create the type property.
                        EnsoPropertyValue_u typeValue[PROPERTY_GROUP_MAX];
                        typeValue[DESIRED_GROUP].uint32Value = type;
                        typeValue[REPORTED_GROUP].uint32Value = type;

                        result = LSD_CreateProperty(*newObject,
                                PROP_TYPE_ID,
                                PROP_TYPE_CLOUD_NAME,
                                evUnsignedInt32,
                                PROPERTY_PUBLIC,
                                false,
                                false,
                                typeValue);

                        if (result < 0)
                        {
                            LOG_Error("Failed to create type property %d", result);
                        }

                        // Create the device status private property.
                        EnsoPropertyValue_u deviceStatusValue[PROPERTY_GROUP_MAX];
                        deviceStatusValue[DESIRED_GROUP].uint32Value = THING_CREATED;
                        deviceStatusValue[REPORTED_GROUP].uint32Value = THING_CREATED;

                        result = LSD_CreateProperty(*newObject,
                                PROP_DEVICE_STATUS_ID,
                                PROP_DEVICE_STATUS_CLOUD_NAME,
                                evUnsignedInt32,
                                PROPERTY_PRIVATE,
                                false,
                                false,
                                deviceStatusValue);
                        if (result < 0)
                        {
                            LOG_Error("Failed to create device status property %d", result);
                        }

                        result = LSD_RegisterEnsoObject(*newObject);

                        if (result < 0)
                        {
                            LOG_Error("Failed to register new device");
                        }
                    }
                    else
                    {
                        LOG_Error("Failed to create new device object");
                    }
                }
                else
                {
                    LOG_Error("Model missing from template for device creation");
                }
            }
            else
            {
                LOG_Error("Manufacturer missing from template for device creation");
            }
        }
        else
        {
            LOG_Error("Type missing from template for device creation");
        }
    }
    else
    {
        LOG_Error("Device ID missing from template for device creation");
    }

    return result;
}



/**
 * \name   _CreateProperties
 * \brief  Create properties for our new device
 * \param  object    The object
 * \param  device    The device
 * \param  defaults  defaults to use
 * \return EnsoErrorCode_e 0 = success else error code
 */
static EnsoErrorCode_e _CreateProperties(EnsoObject_t* object, EnsoDeviceId_t* device, char* defaults)
{
    char template[MAX_TEMPLATE_SIZE];
    size_t bytesCopied;

    // Fetch the template to iterate over
    EnsoErrorCode_e result = LSD_GetPropertyBufferByCloudName(&_gwDeviceId,
            DESIRED_GROUP, PROP_TEMPLATE_NAME, MAX_TEMPLATE_SIZE, template, &bytesCopied);

    if (result == eecNoError)
    {
        char keyBuffer[KVP_MAX_ELEMENT_LENGTH];
        const char* next = template;

        do
        {
            next = KVP_GetNextKey(next, keyBuffer, KVP_MAX_ELEMENT_LENGTH);

            if (keyBuffer[0])
            {
                char propertyType[KVP_MAX_ELEMENT_LENGTH];

                // We have a key, get type from template
                if (KVP_GetString(keyBuffer, template, propertyType, KVP_MAX_ELEMENT_LENGTH))
                {
                    if (strcmp(propertyType, "bool") == 0)
                    {
                        bool initialValue;

                        // Use provided value if we find one, no matter if not
                        if (KVP_GetBool(keyBuffer, defaults, &initialValue))
                        {
                            LOG_Info("Create property : %s as %s with %i", keyBuffer, propertyType,
                                initialValue);
                        }
                        else
                        {
                            LOG_Info("Create property : %s as %s with default value", keyBuffer,
                                    propertyType);
                            initialValue = BOOL_PROP_DEFAULT;
                        }

                        result = _CreateBoolProperty(object, device, _GenerateAgentID(), keyBuffer,
                                initialValue);
                    }
                    else if (strcmp(propertyType, "string") == 0)
                    {
                        char initialValue[KVP_MAX_ELEMENT_LENGTH];

                        // Use provided value if we find one, otherwise use empty string
                        if (!KVP_GetString(keyBuffer, defaults, initialValue, KVP_MAX_ELEMENT_LENGTH))
                        {
                            initialValue[0] = 0;
                        }

                        result = _CreateStringProperty(object, device, _GenerateAgentID(), keyBuffer,
                                initialValue);

                        LOG_Info("Create property : %s as %s", keyBuffer, initialValue);
                    }
                    else if (strcmp(propertyType, "uint") == 0)
                    {
                        int32_t initialValue;

                        // Use provided value if we find one, no matter if not
                        if (KVP_GetInt(keyBuffer, defaults, &initialValue))
                        {
                            LOG_Info("Create property : %s as %s with %i", keyBuffer, propertyType,
                                initialValue);
                        }
                        else
                        {
                            LOG_Info("Create property : %s as %s with default value", keyBuffer,
                                    propertyType);
                            initialValue = UINT_PROP_DEFAUT;
                        }

                        result = _CreateUIntProperty(object, device, _GenerateAgentID(), keyBuffer,
                                (uint32_t)initialValue);
                    }
                    else if (strcmp(propertyType, "int") == 0)
                    {
                        int32_t initialValue;

                        // Use provided value if we find one, no matter if not
                        if (KVP_GetInt(keyBuffer, defaults, &initialValue))
                        {
                            LOG_Info("Create property : %s as %s with %i", keyBuffer, propertyType,
                                initialValue);
                        }
                        else
                        {
                            LOG_Info("Create property : %s as %s with default value", keyBuffer,
                                    propertyType);
                            initialValue = INT_PROP_DEFAULT;
                        }

                        result = _CreateIntProperty(object, device, _GenerateAgentID(), keyBuffer,
                                initialValue);
                    }
                    else if (strcmp(propertyType, "float") == 0)
                    {
                        float initialValue;

                        // Use provided value if we find one, no matter if not
                        if (KVP_GetFloat(keyBuffer, defaults, &initialValue))
                        {
                            LOG_Info("Create property : %s as %s with %f", keyBuffer, propertyType,
                                initialValue);
                        }
                        else
                        {
                            LOG_Info("Create property : %s as %s with default value", keyBuffer,
                                    propertyType);
                            initialValue = FLOAT_PROP_DEFAULT;
                        }

                        result = _CreateFloatProperty(object, device, _GenerateAgentID(), keyBuffer,
                                initialValue);
                    }
                    else
                    {
                        LOG_Error("Unsupported property type : %s", propertyType);
                        result = eecFunctionalityNotSupported;
                    }
                }
            }
        }
        while ((result == eecNoError) && next);
    }

    return result;
}



/**
 * \name   _CreateDefaultProperties
 * \brief  Create default properties that all test handler generated devices have
 * \param  object    The object
 * \param  device    The device
 * \param  defaults  defaults to use
 * \return EnsoErrorCode_e 0 = success else error code
 */
static EnsoErrorCode_e _CreateDefaultProperties(EnsoObject_t* object, EnsoDeviceId_t* device,
        char* defaults)
{
    // Device has a delete property that, if set, will trigger device deletion
    EnsoErrorCode_e result = _CreateBoolProperty(object, device, _GenerateAgentID(),
            PROP_DELETE_NAME, false);

    if (result == eecNoError)
    {
        result = _CreateIntProperty(object, device, _GenerateAgentID(),
                PROP_RESPOND_NAME, 0);

        if (result == eecNoError)
        {
            result = _CreateBoolProperty(object, device, _GenerateAgentID(),
                    PROP_OFFLINE_NAME, false);

            if (result != eecNoError)
            {
                LOG_Error("Failed to create offline property?");
            }
        }
        else
        {
            LOG_Error("Failed to create respo property?");
        }
    }
    else
    {
        LOG_Error("Failed to create delet property?");
    }

    return result;
}



/**
 * \name   _InterceptDelete
 * \brief  Check whether device property update is actually a request to delete device
 * \param  property  The property to check
 * `parma  newValue  The name value
 * \param  device    The owning device
 * \return bool      true if we are deleting the device
 */
static bool _InterceptDelete(EnsoProperty_t* property, EnsoPropertyValue_u newValue,
        const EnsoDeviceId_t* device)
{
    bool result = false;

    if (strncmp(property->cloudName, PROP_DELETE_NAME, LSD_PROPERTY_NAME_BUFFER_SIZE) == 0)
    {
        // It is the delete property
        if (newValue.booleanValue)
        {
            // We are to delete this device
            LSD_DestroyEnsoDevice(*device);
            result = true;
        }
    }

    return result;
}



/**
 * \name   _InterceptReservedProperty
 * \brief  Check whether device property update is for a reserved property
 * \param  property  The property to check
 * \param  property  The property to look for
 * `parma  newValue  The new value
 * \param  device    The owning device
 * \return bool      true if it was a reserved property
 */
static bool _InterceptReservedProperty(EnsoProperty_t* property, const char* propertyName,
        EnsoPropertyValue_u newValue, const EnsoDeviceId_t* device)
{
    bool result = false;

    if (strncmp(property->cloudName, propertyName, LSD_PROPERTY_NAME_BUFFER_SIZE) == 0)
    {
        // It is a reserved property, set the value immediately
        if (LSD_SetPropertyValueByAgentSideId(TEST_DEVICE_HANDLER, device,
            REPORTED_GROUP, property->agentSidePropertyID, newValue) != eecNoError)
        {
            LOG_Error("Failed to set REPORTED_GROUP for %s property?", propertyName);
        }

        result = true;
    }

    return result;
}



/**
 * \name   _GetRespondValueForDevice
 * \brief  Check whether device property update is for respond property
 * \param  deviceId  The owning device
 * \return uint32_t  The reported value of the property or zero on default
 */
static uint32_t _GetRespondValueForDevice(const EnsoDeviceId_t* deviceId)
{
    uint32_t result = 0;

    EnsoProperty_t* respond = LSD_GetPropertyByCloudName(deviceId, PROP_RESPOND_NAME);

    if (respond)
    {
        result = respond->reportedValue.uint32Value;
    }
    else
    {
        LOG_Error("Unable to find respond property for group?");
    }

    return result;
}



/**
 * \name   _GetOfflineValueForDevice
 * \brief  Check whether this device is declared to be offline
 * \param  deviceId  The owning device
 * \return uint32_t  The offline value of the device or false on default
 */
static bool _GetOfflineValueForDevice(const EnsoDeviceId_t* deviceId)
{
    bool result = false;

    EnsoProperty_t* respond = LSD_GetPropertyByCloudName(deviceId, PROP_OFFLINE_NAME);

    if (respond)
    {
        result = respond->reportedValue.booleanValue;
    }
    else
    {
        LOG_Error("Unable to find offline property for group?");
    }

    return result;
}



/**
 * \name   _TimerCallBackUpdateProperty
 * \brief  When timer expires update reported value for the property
 * \param  p     pointer to copy of delta message block
 */
static void _TimerCallbackUpdateProperty(void* p)
{
    if (!p)
    {
        LOG_Error("Pointer invalid from timer?");
        return;
    }

    ECOM_DeltaMessage_t* message = (ECOM_DeltaMessage_t*) p;

    if (message->numProperties != 1)
    {
        LOG_Warning("We are only expecting 1 property value in callback?");
    }

    // Is this update simple?
    EnsoProperty_t* property =
            LSD_GetPropertyByAgentSideId(&message->deviceId, message->deltasBuffer[0].agentSidePropertyID);

    if (property)
    {
        switch (property->type.valueType)
        {
            case evInt32:
            case evUnsignedInt32:
            case evFloat32:
            case evBoolean:
            case evString:
            {
                // Simple update, we have the value in the delta
                if (LSD_SetPropertyValueByAgentSideId(TEST_DEVICE_HANDLER, &message->deviceId,
                    REPORTED_GROUP, message->deltasBuffer[0].agentSidePropertyID,
                    message->deltasBuffer[0].propertyValue) != eecNoError)
                {
                    LOG_Error("Failed to set REPORTED_GROUP for value in %s", property->cloudName);
                }
            }
            break;

            case evBlobHandle:
            {
                // Object update, read first and then copy
                char desiredValue[MAX_TEMPLATE_SIZE];
                size_t bytesCopied;

                if (LSD_GetPropertyBufferByAgentSideId(&message->deviceId, DESIRED_GROUP,
                    message->deltasBuffer[0].agentSidePropertyID, MAX_TEMPLATE_SIZE, desiredValue, &bytesCopied)
                    == eecNoError)
                {
                    // Copy the desired value to the reported value
                    if (LSD_SetPropertyBufferByAgentSideId(TEST_DEVICE_HANDLER, &message->deviceId,
                            REPORTED_GROUP, message->deltasBuffer[0].agentSidePropertyID, MAX_TEMPLATE_SIZE,
                            desiredValue, &bytesCopied) != eecNoError)
                    {
                        LOG_Error("Failed to set REPORTED_GROUP for string in %s", property->cloudName);
                    }
                }
                else
                {
                    LOG_Error("Failed to find DESIRED_GROUP for %s", property->cloudName);
                }
            }
            break;

            default:
            {
                LOG_Error("Unsupported property type?");
            }
            break;
        }
    }
    else
    {
        LOG_Error("Didn't find the test handler property that we were notified of?");
    }

    free(p);
}
