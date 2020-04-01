/*******************************************************************************
 * \file	SDcomms.c
 * \brief 	Receive, transmit, and processing uart serial messages.
 * \note 	Project: 	WG2. Low cost wireless gateway
 * \author  Abdul Ben-Rashed
 ******************************************************************************/

#define _SDCOMMS_C_

#include "C:/Users/ndiwathe/Documents/MCUXpressoIDE_11.1.1_3241/workspace/EnsoAgent/source/OSAL/RT1050/wisafe_drv/wisafe_main.h"
#include "fsl_gpio.h"
#include "fsl_debug_console.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "SDcomms.h"
#include "fsl_lpuart.h"
#include <timer.h>
#include "messages.h"
#include "radio.h"
#include "logic.h"

#include "LOG_Api.h"


/*******************************************************************************
* Definitions
******************************************************************************/
/* UART1 related */
#define DEBUG_LPUART 			LPUART1
#define DEBUG_LPUART_CLK_FREQ 	BOARD_DebugConsoleSrcFreq()
#define DEBUG_LPUART_IRQn 		LPUART1_IRQn
#define DEBUG_LPUART_IRQHandler LPUART1_IRQHandler

/*******************************************************************

	local procedures

*******************************************************************/

static void ProcessIncomingMsg(void);
static void SaveIncomingByte(uint8_t IncomingByte);
static void ProcessIncomingByte(void);
static void SendOutgoingRawByte(uint8_t OutgoingByte);
static void SendOutgoingMsgByte(uint8_t OutgoingMsgByte);
static void SendOutgoingMsgEnd(void);
static void SendOutgoingMsgBlock (volatile uint8_t *BlockPtr, uint8_t Count);
static void ProcessIncomingByteRaw(uint8_t data);


static uint8_t SerialData;

#if TEST_WISAFE_DRIVER
/*******************************************************************************
 * \brief	LPUART interrupt handler.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

void DEBUG_LPUART_IRQHandler(void)
{
    // If new data arrived.
    if ((kLPUART_RxDataRegFullFlag)&LPUART_GetStatusFlags(DEBUG_LPUART))
    {
    	SerialData = DEBUG_LPUART->DATA;
    	ProcessIncomingByte();
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}


/*******************************************************************************
 * \brief	Initialise LPUART.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

void Init_Uart(void)
{
	lpuart_config_t config;

    /*
     * config.baudRate_Bps = 115200U;
     * config.parityMode = kLPUART_ParityDisabled;
     * config.stopBitCount = kLPUART_OneStopBit;
     * config.txFifoWatermark = 0;
     * config.rxFifoWatermark = 0;
     * config.enableTx = false;
     * config.enableRx = false;
    */
	LPUART_GetDefaultConfig(&config);
	config.baudRate_Bps = (19200U);					//BOARD_DEBUG_UART_BAUDRATE;
	config.enableTx = true;
	config.enableRx = true;

	LPUART_Init(DEBUG_LPUART, &config, DEBUG_LPUART_CLK_FREQ);

	// Enable RX interrupt.
	LPUART_EnableInterrupts(DEBUG_LPUART, kLPUART_RxDataRegFullInterruptEnable);
	EnableIRQ(DEBUG_LPUART_IRQn);
}



/*******************************************************************************
 * \brief	Test LPUART.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

void TestUart(void)
{
	// Send data only when LPUART TX register is empty and ring buffer has data to send out.
	while( !(kLPUART_TxDataRegEmptyFlag & LPUART_GetStatusFlags(DEBUG_LPUART)) );
	LPUART_WriteByte(DEBUG_LPUART, 'T');
	while( !(kLPUART_TxDataRegEmptyFlag & LPUART_GetStatusFlags(DEBUG_LPUART)) );
	LPUART_WriteByte(DEBUG_LPUART, 'E');
	while( !(kLPUART_TxDataRegEmptyFlag & LPUART_GetStatusFlags(DEBUG_LPUART)) );
	LPUART_WriteByte(DEBUG_LPUART, 'S');
	while( !(kLPUART_TxDataRegEmptyFlag & LPUART_GetStatusFlags(DEBUG_LPUART)) );
	LPUART_WriteByte(DEBUG_LPUART, 'T');
}
#endif


/*******************************************************************************
 * \brief	Process incoming message.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

static void ProcessIncomingMsg(void)
{
	//SDMsgCnt++;	/* a message has been received */

	// ???? Unit Test length changed from 6 to 8 in 12/11
	if( ((IncomingMsgLen==6) || (IncomingMsgLen==8)) && (IncomingMsg[0]==SPIMSG_UNIT_TEST) )
	{
		// treat as a transparent message
		PassThruMsgLen = IncomingMsgLen;
		CopyMap(PassThruMsg,IncomingMsg,0);					// don't dither

		// but also triggers inviting additional nodes
		SDMsgRcvd = SPIMSG_UNIT_TEST;
	}
	// ???? AlarmIdent length changed from 7 to 8 in 8/11, and to 9 in 2/12
	else if( (IncomingMsgLen>=7) && (IncomingMsg[0]==SPIMSG_ALARM_IDENT) )
	{
		SDMsgRcvd = SPIMSG_ALARM_IDENT;
		// record 3 byte ID and 2 byte model
		GateWayID[0] = IncomingMsg[1];						// remember Alarm identity
		GateWayID[1] = IncomingMsg[2];
		GateWayID[2] = IncomingMsg[3];
		SDModel[0] = IncomingMsg[4];						// and model
		SDModel[1] = IncomingMsg[5];
		SDAlarmIdentValid = true;
	}
	else if( (IncomingMsgLen==1) && ( (IncomingMsg[0]==SPIMSG_RM_DIAG_REQ) || (IncomingMsg[0]==SPIMSG_RM_CLEAR_CMD)
								      || (IncomingMsg[0]==SPIMSG_RM_RESET_CMD) || (IncomingMsg[0]==SPIMSG_RM_ID_REQ) ) )
	{
		// various 1-byte messages to be analysed later
		SDMsgRcvd = IncomingMsg[0];
	}
	else if( (IncomingMsgLen==1) && ( (IncomingMsg[0]==SPIMSG_RM_INC_FREQ) || (IncomingMsg[0]==SPIMSG_RM_DEC_FREQ)
									   || (IncomingMsg[0]==SPIMSG_RM_EXIT_FREQ) || (IncomingMsg[0]==SPIMSG_RM_SAVE_FREQ)
									   || (IncomingMsg[0]==SPIMSG_RM_FINE_INC_FREQ) || (IncomingMsg[0]==SPIMSG_RM_FINE_DEC_FREQ) ) )
	{
		if(RadioInContinuousTransmit)							//In continuous transmit mode
		{
			SDMsgRcvd = IncomingMsg[0];
		}
	}
	else if( (IncomingMsgLen==4) && (IncomingMsg[0]==SPIMSG_RM_MFCT_MODE) )
	{
		if( (IncomingMsg[1]=='M') && (IncomingMsg[2]=='F') && (IncomingMsg[3]=='T'))
		{
			SDMsgRcvd = IncomingMsg[0];
		}
		else
		{
			SDMsgRcvd = SPIMSG_INVALID;
		}
	}
	else if( (IncomingMsgLen==4) && (IncomingMsg[0]==SPIMSG_RM_OPER_MODE) )
	{
		if( (IncomingMsg[1]=='O') && (IncomingMsg[2]=='P') && (IncomingMsg[3]=='R'))
		{
			SDMsgRcvd = IncomingMsg[0];
		}
		else
		{
			SDMsgRcvd = SPIMSG_INVALID;
		}
	}
	else if(IncomingMsg[0]==SPIMSG_RM_EXTENDED_REQ)
	{
		// three parameter bytes to save
		ExtendedMsgLen = IncomingMsgLen;
		ExtendedArg1 = IncomingMsg[1];
		ExtendedArg2 = IncomingMsg[2];
		ExtendedArg3 = IncomingMsg[3];
		if(ExtendedArg1==SPIMSG_EXTD_OPT_SIDMAP_UPDATE)
		{
			// remember new SIDMap
			CopyMap(NewSIDMap,&IncomingMsg[2],0);
		}
		SDMsgRcvd = IncomingMsg[0];							// valid extended diagnostic
	}
	else if (IncomingMsg[0]==SPIMSG_RM_PROD_TST_CMD)
	{
		// one parameter byte to save
		ExtendedMsgLen = IncomingMsgLen;
		ExtendedArg1 = IncomingMsg[1];
		SDMsgRcvd = IncomingMsg[0];							// valid extended diagnostic
	}
	else if (IncomingMsgLen<=9)
	{
		// assume it is a transparent message
		PassThruMsgLen = IncomingMsgLen;
		CopyMap(PassThruMsg,IncomingMsg,0);					// don't dither
		PassThruMsg[8] = IncomingMsg[8];					// ninth byte might be valid
		SDMsgRcvd = SPIMSG_TRANSPARENT;
	}
	else
	{
		SDMsgRcvd = SPIMSG_INVALID;
	}

	// done analysing message

	IncomingMsgLen = 0;										// now ready for next message
}



/*******************************************************************************
 * \brief	Save incoming byte.
 *
 * \param   IncomingByte. Byte to be saved.
 * \return	None.
 ******************************************************************************/

static void SaveIncomingByte(uint8_t IncomingByte)
{
	// save incoming message byte
	if (IncomingMsgLen<SPIMSGSIZE)
	{
		IncomingMsg[IncomingMsgLen++] = IncomingByte;			// append byte to message
	}
	else
	{
		// incoming message too long - maybe RM crashed and rebooted?
		// note - could take some action in this case...
		IncomingMsgLen = 0;										// throw away whatever is in our buffer
	}
}



/*******************************************************************************
 * \brief	Process incoming byte.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

static void ProcessIncomingByte(void)
{
	// we received a byte in SSPBUF

	if (IncomingEscapeState)
	{
		// we are in escape state - previous incoming byte was ESC
		SaveIncomingByte((SerialData==1)? FLAG: ESC);					// {ESC,1} means FLAG, {ESC,2} means ESC
		IncomingEscapeState = false;									// no longer in escape state
	}
	else
	{
		if (SerialData==ESC)
		{
			IncomingEscapeState = true;									// ESC received means we enter escape state
		}
		else if (SerialData==FLAG)
		{
			ProcessIncomingMsg();										// FLAG received means we have end of message
		}
		else
		{
			SaveIncomingByte(SerialData);								// here is next incoming byte
		}
	}
}



/*******************************************************************************
 * \brief	Send out byte.
 *
 * \param   OutgoingByte. Byte to be sent.
 * \return	None.
 ******************************************************************************/

static void SendOutgoingRawByte(uint8_t OutgoingByte)
{
	static uint8_t index;

#if TEST_WISAFE_DRIVER
	while( !(kLPUART_TxDataRegEmptyFlag & LPUART_GetStatusFlags(DEBUG_LPUART)) );		// send to uart
	LPUART_WriteByte(DEBUG_LPUART, OutgoingByte);
#endif

	OutgoingMsgRaw[index++] = OutgoingByte;
	if(OutgoingByte==FLAG)
	{
		index = 0;
		writeWGqueue(OutgoingMsgRaw);
	}
	else if(index>=sizeof(OutgoingMsgRaw))
	{
		index = 0;
		LOG_Info("Warning, buffer overflow.");
	}
}



/*******************************************************************************
 * \brief	Send byte to buffer.
 *
 * \param   OutgoingMsgByte. Byte to be sent.
 * \return	None.
 ******************************************************************************/

static void SendOutgoingMsgByte(uint8_t OutgoingMsgByte)
{
	if (OutgoingMsgByte==FLAG)
	{
		// must send {ESC, 0x01} instead of FLAG
		SendOutgoingRawByte(ESC);
		SendOutgoingRawByte(0x01);
	}
	else if (OutgoingMsgByte==ESC)
	{
		// must send {ESC, 0x02} instead of ESC
		SendOutgoingRawByte(ESC);
		SendOutgoingRawByte(0x02);
	}
	else
	{
		SendOutgoingRawByte(OutgoingMsgByte);	// OK to send without escape
	}
}



/*******************************************************************************
 * \brief	Send end of message byte.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

static void SendOutgoingMsgEnd(void)
{
	SendOutgoingRawByte(FLAG);
}



/*******************************************************************************
 * \brief	Send a message block.
 *
 * \param   *BlockPtr. Pointer to the block to be sent.
 * \return	Count. Number of bytes.
 ******************************************************************************/

void SendOutgoingMsgBlock(volatile uint8_t *BlockPtr, uint8_t Count)
{
	while ((Count--)>0)
	{
		SendOutgoingMsgByte(*BlockPtr++);
	}
}



/*******************************************************************************
 * \brief	Send out a message.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

void SendOutgoingMsg(void)
{
	SendOutgoingMsgBlock(OutgoingMsg,OutgoingMsgLen);
	SendOutgoingMsgEnd();
}



/*******************************************************************************
 * \brief	Send status request message.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

void SendStatusRequestMsg(void)
{
	SendOutgoingMsgByte(SPIMSG_STATUS);
	SendOutgoingMsgEnd();
	StatusRequestTime = SysTime;	/* make note of time when we ask SD status */
}



/*******************************************************************************
 * \brief	Send ACK message.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

void SendAckMsg(void)
{
	SendOutgoingMsgByte(SPIMSG_ACK);
	SendOutgoingMsgEnd();
}



/*******************************************************************************
 * \brief	Send NACK message.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

void SendNackMsg (void)
{
	SendOutgoingMsgByte(SPIMSG_NACK);
	SendOutgoingMsgEnd();
}



/*******************************************************************************
 * \brief	Send diagnostic result message.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

void SendDiagResultMsg(void)
{
	SendOutgoingMsgByte(SPIMSG_RM_DIAG_RESULT);				// RM Diagnostic Result
	SendOutgoingMsgByte(32);								// send word squeezed into byte, unloaded battery voltage
	SendOutgoingMsgByte(32);								// send word squeezed into byte, Loaded battery voltage
	SendOutgoingMsgByte(32);								// send word squeezed into byte
	SendOutgoingMsgByte(100);								// report RSSI last time we read it from a node in our mesh
	SendOutgoingMsgByte(FWREV);
	SendOutgoingMsgByte(GateWayID[0]);
	SendOutgoingMsgByte(GateWayID[1]);
	SendOutgoingMsgByte(GateWayID[2]);
	SendOutgoingMsgByte(0);									// critical faults
	SendOutgoingMsgByte(0);									// number of radio failures we've seen
	SendOutgoingMsgByte(SID);								// need to report SID in some message!
	SendOutgoingMsgByte(7);									// SID of device whose RSSI is in this message
	// end message with a FLAG
	SendOutgoingMsgEnd();
}



/*******************************************************************************
 * \brief	Send RM ID response message.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

void SendRMIdResponseMsg(void)
{
	SendOutgoingMsgByte(SPIMSG_RM_ID_RESPONSE);				// RM ID
	SendOutgoingMsgByte(0);									// No RM ID needed. Use all zeroes
	SendOutgoingMsgByte(0);
	SendOutgoingMsgByte(0);
	SendOutgoingMsgByte(MeshKey[0]);
	SendOutgoingMsgByte(MeshKey[1]);
	SendOutgoingMsgByte(MeshKey[2]);
	SendOutgoingMsgEnd();									// mark end of message
}



/*******************************************************************************
 * \brief	Send RM extended response message.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

void SendRMExtendedResponseMsg(void)
{
	if(ExtendedMsgLen<2)
	{
		// need a parameter
		SendNackMsg();
	}
	else
	{
		switch (ExtendedArg1)
		{
			case SPIMSG_EXTD_OPT_SIDMAP:
				// SID map report
				SendOutgoingMsgByte(SPIMSG_RM_EXTENDED_RESPONSE);
				SendOutgoingMsgByte(SPIMSG_EXTD_OPT_SIDMAP);
				SendOutgoingMsgBlock(SIDMap,sizeof(SIDMap));
				break;

			case SPIMSG_EXTD_OPT_SIDMAP_UPDATE:
				// SIDMap update
				ReasonsForRumor |= REASON_SIDMAP_UPDATE;		// SIDMap was already updated when SPI message received
				SendOutgoingMsgByte(SPIMSG_ACK);
				break;

			case SPIMSG_EXTD_OPT_REMOTE_STATUS:
				// remote status report
				ExtendedArg2 &= SID_MASK;						// for safety
				if(TestBit(SIDMap,ExtendedArg2))
				{
					StatusRequestTargetSID = ExtendedArg2;		// ???? what if StatusRequestTargetSID already in use ????
					ReasonsForRumor |= REASON_STATUS_REQUEST;
					MapMsgType = ExtendedArg3;					// save remote status type here ????
					SendOutgoingMsgByte(SPIMSG_ACK);			// remote status report will come later
				}
				else
				{
					SendOutgoingMsgByte(SPIMSG_NACK);			// requested node not in mesh!
				}
				break;

			case SPIMSG_EXTD_OPT_REMOTE_MAP:
				// request a map from a remote node
				ExtendedArg2 &= SID_MASK;						// for safety
				if(TestBit(SIDMap,ExtendedArg2))
				{
					MapRequestTargetSID = ExtendedArg2;			// ???? what if MapRequestTargetSID already in use ????
					ReasonsForRumor |= REASON_MAP_REQUEST;
					MapMsgType = ExtendedArg3;					// save map type here
					SendOutgoingMsgByte(SPIMSG_ACK);			// remote status report will come later
				}
				else
				{
					SendOutgoingMsgByte(SPIMSG_NACK);			// requested node not in mesh!
				}
				break;

			case SPIMSG_EXTD_OPT_BUTTON_PRESS:
				// simulate button presses
				SendOutgoingMsgByte(SPIMSG_ACK);
				SimulatedButtonPressCount = ExtendedArg2;		// as if this many button presses
				break;

			case SPIMSG_EXTD_OPT_SD_RUMOR_TARGET:
				// request that next SD rumour goes to single target
				// note - if ExtendedArg2 == NULL_SID, then no target is specified
				SDRumorTargetSID = NULL_SID;					// assume no target for next SD rumour
				if (ExtendedArg2<=SID_MASK)
				{
					SDRumorTargetSID = ExtendedArg2;			// target for next SD rumour
				}
				// respond with message that includes ReasonsForRumor
				SendOutgoingMsgByte(SPIMSG_RM_EXTENDED_RESPONSE);
				SendOutgoingMsgByte(SPIMSG_EXTD_OPT_SD_RUMOR_TARGET);	// reasons response
				SendOutgoingMsgByte(SDRumorTargetSID);					// target just set, if any
				SendOutgoingMsgByte((uint8_t) (ReasonsForRumor>>8));		// high byte
				SendOutgoingMsgByte((uint8_t) (ReasonsForRumor));			// low byte
				break;

			default:
				// should send a NACK
				SendOutgoingMsgByte(SPIMSG_NACK);
				break;
		}
		SendOutgoingMsgEnd();									// mark end of message
	}
}



/*******************************************************************************
 * \brief	Send RM production test response message.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

void SendRMProdTstResponseMsg(void)
{
	if(ExtendedMsgLen<2)
	{
		// need a parameter
		SendNackMsg();
	}
	else
	{
		switch (ExtendedArg1)
		{
			case 0:
				// stop previous test
				SendNackMsg();
				break;

			case 1:
				// Comms check
				SendOutgoingMsgByte(SPIMSG_RM_PROD_TST_RESPONSE);
				SendOutgoingMsgByte(ExtendedArg1);
				SendOutgoingMsgEnd();
				break;

			case 5:
				// continuous transmit
				SendAckMsg();
				RadioContinuousTransmit();
				break;

			case 6:
				// continuous receive
				SendAckMsg();
				break;

			case 7:
				SendAckMsg();

				MeshKey[0] = RESERVED_MM0;
				MeshKey[1] = RESERVED_MM1;
				MeshKey[2] = RESERVED_MM2;

				SID = 1;
				SIDMap[0] = 0xFF;									// 8 units in the network
				NetworkMeshID = MeshKey[0] & 0x3F;
				TrueMID = NetworkMeshID;

				UpdateKeys(&MeshKey[0]);
				LongMsgFirstByte = (NetworkMeshID<<2)+MSG_LEN_LONG;
				MediumMsgFirstByte = (NetworkMeshID<<2)+MSG_LEN_MEDIUM;
#if SCRAMBLED_WISAFE
				// ABR Scramble
				Scramble0 = MeshKey[1];
#endif

				SysTime = 0;
				break;

			default:
				SendNackMsg();
				break;
		}
	}
}



/*******************************************************************************
 * \brief	Process incoming message.
 *
 * \param   *ptr. Pointer to buffer of data to be processed.
 * \return	None.
 ******************************************************************************/

void ProcessIncomingMsgRaw(uint8_t *ptr)
{
	uint8_t data,index;

	index = 0;
	do
	{
		data = ptr[index++];
		ProcessIncomingByteRaw(data);
	}while( (data != 0x7E) && (index<sizeof(IncomingMsgRaw)) );
}



/*******************************************************************************
 * \brief	Process incoming byte.
 *
 * \param   data. Byte to be processed.
 * \return	None.
 ******************************************************************************/

static void ProcessIncomingByteRaw(uint8_t data)
{
	if (IncomingEscapeState)
	{
		// we are in escape state - previous incoming byte was ESC
		SaveIncomingByte((data==1)? FLAG: ESC);						// {ESC,1} means FLAG, {ESC,2} means ESC
		IncomingEscapeState = false;								// no longer in escape state
	}
	else
	{
		if (data==ESC)
		{
			IncomingEscapeState = true;								// ESC received means we enter escape state
		}
		else if (data==FLAG)
		{
			ProcessIncomingMsg();									// FLAG received means we have end of message
		}
		else
		{
			SaveIncomingByte(data);									// here is next incoming byte
		}
	}
}










