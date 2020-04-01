/*
 * Copyright 2010-2015 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */
/*
* \Copyright (C) 2016 Intamac Ltd - All Rights Reserved.
* Unauthorized copying of this file, via any medium is strictly prohibited
*/
/**
 * @file aws_iot_config.h
 * @brief AWS IoT specific configuration file
 */

#ifndef SRC_SHADOW_IOT_SHADOW_CONFIG_H_
#define SRC_SHADOW_IOT_SHADOW_CONFIG_H_


// Get from console45false
// =================================================
// Change the Host Name : As Per Rob (14-sep-2019) for production 
#define AWS_IOT_MQTT_HOST  "a1i5mar9xlen2b-ats.iot.eu-central-1.amazonaws.com"	/*Production*/
//#define AWS_IOT_MQTT_HOST    "a2ly63dtviyxr8-ats.iot.eu-west-1.amazonaws.com"   /*Staging*/
//#define AWS_IOT_MQTT_HOST              "a2ly63dtviyxr8.iot.eu-west-1.amazonaws.com"		//////// Sprue staging server

// #define AWS_IOT_MQTT_HOST              "a1ss23pdjc9izo.iot.eu-west-1.amazonaws.com"		//////// [RE:conf] nchatzi
// #define AWS_IOT_MQTT_HOST              "a2bveqnyu0ebyv.iot.eu-west-1.amazonaws.com"		//////// [RE:conf] Sprue original
// #define AWS_IOT_MQTT_HOST              "stg-certmgr.wi-safeconnect.com"			//////// [RE:conf] Sprue new
#define AWS_IOT_MQTT_PORT              8883
#define AWS_IOT_MQTT_CLIENT_ID         "" // Do not use
#define AWS_IOT_MY_THING_NAME          "" // Do not use

// =================================================

// MQTT PubSub
#define AWS_IOT_MQTT_TX_BUF_LEN 4096 ///< Any time a message is sent out through the MQTT layer. The message is copied into this buffer anytime a publish is done. This will also be used in the case of Thing Shadow
#define AWS_IOT_MQTT_RX_BUF_LEN 4096 ///< Any message that comes into the device should be less than this buffer size. If a received message is bigger than this buffer size the message will be dropped.
#define AWS_IOT_MQTT_NUM_SUBSCRIBE_HANDLERS 50 ///< Maximum number of topic filters the MQTT client can handle at any given time. This should be increased appropriately when using Thing Shadow

// Thing Shadow specific configs
#define SHADOW_MAX_SIZE_OF_RX_BUFFER AWS_IOT_MQTT_RX_BUF_LEN+1 ///< Maximum size of the SHADOW buffer to store the received Shadow message
#define MAX_SIZE_OF_UNIQUE_CLIENT_ID_BYTES 80  ///< Maximum size of the Unique Client Id. For More info on the Client Id refer \ref response "Acknowledgments"
#define MAX_SIZE_CLIENT_ID_WITH_SEQUENCE MAX_SIZE_OF_UNIQUE_CLIENT_ID_BYTES + 10 ///< This is size of the extra sequence number that will be appended to the Unique client Id
#define MAX_SIZE_CLIENT_TOKEN_CLIENT_SEQUENCE MAX_SIZE_CLIENT_ID_WITH_SEQUENCE + 20 ///< This is size of the the total clientToken key and value pair in the JSON
#define MAX_ACKS_TO_COMEIN_AT_ANY_GIVEN_TIME 10 ///< At Any given time we will wait for this many responses. This will correlate to the rate at which the shadow actions are requested
#define MAX_THINGNAME_HANDLED_AT_ANY_GIVEN_TIME 10 ///< We could perform shadow action on any thing Name and this is maximum Thing Names we can act on at any given time
#define MAX_JSON_TOKEN_EXPECTED 500 ///< These are the max tokens that is expected to be in the Shadow JSON document. Include the metadata that gets published
#define MAX_SHADOW_TOPIC_LENGTH_WITHOUT_THINGNAME 60 ///< All shadow actions have to be published or subscribed to a topic which is of the format $aws/things/{thingName}/shadow/update/accepted. This refers to the size of the topic without the Thing Name
#define MAX_SIZE_OF_THING_NAME 50 ///< The Thing Name should not be bigger than this value. Modify this if the Thing Name needs to be bigger
#define MAX_SHADOW_TOPIC_LENGTH_BYTES MAX_SHADOW_TOPIC_LENGTH_WITHOUT_THINGNAME + MAX_SIZE_OF_THING_NAME ///< This size includes the length of topic with Thing Name

// Auto Reconnect specific config
#define AWS_IOT_MQTT_MIN_RECONNECT_WAIT_INTERVAL 1000 ///< Minimum time before the First reconnect attempt is made as part of the exponential back-off algorithm
#define AWS_IOT_MQTT_MAX_RECONNECT_WAIT_INTERVAL 8000 ///< Maximum time interval after which exponential back-off will stop attempting to reconnect.

/*
 * Wrapper define for AWS SDK.
 *
 * IMP : In the Ameba SDK, WLAN Library also defines function so to avoid
 * conflict we need to define init_timer and to replace it with OSAL_init_timer
 * here.
 */
#define init_timer      OSAL_init_timer

#endif /* SRC_SHADOW_IOT_SHADOW_CONFIG_H_ */
