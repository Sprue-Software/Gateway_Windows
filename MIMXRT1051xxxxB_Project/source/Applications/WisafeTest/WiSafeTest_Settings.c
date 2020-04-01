/*!****************************************************************************
*
* \file WiSafe_Settings.c
*
* \author James Green
*
* \brief Timing related constants for WiSafe
*
* \Copyright (C) 2017 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/

#include "WiSafe_Settings.h"

//////// [RE:platform]
////////#ifdef Ameba
#define WISAFETEST_NETWORK_REFRESH_PERIOD       (90)
////////#else
////////#define WISAFETEST_NETWORK_REFRESH_PERIOD       (10)
////////#endif

/* Constants for Learn mode and SID map polling. */
uint32_t normalNetworkRefreshPeriod = WISAFETEST_NETWORK_REFRESH_PERIOD; // How often in seconds to poll for new SID map entries when in normal operation (not Learn mode).

/* Constants for Testing for missing nodes. */
uint32_t testMissingMinimumTimeBetweenScans = 3; // Minimum time in seconds betweens issues of scan messages.
