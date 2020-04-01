/*******************************************************************************
 * \file	logic.c
 * \brief 	Wisafe-2 main loop of handling received and transmitted messages.
 * 			Chirping to receive messages and listening to chirps to transmit
 * 			messages..
 * \note 	Project: 	WG2. Low cost wireless gateway
 * \author  Abdul Ben-Rashed
 ******************************************************************************/

#define _LOGIC_C_

//#include "wisafe_main.h"
#include "fsl_gpio.h"
#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "logic.h"
#include <timer.h>
#include "messages.h"
#include "radio.h"
#include "SDcomms.h"
#include "filesystem.h"
#include "HAL.h"
#include "LOG_Api.h"

static learn_state_type_t learnInStatus = LEARN_INACTIVE;
static uint32_t wisafeMainLoopCount = 0;

/*******************************************************************************
 * \brief	Get Wisafe main loop count. Monitored by watchdog
 *
 * \param	None.
 * \return	count - number of times RM_MainLoop has been called
 ******************************************************************************/

uint32_t getWisafeMainLoopCount(void)
{
    return wisafeMainLoopCount;
}


/*******************************************************************************
 * \brief	Get Wisafe learn-in state
 *
 * \param	None.
 * \return	None.
 ******************************************************************************/

learn_state_type_t getWisafeLearnInStatus(void)
{
    return learnInStatus;
}


/*******************************************************************************
 * \brief	Reset Wisafe learn-in state to inactive
 *
 * \param	None.
 * \return	None.
 ******************************************************************************/

void resetWisafeLearnInStatus(void)
{
    learnInStatus = LEARN_INACTIVE;
}


/*******************************************************************************
 * \brief	Initialise Wisafe-2 configuration.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

void Init_RM(void)
{
	LocalRSSISID = NULL_SID;								// no RSSI remote
	SDRumorTargetSID = NULL_SID;							// no specified target for next SD rumour */

	// try to recover saved mesh info in EEPROM
	if(RestoreMeshInfo())
	{
#if SCRAMBLED_WISAFE
		if(SID!=NULL_SID)
		{
			// ABR Scramble
			Scramble0 = MeshKey[1];
			//ReasonsForRumor |= REASON_CHECK_NBRS;			// need to recover neighbour information
		}
		else
		{
			Scramble0 = 0;
		}
#else
		if(SID!=NULL_SID)
		{
			//ReasonsForRumor |= REASON_CHECK_NBRS;			// need to recover neighbour information
		}
#endif
	}
	else
	{
		// no mesh info stored in EEPROM
		ResetEEPROM();										// write all bits in EEPROM connection map
		SID = NULL_SID;										// unassigned SID
		NetworkMeshID = NULL_MID;
		TrueMID = NetworkMeshID;
		MeshKey[0] = 0;										// default magic number for NULLMID
		MeshKey[1] = 0;
		MeshKey[2] = 0;
#if SCRAMBLED_WISAFE
		// ABR Scramble
		Scramble0 = 0;
#endif
		ClearMap(SIDMap);									// nobody in mesh yet
		//PRINTF("False data");
	}

	// create a random seed
	//Not use in WG2. but will be in Gen5
	//Random[0] = rand()%256;
	//Random[1] = rand()%256;
	//Random[2] = rand()%256;

	SysTime = 0;											// forget any time spent during initialisation

	UpdateKeys(&MeshKey[0]);
	LongMsgFirstByte = (NetworkMeshID<<2)+MSG_LEN_LONG;
	MediumMsgFirstByte = (NetworkMeshID<<2)+MSG_LEN_MEDIUM;

	if((RMCONFIG.FreqFracKey0 == FREQ_FRAC_KEY0) && (RMCONFIG.FreqFracKey1 == FREQ_FRAC_KEY1))
	{
		SetRadioProperty (FREQ_CONTROL_FRAC_GROUP, FREQ_CONTROL_FRAC_INDEX2, RMCONFIG.FreqFrac2);
		SetRadioProperty (FREQ_CONTROL_FRAC_GROUP, FREQ_CONTROL_FRAC_INDEX3, RMCONFIG.FreqFrac3);
	}
}



/*******************************************************************************
 * \brief	Updates the SID map.
 *
 * \note	Nodes cannot be added here.
 * \param   *NewMap. Pointer to the new SID map.
 * \return	None.
 ******************************************************************************/

void SIDMapUpdate (volatile uint8_t *NewMap)
{
	uint8_t OurSID;
	bool NoMap;

	NoMap = true;

	// did we just get kicked out of the mesh?
	if (!TestBit(NewMap,SID))
	{
		// we've been terminated! clear EEPROM and reset
		ResetEEPROM();							// this will wipe key indicating no mesh info saved
		SID = NULL_SID;							// unassigned SID
		NetworkMeshID = NULL_MID;
		TrueMID = NetworkMeshID;
		MeshKey[0] = 0;							// default magic number for NULLMID
		MeshKey[1] = 0;
		MeshKey[2] = 0;
		ClearMap(SIDMap);						// nobody in mesh yet

		UpdateKeys(&MeshKey[0]);
		LongMsgFirstByte = (NetworkMeshID<<2)+MSG_LEN_LONG;
		MediumMsgFirstByte = (NetworkMeshID<<2)+MSG_LEN_MEDIUM;
#if SCRAMBLED_WISAFE
		Scramble0 = 0;
#endif
		learnInStatus = LEARN_UNLEARNT;

		// tell SD something...
		//SendTerminatedMsg();					// Send to SD SID Map report with no members in mesh
	}

	// for each deleted node, remove records of its existence
	for (OurSID = 0;  OurSID<MAX_MESH_SIZE;  OurSID++)
	{
		if (TestBit(SIDMap,OurSID) && (!TestBit(NewMap,OurSID)))
		{
			// Our SID is getting kicked out!
			ClearBit(SIDMap,OurSID);			// make sure existence erased
			ClearBit(NbrMap,OurSID);			// make sure existence as neighbour erased
		}

		if ( (OurSID != SID) && (TestBit(NewMap,OurSID)) )
		{
			NoMap = false;
		}
	}

	if(NoMap)
	{
		ResetEEPROM();
		SID = NULL_SID;
		NetworkMeshID = NULL_MID;
		TrueMID = NetworkMeshID;
		MeshKey[0] = 0;
		MeshKey[1] = 0;
		MeshKey[2] = 0;
		ClearMap(SIDMap);

		UpdateKeys(&MeshKey[0]);
		LongMsgFirstByte = (NetworkMeshID<<2)+MSG_LEN_LONG;
		MediumMsgFirstByte = (NetworkMeshID<<2)+MSG_LEN_MEDIUM;
#if SCRAMBLED_WISAFE
		Scramble0 = 0;
#endif
		learnInStatus = LEARN_UNLEARNT;
	}

	SaveMeshInfo();								// in case we reset, we can recover with new node
}



/*******************************************************************************
 * \brief	Updates the "Nbr" map.
 *
 * \param   id. SID of the new neighbour.
 * \param   *map. Pointer to the Neighbour map "Nbr".
 * \return	None.
 ******************************************************************************/

void SetNbrBit (uint8_t id, volatile uint8_t *map)
{
	if(id<MAX_MESH_SIZE)
	{
		if(TestBit(SIDMap,id)!=0)
		{
			// we have an OKMSG from a node in our mesh
			SetBit(map,id);						// take note of who we are hearing from
		}
	}
}



/*******************************************************************************
 * \brief	Lurks to detect neighbours.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

/*
void Lurk(void)
{
	uint8_t id;
	uint32_t LurkTime, LurkTimeLeft;
	uint32_t StartTime, CurrentTime;


	LurkTime = ONE_SECOND;

	// Dither by randomly lurking one more tick
	NextRandom();											// generate new random bit
	LurkTime += ((Random[0]&1)*FIFTY_MILLISECOND);

	StartTime = GetCurrentTimerCount();
	CurrentTime = StartTime;
	while( (CurrentTime-StartTime)<LurkTime )
	{
		LurkTimeLeft = LurkTime - (CurrentTime-StartTime);
		if(LurkTimeLeft>LurkTime)							//In case it happens when timer counter overflows
		{
			LurkTimeLeft = 0;
		}
		id = ReceiveOkMessage(LurkTimeLeft);				// sender of received message
		if(id != NULL_SID)
		{
			SetNbrBit(id,LurkMap);							// record id as neighbour
		}
		CurrentTime = GetCurrentTimerCount();
	}

}
*/



/*******************************************************************************
 * \brief	Updates Random[] parameters.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

void NextRandom(void)
{
	// maximum length 24-bit LFSR
	// compute new random bit in lsb of Random[0] as xor of taps 24, 23, 22, and 17
	// (See Xilinx app note XAPP 052, referenced by Wikipedia)
	// shift left other 23 bits
	// additional XOR with 24-bit value

	uint8_t BitCnt;

	BitCnt = 1;	// count taps with set bits

	if ((Random[2] & 0x80)!=0)	// tap 24
	{
		BitCnt++;
	}
	if ((Random[2] & 0x40)!=0)	// tap 23
	{
		BitCnt++;
	}
	if ((Random[2] & 0x20)!=0)	// tap 22
	{
		BitCnt++;
	}
	if ((Random[2] & 0x01)!=0)	// tap 17
	{
		BitCnt++;
	}

	// lsb of BitCnt is now result of xoring taps
	Random[2] <<= 1;			// shift towards msb
	if (Random[1]&0x80)
	{
		Random[2] += 1;
	}

	Random[2] ^= 0x12;			// break up bursty runs

	Random[1] <<= 1;			// shift towards msb
	if (Random[0]&0x80)
	{
		Random[1]++;
	}

	Random[1] ^= 0x34;			// break up bursty runs

	Random[0] <<= 1;			// shift towards msb
	if (BitCnt&0x01)
	{
		Random[0]++;
	}

	Random[0] ^= 0x56;			// break up bursty runs
}



/*******************************************************************************
 * \brief	Starts the rumour.
 *
 * \param   msg. Rumour message byte (type).
 * \return	None.
 ******************************************************************************/

void StartRumor(uint8_t msg)
{
	CopyMsgToRumor();						// save raw message in RumorBuf and length in RumorMsgLen
	RumorTTL = DEFAULT_TTL;					// new rumour has lots of time to live
	RumorBufMsg = msg;						// record message byte
	RumorBufSeq = RumorSequenceNumber++;	// note - should build rumour first so that increment is after use
	RumorBufInitiator = SID;				// we are starting this rumour
}



/*******************************************************************************
 * \brief	Starts reason rumour.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

void StartReasonRumor(void)
{
	/* build rumour based on reasons in order of priority */
	if((ReasonsForRumor&REASON_PROPAGATE_RUMOR)!=0)
	{
		// rumour is already in RumorBuf and RumorMap already set
		ReasonForCurrentRumor = REASON_PROPAGATE_RUMOR;
		CopyRumorToMsg();											// make sure rumour also in msgbuf
	}
	else if((ReasonsForRumor&REASON_SPREAD_RUMOR)!=0)
	{
		// rumour is already in RumorBuf and RumorMap already set
		ReasonForCurrentRumor = REASON_SPREAD_RUMOR;
		CopyRumorToMsg();											// make sure rumour also in msgbuf
	}
	else
	{
		// we are starting a new rumour
		// assume rumour is targeted at all other nodes
		CopyMap(RumorMap,SIDMap,0);									// these all need to be told
		ClearBit(RumorMap,SID);										// except for us of course

		if((ReasonsForRumor&REASON_STATUS_REPLY)!=0)
		{
			ReasonForCurrentRumor = REASON_STATUS_REPLY;
			BuildStatusReplyMsg();									// this is the message to spread
			ClearMap(RumorMap);
			SetBit(RumorMap,StatusReplyTargetSID);					// only one target
			StartRumor(MSG_STATUSREPLY);

			// also perform a neighbour check too!
			//ReasonsForRumor |= REASON_CHECK_NBRS;					// good time to check neighbours
		}
		else if((ReasonsForRumor&REASON_STATUS_REQUEST)!=0)
		{
			/*
			ReasonForCurrentRumor = REASON_STATUS_REQUEST;
			BuildStatusRequestMsg(StatusRequestTargetSID);			// request status of remote SID (w/confirmation if fails)
			ClearMap(RumorMap);
			SetBit(RumorMap,StatusRequestTargetSID);				// target SID
			StartRumor(MSG_STATUSREQ);
			// set up confirmation handling
			ConfirmationState = MSG_STATUSREQ;
			ConfirmationTime = SysTime;
			*/
			ReasonForCurrentRumor = REASON_STATUS_REQUEST;
			BuildStatusRequestMsg(StatusRequestTargetSID);			// request status of remote SID (w/confirmation if fails)			
			ClearMap(RumorMap);
			SetBit(RumorMap,StatusRequestTargetSID);				// target SID			
			StartRumor(MSG_STATUSREQ);
			if(MapMsgType==0)
			{				
				// set up confirmation handling
				ConfirmationState = MSG_STATUSREQ;
				ConfirmationTime = SysTime;
			}			
		}
		else if((ReasonsForRumor&REASON_MAP_REPLY)!=0)
		{
			ReasonForCurrentRumor = REASON_MAP_REPLY;
			BuildMapReplyMsg();										// this is message to spread
			ClearMap(RumorMap);
			SetBit(RumorMap,MapReplyTargetSID);						// only one target
			StartRumor(MSG_MAPREPLY);
		}
		else if((ReasonsForRumor&REASON_MAP_REQUEST)!=0)
		{
			ReasonForCurrentRumor = REASON_MAP_REQUEST;
			BuildMapRequestMsg(MapRequestTargetSID);				// request status of remote SID (w/confirmation if fails)
			ClearMap(RumorMap);
			SetBit(RumorMap,MapRequestTargetSID);					// target SID
			StartRumor(MSG_MAPREQ);
		}
		else if((ReasonsForRumor&REASON_SD_RUMOR)!=0)
		{
			ReasonForCurrentRumor = REASON_SD_RUMOR;
			// don't clear PassThruMsgLen here because we may have to rebuild rumour
			BuildSDRumorMsg();										// this is message to spread
			CopyMsgToRumor();										// save raw message in RumorBuf and length in RumorMsgLen

			if(SDRumorTargetSID!=NULL_SID)
			{
				// SD has previously specified a single target for the next rumour - which is now!
				ClearMap(RumorMap);
				SetBit(RumorMap,SDRumorTargetSID);					// target SID
				SDRumorTargetSID = NULL_SID;						// now clear it as is only good once.
			}

			StartRumor(MSG_SD_RUMOR);
		}
		else if((ReasonsForRumor&(REASON_CHECK_NBRS+REASON_SNIFF_NBRS))!=0)
		{
			if((ReasonsForRumor&REASON_CHECK_NBRS)!=0)
			{
				ReasonForCurrentRumor = REASON_CHECK_NBRS;
			}
			else
			{
				ReasonForCurrentRumor = REASON_SNIFF_NBRS;
			}
			BuildJoinedMsg();										// this is message to spread
			ClearMap(RumorMap);										// nobody is target for this rumour!
			SetBit(RumorMap,SID);									// set at least one bit so rumour attempt lasts full 3 seconds
			StartRumor(MSG_JOINED);									// sets RumorTTL to DEFAULT_TTL, but this rumour won't propagate
		}
		else if((ReasonsForRumor&REASON_JOINED)!=0)
		{
			// NOTE - REASON_JOINED must be lower priority than REASON_CHECK_NBRS, since both set when new node joins
			ReasonForCurrentRumor = REASON_JOINED;
			BuildJoinedMsg();										// this is message to spread
			StartRumor(MSG_JOINED);

			// we are about to broadcast our NbrMap so other nodes can update their connections tables
			// therefore we must update our connection table to match
			//UpdateConnections(SID,NbrMap);						// we are connected to our neighbours
		}
		else if((ReasonsForRumor&REASON_SIDMAP_UPDATE)!=0)
		{
			ReasonForCurrentRumor = REASON_SIDMAP_UPDATE;
			BuildSIDMapUpdateMsg();									// our SIDMap has already been updated - now we share our version with all others
			StartRumor(MSG_SIDMAP_UPDATE);
			// set up confirmation handling
			ConfirmationState = MSG_SIDMAP_UPDATE;
			ConfirmationTime = SysTime;
		}
	}

}



/*******************************************************************************
 * \brief	Checks if rumour is for us.
 *
 * \param   id. SID of rumour acknowledging node.
 * \return	True, if ACK for us. False, if ACK is for some one else.
 *******************************************************************************/

bool AckRumorForUs(uint8_t id)
{
	bool success;

	success = false;												// assume ack not for us

 	if(((rcvbuf[1]&SID_MASK)==id) && (rcvbuf[2]==MSG_ACKRUMOR))
	{
		if(rcvbuf[5]==(MeshKey[2]^MM_XOR_REV12))
		{
			// other node is newer than Rev. 1.0/1.1
			if((rcvbuf[3]&SID_MASK)==SID)
			{
				success = true;										// ack aimed at us!
			}
			else
			{
				success = false;									// ack aimed at someone else!
			}
		}
		else
		{
			success = true;											// with Rev. 1.0/1.1, cannot confirm who ack is aimed at
		}
	}

	return success;
}



/*******************************************************************************
 * \brief	Checks propagation ACK.
 *
 * \param   id. SID of propagation acknowledging node.
 * \return	True, if ACK for us. False, if ACK is for some one else.
 ******************************************************************************/

bool PropagationAccepted(uint8_t id)
{
	bool success;

	success = false;					// assume failure to receive ack

	if(ReceivePropagateAckMessage(id))
	{
		// received a message from expected remote
		if(rcvbuf[2]==MSG_ACKRUMOR)
		{
			// it was the expected ack
			if(rcvbuf[5]==(MeshKey[2]^MM_XOR_PROPAGATE_ACK))
			{
				// other node is newer than Rev. 1.0/1.1
				if((rcvbuf[3]&SID_MASK)==SID)
				{
					success = true;		// is ack aimed at us!
				}
				else
				{
					success = false;	// is ack aimed at someone else!
				}

			}
			else
			{
				success = true;			// with Rev 1.0/1.1 cannot confirm who ack is aimed at
			}
		}
	}

	return success;
}



/*******************************************************************************
 * \brief	Starts reason rumour.
 *
 * \param   *map. Pointer of bitmap of nodes did not receive the rumour.
 * \param	ttl. Rumour time to live TTL.
 * \return	None.
 ******************************************************************************/

void ProcessConfirmation(volatile uint8_t *map, uint8_t ttl)
{
	//  generate message to tell SD about confirmation
	if((ConfirmationState==MSG_STATUSREQ) && (CountBits(map)==0))
	{
		// remote unit received status request - no point telling SD
		ConfirmationState = 0;					// no longer expecting a confirmation
	}

	if(ConfirmationState!=0)
	{
		if(OutgoingMsgLen==0)
		{
			OutgoingMsg[0] = SPIMSG_RM_EXTENDED_RESPONSE;
			OutgoingMsg[1] = SPIMSG_EXTD_OPT_MISSING_MAP;	// confirmation report
			OutgoingMsg[2] = ttl;							// tells how many hops needed
			CopyMap(&OutgoingMsg[3],map,0);					// 8 bytes
			OutgoingMsgLen = 11;
		}

		if(ConfirmationState==MSG_SIDMAP_UPDATE)
		{
			// but what if not all nodes heard about update?  not clear what to do!
			SIDMapUpdate(NewSIDMap);						// all other nodes already notified - now purge our records
		}

		// eat confirmation even if can't tell SD?  nothing else to do!
		ConfirmationState = 0;								// no longer expecting a confirmation
	}
	else
	{
		;													// unexpected confirmation!
	}

}



/*******************************************************************************
 * \brief	Generates confirmation.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

void GenerateConfirmation(void)
{
	if (RumorBufMsg&MSG_CONFIRM_BIT)
	{
		// rumour initiator wants confirmation
		if(RumorBufInitiator==SID)
		{
			// we are initiator!
			ProcessConfirmation(RumorMap,RumorTTL);				// map of un-contacted nodes
		}
		else
		{
			// start rumour targeted at rumour initiator and send it without stopping listen
			ReasonForCurrentRumor = REASON_SPREAD_RUMOR;
			ReasonsForRumor |= ReasonForCurrentRumor;
#if ENHANCED_DEBUG
			LOG_Info(LOG_YELLOW "Restart listen time");
#endif
			ListenTimeLeft = LISTEN_DURATION;					// restart 3 seconds of listening
			// create confirmation rumour
			BuildRumorConfirmationMsg(RumorTTL);				// this is message to spread
			ClearMap(RumorMap);									// only one required destination
			SetBit(RumorMap,RumorBufInitiator);					// namely, rumour initiator
			//not needed! RumorTargetSID = RumorBufInitiator;	//???? is this needed
			StartRumor(MSG_CONFIRM);
		}
	}
}



/*******************************************************************************
 * \brief	Clears all maps and rumours.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

void ClearMapsAndRumors(void)
{
	// erase all maps and current rumours and rumour history
	ClearMap(SIDMap);											// nobody in mesh yet
	ClearMap(NbrMap);
	ClearMap(RecentNbrMap0);
	ClearMap(RecentNbrMap1);
	ClearMap(RecentNbrMap2);
	ClearMap(LurkMap);

	ReasonsForRumor = 0;										// currently no reason to send a rumour
	ReasonForCurrentRumor = 0;

	CheckNbrsCount = 0;											// next time, check frequently
	CheckNbrsMinute = Minute;

	RSSICutOff = 0;
}



/*******************************************************************************
 * \brief	Main loop.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/
static int commandTimeoutCount = 0;
void RM_MainLoop(void)
{
	wisafeMainLoopCount++;  // keep watchdog happy
	// main loop
	//while (1)			// removed for FreeRTOS
	// ABR
	LOG_Info("WiSafe RM Task");
	{
		// at this point, we may have a rumour in RumorBuf ready to spread, in which case REASON_SPREAD_RUMOR will be set
		// or a rumour in RumorBuf ready to propagate, in which case REASON_PROPAGATE_RUMOR will be set
		// other bits also may be set in ReasonsForRumor

		if(SID!=NULL_SID)
		{
			// keep track of minutes
			if((SysTime-MinuteSysTime)>MINUTE_SYS_TIME_COUNT)
			{
				MinuteSysTime = SysTime;
				Minute++;

				// time out of special "999999" test mode
				if((Minute>=3) && MeshMagicIsReserved())
				{
					// restart with NULL_SID
					ResetEEPROM();							// this will wipe key indicating no mesh info saved

					SID = NULL_SID;										// unassigned SID
					NetworkMeshID = NULL_MID;
					TrueMID = NetworkMeshID;
					MeshKey[0] = 0;										// default magic number for NULLMID
					MeshKey[1] = 0;
					MeshKey[2] = 0;
					ClearMap(SIDMap);									// nobody in mesh yet

					UpdateKeys(&MeshKey[0]);
					LongMsgFirstByte = (NetworkMeshID<<2)+MSG_LEN_LONG;
					MediumMsgFirstByte = (NetworkMeshID<<2)+MSG_LEN_MEDIUM;

					SysTime = 0;										// forget any time spent during initialisation
				}


				// time to check neighbours?
				if(CheckNbrsCount<6)
				{
					// we have not checked neighbours many times yet - check more often
					if((Minute-CheckNbrsMinute)>=(CHECK_NBRS_SHORT_INTERVAL+SID))
					{
						CheckNbrsMinute = Minute;
						//ReasonsForRumor |= REASON_CHECK_NBRS;
						CheckNbrsCount++;					// increment count if small, but not if large (so never overflows)
					}
				}
				else
				{
					if((Minute-CheckNbrsMinute)>=(CHECK_NBRS_LONG_INTERVAL+SID))
					{
						CheckNbrsMinute = Minute;
						//ReasonsForRumor |= REASON_CHECK_NBRS;
					}
				}
			}
		}

		// Check for received message from SD.
		if(SDMsgRcvd==SPIMSG_NULL)
		{
			commandTimeoutCount=0;
			checkWGtoRMqueue();
		}
		else
		{
			// check if previous command is taking too long and reset
			commandTimeoutCount++;
			if (commandTimeoutCount >= 3)
			{
				SDMsgRcvd = SPIMSG_NULL;
				commandTimeoutCount=0;
			}
			LOG_Error("SDMsgRcvd != SPIMSG_NULL");
		}

		if(SDMsgRcvd!=SPIMSG_NULL)
		{
#if ENHANCED_DEBUG
			LOG_Info(LOG_YELLOW "Process command 0x%02x!",SDMsgRcvd);
#endif
			if(SDMsgRcvd==SPIMSG_UNIT_TEST)
			{
				ReasonsForRumor |= REASON_SD_RUMOR;			// SD wants us to spread this as a rumour
			}
			else if(SDMsgRcvd==SPIMSG_TRANSPARENT)
			{
				// note - message stashed in PassThruMsg, until can be sent as rumour
				ReasonsForRumor |= REASON_SD_RUMOR;			// SD wants us to spread this as a rumour
				SendAckMsg();	/* send an ACK */
			}
			else if(SDMsgRcvd==SPIMSG_RM_DIAG_REQ)
			{
				SendDiagResultMsg();						// perform diagnostic tests and send result
			}
			else if(SDMsgRcvd==SPIMSG_RM_ID_REQ)
			{
				SendRMIdResponseMsg();						// report our serial number and MagicKeys
			}
			else if(SDMsgRcvd==SPIMSG_RM_EXTENDED_REQ)
			{
				SendRMExtendedResponseMsg();				// report various things
			}
			else if(SDMsgRcvd==SPIMSG_RM_CLEAR_CMD)
			{
				// Do we need confirmation, there is possibility of clearing the EEPROM by mistake
				if(TestEEPROM_MFCT())
				{
					// no clearing allowed in operation mode
					SendNackMsg();
				}
				else
				{
					ResetEEPROM();							// this will wipe key indicating no mesh info saved

					SID = NULL_SID;										// unassigned SID
					NetworkMeshID = NULL_MID;
					TrueMID = NetworkMeshID;
					MeshKey[0] = 0;										// default magic number for NULLMID
					MeshKey[1] = 0;
					MeshKey[2] = 0;
					ClearMap(SIDMap);									// nobody in mesh yet

					UpdateKeys(&MeshKey[0]);
					LongMsgFirstByte = (NetworkMeshID<<2)+MSG_LEN_LONG;
					MediumMsgFirstByte = (NetworkMeshID<<2)+MSG_LEN_MEDIUM;

					SysTime = 0;										// forget any time spent during initialisation

					SendAckMsg();
				}
			}
			else if(SDMsgRcvd==SPIMSG_RM_RESET_CMD)
			{
				// Do we need confirmation, there is possibility of clearing the EEPROM by mistake
				if(TestEEPROM_MFCT())
				{
					// no clearing allowed in operation mode
					SendNackMsg();
				}
				else
				{
					ResetEEPROM();							// this will wipe key indicating no mesh info saved
					//ResetEEPROM_MFCT();					// not needed. we can be here only in manufacture mode

					SID = NULL_SID;										// unassigned SID
					NetworkMeshID = NULL_MID;
					TrueMID = NetworkMeshID;
					MeshKey[0] = 0;										// default magic number for NULLMID
					MeshKey[1] = 0;
					MeshKey[2] = 0;
					ClearMap(SIDMap);									// nobody in mesh yet

					UpdateKeys(&MeshKey[0]);
					LongMsgFirstByte = (NetworkMeshID<<2)+MSG_LEN_LONG;
					MediumMsgFirstByte = (NetworkMeshID<<2)+MSG_LEN_MEDIUM;

					SysTime = 0;										// forget any time spent during initialisation

					SendAckMsg();
				}
			}
			else if(SDMsgRcvd==SPIMSG_RM_PROD_TST_CMD)
			{
				if(TestEEPROM_MFCT())
				{
					// no tests allowed after manufacturing test stamp!
					SendNackMsg();
				}
				else
				{
					SendRMProdTstResponseMsg();				// perform a production test - might not return!
				}
			}
			else if(SDMsgRcvd==SPIMSG_RM_MFCT_MODE)
			{
				ResetEEPROM_MFCT();							// clear MFCT stamp entering manufacturing mode
			}
			else if(SDMsgRcvd==SPIMSG_RM_OPER_MODE)
			{
				SetEEPROM_MFCT();							// write MFCT stamp entering operational mode
			}
			else
			{
				// mystery message ????
				SendNackMsg();								// send some kind of NACK
			}

			// eat SD message
			SDMsgRcvd = SPIMSG_NULL;						// i.e. no message pending
		}




		// are we in a network or do we need to join a mesh?
		if(SimulatedButtonPressCount!=0)
		{
			if((SimulatedButtonPressCount & ~LEARN_BUTTON_PRESS_MASK) == CMD_UNLEARN)
			{
				LOG_Info("Unlearn the GateWay. \n\n\n\n\n");

				ResetEEPROM();										// write all bits in EEPROM connection map

				SID = NULL_SID;										// unassigned SID
				NetworkMeshID = NULL_MID;
				TrueMID = NetworkMeshID;
				MeshKey[0] = 0;										// default magic number for NULLMID
				MeshKey[1] = 0;
				MeshKey[2] = 0;
				ClearMap(SIDMap);									// nobody in mesh yet

				UpdateKeys(&MeshKey[0]);
				LongMsgFirstByte = (NetworkMeshID<<2)+MSG_LEN_LONG;
				MediumMsgFirstByte = (NetworkMeshID<<2)+MSG_LEN_MEDIUM;

				SysTime = 0;										// forget any time spent during initialisation
				// only send ack if unlearn command received via the network (not on-board pushbutton)
				if ((SimulatedButtonPressCount & LEARN_BUTTON_PRESS_MASK) == 0)
				{
					SendAckMsg();
				}
#if SCRAMBLED_WISAFE
				Scramble0 = 0;
#endif
				learnInStatus = LEARN_UNLEARNT;
			}
			else if((SimulatedButtonPressCount & ~LEARN_BUTTON_PRESS_MASK) == CMD_LEARN)
			{
				// if already in a mesh, ignore
				if(SID==NULL_SID)
				{
					// not already in a mesh - try to join
					LOG_Info("Learn in the GateWay. \n\n\n\n\n")
					learnInStatus = LEARN_ACTIVE;
					if(JoinNetwork())
					{
						//PRINTF("Successfully joined network ***! \n\r");// success
						// only send ack if unlearn command received via the network (not on-board pushbutton)
						if ((SimulatedButtonPressCount & LEARN_BUTTON_PRESS_MASK) == 0)
						{
							SendAckMsg();
						}
						learnInStatus = LEARN_JOINED;
					}
					else
					{
						learnInStatus = LEARN_INACTIVE;
					}
				}
			}
			SimulatedButtonPressCount = 0;
		}




		// check for timeout of an expected confirmation

		// CONFIRMATION_TIMEOUT is number of eight second ticks to wait for confirmation message back ????
		if(ConfirmationState!=0)
		{
			if((SysTime-ConfirmationTime)>CONFIRMATION_TIMEOUT)
			{
				ConfirmationState = 0;						// no longer expecting a confirmation
			}
		}



		// at this point, we may have a rumour ready to spread/propagate in RumorBuf
		// additional bits may have been set in ReasonsForRumor



		// if in manufacturing test, don't send any rumours
		// changed so it does communicate with the reserved keys
		/*
		if(MeshMagicIsReserved())
		{
			ReasonsForRumor = 0;							// no rumours allowed in manufacturing test
		}
		*/


		// if we have a reason to start a rumour, build it now in RumorBuf
		if(ReasonsForRumor!=0)
		{
			PreviousRSN = RumorSequenceNumber;				// may want to restore it
			StartReasonRumor();								// might increment RumorSequenceNumber
		}



		// chirp once, or more if we want to listen
#if ENHANCED_DEBUG
		LOG_Info(LOG_YELLOW "Chirp!");
#endif
		if(SID!=NULL_SID)
		{
			uint16_t OriginalReason;
			uint8_t  ChirpCount;

			OriginalReason = ReasonForCurrentRumor;			// to test for reason changing to REASON_PROPAGATE_RUMOR
			ChirpCount = 0;
			do
			{
				if(ChirpCount!=0)
				{
					Delay(1);								// spread out chirping slightly in case listener is temporarily busy
				}
				ChirpToReceive();							//Chirp();// if we accept a rumour for propagation, ReasonForCurrentRumor will change to REASON_PROPAGATE_RUMOR
															// if some other node is listening, ReasonForCurrentRumor will be 0
				ChirpCount++;
				/////TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT/////////////////
				//	OutgoingMsg[0] = 48+ChirpCount;
				//	OutgoingMsgLen = 1;
				//	SendOutgoingMsg();												// tell SD about rumour we just received
				//	OutgoingMsgLen = 0;
				//////////////////////////////////////////////////////////
			}while((ReasonForCurrentRumor!=0) && (ChirpCount<5));

			// at this point, either we have Chirped once and we have no reason for rumour
			// or we had a reason for rumour but lost it because some node was listening
			// or we have chirped 5 times, possibly accepting a rumour for propagation

			if((OriginalReason!=0) && (OriginalReason!=ReasonForCurrentRumor))
			{
				// some other node is listening or we picked up a rumour
				RumorSequenceNumber = PreviousRSN;			// increment was not necessary since we have temporarily abandoned our rumour
			}
		}




		// we have chirped - now listen
		// cannot listen if somebody else is in listen mode
		if(ReasonForCurrentRumor!=0)
		{
			SomeNodeHasUrgentMsg = false;					// gets set in Listen if some other node wants to spread a message
			CopyRumorToMsg();								// get rumour ready in msgbuf
			ListenTimeLeft = LISTEN_DURATION;				// will listen for this long, unless time extended

			while(ListenTimeLeft>0)
			{
				uint16_t TimeValue;

#if ENHANCED_DEBUG
				LOG_Info(LOG_YELLOW "Listening, RFCR=0x%04x!",ReasonForCurrentRumor);
#endif
				// now pass rumour to one other node
				TimeValue = SysTime;
				if(ReasonForCurrentRumor!=0)
				{
#if ENHANCED_DEBUG
					LOG_Info(LOG_YELLOW "Listen to Tx packet");
#endif
					ListenToTransmitPacket(ListenTimeLeft);					// call RcvMsg once and process result
#if ENHANCED_DEBUG
					LOG_Info(LOG_YELLOW "Listen to Tx packet complete");
#endif
				}
				TimeValue = SysTime-TimeValue;				// now have number of ticks spent this call to Listen

				// keep track of time left for listening

				if(ListenTimeLeft>TimeValue)
				{
					if(TimeValue==0)
					{
						TimeValue = 1;
					}
					ListenTimeLeft -= TimeValue;			// still time left */
					//LOG_Info(LOG_YELLOW "Listening!");
				}
				else
				{
					ListenTimeLeft = 0;						// time used up

					if(ReasonForCurrentRumor==0)
					{
						;// rumour has been cleared up during Listen - perhaps it was propagated to another node
					}
					else if(ReasonForCurrentRumor!=REASON_PROPAGATE_RUMOR)
					{
						ReasonsForRumor &= ~ReasonForCurrentRumor;	// our reason has been satisfied

						// decide whether to propagate

						if((ReasonForCurrentRumor&(REASON_CHECK_NBRS+REASON_SNIFF_NBRS))!=0)
						{
							/*
							// special cases - do not propagate this rumour
							if ((ReasonForCurrentRumor==REASON_SNIFF_NBRS) || (ACPowered))
							{
								// report current neighbours to AC-powered SD
								if (OutgoingMsgLen==0)
								{
									OutgoingMsg[0] = SPIMSG_RM_EXTENDED_RESPONSE;
									OutgoingMsg[1] = SPIMSG_EXTD_OPT_NBR_CHECK;
									CopyMap(&OutgoingMsg[2],(RSSICutOff!=0)? LurkMap: RecentNbrMap0,0);	// 8 bytes
									OutgoingMsg[10] = SID;												// for convenience, append our SID
									OutgoingMsgLen = 11;
								}
							}

							// now have several recent neighbour maps
							if (ReasonForCurrentRumor==REASON_CHECK_NBRS)
							{
								// if AC-powered host requested this check, don't mess with update
								if (UpdateNbrMap())
								{
									BroadcastNbrMap = TRUE;												// our NbrMap has changed - better tell everyone
								}
							}

							ReasonForCurrentRumor = 0;													// that's the end of the check/sniff nbrs rumour

							// if anything has changed, tell everyone about our neighbours
							if (BroadcastNbrMap)
							{
								ReasonsForRumor |= REASON_JOINED;										// we'll report results in a future rumour
								BroadcastNbrMap = FALSE;												// assume remour will get through?  could use confirmations here
							}
							*/

							//ABR
							ReasonForCurrentRumor = 0;
						}
						else if(CountBits(RumorMap)!=0)
						{
							// still nodes left who haven't heard rumour - propagate rumour (now or later)

							// propagate rumour (possibly without stopping listening) (with reduced TTL)
							ReasonsForRumor |= REASON_PROPAGATE_RUMOR;
#if ENHANCED_DEBUG
							LOG_Info(LOG_YELLOW "Copy map 1");
#endif
							CopyMap(PropagateMap,SIDMap,0);					// for now we are willing to propagate to any other node
#if ENHANCED_DEBUG
							LOG_Info(LOG_YELLOW "Copy map 1 complete");
#endif
							PropagationCount = 0;							// count 3-second attempts to propagate
#if ENHANCED_DEBUG
							LOG_Info(LOG_YELLOW "Copy rumour to msg 1");
#endif
							CopyRumorToMsg();								// make sure rumour is in msgbuf as well as RumorBuf
#if ENHANCED_DEBUG
							LOG_Info(LOG_YELLOW "Copy rumour to msg 1 complete");
#endif

#if ENHANCED_DEBUG
							LOG_Info(LOG_YELLOW "Restart listen time");
#endif
							ListenTimeLeft = LISTEN_DURATION;				// restart 3 seconds of listening
							ReasonForCurrentRumor = REASON_PROPAGATE_RUMOR;
						}
						else
						{
							// if not propagating, rumour spread is done!
							ReasonForCurrentRumor = 0;						//* rumour is gone. rumour spread is complete
#if ENHANCED_DEBUG
							LOG_Info(LOG_YELLOW "Gen confirm 1");
#endif
							GenerateConfirmation();							// rumour initiator may want a success confirmation
#if ENHANCED_DEBUG
							LOG_Info(LOG_YELLOW "Gen confirm 1 complete");
#endif
						}
					}
					else if(ReasonForCurrentRumor==REASON_PROPAGATE_RUMOR)
					{
						// this is bad - we're trying to propagate a message but nobody responded over the last 3 seconds!
						PropagationCount++;									// count 3-second attempts to propagate
						if(PropagationCount<PROPAGATION_COUNT_LIMIT)
						{
							// give other nodes a chance to jump in by exiting listen loop - will resume in a second
							ReasonForCurrentRumor = 0;						// will get set in a second
#if ENHANCED_DEBUG
							LOG_Info(LOG_YELLOW "Copy rumour to msg 2");
#endif
							CopyRumorToMsg();								// make sure rumour is in msgbuf as well as RumorBuf
#if ENHANCED_DEBUG
							LOG_Info(LOG_YELLOW "Copy rumour to msg 2 complete");
#endif
						}
						else
						{
							// this is very bad - we're going to have to discard the rumour after failing to propagate!
							ReasonsForRumor &= ~ReasonForCurrentRumor;		// our reason has been satisfied
							ReasonForCurrentRumor = 0;						// rumour is gone
#if ENHANCED_DEBUG
							LOG_Info(LOG_YELLOW "Gen confirm 2");
#endif
							GenerateConfirmation();							// when abandoning rumour, may have to send a confirmation
#if ENHANCED_DEBUG
							LOG_Info(LOG_YELLOW "Gen confirm 2 complete");
#endif
						}
					}
				}
			}
			// we are done listening
		}

		// we are done chirping/listening

		// now pass message to SD if one received recently
		if(OutgoingMsgLen>0)
		{
#if ENHANCED_DEBUG
			LOG_Info(LOG_YELLOW "Send outgoing msg");
#endif
			SendOutgoingMsg();												// tell SD about rumour we just received
#if ENHANCED_DEBUG
			LOG_Info(LOG_YELLOW "Send outgoing msg complete");
#endif
			OutgoingMsgLen = 0;												// eat OutgoingMsg
		}

#if SCRAMBLED_WISAFE
		// ABR introduce radio sleep to save power///////
#if ENHANCED_DEBUG
		LOG_Info(LOG_YELLOW "Enter sleep");
#endif
		Sleep_Radio();
#if ENHANCED_DEBUG
		LOG_Info(LOG_YELLOW "Enter sleep complete");
#endif
		/////////////////////////////////////////////////
#endif
		/*
		if(DoLurk && (SID!=NULL_SID))
		{
			Lurk();															// instead of sleeping, lurk!
		}
		*/
		//Delay(3000);//Sleep is removed, so need to be done every 3 seconds.
		/*
		StartTime = GetCurrentTimerCount();
		do
		{
			CurrentTime = GetCurrentTimerCount();
		}
		while( (SDMsgRcvd == SPIMSG_NULL) && (ReasonForCurrentRumor == 0) && ((CurrentTime-StartTime)<(uint32_t)(THREE_SECOND)) );
		*/
	}
	LOG_Info(LOG_GREEN "WiSafe RM Exit");
}



/*******************************************************************************
 * \brief	Reset Wisafe-2 configuration flash.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

void ResetEEPROM(void)
{
	uint8_t *ptr;

	ptr = &RMCONFIG.EEKey0;

	readRadioConfigFile();								//To get the MFCT_STAMPs

	for(uint8_t i=0; i<(sizeof(RMCONFIG)-6); i++)
	{
		*ptr = 0xff;
		ptr++;
	}

	writeRadioConfigFile();
}



/*******************************************************************************
 * \brief	Reset manufacturing configuration flash area.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

void ResetEEPROM_MFCT(void)
{
	readRadioConfigFile();								//Redundant, but to be sure not to corrupt data in case one variable in RAM is corrupted
	RMCONFIG.MFCT_Stamp0 = 0xff;
	RMCONFIG.MFCT_Stamp1 = 0xff;
	writeRadioConfigFile();
}



/*******************************************************************************
 * \brief	Set manufacturing configuration flash area.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

void SetEEPROM_MFCT(void)
{
	readRadioConfigFile();								//Redundant, but to be sure not to corrupt data in case one variable in RAM is corrupted
	RMCONFIG.MFCT_Stamp0 = MFCT_STAMP0;
	RMCONFIG.MFCT_Stamp1 = MFCT_STAMP1;
	writeRadioConfigFile();
}



/*******************************************************************************
 * \brief	Check manufacturing state.
 *
 * \param   None.
 * \return	True, if manufacturing completed. False, if not.
 ******************************************************************************/

bool TestEEPROM_MFCT(void)
{
	// returns TRUE if "manufacturing completed" stamp in EEPROM manufacturing stamp area
	return ((RMCONFIG.MFCT_Stamp0==MFCT_STAMP0) && (RMCONFIG.MFCT_Stamp1==MFCT_STAMP1));
}



/*******************************************************************************
 * \brief	Saves mesh information.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

#define EEPROM_KEY_0 	0x56
#define EEPROM_KEY_1 	0xCD

void SaveMeshInfo(void)
{
	uint8_t checksum;
	uint8_t *ptr;


	checksum = 0;
	ptr = &RMCONFIG.EEKey0;

	readRadioConfigFile();						//To get the MFCT_STAMPs

	RMCONFIG.EEKey0 = EEPROM_KEY_0;
	RMCONFIG.EEKey1 = EEPROM_KEY_1;
	RMCONFIG.SID = SID;
	RMCONFIG.MeshKey[0] = MeshKey[0];
	RMCONFIG.MeshKey[1] = MeshKey[1];
	RMCONFIG.MeshKey[2] = MeshKey[2];
	for(uint8_t i=0; i<sizeof(SIDMap); i++)
	{
		RMCONFIG.SIDMap[i] = SIDMap[i];
	}
	RMCONFIG.GateWayID[0] = GateWayID[0];
	RMCONFIG.GateWayID[1] = GateWayID[1];
	RMCONFIG.GateWayID[2] = GateWayID[2];
	RMCONFIG.SDModel[0] = SDModel[0];
	RMCONFIG.SDModel[1] = SDModel[1];

	for(uint8_t i=0; i<(sizeof(RMCONFIG)-7); i++)		//from CRC onwards not included in CRC calculations
	{
		checksum ^= *ptr;
		ptr++;
	}

	RMCONFIG.CheckSum = checksum;

	writeRadioConfigFile();
}



/*******************************************************************************
 * \brief	Restore mesh information.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

bool RestoreMeshInfo(void)
{
	bool result;
	uint8_t checksum;
	uint8_t *ptr;


	result = true;
	checksum = 0;
	ptr = &RMCONFIG.EEKey0;

	readRadioConfigFile();

	if(RMCONFIG.EEKey0 != EEPROM_KEY_0)
	{
		result = false;
	}

	if(RMCONFIG.EEKey1 != EEPROM_KEY_1)
	{
		result = false;
	}

	if(result)
	{
		SID = RMCONFIG.SID;
		MeshKey[0] = RMCONFIG.MeshKey[0];
		MeshKey[1] = RMCONFIG.MeshKey[1];
		MeshKey[2] = RMCONFIG.MeshKey[2];
		TrueMID = MeshKey[0] & SID_MASK;			// bottom 6 bits are Mesh ID
		NetworkMeshID = TrueMID;
		for(uint8_t i=0; i<sizeof(SIDMap); i++)
		{
			SIDMap[i] = RMCONFIG.SIDMap[i];
		}
		GateWayID[0] = RMCONFIG.GateWayID[0];
		GateWayID[1] = RMCONFIG.GateWayID[1];
		GateWayID[2] = RMCONFIG.GateWayID[2];
		SDModel[0] = RMCONFIG.SDModel[0];
		SDModel[1] = RMCONFIG.SDModel[1];
	}

	for(uint8_t i=0; i<(sizeof(RMCONFIG)-7); i++)	//MFCT_STAMPs area and after, not included
	{
		checksum ^= (*ptr);
		ptr++;
	}

	if(checksum != RMCONFIG.CheckSum)
	{
		result = false;
	}

	return result;
}



/*******************************************************************************
 * \brief	Checks mesh keys.
 *
 * \param   None.
 * \return	True, if special reserved mesh keys used. False, if not.
 ******************************************************************************/

bool MeshMagicIsReserved(void)
{
	return 	((MeshKey[0]==RESERVED_MM0) && (MeshKey[1]==RESERVED_MM1) && (MeshKey[2]==RESERVED_MM2));
}



/*******************************************************************************
 * \brief	Saves frequency fractions.
 *
 * \param   freqfrac2. 2nd frequency fraction.
 * \param	freqfrac3. 3rd frequency fraction.
 * \return	None.
 ******************************************************************************/

void SaveFreqFrac(uint8_t freqfrac2, uint8_t freqfrac3)
{
#if FUNCTIONAL_TEST
	// save calibration data to flash
	uint8_t wisafeCalData[] = {FREQ_FRAC_KEY0,FREQ_FRAC_KEY1,freqfrac2,freqfrac3};
	if (HAL_SetWisafeCalibration(wisafeCalData) != 0)
	{
		LOG_Error("Failed to write WiSafe calibration data to flash");
	}
#endif
	readRadioConfigFile();									//Redundant, but to be sure not to corrupt data in case one variable in RAM is corrupted
	RMCONFIG.FreqFracKey0 = FREQ_FRAC_KEY0;
	RMCONFIG.FreqFracKey1 = FREQ_FRAC_KEY1;
	RMCONFIG.FreqFrac2 = freqfrac2;
	RMCONFIG.FreqFrac3 = freqfrac3;
	writeRadioConfigFile();
}
