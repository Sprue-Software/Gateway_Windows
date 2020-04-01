/*!****************************************************************************
 *
 * \file UPG_Api.c
 *
 * \author Rhod Davies
 *
 * \brief Upgrade Handler Implementation
 *
 * \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 *
 *****************************************************************************/
#include "UPG_Api.h"
#include "UPG_Upgrader.h"
#include "LOG_Api.h"
#include "LSD_Api.h"
#include "ECOM_Api.h"
#include "ECOM_Messages.h"
#include "HAL.h"
#include "LSD_Api.h"
#include "APP_Types.h"
#include "SYS_Gateway.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Check FW Name as default */
#ifndef CHECK_UPG_FWNAM
#define CHECK_UPG_FWNAM         0
#endif

#if CHECK_UPG_FWNAM
#define VALID_FW_NAME(new, old) ((new) != NULL && (old != NULL) && strcmp((new), (old)) != 0)
#else
#define VALID_FW_NAME(...)   (1)    // True
#endif

static EnsoDeviceId_t gwid;

void UPG_Handler(MessageQueue_t mq);

/**
 * \name UPG_Init
 *
 * \brief Initialise the Upgrade Handler
 *
 * \return EnsoErrorCode_e
 */
EnsoErrorCode_e UPG_Init(void)
{
    SYS_GetDeviceId(&gwid);
    const char * name = "/upgDH";
    LOG_Trace("Upgrade Handler %s", name);
    MessageQueue_t mq = OSAL_NewMessageQueue(name, ECOM_MAX_MESSAGE_QUEUE_DEPTH, ECOM_MAX_MESSAGE_SIZE);
    if (mq == NULL)
    {
        LOG_Error("OSAL_NewMessageQueue(\"%s\", %d, %d) failed", name, ECOM_MAX_MESSAGE_QUEUE_DEPTH, ECOM_MAX_MESSAGE_SIZE);
        return eecInternalError;
    }
    Thread_t listeningThread = OSAL_NewThread(UPG_Handler, mq);
    if (listeningThread == 0)
    {
        LOG_Error("OSAL_NewThread failed for upgDH");
        return eecInternalError;
    }
    ECOM_RegisterMessageQueue(UPGRADE_HANDLER, mq);
    return eecNoError;
}


/**
 * \brief   Return string representation of state
 *
 * \param   state
 *
 * \return  char * pointing to null terminated string representation of state
 */
const char * UPG_State2s(uint32_t state)
{
    return state == UPGRADE_COMPLETE ? "Complete" :
           state == UPGRADE_STARTED  ? "Started"  :
           state == UPGRADE_BOOTING  ? "Booting"  :
           state == UPGRADE_FAILED   ? "Failed"   : "Unknown";
}

/**
 * \brief   Set Error helper for upgrd_error
 *
 * \param   error new percentage for upgrd_error
 */
void UPG_setError(uint32_t error)
{
    EnsoPropertyValue_u val = { .uint32Value = error };
    EnsoErrorCode_e ret = LSD_SetPropertyValueByAgentSideId(UPGRADE_HANDLER, &gwid, REPORTED_GROUP, PROP_UPGRD_PROG_ID, val);
    LOG_Trace("%d: %s", error, LSD_EnsoErrorCode_eToString(ret));
}

/**
 * \brief   Set Progress helper for upgrd_prog
 *
 * \param   percent new percentage for upgrd_prog
 */
void UPG_setProgress(uint32_t percent)
{
    EnsoPropertyValue_u val = { .uint32Value = percent };
    EnsoErrorCode_e ret = LSD_SetPropertyValueByAgentSideId(UPGRADE_HANDLER, &gwid, REPORTED_GROUP, PROP_UPGRD_PROG_ID, val);
    LOG_Trace("%d%%: %s", percent, LSD_EnsoErrorCode_eToString(ret));
}

/**
 * \brief   Set Retry helper for upgrd_retry
 *
 * \param   retry new retry count for upgrd_retry
 */
void UPG_setRetry(uint32_t retry)
{
    EnsoPropertyValue_u val = { .uint32Value = retry };
    EnsoErrorCode_e ret = LSD_SetPropertyValueByAgentSideId(UPGRADE_HANDLER, &gwid, REPORTED_GROUP, PROP_UPGRD_PROG_ID, val);
    LOG_Trace("%d%%: %s", retry, LSD_EnsoErrorCode_eToString(ret));
}

/**
 * \brief   Set State helper for upgrd_state
 *
 * \param   state new state for upgrd_state
 */
void UPG_setState(uint32_t state)
{
    EnsoPropertyValue_u val = { .uint32Value = state };
    EnsoErrorCode_e ret = LSD_SetPropertyValueByAgentSideId(UPGRADE_HANDLER, &gwid, REPORTED_GROUP, PROP_UPGRD_STATE_ID, val);
    LOG_Trace("%d %s: %s", state, UPG_State2s(state), LSD_EnsoErrorCode_eToString(ret));
}

/**
 * \brief   Set Status helper for upgrd_size
 *
 * \param   size new size for upgrd_size
 */
void UPG_setSize(uint32_t size)
{
    EnsoPropertyValue_u val = { .uint32Value = size };
    EnsoErrorCode_e ret = LSD_SetPropertyValueByAgentSideId(UPGRADE_HANDLER, &gwid, REPORTED_GROUP, PROP_UPGRD_SIZE_ID, val);
    LOG_Trace("%d: %s", size, LSD_EnsoErrorCode_eToString(ret));
}

/**
 * \brief   Set Status helper for upgrd_prog
 *
 * \param   status new status for upgrd_statu
 */
void UPG_setStatus(uint32_t status)
{
    EnsoPropertyValue_u val = { .uint32Value = status };
    EnsoErrorCode_e ret = LSD_SetPropertyValueByAgentSideId(UPGRADE_HANDLER, &gwid, REPORTED_GROUP, PROP_UPGRD_STATUS_ID, val);
    LOG_Trace("%d: %s", status, LSD_EnsoErrorCode_eToString(ret));
}

/**
 * \brief   Upgrade Handler - receives messages on mq
 *
 * \param   mq Message queue on which to listen.
 */
void UPG_Handler(MessageQueue_t mq)
{
    LOG_Trace("Upgrade Handler");
    char *   url   = NULL;
    char *   hash  = NULL;
    char *   key   = NULL;
    char *   fwnam = NULL;
    uint32_t trgrd = 0;
    for (;;)
    {
        static char msg[ECOM_MAX_MESSAGE_SIZE];
        MessagePriority_e pri;
        int size = OSAL_ReceiveMessage(mq, msg, sizeof msg, &pri);
        uint8_t messageId = msg[0];
        ECOM_DeltaMessage_t * dm = (ECOM_DeltaMessage_t*)msg;
        if (size < 1)
        {
            LOG_Error("ReceiveMessage error %d", size);
        }
        else if (messageId != ECOM_DELTA_MSG)
        {
            LOG_Error("Expected ECOM_DELTA_MSG(%d), got %d", ECOM_DELTA_MSG, messageId);
        }
        else if (size != sizeof *dm)
        {
            LOG_Error("Expected ECOM_DELTA_MSG size(%d), got %d", sizeof * dm, size);
        }
        else if (DESIRED_GROUP != dm->propertyGroup)
        {
            LOG_Error("Expected DESIRED_GROUP(%d), got %d", DESIRED_GROUP, dm->propertyGroup);
        }
        else if (UPGRADE_HANDLER != dm->destinationId)
        {
            LOG_Error("Expected UPGRADE_HANDLER(%d), got %d", UPGRADE_HANDLER, dm->destinationId);
        }
        else
        {
            for (int i = 0; i < dm->numProperties; i++)
            {   
                EnsoPropertyDelta_t    delta = dm->deltasBuffer[i];
                EnsoAgentSidePropertyId_t id = delta.agentSidePropertyID;
                EnsoProperty_t * prop = LSD_GetPropertyByAgentSideId(&gwid, delta.agentSidePropertyID);
                char name[LSD_PROPERTY_NAME_BUFFER_SIZE] = "";
                LSD_GetPropertyCloudNameFromAgentSideId(&gwid, id, sizeof name, name);
                MemoryHandle_t memoryHandle = delta.propertyValue.memoryHandle;
                if (prop->type.valueType == evBlobHandle && memoryHandle != NULL)
                {
                    size_t size = OSAL_GetBlockSize(memoryHandle);
                    MemoryHandle_t new = OSAL_MemoryRequest(NULL, size);
                    memcpy(new, memoryHandle, size);
                    delta.propertyValue.memoryHandle = memoryHandle = new;
                }
                static char buf[1024] = "";
                int cnt = 0;
                LSD_ConvertTypedDataToJsonValue(&buf[cnt], sizeof buf - cnt, &cnt, prop->desiredValue, prop->type.valueType);
                LOG_Trace("Property 0x%08x %s:%s", id, name, buf);
                if (id != PROP_FWNAM_ID && eecNoError != LSD_SetPropertiesOfDevice(dm->destinationId, &gwid, REPORTED_GROUP, &delta, 1))
                {
                    LOG_Error("LSD_SetPropertiesOfDevice failed for property 0x%08x %s", id, name);
                }
                else
                {
                    switch (id)
                    {
                    case PROP_FWNAM_ID:
                        fwnam = memoryHandle;
                        LOG_Trace("fwnam: %s", fwnam);
                        break;
                    case PROP_UPGRD_URL_ID:
                        url = memoryHandle;
                        LOG_Trace("url: %s", url);
                        break;
                    case PROP_UPGRD_KEY_ID:
                        key = memoryHandle;
                        LOG_Trace("key: %s", key);
                        break;
                    case PROP_UPGRD_HASH_ID:
                        hash = memoryHandle;
                        LOG_Trace("hash: %s", hash);
                        break;
                    case PROP_UPGRD_TRGRD_ID:
                        trgrd = delta.propertyValue.uint32Value;
                        LOG_Trace("trgrd: %u", trgrd);
                        break;
                    }
                }
            }
            LOG_Trace("url:%s key:%s hash:%s fwnam:%s trgrd:%u",
                      url  ? url  : "", key   ? key   : "",
                      hash ? hash : "", fwnam ? fwnam : "", trgrd);
           //if (url && *url && key && *key && hash && *hash && VALID_FW_NAME(fwnam, gw_fwnam) && trgrd)
            if (url)
            {
                LOG_Trace("Started url: %s key: %s hash: %s", url, key, hash);
                UPG_setState(UPGRADE_STARTED);
                LSD_DumpObjectStore();
                int ret = UPG_Upgrader(url, key, hash);
                UPG_setState(ret == 0 ? UPGRADE_BOOTING : UPGRADE_FAILED);
                trgrd = 0;
            }
        }
    }
}
