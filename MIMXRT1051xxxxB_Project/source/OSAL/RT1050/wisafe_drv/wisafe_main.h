/*
 * wisafe_main.h
 *
 *  Created on: 21 May 2018
 *      Author: abenrashed
 */

#include <stdint.h>
#include <stdbool.h>

#ifndef _MAIN_H_
#define _MAIN_H_

#define MFCT_STAMP0 				0x67
#define MFCT_STAMP1					0x89

#define FREQ_FRAC_KEY0				29
#define FREQ_FRAC_KEY1				13

#define RESERVED_MM0 				0x99
#define RESERVED_MM1 				0x99
#define RESERVED_MM2 				0x99

#define SPIMSGSIZE 					16

#define DEFAULT_TTL 				5
#define LISTEN_DURATION 			26
#define PROPAGATION_COUNT_LIMIT 	5
#define CHECK_NBRS_SHORT_INTERVAL 	60
#define CHECK_NBRS_LONG_INTERVAL 	1440
#define SHORT_CHIRP_COUNT 			29


#define RCVBUFLEN 					20
#define MSGBUFLEN 					24
#define PASSBUFLEN					16
#define MAX_MESH_SIZE 				50

#define REASON_SD_RUMOR 			0x0008
#define REASON_CHECK_NBRS 			0x0010
#define REASON_JOINED 				0x0020
#define REASON_STATUS_REPLY 		0x0040
#define REASON_STATUS_REQUEST 		0x0080
#define REASON_PROPAGATE_RUMOR 		0x0100
#define REASON_SPREAD_RUMOR 		0x0200
#define REASON_MAP_REPLY 			0x0800
#define REASON_MAP_REQUEST 			0x1000
#define REASON_SIDMAP_UPDATE 		0x2000
#define REASON_SNIFF_NBRS 			0x4000


#define SPIMSG_NULL 				0
#define SPIMSG_TRANSPARENT 			0xFF

#define SPIMSG_STATUS 				0x41
#define SPIMSG_ACK 					0x46
#define SPIMSG_NACK 				0x47
#define SPIMSG_UNIT_TEST 			0x70
#define SPIMSG_ALARM_IDENT 			0x91
#define SPIMSG_DO_EXDIG_ID 			0xC3
#define SPIMSG_RM_DIAG_REQ 			0xD1
#define SPIMSG_RM_DIAG_RESULT 		0xD2
#define SPIMSG_RM_EXTENDED_REQ 		0xD3
#define SPIMSG_RM_EXTENDED_RESPONSE 0xD4
#define SPIMSG_RM_SD_FAULT 			0xD5
#define SPIMSG_RM_CLEAR_CMD 		0xE2
#define SPIMSG_RM_RESET_CMD 		0xE5
#define SPIMSG_RM_ID_REQ 			0xE6
#define SPIMSG_RM_ID_RESPONSE 		0xE7
#define SPIMSG_RM_PROD_TST_CMD 		0xE8
#define SPIMSG_RM_PROD_TST_RESPONSE 0xE9
#define SPIMSG_RM_INC_FREQ			0x49	// 'I'
#define SPIMSG_RM_FINE_INC_FREQ		0x48
#define SPIMSG_RM_DEC_FREQ			0x44	// 'D'
#define SPIMSG_RM_FINE_DEC_FREQ		0x43
#define SPIMSG_RM_EXIT_FREQ			0x45	// 'E'
#define SPIMSG_RM_SAVE_FREQ			0x53	// 'S'
#define SPIMSG_RM_MFCT_MODE			0xEA
#define SPIMSG_RM_OPER_MODE			0xEB
#define SPIMSG_INVALID 				0xFF

#define SPIMSG_EXTD_OPT_SIDMAP 			0x03
#define SPIMSG_EXTD_OPT_SIDMAP_UPDATE 	0x04
#define SPIMSG_EXTD_OPT_REMOTE_STATUS 	0x06
#define SPIMSG_EXTD_OPT_NBR_CHECK 		0x10
#define SPIMSG_EXTD_OPT_REMOTE_MAP 		0x11
#define SPIMSG_EXTD_OPT_BUTTON_PRESS 	0x12
#define SPIMSG_EXTD_OPT_SET_CONFIG 		0x14
#define SPIMSG_EXTD_OPT_GET_CONFIGS 	0x15
#define SPIMSG_EXTD_OPT_INIT_CRC 		0x16
#define SPIMSG_EXTD_OPT_LURKMAP 		0x17
#define SPIMSG_EXTD_OPT_SET_LURK_MODE 	0x18
#define SPIMSG_EXTD_OPT_RSSI_CUT_OFF 	0x19
#define SPIMSG_EXTD_OPT_SD_RUMOR_TARGET 0x1A

#define SPIMSG_EXTD_OPT_MISSING_MAP 	0x01
#define SPIMSG_EXTD_OPT_REMOTE_ID 		0x09

#define MSG_RUMOR_BIT 					0x20
#define MSG_CONFIRM_BIT 				0x40

#define OLD_NEWS_BIT 					0x40

#define MSG_OFFSET_MAP 					7

#define CONFIRMATION_TIMEOUT 			2400	//(8*60*5)
#define MINUTE_SYS_TIME_COUNT			480

#define LEARN_BUTTON_PRESS_MASK			0x80
#define CMD_UNLEARN				0x65
#define CMD_LEARN				0x01

typedef enum _learn_state_type {
    LEARN_INACTIVE,
    LEARN_ACTIVE,
    LEARN_JOINED,
    LEARN_UNLEARNT
} learn_state_type_t;

extern uint32_t getWisafeMainLoopCount(void);
extern learn_state_type_t getWisafeButtonStatus(void);
extern learn_state_type_t getWisafeLearnInStatus(void);
extern void resetWisafeLearnInStatus(void);
extern void writeWGqueue(uint8_t *message);
extern void writeRMqueue(void);
extern void checkWGtoRMqueue(void);
extern void set_syncMatchFlag(void);
extern bool wait_syncMatch(uint32_t timeout);
#endif /* MAIN_H_ */
