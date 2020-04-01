/*
 * logic.h
 *
 *  Created on: 21 May 2018
 *      Author: abenrashed
 */

#ifndef _LOGIC_H_
#define _LOGIC_H_

#ifdef _LOGIC_C_
#undef _LOGIC_C_

#include "board.h"

volatile uint8_t PropagationCount;				// counts 3-second attempts to propagate
volatile uint8_t StatusRequestTargetSID;		// target of MSG_STATUSREQ rumour, if any
volatile uint8_t StatusReplyTargetSID;			// target of MSG_STATUSREPLY rumour, if any
volatile uint8_t MapRequestTargetSID;			// target of MSG_MAPREQ rumour, if any
volatile uint8_t MapReplyTargetSID;				// target of MSG_MAPREPLY rumour, if any
volatile uint8_t RumorByte5;					// fifth byte of rumour - might be rumour target of MSG_STATUSREQ or MSG_MAPREQ
volatile uint8_t RumorByte6;					// sixth byte of rumour - might be map type or Rev12 indicator

volatile uint8_t SDRumorTargetSID;				// target of next SD rumour, if any

volatile uint16_t ReasonsForRumor;				// bitmap of pending rumour reasons
volatile uint16_t ReasonForCurrentRumor;		// bit set shows reason for current rumour

volatile uint16_t ListenTimeLeft;				// number of ticks left to listen */

volatile uint8_t Random[3];						// random 24-bit Linear Feedback Shift Register changing every 3 seconds or so
volatile uint8_t MeshMagic[3];					// Mesh unique identifier
volatile uint8_t SID;							// our short ID */
volatile uint8_t MeshID;						// our mesh ID (or NULL_MID during join exchanges)
volatile uint8_t TrueMID;						// our true mesh ID */
volatile uint8_t SIDMap[8];						// one bit set for each used SID in our mesh */
volatile uint8_t NbrMap[8];						// one bit set for each neighbouring SID in our mesh */
volatile uint8_t RecentNbrMap0[8];				// one bit set for each SID we talked to during recent rumours
volatile uint8_t RecentNbrMap1[8];				// aged copy
volatile uint8_t RecentNbrMap2[8];				// doubly aged copy
volatile uint8_t RSSICutOff;					// minimum acceptable RSSI when sniffing neighbours
volatile uint8_t RSSITarget;					// node of interest when saving RSSI */
volatile uint8_t RumorMap[8];					// one bit set for each SID who hasn't heard rumour */
volatile uint8_t PropagateMap[8];				// one bit set for each node that we are willing to propagate to
volatile uint8_t NewSIDMap[8];					// used when updating SID Map
volatile uint8_t LurkMap[8];					// bit set for each overheard OK message when lurking

volatile uint8_t RumorBuf[MSGBUFLEN];			// holds rumour we might be spreading - either bytes or raw bits
volatile uint8_t RumorMsgLen;					// number of bits in RumorBuf
volatile uint8_t RumorBufMsg;					// type of message in RumorBuf
volatile uint8_t RumorBufInitiator;				// initiator of message in RumorBuf
volatile uint8_t RumorBufSeq;					// sequence number of message in RumorBuf

volatile uint16_t Minute;						// increments each minute
volatile uint16_t MinuteSysTime;				// value of SysTime last time when Minute last incremented
volatile uint16_t CheckNbrsMinute;				// Minute when last checked neighbours
volatile uint8_t CheckNbrsCount;				// counts number of times neighbours checked, up to some small limit value
volatile uint16_t CheckBatteryMinute;			// Minute when last checked battery
volatile uint16_t CheckSDMinute;				// Minute when last checked battery
volatile uint8_t ConfirmationState;				// determines how to handle incoming confirmations
volatile uint32_t ConfirmationTime;				// when sent out message that should generate a confirmation

volatile uint8_t RumorSequenceNumber;			// increments for each rumour we initiate
volatile uint8_t PreviousRSN;					// un-incremented RumorSequenceNumber

bool SomeNodeHasUrgentMsg;						// gets set in Listen if some node has urgent msg bit set

volatile uint8_t MapMsgType;					// determines type of map exchanged in MSG_MAPREQUEST and MSG_MAPREPLY messages

volatile uint8_t CriticalFaults;				// bit map of fault conditions
volatile uint8_t ReportedCriticalFaults;		// CriticalFaults reported to SD
volatile bool SentSDFault;						// whether we have told network that our SD has died

volatile bool BroadcastNbrMap;					// whether we should broadcast our NbrMap as a rumor

volatile uint8_t SimulatedButtonPressCount = 0;		// as if button has been pressed this many times


volatile uint8_t ShortChirpCount;				// increments each chirp
volatile uint8_t LocalRSSIValue;				// most recent RSSIValue for remote in our mesh
volatile uint8_t LocalRSSISID;					// SID of node whose RSSI is LocalRSSIValue

void Init_RM(void);
void SIDMapUpdate (volatile uint8_t *NewMap);
void SetNbrBit (uint8_t id, volatile uint8_t *map);
void NextRandom(void);
void StartRumor(uint8_t msg);
void StartReasonRumor(void);
bool AckRumorForUs(uint8_t id);
bool PropagationAccepted(uint8_t id);
void ProcessConfirmation(volatile uint8_t *map, uint8_t ttl);
void GenerateConfirmation(void);
void ClearMapsAndRumors(void);
void RM_MainLoop(void);
void ResetEEPROM(void);
void ResetEEPROM_MFCT(void);
void SetEEPROM_MFCT(void);
void SaveMeshInfo(void);
bool RestoreMeshInfo(void);
bool MeshMagicIsReserved(void);
bool TestEEPROM_MFCT(void);
void SaveFreqFrac(uint8_t freqfrac2, uint8_t freqfrac3);

#else

extern volatile uint8_t PropagationCount;				// counts 3-second attempts to propagate
extern volatile uint8_t StatusRequestTargetSID;		// target of MSG_STATUSREQ rumour, if any
extern volatile uint8_t StatusReplyTargetSID;			/* target of MSG_STATUSREPLY rumour, if any */
extern volatile uint8_t MapRequestTargetSID;			/* target of MSG_MAPREQ rumour, if any */
extern volatile uint8_t MapReplyTargetSID;				/* target of MSG_MAPREPLY rumour, if any */
extern volatile uint8_t RumorByte5;					/* fifth byte of rumour - might be rumour target of MSG_STATUSREQ or MSG_MAPREQ */
extern volatile uint8_t RumorByte6;					/* sixth byte of rumour - might be map type or Rev12 indicator */

extern volatile uint8_t SDRumorTargetSID;				/* target of next SD rumour, if any */

extern volatile uint16_t ReasonsForRumor;				/* bitmap of pending rumour reasons */
extern volatile uint16_t ReasonForCurrentRumor;		/* bit set shows reason for current rumour */

extern volatile uint16_t ListenTimeLeft;				/* number of ticks left to listen */

extern volatile uint8_t Random[3];						/* random 24-bit Linear Feedback Shift Register changing every 3 seconds or so */
extern volatile uint8_t MeshMagic[3];					/* Mesh unique identifier */
extern volatile uint8_t SID;							/* our short ID */
extern volatile uint8_t MeshID;						/* our mesh ID (or NULL_MID during join exchanges) */
extern volatile uint8_t TrueMID;						/* our true mesh ID */
extern volatile uint8_t SIDMap[8];						/* one bit set for each used SID in our mesh */
extern volatile uint8_t NbrMap[8];						/* one bit set for each neighbouring SID in our mesh */
extern volatile uint8_t RecentNbrMap0[8];				/* one bit set for each SID we talked to during recent rumours */
extern volatile uint8_t RecentNbrMap1[8];				/* aged copy */
extern volatile uint8_t RecentNbrMap2[8];				/* doubly aged copy */
extern volatile uint8_t RSSICutOff;					/* minimum acceptable RSSI when sniffing neighbours */
extern volatile uint8_t RSSITarget;					/* node of interest when saving RSSI */
extern volatile uint8_t RumorMap[8];					/* one bit set for each SID who hasn't heard rumour */
extern volatile uint8_t PropagateMap[8];				/* one bit set for each node that we are willing to propagate to */
extern volatile uint8_t NewSIDMap[8];					/* used when updating SID Map */
extern volatile uint8_t LurkMap[8];					/* bit set for each overheard OK message when lurking */
//volatile bool DoLurk;							/* true if we should lurk instead of sleep */

//volatile uint16_t ActCheckTime;					/* SysTime of most recent check of ActiveMap */

extern volatile uint8_t RumorBuf[];			/* holds rumour we might be spreading - either bytes or raw bits */
extern volatile uint8_t RumorMsgLen;					/* number of bits in RumorBuf */
extern volatile uint8_t RumorBufMsg;					/* type of message in RumorBuf */
extern volatile uint8_t RumorBufInitiator;				/* initiator of message in RumorBuf */
extern volatile uint8_t RumorBufSeq;					/* sequence number of message in RumorBuf */




extern volatile uint16_t Minute;						/* increments each minute */
extern volatile uint16_t MinuteSysTime;				/* value of SysTime last time when Minute last incremented */
extern volatile uint16_t CheckNbrsMinute;				/* Minute when last checked neighbours */
extern volatile uint8_t CheckNbrsCount;				/* counts number of times neighbours checked, up to some small limit value */
extern volatile uint16_t CheckBatteryMinute;			/* Minute when last checked battery */
extern volatile uint16_t CheckSDMinute;				/* Minute when last checked battery */
extern volatile uint8_t ConfirmationState;				/* determines how to handle incoming confirmations */
extern volatile uint32_t ConfirmationTime;				/* when sent out message that should generate a confirmation */

extern volatile uint8_t RumorSequenceNumber;			/* increments for each rumour we initiate */
extern volatile uint8_t PreviousRSN;					/* un-incremented RumorSequenceNumber */

extern bool SomeNodeHasUrgentMsg;						/* gets set in Listen if some node has urgent msg bit set */


extern volatile uint8_t MapMsgType;					/* determines type of map exchanged in MSG_MAPREQUEST and MSG_MAPREPLY messages */


extern volatile uint8_t CriticalFaults;				/* bit map of fault conditions */
extern volatile uint8_t ReportedCriticalFaults;		/* CriticalFaults reported to SD */
extern volatile bool SentSDFault;						/* whether we have told network that our SD has died */


extern volatile bool BroadcastNbrMap;					/* whether we should broadcast our NbrMap as a rumor */

extern volatile uint8_t SimulatedButtonPressCount;		/* as if button has been pressed this many times */

extern volatile uint8_t ShortChirpCount;				/* increments each chirp */
extern volatile uint8_t LocalRSSIValue;				/* most recent RSSIValue for remote in our mesh */
extern volatile uint8_t LocalRSSISID;					/* SID of node whose RSSI is LocalRSSIValue */


extern void Init_RM(void);
extern void SIDMapUpdate(volatile uint8_t *NewMap);
extern void SetNbrBit (uint8_t id, volatile uint8_t *map);
extern void NextRandom(void);
extern void StartRumor(uint8_t msg);
extern void StartReasonRumor(void);
extern bool AckRumorForUs(uint8_t id);
extern bool PropagationAccepted(uint8_t id);
extern void ProcessConfirmation(volatile uint8_t *map, uint8_t ttl);
extern void GenerateConfirmation(void);
extern void ClearMapsAndRumors(void);
extern void RM_MainLoop(void);
extern void ResetEEPROM(void);
extern void ResetEEPROM_MFCT(void);
extern void SetEEPROM_MFCT(void);
extern void SaveMeshInfo(void);
extern bool RestoreMeshInfo(void);
extern bool MeshMagicIsReserved(void);
extern bool TestEEPROM_MFCT(void);
extern void SaveFreqFrac(uint8_t freqfrac2, uint8_t freqfrac3);

#endif	// _LOGIC_C_

#endif // _LOGIC_H_
