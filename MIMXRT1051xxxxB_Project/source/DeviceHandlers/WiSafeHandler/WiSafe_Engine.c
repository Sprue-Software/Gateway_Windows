/*!****************************************************************************
 *
 * \file WiSafe_Engine.c
 *
 * \author James Green
 *
 * \brief WiSafe core engine that processes received messages and timers.
 *
 * \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 *
 *****************************************************************************/

#include <string.h>

#include "LOG_Api.h"
#include "LSD_Api.h"
#include "ECOM_Api.h"
#include "WiSafe_Engine.h"
#include "WiSafe_RadioComms.h"
#include "Sprue/spruerm_protocol.h"
#include "WiSafe_Protocol.h"
#include "WiSafe_Discovery.h"
#include "WiSafe_DAL.h"
#include "WiSafe_Event.h"
#include "SYS_Gateway.h"



#define REAL_TEST_MESSAGE   true       /*ABR added*/
/**
 * \brief Send notification to LED Handler to flash Learn LED
 *
 * \param none
 *
 * \return none
 */
static void Wisafe_FlashLearnLED(void)
{
    EnsoErrorCode_e ensoError;

    /* Get "In Network" property */
    EnsoPropertyValue_u inNetwProperty = WiSafe_DALGetReportedProperty(GATEWAY_DEVICE_ID, DAL_PROPERTY_IN_NETWORK_IDX, &ensoError);
    if (ensoError != eecNoError)
    {
        LOG_Error("Failed to get In-Network Property. Error %d", ensoError);
        return;
    }

    /*
     * If "In Network" flags is not fired, it means SID map is empty so need to
     * flash Learn LED to indicate whether first learn.
     */
    if (inNetwProperty.booleanValue == false)
    {
        EnsoDeviceId_t deviceId = EnsoDeviceFromWiSafeID(GATEWAY_DEVICE_ID);

        /* Flash LED three times */
        EnsoPropertyValue_u flashCountValue = {
                .uint32Value = 3
        };

        /* Create a delta */
        EnsoPropertyDelta_t delta =
        {
                .agentSidePropertyID = PROP_LEARN_LED_FLASH_ID,
                .propertyValue = flashCountValue
        };

        /* Send LED flash info to LED Handler */
        EnsoErrorCode_e retVal = ECOM_SendUpdateToSubscriber(LED_DEVICE_HANDLER,
                                                             deviceId,
                                                             REPORTED_GROUP,
                                                             1,
                                                             &delta);

        if (retVal != eecNoError)
        {
            LOG_Error("Failed to send Learn Led Update to subscribers! Error %d", retVal);
        }
    }
}
/**
 * Helper to handle an RM Diagnostic Result message received over SPI.
 *
 * @param buf The buffer containing the message received over SPI.
 */
static void HandleRMDiagnosticResult(radioCommsBuffer_t* buf)
{
    msgRMDiagnosticResult_t msg = WiSafe_DecodeRMDiagnosticResult(buf);
    gateWaySid = msg.sid; // ABR added
    LOG_Info("RM Diagnostic Result received from device 0x%06x, updating shadow.", msg.id);

    EnsoPropertyValue_u valueFC = { .uint32Value = msg.radioFaultCount };
    WiSafe_DALSetReportedProperty(msg.id, DAL_PROPERTY_DEVICE_RADIO_FAULT_COUNT_IDX, DAL_PROPERTY_DEVICE_RADIO_FAULT_COUNT, evUnsignedInt32, valueFC);

    EnsoPropertyValue_u valueL = { .uint32Value = msg.lastRSSI };
    WiSafe_DALSetReportedProperty(msg.id, DAL_PROPERTY_DEVICE_RADIO_RSSI_IDX, DAL_PROPERTY_DEVICE_RADIO_RSSI, evUnsignedInt32, valueL);

    LOG_Info("RM Firmware version: %x", msg.firmwareRevision);
}

/**
 * For the given device, see if this sequence number represents a
 * duplicate packet or not. At the same time, use the provided
 * sequence number to update our copy of the last sequence number.
 *
 * @param id The device ID.
 * @param seq The sequene number to check.
 *
 * @return True if the packet is ok to use, false if it's a duplicate
 * and should be ignored.
 */
static bool CheckDuplicateAndUpdateSeq(deviceId_t id, seq_t seq)
{
    bool result;

    /* See if we have the last sequence number that we received? */
    EnsoErrorCode_e error;
    EnsoPropertyValue_u mostRecentSeq = WiSafe_DALGetReportedProperty(id, DAL_PROPERTY_DEVICE_LAST_SEQUENCE_IDX, &error);
    if (error == eecNoError)
    {
        /* Is it a duplicate? */
        result = (mostRecentSeq.uint32Value != seq);
    }
    else
    {
        /* We don't have one, so we can accept this packet. */
        result = true;
    }

    /* Remember this sequence number. */
    EnsoPropertyValue_u value = { .uint32Value = seq };
    WiSafe_DALSetReportedProperty(id, DAL_PROPERTY_DEVICE_LAST_SEQUENCE_IDX, DAL_PROPERTY_DEVICE_LAST_SEQUENCE, evUnsignedInt32, value);

    return result;
}

/**
 * Process an incoming buffer.
 *
 * G2-556 : Do we keep track of the state of the system ?
 * For example device tested message should only be received while wisafe is in learning mode? Is that check?
 * What if the cloud has disabled learning mode for example? Or if the timer that sets the learning mode timeout has expired?
 *
 * @param buf
 */
void WiSafe_EngineProcessRX(radioCommsBuffer_t* buf)
{
    if ((buf != NULL) && (buf->count >= 1))
    {
        switch(buf->data[0])
        {
            /* An ACK. */
            case (SPRUERM_CMD_OF(SPRUERM_ACK)):
            {
                /* Just eat it. */
                break;
            }

            /* Status request received. */
            case (SPRUERM_CMD_OF(SPRUERM_STATUS_REQ)):
            {
                /* We have received a "Status Request" so send an "Alarm Identification" in response. */
                msgAlarmIdent_t response = WiSafe_NewAlarmIdent();

                /* Set flags. */
                response.batteryFaultRM = false;
                response.acFailed = false; // Can't be false or else we'd be off.
                response.batteryFaultSD = false;
                response.isOnBase = false;
                response.faulty = false;
                response.calibrated= true; // Always true.

                /* Transmit the response. */
                radioCommsBuffer_t* buffer = WiSafe_EncodeAlarmIdent(&response);
                if (buffer != NULL)
                {
                    WiSafe_RadioCommsTx(buffer);
                }
                else
                {
                    LOG_Error("No free buffers to send 'Alarm Indentification' response.");
                }

                break;
            }

            /* Ext Diag Response received - this message type has a
               subcommand number enabling a variety of different
               messages, so need to check the subcommand to find out
               which. */

            // G2-556: Quite a few magic numbers
            case (SPRUERM_CMD_OF(SPRUERM_RM_EXTDIAG_RESPONSE)):
            {
                if (buf->count >= 2)
                {
                    /* Examine the subcommand number. */
                    switch (buf->data[1])
                    {
                        /* We have received a SID map in response to our 'request SID map' message. */
                        case sprueOptSidMap:
                        {
                            if (buf->count == 10)
                            {
                                /* Pass on to discovery module. */
                                WiSafe_DiscoveryProcessSIDMap(&(buf->data[2]));
                            }
                            else
                            {
                                LOG_Error("Received SID map response with bad length.");
                            }
                            break;
                        }

                        /* We have received a missing node report in response to our 'SID map update' message. */
                        case sprueOptMissingNode:
                        {
                            if (buf->count == 11)
                            {
                                /* Pass on to discovery module. */
                                WiSafe_DiscoveryMissingNodeReport(&(buf->data[3]));
                            }
                            else
                            {
                                LOG_Error("Received missing node report with bad length.");
                            }
                            break;
                        }

                        /* We have received a remote status report in response to out 'request remote status' message. */
                        case sprueOptRemoteRmStatus:
                        {
                            if (buf->count == 13)
                            {
                                /* Pass on to fault module. */
                                WiSafe_FaultRemoteStatusReportReceived(buf);
                            }
                            else
                            {
                                LOG_Error("Received remote status report with bad length.");
                            }
                            break;
                        }

                        /* We have received a remote ID report message. */
                        case sprueOptRemoteIdReport:
                        {
                            /* We currently don't do anything with this message - it's not in the spec.
                               FYI. The structure of the message is:
                               Byte 0 = d4
                               Byte 1 = 09
                               Byte 2 = SID
                               Byte 3..5 = Device ID
                            */
                            LOG_Info("Remote ID report received - ignoring.");
                            break;
                        }

                        /* We have recevied a SID map update - this happens when an ACU has reorganised the network,
                           by holding down the search button for 5 seconds. */
                        case sprueOptidMapUpd:
                        {
                            if (buf->count == 10)
                            {
                                WiSafe_DiscoveryProcessSIDMapUpdate(&(buf->data[2]));
                            }
                            else
                            {
                                LOG_Error("Received SID map update with bad length.");
                            }
                            break;
                        }

                        /* We have received a Rumour Target response. */
                        case sprueOptRumourTarget:
                        {
                            if (buf->count == 5)
                            {
                                WiSafe_DiscoveryProcessRumourTargetResponse();
                            }
                            else
                            {
                                LOG_Error("Received Rumour Target Response with bad length.");
                            }
                            break;
                        }

                        default:
                        {
                            LOG_Error("Received unknown subcommand (0x%02x) for extdiag response.", buf->data[1]);
                        }
                    }
                }
                else
                {
                    LOG_Error("Received short extdiag response.");
                }
                break;
            }

            /* Device tested message received. */
            case (SPRUERM_CMD_OF(SPRUERM_DEVICE_TESTED)):
            {
                if (buf->count == 10)
                {
                    msgDeviceTested_t msg = WiSafe_DecodeDeviceTested(buf);
                    if (CheckDuplicateAndUpdateSeq(msg.id, msg.seq))
                    {
                        /* Pass on to discovery module. */
                        WiSafe_DiscoveryProcessDeviceTested(&msg,
                                                          ONLINE_STATUS_ONLINE, REAL_TEST_MESSAGE);/*ABR changed*/
                    }
                    else
                    {
                        LOG_Warning("Duplicate message ignored for device %06x.", msg.id);
                    }
                }
                else
                {
                    LOG_Error("Received 'device tested' message with bad length.");
                }
                break;
            }

            /* Fault received. */
            case (SPRUERM_CMD_OF(SPRUERM_FAULT)):
            {
                if (buf->count == 9)
                {
                    msgFault_t msg = WiSafe_DecodeFault(buf);
                    if (CheckDuplicateAndUpdateSeq(msg.id, msg.seq))
                    {
                        /* Pass on to fault/alarm module. */
                        WiSafe_FaultReceived(&msg);
                    }
                    else
                    {
                        LOG_Warning("Duplicate message ignored for device %06x.", msg.id);
                    }
                }
                else
                {
                    LOG_Error("Received 'fault' message with bad length.");
                }
                break;
            }

            /* Alarm received. */
            case (SPRUERM_CMD_OF(SPRUERM_ALARM)):
            {
                if (buf->count == 8)
                {
                    msgAlarm_t msg = WiSafe_DecodeAlarm(buf);
                    if (CheckDuplicateAndUpdateSeq(msg.id, msg.seq))
                    {
                        /* Pass on to fault/alarm module. */
                        WiSafe_AlarmReceived(&msg);
                    }
                    else
                    {
                        LOG_Warning("Duplicate message ignored for device %06x.", msg.id);
                    }
                }
                else
                {
                    LOG_Error("Received 'alarm' message with bad length.");
                }
                break;
            }

            /* Extended Diagnostic Variable Response received. */
            case (SPRUERM_CMD_OF(SPRUERM_EXTDIG_DATASLOT_RESULT)):
            {
                if (buf->count == 11)
                {
                    msgExtDiagVarResp_t msg = WiSafe_DecodeExtDiagVarResp(buf);
                    if (CheckDuplicateAndUpdateSeq(msg.id, msg.seq))
                    {
                        /* Pass on to fault/alarm module. */
                        WiSafe_ExtDiagVarRespReceived(&msg);
                    }
                    else
                    {
                        LOG_Warning("Duplicate message ignored for device %06x.", msg.id);
                    }
                }
                else
                {
                    LOG_Error("Received 'Extended Diagnostic Variable Response' message with bad length.");
                }
                break;
            }

            /* Fault detail received. */
            case (SPRUERM_CMD_OF(SPRUERM_FAULT_DETAILS)):
            {
                if (buf->count == 10)
                {
                    msgFaultDetail_t msg = WiSafe_DecodeFaultDetail(buf);
                    if (CheckDuplicateAndUpdateSeq(msg.id, msg.seq))
                    {
                        /* Pass on to fault/alarm module. */
                        WiSafe_FaultDetailReceived(&msg);
                    }
                    else
                    {
                        LOG_Warning("Duplicate message ignored for device %06x.", msg.id);
                    }
                }
                else
                {
                    LOG_Error("Received 'fault details' message with bad length.");
                }
                break;
            }

            /* Alarm stop received. */
            case (SPRUERM_CMD_OF(SPRUERM_ALARM_STOP)):
            {
                if (buf->count == 7)
                {
                    msgAlarmStop_t msg = WiSafe_DecodeAlarmStop(buf);
                    if (CheckDuplicateAndUpdateSeq(msg.id, msg.seq))
                    {
                        /* Pass on to fault/alarm module. */
                        WiSafe_FaultAlarmStopReceived(&msg);
                    }
                    else
                    {
                        LOG_Warning("Duplicate message ignored for device %06x.", msg.id);
                    }
                }
                else
                {
                    LOG_Error("Received 'alarm stop' message with bad length.");
                }
                break;
            }

            /* Hush message. */
            case (SPRUERM_CMD_OF(SPRUERM_HUSH)):
            {
                if (buf->count == 7)
                {
                    /* Pass on to fault/alarm module. */
                    msgHush_t msg = WiSafe_DecodeHush(buf);
                    if (CheckDuplicateAndUpdateSeq(msg.id, msg.seq))
                    {
                        /* Pass on to fault/alarm module. */
                        WiSafe_FaultHushReceived(&msg);
                    }
                    else
                    {
                        LOG_Warning("Duplicate message ignored for device %06x.", msg.id);
                    }
                }
                else
                {
                    LOG_Error("Received 'hush' message with bad length.");
                }
                break;
            }

            /* Locate message. */
            case (SPRUERM_CMD_OF(SPRUERM_LOCATE)):
            {
                if (buf->count == 8)
                {
                    /* Pass on to fault/alarm module. */
                    msgLocate_t msg = WiSafe_DecodeLocate(buf);
                    if (CheckDuplicateAndUpdateSeq(msg.id, msg.seq))
                    {
                        /* Pass on to fault/alarm module. */
                        WiSafe_FaultLocateReceived(&msg);
                    }
                    else
                    {
                        LOG_Warning("Duplicate message ignored for device %06x.", msg.id);
                    }
                }
                else
                {
                    LOG_Error("Received 'locate' message with bad length.");
                }
                break;
            }

            /* NACK received. */
            case (SPRUERM_CMD_OF(SPRUERM_NACK)):
            {
                LOG_Warning("Received unexpected NACK.");
                break;
            }

            /* RM Diagnostic Result message. */
            case (SPRUERM_CMD_OF(SPRUERM_RM_DIAGNOSTIC_RESULT)):
            {
                if (buf->count == 13)
                {
                    HandleRMDiagnosticResult(buf);
                }
                else
                {
                    LOG_Error("Received 'RM Diagnostic Result' message with bad length.");
                }
                break;
            }

            /* RM SD Fault message. */
            case (SPRUERM_CMD_OF(SPRUERM_RM_SD_FAULT)):
            {
                if (buf->count == 7)
                {
                    /* Pass on to fault/alarm module. */
                    WiSafe_FaultRMFaultReceived(buf);
                }
                else
                {
                    LOG_Error("Received 'RM SD Fault' message with bad length.");
                }
                break;
            }

            /* Request Remote ID Results. */
            case (SPRUERM_CMD_OF(SPRUERM_DO_EXTDIG_ID_RESULT)):
            {
                if (buf->count == 9)
                {
                    /* Pass on to discovery module. */
                    msgRequestRemoteIdResult_t msg = WiSafe_DecodeRemoteIdResult(buf);
                    WiSafe_DiscoveryRemoteStatusReceived(&msg);
                }
                else
                {
                    LOG_Error("Received 'Request Remote Id Result' message with bad length.");
                }
                break;
            }

            default:
            {
                LOG_Error("Received unknown message of type 0x%02x", buf->data[0]);
                break;
            }
        }
    }
    else
    {
        LOG_Error("Received empty message?");
    }
}

/**
 * Helper to send a 'device test' message over SPI.
 *
 * @param id The ID of the device to be tested.
 */
static void IssueTestToDevice(deviceId_t id)
{
    LOG_Info("Device test requested for device 0x%06x.", id);

    /* We need to determine the priority of this device. */
    EnsoErrorCode_e error;
    EnsoPropertyValue_u type = WiSafe_DALGetReportedProperty(id, PROP_TYPE_ID, &error);
    if (error != eecNoError)
    {
        LOG_Error("Cannot issue device test because priority not known for device 0x%06x.", id);
        return;
    }

    /* We need to determine the model of this device. */
    EnsoPropertyValue_u model = WiSafe_DALGetReportedProperty(id, DAL_PROPERTY_DEVICE_MODEL_IDX, &error);
    if (error != eecNoError)
    {
        LOG_Error("Cannot issue device test because model not known for device 0x%06x.", id);
        return;
    }

    radioCommsBuffer_t* buffer = WiSafe_EncodeDeviceTest(id, model.uint32Value, (type.uint32Value & ~DAL_PROPERTY_DEVICE_TYPE_WISAFE_MASK));
    if (buffer != NULL)
    {
        WiSafe_RadioCommsTx(buffer);
    }
    else
    {
        LOG_Error("Cannot issue device test because no buffers free for device 0x%06x.", id);
    }
}

/**
 * Helper to send a 'device test' message over SPI to the first device
 * we know about.
 *
 */
static void IssueTest()
{
    /* Find a device. */
    deviceId_t buffer[DAL_MAXIMUM_NUMBER_WISAFE_DEVICES];
    uint16_t numDevices = 0;
    EnsoErrorCode_e enumerateError = WiSafe_DALDevicesEnumerate(buffer, DAL_MAXIMUM_NUMBER_WISAFE_DEVICES, &numDevices);
    if (enumerateError == eecNoError)
    {
        if (numDevices > 0)
        {
            IssueTestToDevice(buffer[0]);
            return;
        }
    }

    LOG_Error("Unable to find a device to receive test command.");
}


/**
 * Helper to identify a device by sounding its siren, strobe etc.
 * Send a Rumour Target message and on receiving a reply, send a Device Tested.
 * Note that the Rumour Target is not supported in old RM firmware - a NACK
 * will be received from the RM if it is not supported. Likewise, the remote
 * device must also have a RM firmware version that supports the Rumour Target
 * message for the identify siren feature to work.
 *
 */
static void IdentifyDevice(deviceId_t id)
{
    /* Find a device. */
    EnsoDeviceId_t deviceId = EnsoDeviceFromWiSafeID(id);
    EnsoPropertyValue_u sid;
    EnsoErrorCode_e error = LSD_GetPropertyValueByAgentSideId(&deviceId, REPORTED_GROUP, DAL_PROPERTY_DEVICE_SID_IDX, &sid);
    if (error == eecNoError)
    {
        /* We've found it. */
        LOG_Info("Identifying device 0x%06x. Sid = %u", id, sid.uint32Value);

        radioCommsBuffer_t* buffer = WiSafe_EncodeRumourTarget((sid_t)sid.uint32Value);
        if (buffer != NULL)
        {
            WiSafe_RadioCommsTx(buffer);
        }
        else
        {
            LOG_Error("No buffers free for device 0x%06x.", id);
        }
    }
    else
    {
        LOG_Error("No SID for device 0x%06x. Reason: %i ", id, error);
    }
}

/**
 * Helper to locate devices in alarm state by silencing all other alarms that
 * are not in the Cannot Be Silenced state. A WiSafe Locate message is sent
 * containing the device id of the gateway.
 */
static void Locate(void)
{
    radioCommsBuffer_t* buffer = WiSafe_EncodeLocate();
    if (buffer != NULL)
    {
        WiSafe_RadioCommsTx(buffer);
    }
    else
    {
        LOG_Error("Cannot issue locate because no buffers free");
    }
}

/**
 * Helper to enable/disable the sounder on a given device.
 *
 * @param id The device.
 * @param enabled Whether to enable or disable.
 */
static void EnableSounder(deviceId_t id, bool enabled)
{
    LOG_Info("Sounder %s for for device 0x%06x.", enabled?"enabled":"disable", id);

    radioCommsBuffer_t* buffer = WiSafe_EncodeSounderEnable(id, enabled);
    if (buffer != NULL)
    {
        WiSafe_RadioCommsTx(buffer);
    }
    else
    {
        LOG_Error("Cannot control sounder enablement because no buffers free for device 0x%06x.", id);
    }
}

/**
 * Helper to silence all devices by sending a Hush message.
 *
 * Note that alarms in "cannot be silenced" state will not be silenced by the
 * Hush command.
 */
static void SilenceAll(void)
{
    LOG_Info("Silence All");

    radioCommsBuffer_t* buffer = WiSafe_EncodeHush();
    if (buffer != NULL)
    {
        WiSafe_RadioCommsTx(buffer);
    }
    else
    {
        LOG_Error("Cannot silence all because no buffers free");
    }
}

/**
 * Process a control message.
 *
 * @param message The message.
 */
void WiSafe_EngineProcessControlMessage(controlEntry_t* message)
{
    if (message != NULL)
    {
        switch (message->operation)
        {
            case controlOp_beginMissingNodeScan:
            {
                bool deleteMissingDevices = (bool)message->param1;
                WiSafe_DiscoveryTestMode(true, deleteMissingDevices);
                break;
            }

            case controlOp_stopMissingNodeScan:
                WiSafe_DiscoveryTestMode(false, false);
                break;

            case controlOp_issueTest:
                IssueTest(/*(deviceId_t)message->param1*/);
                break;

            case controlOp_learn:
            {
                bool enabled = (bool)message->param1;
                uint32_t timeout = message->param2;
                WiSafe_DiscoveryLearnMode(enabled, timeout);
                break;
            }

            case controlOp_leave:
                WiSafe_DiscoveryLeave();
                break;

            case controlOp_flushAll:
                WiSafe_DiscoveryFlushAll();
                break;

            case controlOp_flush:
                WiSafe_DiscoveryFlush((deviceId_t)message->param1);
                break;

            case controlOp_identifyDevice:
                IdentifyDevice((deviceId_t)message->param1);
                break;

            case controlOp_silenceAll:
                SilenceAll();
                break;

            case controlOp_locate:
                Locate();
                break;

            default:
                LOG_Error("Unhandled control message of type %d.", message->operation);
                break;
        }
    }
}

void WiSafe_EngineProcessGroupDelta(EnsoDeviceId_t* deviceId, EnsoPropertyDelta_t deltas[], int numProperties)
{
    /* We need to spot the special case of learn-in. */
    bool isLearnIn = false;
    for (int idx = 0; idx < numProperties; idx += 1)
    {
        EnsoPropertyDelta_t* delta = &(deltas[idx]);
        if (delta->agentSidePropertyID == DAL_COMMAND_LEARN_IDX)
        {
            isLearnIn = true;
            break;
        }
    }

    /* Iterate over all the properties that have changed, and process them. */
    for (int idx = 0; idx < numProperties; idx += 1)
    {
        EnsoPropertyDelta_t* delta = &(deltas[idx]);

        /* Get the property name for debugging. */
        char propName[LSD_PROPERTY_NAME_BUFFER_SIZE] = {0,};
        LSD_GetPropertyCloudNameFromAgentSideId(deviceId, delta->agentSidePropertyID, sizeof(propName), propName);

        /* Call the engine to process it. In the special case of learn-in, we
           don't want to process changes to the 'state' property because the handling
           code reads that directly from the shadow. */
        if (isLearnIn && (delta->agentSidePropertyID == PROP_STATE_ID))
        {
            LOG_InfoC(LOG_BLUE "Processing group property (%d/%d) delta on device %016llx for key '%s' (%x) - skipped.", (idx + 1), numProperties, deviceId->deviceAddress, propName, delta->agentSidePropertyID);
        }
        else
        {
            LOG_InfoC(LOG_BLUE "Processing group property (%d/%d) delta on device %016llx for key '%s' (%x).", (idx + 1), numProperties, deviceId->deviceAddress, propName, delta->agentSidePropertyID);
            WiSafe_EngineProcessPropertyDelta(deviceId, delta);
        }
    }
}

/**
 * Process a delta to a property.
 *
 * @param deviceId The device that has the delta.
 * @param delta    The delta.
 */
void WiSafe_EngineProcessPropertyDelta(EnsoDeviceId_t* deviceId, EnsoPropertyDelta_t* delta)
{
    if (delta != NULL || deviceId != NULL)
    {
        /* See if the delta is for a property on the gateway. */
        if (SYS_IsGateway(deviceId))
        {
            /* Each property corresponds to a different command. */

            /* Is it a missing node scan command? */
            if (delta->agentSidePropertyID == DAL_COMMAND_TEST_MODE_IDX)
            {
                /* Get the flush parameter that we were passed. */
                EnsoErrorCode_e error;
                EnsoPropertyValue_u flush = WiSafe_DALGetDesiredProperty(GATEWAY_DEVICE_ID, DAL_COMMAND_TEST_MODE_FLUSH_IDX, &error);
                if (error == eecNoError)
                {
                    LOG_Info("Received test mode command.");

                    /* Set the gateway state to test mode. */
                    EnsoPropertyValue_u valueS = { .uint32Value = GatewayStateTestMode };
                    WiSafe_DALSetReportedProperty(GATEWAY_DEVICE_ID, PROP_STATE_ID, (propertyName_t)PROP_STATE_CLOUD_NAME, evUnsignedInt32, valueS);

                    /* Begin the test. */
                    WiSafe_ControlBeginMissingNodeScan(flush.booleanValue);

                    /* Update timestamp to indicate we have processed the command. Also update reported flush to match desired. */
                    WiSafe_DALSetReportedProperty(GATEWAY_DEVICE_ID, DAL_COMMAND_TEST_MODE_IDX, DAL_COMMAND_TEST_MODE, evUnsignedInt32, delta->propertyValue);
                    WiSafe_DALSetReportedProperty(GATEWAY_DEVICE_ID, DAL_COMMAND_TEST_MODE_FLUSH_IDX, DAL_COMMAND_TEST_MODE_FLUSH, evBoolean, flush);
                }
                else
                {
                    LOG_Error("Unable to read flush for Test Mode command.");
                }
            }

            /* Is the command related to learn-in on a WiSafe network? */
            else if (delta->agentSidePropertyID == DAL_COMMAND_LEARN_IDX)
            {
                LOG_Info("Received learn-in command.");

                /* See what desired 'state' has been requested to see if we're starting or stopping. */
                bool isStarting = false;

                EnsoErrorCode_e error;
                EnsoPropertyValue_u desiredState = WiSafe_DALGetDesiredProperty(GATEWAY_DEVICE_ID, PROP_STATE_ID, &error);
                if (error == eecNoError)
                {
                    if (desiredState.uint32Value == GatewayStateLearning)
                    {
                        isStarting = true;
                    }
                    else if (desiredState.uint32Value == GatewayStateRunning)
                    {
                        isStarting = false;
                    }
                    else
                    {
                        LOG_Error("Learn-in command received in inconsistent state.");
                        isStarting = false; // Default to stopping.
                    }
                }
                else
                {
                    LOG_Error("Learn-in command was unable to read desired state.");
                    isStarting = false; // Default to stopping.
                }

                /* Get any timeout that we were passed. */
                EnsoPropertyValue_u timeout = WiSafe_DALGetDesiredProperty(GATEWAY_DEVICE_ID, DAL_COMMAND_LEARN_TIMEOUT_IDX, &error);
                if (error != eecNoError)
                {
                    LOG_Error("Unable to read timeout for Learn command.");
                    timeout.uint32Value = 0;
                }

                /* Are we starting, or stopping? */
                if (isStarting && (timeout.uint32Value != 0))
                {
                    /* Enter learn mode. */
                    WiSafe_ControlLearnMode(true, timeout.uint32Value);

                    /* Update timestamp to indicate we have processed the command. Also update reported timeout to match desired. */
                    WiSafe_DALSetReportedProperty(GATEWAY_DEVICE_ID, DAL_COMMAND_LEARN_IDX, DAL_COMMAND_LEARN, evUnsignedInt32, delta->propertyValue);
                    WiSafe_DALSetReportedProperty(GATEWAY_DEVICE_ID, DAL_COMMAND_LEARN_TIMEOUT_IDX, DAL_COMMAND_LEARN_TIMEOUT, evUnsignedInt32, timeout);

                    Wisafe_FlashLearnLED();
                }
                else
                {
                    /* Leave learn mode. */
                    WiSafe_DALSetReportedProperty(GATEWAY_DEVICE_ID, DAL_COMMAND_LEARN_IDX, DAL_COMMAND_LEARN, evUnsignedInt32, delta->propertyValue);
                    WiSafe_ControlLearnMode(false, 0);
                }
            }

            /* Is it the command to take the gateway out of learn-in mode, or to end test mode, on a WiSafe network? */
            else if (delta->agentSidePropertyID == PROP_STATE_ID)
            {
                /* Get the current state of the gateway. */
                EnsoErrorCode_e error;
                EnsoPropertyValue_u currentState = WiSafe_DALGetReportedProperty(GATEWAY_DEVICE_ID, PROP_STATE_ID, &error);
                if (error == eecNoError)
                {
                    /* If the current state is learn, and the desired state is running, we've been asked to stop. */
                    if ((currentState.uint32Value == GatewayStateLearning) && (delta->propertyValue.uint32Value == GatewayStateRunning))
                    {
                        /* Leave learn mode. */
                        LOG_Info("Received state change request to stop learn-in.");
                        WiSafe_ControlLearnMode(false, 0);
                    }

                    /* If the current state is test mode, and the desired state is running, we've been asked to stop. */
                    else if ((currentState.uint32Value == GatewayStateTestMode) && (delta->propertyValue.uint32Value == GatewayStateRunning))
                    {
                        LOG_Info("Received state change request to stop test mode.");
                        WiSafe_ControlStopMissingNodeScan();
                    }
                }
                else
                {
                    LOG_Error("Unable to read gateway state for Learn command.");
                }
            }

            /* Is it the command to ask the gateway to leave the network? */
            else if (delta->agentSidePropertyID == DAL_COMMAND_FLUSH_IDX)
            {
                WiSafe_ControlLeave();
            }

            /* Is it the command to delete all devices? */
            else if (delta->agentSidePropertyID == DAL_COMMAND_DELETE_ALL_IDX)
            {
                WiSafe_ControlFlushAll();
            }

            /* Is it the sounder test command? */
            else if (delta->agentSidePropertyID == DAL_COMMAND_SOUNDER_TEST_IDX)
            {
                 WiSafe_ControlIssueTest((deviceId_t)(deviceId->deviceAddress));

                /* Update timestamp to indicate we have processed the command. */
                WiSafe_DALSetReportedProperty(GATEWAY_DEVICE_ID, DAL_COMMAND_SOUNDER_TEST_IDX, DAL_COMMAND_SOUNDER_TEST, evUnsignedInt32, delta->propertyValue);
            }

            /* Is it the command to silence all devices? */
            else if (delta->agentSidePropertyID == DAL_COMMAND_SILENCE_ALL_IDX)
            {
                WiSafe_ControlSilenceAll();
                /* Update timestamp to indicate we have processed the command. */
                WiSafe_DALSetReportedProperty(GATEWAY_DEVICE_ID, DAL_COMMAND_SILENCE_ALL_IDX, DAL_COMMAND_SILENCE_ALL, evUnsignedInt32, delta->propertyValue);
            }

            /* Is it the command to locate devices in alarm state? */
            else if (delta->agentSidePropertyID == DAL_COMMAND_LOCATE_IDX)
            {
                WiSafe_ControlLocate();
                WiSafe_DALSetReportedProperty(GATEWAY_DEVICE_ID, DAL_COMMAND_LOCATE_IDX, DAL_COMMAND_LOCATE, evUnsignedInt32, delta->propertyValue);

            }
            else
            {
                LOG_Error("Unhandled gateway property delta for key: %08x.", delta->agentSidePropertyID);
            }
        }
        else
        {
            /* It's a property on a device, see if it's one we know about. */

            /* Is it the platform enabling/disabling the sounder on a device. */
            if (delta->agentSidePropertyID == DAL_PROPERTY_DEVICE_MUTE_IDX)
            {
                EnableSounder((deviceId_t)(deviceId->deviceAddress),
                              delta->propertyValue.booleanValue ? false : true);
                WiSafe_DALSetReportedProperty((deviceId_t)(deviceId->deviceAddress), DAL_PROPERTY_DEVICE_MUTE_IDX, DAL_PROPERTY_DEVICE_MUTE, evBoolean, delta->propertyValue);
            }

            /* Is it the command to identify a specific device? */
            else if (delta->agentSidePropertyID == DAL_COMMAND_IDENTIFY_DEVICE_IDX)
            {
                WiSafe_ControlIdentifyDevice((deviceId_t)(deviceId->deviceAddress));

                /* Update timestamp to indicate we have processed the command. */
                WiSafe_DALSetReportedProperty((deviceId_t)(deviceId->deviceAddress), DAL_COMMAND_IDENTIFY_DEVICE_IDX, DAL_COMMAND_IDENTIFY_DEVICE, evUnsignedInt32, delta->propertyValue);
            }

            /* Is it the command to flush a specific device? */
            else if (delta->agentSidePropertyID == DAL_COMMAND_DEVICE_FLUSH_IDX)
            {
                bool flush = delta->propertyValue.booleanValue;
                if (flush)
                {
                    WiSafe_ControlFlush((deviceId_t)(deviceId->deviceAddress));
                }
            }
            else
            {
                LOG_Error("Unhandled property delta on device 0x%06x for key: %08x.", deviceId->deviceAddress, delta->agentSidePropertyID);
            }
        }
    }
}
