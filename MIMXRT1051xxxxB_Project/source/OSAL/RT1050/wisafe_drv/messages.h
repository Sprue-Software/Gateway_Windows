/*
 * messages.h
 *
 *  Created on: 9 Apr 2018
 *      Author: abenrashed
 */

#ifndef _MESSAGES_H_
#define _MESSAGES_H_

#define MSG_LEN_TINY						0x00
#define MSG_LEN_SHORT						0x01
#define MSG_LEN_MEDIUM						0x02
#define MSG_LEN_LONG						0x03

#define SID_MASK							0x3F
#define ID_MASK								0x3F
#define MAP_DITHER							0xCC
#define DITHER								0xCC
#define FILLER								0x80
#define FWREV								0x00

#define MSG_JOINREQ							0x00
#define MSG_JOINACK							0x01
#define MSG_JOINED							0x22
#define MSG_CONFIRM							0x29
#define MSG_STATUSREQ						0x6B
#define MSG_STATUSREPLY						0x2C
#define MSG_MAPREQ							0x75
#define MSG_MAPREPLY						0x36
#define MSG_SD_RUMOR						0x2E
#define MSG_MFCT_STAMP						0x14
#define MSG_SIDMAP_UPDATE					0x77
#define MSG_PROPAGATE						0x04

#define MSG_ACKRUMOR						0x03

#define MM_XOR_REV12						0x93
#define MM_XOR_PROPAGATE_ACK				0x92
#define INVITING_BIT						0x80
#define URGENT_MSG_BIT						0x80

#define JOIN_REQUEST_MESSAGE_LEN			0x15	//21 bytes
#define RUMOR_MESSAGE_LEN					0x15	//21 bytes
#define JOIN_ACK_MESSAGE_LEN				0x10	//16 bytes not 17. First byte included as Sync.
#define PROPAGATE_ACK_MESSAGE_LEN			0x07	//7 bytes not 8. First byte included as Sync.
#define RUMOUR_ACK_MESSAGE_LEN				0x07
#define CHIRP_MESSAGE_LEN					0x04
#define CHIRP_RESPONSE_MESSAGE_LEN			0x02	//0x05
#define RUMORACK_MESSAGE_LEN				0x0C
#define OK_MESSAGE_LEN	 					0x08

#define NULL_SID							0x40	//64
#define NULL_MID 							0x00

#define VALID								0x01


#ifdef _MESSAGE_C_
#undef _MESSAGE_C_

#include "board.h"

void Init_Messages(void);
void UpdateSID(uint8_t newsid);
void UpdateKeys(uint8_t *keyptr);
void UpdateID(uint8_t *idptr);
uint8_t ReverseByte(uint8_t byte);
uint16_t GetCRC(void);
void CRCInit(void);
void CRCByte(uint8_t byte);
void BuildOkMsg(uint8_t *dataptr);
void BuildRumorAckMsg(uint8_t *dataptr);
void BuildJoinRequestMsg(uint8_t *dataptr);
void BuildChirpResponseMessage(uint8_t *dataptr);
void BuildStatusReplyMsg(void);
void BuildStatusRequestMsg(uint8_t targetsid);
void BuildMapReplyMsg(void);
void BuildMapRequestMsg(uint8_t targetsid);
void BuildSDRumorMsg(void);
void BuildPropagateMsg(uint8_t targetsid);
void BuildJoinedMsg(void);
void BuildSIDMapUpdateMsg(void);
void BuildRumorConfirmationMsg (uint8_t ttl);
void BuildRumorMessage(uint8_t volatile *dataptr, uint8_t CurrentMessage);
void CopyMsgToRumor(void);
void CopyRumorToMsg(void);
void ClearMap(volatile uint8_t *map);
void CopyMap(volatile uint8_t *map, volatile uint8_t *source, volatile uint8_t dither);
void CopyMemory(volatile uint8_t *dest, volatile uint8_t *src, uint8_t len);
void SetBit(volatile uint8_t *map, uint8_t id);
void ClearBit(volatile uint8_t *map, uint8_t id);
bool TestBit(volatile uint8_t *map, uint8_t id);
uint8_t CountBits(volatile uint8_t *map);
uint8_t SeqField(uint8_t SeqTTL);
void PatchMsgWithOldNews (bool OldNews);

#else

extern void Init_Messages(void);
extern void UpdateSID(uint8_t newsid);
extern void UpdateKeys(uint8_t *keyptr);
extern void UpdateID(uint8_t *idptr);
extern uint8_t ReverseByte(uint8_t byte);
extern uint16_t GetCRC(void);
extern void CRCInit(void);
extern void CRCByte(uint8_t byte);
extern void BuildOkMsg(uint8_t *dataptr);
extern void BuildRumorAckMsg(uint8_t *dataptr);
extern void BuildJoinRequestMsg(uint8_t *dataptr);
extern void BuildChirpResponseMessage(uint8_t *dataptr);
extern void BuildStatusReplyMsg(void);
extern void BuildStatusRequestMsg(uint8_t targetsid);
extern void BuildMapReplyMsg(void);
extern void BuildMapRequestMsg(uint8_t targetsid);
extern void BuildSDRumorMsg(void);
extern void BuildPropagateMsg(uint8_t targetsid);
extern void BuildJoinedMsg(void);
extern void BuildSIDMapUpdateMsg(void);
extern void BuildRumorConfirmationMsg (uint8_t ttl);
extern void BuildRumorMessage(uint8_t volatile *dataptr, uint8_t CurrentMessage);
extern void CopyMsgToRumor(void);
extern void CopyRumorToMsg(void);
extern void ClearMap(volatile uint8_t *map);
extern void CopyMap(volatile uint8_t *map, volatile uint8_t *source, volatile uint8_t dither);
extern void CopyMemory(volatile uint8_t *dest, volatile uint8_t *src, uint8_t len);
extern void SetBit(volatile uint8_t *map, uint8_t id);
extern void ClearBit(volatile uint8_t *map, uint8_t id);
extern bool TestBit(volatile uint8_t *map, uint8_t id);
extern uint8_t CountBits(volatile uint8_t *map);
extern uint8_t SeqField(uint8_t SeqTTL);
extern void PatchMsgWithOldNews (bool OldNews);


#endif // _MESSAGE_C_

#endif // _MESSAGES_H_
