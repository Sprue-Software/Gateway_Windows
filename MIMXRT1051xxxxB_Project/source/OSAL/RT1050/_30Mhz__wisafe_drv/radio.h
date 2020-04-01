/*
 * radio.h
 *
 *  Created on: 27 Mar 2018
 *      Author: abenrashed
 */

#ifndef _RADIO_H_
#define _RADIO_H_


#define RADIO_LPSPI_MASTER_BASEADDR 			(LPSPI3)
#define FRAME_TRANSFER_NOT_COMPLETED			((RADIO_LPSPI_MASTER_BASEADDR->SR & 0x00000200) == 0)

#define RADIO_CHIP_SHUTDOWN_GPIO 			GPIO1
#define RADIO_CHIP_SHUTDOWN_GPIO_PIN 			(26U)
#define RADIO_SDN_ON					GPIO_PinWrite(RADIO_CHIP_SHUTDOWN_GPIO, RADIO_CHIP_SHUTDOWN_GPIO_PIN, 1)
#define RADIO_SDN_OFF					GPIO_PinWrite(RADIO_CHIP_SHUTDOWN_GPIO, RADIO_CHIP_SHUTDOWN_GPIO_PIN, 0)

#define RADIO_CHIP_NIRQ_GPIO 				GPIO1
#define RADIO_CHIP_NIRQ_GPIO_PIN 			(27U)
#define RADIO_CHIP_NIRQ_IRQ 				GPIO1_Combined_16_31_IRQn
#define RADIO_CHIP_NIRQ_IRQ_HANDLER 			GPIO1_Combined_16_31_IRQHandler

#define RADIO_CHIP_SELECT_GPIO 				GPIO1
#define RADIO_CHIP_SELECT_GPIO_PIN 			(28U)
#define RADIO_NSEL_ON					GPIO_PinWrite(RADIO_CHIP_SELECT_GPIO, RADIO_CHIP_SELECT_GPIO_PIN, 0)
#define RADIO_NSEL_OFF					GPIO_PinWrite(RADIO_CHIP_SELECT_GPIO, RADIO_CHIP_SELECT_GPIO_PIN, 1)

#define RADIO_CHIP_CTS_GPIO				GPIO3
#define RADIO_CHIP_CTS_GPIO_PIN 			(17U)

#define SYNC_WORD_DETECT_GPIO				GPIO3
#define SYNC_WORD_DETECT_GPIO_PIN			(16U)
#define SYNC_WORD_DETECT_IRQ				GPIO3_Combined_16_31_IRQn
#define SYNC_WORD_DETECT_IRQ_HANDLER			GPIO3_Combined_16_31_IRQHandler
#define SYNC_WORD_DETECT_IRQ_PRIORITY			1

// ABR
#define USER_BUTTON_GPIO				GPIO2
#define USER_BUTTON_GPIO_PIN				(19U)
#define USER_BUTTON_IRQ					GPIO2_Combined_16_31_IRQn
#define USER_BUTTON_IRQ_HANDLER				GPIO2_Combined_16_31_IRQHandler
#define USER_BUTTON_IRQ_PRIORITY			2

//#define DIRECT_MODE_TX_DATA_GPIO 		GPIO1
//#define DIRECT_MODE_TX_DATA_GPIO_PIN 	(10U)


#define NOCHANGESTATE					0x00
#define SLEEP_STATE					0x01
#define SPI_ACTIVE_STATE				0x02
#define READYSTATE					0x03
#define TX_TUNE_STATE					0x05
#define RX_TUNE_STATE					0x06
#define RXSTATE						0x08
#define TXSTATE						0x07

#define NO_RETRANSMIT					0x00
#define RETRANSMIT					0x04

#define CHANNEL_0					0x00
#define NO_RESPONSE_LEN					0x00
#define NO_PARAMETERS					0x00
#define Tx						0x01
#define Rx						0x02



#define POWER_UP					0x02
#define POWER_UP_COMMAND_LEN				0x06
#define POWER_UP_RESPONSE_LEN				0x00

#define SET_PROPERTY					0x11
#define SET_PROPERTY_COMMAND_LEN			0x04
#define SET_PROPERTY_RESPONSE_LEN			0x00

#define GPIO_PIN_CFG					0x13
#define GPIO_PIN_CFG_COMMAND_LEN			0x07
#define GPIO_PIN_CFG_RESPONSE_LEN			0x07

#define FIFO_INFO					0x15
#define FIFO_INFO_COMMAND_LEN				0x01
#define FIFO_INFO_RESPONSE_LEN				0x02

#define GET_INT_STATUS					0x20
#define GET_INT_STATUS_COMMAND_LEN	   		0x03
#define GET_INT_STATUS_RESPONSE_LEN			0x08

#define GET_PH_STATUS					0x21
#define GET_PH_STATUS_COMMAND_LEN	   		0x00
#define GET_PH_STATUS_RESPONSE_LEN			0x02

#define START_TX					0x31

#define START_RX					0x32

#define READ_CMD_BUFF					0x44

#define WRITE_TX_FIFO					0x66

#define READ_RX_FIFO					0x77

/////////////////////////////////////////////
//#define OK_MESSAGE_LEN	 			0x08
//#define INVITING_BIT					0x80
/////////////////////////////////////////////




//////////////////////////////////PROPERTIES///////////////////////////////////////////////////
typedef struct
{
  uint8_t group;
  uint8_t number;
  uint8_t data;
} tRadioProperties;

typedef struct
{
	uint8_t SID;
	uint8_t MeshKey0;
	uint8_t MeshKey1;
	uint8_t MeshKey2;
} tDevice;

//////////////////INT_CTL////////////////////////////////////
#define INT_CTL						0x01		//group

#define INT_CTL_ENABLE					0x00		//number
#define NO_INT_EN					0x00		//data
#define PH_INT_EN					0x01		//data
#define	MODEM_INT_EN					0x02		//data

#define INT_CTL_PH_ENABLE				0x01		//number
#define PACKET_RX_EN					0x10
#define PACKET_SENT_EN					0x20		//data


#define INT_CTL_MODEM_ENABLE				0x02		//number
#define SYNC_DETECT_EN					0x01		//data
#define PREAMBLE_DETECT_EN				0x02		//data



//////////////////PREAMBLE///////////////////////////////////
#define PREAMBLE					0x10		//group

#define PREAMBLE_CONFIG_STD_1				0x01		//number
#define SKIP_SYNC_TIMEOUT				0x80		//data
#define RX_THRESH					0x14		//data 20bits preamble.

#define PREAMBLE_CONFIG					0x04		//number
#define PREAM_FIRST_1					0x20		//data
#define STANDARD_PREAM					0x01		//data



//////////////////SYNC///////////////////////////////////////
#define SYNC						0x11		//group

#define SYNC_CONFIG					0x00		//number
#define SKIP_TX						0x80		//data
#define SYNC_1_BYTE					0x00		//data  0->1byte
#define SYNC_2_BYTE					0x01		//data  1->2bytes
#define SYNC_3_BYTE					0x02		//data  2->3bytes
#define SYNC_4_BYTE					0x03		//data  3->4bytes

#define SYNC_BITS_BYTE_1				0x01		//number
#define SYNC_BITS_BYTE_2				0x02		//number
#define SYNC_BITS_BYTE_3				0x03		//number
#define SYNC_BITS_BYTE_4				0x04		//number


/////////////////MODEM///////////////////////////////////////
#define MODEM_MOD_TYPE_GROUP				0x20
#define MODEM_MOD_TYPE_INDEX0				0x00
#define MOD_TYPE_CW					0x00
#define MOD_TYPE_2GFSK					0x03


/////////////////MODEM FREQUENCY///////////////////////////////////////
#define FREQ_CONTROL_FRAC_GROUP				0x40
#define FREQ_CONTROL_FRAC_INDEX1			0x01
#define FREQ_CONTROL_FRAC_INDEX2			0x02
#define FREQ_CONTROL_FRAC_INDEX3			0x03
#define FREQ_CONTROL_FRAC1_DEFAULT			0x0F       //0x0E	//0x4001 default value
#define FREQ_CONTROL_FRAC2_DEFAULT			0x17       //0xFC	//0x4002 default value
#define FREQ_CONTROL_FRAC3_DEFAULT			0xE4       //0x96	//0x4003 default value




#define NO_OF_RADIO_PROPERTIES 			175	//200


#define RADIO_PROPERTIES_INIT { 	{       0x00    ,       0x00    ,       0x52    }       ,       \
					{       0x00    ,       0x01    ,       0x00    }       ,       \
                                	{       0x00    ,       0x03    ,       0x20    }       ,       \
					{       0x01    ,       0x00    ,       0x00    }       ,       \
                                	{       0x01    ,       0x02    ,       0x00    }       ,       \
                                	{       0x02    ,       0x00    ,       0x00    }       ,       \
                                	{       0x02    ,       0x01    ,       0x00    }       ,       \
                                	{       0x02    ,       0x02    ,       0x00    }       ,       \
                                	{       0x02    ,       0x03    ,       0x00    }       ,       \
                                	{       0x22    ,       0x00    ,       0x20    }       ,       \
                                	{       0x22    ,       0x01    ,       0x7F    }       ,       \
                                	{       0x22    ,       0x02    ,       0x00    }       ,       \
                                	{       0x20    ,       0x00    ,       0x03    }       ,       \
                                	{       0x20    ,       0x01    ,       0x00    }       ,       \
                                	{       0x20    ,       0x02    ,       0x07    }       ,       \
                                	{       0x20    ,       0x51    ,       0x08    }       ,       \
					{       0x20    ,       0x4a    ,       0xFF    }       ,       \
                                	{       0x20    ,       0x4c    ,       0x00    }       ,       \
                                	{       0x10    ,       0x00    ,       0x00    }       ,       \
                                	{       0x10    ,       0x01    ,       0x00    }       ,       \
                                	{       0x10    ,       0x02    ,       0x00    }       ,       \
                                	{       0x10    ,       0x03    ,       0x0F    }       ,       \
                                	{       0x10    ,       0x04    ,       0x31    }       ,       \
                                	{       0x10    ,       0x05    ,       0x00    }       ,       \
                                	{       0x10    ,       0x06    ,       0x00    }       ,       \
                                	{       0x10    ,       0x07    ,       0x00    }       ,       \
                                	{       0x10    ,       0x08    ,       0x00    }       ,       \
                                	{       0x11    ,       0x00    ,       0x00    }       ,       \
                                	{       0x11    ,       0x01    ,       0x55    }       ,       \
                                	{       0x11    ,       0x02    ,       0x55    }       ,       \
                                	{       0x11    ,       0x03    ,       0x95    }       ,       \
                                	{       0x11    ,       0x04    ,       0x80    }       ,       \
                                	{       0x11    ,       0x05    ,       0x00    }       ,       \
                                	{       0x12    ,       0x00    ,       0x00    }       ,       \
                                	{       0x12    ,       0x03    ,       0xFF    }       ,       \
                                	{       0x12    ,       0x04    ,       0xFF    }       ,       \
                                	{       0x12    ,       0x05    ,       0x00    }       ,       \
                                	{       0x12    ,       0x06    ,       0x00    }       ,       \
                                	{       0x12    ,       0x08    ,       0x00    }       ,       \
                                	{       0x12    ,       0x09    ,       0x00    }       ,       \
                                	{       0x12    ,       0x0a    ,       0x00    }       ,       \
                                	{       0x12    ,       0x0b    ,       0x30    }       ,       \
                                	{       0x12    ,       0x0c    ,       0x30    }       ,       \
                                	{       0x12    ,       0x0d    ,       0x00    }       ,       \
                                	{       0x12    ,       0x0e    ,       0x00    }       ,       \
                                	{       0x12    ,       0x0f    ,       0x00    }       ,       \
                                	{       0x12    ,       0x10    ,       0x80    }       ,       \
                                	{       0x12    ,       0x11    ,       0x00    }       ,       \
                                	{       0x12    ,       0x12    ,       0x00    }       ,       \
                                	{       0x12    ,       0x13    ,       0x00    }       ,       \
                                	{       0x12    ,       0x14    ,       0x00    }       ,       \
                                	{       0x12    ,       0x15    ,       0x00    }       ,       \
                                	{       0x12    ,       0x16    ,       0x00    }       ,       \
                                	{       0x12    ,       0x17    ,       0x00    }       ,       \
                                	{       0x12    ,       0x18    ,       0x00    }       ,       \
                                	{       0x12    ,       0x19    ,       0x00    }       ,       \
                                	{       0x12    ,       0x1a    ,       0x00    }       ,       \
                                	{       0x12    ,       0x1b    ,       0x00    }       ,       \
                                	{       0x12    ,       0x1c    ,       0x00    }       ,       \
                                	{       0x12    ,       0x1d    ,       0x00    }       ,       \
                                	{       0x12    ,       0x1e    ,       0x00    }       ,       \
                                	{       0x12    ,       0x1f    ,       0x00    }       ,       \
                                	{       0x12    ,       0x20    ,       0x00    }       ,       \
                                	{       0x12    ,       0x21    ,       0x00    }       ,       \
                                	{       0x12    ,       0x22    ,       0x00    }       ,       \
                                	{       0x12    ,       0x23    ,       0x00    }       ,       \
                                	{       0x12    ,       0x24    ,       0x00    }       ,       \
                                	{       0x12    ,       0x25    ,       0x00    }       ,       \
                                	{       0x12    ,       0x26    ,       0x00    }       ,       \
                                	{       0x12    ,       0x27    ,       0x00    }       ,       \
                                	{       0x12    ,       0x28    ,       0x00    }       ,       \
                                	{       0x12    ,       0x29    ,       0x00    }       ,       \
                                	{       0x12    ,       0x2a    ,       0x00    }       ,       \
                                	{       0x12    ,       0x2b    ,       0x00    }       ,       \
                                	{       0x12    ,       0x2c    ,       0x00    }       ,       \
                                	{       0x12    ,       0x2d    ,       0x00    }       ,       \
                                	{       0x12    ,       0x2e    ,       0x00    }       ,       \
                                	{       0x12    ,       0x2f    ,       0x00    }       ,       \
                                	{       0x12    ,       0x30    ,       0x00    }       ,       \
                                	{       0x12    ,       0x31    ,       0x00    }       ,       \
                                	{       0x12    ,       0x32    ,       0x00    }       ,       \
                                	{       0x12    ,       0x33    ,       0x00    }       ,       \
                                	{       0x12    ,       0x34    ,       0x00    }       ,       \
                                	{       0x30    ,       0x00    ,       0x00    }       ,       \
                                	{       0x30    ,       0x01    ,       0x00    }       ,       \
                                	{       0x30    ,       0x02    ,       0x00    }       ,       \
                                	{       0x30    ,       0x03    ,       0x00    }       ,       \
                                	{       0x30    ,       0x04    ,       0x00    }       ,       \
                                	{       0x30    ,       0x05    ,       0x00    }       ,       \
                                	{       0x30    ,       0x06    ,       0x00    }       ,       \
                                	{       0x30    ,       0x07    ,       0x00    }       ,       \
                                	{       0x30    ,       0x08    ,       0x00    }       ,       \
                                	{       0x30    ,       0x09    ,       0x00    }       ,       \
                                	{       0x30    ,       0x0a    ,       0x00    }       ,       \
                                	{       0x30    ,       0x0b    ,       0x00    }       ,       \
                                	{       0x20    ,       0x03    ,       0x13    }       ,       \
                                	{       0x20    ,       0x04    ,       0x88    }       ,       \
                                	{       0x20    ,       0x05    ,       0x00    }       ,       \
                                	{       0x20    ,       0x06    ,       0x09    }       ,       \
                                	{       0x20    ,       0x07    ,       0xC9    }       ,       \
                                	{       0x20    ,       0x08    ,       0xC3    }       ,       \
                                	{       0x20    ,       0x09    ,       0x80    }       ,       \
                                	{       0x20    ,       0x0a    ,       0x00    }       ,       \
                                	{       0x20    ,       0x0b    ,       0x02    }       ,       \
                                	{       0x20    ,       0x0c    ,       0xBB    }       ,       \
                                	{       0x20    ,       0x18    ,       0x01    }       ,       \
                                	{       0x22    ,       0x03    ,       0x1D    }       ,       \
                                	{       0x40    ,       0x00    ,       0x38    }       ,       \
                                	{       0x40    ,       0x01    ,       0x0F    }       ,       \
                                	{       0x40    ,       0x02    ,       0x17    }       ,       \
                                	{       0x40    ,       0x03    ,       0xE4    }       ,       \
                                	{       0x40    ,       0x04    ,       0x22    }       ,       \
                                	{       0x40    ,       0x05    ,       0x22    }       ,       \
                                	{       0x40    ,       0x06    ,       0x20    }       ,       \
                                	{       0x40    ,       0x07    ,       0xFF    }       ,       \
                                	{       0x20    ,       0x19    ,       0x00    }       ,       \
                                	{       0x20    ,       0x1a    ,       0x08    }       ,       \
                                	{       0x20    ,       0x1b    ,       0x03    }       ,       \
                                	{       0x20    ,       0x1c    ,       0xC0    }       ,       \
                                	{       0x20    ,       0x1d    ,       0x00    }       ,       \
                                	{       0x20    ,       0x1e    ,       0x10    }       ,       \
                                	{       0x20    ,       0x1f    ,       0x10    }       ,       \
                                	{       0x20    ,       0x22    ,       0x00    }       ,       \
                                	{       0x20    ,       0x23    ,       0x4E    }       ,       \
                                	{       0x20    ,       0x24    ,       0x06    }       ,       \
                                	{       0x20    ,       0x25    ,       0x8D    }       ,       \
                                	{       0x20    ,       0x26    ,       0xB9    }       ,       \
                                	{       0x20    ,       0x27    ,       0x07    }       ,       \
                                	{       0x20    ,       0x28    ,       0xFF    }       ,       \
                                	{       0x20    ,       0x29    ,       0x02    }       ,       \
                                	{       0x20    ,       0x2a    ,       0x00    }       ,       \
                                	{       0x20    ,       0x2c    ,       0x00    }       ,       \
                                	{       0x20    ,       0x2d    ,       0x12    }       ,       \
                                	{       0x20    ,       0x2e    ,       0x81    }       ,       \
                                	{       0x20    ,       0x2f    ,       0x18    }       ,       \
                                	{       0x20    ,       0x30    ,       0x01    }       ,       \
                                	{       0x20    ,       0x31    ,       0xAC    }       ,       \
                                	{       0x20    ,       0x32    ,       0xA0    }       ,       \
                                	{       0x20    ,       0x35    ,       0xE0    }       ,       \
                                	{       0x20    ,       0x38    ,       0x11    }       ,       \
                                	{       0x20    ,       0x39    ,       0x11    }       ,       \
                                	{       0x20    ,       0x3a    ,       0x11    }       ,       \
                                	{       0x20    ,       0x3b    ,       0x80    }       ,       \
                                	{       0x20    ,       0x3c    ,       0x1A    }       ,       \
                                	{       0x20    ,       0x3d    ,       0x28    }       ,       \
                                	{       0x20    ,       0x3e    ,       0x00    }       ,       \
                                	{       0x20    ,       0x3f    ,       0x00    }       ,       \
                                	{       0x20    ,       0x40    ,       0x28    }       ,       \
                                	{       0x20    ,       0x42    ,       0xA4    }       ,       \
                                	{       0x20    ,       0x43    ,       0x23    }       ,       \
                                	{       0x20    ,       0x50    ,       0x84    }       ,       \
                                	{       0x20    ,       0x45    ,       0x03    }       ,       \
                                	{       0x20    ,       0x46    ,       0x00    }       ,       \
                                	{       0x20    ,       0x47    ,       0xA6    }       ,       \
                                	{       0x20    ,       0x48    ,       0x01    }       ,       \
                                	{       0x20    ,       0x49    ,       0x00    }       ,       \
                                	{       0x20    ,       0x4e    ,       0x40    }       ,       \
                                	{       0x21    ,       0x00    ,       0xCC    }       ,       \
                                	{       0x21    ,       0x01    ,       0xA1    }       ,       \
                                	{       0x21    ,       0x02    ,       0x30    }       ,       \
                                	{       0x21    ,       0x03    ,       0xA0    }       ,       \
                                	{       0x21    ,       0x04    ,       0x21    }       ,       \
                                	{       0x21    ,       0x05    ,       0xD1    }       ,       \
                                	{       0x21    ,       0x06    ,       0xB9    }       ,       \
                                	{       0x21    ,       0x07    ,       0xC9    }       ,       \
                                	{       0x21    ,       0x08    ,       0xEA    }       ,       \
                                	{       0x21    ,       0x09    ,       0x05    }       ,       \
                                	{       0x21    ,       0x0a    ,       0x12    }       ,       \
                                	{       0x21    ,       0x0b    ,       0x11    }       ,       \
                                	{       0x21    ,       0x0c    ,       0x0A    }       ,       \
                                	{       0x21    ,       0x0d    ,       0x04    }       ,       \
                                	{       0x21    ,       0x0e    ,       0x15    }       ,       \
                                	{       0x21    ,       0x0f    ,       0xFC    }       ,       \
                                	{       0x21    ,       0x10    ,       0x03    }       ,       \
                                	{       0x21    ,       0x11    ,       0x00    } }


#define WISAFE_HISTORY_SIZE           50

#ifdef _RADIO_C_
#undef _RADIO_C_

#include "board.h"

//ABR added for testing
uint8_t WiSafe_RM_Task_Point[WISAFE_HISTORY_SIZE];
uint8_t WiSafe_RM_Task_Point_Index = 0;

bool RadioInContinuousTransmit = false;
uint8_t MeshKey[3];
uint8_t NetworkMeshID;
uint8_t LongMsgFirstByte;
uint8_t MediumMsgFirstByte;
//uint8_t GateWayID[3];

#if SCRAMBLED_WISAFE
volatile uint8_t Scramble0;
#endif
volatile uint8_t TargetSID;
volatile uint8_t RumorSenderSID;
volatile uint8_t RumorSeqNumAndTTL;
volatile uint8_t RumorSeqNum;
volatile uint8_t RumorTTL;
volatile uint8_t ReceivedSeqNumAndTTL;
volatile uint8_t ReceivedSeqNum;
volatile uint8_t ReceivedTTL;
volatile uint8_t msglen;

volatile uint8_t rcvbuf[RCVBUFLEN];
uint8_t msgbuf[MSGBUFLEN];
#if SCRAMBLED_WISAFE
uint8_t dummymsgbuf[MSGBUFLEN];
#endif

uint8_t GetSID(void);
uint8_t* GetKeys(void);
void Init_Radio(void);
#if SCRAMBLED_WISAFE
void Sleep_Radio(void);
#endif
void SetRadioProperty (uint8_t group, uint8_t number, uint8_t data);
void Set_RadioInterruptStatus(void);
void Set_SyncDetected(void);
bool JoinNetwork(void);
bool ReceiveInviteMessage(void);
void SendJoinRequestMessage(void);
bool ReceiveJoinAckMessage(void);
void SendOkMessage(void);
uint8_t RceiveOkMessage(uint32_t TimeLeft);
bool ReceiveRumourAckMessage(uint8_t id);
bool ReceivePropagateAckMessage(uint8_t id);
void Chirp(void);
bool ChirpToReceive(void);
bool ListenToTransmitPacket(uint16_t TimeLeft);
void UploadMessage(uint8_t len);
void DownloadMessage(uint8_t len);
void RadioContinuousTransmit(void);
#if SCRAMBLED_WISAFE
void MsgScramble(uint8_t type);
#endif

#else

//ABR added for testing
extern uint8_t WiSafe_RM_Task_Point[WISAFE_HISTORY_SIZE];
extern uint8_t WiSafe_RM_Task_Point_Index;

extern bool RadioInContinuousTransmit;
extern uint8_t MeshKey[3];
extern uint8_t NetworkMeshID;
extern uint8_t LongMsgFirstByte;
extern uint8_t MediumMsgFirstByte;
//extern uint8_t GateWayID[3];

#if SCRAMBLED_WISAFE
extern volatile uint8_t Scramble0;
#endif
extern volatile uint8_t TargetSID;
extern volatile uint8_t RumorSenderSID;
extern volatile uint8_t RumorSeqNumAndTTL;
extern volatile uint8_t RumorSeqNum;
extern volatile uint8_t RumorTTL;
extern volatile uint8_t ReceivedSeqNumAndTTL;
extern volatile uint8_t ReceivedSeqNum;
extern volatile uint8_t ReceivedTTL;
extern volatile uint8_t msglen;

extern volatile uint8_t rcvbuf[];
extern uint8_t msgbuf[];
#if SCRAMBLED_WISAFE
extern uint8_t dummymsgbuf[];
#endif


extern uint8_t GetSID(void);
extern uint8_t* GetKeys(void);
extern void Init_Radio(void);
#if SCRAMBLED_WISAFE
extern void Sleep_Radio(void);
#endif
extern void SetRadioProperty (uint8_t group, uint8_t number, uint8_t data);
extern void Set_RadioInterruptStatus(void);
extern void Set_SyncDetected(void);
extern bool JoinNetwork(void);
extern bool ReceiveInviteMessage(void);
extern void SendJoinRequestMessage(void);
extern bool ReceiveJoinAckMessage(void);
extern void SendOkMessage(void);
extern uint8_t RceiveOkMessage(uint32_t TimeLeft);
extern bool ReceiveRumourAckMessage(uint8_t id);
extern bool ReceivePropagateAckMessage(uint8_t id);
extern void Chirp(void);
extern bool ChirpToReceive(void);
extern bool ListenToTransmitPacket(uint16_t TimeLeft);
extern void UploadMessage(uint8_t len);
extern void DownloadMessage(uint8_t len);
extern void RadioContinuousTransmit(void);
#if SCRAMBLED_WISAFE
extern void MsgScramble(uint8_t type);
#endif


#endif // _RADIO_C_

#endif // _RADIO_H_
