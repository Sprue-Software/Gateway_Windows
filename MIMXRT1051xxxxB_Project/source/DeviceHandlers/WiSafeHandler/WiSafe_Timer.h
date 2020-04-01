/*!****************************************************************************
*
* \file WiSafe_Timer.h
*
* \author James Green
*
* \brief Module to set timers and trigger when they time-out.
*
* \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

#ifndef _WISAFETIMER_H_
#define _WISAFETIMER_H_

#include <stdint.h>

#include "WiSafe_RadioCommsBuffer.h"

typedef enum
{
    timerType_DiscoveryRetrieveSidMap            = 1,
    timerType_DiscoveryTestMissingInit           = 2,
    timerType_DiscoveryTestMissingTimeout        = 3,
    timerType_DiscoveryTestMissingContinue       = 4,
    timerType_DiscoveryTestMissingUpdateMapAtEnd = 5,
    timerType_DiscoveryLearnModeTimeout          = 6,
    timerType_FaultTimeout                       = 7,
    timerType_AlarmTimeout                       = 8,
    timerType_DiscoveryInterrogate               = 9
}
timerType_e;

typedef struct
{
    uint8_t messageType;
    timerType_e timerType;
} timerMsg_GeneralPurpose1_st;

typedef struct
{
    uint8_t messageType;
    radioCommsBuffer_t* buffer;
} timerMsg_GeneralPurpose2_3_st;

#endif /* _WISAFETIMER_H_ */
