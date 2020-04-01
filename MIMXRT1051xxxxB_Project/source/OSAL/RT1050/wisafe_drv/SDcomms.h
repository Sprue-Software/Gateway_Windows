/*
 * SDcomms.h
 *
 *  Created on: 29 May 2018
 *      Author: abenrashed
 */

#ifndef _SDCOMMS_H_
#define _SDCOMMS_H_

#define SPI_FILL_OUT 		0x00
#define SPI_FILL_IN 		0x00
#define FLAG 				0x7E
#define ESC 				0x7D


#ifdef _SDCOMMS_C_

#include "board.h"

volatile uint8_t SDMsgRcvd = SPIMSG_NULL;					// first byte of incoming message
volatile uint8_t ExtendedArg1;				// second byte of incoming SPIMSG_RM_EXTENDED_REQ message
volatile uint8_t ExtendedArg2;				// third byte of incoming SPIMSG_RM_EXTENDED_REQ message
volatile uint8_t ExtendedArg3;				// fourth byte of incoming SPIMSG_RM_EXTENDED_REQ message
volatile uint8_t ExtendedMsgLen;			// incoming message len
volatile bool SDAlarmIdentValid;
volatile uint8_t SDModel[2];

uint8_t GateWayID[3];			// alarm identity reported by SD in ALARM_IDENT message
volatile uint16_t StatusRequestTime;		// time we last sent Status message to SD

volatile uint8_t IncomingMsg[SPIMSGSIZE];	// messages from SD to RM arrive here
volatile uint8_t IncomingMsgLen;			// index into IncomingMsg
volatile bool IncomingEscapeState=false;	// nonzero means an escape byte just received
volatile uint8_t OutgoingMsg[SPIMSGSIZE];	// messages from RM to SD arrive here
uint8_t OutgoingMsgRaw[SPIMSGSIZE];
uint8_t IncomingMsgRaw[SPIMSGSIZE];
volatile uint8_t OutgoingMsgLen;			// index into OutgoingMsg

volatile uint8_t PassThruMsg[SPIMSGSIZE];	// messages from RM to SD arrive here
volatile uint8_t PassThruMsgLen;			// index into OutgoingMsg


void Init_Uart(void);
void TestUart(void);
void SendOutgoingMsg (void);
void SendStatusRequestMsg(void);
void SendAckMsg(void);
void SendNackMsg(void);
void SendDiagResultMsg(void);
void SendRMIdResponseMsg(void);
void SendRMExtendedResponseMsg(void);
void SendRMProdTstResponseMsg(void);
void ProcessIncomingMsgRaw(uint8_t *ptr);

#else

extern volatile uint8_t SDMsgRcvd;					// first byte of incoming message
extern volatile uint8_t ExtendedArg1;				// second byte of incoming SPIMSG_RM_EXTENDED_REQ message
extern volatile uint8_t ExtendedArg2;				// third byte of incoming SPIMSG_RM_EXTENDED_REQ message
extern volatile uint8_t ExtendedArg3;				// fourth byte of incoming SPIMSG_RM_EXTENDED_REQ message
extern volatile uint8_t ExtendedMsgLen;			// incoming message len
extern volatile bool SDAlarmIdentValid;
extern volatile uint8_t SDModel[2];

extern uint8_t GateWayID[3];			// alarm identity reported by SD in ALARM_IDENT message
extern volatile uint16_t StatusRequestTime;		// time we last sent Status message to SD

extern volatile uint8_t IncomingMsg[];	// messages from SD to RM arrive here
extern volatile uint8_t IncomingMsgLen;			// index into IncomingMsg
extern volatile bool IncomingEscapeState;			// nonzero means an escape byte just received
extern volatile uint8_t OutgoingMsg[];	// messages from RM to SD arrive here
extern uint8_t OutgoingMsgRaw[];
extern uint8_t IncomingMsgRaw[];
extern volatile uint8_t OutgoingMsgLen;			// index into OutgoingMsg

extern volatile uint8_t PassThruMsg[];	// messages from RM to SD arrive here
extern volatile uint8_t PassThruMsgLen;			// index into OutgoingMsg


extern void TestUart(void);
extern void Init_Uart(void);
extern void SendOutgoingMsg (void);
extern void SendStatusRequestMsg(void);
extern void SendAckMsg(void);
extern void SendNackMsg(void);
extern void SendDiagResultMsg(void);
extern void SendRMIdResponseMsg(void);
extern void SendRMExtendedResponseMsg(void);
extern void SendRMProdTstResponseMsg(void);
extern void ProcessIncomingMsgRaw(uint8_t *ptr);

#endif // _SDCOMMS_C_

#endif // _SDCOMMS_H_
