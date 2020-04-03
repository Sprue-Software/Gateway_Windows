/*
 * WF200_netif.c
 *
 *  Created on: 12 Mar 2020
 *      Author: SMurva
 */

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include "WF200_host_common.h"
#include "lwip/netifapi.h"
#include "dhcp_server.h"


/*****************************************************************************
 * DEFINES
******************************************************************************/

#define USE_DHCP_CLIENT_DEFAULT    1   // If defined, DHCP is enabled,
                                       // otherwise static address below is used
#define STATION_NETIF0 's'
#define STATION_NETIF1 't'
#define SOFTAP_NETIF0  'a'
#define SOFTAP_NETIF1  'p'


/*******************************************************************************
 * VARIABLES
 ******************************************************************************/

/* station and softAP network interface structures */
static struct netif sta_netif;
static struct netif ap_netif;

int use_dhcp_client = USE_DHCP_CLIENT_DEFAULT;



/*******************************************************************************
 * FUNCTION PROTOTYPES
 ******************************************************************************/


/*******************************************************************************
 * FUNCTION DEFINITIONS
 ******************************************************************************/

/******************************************************************************
 * @brief       lwip_set_sta_link_up: Set station link status to up.
 *
 * @param[in]   None
 *
 * @return      Error Status
 *
 ******************************************************************************/
sl_status_t lwip_set_sta_link_up(void)
{
	err_t err;
	sl_status_t WF200_DriverStatus = SL_STATUS_OK;

	err = netifapi_netif_set_up(&sta_netif);

	if(err != ERR_OK)
	{
		// Log failure
		PRINTF("lwip_set_sta_link_up: ERROR netif set up fail = %i", err);
		WF200_DriverStatus = SL_STATUS_FAIL;
		WF200_DEBUG_TRAP;
	}

	if(WF200_DriverStatus == SL_STATUS_OK)
	{
		err = netifapi_netif_set_link_up(&sta_netif);
	}

	if(err != ERR_OK)
	{
		// Log failure
		PRINTF("lwip_set_sta_link_up: ERROR - netif set sta link up fail = %i", err);
		WF200_DriverStatus = SL_STATUS_FAIL;
		WF200_DEBUG_TRAP;
	}

	if(WF200_DriverStatus == SL_STATUS_OK)
	{
		// Update the Status in the Control Structure
		WF200_Set_STA_LinkStatus(WF200_STA_LINK_STATUS_UP);
	}

	if (use_dhcp_client)
	{
		//dhcpclient_set_link_state(1); // ??
	}

    return WF200_DriverStatus;
}


/******************************************************************************
 * @brief       lwip_set_sta_link_down: Set station link status to down.
 *
 * @param[in]   None
 *
 * @return      Error Status
 *
 ******************************************************************************/
sl_status_t lwip_set_sta_link_down(void)
{
	err_t err;
	sl_status_t WF200_DriverStatus = SL_STATUS_OK;

    if (use_dhcp_client)
    {
    	// dhcpclient_set_link_state(0);
    }

    err = netifapi_netif_set_link_down(&sta_netif);

	if(err != ERR_OK)
	{
		// Log failure
		PRINTF("lwip_set_sta_link_down: ERROR - netif set sta link down fail = %i", err);
		WF200_DriverStatus = SL_STATUS_FAIL;
		WF200_DEBUG_TRAP;
	}

	err = netifapi_netif_set_down(&sta_netif);

	if(err != ERR_OK)
	{
		// Log failure
		PRINTF("lwip_set_sta_link_down: ERROR - netif set down fail = %i", err);
		WF200_DriverStatus = SL_STATUS_FAIL;
		WF200_DEBUG_TRAP;
	}

	if(WF200_DriverStatus == SL_STATUS_OK)
	{
	    // Update the Status in the Control Structure
	    WF200_Set_STA_LinkStatus(WF200_STA_LINK_STATUS_DOWN);
	}

    return WF200_DriverStatus;
}


/******************************************************************************
 * @brief       lwip_set_ap_link_up: Set AP link status to up.
 *
 * @param[in]   None
 *
 * @return      Error Status
 *
 ******************************************************************************/
sl_status_t lwip_set_ap_link_up(void)
{
	err_t err;
	sl_status_t WF200_DriverStatus = SL_STATUS_OK;

    err = netifapi_netif_set_up(&ap_netif);

    if(err != ERR_OK)
	{
    	// Log failure
		PRINTF("lwip_set_ap_link_up: ERROR - netif set up fail = %i", err);
		WF200_DriverStatus = SL_STATUS_FAIL;
		WF200_DEBUG_TRAP;
	}

    err = netifapi_netif_set_link_up(&ap_netif);

	if(err != ERR_OK)
	{
		// Log failure
		PRINTF("lwip_set_ap_link_up: ERROR - netif set link up fail = %i", err);
		WF200_DriverStatus = SL_STATUS_FAIL;
		WF200_DEBUG_TRAP;
	}

	if(WF200_DriverStatus == SL_STATUS_OK)
	{
		// Update the Status in the Control Structure
		WF200_Set_AP_LinkStatus(WF200_AP_LINK_STATUS_UP);
	}

    dhcpserver_start();

    return WF200_DriverStatus;
}


/******************************************************************************
 * @brief       lwip_set_ap_link_down: Set AP link status to down.
 *
 * @param[in]   None
 *
 * @return      Error Status
 *
 ******************************************************************************/
sl_status_t lwip_set_ap_link_down(void)
{
	err_t err;
	sl_status_t WF200_DriverStatus = SL_STATUS_OK;

	dhcpserver_stop();

	err = netifapi_netif_set_link_down(&ap_netif);

	if(err != ERR_OK)
	{
		// Log failure
		PRINTF("lwip_set_ap_link_down: ERROR - netif set link down fail = %i", err);
		WF200_DriverStatus = SL_STATUS_FAIL;
		WF200_DEBUG_TRAP;
	}

    err = netifapi_netif_set_down(&ap_netif);

	if(err != ERR_OK)
	{
		// Log failure
		PRINTF("lwip_set_ap_link_down: ERROR: - netif set down fail = %i", err);
		WF200_DriverStatus = SL_STATUS_FAIL;
		WF200_DEBUG_TRAP;
	}

	if(WF200_DriverStatus == SL_STATUS_OK)
	{
		// Update the Status in the Control Structure
	    WF200_Set_AP_LinkStatus(WF200_AP_LINK_STATUS_DOWN);
	}

    return WF200_DriverStatus;
}


/******************************************************************************
 * @brief       WF200_Get_AP_NetIF: returns pointer to the AP NetIF structure
 *
 * @param[in]   None
 *
 * @return      Pointer to the AP NetIF
 *
 ******************************************************************************/
struct netif* WF200_Get_AP_NetIF(void)
{
	return &ap_netif;
}


/******************************************************************************
 * @brief       WF200_Get_STA_NetIF: returns pointer to the STA NetIF structure
 *
 * @param[in]   None
 *
 * @return      Pointer to the STA NetIF
 *
 ******************************************************************************/
struct netif* WF200_Get_STA_NetIF(void)
{
	return &sta_netif;
}


/******************************************************************************
 * @brief       WF200_Set_STA_NetIF: Copies over input to the NetIF
 *
 * @param[in]   netif pointer
 *
 * @return      None
 *
 ******************************************************************************/
void WF200_Set_STA_NetIF(struct netif *netif)
{
	sta_netif = *netif;
}


/******************************************************************************
 * @brief       WF200_Set_STA_NetIF: Copies over input to the NetIF
 *
 * @param[in]   netif pointer
 *
 * @return      None
 *
 ******************************************************************************/
void WF200_Set_AP_NetIF(struct netif *netif)
{
	ap_netif = *netif;
}


/*****************************************************************************
 * @brief
 *    Initializes the hardware parameters. Called from ethernetif_init().
 *
 * @param[in] netif: the already initialized lwip network interface structure
 *
 * @return
*    None
******************************************************************************/
static void low_level_init(struct netif *netif)
{
	const sl_wfx_context_t *pWF200_Context;

    /* set netif MAC hardware address length */
    netif->hwaddr_len = ETH_HWADDR_LEN;

    pWF200_Context = WF200_Get_WF200_Context();

    /* Check which netif is initialized and set netif MAC hardware address */
    if (netif->name[0] == SOFTAP_NETIF0)
    {
    	netif->hwaddr[0] =  pWF200_Context->mac_addr_1.octet[0];
        netif->hwaddr[1] =  pWF200_Context->mac_addr_1.octet[1];
        netif->hwaddr[2] =  pWF200_Context->mac_addr_1.octet[2];
        netif->hwaddr[3] =  pWF200_Context->mac_addr_1.octet[3];
        netif->hwaddr[4] =  pWF200_Context->mac_addr_1.octet[4];
        netif->hwaddr[5] =  pWF200_Context->mac_addr_1.octet[5];
  }
  else
  {
	  netif->hwaddr[0] =  pWF200_Context->mac_addr_0.octet[0];
      netif->hwaddr[1] =  pWF200_Context->mac_addr_0.octet[1];
      netif->hwaddr[2] =  pWF200_Context->mac_addr_0.octet[2];
      netif->hwaddr[3] =  pWF200_Context->mac_addr_0.octet[3];
      netif->hwaddr[4] =  pWF200_Context->mac_addr_0.octet[4];
      netif->hwaddr[5] =  pWF200_Context->mac_addr_0.octet[5];
  }

  /* set netif maximum transfer unit */
  netif->mtu = 1500;

  /* Accept broadcast address and ARP traffic */
  netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;

  /* Set netif link flag */
  netif->flags |= NETIF_FLAG_LINK_UP;
}

/******************************************************************************
 * @brief
 *    This function should does the actual transmission of the packet(s).
 *    The packet is contained in the pbuf that is passed to the function.
 *    This pbuf might be chained.
 *
 * @param[in] netif: the lwip network interface structure
 *
 * @param[in] p: the packet to send
 *
 * @return
 *    ERR_OK if successful
 ******************************************************************************/
static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
  err_t errval;
  struct pbuf *q;
  sl_wfx_send_frame_req_t* tx_buffer;
  sl_status_t result;
  uint8_t *buffer ;
  uint32_t frame_length;

  /* Compute packet frame length */
  frame_length = SL_WFX_ROUND_UP(p->tot_len, 2);

  sl_wfx_allocate_command_buffer((sl_wfx_generic_message_t**)(&tx_buffer),
                                 SL_WFX_SEND_FRAME_REQ_ID,
                                 SL_WFX_TX_FRAME_BUFFER,
                                 frame_length + sizeof(sl_wfx_send_frame_req_t));
  buffer = tx_buffer->body.packet_data;

  for(q = p; q != NULL; q = q->next)
  {
    /* Copy the bytes */
    memcpy(buffer, q->payload, q->len);
    buffer += q->len;
  }

  /* Transmit to the station or softap interface */
  if (netif->name[0] == SOFTAP_NETIF0)
  {
    result = sl_wfx_send_ethernet_frame(tx_buffer,
                                        frame_length,
                                        SL_WFX_SOFTAP_INTERFACE,
                                        0);
  }
  else
  {
    result = sl_wfx_send_ethernet_frame(tx_buffer,
                                        frame_length,
                                        SL_WFX_STA_INTERFACE,
                                        0);
  }

  sl_wfx_free_command_buffer((sl_wfx_generic_message_t*) tx_buffer,
                             SL_WFX_SEND_FRAME_REQ_ID,
                             SL_WFX_TX_FRAME_BUFFER);

  if(result == SL_STATUS_OK)
  {
    errval = ERR_OK;
  }
  else
  {
    errval = ERR_MEM;
  }

  return errval;
}

/******************************************************************************
 * @brief
 *    This function transfers the receive packets from the wf200 to lwip.
 *
 * @param[in] netif: the lwip network interface structure
 *
 * @param[in] rx_buffer: the ethernet frame received by the wf200
 *
 * @return    LwIP pbuf filled with received packet, or NULL on error
 *
 ******************************************************************************/
static struct pbuf *low_level_input(struct netif *netif, sl_wfx_received_ind_t* rx_buffer)
{
  struct pbuf *p, *q;
  uint8_t *buffer;

  /* Obtain the packet by removing the padding. */
  buffer = (uint8_t *)&(rx_buffer->body.frame[rx_buffer->body.frame_padding]);

  if (rx_buffer->body.frame_length > 0)
  {
    /* We allocate a pbuf chain of pbufs from the Lwip buffer pool */
    p = pbuf_alloc(PBUF_RAW, rx_buffer->body.frame_length, PBUF_POOL);
  }

  if (p != NULL)
  {
    for(q = p; q != NULL; q = q->next)
    {
      /* Copy remaining data in pbuf */
      memcpy(q->payload, buffer, q->len);
      buffer += q->len;
    }
  }

  return p;
}

/******************************************************************************
 * @brief
 *    This function implements the wf200 received frame callback.
 *
 * @param[in] rx_buffer: the ethernet frame received by the wf200
 *
 * @return    None
 *
 ******************************************************************************/
void sl_wfx_host_received_frame_callback(sl_wfx_received_ind_t* rx_buffer)
{
  struct pbuf *p;
  struct netif *netif;

  /* Check packet interface to send to AP or STA interface */
  if((rx_buffer->header.info & SL_WFX_MSG_INFO_INTERFACE_MASK) ==
     (SL_WFX_STA_INTERFACE << SL_WFX_MSG_INFO_INTERFACE_OFFSET))
  {
    /* Send to station interface */
    netif = WF200_Get_STA_NetIF();
  }
  else
  {
    /* Send to softAP interface */
    netif = WF200_Get_AP_NetIF();
  }

  if (netif != NULL)
  {
    p = low_level_input(netif, rx_buffer);
    if (p != NULL)
    {
      if (netif->input(p, netif) != ERR_OK)
      {
        pbuf_free(p);
      }
    }
  }
}


/******************************************************************************
 * @brief
 *    called at the beginning of the program to set up the network interface.
 *
 * @param[in] netif: the lwip network interface structure
 *
 * @return    ERR_OK if successful
 *
 ******************************************************************************/
err_t sta_ethernetif_init(struct netif *netif)
{
  LWIP_ASSERT("netif != NULL", (netif != NULL));

  /* Set the netif name to identify the interface */
  netif->name[0] = STATION_NETIF0;
  netif->name[1] = STATION_NETIF1;

  netif->output = etharp_output;
  netif->linkoutput = low_level_output;

  /* initialize the hardware */
  low_level_init(netif);

  WF200_Set_STA_NetIF(netif);

  return ERR_OK;
}

/******************************************************************************
 * @brief
 *    called at the beginning of the program to set up the network interface.
 *
 * @param[in] netif: the lwip network interface structure
 *
 * @return    ERR_OK if successful
 *
 ******************************************************************************/
err_t ap_ethernetif_init(struct netif *netif)
{
  LWIP_ASSERT("netif != NULL", (netif != NULL));

  /* Set the netif name to identify the interface */
  netif->name[0] = SOFTAP_NETIF0;
  netif->name[1] = SOFTAP_NETIF1;

  netif->output = etharp_output;
  netif->linkoutput = low_level_output;

  /* initialize the hardware */
  low_level_init(netif);
  WF200_Set_AP_NetIF(netif);

  return ERR_OK;
}