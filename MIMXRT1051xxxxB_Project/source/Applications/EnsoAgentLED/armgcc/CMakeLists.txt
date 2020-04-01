INCLUDE(CMakeForceCompiler)

# CROSS COMPILER SETTING
SET(CMAKE_SYSTEM_NAME Generic)
CMAKE_MINIMUM_REQUIRED (VERSION 2.6)

# THE VERSION NUMBER
SET (Tutorial_VERSION_MAJOR 1)
SET (Tutorial_VERSION_MINOR 0)

# ENABLE ASM
ENABLE_LANGUAGE(ASM)

SET(CMAKE_STATIC_LIBRARY_PREFIX)
SET(CMAKE_STATIC_LIBRARY_SUFFIX)

SET(CMAKE_EXECUTABLE_LIBRARY_PREFIX)
SET(CMAKE_EXECUTABLE_LIBRARY_SUFFIX)


 
# DIRECTORIES
SET(ProjDirPath ${CMAKE_CURRENT_SOURCE_DIR})
SET(SDKDirPath "$ENV{SDK_DIR}")
SET(TopDirPath "$ENV{TOP_DIR}")
SET(LibDirPath "${TopDirPath}/Libs")
SET(SrcDirPath "${TopDirPath}/Source")



# NAME
SET(ProjName $ENV{BIN_NAME})



SET(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -D__STARTUP_CLEAR_BSS")
SET(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -D__STARTUP_INITIALIZE_NONCACHEDATA")
SET(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -Wall")
SET(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -fno-common")
SET(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -ffunction-sections")
SET(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -fdata-sections")
SET(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -ffreestanding")
SET(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -fno-builtin")
SET(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -mthumb")
SET(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -mapcs")
SET(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -std=gnu99")
SET(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -mcpu=cortex-m7")
SET(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -mfloat-abi=hard")
SET(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -mfpu=fpv5-d16")


	
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DFSL_SDK_ENABLE_DRIVER_CACHE_CONTROL=1")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DSDRAM_MPU_INIT")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DFSL_FEATURE_PHYKSZ8081_USE_RMII50M_MODE")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_POSIX_SOURCE")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DCPU_MIMXRT1052DVL6A")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DSDK_DEBUGCONSOLE=1")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DPRINTF_ADVANCED_ENABLE=1")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_RTOS=1")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DLWIP_DNS=1")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DLWIP_DHCP=1")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DMBEDTLS_CONFIG_FILE='\"ksdk_mbedtls_config_ea3.h\"'")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DFSL_RTOS_FREE_RTOS")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-common")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fdata-sections")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffreestanding")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-builtin")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mthumb")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mapcs")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-m7")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfloat-abi=hard")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpu=fpv5-d16")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -MMD")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -MP")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fomit-frame-pointer")

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-function")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-variable")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-but-set-variable")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-strict-aliasing")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-pointer-sign")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fmessage-length=0")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsigned-char")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_WANT_IO_C99_FORMATS")

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_IOT_DEBUG")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_IOT_INFO")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_IOT_WARN")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_IOT_ERROR")

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DSTO_DIR='\"\"'")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DGW_FWNAM='\"1_sprue_1_36_sprue_hub_intamac_sprue_ameba\"'")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DCHECK_UPG_FWNAM=0")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DFORMAT_DEVICE=0")	########
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DOTA_OVER_HTTPS_TEST=0")	########

########SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DFLASH_DRIVER='\"EFS_FileSystem.hf_512mbit.h\"'")
########SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DFLASH_DRIVER='\"EFS_FileSystem.winbond_8mbit.h\"'")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DFLASH_DRIVER='\"EFS_FileSystem.issi_32mbit.h\"'")

########SET(CMAKE_C_FLAGS_RAMDEBUG "${CMAKE_C_FLAGS_RAMDEBUG} -DEFS_FLASH_INIT")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DEFS_FLASH_INIT")



SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --specs=nano.specs")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --specs=nosys.specs")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wall")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fno-common")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -ffunction-sections")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fdata-sections")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -ffreestanding")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fno-builtin")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mthumb")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mapcs")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Xlinker")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --gc-sections")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Xlinker")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Xlinker")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -z")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Xlinker")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} muldefs")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mcpu=cortex-m7")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mfloat-abi=hard")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mfpu=fpv5-d16")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Map=$ENV{BUILD_TYPE}/${ProjName}.map")



include_directories(${ProjDirPath}/..)
include_directories(${SDKDirPath}/CMSIS/Include)
include_directories(${SDKDirPath}/devices)
include_directories(${SDKDirPath}/middleware/mbedtls/include)
include_directories(${SDKDirPath}/middleware/mbedtls/port/ksdk)
include_directories(${SDKDirPath}/rtos/freertos/Source/include)
include_directories(${SDKDirPath}/rtos/freertos/Source/portable/GCC/ARM_CM4F)
include_directories(${SDKDirPath}/devices/MIMXRT1052/drivers)
include_directories(${SDKDirPath}/devices/MIMXRT1052)
include_directories(${SDKDirPath}/devices/MIMXRT1052/utilities/io)
include_directories(${SDKDirPath}/devices/MIMXRT1052/utilities/str)
include_directories(${SDKDirPath}/devices/MIMXRT1052/utilities/log)
include_directories(${SDKDirPath}/devices/MIMXRT1052/utilities)
include_directories(${SDKDirPath}/middleware/lwip/port)
include_directories(${SDKDirPath}/middleware/lwip/src)
include_directories(${SDKDirPath}/middleware/lwip/src/include)
include_directories(${SDKDirPath}/middleware/lwip/contrib/apps)
#include_directories(${SDKDirPath}/middleware/lwip/src/include/lwip/apps)

include_directories(${LibDirPath}/aws-iot-device-sdk-embedded-C/include)
include_directories(${LibDirPath}/aws-iot-device-sdk-embedded-C/external_libs/jsmn)

include_directories(${SrcDirPath}/Applications/Common/)
include_directories(${SrcDirPath}/CloudComms/)
include_directories(${SrcDirPath}/CloudComms/AWS/)
include_directories(${SrcDirPath}/CloudComms/AWS/FreeRTOS/)
include_directories(${SrcDirPath}/Configuration/RT1050/)
include_directories(${SrcDirPath}/DeviceHandlers/Api/)
include_directories(${SrcDirPath}/DeviceHandlers/Common/)
include_directories(${SrcDirPath}/DeviceHandlers/GatewayHandler/)
include_directories(${SrcDirPath}/DeviceHandlers/LEDHandler/)
include_directories(${SrcDirPath}/DeviceHandlers/TestHandler/)
include_directories(${SrcDirPath}/DeviceHandlers/UpgradeHandler/)
include_directories(${SrcDirPath}/DeviceHandlers/UpgradeHandler/firmware/)
include_directories(${SrcDirPath}/DeviceHandlers/WiSafeHandler/)
include_directories(${SrcDirPath}/EnsoComms/)
include_directories(${SrcDirPath}/HAL/)
include_directories(${SrcDirPath}/KeyStore/)
include_directories(${SrcDirPath}/KeyStore/FreeRTOS/)
include_directories(${SrcDirPath}/LocalShadow/Api)
include_directories(${SrcDirPath}/LocalShadow/ObjectStore)
include_directories(${SrcDirPath}/Logger/)
include_directories(${SrcDirPath}/OSAL/)
include_directories(${SrcDirPath}/OSAL/RT1050/)
include_directories(${SrcDirPath}/OSAL/RT1050/board/)
include_directories(${SrcDirPath}/OSAL/RT1050/aws/platform/RT1050/common/)
include_directories(${SrcDirPath}/OSAL/RT1050/aws/platform/RT1050/mbedtls/)
include_directories(${SrcDirPath}/OSAL/RT1050/efs/)
include_directories(${SrcDirPath}/OSAL/RT1050/tinyprintf/)
include_directories(${SrcDirPath}/OSAL/RT1050/watchdog/)
include_directories(${SrcDirPath}/OSAL/RT1050/sntp/)
include_directories(${SrcDirPath}/OSAL/RT1050/ota/)
include_directories(${SrcDirPath}/Storage/)
include_directories(${SrcDirPath}/Watchdog/)



add_executable(${ProjName}.elf 
"${SDKDirPath}/devices/MIMXRT1052/drivers/fsl_enet.c"
"${SDKDirPath}/devices/MIMXRT1052/drivers/fsl_trng.c"
"${SDKDirPath}/devices/MIMXRT1052/drivers/fsl_common.c"
"${SDKDirPath}/devices/MIMXRT1052/drivers/fsl_dcp.c"
"${SDKDirPath}/devices/MIMXRT1052/drivers/fsl_cache.c"
"${SDKDirPath}/devices/MIMXRT1052/drivers/fsl_clock.c"
"${SDKDirPath}/devices/MIMXRT1052/drivers/fsl_rtwdog.c"
"${SDKDirPath}/devices/MIMXRT1052/system_MIMXRT1052.c"
"${SDKDirPath}/devices/MIMXRT1052/gcc/startup_MIMXRT1052.S"
"${SDKDirPath}/devices/MIMXRT1052/drivers/fsl_lpuart.c"
"${SDKDirPath}/devices/MIMXRT1052/utilities/io/fsl_io.c"
"${SDKDirPath}/devices/MIMXRT1052/utilities/log/fsl_log.c"
"${SDKDirPath}/devices/MIMXRT1052/utilities/str/fsl_str.c"
"${SDKDirPath}/devices/MIMXRT1052/utilities/fsl_debug_console.c"
"${SDKDirPath}/devices/MIMXRT1052/utilities/fsl_assert.c"
"${SDKDirPath}/devices/MIMXRT1052/drivers/fsl_gpio.c"
"${SDKDirPath}/devices/MIMXRT1052/drivers/fsl_flexspi.c"
"${SDKDirPath}/middleware/lwip/port/ethernetif.c"
"${SDKDirPath}/middleware/lwip/port/sys_arch.c"
"${SDKDirPath}/middleware/lwip/src/api/api_lib.c"
"${SDKDirPath}/middleware/lwip/src/api/api_msg.c"
"${SDKDirPath}/middleware/lwip/src/api/err.c"
"${SDKDirPath}/middleware/lwip/src/api/netbuf.c"
"${SDKDirPath}/middleware/lwip/src/api/netdb.c"
"${SDKDirPath}/middleware/lwip/src/api/netifapi.c"
"${SDKDirPath}/middleware/lwip/src/api/sockets.c"
"${SDKDirPath}/middleware/lwip/src/api/tcpip.c"
"${SDKDirPath}/middleware/lwip/src/core/def.c"
"${SDKDirPath}/middleware/lwip/src/core/dns.c"
"${SDKDirPath}/middleware/lwip/src/core/inet_chksum.c"
"${SDKDirPath}/middleware/lwip/src/core/init.c"
"${SDKDirPath}/middleware/lwip/src/core/ip.c"
"${SDKDirPath}/middleware/lwip/src/core/mem.c"
"${SDKDirPath}/middleware/lwip/src/core/memp.c"
"${SDKDirPath}/middleware/lwip/src/core/netif.c"
"${SDKDirPath}/middleware/lwip/src/core/pbuf.c"
"${SDKDirPath}/middleware/lwip/src/core/raw.c"
"${SDKDirPath}/middleware/lwip/src/core/stats.c"
"${SDKDirPath}/middleware/lwip/src/core/sys.c"
"${SDKDirPath}/middleware/lwip/src/core/tcp.c"
"${SDKDirPath}/middleware/lwip/src/core/tcp_in.c"
"${SDKDirPath}/middleware/lwip/src/core/tcp_out.c"
"${SDKDirPath}/middleware/lwip/src/core/timeouts.c"
"${SDKDirPath}/middleware/lwip/src/core/udp.c"
"${SDKDirPath}/middleware/lwip/src/core/ipv4/autoip.c"
"${SDKDirPath}/middleware/lwip/src/core/ipv4/dhcp.c"
"${SDKDirPath}/middleware/lwip/src/core/ipv4/etharp.c"
"${SDKDirPath}/middleware/lwip/src/core/ipv4/icmp.c"
"${SDKDirPath}/middleware/lwip/src/core/ipv4/igmp.c"
"${SDKDirPath}/middleware/lwip/src/core/ipv4/ip4.c"
"${SDKDirPath}/middleware/lwip/src/core/ipv4/ip4_addr.c"
"${SDKDirPath}/middleware/lwip/src/core/ipv4/ip4_frag.c"
"${SDKDirPath}/middleware/lwip/src/core/ipv6/dhcp6.c"
"${SDKDirPath}/middleware/lwip/src/core/ipv6/ethip6.c"
"${SDKDirPath}/middleware/lwip/src/core/ipv6/icmp6.c"
"${SDKDirPath}/middleware/lwip/src/core/ipv6/inet6.c"
"${SDKDirPath}/middleware/lwip/src/core/ipv6/ip6.c"
"${SDKDirPath}/middleware/lwip/src/core/ipv6/ip6_addr.c"
"${SDKDirPath}/middleware/lwip/src/core/ipv6/ip6_frag.c"
"${SDKDirPath}/middleware/lwip/src/core/ipv6/mld6.c"
"${SDKDirPath}/middleware/lwip/src/core/ipv6/nd6.c"
"${SDKDirPath}/middleware/lwip/src/netif/ethernet.c"
"${SDKDirPath}/middleware/lwip/src/netif/lowpan6.c"
"${SDKDirPath}/middleware/lwip/src/netif/slipif.c"
"${SDKDirPath}/middleware/lwip/src/netif/ppp/auth.c"
"${SDKDirPath}/middleware/lwip/src/netif/ppp/ccp.c"
"${SDKDirPath}/middleware/lwip/src/netif/ppp/chap_ms.c"
"${SDKDirPath}/middleware/lwip/src/netif/ppp/chap-md5.c"
"${SDKDirPath}/middleware/lwip/src/netif/ppp/chap-new.c"
"${SDKDirPath}/middleware/lwip/src/netif/ppp/demand.c"
"${SDKDirPath}/middleware/lwip/src/netif/ppp/eap.c"
"${SDKDirPath}/middleware/lwip/src/netif/ppp/eui64.c"
"${SDKDirPath}/middleware/lwip/src/netif/ppp/fsm.c"
"${SDKDirPath}/middleware/lwip/src/netif/ppp/ipcp.c"
"${SDKDirPath}/middleware/lwip/src/netif/ppp/ipv6cp.c"
"${SDKDirPath}/middleware/lwip/src/netif/ppp/lcp.c"
"${SDKDirPath}/middleware/lwip/src/netif/ppp/lwip_ecp.c"
"${SDKDirPath}/middleware/lwip/src/netif/ppp/magic.c"
"${SDKDirPath}/middleware/lwip/src/netif/ppp/mppe.c"
"${SDKDirPath}/middleware/lwip/src/netif/ppp/multilink.c"
"${SDKDirPath}/middleware/lwip/src/netif/ppp/ppp.c"
"${SDKDirPath}/middleware/lwip/src/netif/ppp/pppapi.c"
"${SDKDirPath}/middleware/lwip/src/netif/ppp/pppcrypt.c"
"${SDKDirPath}/middleware/lwip/src/netif/ppp/pppoe.c"
"${SDKDirPath}/middleware/lwip/src/netif/ppp/pppol2tp.c"
"${SDKDirPath}/middleware/lwip/src/netif/ppp/pppos.c"
"${SDKDirPath}/middleware/lwip/src/netif/ppp/upap.c"
"${SDKDirPath}/middleware/lwip/src/netif/ppp/utils.c"
"${SDKDirPath}/middleware/lwip/src/netif/ppp/vj.c"
#"${SDKDirPath}/middleware/lwip/src/apps/sntp/sntp.c"
"${SDKDirPath}/middleware/mbedtls/library/aes.c"
"${SDKDirPath}/middleware/mbedtls/library/aesni.c"
"${SDKDirPath}/middleware/mbedtls/library/arc4.c"
"${SDKDirPath}/middleware/mbedtls/library/asn1parse.c"
"${SDKDirPath}/middleware/mbedtls/library/asn1write.c"
"${SDKDirPath}/middleware/mbedtls/library/base64.c"
"${SDKDirPath}/middleware/mbedtls/library/bignum.c"
"${SDKDirPath}/middleware/mbedtls/library/blowfish.c"
"${SDKDirPath}/middleware/mbedtls/library/camellia.c"
"${SDKDirPath}/middleware/mbedtls/library/ccm.c"
"${SDKDirPath}/middleware/mbedtls/library/certs.c"
"${SDKDirPath}/middleware/mbedtls/library/cipher.c"
"${SDKDirPath}/middleware/mbedtls/library/cipher_wrap.c"
"${SDKDirPath}/middleware/mbedtls/library/cmac.c"
"${SDKDirPath}/middleware/mbedtls/library/ctr_drbg.c"
"${SDKDirPath}/middleware/mbedtls/library/debug.c"
"${SDKDirPath}/middleware/mbedtls/library/des.c"
"${SDKDirPath}/middleware/mbedtls/library/dhm.c"
"${SDKDirPath}/middleware/mbedtls/library/ecdh.c"
"${SDKDirPath}/middleware/mbedtls/library/ecdsa.c"
"${SDKDirPath}/middleware/mbedtls/library/ecjpake.c"
"${SDKDirPath}/middleware/mbedtls/library/ecp.c"
"${SDKDirPath}/middleware/mbedtls/library/ecp_curves.c"
"${SDKDirPath}/middleware/mbedtls/library/entropy.c"
"${SDKDirPath}/middleware/mbedtls/library/entropy_poll.c"
"${SDKDirPath}/middleware/mbedtls/library/error.c"
"${SDKDirPath}/middleware/mbedtls/library/gcm.c"
"${SDKDirPath}/middleware/mbedtls/library/havege.c"
"${SDKDirPath}/middleware/mbedtls/library/hmac_drbg.c"
"${SDKDirPath}/middleware/mbedtls/library/md.c"
"${SDKDirPath}/middleware/mbedtls/library/md_wrap.c"
"${SDKDirPath}/middleware/mbedtls/library/md2.c"
"${SDKDirPath}/middleware/mbedtls/library/md4.c"
"${SDKDirPath}/middleware/mbedtls/library/md5.c"
"${SDKDirPath}/middleware/mbedtls/library/memory_buffer_alloc.c"
#"${SDKDirPath}/middleware/mbedtls/library/net_sockets.c"
"${SDKDirPath}/middleware/mbedtls/library/oid.c"
"${SDKDirPath}/middleware/mbedtls/library/padlock.c"
"${SDKDirPath}/middleware/mbedtls/library/pem.c"
"${SDKDirPath}/middleware/mbedtls/library/pk.c"
"${SDKDirPath}/middleware/mbedtls/library/pk_wrap.c"
"${SDKDirPath}/middleware/mbedtls/library/pkcs5.c"
"${SDKDirPath}/middleware/mbedtls/library/pkcs11.c"
"${SDKDirPath}/middleware/mbedtls/library/pkcs12.c"
"${SDKDirPath}/middleware/mbedtls/library/pkparse.c"
"${SDKDirPath}/middleware/mbedtls/library/pkwrite.c"
"${SDKDirPath}/middleware/mbedtls/library/platform.c"
"${SDKDirPath}/middleware/mbedtls/library/ripemd160.c"
"${SDKDirPath}/middleware/mbedtls/library/rsa.c"
"${SDKDirPath}/middleware/mbedtls/library/sha1.c"
"${SDKDirPath}/middleware/mbedtls/library/sha256.c"
"${SDKDirPath}/middleware/mbedtls/library/sha512.c"
"${SDKDirPath}/middleware/mbedtls/library/ssl_cache.c"
"${SDKDirPath}/middleware/mbedtls/library/ssl_ciphersuites.c"
"${SDKDirPath}/middleware/mbedtls/library/ssl_cli.c"
"${SDKDirPath}/middleware/mbedtls/library/ssl_cookie.c"
"${SDKDirPath}/middleware/mbedtls/library/ssl_srv.c"
"${SDKDirPath}/middleware/mbedtls/library/ssl_ticket.c"
"${SDKDirPath}/middleware/mbedtls/library/ssl_tls.c"
"${SDKDirPath}/middleware/mbedtls/library/threading.c"
"${SDKDirPath}/middleware/mbedtls/library/timing.c"
"${SDKDirPath}/middleware/mbedtls/library/version.c"
"${SDKDirPath}/middleware/mbedtls/library/version_features.c"
"${SDKDirPath}/middleware/mbedtls/library/x509.c"
"${SDKDirPath}/middleware/mbedtls/library/x509_create.c"
"${SDKDirPath}/middleware/mbedtls/library/x509_crl.c"
"${SDKDirPath}/middleware/mbedtls/library/x509_crt.c"
"${SDKDirPath}/middleware/mbedtls/library/x509_csr.c"
"${SDKDirPath}/middleware/mbedtls/library/x509write_crt.c"
"${SDKDirPath}/middleware/mbedtls/library/x509write_csr.c"
"${SDKDirPath}/middleware/mbedtls/library/xtea.c"
"${SDKDirPath}/middleware/mbedtls/port/ksdk/ksdk_mbedtls.c"
"${SDKDirPath}/middleware/mbedtls/port/ksdk/des_alt.c"
"${SDKDirPath}/rtos/freertos/Source/portable/MemMang/heap_5.c"
"${SDKDirPath}/rtos/freertos/FreeRTOS_for_Kinetis_SDK_v2.0.txt"
"${SDKDirPath}/rtos/freertos/License/license.txt"
"${SDKDirPath}/rtos/freertos/Source/include/stdint.readme"
"${SDKDirPath}/rtos/freertos/Source/croutine.c"
"${SDKDirPath}/rtos/freertos/Source/list.c"
"${SDKDirPath}/rtos/freertos/Source/queue.c"
"${SDKDirPath}/rtos/freertos/Source/readme.txt"
"${SDKDirPath}/rtos/freertos/Source/tasks.c"
"${SDKDirPath}/rtos/freertos/Source/timers.c"
"${SDKDirPath}/rtos/freertos/Source/event_groups.c"
"${SDKDirPath}/rtos/freertos/Source/portable/readme.txt"
"${SDKDirPath}/rtos/freertos/Source/portable/GCC/ARM_CM4F/fsl_tickless_systick.c"
"${SDKDirPath}/rtos/freertos/Source/portable/GCC/ARM_CM4F/port.c"

"${LibDirPath}/aws-iot-device-sdk-embedded-C/src/aws_iot_mqtt_client_yield.c"
"${LibDirPath}/aws-iot-device-sdk-embedded-C/src/aws_iot_mqtt_client_common_internal.c"
"${LibDirPath}/aws-iot-device-sdk-embedded-C/src/aws_iot_mqtt_client_subscribe.c"
"${LibDirPath}/aws-iot-device-sdk-embedded-C/src/aws_iot_mqtt_client.c"
"${LibDirPath}/aws-iot-device-sdk-embedded-C/src/aws_iot_shadow.c"
"${LibDirPath}/aws-iot-device-sdk-embedded-C/src/aws_iot_mqtt_client_unsubscribe.c"
"${LibDirPath}/aws-iot-device-sdk-embedded-C/src/aws_iot_mqtt_client_connect.c"
"${LibDirPath}/aws-iot-device-sdk-embedded-C/src/aws_iot_shadow_records.c"
"${LibDirPath}/aws-iot-device-sdk-embedded-C/src/aws_iot_shadow_actions.c"
"${LibDirPath}/aws-iot-device-sdk-embedded-C/src/aws_iot_shadow_json.c"
"${LibDirPath}/aws-iot-device-sdk-embedded-C/src/aws_iot_mqtt_client_publish.c"
"${LibDirPath}/aws-iot-device-sdk-embedded-C/src/aws_iot_json_utils.c"
"${LibDirPath}/aws-iot-device-sdk-embedded-C/external_libs/jsmn/jsmn.c"

"${SrcDirPath}/Applications/Common/APP_Init.c"
"${SrcDirPath}/Applications/Common/APP_Types.c"

"${SrcDirPath}/CloudComms/CLD_CommsInterface.c"
"${SrcDirPath}/CloudComms/AWS/AWS_CommsHandler.c"
"${SrcDirPath}/CloudComms/AWS/AWS_FaultBuffer.c"
"${SrcDirPath}/CloudComms/AWS/AWS_Timestamps.c"

"${SrcDirPath}/DeviceHandlers/Common/KVP_Api.c"
"${SrcDirPath}/DeviceHandlers/LEDHandler/LEDH_Api.c"
"${SrcDirPath}/DeviceHandlers/TestHandler/THA_Api.c"
"${SrcDirPath}/DeviceHandlers/UpgradeHandler/UPG_Api.c"
"${SrcDirPath}/DeviceHandlers/UpgradeHandler/RT1050/UPG_Upgrader.c"

"${SrcDirPath}/EnsoComms/ECOM_MessageBasedApi.c"

"${SrcDirPath}/HAL/RT1050/HAL.c"

"${SrcDirPath}/LocalShadow/Api/LSD_Api.c"
"${SrcDirPath}/LocalShadow/Api/LSD_Types.c"
"${SrcDirPath}/LocalShadow/ObjectStore/LSD_EnsoObjectStore.c"
"${SrcDirPath}/LocalShadow/ObjectStore/LSD_PropertyStore.c"
"${SrcDirPath}/LocalShadow/ObjectStore/LSD_Subscribe.c"

"${SrcDirPath}/Logger/LOG_Api.c"

"${SrcDirPath}/KeyStore/FreeRTOS/KEY_Api.c"

"${SrcDirPath}/OSAL/OSAL_Debug.c"
"${SrcDirPath}/OSAL/RT1050/OSAL_Api.c"
"${SrcDirPath}/OSAL/RT1050/board/pin_mux.c"
"${SrcDirPath}/OSAL/RT1050/board/board.c"
"${SrcDirPath}/OSAL/RT1050/board/clock_config.c"
"${SrcDirPath}/OSAL/RT1050/board/fsl_phy.c"
"${SrcDirPath}/OSAL/RT1050/efs/EFS_FileSystem.c"
"${SrcDirPath}/OSAL/RT1050/ota/ota.c"
"${SrcDirPath}/OSAL/RT1050/tinyprintf/tinyprintf.c"
"${SrcDirPath}/OSAL/RT1050/watchdog/watchdog.c"
"${SrcDirPath}/OSAL/RT1050/sntp/sntp.c"
"${SrcDirPath}/OSAL/RT1050/sntp/sntp_client.c"
"${SrcDirPath}/OSAL/RT1050/aws/platform/RT1050/common/timer.c"
"${SrcDirPath}/OSAL/RT1050/aws/platform/RT1050/mbedtls/network_mbedtls_wrapper.c"
"${SrcDirPath}/OSAL/RT1050/mbedtls/net_sockets.c"
"${SrcDirPath}/OSAL/RT1050/main/main.c"

"${SrcDirPath}/Storage/STO_Handler.c"
"${SrcDirPath}/Storage/STO_Manager.c"

"${SrcDirPath}/Watchdog/Watchdog.c"

"${ProjDirPath}/../EnsoMain.c"
"${ProjDirPath}/../AmebaWISAFESimulation/GW_Handler.c"
"${ProjDirPath}/../AmebaWISAFESimulation/SYS_Gateway.c"
)



set(CMAKE_EXE_LINKER_FLAGS_RAMDEBUG "${CMAKE_EXE_LINKER_FLAGS_RAMDEBUG} -T${ProjDirPath}/link_ramdebug.ld -static")
set(CMAKE_EXE_LINKER_FLAGS_COPYDOWN "${CMAKE_EXE_LINKER_FLAGS_COPYDOWN} -T${ProjDirPath}/link_copydown.ld -static")
set(CMAKE_EXE_LINKER_FLAGS_XIP "${CMAKE_EXE_LINKER_FLAGS_XIP} -T${ProjDirPath}/link_xip.ld -static")



TARGET_LINK_LIBRARIES(${ProjName}.elf -Wl,--start-group)
target_link_libraries(${ProjName}.elf optimized m)
target_link_libraries(${ProjName}.elf optimized c)
target_link_libraries(${ProjName}.elf optimized gcc)
target_link_libraries(${ProjName}.elf optimized nosys)
target_link_libraries(${ProjName}.elf optimized m)
target_link_libraries(${ProjName}.elf optimized c)
target_link_libraries(${ProjName}.elf optimized gcc)
target_link_libraries(${ProjName}.elf optimized nosys)
TARGET_LINK_LIBRARIES(${ProjName}.elf -Wl,--end-group)



ADD_CUSTOM_COMMAND(TARGET ${ProjName}.elf POST_BUILD
	COMMAND arm-none-eabi-size ${EXECUTABLE_OUTPUT_PATH}/${ProjName}.elf -B
)
ADD_CUSTOM_COMMAND(TARGET ${ProjName}.elf POST_BUILD
	COMMAND arm-none-eabi-size ${EXECUTABLE_OUTPUT_PATH}/${ProjName}.elf -A -x
)



IF(CMAKE_BUILD_TYPE MATCHES copydown OR CMAKE_BUILD_TYPE MATCHES xip)

	ADD_CUSTOM_COMMAND(TARGET ${ProjName}.elf POST_BUILD
		COMMAND ${CMAKE_OBJCOPY} -O srec ${EXECUTABLE_OUTPUT_PATH}/${ProjName}.elf ${EXECUTABLE_OUTPUT_PATH}/${ProjName}.srec
	)

ENDIF()
