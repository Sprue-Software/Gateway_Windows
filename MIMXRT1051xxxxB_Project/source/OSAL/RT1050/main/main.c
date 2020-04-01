/*******************************************************************************
 * Includes
 ******************************************************************************/

#include "board.h"
#include "ksdk_mbedtls.h"

#include "fsl_debug_console.h"
#include "fsl_gpio.h"
#include "fsl_iomuxc.h"
#include "fsl_gpt.h"

#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_gpio.h"
#include "lwip/opt.h"
#include "lwip/tcpip.h"
#include "lwip/dhcp.h"
#include "lwip/prot/dhcp.h"
#include "lwip/dns.h"
#include "lwip/sockets.h" //nishi
#include "lwip/netdb.h"
#include "netif/ethernet.h"
//#include <ethernetif.h> //nishi
#include "sntp_client.h"
//#include "aws_iot_log.h" //nishi

//#include "timer_interface.h"
#include "HAL.h"
#include "OSAL_Api.h"
#include "LOG_Api.h"
#include "KEY_Api.h"
#include "SPI_Flash.h"
#include "EFS_FileSystem.h"
#include <STO_FileSystem.h>

#include "sntp_client.h"
//#include "virtual_nic_api.h"
#include "fsl_phy.h"
#include "lpm.h"
#include "power.h"
#include "LED_manager.h"
#define RECEIVER_PORT_NUM 6001 
void IntegertoString(char * string, int number);
static int retry_Count=0;
char * serial_malloc ;
uint8_t * Serial_str;
uint8_t * mac_str;
#define SERIAL_SIZE 24
#define STRING_SIZE 10
#define STRING_SERIAL 120
#define STRING_MAC 18
#define  BYTE_SEND 500
//const uint8_t serial_data[160];
//const uint8_t macdata[];
/*******************************************************************************
 * Definitions
 ******************************************************************************/
//nishi
//SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DSOFTWARE_MAJOR_VERSION=1")
//SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DSOFTWARE_MINOR_VERSION=0")
#define SOFTWARE_MAJOR_VERSION 1
#define SOFTWARE_MINOR_VERSION 0
#define  SOFTWARE_REVISION 0
#define PRIORITIE_OFFSET 4
//nishi
// Address of PHY interface
#define EXAMPLE_PHY_ADDRESS BOARD_ENET0_PHY_ADDRESS

// System clock name
#define EXAMPLE_CLOCK_NAME kCLOCK_CoreSysClk

#define ENSO_AGENT_STACK_SIZE           configMINIMAL_STACK_SIZE  //////// [RE:mem]

#if OTA_OVER_HTTPS_TEST
    #define OTA_HOST        "s3-eu-west-1.amazonaws.com"    //////// [RE:conf] nchatzi
    #define OTA_PORT        "443"   //////// [RE:fixme] This is used as a pointer in EnsoAgentSettings_t
    #define OTA_RESOURCE    "GET /sprue/ea3_wsgw.imx HTTP/1.1\r\nHost: s3-eu-west-1.amazonaws.com\r\nUser-Agent: curl/7.52.1\r\nAccept: */*\r\n\r\n"    //////// [RE:conf] nchatzi
#endif
 void Gateway_Status(void);
/**************************** TYPE DEFINITIONS ********************************/

//////// [RE:fixme] Why uint8_t pointers?
typedef struct
{
    uint8_t* name;
    ////////uint8_t* wlanSSID;	//////// [RE:platform]
    ////////uint8_t* wlanPassword;	////////[RE:platform]
    uint8_t* rootCA;
    uint8_t* certificate;
    uint8_t* privateKey;
#if OTA_OVER_HTTPS_TEST
    uint8_t* otaURL;
    uint8_t* otaPort;
    uint8_t* otaResourceName;
#endif
} EnsoAgentSettings_t;

uint8_t IP4_ADDR_NET[4];
uint8_t buff_IP4[4];
struct Status_Gate Gateway_Status_request;
//char gateway_status[800]; 
uint8_t* gateway_status;
/****************************** VARIABLES *************************************/
static bool wg2c = false;
static bool _bSystemReadyToRun = false;
static gpio_pin_config_t gpio_config_output = {kGPIO_DigitalOutput, 0, kGPIO_NoIntmode};

 struct netif fsl_netif0; //Change By nishi
 struct netif fsl_netif1; // Change By Nishi

static bool usb_link_status_up = false;
static bool eth_link_status_up = false;
static  bool eth_end_of_line = false;
static const EnsoAgentSettings_t ensoAgentSettings =
{
    .name = "EnsoMain",
    ////////.wlanSSID = WLAN_SSID,	//////// [RE:platform]
    ////////.wlanPassword = WLAN_PASSWORD,	//////// [RE:platform]
    .rootCA = "root",
    .certificate = "cert",
    .privateKey = "key",
#if OTA_OVER_HTTPS_TEST
    .otaURL = OTA_HOST,
    .otaPort = OTA_PORT,
    .otaResourceName = OTA_RESOURCE
#endif
};


const char _RootValue[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIE0zCCA7ugAwIBAgIQGNrRniZ96LtKIVjNzGs7SjANBgkqhkiG9w0BAQUFADCB\n"
    "yjELMAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQL\n"
    "ExZWZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTowOAYDVQQLEzEoYykgMjAwNiBWZXJp\n"
    "U2lnbiwgSW5jLiAtIEZvciBhdXRob3JpemVkIHVzZSBvbmx5MUUwQwYDVQQDEzxW\n"
    "ZXJpU2lnbiBDbGFzcyAzIFB1YmxpYyBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0\n"
    "aG9yaXR5IC0gRzUwHhcNMDYxMTA4MDAwMDAwWhcNMzYwNzE2MjM1OTU5WjCByjEL\n"
    "MAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQLExZW\n"
    "ZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTowOAYDVQQLEzEoYykgMjAwNiBWZXJpU2ln\n"
    "biwgSW5jLiAtIEZvciBhdXRob3JpemVkIHVzZSBvbmx5MUUwQwYDVQQDEzxWZXJp\n"
    "U2lnbiBDbGFzcyAzIFB1YmxpYyBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0aG9y\n"
    "aXR5IC0gRzUwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCvJAgIKXo1\n"
    "nmAMqudLO07cfLw8RRy7K+D+KQL5VwijZIUVJ/XxrcgxiV0i6CqqpkKzj/i5Vbex\n"
    "t0uz/o9+B1fs70PbZmIVYc9gDaTY3vjgw2IIPVQT60nKWVSFJuUrjxuf6/WhkcIz\n"
    "SdhDY2pSS9KP6HBRTdGJaXvHcPaz3BJ023tdS1bTlr8Vd6Gw9KIl8q8ckmcY5fQG\n"
    "BO+QueQA5N06tRn/Arr0PO7gi+s3i+z016zy9vA9r911kTMZHRxAy3QkGSGT2RT+\n"
    "rCpSx4/VBEnkjWNHiDxpg8v+R70rfk/Fla4OndTRQ8Bnc+MUCH7lP59zuDMKz10/\n"
    "NIeWiu5T6CUVAgMBAAGjgbIwga8wDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8E\n"
    "BAMCAQYwbQYIKwYBBQUHAQwEYTBfoV2gWzBZMFcwVRYJaW1hZ2UvZ2lmMCEwHzAH\n"
    "BgUrDgMCGgQUj+XTGoasjY5rw8+AatRIGCx7GS4wJRYjaHR0cDovL2xvZ28udmVy\n"
    "aXNpZ24uY29tL3ZzbG9nby5naWYwHQYDVR0OBBYEFH/TZafC3ey78DAJ80M5+gKv\n"
    "MzEzMA0GCSqGSIb3DQEBBQUAA4IBAQCTJEowX2LP2BqYLz3q3JktvXf2pXkiOOzE\n"
    "p6B4Eq1iDkVwZMXnl2YtmAl+X6/WzChl8gGqCBpH3vn5fJJaCGkgDdk+bW48DW7Y\n"
    "5gaRQBi5+MHt39tBquCWIMnNZBU4gcmU7qKEKQsTb47bDN0lAtukixlE0kF6BWlK\n"
    "WE9gyn6CagsCqiUXObXbf+eEZSqVir2G3l6BFoMtEMze/aiCKm0oHw0LxOXnGiYZ\n"
    "4fQRbxC1lfznQgUy286dUV4otp6F01vvpX1FQHKOtw5rDgb7MzVIcbidJ4vEZV8N\n"
    "hnacRHr2lVz2XTIIM6RUthg/aFzyQkqFOFSDX9HoLPKsEdao7WNq\n"
    "-----END CERTIFICATE-----\n";

const char _CertValue[] =
"-----BEGIN CERTIFICATE-----\n"
"MIIDljCCAn4CCQC9WRbEMM4iFjANBgkqhkiG9w0BAQsFADCBiDELMAkGA1UEBhMC\n"
"VUsxEjAQBgNVBAgMCU5vcnRoYW50czEUMBIGA1UEBwwLTm9ydGhhbXB0b24xHDAa\n"
"BgNVBAoME0ludGFtYWMgU3lzdGVtcyBMdGQxFjAUBgNVBAsMDUVtYmVkZGVkIFRl\n"
"YW0xGTAXBgNVBAMMEEVtYmVkZGVkIFRlc3QgQ0EwHhcNMTkwMTIxMTMxNzM5WhcN\n"
"MjAwMTIxMTMxNzM5WjCBkDELMAkGA1UEBhMCR0IxEjAQBgNVBAgMCU5vcnRoYW50\n"
"czEUMBIGA1UEBwwLTm9ydGhhbXB0b24xGDAWBgNVBAoMD0ludGFtYWMgU3lzdGVt\n"
"czEZMBcGA1UECwwQMDAwMGYwMDM4Y2U3YWM5NzEiMCAGA1UEAwwZRW1iZWRkZWQt\n"
"MDAwMGYwMDM4Y2U3YWM5NzCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB\n"
"AJeYzOVUUG/UdZWaWtE2GkboOo5pkdXMHQeXwPtXfG8pUw/p/Pm/6g9eJCEGMvEV\n"
"21WYnqFMA+fJ7BmgQvAlcFN3KQ6gil4XsIHWo5EyRMi447yOjUDalJowm9D4sR6n\n"
"JvJheiLrrjc+8c3fksjiUatPyhFrdaHj/lLmuYUYW5LhcidJz3AgKYvAUr37puyP\n"
"iafxrsydUxT+8iZMUDKaRD2x1zz4KGd68kxZcqcAL0Wu0AakU8hbmkAR/COKG6lh\n"
"xQz1yBXHl8v5XFO1iHjgz8LKKKEr21nFLkJJp8J0xWFy5ds6xjYfzhKDuEJRZv3L\n"
"GQKWvbCwBRK/RSQxDznWA0cCAwEAATANBgkqhkiG9w0BAQsFAAOCAQEAYVzJs5Qy\n"
"RKFSx9GuK0cbLC4QG494K4JZDB9xfcD/yxGADI9+hIpTZN+A1YhKGdnrAjTZqYIq\n"
"vSIQbfrizlvC+X4+Y0GflRacgW+gsxCpa4g2/L5x/rj0/7KWDyf0WB9Y5HBsGp+1\n"
"uMDupBOvrRarZkqK2IlBxKYIYP7HPwS9Mr8uWDnXOxZ9ed5b5e6XLhw8dgjh+SVl\n"
"s/UOLll2Y7ZSHQ0Rijxg2Einme19fm6L+tmO4xHDeowTQtvGLgoQB857NV+H3jgc\n"
"8Ri7aFo3frllpWnqn5GyQs0FelBwkgNR5IosYlsUYROWXTYNUISB26BKWrR9QrHD\n"
"A9HK1Fp1Sm7qyQ==\n"
"-----END CERTIFICATE-----\n"
"-----BEGIN CERTIFICATE-----\n"
"MIID5TCCAs2gAwIBAgIJAN03f63/rbUzMA0GCSqGSIb3DQEBCwUAMIGIMQswCQYD\n"
"VQQGEwJVSzESMBAGA1UECAwJTm9ydGhhbnRzMRQwEgYDVQQHDAtOb3J0aGFtcHRv\n"
"bjEcMBoGA1UECgwTSW50YW1hYyBTeXN0ZW1zIEx0ZDEWMBQGA1UECwwNRW1iZWRk\n"
"ZWQgVGVhbTEZMBcGA1UEAwwQRW1iZWRkZWQgVGVzdCBDQTAeFw0xNzA5MDgwOTIx\n"
"MjRaFw0xODA5MDgwOTIxMjRaMIGIMQswCQYDVQQGEwJVSzESMBAGA1UECAwJTm9y\n"
"dGhhbnRzMRQwEgYDVQQHDAtOb3J0aGFtcHRvbjEcMBoGA1UECgwTSW50YW1hYyBT\n"
"eXN0ZW1zIEx0ZDEWMBQGA1UECwwNRW1iZWRkZWQgVGVhbTEZMBcGA1UEAwwQRW1i\n"
"ZWRkZWQgVGVzdCBDQTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALju\n"
"vvKz/4P9olDVkqteWIgbldMyNkacnCVRgg0nbDResA4hT2wOysweUsQA8QqlrHc9\n"
"cyzp3eDWWYlo37qMxeBQqBWTG3fqkw2Q666BHIczaVBETCCkQbDnQUD9Sd5jjYtS\n"
"If0M2Ef86UNgey1hbBQKP7PoG4xrhYwz42SOIX0ozDNU+ZOLK3KcMPX3X3EbVBUZ\n"
"UxwqWJmexYFJs3a1sJdrc9j1QG8sIERFyeHLr+CaUx6pkzM9JURf8zTwlptqxYCV\n"
"GoXilUFAn+YieQWtN4bCZHg56dizAJcUnzGIHHtUSQzshCYXJUvKy58FBQ+tdVMf\n"
"tjqJVj5gVYLnNpM/Vc0CAwEAAaNQME4wHQYDVR0OBBYEFIaGZTMQWdkiUurkAumQ\n"
"mFKuO5QxMB8GA1UdIwQYMBaAFIaGZTMQWdkiUurkAumQmFKuO5QxMAwGA1UdEwQF\n"
"MAMBAf8wDQYJKoZIhvcNAQELBQADggEBAJBVlUI47psRqjZOYD2K1MbhgFoUISwe\n"
"hG23xKH93EJ3/RBjL844tbR6lfpwVVrlLhAIa4vDmJDSazerKH07BT5Al7MCepN/\n"
"bo2+D1VA2T+IssoQp57WYylz9gijsAymDaAoKKmFPfyrtEETC23CZsyHqoi68SEM\n"
"AYnD8eefXu5bWyFe7s3KeMy+NYFHXPfVvtY6spyKjNSC5P0yqytuq8NhytGNMNj6\n"
"m2xH88ziK3v8w0qC4Dz1gEQHxZVe+fM7NquPhpzyFkKEWWM9PRtOwiG625R/Pi3T\n"
"VO8pwvES36/zZy0+I4jR9TnMB5buvv2zOoPtWop1P4XZReGeTodXLD4=\n"
"-----END CERTIFICATE-----\n";

const char _PKeyValue[] =
    "-----BEGIN RSA PRIVATE KEY-----\n"
"MIIEpAIBAAKCAQEAl5jM5VRQb9R1lZpa0TYaRug6jmmR1cwdB5fA+1d8bylTD+n8\n"
"+b/qD14kIQYy8RXbVZieoUwD58nsGaBC8CVwU3cpDqCKXhewgdajkTJEyLjjvI6N\n"
"QNqUmjCb0PixHqcm8mF6IuuuNz7xzd+SyOJRq0/KEWt1oeP+Uua5hRhbkuFyJ0nP\n"
"cCApi8BSvfum7I+Jp/GuzJ1TFP7yJkxQMppEPbHXPPgoZ3ryTFlypwAvRa7QBqRT\n"
"yFuaQBH8I4obqWHFDPXIFceXy/lcU7WIeODPwsoooSvbWcUuQkmnwnTFYXLl2zrG\n"
"Nh/OEoO4QlFm/csZApa9sLAFEr9FJDEPOdYDRwIDAQABAoIBAEYfhS/Th32jLlzs\n"
"UHQT7aW9CFEV3kKiLw9zD+5zcnjNCcIDv2QbdP500ouAHZJNRO7cMQx1aB0Q1yin\n"
"bC8/ciz5osFEW5zYomn8yh6AvTaH57gxzH8iXLjSIVFRqESAl1Bo7KE4mZauhBVy\n"
"BKjtn096EgNqzLf9CYh5d3lsYfeXoQBPT934omB3XhfluaCNiVbJElNesQlYC/BC\n"
"GpiR3AMFkE0M9HJUwmp/JBu5jNsGJD7cXQYBd7MDSUGmZq68je1Szdz/2GXM4G+6\n"
"IzVwZbInwyZRMAEeVxVhtAgM47MAnt4B5mzgkAkvT7u9eH6llpWO/umyL3mj6wse\n"
"wZsddSkCgYEAxbwPbjhSAGOQ9WjD81xE09FdImgNDOuPckrYkWHR/gOFsPW/yrPe\n"
"Rg3NajsyiruaPcm7fDl9ochBlBpysPrPx9dCoqvULqQ5/+CCS7aHK+UA/sQi4rIS\n"
"NePcD9kETVU8Rr43QqJ11vEIaBuCYYe3d2ckffrDrdADkOswb8qwerMCgYEAxERh\n"
"RBTdkcBxyRwiGHVIz6jF32zsc900cYp7/RSwwmyF7AniaXrAmTu0c9AjIbFhwNGU\n"
"xsAE8fhTOpY9DUHm/vV2p8ruixSZ4+FjxPVTUoXOL37Of2u/XYEnhB6iCGStTc+t\n"
"8MbSYdtJZbou6JRZoIzmURnOTT8suLtWzYsJ7x0CgYEAjWLsMaap08dd5mxz+HZY\n"
"bJD+pFR8SGnDFzk3Y7TrX1MLbD48VmeIntTNtZEAkbyVDGtL2QaOs3iqk4jZy3x7\n"
"x+w+pGxy+qrJIhJZeGPagWNs874xJ6GmbcwxFU/ayKUSxY7LmqTp17hfh3lsH+rY\n"
"H9Orfz0oYAcmFaBl8PmgeQsCgYEArAcPhiAP0Sfv1AmPrQZoCnPw9BPB8RBrXjW2\n"
"1a4j/FYo0CodxLxuQiE92uENWgPjHaHVmJtH+lrhgJGc85jXApReK1ZI3ajx9fZh\n"
"f4pUPRnBDopELxVfB3MkEr7S4S51ZKVq3Yc3ccRGQh797KfTI1E8sss+syBzb3vs\n"
"6n4699UCgYBw8yNxhGWuLJUYzjtYoPc6cwR6BkLmF2XHyh23i64i9iE+cSb7m3Mx\n"
"v5flHE6y1NRJwZBZxOqdslm3l/bhVwR+VN4Am5yytbuqiL9DgJM8NgYfKYATlfcs\n"
"AdG086mcJRYubofivnBVsUcnEDANb8OvRwJsyf+xKwt2KwZOf2hYqA==\n"
"-----END RSA PRIVATE KEY-----\n";

extern int https_update_ota(char* host, char* port, char* resource);  //////// [nchatzi] I'd be nicer if the OTA functions were abstracted into the HAL/OSAL API...
void initLoggingProcess(void);



/*******************************************************************************
 * Code
 ******************************************************************************/

bool getSystemRunStatus(void)
{
    return _bSystemReadyToRun;
}

bool gatewayTypeWG2C(void)
{
    return wg2c;
}

void InitStatsTimer (void)
{
    gpt_config_t gptConfig;

    /*Clock setting for GPT*/
    CLOCK_SetMux(kCLOCK_PerclkMux, 0);
    CLOCK_SetDiv(kCLOCK_PerclkDiv, 0);

    GPT_GetDefaultConfig(&gptConfig);

    /* Initialize GPT module */
    GPT_Init(GPT1, &gptConfig);

    GPT_SetClockDivider(GPT1, 1500);

    GPT_StartTimer(GPT1);
}

uint32_t StatsTimerValue(void)
{
	return GPT_GetCurrentTimerCount(GPT1);
}

void vApplicationStackOverflowHook(xTaskHandle *pxTask, signed char *pcTaskName )
{
	LOG_Error("Task [%s] has a corrupt stack",pcTaskName); 
	for( ;; );
}

/**
 * \brief Dumps Initial Certificates to KeyStore. Used for test purposes.
 *
 * \param none
 * \return none
 */
static void dump_initial_certificates_if_not_exist(void)
{
    uint8_t *flashData;
    int32_t retVal;
    void* keys = KEY_Open();

    if (keys == NULL)
    {
        LOG_Error("Failed to open keystore!");
        return;
    }

    if (KEY_Get(keys, ensoAgentSettings.rootCA) == NULL)
    {
        LOG_Warning("RootCA not found in FS, checking flash serialisation");
        flashData = HAL_GetRootCA();
#if USE_SERIALISATION_IN_FLASH
        if (flashData[0] != 0xFF)
        {
            LOG_Warning("RootCA found in flash serialisation, saving new...");
            retVal = KEY_Add(keys, ensoAgentSettings.rootCA, flashData);
           // dumpHex(flashData,sizeof(flashData));
        }
        else
#endif
        {
            LOG_Warning("RootCA not found in flash, saving hard-coded value");
            retVal = KEY_Add(keys, ensoAgentSettings.rootCA, _RootValue);
        }

        if (retVal != 0)
        {
            LOG_Error("Failed to write RootCA!");
            return;
        }
    }
    else
    {
        LOG_Info("Loaded existing RootCA from FS");
    }

    if (KEY_Get(keys, ensoAgentSettings.certificate) == NULL)
    {
        LOG_Warning("Certificate not found in FS, checking flash serialisation");
        flashData = HAL_GetCertificate();
#if USE_SERIALISATION_IN_FLASH
        if (flashData[0] != 0xFF)
        {
            LOG_Warning("Certificate found in flash serialisation, saving new...");
            retVal = KEY_Add(keys, ensoAgentSettings.certificate, flashData);
        }
        else
#endif
        {
            LOG_Warning("Certificate not found in flash, saving hard-coded value");
            retVal = KEY_Add(keys, ensoAgentSettings.certificate, _CertValue);
        }

        if (retVal != 0)
        {
            LOG_Error("Failed to write Certificate!");
            return;
        }
    }
    else
    {
        LOG_Info("Loaded existing Certificate from FS");
    }

    if (KEY_Get(keys, ensoAgentSettings.privateKey) == NULL)
    {
        LOG_Warning("Private key not found in FS, checking flash serialisation");
        flashData = HAL_GetKey();
#if USE_SERIALISATION_IN_FLASH
        if (flashData[0] != 0xFF)
        {
            LOG_Warning("Private key found in flash serialisation, saving new...");
            retVal = KEY_Add(keys, ensoAgentSettings.privateKey, flashData);
        }
        else
#endif
        {
            LOG_Warning("Private key not found in flash, saving hard-coded value");
            retVal = KEY_Add(keys, ensoAgentSettings.privateKey, _PKeyValue);
        }

        if (retVal != 0)
        {
            LOG_Error("Failed to write Key!");
            return;
        }
    }
    else
    {
        LOG_Info("Loaded existing Key from FS");
    }

    LOG_Info("Initial certificates are saved!");
}



static void BOARD_InitModuleClock(void)
{
    const clock_enet_pll_config_t config =
    {
       true,
       false,
       false,
       1,
       1
    };
   CLOCK_InitEnetPll(&config);
}



static void delay(int n)
{
    volatile uint32_t i = 0;
    for (i = 0; i < n; ++i)
    {
        __asm("NOP"); /* delay */
    }
}



static void setup_board()
{
    // Initialise board
    gpio_pin_config_t gpio_config = {kGPIO_DigitalOutput, 0, kGPIO_NoIntmode};
    BOARD_ConfigMPU();
    BOARD_InitPins();
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();
    BOARD_InitModuleClock();

#if LOW_POWER_MODE
    if (true != LPM_Init(LPM_PowerModeSysIdle))
    {
        LOG_Error("LPM Init Failed!");
    }

    LPM_DisablePLLs(LPM_PowerModeSysIdle);

    SystemCoreClockUpdate();
#endif

    // Data cache must be temporarily disabled to be able to use sdram
    // N.B. Do this later? (after ifup, before DHCP)
    SCB_DisableDCache();
    // SCB_DisableICache();

    // Setup RAM regions
    const HeapRegion_t xHeapRegions[] =
    {
        { ( uint8_t * ) 0x20000000UL, 0x20000 },    // DTC
        { ( uint8_t * ) 0x20200000UL, 0x40000 },    // OC
        { ( uint8_t * ) 0x80200000UL, 0xC00000 },   // SDRAM (CADB)
        { NULL, 0 }
    };
    vPortDefineHeapRegions( xHeapRegions );

    // Setup HW
    // Enable Ethernet
    GPIO_PinInit(GPIO1, 14, &gpio_config_output);
    enable_wired_ethernet(true);
    CRYPTO_InitHardware();
}



/*
 * \brief Sets system time. Gets time from Cloud using sntp.
 *
 * \param  params Not used
 *
 * \return nonde
 */
void setup_system_time(void)
{
    LOG_Info("Getting SNTP time...");

    extern void OSAL_SetTime(time_t newSystemTimeInSec);

    long sec, usec, tick;

    OSAL_sleep_ms(15000);//ABR added

    // Initialise SNTP to get time
    sntp_init();
    sec = 0;
    usec = 0;
    tick = 0;

    // Wait until get a valid time
    //////// [RE:fixme] Re-init after a while, in case we miss the SNTP packet - although an early wdog would take care of this
    //Its Blocking call for SNTP Time.
    while ((sec == 0) && (retry_Count<15))
    {
        OSAL_sleep_ms(1000);//ABR changed 100

        sntp_get_lasttime(&sec, &usec, &tick);
        //This fix  only works at Boot-Up time:

        //Need better option: require a seprate thread to monitor Network and change interface

        //Quick Fix : try for few miliseconds on USB interface, if unable to connect SNTP
        //and Ethernet is connected then switch to Ethernet and try SNTP
        //
        // Changing Network interface on run time is not supported if both interface is connected.
         //(USB as Ethernet (RNDIS), Ethernet )

         if (retry_Count>10)
        {
            // Unfortunatly we have to call netif_set_default()             // to switch network.
            // Need to implement Network Monitoring thread( 2 weeks Work, on HOLD): No Need of retry count

            if(eth_link_status_up)
            {
                if (eth_end_of_line==false)
                {
                LOG_Info("Change default Port to Ethernet");
                netif_set_default(&fsl_netif0);
                if (usb_link_status_up)
                {
                   netif_set_link_down(&fsl_netif1);
                   netif_remove(&fsl_netif1);
                  
                 }
                  //retry_Count=0;//ABR changed
                 eth_end_of_line=true;
                }

            }

        }
         LOG_Info("No SNTP time...");
         retry_Count++;
           
    }

     // Set System Time in seconds.
     // Decrease seconds by device elapsed time from startup
    OSAL_SetTime(sec - tick * portTICK_PERIOD_MS / 1000);
}



 void dump_net_config(struct netif *intf)
{
   
   struct dhcp *dhcp0,*dhcp1;
	LOG_Info("IPv4 Address	   : %u.%u.%u.%u", ((u8_t *)&intf->ip_addr.addr)[0],
		   ((u8_t *)&intf->ip_addr.addr)[1], ((u8_t *)&intf->ip_addr.addr)[2],
		   ((u8_t *)&intf->ip_addr.addr)[3]);

	LOG_Info("IPv4 Netmask	   : %u.%u.%u.%u", ((u8_t *)&intf->netmask.addr)[0],
		   ((u8_t *)&intf->netmask.addr)[1], ((u8_t *)&intf->netmask.addr)[2],
		   ((u8_t *)&intf->netmask.addr)[3]);

	LOG_Info("IPv4 Gateway	   : %u.%u.%u.%u", ((u8_t *)&intf->gw.addr)[0],
		   ((u8_t *)&intf->gw.addr)[1], ((u8_t *)&intf->gw.addr)[2],
		   ((u8_t *)&intf->gw.addr)[3]);

    const ip_addr_t *dns_ip = dns_getserver(0);

	LOG_Info("IPv4 DNS	   : %u.%u.%u.%u", ((u8_t *)&dns_ip->addr)[0],
		   ((u8_t *)&dns_ip->addr)[1], ((u8_t *)&dns_ip->addr)[2],
	  	   ((u8_t *)&dns_ip->addr)[3]);
   
    dhcp0 = (struct dhcp *)netif_get_client_data(&fsl_netif0, LWIP_NETIF_CLIENT_DATA_INDEX_DHCP);
  //This will check  DHCP network , and reinit network and dhcp start if DHCP Lost
   // Issue: DHCP Lost after 12 Hours and works fine with routers
   //Added reset in error.c in middleware emdtls
  LOG_Error("Network Check %x",dhcp0->state);
  if(dhcp0->state != DHCP_STATE_BOUND)
 {
    netif_set_link_up(&fsl_netif0);
    LOG_Error("Re-init Ethernet ");
    eth_link_status_up = true;
     if (ERR_OK != dhcp_start(&fsl_netif0)) 
    {
        LOG_Error("dhcp_start(&fsl_netif0) Fail");
    }
 }

  
  
    
}

static void usb_link_status_callback(bool status, void *callback_data)
{
	struct netif *netif = (struct netif *)callback_data;

	if (NULL == netif) return;

	if (status)
	{
		LOG_Info("%c%c up",netif->name[0],netif->name[1]);
		netif_set_link_up(netif);		
		netif_set_default(&fsl_netif1);
		usb_link_status_up = true;
        Gateway_Status_request.Tablet_Connected=1;
	}
	else
	{
		LOG_Info("%c%c down",netif->name[0],netif->name[1]);
		netif_set_link_down(netif);
		netif_set_default(&fsl_netif0);
		usb_link_status_up = false;
        Gateway_Status_request.Tablet_Connected=0;
	}
}

static void link_status_callback(bool status, void *callback_data)
{
	struct netif *netif = (struct netif *)callback_data;

	if (NULL == netif) return;

	if (status)
	{
		LOG_Info("%c%c up",netif->name[0],netif->name[1]);
		netif_set_link_up(netif);
		eth_link_status_up = true;
	}
	else
	{
		LOG_Info("%c%c down",netif->name[0],netif->name[1]);
		netif_set_link_down(netif);
		eth_link_status_up = false;
	}
}


/*!
 * @brief Report network status as combination of either usb and/or ethernet link status up
 */
bool network_link_status(void)
{
    if (_bSystemReadyToRun && (usb_link_status_up || eth_link_status_up))
    {
        return true;
    }
    return false;
}

/*!
 * @brief Report Gateway status to Host using Wired Network Port 
  */
  void EndofLineTest(void *arg)
  {
    TickType_t xLastWakeTime;
    struct netif *intf;
    uint8_t *debugMessage;
    int socket_fd,socket_fd2;
    struct sockaddr_in sa,ra;
    int recv_data; char rcv_buffer[80]; 
    int clientlen; /* byte size of client's address */
    UBaseType_t uxHighWaterMark;
    //Gateway_Status_request.Battery_connected=0;
    Gateway_Status_request.Battery_voltage[0]=0;
    Gateway_Status_request.Battery_voltage[1]=0;
    Gateway_Status_request.Charging_Fault=0;
    Gateway_Status_request.Switch_Position=0; //set default to 0
    Gateway_Status_request.Tablet_Connected=0;
    Gateway_Status_request.Newtork_Status=0;
    Gateway_Status_request.ZWave_module=0;
    Gateway_Status_request.Software_version[0]=SOFTWARE_MAJOR_VERSION;
    Gateway_Status_request.Software_version[1]=SOFTWARE_MINOR_VERSION;
    Gateway_Status_request.Software_version[2]=SOFTWARE_REVISION;
    //Gateway_Status_request.MAC_address="E"
    //
    gateway_status=(uint8_t*)malloc(600); // Allocate 600 Bytes on heap .
    //Observed  stack curruption 


    //int32_t uxHighWaterMark;
    //Gateway_Status_request.MAC_address=HAL_GetMACAddressString();
   // LOG_Info(" IPv4 Address : %u.%u.%u.%u", IP4_ADDR_NET[0],IP4_ADDR_NET[1], IP4_ADDR_NET[2],IP4_ADDR_NET[3]);

     /* Creates an UDP socket (SOCK_DGRAM) with Internet Protocol Family (PF_INET).
    * Protocol family and Address family related. For example PF_INET Protocol Family and AF_INET family are coupled.
    */
    clientlen=sizeof(sa);
    vTaskDelay(2000); //delay for ethernet Port init
    // Creating Two socket For send and receive 
    socket_fd = socket(PF_INET, SOCK_DGRAM, 0); 
    socket_fd2 = socket(PF_INET, SOCK_DGRAM, 0);

    if ( socket_fd < 0 )
    {

        LOG_Info("socket call failed");
    
    }
    if ( socket_fd2 < 0 )
    {

        LOG_Info("socket 2 call failed");

    }

    memset(&ra, 0, sizeof(struct sockaddr_in));
    ra.sin_family = AF_INET;
    ra.sin_addr.s_addr = PP_HTONS(INADDR_ANY);
    ra.sin_port = htons(RECEIVER_PORT_NUM);

    memset(&sa, 0, sizeof(struct sockaddr_in));

    /* Bind the UDP socket to the port RECEIVER_PORT_NUM and to the current
    * machines IP address (Its defined by RECEIVER_PORT_NUM).
    * Once bind is successful for UDP sockets application can operate
    * on the socket descriptor for sending or receiving data.
    */
    if (bind(socket_fd, (struct sockaddr *)&ra, sizeof(struct sockaddr_in)) == -1)
    {

        //OSAL_printf("Bind to Port Number %d ,IP address %s failed\n",RECEIVER_PORT_NUM,RECEIVER_IP_ADDR);
        close(socket_fd);
   
    }
/* RECEIVER_PORT_NUM is port on which Server waits for data to
* come in. It copies the received data into receive buffer and
* prints the received data as string. If no data is available it 
* blocks.recv*/


    while(1)
    {



        /* Inspect our own high water mark on entering the task. */
        //uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
        LOG_Info(" FireAngel EOL Loop \n");
       // OSAL_sleep_ms(100);
      //  uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
       // LOG_Info(" FireAngel EOL Loop Water Mark %x \n",uxHighWaterMark);
       Gateway_Status();
      // LOG_Info(" FireAngel EOL Loop Water After Mark %x \n",uxHighWaterMark);
        //uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );

        //Blocking Call recvfrom: Not Closed the Socket Connection          // This will be used in future for sending Device Status periodicaly to tools /Cloud
        recv_data =recvfrom(socket_fd, rcv_buffer, sizeof(rcv_buffer), 0,(struct sockaddr *) &sa, &clientlen);
        LOG_Info("data %s",gateway_status );
        if (strncmp(rcv_buffer,"Gateway_Status",14))
        {
            sendto(socket_fd2, "Please verify command ",22,0,(struct sockaddr*)&sa,sizeof(sa));
        }
        else
        {
        if(recv_data > 0)
        {

            LOG_Info("Data Recived");
            sendto(socket_fd2, gateway_status,BYTE_SEND,0,(struct sockaddr*)&sa,sizeof(sa));
        }
   }
    } //endof while
} 

/*!
 * @brief Read Status of diffrent gateway periphral 
 */
    void Gateway_Status(void)
   {
      

         if (gateway_status!=NULL)
         {
            memset(gateway_status,0xff,BYTE_SEND); 
        snprintf(gateway_status,BYTE_SEND, "Board_Type: %s,Software version: %d.%d.%d,Serial number: %s, MAC ID: %s,Is Battery Connected: %d,Battery Voltage: %d.%02d v,Battery Charging Status: %d,Battery Switch Position: %d,Is Tablet Connected: %d,Newtork Status: %d,Is Z-Wave module Connected: %d##"
          ,Gateway_Status_request.Board_Type, 
          Gateway_Status_request.Software_version[0],Gateway_Status_request.Software_version[1],
          Gateway_Status_request.Software_version[2],Serial_str,
          mac_str,
          Gateway_Status_request.Battery_connected,
          Gateway_Status_request.Battery_voltage[0],
          Gateway_Status_request.Battery_voltage[1],
          Gateway_Status_request.Charging_Fault,
          Gateway_Status_request.Switch_Position,
          Gateway_Status_request.Tablet_Connected,
          Gateway_Status_request.Newtork_Status,
          Gateway_Status_request.ZWave_module); 
    }

    } 

/*!
 * @brief Read Status of diffrent gateway periphral in Loop 
 * In future, It will be use to report device status to cloud 
 */
 #if 0
void EndofLineTest_Ethernet_Thread(void *arg)
  {
   
  
     for( ;; )
     {
         // Wait for the next cycle.
         vTaskDelayUntil( &xLastWakeTime, xFrequency );


  
         Gateway_Status();
         #if 1
        LOG_Info("#######################");
        LOG_Info("Software version: %d.%d.%d", Gateway_Status_request.Software_version[0], Gateway_Status_request.Software_version[1], Gateway_Status_request.Software_version[2]);
        LOG_Info("Reading Serial_Number... %s",Gateway_Status_request.Serial_Number );
        LOG_Info("Reading MAC_address... %s",Gateway_Status_request.MAC_address );
        LOG_Info("Reading Battery_connected... %d",Gateway_Status_request.Battery_connected );
        LOG_Info("Reading Batt Voltage... %d.%03d",Gateway_Status_request.Battery_voltage[0],Gateway_Status_request.Battery_voltage[1] );
        LOG_Info("Reading Charging_Fault... %d",Gateway_Status_request.Charging_Fault );
        LOG_Info("Reading Switch_Position... %d",Gateway_Status_request.Switch_Position );
        LOG_Info("Reading Tablet_Connected... %d",Gateway_Status_request.Tablet_Connected );
        LOG_Info("Reading Newtork_Status... %d",Gateway_Status_request.Newtork_Status );
        if(network_link_status())
        {
            intf=&fsl_netif1;
         
            LOG_Info(" Ip address... %s",IP4_ADDR_NET );
        }
        LOG_Info("#######################");
        #endif 
         // Perform action here.
     }

  }
  #endif 
   
    /*!
 * @brief The main function containing client thread.
 */
static void setup_lan_and_time(void *arg)
{
    const uint8_t mac[NETIF_MAX_HWADDR_LEN];

    uint8_t serial[SERIAL_SIZE];
    char string[STRING_SIZE];
    Serial_str= (char *) malloc(STRING_SERIAL);
    mac_str= (char*)malloc(STRING_MAC);
    HAL_GetMACAddressBytes(mac);

    LOG_Info("Reading MAC address... [%02x %02x %02x %02x %02x %02x]",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
  

    LOG_Info("Setting up LAN...");
    HAL_serialBytes(serial);
    if (Serial_str!=NULL)
    {
        snprintf(Serial_str,STRING_SERIAL,"0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x", 
        serial[0],serial[1],serial[2],serial[3],serial[4],serial[5],serial[6],serial[7],
        serial[8],serial[9],serial[10],serial[11],serial[12],serial[13],serial[14],serial[15],
        serial[16],serial[17],serial[18],serial[19],serial[20],serial[21],serial[22],serial[23]);
    }
      if (mac_str!=NULL)
    {
        snprintf(mac_str,STRING_MAC,"%02x %02x %02x %02x %02x %02x",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    }
    ip4_addr_t fsl_netif0_ipaddr, fsl_netif0_netmask, fsl_netif0_gw;
    ip4_addr_t fsl_netif1_ipaddr, fsl_netif1_netmask, fsl_netif1_gw;
#if 0
    ethernetif_config_t fsl_enet_config0 = {
        .phyAddress = EXAMPLE_PHY_ADDRESS,
        .clockName = EXAMPLE_CLOCK_NAME
    };

    memcpy(fsl_enet_config0.macAddress, &mac, NETIF_MAX_HWADDR_LEN);
#endif
    LWIP_UNUSED_ARG(arg);

    IP4_ADDR(&fsl_netif0_ipaddr, 0, 0, 0, 0);
    IP4_ADDR(&fsl_netif0_netmask, 0, 0, 0, 0);
    IP4_ADDR(&fsl_netif0_gw, 0, 0, 0, 0);

    IP4_ADDR(&fsl_netif1_ipaddr, 0, 0, 0, 0);
    IP4_ADDR(&fsl_netif1_netmask, 0, 0, 0, 0);
    IP4_ADDR(&fsl_netif1_gw, 0, 0, 0, 0);


    tcpip_init(NULL, NULL);
     //nishi
    //netif_add(&fsl_netif0, &fsl_netif0_ipaddr, &fsl_netif0_netmask, &fsl_netif0_gw,&fsl_enet_config0, ethernetif0_init, tcpip_input);
    PHY_RegisterLinkStatusCallback(link_status_callback, (void *)&fsl_netif0);
    netif_set_up(&fsl_netif0);
    netif_set_default(&fsl_netif0);
     //nishi
    //netif_add(&fsl_netif1, &fsl_netif1_ipaddr, &fsl_netif1_netmask, &fsl_netif1_gw, NULL, vnic_init, tcpip_input);
    vnic_RegisterLinkStatusCallback(usb_link_status_callback, (void *)&fsl_netif1);     
    netif_set_up(&fsl_netif1);

    LOG_Info("Getting IP address from DHCP...");
    /********/SCB_DisableDCache();
    if (ERR_OK != dhcp_start(&fsl_netif0)) 
    {
        LOG_Error("dhcp_start(&fsl_netif0) failde");
    }
    dhcp_start(&fsl_netif1);

    struct dhcp *dhcp0,*dhcp1;
    dhcp0 = (struct dhcp *)netif_get_client_data(&fsl_netif0, LWIP_NETIF_CLIENT_DATA_INDEX_DHCP);
    dhcp1 = (struct dhcp *)netif_get_client_data(&fsl_netif1, LWIP_NETIF_CLIENT_DATA_INDEX_DHCP);

    while ((dhcp0->state != DHCP_STATE_BOUND) && (dhcp1->state != DHCP_STATE_BOUND))
    {
        vTaskDelay(1000);
    }

    if (dhcp0->state == DHCP_STATE_BOUND)
    {
        LOG_Info("Ethernet DHCP data");
        dump_net_config(&fsl_netif0);

    }
   
        
    if (dhcp1->state == DHCP_STATE_BOUND)
    {
        LOG_Info("USB DHCP data");
        dump_net_config(&fsl_netif1);
    }
    
    LOG_Info("DHCP OK");
    //Gateway_Status_request.Serial_Number=HAL_GetSerialNumberString();
    #if 1
    if(xTaskCreate(EndofLineTest, ((const char*)"EndofLineTest"), ENSO_AGENT_STACK_SIZE, NULL, tskIDLE_PRIORITY + 3 + PRIORITIE_OFFSET, NULL) != pdPASS)
    {
        LOG_Info("Task creation failed!.");
    }
    #endif 
    // LAN Connected so get time from cloud
    setup_system_time();

    // Perform OTA update test
#if OTA_OVER_HTTPS_TEST
    if (https_update_ota(ensoAgentSettings.otaURL,
                         ensoAgentSettings.otaPort,
                         ensoAgentSettings.otaResourceName) == 0)
    {
        HAL_Reboot();
    }
    else
    {
        LOG_Error("\r\nOTA Failed!");
    }
#endif

#if configGENERATE_RUN_TIME_STATS
    static char stats_buf[1024];
    vTaskGetRunTimeStats(stats_buf);
    LOG_Error("Schedule Stats\n%s\n",stats_buf);
#endif

    // Send a signal to EnsoAgent Thread to start EnsoAgent application
    _bSystemReadyToRun = true;

    // Kill init thread after all init tasks done
    vTaskDelete(NULL);
}

/*
 * \brief  Wrapper Function (Thread) to call EnsoAgent main function.
 *
 * \param  params Not used
 *
 * \return nonde
 */
void EnsoAgentLED(void* params)
{
    extern int EnsoMain(int argc, char* argv[]);

    // We need to wait until system (LAN, Time etc) up and run
    while (_bSystemReadyToRun == false)
    {
        OSAL_sleep_ms(100);
    }

#if FORMAT_DEVICE
    LOG_Info("EFS contents (before format):");
    EFS_List();
    LOG_Info("Formatting EFS Device Storage!");
    EFS_FormatDevice();
    LOG_Info("Formatting STO Device Storage!");
    STO_FormatDevice();
#endif

    // If there does not exist crypto keys, dump initial keys
    dump_initial_certificates_if_not_exist();

    LOG_Info("EFS contents:");
    EFS_List();

    // Prepare parameters for EnsoAgent main
    char* argv[4];
    argv[0] = ensoAgentSettings.name;
    argv[1] = ensoAgentSettings.rootCA;
    argv[2] = ensoAgentSettings.certificate;
    argv[3] = ensoAgentSettings.privateKey;

    // Call the EnsoAgent Application
    int retVal = EnsoMain(sizeof(argv) / sizeof(argv[0]), argv);

    if (0 != retVal)
    {
        LOG_Error("\n\n");
        LOG_Error("EnsoMain failed to initialise, rebooting...\n\n");
        OSAL_sleep_ms(1000);
        HAL_Reboot();
    }

    // Kill the Enso Agent Thread
    vTaskDelete(NULL);
}


/*!
 * @brief Main function.
 */
int main(void)
{
    //////// [RE:fixme] May want to start wdog here/ASAP rather than later
  
    // Setup HW
    setup_board();

    initLoggingProcess();

    LED_Manager_Init();
    

    LOG_Info("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
    LOG_Info("Starting");
    // check pin status, 0 = WG-2C, 1 = WG-2R
    if (GPIO_PinRead(GPIO3, 14))
    {
        LOG_Info("Platform detected as WG-2R");
        Gateway_Status_request.Board_Type="WG-2R";
    }
    else
    {
        LOG_Info("Platform detected as WG-2C");
        wg2c = true;
        Gateway_Status_request.Board_Type="WG-2C";
    }
    LOG_Info("Software version: %d.%d.%d", SOFTWARE_MAJOR_VERSION, SOFTWARE_MINOR_VERSION, SOFTWARE_REVISION);
    LOG_Info("Build date %s %s", __DATE__, __TIME__);
    LOG_Info("Clock speed = %dMHz", CLOCK_GetFreq(kCLOCK_CpuClk)/1000000);
    
    // Init flash driver
    SPI_Flash_Init();

    EFS_Init(); //////// [RE:nb]  Need to init EFS driver

    STO_Driver_Init(); //////// [RE:nb]  Need to init STO driver
    

    if (wg2c)
    {
        if (initPowerMonitor() == 0)
        {
#if BATTERY_FITTED
            if (initBatteryManager() != 0)
            {
                LOG_Error("Failed to initialise battery manager task");
            }
                Gateway_Status_request.Battery_connected=1;
#endif
        }
        else
        {
            LOG_Error("Failed to initialise power monitoring task");
        }
    }

    // Run RTOS task
    if(xTaskCreate(setup_lan_and_time, ((const char*)"setup_lan_and_time"), ENSO_AGENT_STACK_SIZE, NULL, tskIDLE_PRIORITY + 3 + PRIORITIE_OFFSET, NULL) != pdPASS)
    {
        LOG_Info("Task creation failed!.");
    }

    // Create a thread for EnsoAgent Application
    if(xTaskCreate(EnsoAgentLED, ((const char*)"EnsoAgentLED"), ENSO_AGENT_STACK_SIZE, NULL, tskIDLE_PRIORITY + 0, NULL) != pdPASS)
    {
        LOG_Info("Task creation failed!.");
    }



    //SJG
    vnic_main();	

    // Run RTOS
    vTaskStartScheduler();

    // Should not reach this statement
    for (;;)
        ;
}


#define LOG_OUTPUT_QUEUE_LENGTH      1000
#define LOG_OUTPUT_QUEUE_ITEM_SIZE   sizeof(char *)

QueueHandle_t debugQueue = NULL;

int maxQueuedMsgCount = 0;
void LogOutputTask(void *params)
{
    uint8_t *debugMessage;
    int msgQueueCount;
    int msgCount = 0;

    for (;;)
    {
        msgQueueCount = uxQueueMessagesWaiting(debugQueue);
        if (msgQueueCount > maxQueuedMsgCount)
        {
            maxQueuedMsgCount = msgQueueCount;
        }

        if( xQueueReceive( debugQueue, &debugMessage, portMAX_DELAY ) == pdPASS )
        {
            OSAL_printf("%s\r",debugMessage);
            vPortFree(debugMessage);
            msgCount++;
            if (msgCount >=100)
            {
                msgCount = 0;
                //OSAL_printf("\r\n=================>>>>>>>>>>>>>>>>>>>  Current pending msg = %d, Max = %d\r\n\r\n",msgQueueCount,maxQueuedMsgCount);
            }
        }
    }
}

/*
 * \brief  Create logging task and resources
 *
 * \param  None
 *
 * \return None
 */
void initLoggingProcess(void)
{
    OSAL_printf("initLoggingProcess!\r\n");

    // Queue for debug messages to be output
    debugQueue = xQueueCreate( LOG_OUTPUT_QUEUE_LENGTH, LOG_OUTPUT_QUEUE_ITEM_SIZE );
    if(debugQueue == NULL)
    {
        OSAL_printf("Log output message queue creation failed!\r\n");
        while(1);
    }

    // Create a thread for Debug output
    if(xTaskCreate(LogOutputTask, ((const char*)"Log output"), ENSO_AGENT_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
    {
        OSAL_printf("Log output task creation failed!\r\n");
    }
}
