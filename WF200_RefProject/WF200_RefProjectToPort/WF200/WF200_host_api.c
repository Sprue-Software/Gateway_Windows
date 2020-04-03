/*
 * WF200_host_api.c
 *
 *  Created on: 11 Mar 2020
 *      Author: SMurva
 */

/*******************************************************************************
 * Includes
 ******************************************************************************/

#include "WF200_host_common.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "sl_wfx.h"
#include "sl_wfx_host.h"

/* Firmware include */
#include "sl_wfx_wf200_C0.h"

/* Platform Data Set (PDS) include */
#include "brd8022a_pds.h"
#include "brd8023a_pds.h"

#include "fsl_gpio.h"
#include "event_groups.h"
#include "dhcp_server.h"


/*******************************************************************************
 * Definitions
 ******************************************************************************/



/*******************************************************************************
 * Variables
 ******************************************************************************/
extern volatile bool scan_ongoing;

uint8_t *pClient_MAC;
scan_result_list_t scan_list[SL_WFX_MAX_SCAN_RESULTS];
uint8_t scan_count = 0;
uint8_t scan_count_web = 0;

struct
{
    uint32_t sl_wfx_firmware_download_progress;
    uint8_t waited_event_id;
}host_context;

char event_log[50];


/*******************************************************************************
 * FUNCTION PROTOTYPES
 ******************************************************************************/



/*******************************************************************************
 * FUNCTION DEFINITIONS
 ******************************************************************************/

/*******************************************************************************
 * @brief       sl_wfx_host_init
 *
 * @param[in]   None
 *
 * @return      Error Status
 *
 ******************************************************************************/
sl_status_t sl_wfx_host_init(void)
{
	QueueHandle_t queueHandle = NULL;

	host_context.sl_wfx_firmware_download_progress = 0;

	queueHandle = xQueueCreate(SL_WFX_EVENT_LIST_SIZE, sizeof(uint8_t));

    if(queueHandle != NULL)
    {
    	WF200_SetQueueHandle(WF200_COMMS_TASK, queueHandle);
    }
    else
    {
	    PRINTF("sl_wfx_host_init: Failed to create queue\n");
	    WF200_DEBUG_TRAP;
    }

    return SL_STATUS_OK;
}


/*******************************************************************************
 * @brief       sl_wfx_host_get_firmware_data
 *
 * @param[in]   Pointer to data
 *              Data Size
 *
 * @return      Error Status
 *
 ******************************************************************************/
sl_status_t sl_wfx_host_get_firmware_data(const uint8_t** data, uint32_t data_size)
{
	assert(data_size < 10000);

    *data = &sl_wfx_firmware[host_context.sl_wfx_firmware_download_progress];
    host_context.sl_wfx_firmware_download_progress += data_size;

    return SL_STATUS_OK;
}


/*******************************************************************************
 * @brief       sl_wfx_host_get_firmware_size
 *
 * @param[in]   Pointer to FW size
 *
 * @return      Error Status
 *
 ******************************************************************************/
sl_status_t sl_wfx_host_get_firmware_size(uint32_t* firmware_size)
{
	assert(firmware_size != NULL);

	*firmware_size = sizeof(sl_wfx_firmware);
    return SL_STATUS_OK;
}


/*******************************************************************************
 * @brief       sl_wfx_host_get_pds_data
 *
 *
 * @param[in]   Pointer to PDS data
 *              Index
 *
 * @return      Error Status
 *
 ******************************************************************************/
sl_status_t sl_wfx_host_get_pds_data(const char **pds_data, uint16_t index)
{
	// Manage dynamically the PDS in function of the chip connected
    if (strncmp("WFM200", (char *)sl_wfx_context->wfx_opn, 6) == 0)
    {
    	*pds_data = pds_table_brd8023a[index];
    }
    else
    {
    	*pds_data = pds_table_brd8022a[index];
    }

    return SL_STATUS_OK;
}


/*******************************************************************************
 * @brief       sl_wfx_host_get_pds_size
 *
 * @param[in]   Pointer to PDS size
 *
 * @return      Error Status
 *
 ******************************************************************************/
sl_status_t sl_wfx_host_get_pds_size(uint16_t *pds_size)
{
	assert(pds_size != NULL);

	// Manage dynamically the PDS in function of the chip connected
    if (strncmp("WFM200", (char *)sl_wfx_context->wfx_opn, 6) == 0)
    {
    	*pds_size = SL_WFX_ARRAY_COUNT(pds_table_brd8023a);
    }
    else
    {
    	*pds_size = SL_WFX_ARRAY_COUNT(pds_table_brd8022a);
    }

    return SL_STATUS_OK;
}


/*******************************************************************************
 * @brief       sl_wfx_host_deinit
 *
 * @param[in]   None
 *
 * @return      Error Status
 *
 ******************************************************************************/
sl_status_t sl_wfx_host_deinit(void)
{
  return SL_STATUS_OK;
}


/*******************************************************************************
 * @brief       sl_wfx_host_reset_chip
 *
 * @param[in]   None
 *
 * @return      Error Status
 *
 ******************************************************************************/
sl_status_t sl_wfx_host_reset_chip(void)
{
	// Hold pin high to get chip out of reset
	GPIO_PinWrite(GPIO1, GPIO1_PIN_29_WF200_RESET_PIN, GPIO_PIN_RESET);


    // Create a delay, to allow the Wi-Fi module to come
    // out of reset, Will use appropriate timers in system implementation
    for (volatile uint32_t delayLoop = 0; delayLoop < 500000; delayLoop++)
    {
        __ASM("nop");
    }

	GPIO_PinWrite(GPIO1, GPIO1_PIN_29_WF200_RESET_PIN, GPIO_PIN_SET);

    for (volatile uint32_t delayLoop = 0; delayLoop < 500000; delayLoop++)
    {
        __ASM("nop");
    }

	return SL_STATUS_OK;
}


/*******************************************************************************
 * @brief       sl_wfx_host_hold_in_reset
 *
 * @param[in]   None
 *
 * @return      Error Status
 *
 ******************************************************************************/
sl_status_t sl_wfx_host_hold_in_reset(void)
{
	GPIO_PinWrite(GPIO1, GPIO1_PIN_29_WF200_RESET_PIN, GPIO_PIN_RESET);

	return SL_STATUS_OK;
}


/*******************************************************************************
 * @brief       sl_wfx_host_set_wake_up_pin
 *
 * @param[in]   state
 *
 * @return      Error Status
 *
 ******************************************************************************/
sl_status_t sl_wfx_host_set_wake_up_pin(uint8_t state)
{
    if ( state > 0 )
    {
    	GPIO_PinWrite(GPIO1, GPIO1_PIN_30_WF200_WAKEUP_PIN, GPIO_PIN_SET);
    }
    else
    {
    	GPIO_PinWrite(GPIO1, GPIO1_PIN_30_WF200_WAKEUP_PIN, GPIO_PIN_RESET);
    }

    return SL_STATUS_OK;
}


/*******************************************************************************
 * @brief       sl_wfx_host_wait_for_wake_up
 *
 * @param[in]   None
 *
 * @return      Error Status
 *
 ******************************************************************************/
sl_status_t sl_wfx_host_wait_for_wake_up(void)
{
	const WF200_ControlInfo_t *pControlInfo = WF200_GetControlInfoPtr();

    xEventGroupClearBits(pControlInfo->EventGroupHandle, SL_WFX_INTERRUPT);
    xEventGroupWaitBits(pControlInfo->EventGroupHandle, SL_WFX_INTERRUPT, pdTRUE, pdTRUE, 3/portTICK_PERIOD_MS);

    return SL_STATUS_OK;
}


/*******************************************************************************
 * @brief       sl_wfx_host_sleep_grant
 *
 * @param[in]
 *
 * @return      Error Status
 *
 ******************************************************************************/
sl_status_t sl_wfx_host_sleep_grant(sl_wfx_host_bus_transfer_type_t type,
                                    sl_wfx_register_address_t address,
                                    uint32_t length)
{
  return SL_STATUS_WIFI_SLEEP_GRANTED;
}


/*******************************************************************************
 * @brief       sl_wfx_host_setup_waited_event
 *
 * @param[in]   event_id
 *
 * @return      Error Status
 *
 ******************************************************************************/
sl_status_t sl_wfx_host_setup_waited_event(uint8_t event_id)
{
  host_context.waited_event_id = event_id;

  return SL_STATUS_OK;
}


/*******************************************************************************
 * @brief       sl_wfx_host_wait_for_confirmation
 *
 *
 * @param[in]   confirmation_id
 *              timeout_ms
 *              event_payload_out
 *
 * @return      Error Status
 *
 ******************************************************************************/
sl_status_t sl_wfx_host_wait_for_confirmation(uint8_t confirmation_id,
                                              uint32_t timeout_ms,
                                              void **event_payload_out)
{
  BaseType_t status;
  uint8_t posted_event_id;
  const WF200_ControlInfo_t *pControlInfo = WF200_GetControlInfoPtr();

  if(pControlInfo == NULL)
  {
	PRINTF("sl_wfx_host_wait_for_confirmation: Error NULL Ptr\n");
	WF200_DEBUG_TRAP;
  }

  for(uint32_t i = 0; i < timeout_ms; i++)
  {
    /* Wait for an event posted by the function sl_wfx_host_post_event() */
	status = xQueueReceive(pControlInfo->WF200_TaskInfo[WF200_COMMS_TASK].QueueHandle,
			    &posted_event_id, 1);

    if(status == pdTRUE)
    {
      /* Once a message is received, check if it is the expected ID */
      if (confirmation_id == posted_event_id)
      {
        /* Pass the confirmation reply and return*/
        if ( event_payload_out != NULL )
        {
          *event_payload_out = sl_wfx_context->event_payload_buffer;
        }
        return SL_STATUS_OK;
      }
    }
  }
  /* The wait for the confirmation timed out, return */
  return SL_STATUS_TIMEOUT;
}


/*******************************************************************************
 * @brief       sl_wfx_host_wait
 *
 *
 * @param[in]   wait_time
 *
 * @return      Error Status
 *
 ******************************************************************************/
sl_status_t sl_wfx_host_wait(uint32_t wait_time)
{
	// Will be replaced with proper system timer
    for (volatile uint32_t delayLoop = 0; delayLoop < 500000; delayLoop++)
    {
        __ASM("nop");
    }

	return SL_STATUS_OK;
}


/*******************************************************************************
 * @brief       sl_wfx_host_post_event
 *
 * @param[in]   event_payload
 *
 * @return      Error Status
 *
 ******************************************************************************/
sl_status_t sl_wfx_host_post_event(sl_wfx_generic_message_t *event_payload)
{
  const WF200_ControlInfo_t *pControlInfo = WF200_GetControlInfoPtr();

  if(pControlInfo == NULL)
  {
	PRINTF("sl_wfx_host_post_event: Error NULL Ptr\n");
	WF200_DEBUG_TRAP;
  }

  switch(event_payload->header.id){
    /******** INDICATION ********/
  case SL_WFX_CONNECT_IND_ID:
    {
      sl_wfx_connect_ind_t* connect_indication = (sl_wfx_connect_ind_t*) event_payload;
      sl_wfx_connect_callback(connect_indication->body.mac, connect_indication->body.status);
      break;
    }
  case SL_WFX_DISCONNECT_IND_ID:
    {
      sl_wfx_disconnect_ind_t* disconnect_indication = (sl_wfx_disconnect_ind_t*) event_payload;
      sl_wfx_disconnect_callback(disconnect_indication->body.mac, disconnect_indication->body.reason);
      break;
    }
  case SL_WFX_START_AP_IND_ID:
    {
      sl_wfx_start_ap_ind_t* start_ap_indication = (sl_wfx_start_ap_ind_t*) event_payload;
      sl_wfx_start_ap_callback(start_ap_indication->body.status);
      break;
    }
  case SL_WFX_STOP_AP_IND_ID:
    {
      sl_wfx_stop_ap_callback();
      break;
    }
  case SL_WFX_RECEIVED_IND_ID:
    {
      sl_wfx_received_ind_t* ethernet_frame = (sl_wfx_received_ind_t*) event_payload;
      if ( ethernet_frame->body.frame_type == 0 )
      {
        sl_wfx_host_received_frame_callback( ethernet_frame );
      }
      break;
    }
  case SL_WFX_SCAN_RESULT_IND_ID:
    {
      sl_wfx_scan_result_ind_t* scan_result = (sl_wfx_scan_result_ind_t*) event_payload;
      sl_wfx_scan_result_callback(&scan_result->body);
      break;
    }
  case SL_WFX_SCAN_COMPLETE_IND_ID:
    {
      sl_wfx_scan_complete_ind_t* scan_complete = (sl_wfx_scan_complete_ind_t*) event_payload;
      sl_wfx_scan_complete_callback(scan_complete->body.status);
      break;
    }
  case SL_WFX_AP_CLIENT_CONNECTED_IND_ID:
    {
      sl_wfx_ap_client_connected_ind_t* client_connected_indication = (sl_wfx_ap_client_connected_ind_t*) event_payload;
      sl_wfx_client_connected_callback(client_connected_indication->body.mac);
      break;
    }
  case SL_WFX_AP_CLIENT_REJECTED_IND_ID:
    {
      sl_wfx_ap_client_rejected_ind_t* ap_client_rejected_indication = (sl_wfx_ap_client_rejected_ind_t*) event_payload;
      sl_wfx_ap_client_rejected_callback(ap_client_rejected_indication->body.reason, ap_client_rejected_indication->body.mac);
      break;
    }
  case SL_WFX_AP_CLIENT_DISCONNECTED_IND_ID:
    {
      sl_wfx_ap_client_disconnected_ind_t* ap_client_disconnected_indication = (sl_wfx_ap_client_disconnected_ind_t*) event_payload;
      sl_wfx_ap_client_disconnected_callback(ap_client_disconnected_indication->body.reason, ap_client_disconnected_indication->body.mac);
      break;
    }
  case SL_WFX_GENERIC_IND_ID:
    {
      break;
    }
  case SL_WFX_EXCEPTION_IND_ID:
    {
      PRINTF("Firmware Exception\r\n");
      sl_wfx_exception_ind_t *firmware_exception = (sl_wfx_exception_ind_t*)event_payload;
      PRINTF ("Exception data = ");
      for(int i = 0; i < SL_WFX_EXCEPTION_DATA_SIZE; i++)
      {
        PRINTF ("%X, ", firmware_exception->body.data[i]);
      }
      PRINTF("\r\n");
      break;
    }
  case SL_WFX_ERROR_IND_ID:
    {
      PRINTF("Firmware Error\r\n");
      sl_wfx_error_ind_t *firmware_error = (sl_wfx_error_ind_t*)event_payload;
      PRINTF ("Error type = %lu\r\n",firmware_error->body.type);
      break;
    }
  }

  if(host_context.waited_event_id == event_payload->header.id)
  {
    if(event_payload->header.length < SL_WFX_EVENT_MAX_SIZE)
    {
      /* Post the event in the queue */
      memcpy( sl_wfx_context->event_payload_buffer,
             (void*) event_payload,
             event_payload->header.length );
      xQueueOverwrite(pControlInfo->WF200_TaskInfo[WF200_COMMS_TASK].QueueHandle,
    		  (void *) &event_payload->header.id);
    }
  }

  return SL_STATUS_OK;
}


/*******************************************************************************
 * @brief       sl_wfx_host_allocate_buffer
 *
 * @param[in]   Pointer to buffer
 *              Buffer Type
 *              Buffer Size
 *
 * @return      Error Status
 *
 ******************************************************************************/
sl_status_t sl_wfx_host_allocate_buffer(void** buffer, sl_wfx_buffer_type_t type, uint32_t buffer_size)
{
  *buffer = pvPortMalloc( buffer_size );

  return SL_STATUS_OK;
}


/*******************************************************************************
 * @brief       sl_wfx_host_free_buffer
 *
 * @param[in]   Pointer to buffer
 *              type
 *
 * @return      Error Status
 *
 ******************************************************************************/
sl_status_t sl_wfx_host_free_buffer(void* buffer, sl_wfx_buffer_type_t type)
{
  vPortFree(buffer);

  return SL_STATUS_OK;
}


/*******************************************************************************
 * @brief       sl_wfx_host_transmit_frame
 *
 * @param[in]   Pointer to frame
 *              Frame length
 *
 * @return      Error Status
 *
 ******************************************************************************/
sl_status_t sl_wfx_host_transmit_frame(void* frame, uint32_t frame_len)
{
  return sl_wfx_data_write( frame, frame_len );
}

#if SL_WFX_DEBUG_MASK
void sl_wfx_host_log(const char *string, ...)
{
  va_list valist;

  va_start(valist, string);
  vPRINTF(string, valist);
  va_end(valist);
}
#endif



/*******************************************************************************
 * @brief       sl_wfx_host_lock
 *
 * @param[in]   None
 *
 * @return      Error Status
 *
 ******************************************************************************/
sl_status_t sl_wfx_host_lock(void)
{
	sl_status_t status = SL_STATUS_FAIL;
	const WF200_ControlInfo_t *pControlInfo = WF200_GetControlInfoPtr();

	if(pControlInfo == NULL)
	{
		PRINTF("sl_wfx_host_lock: ERROR - NULL Ptr\n");
		WF200_DEBUG_TRAP;
	}

    if(pControlInfo->ReceiveFrameSemaphoreHandle == NULL)
    {
    	PRINTF("sl_wfx_host_lock: ERROR - Trying to use a NULL Semaphore Ptr\n");
	    WF200_DEBUG_TRAP;
    }

    if(xSemaphoreTake(pControlInfo->ReceiveFrameSemaphoreHandle, 500) == pdTRUE)
    {
       status = SL_STATUS_OK;
    }
    else
    {
    	PRINTF("WF200 Driver Mutex Timeout\r\n");
        status = SL_STATUS_TIMEOUT;
    }

    return status;
}


/*******************************************************************************
 * @brief       sl_wfx_host_unlock
 *
 * @param[in]   None
 *
 * @return      Error Status
 *
 ******************************************************************************/
sl_status_t sl_wfx_host_unlock(void)
{
	sl_status_t status = SL_STATUS_FAIL;
	const WF200_ControlInfo_t *pControlInfo = WF200_GetControlInfoPtr();

	if(pControlInfo == NULL)
	{
		PRINTF("sl_wfx_host_unlock: ERROR - NULL Ptr\n");
		WF200_DEBUG_TRAP;
	}

	if(pControlInfo->ReceiveFrameSemaphoreHandle == NULL)
	{
		PRINTF("sl_wfx_host_unlock: ERROR - Trying to use a NULL Semaphore Ptr\n");
		WF200_DEBUG_TRAP;
	}

	if(xSemaphoreGive(pControlInfo->ReceiveFrameSemaphoreHandle) == pdTRUE)
    {
		status = SL_STATUS_OK;
	}

    return status;
}


/*******************************************************************************
 * @brief        sl_wfx_scan_result_callback: Callback for individual scan
 *               result
 *
 * @param[in]    scan_result
 *
 * @return       None
 *
 ******************************************************************************/
void sl_wfx_scan_result_callback(sl_wfx_scan_result_ind_body_t* scan_result)
{
  scan_count++;
  PRINTF(
    "# %2d %2d  %03d %02X:%02X:%02X:%02X:%02X:%02X  %s",
    scan_count,
    scan_result->channel,
    ((int16_t)(scan_result->rcpi - 220) / 2),
    scan_result->mac[0], scan_result->mac[1],
    scan_result->mac[2], scan_result->mac[3],
    scan_result->mac[4], scan_result->mac[5],
    scan_result->ssid_def.ssid);
  /*Report one AP information*/
  PRINTF("\r\n");
  if (scan_count <= SL_WFX_MAX_SCAN_RESULTS) {
    scan_list[scan_count - 1].ssid_def = scan_result->ssid_def;
	scan_list[scan_count - 1].channel = scan_result->channel;
	scan_list[scan_count - 1].security_mode = scan_result->security_mode;
	scan_list[scan_count - 1].rcpi = scan_result->rcpi;
	memcpy(scan_list[scan_count - 1].mac, scan_result->mac, 6);
  }
}


/*******************************************************************************
 * @brief       sl_wfx_scan_complete_callback: Callback for scan complete
 *
 *
 * @param[in]   status
 *
 * @return      None
 *
 ******************************************************************************/
void sl_wfx_scan_complete_callback(uint32_t status)
{
	const WF200_ControlInfo_t *pControlInfo = WF200_GetControlInfoPtr();

	scan_count_web = scan_count;
    scan_count = 0;
    xEventGroupSetBits(pControlInfo->EventGroupHandle, SL_WFX_SCAN_COMPLETE);
    scan_ongoing = false;
}


/*******************************************************************************
 * @brief sl_wfx_connect_callback: Callback when station connects
 *
 * @param[in]   mac
 *              status
 *
 * @return      None
 *
 ******************************************************************************/
void sl_wfx_connect_callback(uint8_t* mac, uint32_t status)
{
  const WF200_ControlInfo_t *pControlInfo = WF200_GetControlInfoPtr();

  if(pControlInfo == NULL)
  {
	PRINTF("sl_wfx_connect_callback: Error NULL Ptr\n");
	WF200_DEBUG_TRAP;
  }

  switch(status){
  case WFM_STATUS_SUCCESS:
    {
      PRINTF("Connected\r\n");
      sl_wfx_context->state |= SL_WFX_STA_INTERFACE_CONNECTED;
      xEventGroupSetBits(pControlInfo->EventGroupHandle, SL_WFX_CONNECT);
      break;
    }
  case WFM_STATUS_NO_MATCHING_AP:
    {
      strcpy(event_log, "Connection failed, access point not found");
      PRINTF(event_log);
      PRINTF("\r\n");
      break;
    }
  case WFM_STATUS_CONNECTION_ABORTED:
    {
      strcpy(event_log, "Connection aborted");
      PRINTF(event_log);
      PRINTF("\r\n");
      break;
    }
  case WFM_STATUS_CONNECTION_TIMEOUT:
    {
      strcpy(event_log, "Connection timeout");
      PRINTF(event_log);
      PRINTF("\r\n");
      break;
    }
  case WFM_STATUS_CONNECTION_REJECTED_BY_AP:
    {
      strcpy(event_log, "Connection rejected by the access point");
      PRINTF(event_log);
      PRINTF("\r\n");
      break;
    }
  case WFM_STATUS_CONNECTION_AUTH_FAILURE:
    {
      strcpy(event_log, "Connection authentication failure");
      PRINTF(event_log);
      PRINTF("\r\n");
      break;
    }
  default:
    {
      strcpy(event_log, "Connection attempt error");
      PRINTF(event_log);
      PRINTF("\r\n");
    }
  }
}


/*******************************************************************************
 * @brief       sl_wfx_disconnect_callback: Callback for station disconnect
 *
 * @param[in]   mac
 *              reason
 *
 * @return      None
 *
 ******************************************************************************/
void sl_wfx_disconnect_callback(uint8_t* mac, uint16_t reason)
{
  const WF200_ControlInfo_t *pControlInfo = WF200_GetControlInfoPtr();

  if(pControlInfo == NULL)
  {
	PRINTF("sl_wfx_disconnect_callback: Error NULL Ptr\n");
	WF200_DEBUG_TRAP;
  }

  PRINTF("Disconnected %d\r\n", reason);
  sl_wfx_context->state &= ~SL_WFX_STA_INTERFACE_CONNECTED;
  xEventGroupSetBits(pControlInfo->EventGroupHandle, SL_WFX_DISCONNECT);

  WF200_Set_STA_ModeNetworkConnected(false);
}


/*******************************************************************************
 * @brief       sl_wfx_start_ap_callback: Callback for AP started
 *
 * @param[in]   status
 *
 * @return      None
 *
 ******************************************************************************/
void sl_wfx_start_ap_callback(uint32_t status)
{
	const WF200_ControlInfo_t *pControlInfo = WF200_GetControlInfoPtr();

    if (status == 0)
    {
    	PRINTF("AP started\r\n");
        sl_wfx_context->state |= SL_WFX_AP_INTERFACE_UP;
        xEventGroupSetBits(pControlInfo->EventGroupHandle, SL_WFX_START_AP);
    }
    else
    {
    	PRINTF("AP start failed\r\n");
        //  strcpy(event_log, "AP start failed");
    }
}


/*******************************************************************************
 * @brief       sl_wfx_stop_ap_callback: Callback for AP stopped
 *
 * @param[in]
 *
 * @return
 *    None
 ******************************************************************************/
void sl_wfx_stop_ap_callback(void)
{
	const WF200_ControlInfo_t *pControlInfo = WF200_GetControlInfoPtr();

	dhcpserver_clear_stored_mac ();
    PRINTF("SoftAP stopped\r\n");
    sl_wfx_context->state &= ~SL_WFX_AP_INTERFACE_UP;
    xEventGroupSetBits(pControlInfo->EventGroupHandle, SL_WFX_STOP_AP);
}


/*******************************************************************************
 * @brief       sl_wfx_client_connected_callback
 *
 * @param[in]   mac
 *
 * @return      None
 *    None
 ******************************************************************************/
void sl_wfx_client_connected_callback(uint8_t* mac)
{
  // Save the MAC for
  // disconnection purposes
  pClient_MAC = mac;

  PRINTF("Client connected, MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
         mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  PRINTF("Open a web browser and go to http://%d.%d.%d.%d\r\n",
         AP_IP_ADDR0_DEFAULT, AP_IP_ADDR1_DEFAULT, AP_IP_ADDR2_DEFAULT, AP_IP_ADDR3_DEFAULT);
}


/*******************************************************************************
 * @brief       sl_wfx_ap_client_rejected_callback: Callback for client rejected
 *              from AP
 *
 * @param[in]   status
 *              mac
 *
 * @return      None
 *
 ******************************************************************************/
void sl_wfx_ap_client_rejected_callback(uint32_t status, uint8_t* mac)
{
  struct eth_addr mac_addr;
  memcpy(&mac_addr, mac, SL_WFX_BSSID_SIZE);
  dhcpserver_remove_mac(&mac_addr);
  PRINTF("Client rejected, reason: %d, MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
         (int)status, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}


/*******************************************************************************
 * @brief       sl_wfx_ap_client_disconnected_callback: Callback for AP client
 *              disconnect
 *
 * @param[in]   satus
 *              mac
 *
 * @return      None
 *
 ******************************************************************************/
void sl_wfx_ap_client_disconnected_callback(uint32_t status, uint8_t* mac)
{
  struct eth_addr mac_addr;
  memcpy(&mac_addr, mac, SL_WFX_BSSID_SIZE);
  dhcpserver_remove_mac(&mac_addr);
  PRINTF("Client disconnected, reason: %d, MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
         (int)status, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}
