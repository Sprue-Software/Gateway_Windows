/*******************************************************************************
 * \file	messages.c
 * \brief 	Building and structuring all Wisafe-2 messages including CRC.
 * \note 	Project: 	WG2. Low cost wireless gateway
 * \author  Abdul Ben-Rashed
 ******************************************************************************/

#define _MESSAGE_C_

#include "C:/Users/ndiwathe/Documents/MCUXpressoIDE_11.1.1_3241/workspace/EnsoAgent/source/OSAL/RT1050/wisafe_drv/wisafe_main.h"
#include "fsl_gpio.h"
#include "fsl_debug_console.h"
//#include "fsl_lpspi.h" //nishi
#include "messages.h"
#include <timer.h>
#include "radio.h"
#include "logic.h"
#include "SDcomms.h"


#define POLY    0x8005

static uint16_t MSGcrc;

static uint8_t Key0;
static uint8_t Key1;
static uint8_t Key2;
static uint8_t ID[3];



/*******************************************************************************
 * \brief	Initialise mesh network keys.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

void Init_Messages(void)
{
	Key0 = 0;
	Key1 = 0;
	Key2 = 0;
}



/*******************************************************************************
 * \brief	Update WG2 SID.
 *
 * \param   newsid. New WG SID allocated after joined in a network
 * \return	None.
 ******************************************************************************/

void UpdateSID(uint8_t newsid)
{
	SID = newsid & SID_MASK;
}



/*******************************************************************************
 * \brief	Update mesh network Keys.
 *
 * \param   *keyptr. Pointer to mesh network keys.
 * \return	None.
 ******************************************************************************/

void UpdateKeys(uint8_t *keyptr)
{
	uint8_t *ptr;

	ptr = keyptr;//Do not use keyptr directly and increment it and screw up everything

	Key0 = *ptr++;
	Key1 = *ptr++;
	Key2 = *ptr++;
}



/*******************************************************************************
 * \brief	Update WG2 ID.
 *
 * \param   *idptr. Pointer to gateway ID.
 * \return	None.
 ******************************************************************************/

void UpdateID(uint8_t *idptr)
{
	uint8_t *ptr;

	ptr = idptr;

	ID[0] = *ptr++;
	ID[1] = *ptr++;
	ID[2] = *ptr++;
}



/*******************************************************************************
 * \brief	Reverse bits of a byte.
 *
 * \param   byte. Byte to be reversed.
 * \return	Reversed byte.
 ******************************************************************************/

uint8_t ReverseByte(uint8_t byte)
{
	uint8_t reversedbyte = 0;

	for (uint8_t i=0; i<8; i++)
	{
		reversedbyte = reversedbyte<<1;
		reversedbyte += (byte & 0x01);
		byte = byte>>1;
	}

	return reversedbyte;
}


/*
uint8_t ReverseByte(uint8_t byte)
{
   uint8_t v;

   v = byte;
   v = (v & 0xF0) >> 4 | (v & 0x0F) << 4;
   v = (v & 0xCC) >> 2 | (v & 0x33) << 2;
   v = (v & 0xAA) >> 1 | (v & 0x55) << 1;

   return v;
}
*/



/*******************************************************************************
 * \brief	Get CRC of a message.
 *
 * \param   None.
 * \return	Message CRC value.
 ******************************************************************************/

uint16_t GetCRC(void)
{
	return MSGcrc;
}



/*******************************************************************************
 * \brief	Initialise CRC.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

void CRCInit(void)
{
  if( (Key0 & 0x3F) != NULL_MID)
  {
	  MSGcrc = ( ((uint16_t) Key1) << 8 ) + Key2;
  }
  else
  {
#if SCRAMBLED_WISAFE
	  MSGcrc = 0x29;	//ABR Scramble use initial CRC as 0x29 instead of 0 in the normal unscrambled messages
#else
	  MSGcrc = 0;
#endif
  }
}



/*******************************************************************************
 * \brief	Update CRC.
 *
 * \param   byte. Byte to be included in CRC.
 * \return	None.
 ******************************************************************************/

void CRCByte(uint8_t byte)
{
    uint8_t xmsk;

    xmsk = 0x80;	/* MSB first */
    while(xmsk)
    {
            if(byte&xmsk)
            {
              if(MSGcrc&0x8000)
            	  MSGcrc <<= 1;
              else
            	  MSGcrc = (MSGcrc<<1) ^ POLY;
            }
            else
            {
              if(MSGcrc&0x8000)
            	  MSGcrc = (MSGcrc<<1) ^ POLY;
              else
            	  MSGcrc <<= 1;
            }
            // on to next bit in byte
            xmsk >>= 1;
    }
}



/*******************************************************************************
 * \brief	Build Ok message.
 *
 * \param   *dataptr. Pointer to Ok message.
 * \return	None.
 ******************************************************************************/

void BuildOkMsg(uint8_t *dataptr)
{
	uint8_t *crcdataptr;


	*dataptr++ = 0x2A;//0xAA
	*dataptr++ = 0xAA;
	*dataptr++ = 0xAA;
	*dataptr++ = 0xA9;

	crcdataptr = dataptr;							//pointer to first data field to be crc calculated

	*dataptr++ = ((Key0<<2) + MSG_LEN_SHORT);
	*dataptr++ = SID;

	CRCInit();
	for(uint8_t count=1; count<3; count++)
	{
		CRCByte(*crcdataptr++);
	}

	MSGcrc = ~MSGcrc;
	*dataptr++ = (uint8_t)(MSGcrc>>8);
	*dataptr++ = (uint8_t)(MSGcrc&0x00FF);
}



/*******************************************************************************
 * \brief	Build rumour ack message.
 *
 * \param   *dataptr. Pointer to rumour ack message.
 * \return	None.
 ******************************************************************************/

void BuildRumorAckMsg(uint8_t *dataptr)
{
	dataptr[0] = 0xAA;
	dataptr[1] = 0xAA;
	dataptr[2] = 0xAA;
	dataptr[3] = 0xA9;

	dataptr[4] = ((Key0<<2) + MSG_LEN_MEDIUM);
	dataptr[5] = SID;
	dataptr[6] = MSG_ACKRUMOR;
	dataptr[7] = RumorSenderSID;
	dataptr[8] = GateWayID[1];//ID[1];
	dataptr[9] = GateWayID[2]^MM_XOR_REV12;//ID[2]^MM_XOR_REV12;

	CRCInit();
	for(uint8_t i=4; i<10; i++)
	{
		CRCByte(dataptr[i]);
	}

	MSGcrc = ~MSGcrc;
	dataptr[10] = (uint8_t)(MSGcrc>>8);
	dataptr[11] = (uint8_t)(MSGcrc&0x00FF);

#if SCRAMBLED_WISAFE
	// ABR Scramble
	if(Scramble0!=0)
	{
		MsgScramble(2);
	}
#endif
}



/*******************************************************************************
 * \brief	Build join request message.
 *
 * \param   *dataptr. Pointer to join request message.
 * \return	None.
 ******************************************************************************/

void BuildJoinRequestMsg(uint8_t *dataptr)
{
	dataptr[0] = 0xAA;
	dataptr[1] = 0xAA;
	dataptr[2] = 0xAA;
	dataptr[3] = 0xA9;

	dataptr[4] = 0x03;
	dataptr[5] = FILLER;
	dataptr[6] = MSG_JOINREQ;
	dataptr[7] = FWREV;
	dataptr[8] = GateWayID[0];
	dataptr[9] = GateWayID[1];
	dataptr[10] = GateWayID[2];
	dataptr[11] = DITHER;
	dataptr[12] = DITHER;
	dataptr[13] = DITHER;
	dataptr[14] = DITHER;
	dataptr[15] = DITHER;
	dataptr[16] = DITHER;
	dataptr[17] = DITHER;
	dataptr[18] = DITHER;

	CRCInit();
	for(uint8_t i=4; i<19; i++)
	{
		CRCByte(dataptr[i]);
	}

	MSGcrc = ~MSGcrc;
	dataptr[19] = (uint8_t)(MSGcrc>>8);
	dataptr[20] = (uint8_t)(MSGcrc&0x00FF);
#if SCRAMBLED_WISAFE
	// ABR Scramble
	if(Scramble0!=0)
	{
		MsgScramble(2);
	}
#endif
}



/*******************************************************************************
 * \brief	Build chirp response message.
 *
 * \param   *dataptr. Pointer to chirp response message.
 * \return	None.
 ******************************************************************************/

void BuildChirpResponseMessage(uint8_t *dataptr)
{
	dataptr[0] = 0xAA;
	dataptr[1] = 0xAA;
	dataptr[2] = 0xAA;
	dataptr[3] = 0xA9;
	dataptr[4] = ((Key0<<2) + MSG_LEN_TINY);
}



/*******************************************************************************
 * \brief	Build status reply message.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

void BuildStatusReplyMsg(void)
{
	BuildRumorMessage(&msgbuf[0], MSG_STATUSREPLY);
	msglen = RUMOR_MESSAGE_LEN;						//Length in bytes
	msglen <<=3;									//Length in bits
}



/*******************************************************************************
 * \brief	Build status request message.
 *
 * \param   targetsid. Target SID of this message.
 * \return	None.
 ******************************************************************************/

void BuildStatusRequestMsg(uint8_t targetsid)
{
	TargetSID = targetsid;
	if(MapMsgType==0)
	{
		BuildRumorMessage(&msgbuf[0], MSG_STATUSREQ);
	}
	else
	{
		PassThruMsg[0] = SPIMSG_DO_EXDIG_ID;
		PassThruMsg[1] = 0;
		PassThruMsg[2] = 0;
		PassThruMsg[3] = 0;
		PassThruMsg[4] = 0;
		PassThruMsg[5] = 0;
		PassThruMsg[6] = 0;
		PassThruMsg[7] = 0;
		PassThruMsg[8] = 0;
		PassThruMsgLen = 4;
		BuildRumorMessage(&msgbuf[0], MSG_SD_RUMOR);
	}
	msglen = RUMOR_MESSAGE_LEN;						//Length in bytes
	msglen <<=3;									//Length in bits
}



/*******************************************************************************
 * \brief	Build map reply message.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

void BuildMapReplyMsg(void)
{
	BuildRumorMessage(&msgbuf[0], MSG_MAPREPLY);
	msglen = RUMOR_MESSAGE_LEN;						//Length in bytes
	msglen <<=3;									//Length in bits
}



/*******************************************************************************
 * \brief	Build map request message.
 *
 * \param   targetsid. Target SID of this message.
 * \return	None.
 ******************************************************************************/

void BuildMapRequestMsg(uint8_t targetsid)
{
	TargetSID = targetsid;
	BuildRumorMessage(&msgbuf[0], MSG_MAPREQ);
	msglen = RUMOR_MESSAGE_LEN;						//Length in bytes
	msglen <<=3;									//Length in bits
}



/*******************************************************************************
 * \brief	Build rumour message.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

void BuildSDRumorMsg(void)
{
	BuildRumorMessage(&msgbuf[0], MSG_SD_RUMOR);
	msglen = RUMOR_MESSAGE_LEN;						//Length in bytes
	msglen <<=3;									//Length in bits
}



/*******************************************************************************
 * \brief	Build propagate message.
 *
 * \param   targetsid. Target SID of this message.
 * \return	None.
 ******************************************************************************/

void BuildPropagateMsg(uint8_t targetsid)
{
	TargetSID = targetsid;
	BuildRumorMessage(&msgbuf[0], MSG_PROPAGATE);
	msglen = RUMOR_MESSAGE_LEN;						//Length in bytes
	msglen <<=3;									//Length in bits
}



/*******************************************************************************
 * \brief	Build joined message.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

void BuildJoinedMsg(void)
{
	BuildRumorMessage(&msgbuf[0], MSG_JOINED);
	msglen = RUMOR_MESSAGE_LEN;						//Length in bytes
	msglen <<=3;									//Length in bits
}



/*******************************************************************************
 * \brief	Build SID map update message.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

void BuildSIDMapUpdateMsg(void)
{
	BuildRumorMessage(&msgbuf[0], MSG_SIDMAP_UPDATE);
	msglen = RUMOR_MESSAGE_LEN;						//Length in bytes
	msglen <<=3;									//Length in bits;
}



/*******************************************************************************
 * \brief	Build rumour confirmation message.
 *
 * \param   ttl. Time to live left for the message.
 * \return	None.
 ******************************************************************************/

void BuildRumorConfirmationMsg (uint8_t ttl)
{
	RumorTTL = ttl;
	BuildRumorMessage(&msgbuf[0], MSG_CONFIRM);
	msglen = RUMOR_MESSAGE_LEN;						//Length in bytes
	msglen <<=3;									//Length in bits
}



/*******************************************************************************
 * \brief	Build rumour message.
 *
 * \param   CurrentMessage. The rumour message type.
 * \return	None.
 ******************************************************************************/

void BuildRumorMessage(uint8_t volatile *dataptr, uint8_t CurrentMessage)
{
	uint8_t index;


	index = 0;

	//Now the message
	//Exactly 28bit preamble seems to be the best
	dataptr[index++] = 0x2A;//0xAA
	dataptr[index++] = 0xAA;
	dataptr[index++] = 0xAA;
	dataptr[index++] = 0xA9;

	dataptr[index++] = ((Key0<<2) + MSG_LEN_LONG);
	dataptr[index++] = SID;
	dataptr[index++] = CurrentMessage;
	dataptr[index++] = SID;//OriginatorSID;
	dataptr[index++] = (RumorSequenceNumber<<4) | DEFAULT_TTL;


	switch(CurrentMessage)
	{
		case MSG_JOINED:
			dataptr[index++] = GateWayID[1];//ID[1];
			dataptr[index++] = GateWayID[2];//ID[2];
			NbrMap[7] = GateWayID[0];//ID[0];
			CopyMap(&dataptr[index],NbrMap,MAP_DITHER);
			NbrMap[7] = 0;
			break;

		case MSG_CONFIRM:
			dataptr[index++] = RumorTTL;
			dataptr[index++] = Key2;
			CopyMap(&dataptr[index],RumorMap,MAP_DITHER);
			break;

		case MSG_STATUSREQ:
			dataptr[index++] = TargetSID;
			dataptr[index++] = Key2;
			CopyMap(&dataptr[index],NbrMap,MAP_DITHER);	//Irrelevant, send any as it is not used. NbrMap will do.
			break;

		case MSG_STATUSREPLY:
			dataptr[index++] = 32;		//VBAT no load. Equivalent to 3.3V
			dataptr[index++] = 32;		//VBAT with load. Equivalent to 3.3V
			dataptr[index++] = 32;		//VBAT with load minimum over recent history
			dataptr[index++] = 100;		//RSSI read from a node in the mesh. Not used so a random number given
			dataptr[index++] = FWREV;	//Longer Loaded VBAT high byte
			dataptr[index++] = GateWayID[0];//ID[0];
			dataptr[index++] = GateWayID[1];//ID[1];
			dataptr[index++] = GateWayID[2];//ID[2];
			dataptr[index++] = 0;		//Critical faults. None
			dataptr[index++] = 0;		//Radio failure count. In RM, the number of resets is counted. Cannot implement in Gateway.
			break;

		case MSG_MAPREQ:
			dataptr[index++] = TargetSID;
			dataptr[index++] = MapMsgType;
			CopyMap(&dataptr[index],NbrMap,MAP_DITHER);	//Filler, irrelevant, send any as it is not used. NbrMap will do.
			break;

		case MSG_MAPREPLY:
			dataptr[index++] = Key1;//Filler
			dataptr[index++] = MapMsgType;
			if(MapMsgType==0)
			{
				CopyMap(&dataptr[index],SIDMap,MAP_DITHER);
			}
			else
			{
				CopyMap(&dataptr[index],NbrMap,MAP_DITHER);//MapMsgType 3. All other types not used in gateway.
			}
			break;

		case MSG_SD_RUMOR:
			dataptr[index++] = PassThruMsg[8];
			dataptr[index++] = PassThruMsgLen;
			CopyMap(&dataptr[index],PassThruMsg,MAP_DITHER);
			dataptr[5] |= URGENT_MSG_BIT;	//Used with SID for SD_RUMOR, PROPAGATE_RUMOR, and SPREAD_RUMOR(SD_RUMOR for a message accepted
											//to be propagated
			break;

		case MSG_MFCT_STAMP:
			dataptr[index++] = Key1;				//Filler
			dataptr[index++] = Key2;				//Filler
			CopyMap(&dataptr[index],RumorMap,MAP_DITHER);	//This is a confirmation message. RumorMap contains data (stamp information) received
														//from MSG_MFCT_STAMP message.
			break;

		case MSG_SIDMAP_UPDATE:
			dataptr[index++] = Key1;				//Filler
			dataptr[index++] = Key2;				//Filler
			CopyMap(&dataptr[index],NewSIDMap,MAP_DITHER);
			break;

		case MSG_PROPAGATE:
			dataptr[7] = TargetSID;
			dataptr[8] = Key0;							//Filler
			dataptr[9] = Key1;							//Filler
			dataptr[10] = Key2;							//Filler
			CopyMap(&dataptr[11],RumorMap,MAP_DITHER);	//nodes that have not yet heard the rumour
			dataptr[5] |= URGENT_MSG_BIT;
			break;

		default:
			break;

	}

	CRCInit();
	for(uint8_t i=4; i<19; i++)
	{
		CRCByte(dataptr[i]);
	}

	MSGcrc = ~MSGcrc;
	dataptr[19] = (uint8_t)(MSGcrc>>8);
	dataptr[20] = (uint8_t)(MSGcrc&0x00FF);

#if SCRAMBLED_WISAFE
	// ABR Scramble
	if(Scramble0!=0)
	{
		MsgScramble(1);
	}
#endif
}



/*******************************************************************************
 * \brief	Copy message to rumour buffer.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

void CopyMsgToRumor(void)
{
	uint8_t i;
	uint8_t last;

	RumorMsgLen = msglen;			// we'll need this when copying back to msgbuf

	last = msglen>>3;
	for(i=0; i<=last; i++)
	{
		RumorBuf[i] = msgbuf[i];
	}
}



/*******************************************************************************
 * \brief	Copy rumour back.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

void CopyRumorToMsg(void)
{
	uint8_t i;
	uint8_t last;

	msglen = RumorMsgLen;	// number of bits in RumorBuf

	last = msglen>>3;
	for (i = 0;  i<=last;  i++)
	{
		msgbuf[i] = RumorBuf[i];
	}
}



/*******************************************************************************
 * \brief	Clear map.
 *
 * \param   *map. Pointer to map to be cleared.
 * \return	None.
 ******************************************************************************/

void ClearMap(volatile uint8_t *map)
{
	uint8_t i;

	for(i=0; i<sizeof(SIDMap); i++)
	{
		map[i] = 0;
	}
}



/*******************************************************************************
 * \brief	Copy map.
 *
 * \param   *map. Pointer to destination map.
 * \param	*source. Pointer to source map.
 * \param 	dither. Value to be exclusive ORed with the map.
 * \return	None.
 ******************************************************************************/

void CopyMap(volatile uint8_t *map, volatile uint8_t *source, volatile uint8_t dither)
{
	uint8_t i;

	for(i=0; i<sizeof(SIDMap); i++)
	{
		map[i] = source[i]^dither;
	}
}



/*******************************************************************************
 * \brief	Copy memory.
 *
 * \param   *dest. Pointer to destination memory.
 * \param	*src. Pointer to source memory.
 * \param 	len. Length of data block to be copied.
 * \return	None.
 ******************************************************************************/

void CopyMemory(volatile uint8_t *dest, volatile uint8_t *src, uint8_t len)
{
	while ((len--)>0)
	{
		*dest++ = *src++;
	}
}



/*******************************************************************************
 * \brief	Set a bit in a map.
 *
 * \param	*map. Pointer to a map.
 * \param 	id. Index of a Bit to be set.
 * \return	None.
 ******************************************************************************/

void SetBit(volatile uint8_t *map, uint8_t id)
{
	id &= SID_MASK;					/* to be safe */
	map[id>>3] |= (1<<(id&7));
}



/*******************************************************************************
 * \brief	Clear a bit in a map.
 *
 * \param	*map. Pointer to a map.
 * \param 	id. Index of a Bit to be cleared.
 * \return	None.
 ******************************************************************************/

void ClearBit(volatile uint8_t *map, uint8_t id)
{
	id &= SID_MASK;							// to be safe
	map[id>>3] &= ~(1<<(id&7));
}



/*******************************************************************************
 * \brief	Test a bit in a map.
 *
 * \param	*map. Pointer to a map.
 * \param 	id. Index of a Bit to be cleared.
 * \return	True, if specified bit set. False, if not.
 ******************************************************************************/

bool TestBit(volatile uint8_t *map, uint8_t id)
{
	// return TRUE if specified bit set
	return 	((map[id>>3] & (1<<(id&7))) != 0);
}



/*******************************************************************************
 * \brief	Count set bits in a map.
 *
 * \param	*map. Pointer to a map.
 * \return	1, if at least 1 bit set. 0, if no bit set.
 ******************************************************************************/

uint8_t CountBits(volatile uint8_t *map)
{
	uint8_t i;
	uint8_t bitset;

	bitset = 0;

	// return 1 if there is at least one bit set
	for(i=0; i<sizeof(SIDMap); i++)
	{
		if(map[i]!=0)
		{
			bitset = 1;
		}
	}

	return bitset;
}



/*******************************************************************************
 * \brief	Get sequence number.
 *
 * \param	SeqTTL. Byte containing the sequence number.
 * \return	Sequence number.
 ******************************************************************************/

uint8_t SeqField(uint8_t SeqTTL)
{
	return (SeqTTL>>4);
}



/*******************************************************************************
 * \brief	Patch message with old news.
 *
 * \note	If PREXMT is changed, patch locations and values will change.
 * \param	OldNews. Old news logic.
 * \return	None.
 ******************************************************************************/

#define ON_PATCH_BYTE_0 	4
#define ON_PATCH_MASK_0 	1
#define ON_PATCH_BYTE_1 	18
#define ON_PATCH_MASK_1 	0x03
#define ON_PATCH_BYTE_2 	19
#define ON_PATCH_MASK_2 	0x01
#define ON_PATCH_BYTE_3 	20
#define ON_PATCH_MASK_3 	0x0C

void PatchMsgWithOldNews (bool OldNews)
{
	if (OldNews ^ ((msgbuf[ON_PATCH_BYTE_0]&ON_PATCH_MASK_0)!=0))
	{
		// message has wrong "old news" bit setting
		msgbuf[ON_PATCH_BYTE_0] ^= ON_PATCH_MASK_0;
		msgbuf[ON_PATCH_BYTE_1] ^= ON_PATCH_MASK_1;
		msgbuf[ON_PATCH_BYTE_2] ^= ON_PATCH_MASK_2;
		msgbuf[ON_PATCH_BYTE_3] ^= ON_PATCH_MASK_3;
	}
}















