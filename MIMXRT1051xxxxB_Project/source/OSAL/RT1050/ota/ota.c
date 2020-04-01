/*******************************************************************************
 * Includes
 ******************************************************************************/

#include <stdlib.h>
#include <stdio.h>

#include "lwip/opt.h"
#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/md5.h"
#include "fsl_debug_console.h"
#include "mbedtls/net_sockets.h"
#include "LOG_Api.h"
#include "OSAL_Api.h"
#include "EFS_FileSystem.h"
//#include "imx_CRC_ea3_wsgw.h"
#include <SPI_Flash.issi_32mbit.h>
#include "ota.h"
#include "imx_fcb.h"

#include "SPI_Flash.h"



/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* This is the value used for ssl read timeout */
#define IOT_SSL_READ_TIMEOUT	10




/*******************************************************************************
* Prototypes
******************************************************************************/




/*******************************************************************************
* Variables
******************************************************************************/

TLSDataParams tlsDataParams;
char* md5Resource = NULL;
unsigned char* httpsBuffer = NULL;
unsigned char* imageBuffer = NULL;
int imageSize = 0;

const char MARKER_OK[] = "HTTP/1.1 200 OK";
const char MARKER_HEADER_END[] = "\r\n\r\n";
const char MARKER_CONTENT_LENGTH[] = "Content-Length: ";
const char BIN_EXTENSION[] = ".imx";
const char HASH_EXTENSION[] = ".md5";

uint32_t erase_image( uint32_t imageOffset ,uint32_t image_size);

/*******************************************************************************
 * Code
 ******************************************************************************/

void dumpHex(uint8_t* ptr, int len)
{
    PRINTF("------------------------------------------------------------");
    for (int i = 0; i < len; i++)
    {
      if (i % 16 == 0)
      {
        PRINTF("\r\n");
      }
      PRINTF(" %02X ",ptr[i]);
    }
    PRINTF("\r\n------------------------------------------------------------\r\n");
}



static void ssl_debug(void *p_dbg, int level, const char * file, int line, const char * str)
{
    LOG_Trace("%s : %d - %s", file, line, str);
}



void x509_cert_print(char * buf)
{
	int len = strlen(buf);
        char * n = NULL;
        for (char * s = buf; s < &buf[len] && (n = strchr(s, '\n')); s = &n[1])
        {
            *n = 0;
            LOG_Trace("%s", s);
        }
}



/*
 * This is a function to do further verification if needed on the cert received
 */

static int _iot_tls_verify_cert(void *data, mbedtls_x509_crt *crt, int depth, uint32_t *flags) {
	char buf[1024];
	((void) data);
	mbedtls_x509_crt_info(buf, sizeof(buf) - 1, "", crt);
	x509_cert_print(buf);
	LOG_Trace("Verify requested for (Depth %d):", depth);
	if((*flags) == 0) {
		LOG_Trace("This certificate has no flags");
	} else {
		mbedtls_x509_crt_verify_info(buf, sizeof(buf), "  ! ", *flags);
		*flags = 0;
		LOG_Trace("Flags:\n%s", buf);
	}

	return 0;
}



/* Send function used by mbedtls ssl */
static int lwipSend(void *fd, unsigned char const *buf, size_t len)
{
    return lwip_send((*(int *)fd), buf, len, 0);
}



/* Send function used by mbedtls ssl */
static int lwipRecv(void *fd, unsigned char const *buf, size_t len)
{
    return lwip_recv((*(int *)fd), (void *)buf, len, 0);
}



int https_client_tls_init(char* host, char* port)
{
#if 0
	int ret = 0;
    const char *pers = "aws_iot_tls_wrapper";
    char vrfy_buf[512];
    bool ServerVerificationFlag = false;
    const mbedtls_md_info_t *md_info;

    mbedtls_ssl_init(&(tlsDataParams.ssl));
    mbedtls_ssl_config_init(&(tlsDataParams.conf));
	mbedtls_ctr_drbg_init(&(tlsDataParams.ctr_drbg));
    mbedtls_x509_crt_init(&(tlsDataParams.cacert));
    mbedtls_x509_crt_init(&(tlsDataParams.clicert));
    mbedtls_pk_init(&(tlsDataParams.pkey));

	mbedtls_ssl_conf_dbg(&(tlsDataParams.conf), ssl_debug, NULL);

	LOG_Info("Seeding the random number generator...");
	mbedtls_entropy_init(&(tlsDataParams.entropy));
    if((ret = mbedtls_ctr_drbg_seed(&(tlsDataParams.ctr_drbg), mbedtls_entropy_func, &(tlsDataParams.entropy),
                                    (const unsigned char *) pers, strlen(pers))) != 0) {
	    LOG_Error(" failed\n  ! mbedtls_ctr_drbg_seed returned -0x%x", -ret);
	 //  return NETWORK_MBEDTLS_ERR_CTR_DRBG_ENTROPY_SOURCE_FAILED;
    }

    LOG_Info("  . Loading the CA root certificate ...");
    ret = mbedtls_x509_crt_parse(&(tlsDataParams.cacert), (const unsigned char *) mbedtls_test_ca_crt, mbedtls_test_ca_crt_len);
    if (ret < 0)
    {
        LOG_Error(" failed\n  !  mbedtls_x509_crt_parse returned -0x%x while parsing root cert", -ret);
        return NETWORK_X509_ROOT_CRT_PARSE_ERROR;
    }
    LOG_Info(" ok (%d skipped)", ret);

    LOG_Info("  . Loading the client cert. and key...");
    ret = mbedtls_x509_crt_parse(&(tlsDataParams.clicert), (const unsigned char *) mbedtls_test_cli_crt, mbedtls_test_cli_crt_len);
    if (ret != 0)
    {
        LOG_Error(" failed\n  !  mbedtls_x509_crt_parse returned -0x%x while parsing device cert", -ret);
        return NETWORK_X509_DEVICE_CRT_PARSE_ERROR;
    }

    ret = mbedtls_pk_parse_key(&(tlsDataParams.pkey), (const unsigned char *) mbedtls_test_cli_key, mbedtls_test_cli_key_len, NULL, 0);
    if (ret != 0)
    {
        LOG_Error(" failed\n  !  mbedtls_pk_parse_key returned -0x%x while parsing private key", -ret);
        return NETWORK_PK_PRIVATE_KEY_PARSE_ERROR;
    }
    LOG_Info(" ok");
    LOG_Info("Connecting to %s/%s", host, "443");

    struct addrinfo hints;
    struct addrinfo *res;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;


     OSAL_Log(" Connecting to Host %s:\n", host);

    if((ret = mbedtls_net_connect(&(tlsDataParams.fd), host, "443", 0)) != 0) 
    {
        OSAL_Log("ERROR: mbedtls_net_connect ret(%d)\n", ret);
       // goto update_ota_exit;
    }


    LOG_Info("  . Setting up the SSL/TLS structure...");
    if ((ret = mbedtls_ssl_config_defaults(&(tlsDataParams.conf), MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM,
                                           MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
    {
        LOG_Error(" failed\n  ! mbedtls_ssl_config_defaults returned -0x%x", -ret);
        return SSL_CONNECTION_ERROR;
    }

    mbedtls_ssl_conf_verify(&(tlsDataParams.conf), _iot_tls_verify_cert, NULL);
    if (ServerVerificationFlag == true)
    {
        mbedtls_ssl_conf_authmode(&(tlsDataParams.conf), MBEDTLS_SSL_VERIFY_REQUIRED);
    }
    else
    {
        mbedtls_ssl_conf_authmode(&(tlsDataParams.conf), MBEDTLS_SSL_VERIFY_OPTIONAL);
    }
	mbedtls_ssl_conf_rng(&(tlsDataParams.conf), mbedtls_ctr_drbg_random, &(tlsDataParams.ctr_drbg));

    mbedtls_ssl_conf_ca_chain(&(tlsDataParams.conf), &(tlsDataParams.cacert), NULL);
    if ((ret = mbedtls_ssl_conf_own_cert(&(tlsDataParams.conf), &(tlsDataParams.clicert), &(tlsDataParams.pkey))) !=
        0)
    {
        LOG_Error(" failed\n  ! mbedtls_ssl_conf_own_cert returned %d", ret);
        return SSL_CONNECTION_ERROR;
    }

    if ((ret = mbedtls_ssl_setup(&(tlsDataParams.ssl), &(tlsDataParams.conf))) != 0)
    {
        LOG_Error(" failed\n  ! mbedtls_ssl_setup returned -0x%x", -ret);
        return SSL_CONNECTION_ERROR;
    }
    if ((ret = mbedtls_ssl_set_hostname(&(tlsDataParams.ssl), host)) != 0)
    {
        LOG_Error(" failed\n  ! mbedtls_ssl_set_hostname returned %d", ret);
        return SSL_CONNECTION_ERROR;
    }
    LOG_Info("\r\nSSL state connect : %d ", tlsDataParams.ssl.state);

    mbedtls_ssl_set_bio(&(tlsDataParams.ssl), &(tlsDataParams.fd), lwipSend,  (mbedtls_ssl_recv_t*) lwipRecv, NULL);
 
    LOG_Info(" ok");
    LOG_Info("\r\nSSL state connect : %d ", tlsDataParams.ssl.state);
    LOG_Info("  . Performing the SSL/TLS handshake...");
    while ((ret = mbedtls_ssl_handshake(&(tlsDataParams.ssl))) != 0)
    {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
        {
            LOG_Error(" failed\n  ! mbedtls_ssl_handshake returned -0x%x", -ret);
            if (ret == MBEDTLS_ERR_X509_CERT_VERIFY_FAILED)
            {
                LOG_Error(
                    "    Unable to verify the server's certificate. "
                    "    Alternatively, you may want to use "
                    "auth_mode=optional for testing purposes.");
            }
            return SSL_CONNECTION_ERROR;
        }
    }

    LOG_Info(" ok\n    [ Protocol is %s ]\r\n    [ Ciphersuite is %s ]",
              mbedtls_ssl_get_version(&(tlsDataParams.ssl)), mbedtls_ssl_get_ciphersuite(&(tlsDataParams.ssl)));
    if ((ret = mbedtls_ssl_get_record_expansion(&(tlsDataParams.ssl))) >= 0)
    {
        LOG_Info("    [ Record expansion is %d ]", ret);
    }
    else
    {
        LOG_Info("    [ Record expansion is unknown (compression) ]");
    }

    LOG_Info("  . Verifying peer X.509 certificate...");

    if (ServerVerificationFlag == true)
    {
        if ((tlsDataParams.flags = mbedtls_ssl_get_verify_result(&(tlsDataParams.ssl))) != 0)
        {
            LOG_Error(" failed");
            mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "  ! ", tlsDataParams.flags);
            LOG_Error("%s", vrfy_buf);
            ret = SSL_CONNECTION_ERROR;
        }
        else
        {
            LOG_Info(" ok");
            ret = SUCCESS;
        }
    }
    else
    {
        LOG_Info(" Server Verification skipped");
        ret = SUCCESS;
    }

    mbedtls_ssl_conf_read_timeout(&(tlsDataParams.conf), IOT_SSL_READ_TIMEOUT);
#endif
    return 1;
}



/* Release TLS */
void https_client_tls_release()
{
#if 0
    mbedtls_net_free(&(tlsDataParams.fd)); //nishi
    mbedtls_x509_crt_free(&(tlsDataParams.clicert));
    mbedtls_x509_crt_free(&(tlsDataParams.cacert));
    mbedtls_pk_free(&(tlsDataParams.pkey));
    mbedtls_ssl_free(&(tlsDataParams.ssl));
    mbedtls_ssl_config_free(&(tlsDataParams.conf));
	mbedtls_ctr_drbg_free(&(tlsDataParams.ctr_drbg));
    mbedtls_entropy_free(&(tlsDataParams.entropy));
#endif
}



int initOTA()
{
    httpsBuffer = malloc(MBEDTLS_SSL_MAX_CONTENT_LEN);
    LOG_Trace("httpsBuffer = %p", httpsBuffer);
    if (httpsBuffer == NULL)

    {
        LOG_Error("malloc failed");
        return -1;
    }
    memset(httpsBuffer, 0x00, sizeof(httpsBuffer));

////Nishi
    #if 1
    imageBuffer = malloc(FLASHMAP_IMAGE_SIZE);
    LOG_Trace("imageBuffer = %p", imageBuffer);
    if (imageBuffer == NULL)
    {
        LOG_Error("malloc failed");
        return -1;
    }
    memset(imageBuffer, 0xFF, sizeof(imageBuffer));
#endif 
    return 0;
}



void releaseOTA()
{
    if (imageBuffer)
        free(imageBuffer);
    if (httpsBuffer)
        free(httpsBuffer);
    if (md5Resource)
        free(md5Resource);
}



int sendRequest(char* resource,char *host)
{
    //https://stg-upgrade.wi-safeconnect.com/op-1.52_0.13_fireangel_hub_ea3_ameba_stag.imx
    char *request;
    LOG_Info(" Gateway Sending HTTP request for :\r\n%s\r\n", resource);
   //send http request
   // resource= "op-1.52_0.13_fireangel_hub_ea3_ameba_stag.imx\r\n";
    request = (unsigned char *) malloc(strlen("GET /") +
                                              strlen(resource) +
                                              strlen(" HTTP/1.1\r\nHost: ") +
                                              strlen(host) + strlen("\r\n\r\n") + 1);

    sprintf(request, "GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n",resource, host);
    LOG_Info(" Gateway Sending HTTP request:\r\n%s\r\n", request);

    int ret = 0;
    int len = strlen(request);
    while( ( ret = mbedtls_ssl_write( &(tlsDataParams.ssl), request, len ) ) <= 0 )
    {
        if(ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
        {
            LOG_Error(" failed\n  ! mbedtls_ssl_write returned %d", ret);
            return ret;
        }
    }
    len = ret;
    return ret;
}



int receiveResponse(unsigned char* contentBuffer, int* receivedContentSize, int maxContentBufferSize)
{
    int ret = 0;
    int len = 0;
    bool headerReceived = false;
    int expectedContentSize = 0;
    *receivedContentSize = 0;

    do
    {
        if(*receivedContentSize > maxContentBufferSize)
        {
            LOG_Error("FAILED: Content will not fit in buffer");
            return -1;
        }

        ret = mbedtls_ssl_read( &(tlsDataParams.ssl), httpsBuffer, MBEDTLS_SSL_MAX_CONTENT_LEN );

        if(ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
            continue;

        if(ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)
            break;

        if(ret < 0)
        {
            LOG_Error("failed\n  ! mbedtls_ssl_read returned %d", ret);
            return ret;
        }

        if(ret == 0)
        {
            // LOG_Trace("\r\nEOF");
            break;
        }

        len = ret;

        // Receive HTTP header
        if (!headerReceived)
        {
            LOG_Info("Received HTTP header:\r\n%s", httpsBuffer);

            LOG_Info("Parsing HTTP header...");

            // Look for 200
            if(strncmp((char *)httpsBuffer, MARKER_OK, strlen(MARKER_OK)) != 0)
            {
                LOG_Error("FAILED: Bad HTTP response");
                return -1;
            }

            // Look for content length
            int parseIndex = 0;
            for (; (parseIndex < len - strlen(MARKER_CONTENT_LENGTH) + 1) && (expectedContentSize == 0); parseIndex++)
            {
                if(strncmp((char *)(httpsBuffer + parseIndex), MARKER_CONTENT_LENGTH, strlen(MARKER_CONTENT_LENGTH)) == 0)
                {
                    parseIndex += strlen(MARKER_CONTENT_LENGTH);
                    char expectedContentLengthString[32];
                    for (int i = 0; (i < 32) && (parseIndex < len) && (httpsBuffer[parseIndex] != '\r') && (httpsBuffer[parseIndex] != '\n'); i++)
                        expectedContentLengthString[i] = httpsBuffer[parseIndex++];
                    expectedContentSize = atoi(expectedContentLengthString);
                }
            }

            // Content length not found in first frame
            if (expectedContentSize == 0)
            {
                LOG_Error("FAILED: Content length not found in header");
                return -1;
            }

            LOG_Info("Expecting %d bytes", expectedContentSize);

            // Look for header end
            for (; (parseIndex < len - strlen(MARKER_HEADER_END) + 1) && !headerReceived; parseIndex++)
            {
                if(strncmp((char *)(httpsBuffer + parseIndex), MARKER_HEADER_END, strlen(MARKER_HEADER_END)) == 0)
                {
                    LOG_Trace("HEADER END FOUND @ index: %d", parseIndex);

                    headerReceived = true;

                    LOG_Info("Receiving image data...");

                    // Copy remainder to image
                    int remainder = len - 1 - parseIndex - 3;
                    if (remainder > expectedContentSize)
                        remainder = expectedContentSize;
                     LOG_Trace("HEADER REMAINDER = %d", remainder);
                    if (remainder > 0)
                    {
                        memcpy(contentBuffer, &httpsBuffer[parseIndex+4], remainder);
                        *receivedContentSize += remainder;
                    }
                }
            }

           
            // Header not found in first frame
            if (!headerReceived)
            {
                LOG_Error("FAILED: HTTP header not found");
                return -1;
            }
        }

        // Receive image data
        else
        {
            if (*receivedContentSize + len > expectedContentSize)
                len = expectedContentSize - *receivedContentSize;
            memcpy(contentBuffer + *receivedContentSize, httpsBuffer, len);
            *receivedContentSize += len;
        }

              //  for (int i=0;i<1000;i++)
                //{
                //  LOG_Info(" Image data..%x and buffer %x",httpsBuffer[i],contentBuffer[i]);
                //}

        // Check if done
        if (*receivedContentSize == expectedContentSize)
        {
            LOG_Info("Received %d bytes", *receivedContentSize);
            // dumpHex(contentBuffer, *receivedContentSize);
            return *receivedContentSize;
        }
    }
    while(1);

    // Should never reach this
    return -1;
}



int downloadResource(char* host, char* port, char* resource, unsigned char* contentBuffer, int* receivedContentSize, int maxContentBufferSize)
{
    LOG_Info("Downloading resource: %s", resource);
     LOG_Info("Downloading host: %s", host);

    int ret = 0;

    // Initialise HTTPS
    if ((ret = https_client_tls_init(host, port)) < 0)
    {
        https_client_tls_release();
        return ret;
    }

    // Send HTTP request
    if ((ret = sendRequest(resource,host)) < 0)
    {
        https_client_tls_release();
        return ret;
    }

    // Receive HTTP response
    if ((ret = receiveResponse(contentBuffer, receivedContentSize, maxContentBufferSize)) < 0)
    {
        https_client_tls_release();
        return ret;
    }

    // Done
    https_client_tls_release();
    return ret;
}



int generateMD5Resource(char* resource)
{
    if (md5Resource)
        free(md5Resource);
    md5Resource = malloc(strlen(resource) + strlen(HASH_EXTENSION) + 1);

    char* binExtensionSubstring = strstr(resource, BIN_EXTENSION);
    if (!binExtensionSubstring)
    {
        LOG_Error("Bad image resource filename");
        return -1;
    }
    int binExtensionEnd = binExtensionSubstring - resource + strlen(BIN_EXTENSION);
    memcpy(md5Resource, resource, binExtensionEnd);
    memcpy(md5Resource + binExtensionEnd, HASH_EXTENSION, strlen(HASH_EXTENSION));
    memcpy(md5Resource + binExtensionEnd + strlen(HASH_EXTENSION), resource + binExtensionEnd, strlen(resource) - binExtensionEnd + 1);

    return 0;
}



void asciiToBytes(char* str, uint8_t* bytes, int len)
{
    char byteString[3];
    byteString[2] = '\0';
    for (int i = 0; i < len; i++)
    {
        memcpy(byteString, (str + i * 2), 2);
        bytes[i] = strtol(byteString, NULL, 16);
    }
}



void prepareImage()
{
    LOG_Info("Preparing image..%d",imageSize);

    for (int i=0;i<16;i++)
    {
        LOG_Info(" image Header %x...",imageBuffer[i]);
    }
    //memcpy(imageBuffer, imx_fcb, sizeof(imx_fcb));
}



uint8_t checkActiveImageFlag()
{
    uint8_t activeImage;
    SPI_Flash_Read(FLASHMAP_IMAGE_FLAG_SECTOR, &activeImage, 1);   //////// [RE:nb] This assumes SPI flash driver has been initialised!
    return activeImage;
}



uint32_t determineTargetImageOffset()
{
    uint8_t activeImage = checkActiveImageFlag();
    uint32_t targetImageOffset = (activeImage == 0x0a ? FLASHMAP_IMAGE_B_OFFSET : FLASHMAP_IMAGE_A_OFFSET);
    return targetImageOffset;
}



int swapActiveImage()
{
    uint8_t activeImage = checkActiveImageFlag();
    uint8_t flagBuffer[FLASH_PAGE_SIZE];
    memset(flagBuffer, 0xFF, FLASH_PAGE_SIZE);
    flagBuffer[0] = (activeImage == 0x0a ? 0x0b : 0x0a);
    LOG_Info("Setting active image flag to 0x%02X", flagBuffer[0]);

    status_t status = SPI_Flash_Erase(FLASHMAP_IMAGE_FLAG_SECTOR);
    if (status != kStatus_Success)
    {
        LOG_Info("Erase sector failure !");
        return -1;
    }

    status = SPI_Flash_Write(FLASHMAP_IMAGE_FLAG_SECTOR, flagBuffer, sizeof(flagBuffer));
    if (status != kStatus_Success)
    {
        LOG_Info("Program page failure !");
        return -1;
    }

    return 0;
}



int flashErase()
{
    int ret;
    uint8_t activeImage = checkActiveImageFlag();
    uint32_t targetImageOffset = (activeImage == 0x0a ? FLASHMAP_IMAGE_B_OFFSET : FLASHMAP_IMAGE_A_OFFSET);
    LOG_Info(" Erasing FLASH...Image offset %x Image Size%x\r\n",targetImageOffset,imageSize);
     
    erase_image(targetImageOffset,FLASHMAP_IMAGE_SIZE);
    SPI_Flash_Write(targetImageOffset, imageBuffer,imageSize); 

     if (activeImage==0x0a)     
     {  
    LOG_Info(" Erasing FLASH... B Partition\r\n");    
}
else
{
      LOG_Info(" Erasing FLASH... A Partition\r\n");   
}
    #if 0
    if (activeImage==0x0a)     
     {                         
        erase_image(FLASHMAP_IMAGE_B_OFFSET,FLASHMAP_IMAGE_SIZE);
        // Program  Partition A
       ret = SPI_Flash_Write(FLASHMAP_IMAGE_B_OFFSET, imageBuffer,imageSize);  
       LOG_Info(" Erasing FLASH... B Partition\r\n");    
    }else
    {
        erase_image(FLASHMAP_IMAGE_A_OFFSET,FLASHMAP_IMAGE_SIZE);
        ret = SPI_Flash_Write(FLASHMAP_IMAGE_A_OFFSET, imageBuffer, imageSize);  
        LOG_Info(" Erasing FLASH... A Partition\r\n");         
    }
#endif 

    return 1;
}
 uint32_t erase_image( uint32_t imageOffset ,uint32_t image_size)
{
    if (image_size!=0)
    {
      LOG_Info(" Erasing FLASH...Image offset %x Image Size%x\r\n",imageOffset,image_size);
        int ret = 0;

        int sectorNum = 0;
        uint32_t flashDataLen;
        uint8_t flashData[FLASH_SECTOR_SIZE];
        int nSectors = image_size / FLASH_SECTOR_SIZE;
        for (int idxErase = 0; idxErase <nSectors; idxErase++)
        {
          uint32_t eraseAddress = imageOffset + idxErase * FLASH_SECTOR_SIZE;
          LOG_Info("Erasing sector %d of %d @ %p...", (idxErase + 1), nSectors, eraseAddress);
          status_t status = SPI_Flash_Erase(eraseAddress);
          if (status != kStatus_Success)
          {
              LOG_Info("Erase sector failure !");
              return -1;
          }
        }
    }
     
    LOG_Info("Erase Done\r\n");
    return 0;
}


int verifyImage()
{
    int ret;
    int sectorNum = 0;
    static uint8_t flashData[FLASH_SECTOR_SIZE];
    uint32_t flashDataLen;
    int32_t imageLength = imageSize;
    uint8_t activeImage = checkActiveImageFlag();
    uint32_t imageOffset = (activeImage == 0x0a ? FLASHMAP_IMAGE_B_OFFSET : FLASHMAP_IMAGE_A_OFFSET);
  

    LOG_Info("Verifying %d bytes. image offset%x..", imageLength,imageOffset);

    while (imageLength > 0)
    {
        if (imageLength >= FLASH_SECTOR_SIZE)
        {
            flashDataLen = FLASH_SECTOR_SIZE;
        }
        else
        {
            flashDataLen = imageLength;
        }

        if ((ret = SPI_Flash_Read(imageOffset, flashData, flashDataLen)) == 0)
        {
            if (memcmp(&imageBuffer[sectorNum * FLASH_SECTOR_SIZE], (void *)flashData, flashDataLen) != 0)
            {
                LOG_Error("Verification failed");
                return -1;
            }
        }
        else
        {
            LOG_Error("Verification flash read failed %d",ret);
            return -1;
        }
        sectorNum++;
        imageOffset += flashDataLen;
        imageLength -= flashDataLen;
    }

    LOG_Error("Verification OK");
    return 0;
}






int https_update_ota(char* host, char* port, char* resource)
{
    int ret = 0;

    // Initialise OTA buffers
    if ((ret = initOTA()) < 0)
    {
        releaseOTA();
        return ret;
    }

    // Initialise filesystem
    EFS_Init();

    // Download image
  #if 1  //Nishi
    if ((ret = downloadResource(host, port, resource, imageBuffer, &imageSize, FLASHMAP_IMAGE_SIZE)) < 0)
    {
        releaseOTA();
        return ret;
    }
    #endif 

    // Manipulate image
   prepareImage();

#if 1 //Nishi
    // Erase existing image from flash
    if ((ret = flashErase()) < 0)
    {
        releaseOTA();
        return ret;
    }

   

    // Verify written image
    if ((ret = verifyImage()) < 0)
    {
        releaseOTA();
        return ret;
    }

    // Flag new image as active
    if ((ret = swapActiveImage()) < 0)
    {
        releaseOTA();
        return ret;
    }
#endif 
    // Cleanup
    releaseOTA();



    // Done
    return 0;
}
