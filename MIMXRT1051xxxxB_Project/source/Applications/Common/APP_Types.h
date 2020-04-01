/*!****************************************************************************
*
* \file APP_Types.h
*
* \author Evelyne Donnaes
*
* \brief Application Types
*
* \Copyright (C) 2016 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*
*****************************************************************************/
#ifndef _APP_TYPES_H_
#define _APP_TYPES_H_

#include "LSD_Types.h"


/*!****************************************************************************
 * Generic Property Ids
 *****************************************************************************/

// Property groups
#define PROP_GROUP_GATEWAY                      (0x0000 << 16)
#define PROP_GROUP_PRIVATE                      (0x1000 << 16)
#define PROP_GROUP_WISAFE                       (0x2000 << 16)
#define PROP_GROUP_TEST                         (0x3000 << 16)  // For Test Purposes

// Public properties id
#define PROP_TYPE_ID                            (PROP_GROUP_GATEWAY | 0x0001)
#define PROP_MANUFACTURER_ID                    (PROP_GROUP_GATEWAY | 0x0002)
#define PROP_MODEL_ID                           (PROP_GROUP_GATEWAY | 0x0003)
#define PROP_ONLINE_ID                          (PROP_GROUP_GATEWAY | 0x0004)
#define PROP_WISAFE_ID                          (PROP_GROUP_GATEWAY | 0x0005)
#define PROP_FWNAM_ID                           (PROP_GROUP_GATEWAY | 0x0006)
#define PROP_FWVER_ID                           (PROP_GROUP_GATEWAY | 0x0007)
#define PROP_ONLINE_SEQ_NO_ID                   (PROP_GROUP_GATEWAY | 0x0008)
#define PROP_STATE_ID                           (PROP_GROUP_GATEWAY | 0x0009)
#define PROP_FWTIM_ID                           (PROP_GROUP_GATEWAY | 0x000a)

#define PROP_UPGRD_NAME_ID                      (PROP_GROUP_GATEWAY | 0x0010)
#define PROP_UPGRD_URL_ID                       (PROP_GROUP_GATEWAY | 0x0011)
#define PROP_UPGRD_VER_ID                       (PROP_GROUP_GATEWAY | 0x0012)
#define PROP_UPGRD_KEY_ID                       (PROP_GROUP_GATEWAY | 0x0013)
#define PROP_UPGRD_HASH_ID                      (PROP_GROUP_GATEWAY | 0x0014)
#define PROP_UPGRD_PROG_ID                      (PROP_GROUP_GATEWAY | 0x0015)
#define PROP_UPGRD_SIZE_ID                      (PROP_GROUP_GATEWAY | 0x0016)
#define PROP_UPGRD_STATE_ID                     (PROP_GROUP_GATEWAY | 0x0017)
#define PROP_UPGRD_RETRY_ID                     (PROP_GROUP_GATEWAY | 0x0018)
#define PROP_UPGRD_ERROR_ID                     (PROP_GROUP_GATEWAY | 0x0019)
#define PROP_UPGRD_STATUS_ID                    (PROP_GROUP_GATEWAY | 0x001a)
#define PROP_GW_RESET_ID                        (PROP_GROUP_GATEWAY | 0x001b)
#define PROP_GW_REGISTERED_ID                   (PROP_GROUP_GATEWAY | 0x001c)
#define PROP_UPGRD_TRGRD_ID			(PROP_GROUP_GATEWAY | 0x001d)
#define PROP_CERT_MANAGER_URL_ID                (PROP_GROUP_GATEWAY | 0x001e)

// Private properties id
#define PROP_DEVICE_STATUS_ID                   (PROP_GROUP_PRIVATE | 0x0001)
#define PROP_OWNER_ID                           (PROP_GROUP_PRIVATE | 0x0002)
#define PROP_CONNECTION_ID                      (PROP_GROUP_PRIVATE | 0x0003)
#define PROP_LEARN_LED_FLASH_ID                 (PROP_GROUP_PRIVATE | 0x0004)

// Public properties name
extern const char PROP_TYPE_CLOUD_NAME[LSD_PROPERTY_NAME_BUFFER_SIZE];
extern const char PROP_MANUFACTURER_CLOUD_NAME[LSD_PROPERTY_NAME_BUFFER_SIZE];
extern const char PROP_MODEL_CLOUD_NAME[LSD_PROPERTY_NAME_BUFFER_SIZE];
extern const char PROP_ONLINE_CLOUD_NAME[LSD_PROPERTY_NAME_BUFFER_SIZE];
extern const char PROP_ONLINE_SEQ_NO_CLOUD_NAME[LSD_PROPERTY_NAME_BUFFER_SIZE];
extern const char PROP_WISAFE_ID_CLOUD_NAME[LSD_PROPERTY_NAME_BUFFER_SIZE];
extern const char PROP_STATE_CLOUD_NAME[LSD_PROPERTY_NAME_BUFFER_SIZE];

// Private properties name
extern const char PROP_DEVICE_STATUS_CLOUD_NAME[LSD_PROPERTY_NAME_BUFFER_SIZE];
extern const char PROP_OWNER_INTERNAL_NAME[LSD_PROPERTY_NAME_BUFFER_SIZE];
extern const char PROP_CONNECTION_ID_CLOUD_NAME[LSD_PROPERTY_NAME_BUFFER_SIZE];

// Public enumerations
typedef enum {
    GatewayStateFactoryReset = 0,
    GatewayStateSWReset      = 1,
    GatewayStateLearning     = 2,
    GatewayStateRunning      = 3,
    GatewayStateTestMode     = 4
} GatewayState_e;

#endif /* _APP_TYPES_H_ */
