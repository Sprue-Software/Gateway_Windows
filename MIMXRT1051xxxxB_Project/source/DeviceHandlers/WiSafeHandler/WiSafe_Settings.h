/*!****************************************************************************
*
* \file WiSafe_Settings.h
*
* \author James Green
*
* \brief Timing related constants for WiSafe
*
* \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

#include <stdint.h>

#ifndef _WISAFE_SETTINGS_H_
#define _WISAFE_SETTINGS_H_

/* Constants for Learn mode and SID map polling. */
extern uint32_t normalNetworkRefreshPeriod; // How often in seconds to poll for new SID map entries when in normal operation (not Learn mode).

/* Constants for Testing for missing nodes. */
extern uint32_t testMissingMinimumTimeBetweenScans; // Minimum time in seconds betweens issues of scan messages.


#endif
