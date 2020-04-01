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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <string.h>
//#include <timer_platform.h>
//#include <network_interface.h>

//#include "aws_iot_error.h"
//#include "aws_iot_log.h"
//#include "network_interface.h"
#include "network_platform.h"
#include "LOG_Api.h"
#include "KEY_Api.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"

#undef IOT_DEBUG
#define IOT_DEBUG LOG_Trace
#undef IOT_ERROR
#define IOT_ERROR LOG_Error

#if 0 //Nishi
/* This is the value used for ssl read timeout */
#define IOT_SSL_READ_TIMEOUT 10

void x509_cert_print(char * buf)
{
	int len = strlen(buf);
        char * n = NULL;
        for (char * s = buf; s < &buf[len] && (n = strchr(s, '\n')); s = &n[1])
        {
            *n = 0;
            IOT_DEBUG("%s", s);
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
	IOT_DEBUG("Verify requested for (Depth %d):", depth);
	if((*flags) == 0) {
		IOT_DEBUG("This certificate has no flags");
	} else {
		mbedtls_x509_crt_verify_info(buf, sizeof(buf), "  ! ", *flags);
		*flags = 0;
		IOT_DEBUG("Flags:\n%s", buf);
	}

	return 0;
}

void _iot_tls_set_connect_params(Network *pNetwork, char *pRootCALocation, char *pDeviceCertLocation,
								 char *pDevicePrivateKeyLocation, char *pDestinationURL,
								 uint16_t destinationPort, uint32_t timeout_ms, bool ServerVerificationFlag) {
	pNetwork->tlsConnectParams.DestinationPort = destinationPort;
	pNetwork->tlsConnectParams.pDestinationURL = pDestinationURL;
	pNetwork->tlsConnectParams.pDeviceCertLocation = pDeviceCertLocation;
	pNetwork->tlsConnectParams.pDevicePrivateKeyLocation = pDevicePrivateKeyLocation;
	pNetwork->tlsConnectParams.pRootCALocation = pRootCALocation;
	pNetwork->tlsConnectParams.timeout_ms = timeout_ms;
	pNetwork->tlsConnectParams.ServerVerificationFlag = ServerVerificationFlag;
}

IoT_Error_t iot_tls_init(Network *pNetwork, char *pRootCALocation, char *pDeviceCertLocation,
						 char *pDevicePrivateKeyLocation, char *pDestinationURL,
						 uint16_t destinationPort, uint32_t timeout_ms, bool ServerVerificationFlag) {
	_iot_tls_set_connect_params(pNetwork, pRootCALocation, pDeviceCertLocation, pDevicePrivateKeyLocation,
								pDestinationURL, destinationPort, timeout_ms, ServerVerificationFlag);

	pNetwork->connect = iot_tls_connect;
	pNetwork->read = iot_tls_read;
	pNetwork->write = iot_tls_write;
	pNetwork->disconnect = iot_tls_disconnect;
	pNetwork->isConnected = iot_tls_is_connected;
	pNetwork->destroy = iot_tls_destroy;

	pNetwork->tlsDataParams.flags = 0;

	return SUCCESS;
}

IoT_Error_t iot_tls_is_connected(Network *pNetwork) {
	/* Use this to add implementation which can check for physical layer disconnect */
	return NETWORK_PHYSICAL_LAYER_CONNECTED;
}


static int entropy_dummy_source( void *data, unsigned char *output,
                                 size_t len, size_t *olen )
{
    *olen = len;

    size_t use_len;
    int rnd;

    while( len > 0 )
    {
        use_len = len;
        if( use_len > sizeof(int) )
            use_len = sizeof(int);

        rnd = rand();
        memcpy( output, &rnd, use_len );
        output += use_len;
        len -= use_len;
    }

    return( 0 );
}


static void ssl_debug(void *p_dbg, int level, const char * file, int line, const char * str)
{
    LOG_Trace("%s : %d - %s", file, line, str);
}

IoT_Error_t iot_tls_connect(Network *pNetwork, TLSConnectParams *params) {
	int ret = 0;
	const char *pers = "aws_iot_tls_wrapper";
	TLSDataParams *tlsDataParams = NULL;
	char portBuffer[6];
	char vrfy_buf[512];
	char str_buf[128];
#ifdef IOT_DEBUG
	unsigned char buf[MBEDTLS_SSL_MAX_CONTENT_LEN + 1];
#endif

	if(NULL == pNetwork) {
		return NULL_VALUE_ERROR;
	}

	if(NULL != params) {
		_iot_tls_set_connect_params(pNetwork, params->pRootCALocation, params->pDeviceCertLocation,
									params->pDevicePrivateKeyLocation, params->pDestinationURL,
									params->DestinationPort, params->timeout_ms, params->ServerVerificationFlag);
	}

	tlsDataParams = &(pNetwork->tlsDataParams);

	mbedtls_net_init(&(tlsDataParams->server_fd));
	mbedtls_ssl_init(&(tlsDataParams->ssl));
	mbedtls_ssl_config_init(&(tlsDataParams->conf));
	mbedtls_ctr_drbg_init(&(tlsDataParams->ctr_drbg));
	mbedtls_x509_crt_init(&(tlsDataParams->cacert));
	mbedtls_x509_crt_init(&(tlsDataParams->clicert));
	mbedtls_pk_init(&(tlsDataParams->pkey));
	mbedtls_ssl_conf_dbg(&(tlsDataParams->conf), ssl_debug, NULL);

	IOT_DEBUG("Seeding the random number generator...");
	mbedtls_entropy_init(&(tlsDataParams->entropy));

	ret = mbedtls_entropy_add_source(&(tlsDataParams->entropy), entropy_dummy_source, NULL, 16, 1);

    if((ret = mbedtls_ctr_drbg_seed(&(tlsDataParams->ctr_drbg), mbedtls_entropy_func, &(tlsDataParams->entropy),
                                    (const unsigned char *) pers, strlen(pers))) != 0) {
       mbedtls_strerror(ret, str_buf, sizeof str_buf);
       IOT_ERROR("mbedtls_ctr_drbg_seed(): %s", str_buf);
    }

	void * keys = KEY_Open();
    if (keys == NULL)
    {
	    IOT_ERROR("Failed to Open KeysStore");
	    return NETWORK_X509_ROOT_CRT_PARSE_ERROR;
    }
	IOT_DEBUG("Loading the CA root certificate ...");
	char * ca_root_certificate = KEY_Get(keys, pNetwork->tlsConnectParams.pRootCALocation);
	if (ca_root_certificate == NULL)
	{
		IOT_ERROR("Root Certificate %s not found", pNetwork->tlsConnectParams.pDeviceCertLocation);
        KEY_Close(keys);
		return NETWORK_X509_ROOT_CRT_PARSE_ERROR;
	}
	int ca_root_certificate_sz = strlen(ca_root_certificate) + 1;
	ret = mbedtls_x509_crt_parse(&(tlsDataParams->cacert), (const unsigned char *)ca_root_certificate, ca_root_certificate_sz);
	if(ret < 0) {
		mbedtls_strerror(ret, str_buf, sizeof str_buf);
		IOT_ERROR("mbedtls_x509_crt_parse() %s while parsing root cert", str_buf);
        KEY_Free(ca_root_certificate);
        KEY_Close(keys);
		return NETWORK_X509_ROOT_CRT_PARSE_ERROR;
	}
	KEY_Free(ca_root_certificate);
	IOT_DEBUG("ok (%d skipped)", ret);
	IOT_DEBUG("Loading the client cert. and key...");
	char * client_certificate = KEY_Get(keys, pNetwork->tlsConnectParams.pDeviceCertLocation);
	if (client_certificate == NULL)
	{
		IOT_ERROR("Client Certificate %s not found", pNetwork->tlsConnectParams.pDeviceCertLocation);
        KEY_Close(keys);
		return NETWORK_X509_DEVICE_CRT_PARSE_ERROR;
	}
	int client_certificate_sz = strlen(client_certificate) + 1;
	ret = mbedtls_x509_crt_parse(&(tlsDataParams->clicert),  (const unsigned char *)client_certificate, client_certificate_sz);
	if(ret != 0) {
		mbedtls_strerror(ret, str_buf, sizeof str_buf);
		IOT_ERROR("mbedtls_x509_crt_parse(): %s while parsing device cert", str_buf);
		KEY_Free(client_certificate);
		KEY_Close(keys);
		return NETWORK_X509_DEVICE_CRT_PARSE_ERROR;
	}
    KEY_Free(client_certificate);
	char * client_private_key = KEY_Get(keys, pNetwork->tlsConnectParams.pDevicePrivateKeyLocation);
	if (client_private_key == NULL)
	{
		IOT_ERROR("Private Key %s not found", pNetwork->tlsConnectParams.pDeviceCertLocation);
		KEY_Close(keys);
		return NETWORK_PK_PRIVATE_KEY_PARSE_ERROR;
	}
	int client_private_key_sz = strlen(client_private_key) + 1;
	ret =  mbedtls_pk_parse_key(&(tlsDataParams->pkey), (const unsigned char *)client_private_key, client_private_key_sz, NULL, 0);
	if(ret != 0) {
		mbedtls_strerror(ret, str_buf, sizeof str_buf);
		IOT_ERROR("mbedtls_pk_parse_key(): %s while parsing private key", str_buf);
		IOT_DEBUG("path : %s ", pNetwork->tlsConnectParams.pDevicePrivateKeyLocation);
		KEY_Free(client_private_key);
		KEY_Close(keys);
		return NETWORK_PK_PRIVATE_KEY_PARSE_ERROR;
	}
	KEY_Free(client_private_key);
	KEY_Close(keys);
	IOT_DEBUG("ok");
	snprintf(portBuffer, 6, "%d", pNetwork->tlsConnectParams.DestinationPort);
	IOT_DEBUG("Connecting to %s/%s...", pNetwork->tlsConnectParams.pDestinationURL, portBuffer);
	if((ret = mbedtls_net_connect(&(tlsDataParams->server_fd),
					pNetwork->tlsConnectParams.pDestinationURL,
					portBuffer,
					MBEDTLS_NET_PROTO_TCP)) != 0) {
		mbedtls_strerror(ret, str_buf, sizeof str_buf);
		IOT_ERROR("mbedtls_net_connect(): %s", str_buf);
		switch(ret) {
			case MBEDTLS_ERR_NET_SOCKET_FAILED:
				return NETWORK_ERR_NET_SOCKET_FAILED;
			case MBEDTLS_ERR_NET_UNKNOWN_HOST:
				return NETWORK_ERR_NET_UNKNOWN_HOST;
			case MBEDTLS_ERR_NET_CONNECT_FAILED:
			default:
				return NETWORK_ERR_NET_CONNECT_FAILED;
		};
	}

	ret = mbedtls_net_set_block(&(tlsDataParams->server_fd));
	if(ret != 0) {
		mbedtls_strerror(ret, str_buf, sizeof str_buf);
		IOT_ERROR("net_set_block(): %s", str_buf);
		return SSL_CONNECTION_ERROR;
	} IOT_DEBUG("ok");

	IOT_DEBUG("Setting up the SSL/TLS structure...");
	if((ret = mbedtls_ssl_config_defaults(&(tlsDataParams->conf), MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM,
										  MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
		mbedtls_strerror(ret, str_buf, sizeof str_buf);
		IOT_ERROR("mbedtls_ssl_config_defaults(): %s", str_buf);
		return SSL_CONNECTION_ERROR;
	}

	mbedtls_ssl_conf_verify(&(tlsDataParams->conf), _iot_tls_verify_cert, NULL);
	if(pNetwork->tlsConnectParams.ServerVerificationFlag == true) {
		mbedtls_ssl_conf_authmode(&(tlsDataParams->conf), MBEDTLS_SSL_VERIFY_REQUIRED);
	} else {
		mbedtls_ssl_conf_authmode(&(tlsDataParams->conf), MBEDTLS_SSL_VERIFY_OPTIONAL);
	}
	mbedtls_ssl_conf_rng(&(tlsDataParams->conf), mbedtls_ctr_drbg_random, &(tlsDataParams->ctr_drbg));

	mbedtls_ssl_conf_ca_chain(&(tlsDataParams->conf), &(tlsDataParams->cacert), NULL);
	if((ret = mbedtls_ssl_conf_own_cert(&(tlsDataParams->conf), &(tlsDataParams->clicert), &(tlsDataParams->pkey))) !=
	   0) {
		mbedtls_strerror(ret, str_buf, sizeof str_buf);
		IOT_ERROR("mbedtls_ssl_conf_own_cert(): %s", str_buf);
		return SSL_CONNECTION_ERROR;
	}

	mbedtls_ssl_conf_read_timeout(&(tlsDataParams->conf), pNetwork->tlsConnectParams.timeout_ms);

	if((ret = mbedtls_ssl_setup(&(tlsDataParams->ssl), &(tlsDataParams->conf))) != 0) {
		mbedtls_strerror(ret, str_buf, sizeof str_buf);
		IOT_ERROR("mbedtls_ssl_setup(): %s", str_buf);
		return SSL_CONNECTION_ERROR;
	}
	if((ret = mbedtls_ssl_set_hostname(&(tlsDataParams->ssl), pNetwork->tlsConnectParams.pDestinationURL)) != 0) {
		mbedtls_strerror(ret, str_buf, sizeof str_buf);
		IOT_ERROR("mbedtls_ssl_set_hostname(): %s", str_buf);
		return SSL_CONNECTION_ERROR;
	}
	IOT_DEBUG("SSL state connect : %d ", tlsDataParams->ssl.state);
	mbedtls_ssl_set_bio(&(tlsDataParams->ssl), &(tlsDataParams->server_fd), mbedtls_net_send, NULL,
						mbedtls_net_recv_timeout);
	IOT_DEBUG("ok");

	IOT_DEBUG("state connect : %d ", tlsDataParams->ssl.state);
	IOT_DEBUG("Performing the SSL/TLS handshake...");
	while((ret = mbedtls_ssl_handshake(&(tlsDataParams->ssl))) != 0) {
		if(ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			mbedtls_strerror(ret, str_buf, sizeof str_buf);
			IOT_ERROR("mbedtls_ssl_handshake(): %s", str_buf);
			if(ret == MBEDTLS_ERR_X509_CERT_VERIFY_FAILED) {
				IOT_ERROR("Unable to verify the server's certificate. ");
			}
			return SSL_CONNECTION_ERROR;
		}
	}

	IOT_DEBUG("ok [ Protocol is %s ] [ Ciphersuite is %s ]", mbedtls_ssl_get_version(&(tlsDataParams->ssl)),
		  mbedtls_ssl_get_ciphersuite(&(tlsDataParams->ssl)));
	if((ret = mbedtls_ssl_get_record_expansion(&(tlsDataParams->ssl))) >= 0) {
		IOT_DEBUG("    [ Record expansion is %d ]", ret);
	} else {
		IOT_DEBUG("    [ Record expansion is unknown (compression) ]");
	}

	IOT_DEBUG("Verifying peer X.509 certificate...");

	if(pNetwork->tlsConnectParams.ServerVerificationFlag == true) {
		if((tlsDataParams->flags = mbedtls_ssl_get_verify_result(&(tlsDataParams->ssl))) != 0) {
			IOT_ERROR("failed");
			mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "  ! ", tlsDataParams->flags);
			IOT_ERROR("%s", vrfy_buf);
			ret = SSL_CONNECTION_ERROR;
		} else {
			IOT_DEBUG("ok");
			ret = SUCCESS;
		}
	} else {
		IOT_DEBUG(" Server Verification skipped");
		ret = SUCCESS;
	}

#ifdef IOT_DEBUG
	if (mbedtls_ssl_get_peer_cert(&(tlsDataParams->ssl)) != NULL) {
		mbedtls_x509_crt_info((char *) buf, sizeof(buf) - 1, "", mbedtls_ssl_get_peer_cert(&(tlsDataParams->ssl)));
		IOT_DEBUG("Peer certificate information");
		x509_cert_print((char *) buf);
	}
#endif

	mbedtls_ssl_conf_read_timeout(&(tlsDataParams->conf), IOT_SSL_READ_TIMEOUT);

	return (IoT_Error_t) ret;
}

IoT_Error_t iot_tls_write(Network *pNetwork, unsigned char *pMsg, size_t len, Timer *timer, size_t *written_len) {
	size_t written_so_far;
	bool isErrorFlag = false;
	////////int frags, ret;
	int frags, ret = 0;	//////// [RE:fixed] Inited this
	TLSDataParams *tlsDataParams = &(pNetwork->tlsDataParams);

	for(written_so_far = 0, frags = 0;
		written_so_far < len && !has_timer_expired(timer); written_so_far += ret, frags++) {
		while(!has_timer_expired(timer) &&
			  (ret = mbedtls_ssl_write(&(tlsDataParams->ssl), pMsg + written_so_far, len - written_so_far)) <= 0) {
			if(ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
				char str_buf[128];
				mbedtls_strerror(ret, str_buf, sizeof str_buf);
				IOT_ERROR("mbedtls_ssl_write(): %s", str_buf);
				/* All other negative return values indicate connection needs to be reset.
		 		* Will be caught in ping request so ignored here */
				isErrorFlag = true;
				break;
			}
		}
		if(isErrorFlag) {
			break;
		}
	}

	*written_len = written_so_far;

	if(isErrorFlag) {
		return NETWORK_SSL_WRITE_ERROR;
	} else if(has_timer_expired(timer) && written_so_far != len) {
		return NETWORK_SSL_WRITE_TIMEOUT_ERROR;
	}

	return SUCCESS;
}

IoT_Error_t iot_tls_read(Network *pNetwork, unsigned char *pMsg, size_t len, Timer *timer, size_t *read_len) {
	mbedtls_ssl_context *ssl = &(pNetwork->tlsDataParams.ssl);
	size_t rxLen = 0;
	int ret;

	while (len > 0) {
		// This read will timeout after IOT_SSL_READ_TIMEOUT if there's no data to be read
		ret = mbedtls_ssl_read(ssl, pMsg, len);
		if (ret > 0) {
			rxLen += ret;
			pMsg += ret;
			len -= ret;
		} else if (ret == 0 || (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE && ret != MBEDTLS_ERR_SSL_TIMEOUT)) {
			return NETWORK_SSL_READ_ERROR;
		}

		// Evaluate timeout after the read to make sure read is done at least once
		if (has_timer_expired(timer)) {
			break;
		}
	}

	if (len == 0) {
		*read_len = rxLen;
		return SUCCESS;
	}

	if (rxLen == 0) {
		return NETWORK_SSL_NOTHING_TO_READ;
	} else {
		return NETWORK_SSL_READ_TIMEOUT_ERROR;
	}
}

IoT_Error_t iot_tls_disconnect(Network *pNetwork) {
	mbedtls_ssl_context *ssl = &(pNetwork->tlsDataParams.ssl);
	int ret = 0;
	do {
		ret = mbedtls_ssl_close_notify(ssl);
	} while(ret == MBEDTLS_ERR_SSL_WANT_WRITE);

	/* All other negative return values indicate connection needs to be reset.
	 * No further action required since this is disconnect call */

	return SUCCESS;
}

IoT_Error_t iot_tls_destroy(Network *pNetwork) {
	TLSDataParams *tlsDataParams = &(pNetwork->tlsDataParams);

	mbedtls_net_free(&(tlsDataParams->server_fd));

	mbedtls_x509_crt_free(&(tlsDataParams->clicert));
	mbedtls_x509_crt_free(&(tlsDataParams->cacert));
	mbedtls_pk_free(&(tlsDataParams->pkey));
	mbedtls_ssl_free(&(tlsDataParams->ssl));
	mbedtls_ssl_config_free(&(tlsDataParams->conf));
	mbedtls_ctr_drbg_free(&(tlsDataParams->ctr_drbg));
	mbedtls_entropy_free(&(tlsDataParams->entropy));

	return SUCCESS;
}

#ifdef __cplusplus
}
#endif
#endif
