/*!****************************************************************************
*
* \file WiSafeTest.h
*
* \author James Green
*
* \brief Format/pack messages for transmission over SPI.
*
* \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

#ifndef _WISAFETEST_H_
#define _WISAFETEST_H_

typedef struct
{
    uint32_t count;
    uint8_t* data;
} expectedPacket_t;

extern void WiSafe_RegisterExpectedTX(expectedPacket_t** expectedPackets);
extern void WiSafe_WaitForExpectedTX(uint32_t timeoutInSeconds);
extern void WiSafe_RegisterAndWaitForExpectedTX(uint32_t timeoutInSeconds, expectedPacket_t** expectedPackets);
extern bool CheckReportedProperty(deviceId_t id, EnsoAgentSidePropertyId_t agentSideId, EnsoValueType_e type, EnsoPropertyValue_u requiredValue);
extern EnsoErrorCode_e WiSafe_DALSetDesiredProperties(deviceId_t id, EnsoAgentSidePropertyId_t agentSideId[], propertyName_t name[], EnsoValueType_e type[], EnsoPropertyValue_u newValue[], int numProperties, PropertyKind_e kind, bool buffered, bool persistent);

#endif
