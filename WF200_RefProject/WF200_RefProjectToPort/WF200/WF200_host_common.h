/*
 * commom.h
 *
 *  Created on: 14 Jan 2020
 *      Author: SMurva
 */

#ifndef WF200_HOST_COMMON_H_
#define WF200_HOST_COMMON_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include "sl_wfx.h"
#include "sl_wfx_constants.h"

#include "lwip/timeouts.h"
#include "lwip/ip_addr.h"
#include "lwip/init.h"
#include "lwip/dhcp.h"
#include "lwip/prot/dhcp.h"
#include "netif/ethernet.h"
#include "enet_ethernetif.h"
#include "event_groups.h"

#include "lwip/etharp.h"
#include "lwip/tcp.h"
#include "lwip/tcpip.h"
#include "lwip/priv/tcp_priv.h"


/*******************************************************************************
 * DEFINITIONS
 ******************************************************************************/

// WF200 Debug trap used during debugging
#define WF200_DEBUG_TRAP   for(;;)

// TASK RELATED
#define WF200_INIT_TASK_PRIORITY        (configMAX_PRIORITIES - 4)
#define WF200_COMMS_TASK_PRIORITY       (configMAX_PRIORITIES - 3)
#define WF200_EVENT_TASK_PRIORITY       (configMAX_PRIORITIES - 5)
#define WF200_STA_MODE_TASK_PRIORITY    (configMAX_PRIORITIES - 6)
#define WF200_AP_MODE_TASK_PRIORITY     (configMAX_PRIORITIES - 7)
#define WF200_TEST_MODE_TASK_PRIORITY   (configMAX_PRIORITIES - 7)

#define WF200_INIT_TASK_STACK_SIZE        2000
#define WF200_COMMS_TASK_STACK_SIZE       2000
#define WF200_EVENT_TASK_STACK_SIZE       2000
#define WF200_STA_MODE_TASK_STACK_SIZE    2000
#define WF200_AP_MODE_TASK_STACK_SIZE     2000
#define WF200_TEST_MODE_TASK_STACK_SIZE   3000


// SCAN RELATED
#define WF200_SCAN_RESULT_01_CHANNEL_OFFSET   325
#define WF200_SCAN_RESULT_01_RSSI_OFFSET      14
#define WF200_SCAN_RESULT_01_MAC_OFFSET       35
#define WF200_SCAN_RESULT_01_NETWORK_OFFSET   68
#define WF200_SCAN_RESULT_BLOCK_SIZE          117
#define SL_WFX_MAX_SCAN_RESULTS               50


// WF200 GPIO Pins
#define GPIO1_PIN_03_WF200_CHIP_SELECT_PIN      03   // Used in conjunction with SPI as the Chip Select Pin
#define GPIO1_PIN_28_WF200_SPI_DATA_READY_INT   28   // SPI interrupt pin to indicate data is ready to be read
#define GPIO1_PIN_29_WF200_RESET_PIN            29   // Pin to allow the resetting of the WF200
#define GPIO1_PIN_30_WF200_WAKEUP_PIN           30   // Pin used to Wakeup the WF200 if it is in a low power mode
//// #define GPIO_PIN    PUSH SWITCH ??????


// Defaults for the Network to connect to, outside of
// testing these will not be used.
#define WLAN_SSID_DEFAULT       "FireAngel_Test"             // WiFi SSID for client mode
#define WLAN_PASSKEY_DEFAULT    "passkey"                    // WiFi password for client mode
#define WLAN_SECURITY_DEFAULT   WFM_SECURITY_MODE_WPA2_PSK   // WiFi security mode for client mode:
                                                             // WFM_SECURITY_MODE_OPEN/WFM_SECURITY_MODE_WEP/WFM_SECURITY_MODE_WPA2_WPA1_PSK

// Defaults for AP Mode, for input of Network details
// These can be changed by the command line
#define SOFTAP_SSID_DEFAULT       "FireAngel_WF200_AP"         // WiFi SSID for soft ap mode
#define SOFTAP_PASSKEY_DEFAULT    "FireAngel"                  // WiFi password for soft ap mode
#define SOFTAP_SECURITY_DEFAULT   WFM_SECURITY_MODE_WPA2_PSK   // WiFi security for soft ap mode:
                                                               // WFM_SECURITY_MODE_OPEN/WFM_SECURITY_MODE_WEP/WFM_SECURITY_MODE_WPA2_WPA1_PSK
#define SOFTAP_CHANNEL_DEFAULT    6                            // WiFi channel for soft ap

/************************** Access Point Static Default ****************************/
#define AP_IP_ADDR0_DEFAULT        (uint8_t) 10   ///< Static IP: IP address value 0
#define AP_IP_ADDR1_DEFAULT        (uint8_t) 10   ///< Static IP: IP address value 1
#define AP_IP_ADDR2_DEFAULT        (uint8_t) 0    ///< Static IP: IP address value 2
#define AP_IP_ADDR3_DEFAULT        (uint8_t) 1    ///< Static IP: IP address value 3

/*NETMASK*/
#define AP_NETMASK_ADDR0_DEFAULT   (uint8_t) 255  ///< Static IP: Netmask value 0
#define AP_NETMASK_ADDR1_DEFAULT   (uint8_t) 255  ///< Static IP: Netmask value 1
#define AP_NETMASK_ADDR2_DEFAULT   (uint8_t) 255  ///< Static IP: Netmask value 2
#define AP_NETMASK_ADDR3_DEFAULT   (uint8_t) 0    ///< Static IP: Netmask value 3

/*Gateway Address*/
#define AP_GW_ADDR0_DEFAULT        (uint8_t) 0    ///< Static IP: Gateway value 0
#define AP_GW_ADDR1_DEFAULT        (uint8_t) 0    ///< Static IP: Gateway value 1
#define AP_GW_ADDR2_DEFAULT        (uint8_t) 0    ///< Static IP: Gateway value 2
#define AP_GW_ADDR3_DEFAULT        (uint8_t) 0    ///< Static IP: Gateway value 3
/***************************************************************************/

// Events
#define SL_WFX_EVENT_MAX_SIZE   512
#define SL_WFX_EVENT_LIST_SIZE  1


/*******************************************************************************
 * TYPE DEFs
 ******************************************************************************/

typedef enum
{
	WiFi = 0,               // WiFi is used for platform comms
	LTE,                    // LTE is used for platform comms
	MAX_NUM_COMMS_METHODS
} eCommsMethod;


typedef enum
{
  GPIO_PIN_RESET = 0,
  GPIO_PIN_SET
}GPIO_PinState;


typedef enum
{
	WF200_INIT_TASK = 0,
	WF200_COMMS_TASK,
	WF200_EVENT_TASK,
	WF200_STA_MODE_TASK,
	WF200_AP_MODE_TASK,
	WF200_TEST_MODE_TASK,
	WF200_NUM_TASKS
} WF200_TaskIds;


typedef enum
{
	WF200_AP_LINK_STATUS_DOWN = 0,
	WF200_AP_LINK_STATUS_UP
} AP_LinkStatus_t;


typedef enum
{
	WF200_STA_LINK_STATUS_DOWN = 0,
	WF200_STA_LINK_STATUS_UP
} STA_LinkStatus_t;


typedef enum
{
	WF200_AP_NETIF_NOT_ADDED = 0,
	WF200_AP_NETIF_ADDED
} AP_NetIF_Status_t;


typedef enum
{
	WF200_STA_NETIF_NOT_ADDED = 0,
	WF200_STA_NETIF_ADDED
} STA_NetIF_Status_t;


typedef struct
{
	bool TaskCreated;            // Indicates that a Task has been created
	bool TaskRunning;            // Indicates that a Task is running
	TaskHandle_t TaskHandle;     // Creation Task Handle
	QueueHandle_t QueueHandle;   // Queue Handle
} WF200_TaskInfo_t;


typedef struct
{
	bool Awake;                            // Indicates if the WF200 is awake
	bool AP_ModeRequired;                  // Indicates if AP Mode is required, i.e. Network details are required to be input
	bool TestingModeRequired;              // Indicates if Testing Mode is required, for either testing or configuration
	bool STA_ModeRequired;                 // Indicates if STA Mode is required, normal mode of operation
	bool STA_CommandedDisconnect;          // A disconnect can occur by the FW or the Network side, indicates if the FW is disconnecting
	bool STA_ModeNetworkConnected;         // Indicates if the STA Mode has connected to a network
	AP_NetIF_Status_t AP_NetIF_Status;     // NETIF Added Status
	STA_NetIF_Status_t STA_NetIF_Status;   // NETIF Added Status
	AP_LinkStatus_t AP_LinkStatus;         // Link Status
	STA_LinkStatus_t STA_LinkStatus;       // Link Status
	EventGroupHandle_t EventGroupHandle;   // Handle for the Event Group
	SemaphoreHandle_t ReceiveFrameSemaphoreHandle;   // Handle for the Semaphore
	WF200_TaskInfo_t WF200_TaskInfo[WF200_NUM_TASKS];
} WF200_ControlInfo_t;



/*******************************************************************************
 * FUNCTION PROTOTYPES
 ******************************************************************************/

// Initialisation
sl_status_t WF200_CommsSetup(void);


// Mode Setup
sl_status_t WF200_Setup_AP_Mode(void);
sl_status_t WF200_Setup_STA_Mode(void);
sl_status_t WF200_Setup_Testing_Mode(void);


// Task Creation
sl_status_t WF200_CreateCommsTask(void);
sl_status_t WF200_CreateTestModeTask(void);
sl_status_t WF200_Create_AP_ModeTask(void);
sl_status_t WF200_Create_STA_ModeTask(void);
sl_status_t WF200_Create_WF200_InitTask(void);
sl_status_t WF200_CreateEventTask(void);


// Task Deletion
sl_status_t WF200_DeleteTask(const WF200_TaskIds taskId);

// Queue Deletion
sl_status_t WF200_DeleteQueue(const WF200_TaskIds taskId);


// Get/Set
const sl_wfx_context_t* WF200_Get_WF200_Context(void);
const WF200_ControlInfo_t* WF200_GetControlInfoPtr(void);
void WF200_SetTaskCreated(const WF200_TaskIds taskId, const bool taskCreated);
void WF200_SetTaskRunning(const WF200_TaskIds taskId, const bool taskRunning);
void WF200_SetTaskHandle(const WF200_TaskIds taskId, const TaskHandle_t taskHandle);
void WF200_SetQueueHandle(const WF200_TaskIds taskId, const QueueHandle_t queueHandle);
void WF200_SetAwake(const bool awake);
void WF200_Set_AP_ModeRequired(const bool AP_ModeRequired);
void WF200_SetTestingModeRequired(const bool testingModeRequired);
void WF200_Set_STA_ModeRequired(const bool STA_ModeRequired);
void WF200_SetSemaphoreHandle(const SemaphoreHandle_t receiveFrameSemaphoreHandle);
void WF200_Set_STA_CommandedDisconnect(const bool STA_CommandedDisconnect);
void WF200_Set_STA_ModeNetworkConnected(const bool STA_ModeNetorkConnected);
void WF200_SetEventGroupHandle(const EventGroupHandle_t EventGroupHandle);
void WF200_Set_AP_LinkStatus(const AP_LinkStatus_t LinkStatus);
void WF200_Set_STA_LinkStatus(const STA_LinkStatus_t LinkStatus);
void WF200_Set_AP_NetIF_Status(const AP_NetIF_Status_t LinkStatus);
void WF200_Set_STA_NetIF_Status(const STA_NetIF_Status_t LinkStatus);


// TCPI/IP - LWIP
err_t sta_ethernetif_init(struct netif *netif);
err_t ap_ethernetif_init(struct netif *netif);
sl_status_t WF200_AP_ModeSetupNetIF(void);
sl_status_t lwip_start (void);
sl_status_t lwip_set_sta_link_up(void);
sl_status_t lwip_set_sta_link_down(void);
sl_status_t lwip_set_ap_link_up(void);
sl_status_t lwip_set_ap_link_down(void);
sl_status_t lwip_netif_setup(void);
void sl_lwip_process(void);
void dhcpserver_start(void);
struct netif* WF200_Get_AP_NetIF(void);
struct netif* WF200_Get_STA_NetIF(void);
void WF200_Set_AP_NetIF(struct netif *netif);
void WF200_Set_STA_NetIF(struct netif *netif);


sl_status_t WF200_Init(void);
sl_status_t WF200_ShutDown(void);
void sl_wfx_scan_result_callback( sl_wfx_scan_result_ind_body_t* scan_result );


// Test task to carry out a MBEDTLS test
void httpsclient_task(void *arg);



#endif /* WF200_LWIP_HOST_COMMON_H_ */