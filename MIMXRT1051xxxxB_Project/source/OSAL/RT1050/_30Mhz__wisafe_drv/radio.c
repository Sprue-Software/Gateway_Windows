/*******************************************************************************
 * \file	radio.c
 * \brief 	Initialisation and configuration of Si4461C2 radio chip. Handling all
 * 			Wisafe-2 communication with radio.
 * \note 	Project: 	WG2. Low cost wireless gateway
 * \author  Abdul Ben-Rashed
 ******************************************************************************/

#define _RADIO_C_

#include <wisafe_main.h>
#include "fsl_gpio.h"
#include "fsl_debug_console.h"
#include "fsl_lpspi.h"
#include "radio.h"
#include <timer.h>
#include "messages.h"
#include "logic.h"
#include "SDcomms.h"
#include "LOG_Api.h"

#define CRC_CHECK		0x800D
#define RADIOBUFLEN		32

static gpio_pin_config_t radiochipshutdownConfig = {kGPIO_DigitalOutput, 1, kGPIO_NoIntmode};
static gpio_pin_config_t radiochipinterruptrequestConfig = {kGPIO_DigitalInput, 0, kGPIO_IntFallingEdge};
static gpio_pin_config_t radiochipselectConfig = {kGPIO_DigitalOutput, 1, kGPIO_NoIntmode};
static gpio_pin_config_t radiochipcleartosendConfig = {kGPIO_DigitalInput, 0, kGPIO_NoIntmode};
static gpio_pin_config_t radiochipsyncdetectConfig = {kGPIO_DigitalInput, 0, kGPIO_IntRisingEdge};//To generate flag, but do not enable interrupts
// ABR 
static gpio_pin_config_t userbuttoninterruptrequestConfig = {kGPIO_DigitalInput, 0, kGPIO_IntFallingEdge};

static uint8_t  RadioTransmitBuffer[RADIOBUFLEN];
static uint8_t  RadioReceiveBuffer[RADIOBUFLEN];
static volatile uint8_t  RxData[RADIOBUFLEN];

static uint8_t RxFifoCount;
static uint8_t TxFifoSpace;

static volatile bool RadioBusy = true;
static volatile bool SyncDetected = false;
static bool RadioResponseSuccess = false;
static bool RadioCTS = false;



static const tRadioProperties RadioProperty[NO_OF_RADIO_PROPERTIES] = RADIO_PROPERTIES_INIT;

static const uint8_t Patch[64][8] =  {	{0x04,0x21,0x71,0x4B,0x00,0x00,0xDC,0x95}	,	\
										{0x05,0xA6,0x22,0x21,0xF0,0x41,0x5B,0x26}	,	\
										{0xE2,0x2F,0x1C,0xBB,0x0A,0xA8,0x94,0x28}	,	\
										{0x05,0x87,0x67,0xE2,0x58,0x1A,0x07,0x5B}	,	\
										{0xE1,0xD0,0x72,0xD8,0x8A,0xB8,0x5B,0x7D}	,	\
										{0x05,0x11,0xEC,0x9E,0x28,0x23,0x1B,0x6D}	,	\
										{0xE2,0x4F,0x8A,0xB2,0xA9,0x29,0x14,0x13}	,	\
										{0x05,0xD1,0x2E,0x71,0x6A,0x51,0x4C,0x2C}	,	\
										{0xE5,0x80,0x27,0x42,0xA4,0x69,0xB0,0x7F}	,	\
										{0x05,0xAA,0x81,0x2A,0xBD,0x45,0xE8,0xA8}	,	\
										{0xEA,0xE4,0xF0,0x24,0xC9,0x9F,0xCC,0x3C}	,	\
										{0x05,0x08,0xF5,0x05,0x04,0x27,0x62,0x98}	,	\
										{0xEA,0x6B,0x62,0x84,0xA1,0xF9,0x4A,0xE2}	,	\
										{0x05,0xE9,0x77,0x05,0x4F,0x84,0xEE,0x35}	,	\
										{0xE2,0x43,0xC3,0x8D,0xFB,0xAD,0x54,0x25}	,	\
										{0x05,0x14,0x06,0x5E,0x39,0x36,0x2F,0x45}	,	\
										{0xEA,0x0C,0x1C,0x74,0xD0,0x11,0xFC,0x32}	,	\
										{0x05,0xDA,0x38,0xBA,0x0E,0x3C,0xE7,0x8B}	,	\
										{0xEA,0xB0,0x09,0xE6,0xFF,0x94,0xBB,0xA9}	,	\
										{0x05,0xD7,0x11,0x29,0xFE,0xDC,0x71,0xD5}	,	\
										{0xEA,0x7F,0x83,0xA7,0x60,0x90,0x62,0x18}	,	\
										{0x05,0x84,0x7F,0x6A,0xD1,0x91,0xC6,0x52}	,	\
										{0xEA,0x2A,0xD8,0x7B,0x8E,0x4A,0x9F,0x91}	,	\
										{0x05,0xBD,0xAA,0x9D,0x16,0x18,0x06,0x15}	,	\
										{0xE2,0x55,0xAD,0x2D,0x0A,0x14,0x1F,0x5D}	,	\
										{0x05,0xD3,0xE0,0x7C,0x39,0xCF,0x01,0xF0}	,	\
										{0xEF,0x3A,0x91,0x72,0x6A,0x03,0xBB,0x96}	,	\
										{0xE7,0x83,0x6D,0xA4,0x92,0xFC,0x13,0xA7}	,	\
										{0xEF,0xF8,0xFD,0xCF,0x62,0x07,0x6F,0x1E}	,	\
										{0xE7,0x4C,0xEA,0x4A,0x75,0x4F,0xD6,0xCF}	,	\
										{0xE2,0xF6,0x11,0xE4,0x26,0x0D,0x4D,0xC6}	,	\
										{0x05,0xFB,0xBF,0xE8,0x07,0x89,0xC3,0x51}	,	\
										{0xEF,0x82,0x27,0x04,0x3F,0x96,0xA8,0x58}	,	\
										{0xE7,0x41,0x29,0x3C,0x75,0x2A,0x03,0x1C}	,	\
										{0xEF,0xAF,0x59,0x98,0x36,0xAA,0x0F,0x06}	,	\
										{0xE6,0xF6,0x93,0x41,0x2D,0xEC,0x0E,0x99}	,	\
										{0x05,0x29,0x19,0x90,0xE5,0xAA,0x36,0x40}	,	\
										{0xE7,0xFB,0x68,0x10,0x7D,0x77,0x5D,0xC0}	,	\
										{0xE7,0xCB,0xB4,0xDD,0xCE,0x90,0x54,0xBE}	,	\
										{0xE7,0x72,0x8A,0xD6,0x02,0xF4,0xDD,0xCC}	,	\
										{0xE7,0x6A,0x21,0x0B,0x02,0x86,0xEC,0x15}	,	\
										{0xE7,0x7B,0x7C,0x3D,0x6B,0x81,0x03,0xD0}	,	\
										{0xEF,0x7D,0x61,0x36,0x94,0x7C,0xA0,0xDF}	,	\
										{0xEF,0xCC,0x85,0x3B,0xDA,0xE0,0x5C,0x1C}	,	\
										{0xE7,0xE3,0x75,0xBB,0x39,0x22,0x4B,0xA8}	,	\
										{0xEF,0xF9,0xCE,0xE0,0x5E,0xEB,0x1D,0xCB}	,	\
										{0xE7,0xBD,0xE2,0x70,0xD5,0xAB,0x4E,0x3F}	,	\
										{0xE7,0xB7,0x8D,0x20,0x68,0x6B,0x09,0x52}	,	\
										{0xEF,0xA1,0x1B,0x90,0xCD,0x98,0x00,0x63}	,	\
										{0xEF,0x54,0x67,0x5D,0x9C,0x11,0xFC,0x45}	,	\
										{0xE7,0xD4,0x9B,0xC8,0x97,0xBE,0x8A,0x07}	,	\
										{0xEF,0x52,0x8D,0x90,0x63,0x73,0xD5,0x2A}	,	\
										{0xEF,0x03,0xBC,0x6E,0x1C,0x76,0xBE,0x4A}	,	\
										{0xE7,0xC2,0xED,0x67,0xBA,0x5E,0x66,0x21}	,	\
										{0xEF,0xE7,0x3F,0x87,0xBE,0xE0,0x7A,0x6D}	,	\
										{0xE7,0xC9,0x70,0x93,0x1D,0x64,0xF5,0x6C}	,	\
										{0xEF,0xF5,0x28,0x08,0x34,0xB3,0xB6,0x2C}	,	\
										{0xEF,0x3A,0x0A,0xEC,0x0F,0xDB,0x56,0xCA}	,	\
										{0xEF,0x39,0xA0,0x6E,0xED,0x79,0xD0,0x24}	,	\
										{0xE7,0x6C,0x0B,0xAF,0xA9,0x4E,0x40,0xB5}	,	\
										{0xE9,0xB9,0xAF,0xBF,0x25,0x50,0xD1,0x37}	,	\
										{0x05,0x9E,0xDB,0xDE,0x3F,0x94,0xE9,0x6B}	,	\
										{0xEC,0xC5,0x05,0xAA,0x57,0xDC,0x8A,0x5E}	,	\
										{0x05,0x70,0xDA,0x84,0x84,0xDD,0xCA,0x90}	};



//extern SemaphoreHandle_t syncMatch;

static void Init_Keys(void);
static void Write_RadioCommand(uint8_t command, uint8_t len, uint8_t Rlen);
static void ReadRxBuffer(uint8_t len);
void Read_RadioInterruptStatus(void);
static bool Get_RadioResponse(uint8_t command, uint8_t Rlen);
//static void SetRadioProperty (uint8_t group, uint8_t number, uint8_t data);
static bool Get_RadioCTS(void);
static void ChangeState(uint8_t nextstate);
void StartRadioRx(uint8_t channel, uint8_t len, uint8_t nextstate);
static void StartRadioTx(uint8_t channel, uint8_t len, uint8_t nextstate, uint8_t retransmit);
static void ResetFIFO(uint8_t rxtx);
//static bool Read_SyncDetectStatus(void);



/*******************************************************************************
 * \brief	Initialise WG2 mesh parameters.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

void Init_Keys(void)
{
	SID = NULL_SID;
	RumorSenderSID = NULL_SID;
	RumorSeqNum = 0;
	ReceivedSeqNum = 99;
	MeshKey[0] = 0;
	MeshKey[1] = 0;
	MeshKey[2] = 0;
	NetworkMeshID = 0;
	LongMsgFirstByte = 0;
	MediumMsgFirstByte = 0;
	Init_Messages();
}



/*******************************************************************************
 * \brief	Get WG2 SID.
 *
 * \param   None.
 * \return	SID value.
 ******************************************************************************/

uint8_t GetSID(void)
{
	return SID;
}



/*******************************************************************************
 * \brief	Get WG2 mesh keys.
 *
 * \param   None.
 * \return	Pinter to mesh keys.
 ******************************************************************************/

uint8_t* GetKeys(void)
{
	return &MeshKey[0];
}



/*******************************************************************************
 * \brief	Initialise radio (Si4461) RAM, GPIOs, and property configuration.
 *
 * \param   None.
 * \return	None.
 ******************************************************************************/

void Init_Radio(void)
{
#if ENHANCED_DEBUG
    //ABR added for testing
    WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x01;
    WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
    GPIO_PinInit(RADIO_CHIP_SHUTDOWN_GPIO, RADIO_CHIP_SHUTDOWN_GPIO_PIN, &radiochipshutdownConfig);
    GPIO_PinInit(RADIO_CHIP_NIRQ_GPIO, RADIO_CHIP_NIRQ_GPIO_PIN, &radiochipinterruptrequestConfig);
    GPIO_PinInit(RADIO_CHIP_SELECT_GPIO, RADIO_CHIP_SELECT_GPIO_PIN, &radiochipselectConfig);
    GPIO_PinInit(RADIO_CHIP_CTS_GPIO, RADIO_CHIP_CTS_GPIO_PIN, &radiochipcleartosendConfig);
    GPIO_PinInit(SYNC_WORD_DETECT_GPIO, SYNC_WORD_DETECT_GPIO_PIN, &radiochipsyncdetectConfig);
    //GPIO_PinInit(DIRECT_MODE_TX_DATA_GPIO, DIRECT_MODE_TX_DATA_GPIO_PIN, &radiodirectmodetxdataConfig);


    NVIC_EnableIRQ(RADIO_CHIP_NIRQ_IRQ);
    GPIO_PortEnableInterrupts(RADIO_CHIP_NIRQ_GPIO, 1U << RADIO_CHIP_NIRQ_GPIO_PIN);


    NVIC_SetPriority(SYNC_WORD_DETECT_IRQ, SYNC_WORD_DETECT_IRQ_PRIORITY);
    NVIC_EnableIRQ(SYNC_WORD_DETECT_IRQ);

    //GPIO_PortEnableInterrupts(SYNC_WORD_DETECT_GPIO, 1U << SYNC_WORD_DETECT_GPIO_PIN);


    // ABR
    GPIO_PinInit(USER_BUTTON_GPIO, USER_BUTTON_GPIO_PIN, &userbuttoninterruptrequestConfig);
    NVIC_SetPriority(USER_BUTTON_IRQ, USER_BUTTON_IRQ_PRIORITY);
    NVIC_EnableIRQ(USER_BUTTON_IRQ);
    //GPIO_PortEnableInterrupts(USER_BUTTON_GPIO, 1U << USER_BUTTON_GPIO_PIN);


	// Reset radio by driving SDN high then low
    RADIO_SDN_ON;
	Delay(6);									//?msec delay
	RADIO_SDN_OFF;
	Delay(12); 									//?6msec delay


	for(uint8_t i=0;i<64;i++)
	{
		for(uint8_t j=1;j<8;j++)
		{
			RadioTransmitBuffer[j-1] = Patch[i][j];
		}
		Write_RadioCommand(Patch[i][0], 7, 0);
		/*
		do
		{
		  	RadioCTS = Get_RadioCTS();
		}
		while(!RadioCTS);
		*/
		Delay(8);  //////// [RE:fixed] was 4msec
	}


    // Send POWER_UP command to initialise the radio. Ext. Xtal at 30MHz
    RadioTransmitBuffer[0] = 0x81;
    RadioTransmitBuffer[1] = 0x00;
    RadioTransmitBuffer[2] = 0x01;
    RadioTransmitBuffer[3] = 0xC9;//0x8C
    RadioTransmitBuffer[4] = 0xC3;//0xBA
    RadioTransmitBuffer[5] = 0x80;
    Write_RadioCommand(POWER_UP, POWER_UP_COMMAND_LEN, POWER_UP_RESPONSE_LEN);

    while(RadioBusy);
    Read_RadioInterruptStatus();
    if(RadioResponseSuccess)
    {
    	;//PRINTF("Radio POWER_UP initialisation success. \r\n");
    }

    // Send GPIO_PIN_CFG command to initialise the radio
    RadioTransmitBuffer[0] = 0x60;//Pull up and TX_STATE on GPIO0
    RadioTransmitBuffer[1] = 0x48;//Pull up and CTS
    RadioTransmitBuffer[2] = 0x44;//Pull up and TX data in
    RadioTransmitBuffer[3] = 0x5A;//Pull up  and SYNC word detect
    RadioTransmitBuffer[4] = 0x67;
    RadioTransmitBuffer[5] = 0x4B;
    RadioTransmitBuffer[6] = 0x00;
    Write_RadioCommand(GPIO_PIN_CFG, GPIO_PIN_CFG_COMMAND_LEN, NO_RESPONSE_LEN);
    //Use either delay of 1msec, or check CTS. Not both together
    //Delay(2);
    do
    {
      	RadioCTS = Get_RadioCTS();
    }
    while(!RadioCTS);

    //while(RadioBusy);//Does not seem to work
    Read_RadioInterruptStatus();
    if(RadioResponseSuccess)
    {
      	;//PRINTF("Radio GPIO_PIN_CFG initialisation success %d. \r\n",count);
    }

    //'SET_PROPERTIES'
    for(uint8_t list_index = 0; list_index < NO_OF_RADIO_PROPERTIES; list_index++)
    {
         SetRadioProperty(RadioProperty[list_index].group, RadioProperty[list_index].number, RadioProperty[list_index].data);
    }


    Init_Keys();
    GateWayID[0] = 1;
    GateWayID[1] = 2;
    GateWayID[2] = 3;
    UpdateID(&GateWayID[0]);

#if ENHANCED_DEBUG
    //ABR added for testing
    WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x81;
    WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
    //SetRadioProperty (INT_CTL, INT_CTL_ENABLE, NO_INT_EN);
    //GPIO_PortDisableInterrupts(RADIO_CHIP_NIRQ_GPIO, 1U << RADIO_CHIP_NIRQ_GPIO_PIN);
}



/*******************************************************************************
 * \brief   Switch radio transceiver to sleep mode.
 *
 * \param   None.
 * \return  None.
 ******************************************************************************/

void Sleep_Radio(void)
{
#if ENHANCED_DEBUG
    //ABR added for testing
    WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x02;
    WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
    ChangeState(SLEEP_STATE);
}



/*******************************************************************************
 * \brief	Write command to radio transceiver.
 *
 * \param   Command. Command to radio.
 * \param	len. Length of command.
 * \param	Rlen. Length of expected command response.
 * \return	None.
 ******************************************************************************/

void Write_RadioCommand(uint8_t command, uint8_t len, uint8_t Rlen)
{
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x03;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
	uint8_t count;
	RadioBusy = true;
	RadioResponseSuccess = false;


    RADIO_NSEL_ON;

	LPSPI_FlushFifo(RADIO_LPSPI_MASTER_BASEADDR, true, true);
	LPSPI_ClearStatusFlags(RADIO_LPSPI_MASTER_BASEADDR, kLPSPI_AllStatusFlag);

	RADIO_LPSPI_MASTER_BASEADDR->TDR = (uint32_t) command;

    for(count = 0; count < len; count++)
    {
    	while( FRAME_TRANSFER_NOT_COMPLETED )
    	{
    	 	;
    	}
    	LPSPI_FlushFifo(RADIO_LPSPI_MASTER_BASEADDR, true, true);
    	LPSPI_ClearStatusFlags(RADIO_LPSPI_MASTER_BASEADDR, kLPSPI_AllStatusFlag);

    	RADIO_LPSPI_MASTER_BASEADDR->TDR = (uint32_t) RadioTransmitBuffer[count];
    }


    while( FRAME_TRANSFER_NOT_COMPLETED )
    {
     	;
    }
    LPSPI_FlushFifo(RADIO_LPSPI_MASTER_BASEADDR, true, true);
    LPSPI_ClearStatusFlags(RADIO_LPSPI_MASTER_BASEADDR, kLPSPI_AllStatusFlag);
    RADIO_NSEL_OFF;

    if(Rlen != 0)
    {
    	RadioResponseSuccess = Get_RadioResponse(command,Rlen);
    }
#if ENHANCED_DEBUG
    //ABR added for testing
    WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x83;
    WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif

}



/*******************************************************************************
 * \brief	Read radio receive buffer.
 *
 * \param	len. Length of block to be read.
 * \return	None.
 ******************************************************************************/

void ReadRxBuffer(uint8_t len)
{
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x04;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
	//RadioBusy = true;

	RADIO_NSEL_ON;

	LPSPI_FlushFifo(RADIO_LPSPI_MASTER_BASEADDR, true, true);
	LPSPI_ClearStatusFlags(RADIO_LPSPI_MASTER_BASEADDR, kLPSPI_AllStatusFlag);

	RADIO_LPSPI_MASTER_BASEADDR->TDR = (uint32_t) READ_RX_FIFO;

	while( FRAME_TRANSFER_NOT_COMPLETED )
	{
		;
	}

	for(uint8_t count = 0; count < len; count++)
	{
	    LPSPI_FlushFifo(RADIO_LPSPI_MASTER_BASEADDR, true, true);
	    LPSPI_ClearStatusFlags(RADIO_LPSPI_MASTER_BASEADDR, kLPSPI_AllStatusFlag);

	    RADIO_LPSPI_MASTER_BASEADDR->TDR = (uint32_t) 0x00000000;
	    while( FRAME_TRANSFER_NOT_COMPLETED )
	    {
	     	;
	    }

	    RxData[count] = (uint8_t)(RADIO_LPSPI_MASTER_BASEADDR->RDR);
	}

	RADIO_NSEL_OFF;
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x84;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
}



/*******************************************************************************
 * \brief	Read radio interrupt status.
 *
 * \param	None.
 * \return	None.
 ******************************************************************************/

void Read_RadioInterruptStatus(void)
{
    RadioTransmitBuffer[0] = 0x00;
    RadioTransmitBuffer[1] = 0x00;
    RadioTransmitBuffer[2] = 0x00;

    //Write_RadioCommand(GET_INT_STATUS, GET_INT_STATUS_COMMAND_LEN, GET_INT_STATUS_RESPONSE_LEN);
    //With no parameters, all pending interrupts cleared. Saving the time of transmitting 3 bytes
    Write_RadioCommand(GET_INT_STATUS, NO_PARAMETERS, 3);//Only 3rd response byte (PH_PEND) needed for checking
}



/*******************************************************************************
 * \brief	Set radio interrupt status flag.
 *
 * \param	None.
 * \return	None.
 ******************************************************************************/

void Set_RadioInterruptStatus(void)
{
    //ABR added for testing
    //WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x06;

	RadioBusy = false;
}



/*******************************************************************************
 * \brief	Set radio sync detected flag.
 *
 * \param	None.
 * \return	None.
 ******************************************************************************/

void Set_SyncDetected(void)
{
    //ABR added for testing
    //WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] += 0x80;

    SyncDetected = true;
}



/*******************************************************************************
 * \brief	Get radio response to a command.
 *
 * \param	Command. Command to radio.
 * \param	Rlen. Length of command response.
 * \return	None.
 ******************************************************************************/

bool Get_RadioResponse(uint8_t command, uint8_t Rlen)
{
#if ENHANCED_DEBUG
    //ABR added for testing
    WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x08;
    WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
    bool success = false;
    uint16_t count = 0;
    uint8_t RXdata;


    do
    {
    	RADIO_NSEL_ON;

    	LPSPI_FlushFifo(RADIO_LPSPI_MASTER_BASEADDR, true, true);
    	LPSPI_ClearStatusFlags(RADIO_LPSPI_MASTER_BASEADDR, kLPSPI_AllStatusFlag);

    	RADIO_LPSPI_MASTER_BASEADDR->TDR = (uint32_t) READ_CMD_BUFF;
    	while( FRAME_TRANSFER_NOT_COMPLETED )
    	{
    		;
    	}
    	LPSPI_FlushFifo(RADIO_LPSPI_MASTER_BASEADDR, true, true);
    	LPSPI_ClearStatusFlags(RADIO_LPSPI_MASTER_BASEADDR, kLPSPI_AllStatusFlag);

    	RADIO_LPSPI_MASTER_BASEADDR->TDR = (uint32_t) 0x00000000;
    	while( FRAME_TRANSFER_NOT_COMPLETED )
    	{
    	 	;
    	}

        RXdata = (uint8_t)(RADIO_LPSPI_MASTER_BASEADDR->RDR);
        if(RXdata != 0xFF)
        {
        	RADIO_NSEL_OFF;
        }
        else
        {
            success = true;
        }
    }
    while( (count++ < 50) && (RXdata != 0xFF) );



    if(success != false)
    {
        for(uint8_t count = 0; count < Rlen; count++)
        {
        	LPSPI_FlushFifo(RADIO_LPSPI_MASTER_BASEADDR, true, true);
        	LPSPI_ClearStatusFlags(RADIO_LPSPI_MASTER_BASEADDR, kLPSPI_AllStatusFlag);

        	RADIO_LPSPI_MASTER_BASEADDR->TDR = (uint32_t) 0x00000000;
        	while( FRAME_TRANSFER_NOT_COMPLETED )
        	{
        	 	;
        	}

        	RadioReceiveBuffer[count] = (uint8_t)(RADIO_LPSPI_MASTER_BASEADDR->RDR);
        }
    }

    RADIO_NSEL_OFF;
#if ENHANCED_DEBUG
    //ABR added for testing
    WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x88;
    WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif

    return success;
}



/*******************************************************************************
 * \brief	Write radio property.
 *
 * \param	group. Property group.
 * \param	number. Property number.
 * \param	data. Property value.
 * \return	None.
 ******************************************************************************/

void SetRadioProperty (uint8_t group, uint8_t number, uint8_t data)
{
#if ENHANCED_DEBUG
    //ABR added for testing
    WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x09;
    WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
    //bool success;

    RadioTransmitBuffer[0] = group;		// Property group
    RadioTransmitBuffer[1] = 0x01;		// No. of properties to write to
    RadioTransmitBuffer[2] = number;	// Property index
    RadioTransmitBuffer[3] = data;		// Data value

    //RadioCTS = Get_RadioCTS();
    //if( RadioCTS )
    do
    {
    	RadioCTS = Get_RadioCTS();
    }
    while(!RadioCTS);

    Write_RadioCommand(SET_PROPERTY, SET_PROPERTY_COMMAND_LEN, SET_PROPERTY_RESPONSE_LEN);
#if ENHANCED_DEBUG
    //ABR added for testing
    WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x89;
    WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
}



/*******************************************************************************
 * \brief	Get radio CTS.
 *
 * \param	None.
 * \return	CTS state.
 ******************************************************************************/

bool Get_RadioCTS(void)
{
	//bool success = false;
	//uint16_t count = 0;
	//uint8_t RXdata;

	return (((RADIO_CHIP_CTS_GPIO->DR) >> RADIO_CHIP_CTS_GPIO_PIN) & 0x1U);

/*
	do
	{
	  	RADIO_NSEL_ON;

	   	LPSPI_FlushFifo(RADIO_LPSPI_MASTER_BASEADDR, true, true);
	   	LPSPI_ClearStatusFlags(RADIO_LPSPI_MASTER_BASEADDR, kLPSPI_AllStatusFlag);

	   	RADIO_LPSPI_MASTER_BASEADDR->TDR = (uint32_t) READ_CMD_BUFF;
	   	while( FRAME_TRANSFER_NOT_COMPLETED )
	   	{
	   		;
	   	}
	   	LPSPI_FlushFifo(RADIO_LPSPI_MASTER_BASEADDR, true, true);
	   	LPSPI_ClearStatusFlags(RADIO_LPSPI_MASTER_BASEADDR, kLPSPI_AllStatusFlag);

	   	RADIO_LPSPI_MASTER_BASEADDR->TDR = (uint32_t) 0x00000000;
	   	while( FRAME_TRANSFER_NOT_COMPLETED )
	   	{
	   	 	;
	   	}

	   	RXdata = (uint8_t)(RADIO_LPSPI_MASTER_BASEADDR->RDR);

	    RADIO_NSEL_OFF;
	}
	while( (count++ < 30) && (RXdata != 0xFF) );

	if(RXdata == 0xFF)
	{
	    success = true;
	}

	return success;
*/
}



/*******************************************************************************
 * \brief	Join a mesh network.
 *
 * \param	None.
 * \return	True, if successfully joined in. False, if failed.
 ******************************************************************************/

bool JoinNetwork(void)
{
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x0B;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
	uint8_t detections=0;
	bool JoinSuccess;
	bool ReceiveInviteSuccess;
	uint32_t StartTime, CurrentTime;


	Init_Keys();

	JoinSuccess = false;

	ChangeState(READYSTATE);
	//Read_RadioInterruptStatus();
	StartTime = GetCurrentTimerCount();
	do
	{
		ReceiveInviteSuccess = ReceiveInviteMessage();

		if(ReceiveInviteSuccess)
		{
			//Delay(2);
			SendJoinRequestMessage();
			JoinSuccess = ReceiveJoinAckMessage();
			detections++;
		}
		CurrentTime = GetCurrentTimerCount();
	}
	while( (!JoinSuccess) && ((CurrentTime-StartTime)<(uint32_t)(FIVE_SECOND)) );

#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x8B;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
	//PRINTF("Number of detections = %d \r\n",detections);
	return JoinSuccess;
}



/*******************************************************************************
 * \brief	Receive invite message.
 *
 * \param	None.
 * \return	True, if invite received successfully. False, if failed.
 ******************************************************************************/

bool ReceiveInviteMessage(void)
{
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x0C;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
	bool success;
	uint8_t data;
	uint32_t StartTime, CurrentTime;

	success = false;

	//Build and load the joint request message to the FIFO transmit buffer
	//Always reset TxFIFO before building any message


	// Use these if delay is too long in the next function.
	ResetFIFO(Tx);
	BuildJoinRequestMsg(&RadioTransmitBuffer[0]);
	UploadMessage(JOIN_REQUEST_MESSAGE_LEN);



	data = SYNC_4_BYTE;//SKIP_TX + SYNC_4_BYTE;
	SetRadioProperty (SYNC, SYNC_CONFIG, data);
	data = 0x55;
	SetRadioProperty (SYNC, SYNC_BITS_BYTE_1, data);
	SetRadioProperty (SYNC, SYNC_BITS_BYTE_2, data);
	data = 0x95;
	SetRadioProperty (SYNC, SYNC_BITS_BYTE_3, data);
	data = 0x80;
	SetRadioProperty (SYNC, SYNC_BITS_BYTE_4, data);

	ResetFIFO(Rx);										//Seems redundant. Remove it and test
	Read_RadioInterruptStatus();
	SyncDetected = false;
	wait_syncMatch(0);  // ensure sync detect flag is reset
	StartTime = GetCurrentTimerCount();
	StartRadioRx(CHANNEL_0, 3, READYSTATE);
	wait_syncMatch(5000);  // wait up to 5 seconds for sync detect
	do
	{
		Read_RadioInterruptStatus();
		CurrentTime = GetCurrentTimerCount();
	}while( !(RadioReceiveBuffer[2] & 0x10) && ((CurrentTime-StartTime)<(uint32_t)(FIVE_SECOND)) );


	if(RadioReceiveBuffer[2] & 0x10)
	{
		DownloadMessage(3);

		if( (RxData[0] & INVITING_BIT) != 0)
		{
			success = true;
		}
	}
	Read_RadioInterruptStatus();
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x8C;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
	return success;
}



/*******************************************************************************
 * \brief	Send join request message.
 *
 * \param	None.
 * \return	None.
 ******************************************************************************/

void SendJoinRequestMessage(void)
{
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x0D;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
	uint8_t  data;
    //ABR added
    uint32_t StartTime, CurrentTime;

	//Radio chip does not transmit unless sync is transmitted. (bug in radio revB)
	data = SYNC_1_BYTE;
	SetRadioProperty (SYNC, SYNC_CONFIG, data);
	data = 0x00;
	SetRadioProperty (SYNC, SYNC_BITS_BYTE_1, data);

    //ABR added
    StartTime = GetCurrentTimerCount();

	StartRadioTx(CHANNEL_0, JOIN_REQUEST_MESSAGE_LEN, READYSTATE, NO_RETRANSMIT);
	do
	{
		Read_RadioInterruptStatus();
        CurrentTime = GetCurrentTimerCount();
	}while( !(RadioReceiveBuffer[2] & 0x20) && ((CurrentTime-StartTime)<20) );//ABR takes <3msec, we give it 10msec for testing
	Read_RadioInterruptStatus();
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x8D;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
}



/*******************************************************************************
 * \brief	Receive join Ack message.
 *
 * \param	None.
 * \return	True, if join Ack received. False, if not.
 ******************************************************************************/

bool ReceiveJoinAckMessage(void)
{
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x0E;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
	bool success;
	uint8_t  data;
	uint32_t StartTime, CurrentTime;

	success = false;

	// The 2 bytes sync was sufficient. Do not use SKIP_TX. This will cause the Radio to malfunction
	data = SYNC_2_BYTE;//SKIP_TX + SYNC_2_BYTE;
	SetRadioProperty (SYNC, SYNC_CONFIG, data);
	data = 0x95;
	SetRadioProperty (SYNC, SYNC_BITS_BYTE_1, data);
	data = 0xC0;
	SetRadioProperty (SYNC, SYNC_BITS_BYTE_2, data);


	ResetFIFO(Rx);
	Read_RadioInterruptStatus();
	StartRadioRx(CHANNEL_0, JOIN_ACK_MESSAGE_LEN, READYSTATE);//One byte less in this case
	StartTime = GetCurrentTimerCount();
	CurrentTime = StartTime;

	while( !success && ((CurrentTime-StartTime)<(20)) )//10msec window
	{
		CurrentTime = GetCurrentTimerCount();
		ResetFIFO(0);
		if(RxFifoCount<JOIN_ACK_MESSAGE_LEN)
		{
			continue;
		}
		else
		{
			DownloadMessage(JOIN_ACK_MESSAGE_LEN);
			if(RxData[1] == MSG_JOINACK)
			{
				CRCInit();
				CRCByte(MSG_LEN_LONG);
				for (uint8_t i=0; i<JOIN_ACK_MESSAGE_LEN; i++)
				{
					CRCByte(RxData[i]);
				}

				if( GetCRC()==CRC_CHECK )
				{
					SID = RxData[2] & SID_MASK;
					RumorSenderSID = RxData[0] & SID_MASK;
					MeshKey[0] = RxData[3];
					MeshKey[1] = RxData[4];
					MeshKey[2] = RxData[5];
					CopyMap(SIDMap,&RxData[6],MAP_DITHER);
					SetBit(SIDMap,SID);

					NetworkMeshID = MeshKey[0] & ID_MASK;
					TrueMID = NetworkMeshID;
					LongMsgFirstByte = (NetworkMeshID<<2)+MSG_LEN_LONG;
					MediumMsgFirstByte = (NetworkMeshID<<2)+MSG_LEN_MEDIUM;

					//Only update the SID before calling Ok message which will call GetKeys. At this
					//stage MeshID and CRC still need to be initialised to 0, but new SID required
					UpdateSID(SID);

					ResetFIFO(Tx);
					BuildOkMsg(&RadioTransmitBuffer[0]);
					for(uint8_t i=0; i<5; i++)
					{
						UploadMessage(OK_MESSAGE_LEN);
						SendOkMessage();
					}

					//Now update MeshKeys as well
					UpdateKeys(&MeshKey[0]);

					ClearMap(NbrMap);
					SetBit(NbrMap,RumorSenderSID);
					//PRINTF("Joined network %d as %d %d %d \r\n",MeshKey[0],SID,SIDMap[0],SIDMap[1]);

					//ReasonsForRumor |= (REASON_CHECK_NBRS | REASON_JOINED);	// neighbour check is higher priority and will happen first
					ReasonsForRumor |= REASON_JOINED;

					//Save mesh information here
					SaveMeshInfo();
#if SCRAMBLED_WISAFE
					// ABR Scramble
					Scramble0 = MeshKey[1];
#endif

					success = true;
				}
			}
		}
	}

	Read_RadioInterruptStatus();
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x8E;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
	return success;
}



/*******************************************************************************
 * \brief	Send Ok message.
 *
 * \param	None.
 * \return	None.
 ******************************************************************************/

void SendOkMessage(void)
{
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x0F;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
	uint8_t  data;
    //ABR added
    uint32_t StartTime, CurrentTime;

	//Radio chip does not transmit unless sync is transmitted
	data = SYNC_1_BYTE;
	SetRadioProperty (SYNC, SYNC_CONFIG, data);
	data = 0x00;
	SetRadioProperty (SYNC, SYNC_BITS_BYTE_1, data);

    //ABR added
    StartTime = GetCurrentTimerCount();

	StartRadioTx(CHANNEL_0, OK_MESSAGE_LEN, READYSTATE, NO_RETRANSMIT);
	do
	{
		Read_RadioInterruptStatus();
        CurrentTime = GetCurrentTimerCount();
	}while( !(RadioReceiveBuffer[2] & 0x20) && ((CurrentTime-StartTime)<10) );//ABR takes 1mesc, we give it 5msec for testing
	Read_RadioInterruptStatus();
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x8F;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
}



/*******************************************************************************
 * \brief	Receive Ok message.
 *
 * \param	TimeLeft. Time left for receive.
 * \return	SID of the Ok message sender.
 ******************************************************************************/

uint8_t RceiveOkMessage(uint32_t TimeLeft)
{
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x10;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
	uint8_t	sid = NULL_SID;
	uint8_t	data;
	uint32_t StartTime, CurrentTime;

	Read_RadioInterruptStatus();
	data = SKIP_TX + SYNC_4_BYTE;
	SetRadioProperty (SYNC, SYNC_CONFIG, data);
	data = 0x55;
	SetRadioProperty (SYNC, SYNC_BITS_BYTE_1, data);
	SetRadioProperty (SYNC, SYNC_BITS_BYTE_2, data);
	data = 0x95;
	SetRadioProperty (SYNC, SYNC_BITS_BYTE_3, data);
	data =  ReverseByte((NetworkMeshID<<2)+MSG_LEN_SHORT);
	SetRadioProperty (SYNC, SYNC_BITS_BYTE_4, data);

	StartRadioRx(CHANNEL_0, 1, TX_TUNE_STATE);//Receive Ok. No need to receive and check CRC bytes. Quick response needed.

	StartTime = GetCurrentTimerCount();
	do
	{
		Read_RadioInterruptStatus();
		CurrentTime = GetCurrentTimerCount();
	}
	while( !(RadioReceiveBuffer[2] & 0x10) && ((CurrentTime-StartTime)<TimeLeft) );

	if(RadioReceiveBuffer[2] & 0x10)
	{
		DownloadMessage(1);
		sid = RxData[0]&SID_MASK;
	}

#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x90;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
	return sid;
}



/*******************************************************************************
 * \brief	Receive rumour Ack message.
 *
 * \param	id. SID of expected rumour Acking node.
 * \return	True, if correct Ack received. False, if not.
 ******************************************************************************/

bool ReceiveRumourAckMessage(uint8_t id)
{
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x11;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
	bool success;
	uint8_t  data;
	uint32_t StartTime, CurrentTime;

	success = false;

	data = SKIP_TX + SYNC_4_BYTE;
	SetRadioProperty (SYNC, SYNC_CONFIG, data);
	data = 0x55;
	SetRadioProperty (SYNC, SYNC_BITS_BYTE_1, data);
	data = 0x55;
	SetRadioProperty (SYNC, SYNC_BITS_BYTE_2, data);
	data = 0x95;
	SetRadioProperty (SYNC, SYNC_BITS_BYTE_3, data);
	data = ReverseByte(MediumMsgFirstByte);
	SetRadioProperty (SYNC, SYNC_BITS_BYTE_4, data);

	StartTime = GetCurrentTimerCount();
	CurrentTime = StartTime;
	while( !success && ((CurrentTime-StartTime)<(20)) )//10msec window
	{
		Read_RadioInterruptStatus();
		StartRadioRx(CHANNEL_0, RUMOUR_ACK_MESSAGE_LEN, READYSTATE);//One byte less in this case
		do
		{
			Read_RadioInterruptStatus();
			CurrentTime = GetCurrentTimerCount();
		}
		while( !(RadioReceiveBuffer[2] & 0x10) && ((CurrentTime-StartTime)<20) );

		if(RadioReceiveBuffer[2] & 0x10)
		{
			DownloadMessage(RUMOUR_ACK_MESSAGE_LEN);
			CRCInit();
			CRCByte(MediumMsgFirstByte);
			for (uint8_t i=0; i<RUMOUR_ACK_MESSAGE_LEN; i++)
			{
				CRCByte(RxData[i]);
			}
			if( GetCRC()==CRC_CHECK )
			{
				if((RxData[0]&SID_MASK)==id)
				{
					success = true;
					//PowerRcvBuf2 = RxData[1];	// first time, record some of message bytes
					//PowerRcvBuf3 = RxData[2];
					//PowerRcvBuf4 = RxData[3];
					//PowerRcvBuf5 = RxData[4];

					rcvbuf[0] = MediumMsgFirstByte;
					for(uint8_t i=1; i<=PROPAGATE_ACK_MESSAGE_LEN; i++)
					{
						rcvbuf[i] = RxData[i-1];
					}
				}
			}
		}
	}
	Read_RadioInterruptStatus();
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x91;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
    return success;
}



/*******************************************************************************
 * \brief	Receive propagate Ack message.
 *
 * \param	id. SID of expected propagate Acking node.
 * \return	True, if correct Ack received. False, if not.
 ******************************************************************************/

bool ReceivePropagateAckMessage(uint8_t id)
{
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x12;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
	bool success;
	uint8_t  data;
	uint32_t StartTime, CurrentTime;

	success = false;

	data = SKIP_TX + SYNC_4_BYTE;
	SetRadioProperty (SYNC, SYNC_CONFIG, data);
	data = 0x55;
	SetRadioProperty (SYNC, SYNC_BITS_BYTE_1, data);
	data = 0x55;
	SetRadioProperty (SYNC, SYNC_BITS_BYTE_2, data);
	data = 0x95;
	SetRadioProperty (SYNC, SYNC_BITS_BYTE_3, data);
	data = ReverseByte(MediumMsgFirstByte);
	SetRadioProperty (SYNC, SYNC_BITS_BYTE_4, data);

	StartTime = GetCurrentTimerCount();
	CurrentTime = StartTime;
	while( !success && ((CurrentTime-StartTime)<(20)) )//10msec window
	{
		Read_RadioInterruptStatus();
		StartRadioRx(CHANNEL_0, PROPAGATE_ACK_MESSAGE_LEN, READYSTATE);//One byte less in this case
		do
		{
			Read_RadioInterruptStatus();
			CurrentTime = GetCurrentTimerCount();
		}
		while( !(RadioReceiveBuffer[2] & 0x10) && ((CurrentTime-StartTime)<20) );

		if(RadioReceiveBuffer[2] & 0x10)
		{
			DownloadMessage(PROPAGATE_ACK_MESSAGE_LEN);
			CRCInit();
			CRCByte(MediumMsgFirstByte);
			for (uint8_t i=0; i<PROPAGATE_ACK_MESSAGE_LEN; i++)
			{
				CRCByte(RxData[i]);
			}
			if( GetCRC()==CRC_CHECK )
			{
				if((RxData[0]&SID_MASK)==id)
				{
					success = true;
					//PowerRcvBuf2 = RxData[1];	// first time, record some of message bytes
					//PowerRcvBuf3 = RxData[2];
					//PowerRcvBuf4 = RxData[3];
					//PowerRcvBuf5 = RxData[4];

					rcvbuf[0] = MediumMsgFirstByte;
					for(uint8_t i=1; i<=PROPAGATE_ACK_MESSAGE_LEN; i++)
					{
						rcvbuf[i] = RxData[i-1];
					}
				}
			}
		}
	}
	Read_RadioInterruptStatus();
#if ENHANCED_DEBUG
	//ABR added for testing
        WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x92;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
	return success;
}



/*******************************************************************************
 * \brief	Send a chirp message.
 *
 * \param	None.
 * \return	None.
 ******************************************************************************/

void Chirp(void)
{
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x13;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
	uint8_t data;
    //ABR added
    uint32_t StartTime, CurrentTime;

	RadioTransmitBuffer[0] = 0x2A;//0xAA
	RadioTransmitBuffer[1] = 0xAA;
	RadioTransmitBuffer[2] = 0xAA;
	RadioTransmitBuffer[3] = 0xA8;

	UploadMessage(CHIRP_MESSAGE_LEN);

	data = SKIP_TX + SYNC_3_BYTE;
	SetRadioProperty (SYNC, SYNC_CONFIG, data);
	data = 0x55;
	SetRadioProperty (SYNC, SYNC_BITS_BYTE_1, data);
	data = 0x95;
	SetRadioProperty (SYNC, SYNC_BITS_BYTE_2, data);
	data = ReverseByte(NetworkMeshID<<2);
	SetRadioProperty (SYNC, SYNC_BITS_BYTE_3, data);

	Read_RadioInterruptStatus();
    //ABR added
    StartTime = GetCurrentTimerCount();

	StartRadioTx(CHANNEL_0, CHIRP_MESSAGE_LEN, RX_TUNE_STATE, NO_RETRANSMIT);//Transmit chirp
	do
	{
		Read_RadioInterruptStatus();
        CurrentTime = GetCurrentTimerCount();
	}
	while( !(RadioReceiveBuffer[2] & 0x20) && ((CurrentTime-StartTime)<6) );//ABR takes 1/2 msec, we give it 3msec for testing 
	Read_RadioInterruptStatus();
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x93;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
}



/*******************************************************************************
 * \brief	Receive a message.
 *
 * \param	None.
 * \return	True, message received. False, if not.
 ******************************************************************************/

bool ChirpToReceive(void)
{
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x14;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
	bool success;
	uint8_t data;
	uint32_t StartTime, CurrentTime;

	uint8_t RumorMsgByte;
	uint8_t RumorSenderSID;
	uint8_t RumorInitiatorSID;
	bool RumorOldNews;										// sender tells us if we have already heard the rumour


	if((ShortChirpCount<SHORT_CHIRP_COUNT) || (ReasonForCurrentRumor!=0))
	{
		// send short chirp
		ShortChirpCount++;
		//Chirp(); will be sent below as part of receiving a message
	}
	else
	{
		// send long chirp (Ok) every 90 seconds
		ShortChirpCount = 0;
		ResetFIFO(Tx);
		BuildOkMsg(&RadioTransmitBuffer[0]);				// send whole OK message
		UploadMessage(OK_MESSAGE_LEN);
		SendOkMessage();
	}


	success = false;

	ResetFIFO(Tx+Rx);
	//////////////////////////////Chirp message////////////////
	RadioTransmitBuffer[0] = 0x2A;//0xAA
	RadioTransmitBuffer[1] = 0xAA;
	RadioTransmitBuffer[2] = 0xAA;
	RadioTransmitBuffer[3] = 0xA8;

	UploadMessage(CHIRP_MESSAGE_LEN);


	//The following will be used in detecting Chirp response, 4 bytes is too stringent.
	/*
	data = SKIP_TX + SYNC_4_BYTE;
	SetRadioProperty (SYNC, SYNC_CONFIG, data);
	data = 0x55;
	SetRadioProperty (SYNC, SYNC_BITS_BYTE_1, data);
	data = 0x55;
	SetRadioProperty (SYNC, SYNC_BITS_BYTE_2, data);
	data = 0x95;
	SetRadioProperty (SYNC, SYNC_BITS_BYTE_3, data);
	data = ReverseByte(NetworkMeshID<<2);
	SetRadioProperty (SYNC, SYNC_BITS_BYTE_4, data);
	*/
	data = SKIP_TX + SYNC_3_BYTE;
	SetRadioProperty (SYNC, SYNC_CONFIG, data);
	data = 0x55;
	SetRadioProperty (SYNC, SYNC_BITS_BYTE_1, data);
	data = 0x95;
	SetRadioProperty (SYNC, SYNC_BITS_BYTE_2, data);
	data = ReverseByte(NetworkMeshID<<2);
	SetRadioProperty (SYNC, SYNC_BITS_BYTE_3, data);

#if 1
	//Read_RadioInterruptStatus();
    //ABR added
    StartTime = GetCurrentTimerCount();

	StartRadioTx(CHANNEL_0, CHIRP_MESSAGE_LEN, RX_TUNE_STATE, NO_RETRANSMIT);//Transmit chirp
	do
	{
		Read_RadioInterruptStatus();
        CurrentTime = GetCurrentTimerCount();
	}
	while( !(RadioReceiveBuffer[2] & 0x20) && ((CurrentTime-StartTime)<6) );
#else
	//Using the NIRQ is unreliable.Probably because of glitches on the line causing unreal interrupts.
#endif

	////////////////////////////Chirp response///////////////////////
	StartRadioRx(CHANNEL_0, 0, TX_TUNE_STATE);//Receive chirp response.
	SyncDetected = false;
	StartTime = GetCurrentTimerCount();
	BuildOkMsg(&RadioTransmitBuffer[0]);
	UploadMessage(OK_MESSAGE_LEN);
	do
	{
		CurrentTime = GetCurrentTimerCount();
	}
	while( (!SyncDetected) && ((CurrentTime-StartTime)<12) );

	/*
	//For this to this to work, a minimum of 1 byte needs to be received at StartRadioRx above
	do
	{
		Read_RadioInterruptStatus();
		CurrentTime = GetCurrentTimerCount();
	}
	while( !(RadioReceiveBuffer[2] & 0x10) && ((CurrentTime-StartTime)<12) );
	*/

	if(SyncDetected)
	{
		/////////////////////////Ok message///////////////////////
#if 1
		//Read_RadioInterruptStatus();
        //ABR added
        StartTime = GetCurrentTimerCount();

		StartRadioTx(CHANNEL_0, OK_MESSAGE_LEN, RX_TUNE_STATE, NO_RETRANSMIT);//Transmit OK

		// some node is listening (we received chirp response) - we can't spread a rumour right now
		ReasonForCurrentRumor = 0;				// somebody out there is in listen mode! can't spread a rumour

		data = SKIP_TX + SYNC_4_BYTE;
		SetRadioProperty (SYNC, SYNC_CONFIG, data);
		data = 0x55;
		SetRadioProperty (SYNC, SYNC_BITS_BYTE_1, data);
		data = 0x55;
		SetRadioProperty (SYNC, SYNC_BITS_BYTE_2, data);
		data = 0x95;
		SetRadioProperty (SYNC, SYNC_BITS_BYTE_3, data);
		data = ReverseByte(LongMsgFirstByte);
		SetRadioProperty (SYNC, SYNC_BITS_BYTE_4, data);

		do
		{
			Read_RadioInterruptStatus();
            CurrentTime = GetCurrentTimerCount();
		}
		while( !(RadioReceiveBuffer[2] & 0x20) && ((CurrentTime-StartTime)<10) );
#else
		//Using the NIRQ is unreliable.Probably because of glitches on the line causing unreal interrupts.
#endif

		//Read_RadioInterruptStatus();

		////////////////////////Receive message///////////////////

		StartRadioRx(CHANNEL_0, JOIN_ACK_MESSAGE_LEN, TX_TUNE_STATE);	//Receive message. All expected messages here are
																		//of 17bytes. 1st byte used as sync, so only 16 left
		StartTime = GetCurrentTimerCount();
		do
		{
			Read_RadioInterruptStatus();
			CurrentTime = GetCurrentTimerCount();
		}
		while( !(RadioReceiveBuffer[2] & 0x10) && ((CurrentTime-StartTime)<12) );


		if(RadioReceiveBuffer[2] & 0x10)
		{
			DownloadMessage(JOIN_ACK_MESSAGE_LEN);
			CRCInit();
			CRCByte(LongMsgFirstByte);
			rcvbuf[0] = LongMsgFirstByte;
			for (uint8_t i=0; i<JOIN_ACK_MESSAGE_LEN; i++)
			{
				CRCByte(RxData[i]);
				rcvbuf[i+1] = RxData[i];
			}

			if( (GetCRC()==CRC_CHECK) && ((rcvbuf[2]&MSG_RUMOR_BIT)!=0) )
			{
				RumorSenderSID = rcvbuf[1]&SID_MASK;			// SID of node sending us this rumour
				RumorOldNews = ((rcvbuf[1]&OLD_NEWS_BIT)!=0);	// rumour sender sets bit if we have already seen this rumour
				RumorMsgByte = rcvbuf[2];						// extract message byte
				RumorInitiatorSID = rcvbuf[3]&SID_MASK;			// SID of rumour initiator
				RumorByte5 = rcvbuf[5];							// in case is targeted map request or status request rumour
				RumorByte6 = rcvbuf[6];							// used for MapMsgType and Rev12 indicator
				//RumorSenderSID = RxData[0];

				//////////////////////Send Rumour Ack Message//////////////
				ResetFIFO(Tx);									//Not needed, but to make sure TxFIFO is empty.
				BuildRumorAckMsg(&RadioTransmitBuffer[0]);
				UploadMessage(RUMORACK_MESSAGE_LEN);

				//Read_RadioInterruptStatus();//Not needed as the transmit complete flag is cleared while RadioRx above
                //ABR added
                StartTime = GetCurrentTimerCount();

				StartRadioTx(CHANNEL_0, RUMORACK_MESSAGE_LEN, READYSTATE, NO_RETRANSMIT);//Transmit Rumour Ack Message
				do
				{
					Read_RadioInterruptStatus();
                    CurrentTime = GetCurrentTimerCount();
				}
				while( !(RadioReceiveBuffer[2] & 0x20) && ((CurrentTime-StartTime)<12) );

				// Propagate request message detection code
				if(RumorInitiatorSID==SID)
				{
					;// we initiated this rumour - no need to process it
				}
				else if(RumorMsgByte==MSG_JOINED)
				{
					// rumour is about new node joining, or a neighbour update from an old node
					if(!TestBit(SIDMap,RumorInitiatorSID))
					{
						// this is a new node - we should tell it our NbrMap eventually
						BroadcastNbrMap = true;							// next time we check neighbours, broadcast results
						SetBit(SIDMap,RumorInitiatorSID);				// make sure existence marked */
					}

					// add connections. NOTE - for WiSafe2, last byte of map is actually remote SDAlarmIdent[0]
					// so last byte of map must be ignored by UpdateConnections

					//CopyMap(&rcvbuf[MSG_OFFSET_MAP],&rcvbuf[MSG_OFFSET_MAP],MAP_DITHER);		// undither neighbour map in place
					//UpdateConnections(RumorInitiatorSID,&rcvbuf[MSG_OFFSET_MAP]);				// NbrMap for new node (last byte ignored!)

					// save mesh info now that SIDMap has changed
					SaveMeshInfo();									// in case we reset, we can recover with new node

					// generate message to tell SD about new node (if not old news)
					if((OutgoingMsgLen==0) && (!RumorOldNews))
					{
						OutgoingMsg[0] = SPIMSG_RM_EXTENDED_RESPONSE;
						OutgoingMsg[1] = SPIMSG_EXTD_OPT_REMOTE_ID;		// new node report */
						OutgoingMsg[2] = RumorInitiatorSID;				// remote SID
						OutgoingMsg[3] = rcvbuf[MSG_OFFSET_MAP+7];		// eighth byte of map is remote SDAlarmIdent[0]
						OutgoingMsg[4] = rcvbuf[5];						// remote SDAlarmIdent[1]
						OutgoingMsg[5] = rcvbuf[6];						// remote SDAlarmIdent[2]
						OutgoingMsgLen = 6;
					}
					SetBit(NbrMap,RumorSenderSID);
				}
				else if(RumorMsgByte==MSG_SIDMAP_UPDATE)
				{
					// rumour is about new SIDMap
					CopyMap(&rcvbuf[MSG_OFFSET_MAP],&rcvbuf[MSG_OFFSET_MAP],MAP_DITHER);		// undither neighbour map in place
					SIDMapUpdate(&rcvbuf[MSG_OFFSET_MAP]);										// update all maps according to new SID map

					// generate message to tell SD about new SIDMap if not old news
					if((OutgoingMsgLen==0) && (!RumorOldNews))
					{
						OutgoingMsg[0] = SPIMSG_RM_EXTENDED_RESPONSE;
						OutgoingMsg[1] = SPIMSG_EXTD_OPT_SIDMAP_UPDATE;							// SIDMap update report
						CopyMap(&OutgoingMsg[2],SIDMap,0);										// 8 bytes
						OutgoingMsgLen = 10;
					}
				}
				else if(RumorMsgByte==MSG_SD_RUMOR)
				{
					// this is an SD transparent rumour
					// if this is an old rumour still drifting around, ignore it, since we have already told SD about it
					uint8_t RumorSeq;

					RumorSeq = SeqField(rcvbuf[4]);												// sequence number of current rumour
					if(RumorOldNews)
					{
						//PRINTF("OLD NEWS\n\r");// rumour sender has told us that we have already seen this rumour
					}
					else
					{
						// now tell SD about this rumour since it is new news
						CopyMap(OutgoingMsg,&rcvbuf[MSG_OFFSET_MAP],MAP_DITHER);				// copy and undither message
						OutgoingMsg[8] = rcvbuf[MSG_OFFSET_MAP-2];								// ninth byte stashed here
						OutgoingMsgLen = rcvbuf[MSG_OFFSET_MAP-1];								// length is just before message
						OutgoingMsg[OutgoingMsgLen++] = RumorInitiatorSID;						// append rumour initiator SID for SD
						OutgoingMsg[OutgoingMsgLen++] = RumorSeq;								// append sequence number of rumour for SD
						//PRINTF("RECEIVED %x. \n\r",OutgoingMsg[0]);
					}
					SetBit(NbrMap,RumorSenderSID);
				}
				else if((RumorMsgByte==MSG_STATUSREQ) && (SID==RumorByte5))
				{
					// targeted status request aimed at us! (target in byte 5 of incoming msg)
					ReasonsForRumor |= REASON_STATUS_REPLY;										// we'll send off our status when we get a chance
					StatusReplyTargetSID = RumorInitiatorSID;									// send results here
					SetBit(NbrMap,RumorSenderSID);
				}
				else if((RumorMsgByte==MSG_MAPREQ) && (SID==RumorByte5))
				{
					// targeted map request aimed at us! (target in byte 5 of incoming msg)
					ReasonsForRumor |= REASON_MAP_REPLY;										// we'll send off requested map when we get a chance
					MapReplyTargetSID = RumorInitiatorSID;										// send results here
					MapMsgType = RumorByte6;													// this byte from rumour contains type of map we should send
					SetBit(NbrMap,RumorSenderSID);
				}
				else if(RumorMsgByte==MSG_MAPREPLY)
				{
					// remote map data - pass to SD
					if(OutgoingMsgLen==0)
					{
						OutgoingMsg[0] = SPIMSG_RM_EXTENDED_RESPONSE;
						OutgoingMsg[1] = SPIMSG_EXTD_OPT_REMOTE_MAP;
						OutgoingMsg[2] = rcvbuf[MSG_OFFSET_MAP-1];								// map type is just before map
						CopyMap(&OutgoingMsg[3],&rcvbuf[MSG_OFFSET_MAP],MAP_DITHER);			// undither into outgoing message
						OutgoingMsg[11] = RumorInitiatorSID;									// append rumour initiator SID for SD
						OutgoingMsg[12] = SeqField(rcvbuf[4]);									// sequence number of current rumour
						OutgoingMsgLen = 13;
					}
					SetBit(NbrMap,RumorSenderSID);
				}
				else if(RumorMsgByte==MSG_STATUSREPLY)
				{
					// report status back to SD
					if (OutgoingMsgLen==0)
					{
						OutgoingMsg[0] = SPIMSG_RM_EXTENDED_RESPONSE;
						OutgoingMsg[1] = SPIMSG_EXTD_OPT_REMOTE_STATUS;							// remote status report
						OutgoingMsg[2] = RumorInitiatorSID;										// remote SID
						OutgoingMsg[3] = rcvbuf[5];												// VBAT unloaded
						OutgoingMsg[4] = rcvbuf[6];												// VBAT loaded
						// now 8 more values:	VBAT boosted loaded, last measured RSSI, FWREV,
						//						SDAlarmIdent[0..2], Critical Faults, Additional Info
						CopyMap(&OutgoingMsg[5],&rcvbuf[7],0);
						OutgoingMsgLen = 13;
					}
					SetBit(NbrMap,RumorSenderSID);
				}
				else if (RumorMsgByte==MSG_CONFIRM)
				{
					CopyMap(&rcvbuf[MSG_OFFSET_MAP],&rcvbuf[MSG_OFFSET_MAP],MAP_DITHER);		// undither map
					ProcessConfirmation(&rcvbuf[MSG_OFFSET_MAP],rcvbuf[5]);						// map in rcvbuf, RumorBuf[5] = oldttl????
					SetBit(NbrMap,RumorSenderSID);
				}
				success = true;
			}
		}
	}
	Read_RadioInterruptStatus();
	SetRadioProperty (INT_CTL, INT_CTL_ENABLE, NO_INT_EN);										// redundant
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x94;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
	return success;
}



/*******************************************************************************
 * \brief	Transmit a message.
 *
 * \param	TimeLeft. Time left to transmit
 * \return	True, if rumour message transmitted. False, if not.
 ******************************************************************************/

bool ListenToTransmitPacket(uint16_t TimeLeft)
{
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x40;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
	uint8_t chirpsdetected = 0;
	uint8_t okdetected = 0;
	uint8_t id;

	bool success;
	uint8_t data;
	uint8_t FirstByte;
	uint32_t StartTime, StartTimeNew, CurrentTime, TimeOut;

	success = false;
	TimeOut = (uint32_t)(TimeLeft<<8);//	x256 close to 250 as per the Timer.c IRQ handler

	FirstByte = ReverseByte((NetworkMeshID<<2)+MSG_LEN_SHORT);


	StartTime = GetCurrentTimerCount();
	CurrentTime = StartTime;

#if ENHANCED_DEBUG
	LOG_Info(LOG_YELLOW "TimeLeft = %d, TimeOut=%d, StartTime=%d",TimeLeft,TimeOut,StartTime);
#endif
	SyncDetected = true; //For first loop to load the chirp response data and prepare chirp as sync data

	while( !success && ((CurrentTime-StartTime)<TimeOut) )		//~3 second window
	{
#if ENHANCED_DEBUG
//		LOG_Info(LOG_YELLOW "CurrentTime=%d",CurrentTime);
#endif
		//Note: set 0x1210 to 0x80 so that Tx is not corrupted when no Preamble and Sync are transmitted
		if(SyncDetected)
		{
			ResetFIFO(Tx);


#if 1
			//It seems better if exactly 28bit preamble is sent
			RadioTransmitBuffer[0] = 0x2A;//0xAA
			RadioTransmitBuffer[1] = 0xAA;
			RadioTransmitBuffer[2] = 0xAA;
			RadioTransmitBuffer[3] = 0xA9;
			RadioTransmitBuffer[4] = ((MeshKey[0]<<2) + MSG_LEN_TINY);

			UploadMessage(0x05);
#else
				RadioTransmitBuffer[0] = 0xAA;
				RadioTransmitBuffer[1] = 0xAA;
				RadioTransmitBuffer[2] = 0xA9;
				RadioTransmitBuffer[3] = ((MeshKey[0]<<2) + MSG_LEN_TINY);

				UploadMessage(0x04);
#endif

			//Listen for a chirp
			data = SKIP_TX + SYNC_3_BYTE;
			SetRadioProperty (SYNC, SYNC_CONFIG, data);
			data = 0x55;									//Reversed 0xAA
			SetRadioProperty (SYNC, SYNC_BITS_BYTE_1, data);
			SetRadioProperty (SYNC, SYNC_BITS_BYTE_2, data);
			data = 0x15;									//Reversed 0xA8
			SetRadioProperty (SYNC, SYNC_BITS_BYTE_3, data);
		}

		SyncDetected = false;
		wait_syncMatch(0);  // ensure sync detect is reset
		Read_RadioInterruptStatus();
		StartRadioRx(CHANNEL_0, 0, TX_TUNE_STATE);////////////// Receive chirp //////////////////

#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x41;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
		wait_syncMatch((TimeOut+rand()%500)/2);  // wait for sync detection

#if ENHANCED_DEBUG
    //ABR added for testing
    WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x42;
    WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
		if(SyncDetected)
		{
#if ENHANCED_DEBUG
    //ABR added for testing
    WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x43;
    WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
			chirpsdetected++;
#if 1
			///////////////// Transmit chirp response /////////////////
#if 1
            //ABR added
            StartTimeNew = GetCurrentTimerCount();

			StartRadioTx(CHANNEL_0, 0x05, RX_TUNE_STATE, NO_RETRANSMIT);
#else
			StartRadioTx(CHANNEL_0, 0x04, RX_TUNE_STATE, NO_RETRANSMIT);
#endif
			Read_RadioInterruptStatus();
			data = SKIP_TX + SYNC_2_BYTE;
			SetRadioProperty (SYNC, SYNC_CONFIG, data);
			data = 0x95;
			SetRadioProperty (SYNC, SYNC_BITS_BYTE_1, data);
			data =  FirstByte;		//ReverseByte((NetworkMeshID<<2)+MSG_LEN_SHORT);
			SetRadioProperty (SYNC, SYNC_BITS_BYTE_2, data);
			do
			{
				Read_RadioInterruptStatus();
                CurrentTime = GetCurrentTimerCount();
			}
			while( !(RadioReceiveBuffer[2] & 0x20) && ((CurrentTime-StartTimeNew)<12) );
#else
			//NIRQ not working properly.
#endif
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x44;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
			//Listen for Ok message
			StartRadioRx(CHANNEL_0, 3, TX_TUNE_STATE);//Receive Ok. No need to receive and check CRC bytes. Quick response needed.

			ResetFIFO(Tx);
			//BuildRumorMessage(&RadioTransmitBuffer[0], CurrentMessage);
			CopyMemory(&RadioTransmitBuffer[0],&msgbuf[0],RUMOR_MESSAGE_LEN);
			UploadMessage(RUMOR_MESSAGE_LEN);

			StartTimeNew = GetCurrentTimerCount();
			do
			{
				Read_RadioInterruptStatus();
				CurrentTime = GetCurrentTimerCount();
			}
			while( !(RadioReceiveBuffer[2] & 0x10) && ((CurrentTime-StartTimeNew)<12) );
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x45;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif

			DownloadMessage(3);
			id = RxData[0] & SID_MASK;

			/*
			//Need to use above to make sure we transmit after receiving all data bytes
			SyncDetected = false;
			StartTimeNew = GetCurrentTimerCount();
			do
			{
				//SyncDetected = Read_SyncDetectStatus();
				CurrentTime = GetCurrentTimerCount();
			}
			while( (!SyncDetected) && ((CurrentTime-StartTimeNew)<20) );
			*/

			//need to read the Rx FIFO and check if the SID is part of the RumorMap?
			//if(SyncDetected)//&& SID is part of RumorMap)
			if( (RadioReceiveBuffer[2] & 0x10) && TestBit(SIDMap,id) )
			{
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x46;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
				okdetected++;
				SetBit(NbrMap,id);
				//Send the rumour

				if((ReasonForCurrentRumor!=REASON_PROPAGATE_RUMOR) && TestBit(RumorMap,id))
				{
					PatchMsgWithOldNews(false);						// rumour is new to this node, so make sure "old news" bit is cleared
					Delay(1);										// Do not change the 1msec delay. Looks like it is optimum time to makes the rumour message received by the RM.
					Read_RadioInterruptStatus();
                    //ABR added
                    StartTimeNew = GetCurrentTimerCount();
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x47;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif

					StartRadioTx(CHANNEL_0, RUMOR_MESSAGE_LEN, RX_TUNE_STATE, NO_RETRANSMIT);//Transmit rumour message
					do
					{
						Read_RadioInterruptStatus();
                        CurrentTime = GetCurrentTimerCount();
					}
					while( !(RadioReceiveBuffer[2] & 0x20) && ((CurrentTime-StartTimeNew)<20) );
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x48;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif

					if(ReceiveRumourAckMessage(id) && AckRumorForUs(id))
					{
						ClearBit(RumorMap,id);
						// note - could keep count of bits in RumorMap for faster
						if (CountBits(RumorMap)==0)
						{
							ListenTimeLeft = 0;						// can stop listening early
						}
						success = true;
					}
				}
				else if((ReasonForCurrentRumor==REASON_PROPAGATE_RUMOR) && TestBit(PropagateMap,id))
				{
					if (TestBit(RumorMap,id))
					{
						PatchMsgWithOldNews(false);						// rumour is new to this node, so make sure "old news" bit is cleared
					}
					else
					{
						PatchMsgWithOldNews(true);						// rumour is not new to this node, so make sure "old news" bit is set
					}
					Delay(1);										// Do not change the 1msec delay. Looks like it is optimum time to makes the rumour message received by the RM.
					Read_RadioInterruptStatus();
                    //ABR added
                    StartTimeNew = GetCurrentTimerCount();

#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x49;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
					StartRadioTx(CHANNEL_0, RUMOR_MESSAGE_LEN, RX_TUNE_STATE, NO_RETRANSMIT);//Transmit rumour message
					do
					{
						Read_RadioInterruptStatus();
                        CurrentTime = GetCurrentTimerCount();
					}
					while( !(RadioReceiveBuffer[2] & 0x20) && ((CurrentTime-StartTimeNew)<20) );
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x4A;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif

					if(ReceiveRumourAckMessage(id) && AckRumorForUs(id))
					{
						ClearBit(RumorMap,id);
						// note - could keep count of bits in RumorMap for faster
						if (CountBits(RumorMap)==0)
						{
							ReasonsForRumor &= ~REASON_PROPAGATE_RUMOR;
							ReasonForCurrentRumor = REASON_SPREAD_RUMOR;
							ReasonsForRumor |= ReasonForCurrentRumor;
							ListenTimeLeft = 0;						// can stop listening early
						}
						else
						{
							BuildPropagateMsg(id);
							CopyMemory(&RadioTransmitBuffer[0],&msgbuf[0],RUMOR_MESSAGE_LEN);
							UploadMessage(RUMOR_MESSAGE_LEN);
							Read_RadioInterruptStatus();
                            //ABR added
                            StartTimeNew = GetCurrentTimerCount();

#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x4B;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
							StartRadioTx(CHANNEL_0, RUMOR_MESSAGE_LEN, RX_TUNE_STATE, NO_RETRANSMIT);//Transmit rumour message
							do
							{
								Read_RadioInterruptStatus();
                                CurrentTime = GetCurrentTimerCount();
							}
							while( !(RadioReceiveBuffer[2] & 0x20) && ((CurrentTime-StartTimeNew)<20) );
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x4C;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif

							if (PropagationAccepted(id))
							{
								// it worked!  other node will take over
								// we have successfully propagated out the rumour we had
								ReasonsForRumor &= ~ReasonForCurrentRumor;					// our reason has been satisfied
								ReasonForCurrentRumor = 0;									// rumour is gone
								ListenTimeLeft = 0;											// can stop listening early
							}
							else
							{
								// other node refused to propagate, or message got corrupted
								CopyRumorToMsg();											// attempt to propagate failed - reload rumour
							}
						}
						success = true;
					}
				}
			}
		}
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x4D;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
		CurrentTime = GetCurrentTimerCount();
	}
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x4E;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif

	Read_RadioInterruptStatus();
	SetRadioProperty (INT_CTL, INT_CTL_ENABLE, NO_INT_EN);
	//PRINTF("Chirps=%d, Oks=%d  \r\n",chirpsdetected,okdetected);
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x4F;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
        return success;
}



/*******************************************************************************
 * \brief	Change radio operational state.
 *
 * \param	nextstate. Radio next state to be entered.
 * \return	None.
 ******************************************************************************/

void ChangeState(uint8_t nextstate)
{
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x16;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
		RadioTransmitBuffer[0] = nextstate;

		//RadioCTS = Get_RadioCTS();
		//if( RadioCTS )
		do
		{
		  	RadioCTS = Get_RadioCTS();
		}
		while(!RadioCTS);

		Write_RadioCommand(0x34, 1, NO_RESPONSE_LEN);
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x96;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
}



/*******************************************************************************
 * \brief	Start radio receive.
 *
 * \param	channel. Radio channel.
 * \param	len. Length of bytes to be received.
 * \param	nextstate. Radio state after receive completion.
 * \return	None.
 ******************************************************************************/

void StartRadioRx(uint8_t channel, uint8_t len, uint8_t nextstate)
{
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x17;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif

	ResetFIFO(Rx);

	RadioTransmitBuffer[0] = channel;
	RadioTransmitBuffer[1] = 0x00;
	RadioTransmitBuffer[2] = 0x00;
	RadioTransmitBuffer[3] = len;
	RadioTransmitBuffer[4] = NOCHANGESTATE;
	RadioTransmitBuffer[5] = nextstate;
	RadioTransmitBuffer[6] = NOCHANGESTATE;

	//RadioCTS = Get_RadioCTS();
	//if( RadioCTS )
	do
	{
	   	RadioCTS = Get_RadioCTS();
	}
	while(!RadioCTS);

	{
		Write_RadioCommand(START_RX, 7, NO_RESPONSE_LEN);
	}
	//else
	{
	   	//PRINTF("Error1. \r\n");
	}
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x97;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
}



/*******************************************************************************
 * \brief	Start radio transmit.
 *
 * \param	channel. Radio channel.
 * \param	len. Length of bytes to be transmitted.
 * \param	nextstate. Radio state after transmit completed.
 * \param	retransmit. If 1, repeat last transmission. If 0, no retransmit.
 * \return	None.
 ******************************************************************************/

void StartRadioTx(uint8_t channel, uint8_t len, uint8_t nextstate, uint8_t retransmit)
{
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x18;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
	RadioTransmitBuffer[0] = channel;
	RadioTransmitBuffer[1] = (nextstate<<4) + retransmit;//Start TX immediately and switch to RX state when transmission completed
	RadioTransmitBuffer[2] = 0x00;
	RadioTransmitBuffer[3] = len;
	RadioTransmitBuffer[4] = 0x00;
	RadioTransmitBuffer[5] = 0x00;

	//RadioCTS = Get_RadioCTS();
	//if( RadioCTS )
	do
	{
	   	RadioCTS = Get_RadioCTS();
	}
	while(!RadioCTS);

	Write_RadioCommand(START_TX, 6, NO_RESPONSE_LEN);
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x98;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
}



/*******************************************************************************
 * \brief	Upload message to radio FIFO transmit buffer.
 *
 * \param	len. Length of bytes to be written.
 * \return	None.
 ******************************************************************************/

void UploadMessage(uint8_t len)
{
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x19;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
	//RadioCTS = Get_RadioCTS();
	//if( RadioCTS )
	do
	{
	   	RadioCTS = Get_RadioCTS();
	}
	while(!RadioCTS);

	{
		Write_RadioCommand(WRITE_TX_FIFO, len, NO_RESPONSE_LEN);
	}
	//else
	{
		//PRINTF("Upload Message failed \r\n");
	}
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x99;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
}



/*******************************************************************************
 * \brief	Download message from radio FIFO receive buffer.
 *
 * \param	len. Length of bytes to be read.
 * \return	None.
 ******************************************************************************/

void DownloadMessage(uint8_t len)
{
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x1A;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
	//RadioCTS = Get_RadioCTS();
	//if( RadioCTS )
	do
	{
	   	RadioCTS = Get_RadioCTS();
	}
	while(!RadioCTS);

	ReadRxBuffer(len);

#if SCRAMBLED_WISAFE
	// ABR Scramble
	for(uint8_t count=3; count<len; count++)    //Only fifth (index 3 in RxData) byte onward in the received messages is scrambled
	{
		RxData[count] = RxData[count] ^ Scramble0;
	}
#endif
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x9A;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
}



/*******************************************************************************
 * \brief	Reset radio FIFO buffersr.
 *
 * \param	rxtx. Buffer to be reset. Receive, transmit, or both.
 * \return	None.
 ******************************************************************************/

void ResetFIFO(uint8_t rxtx)
{
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x1B;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
	RadioTransmitBuffer[0] = rxtx;

	//Get_RadioCTS();
	//if( RadioCTS )
	do
	{
	   	RadioCTS = Get_RadioCTS();
	}
	while(!RadioCTS);

	Write_RadioCommand(FIFO_INFO, FIFO_INFO_COMMAND_LEN, FIFO_INFO_RESPONSE_LEN);

	RxFifoCount = RadioReceiveBuffer[0];
	TxFifoSpace = RadioReceiveBuffer[1];
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x9B;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
}


#if SCRAMBLED_WISAFE
/*******************************************************************************
 * \brief   Scramble messages for transmission.
 *
 * \param   type. Scrambling actual message buffer or a dummy buffer.
 * \return  None.
 ******************************************************************************/
void MsgScramble(uint8_t type)
{
#if ENHANCED_DEBUG
    //ABR added for testing
    WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x1C;
    WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
    if(type==1)
    {

        for (uint8_t i=8; i<=20; i++)
        {
            msgbuf[i] = msgbuf[i] ^ Scramble0;
        }

        /*
        //msgbuf[7] = msgbuf[7] ^ Scramble0;
        msgbuf[8] = msgbuf[8] ^ Scramble0;
        msgbuf[9] = msgbuf[9] ^ Scramble0;
        msgbuf[10] = msgbuf[10] ^ Scramble0;
        msgbuf[11] = msgbuf[11] ^ Scramble0;
        msgbuf[12] = msgbuf[12] ^ Scramble0;
        msgbuf[13] = msgbuf[13] ^ Scramble0;
        msgbuf[14] = msgbuf[14] ^ Scramble0;
        msgbuf[15] = msgbuf[15] ^ Scramble0;
        msgbuf[16] = msgbuf[16] ^ Scramble0;
        msgbuf[17] = msgbuf[17] ^ Scramble0;
        msgbuf[18] = msgbuf[18] ^ Scramble0;
        msgbuf[19] = msgbuf[19] ^ Scramble0;
        msgbuf[20] = msgbuf[20] ^ Scramble0;
        */
    }
    else if(type==2)
    {
        for (uint8_t i=8; i<=20; i++)
        {
            RadioTransmitBuffer[i] = RadioTransmitBuffer[i] ^ Scramble0;
        }
    }
    else
    {
        for (uint8_t i=8; i<=20; i++)
        {
            dummymsgbuf[i] = dummymsgbuf[i] ^ Scramble0;
        }
    }
#if ENHANCED_DEBUG
    //ABR added for testing
    WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x9C;
    WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif

}
#endif



/*******************************************************************************
 * \brief	Continuous transmit of modulation frequency.
 *
 * \param	None.
 * \return	None.
 ******************************************************************************/

void RadioContinuousTransmit(void)
{
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x1D;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
	uint8_t newfreqfrac2, newfreqfrac3;
	uint16_t systimestart;
	uint8_t finestepchange = 5;


	//PRINTF("In continuous transmit mode.\n\r");

	systimestart = SysTime;
	newfreqfrac2 = FREQ_CONTROL_FRAC2_DEFAULT;
	newfreqfrac3 = FREQ_CONTROL_FRAC3_DEFAULT;

	RadioInContinuousTransmit = true;
	SetRadioProperty (MODEM_MOD_TYPE_GROUP, MODEM_MOD_TYPE_INDEX0, MOD_TYPE_CW);	// Continuous transmit
	ChangeState(TXSTATE);

	while( (SysTime-systimestart)<MINUTE_SYS_TIME_COUNT)
	{
		if(SDMsgRcvd == SPIMSG_RM_INC_FREQ)
		{
			//PRINTF("Increment frequency.\n\r");
			newfreqfrac2++;
			SetRadioProperty (FREQ_CONTROL_FRAC_GROUP, FREQ_CONTROL_FRAC_INDEX2, newfreqfrac2);
			ChangeState(READYSTATE);
			ChangeState(TXSTATE);
			systimestart = SysTime;				// reset time for new one minute
			SDMsgRcvd = SPIMSG_NULL;
		}

		if(SDMsgRcvd == SPIMSG_RM_FINE_INC_FREQ)
		{
			//PRINTF("Fine increment frequency.\n\r");
			if(newfreqfrac3 <= (255-finestepchange))
			{
				newfreqfrac3 = newfreqfrac3 + finestepchange;
				SetRadioProperty (FREQ_CONTROL_FRAC_GROUP, FREQ_CONTROL_FRAC_INDEX3, newfreqfrac3);
				ChangeState(READYSTATE);
				ChangeState(TXSTATE);
				systimestart = SysTime;				// reset time for new one minute
			}
			SDMsgRcvd = SPIMSG_NULL;
		}

		if(SDMsgRcvd == SPIMSG_RM_DEC_FREQ)
		{
			//PRINTF("Decrement frequency.\n\r");
			newfreqfrac2--;
			SetRadioProperty (FREQ_CONTROL_FRAC_GROUP, FREQ_CONTROL_FRAC_INDEX2, newfreqfrac2);
			ChangeState(READYSTATE);
			ChangeState(TXSTATE);
			systimestart = SysTime;				// reset time for new one minute
			SDMsgRcvd = SPIMSG_NULL;
		}

		if(SDMsgRcvd == SPIMSG_RM_FINE_DEC_FREQ)
		{
			//PRINTF("Fine decrement frequency.\n\r");
			if(newfreqfrac3 >= finestepchange)
			{
				newfreqfrac3 = newfreqfrac3 - finestepchange;
				SetRadioProperty (FREQ_CONTROL_FRAC_GROUP, FREQ_CONTROL_FRAC_INDEX3, newfreqfrac3);
				ChangeState(READYSTATE);
				ChangeState(TXSTATE);
				systimestart = SysTime;				// reset time for new one minute
			}
			SDMsgRcvd = SPIMSG_NULL;
		}


		if(SDMsgRcvd == SPIMSG_RM_EXIT_FREQ)
		{
			//PRINTF("Exit continuous transmit mode.\n\r");
			SetRadioProperty (FREQ_CONTROL_FRAC_GROUP, FREQ_CONTROL_FRAC_INDEX2, FREQ_CONTROL_FRAC2_DEFAULT);
			SetRadioProperty (FREQ_CONTROL_FRAC_GROUP, FREQ_CONTROL_FRAC_INDEX3, FREQ_CONTROL_FRAC3_DEFAULT);
			SDMsgRcvd = SPIMSG_NULL;
			break;								//Exit while loop
		}

		if(SDMsgRcvd == SPIMSG_RM_SAVE_FREQ)
		{
			//PRINTF("Save and exit continuous transmit mode.\n\r");
			SaveFreqFrac(newfreqfrac2, newfreqfrac3);
			SDMsgRcvd = SPIMSG_NULL;
			break;								//Exit while loop
		}
	}

	SetRadioProperty (MODEM_MOD_TYPE_GROUP, MODEM_MOD_TYPE_INDEX0, MOD_TYPE_2GFSK);// back to normal 2GFSK transmit mode
	ChangeState(READYSTATE);
	RadioInContinuousTransmit = false;
#if ENHANCED_DEBUG
	//ABR added for testing
	WiSafe_RM_Task_Point[WiSafe_RM_Task_Point_Index] = 0x9D;
	WiSafe_RM_Task_Point_Index = (WiSafe_RM_Task_Point_Index + 1) % sizeof(WiSafe_RM_Task_Point);
#endif
	//PRINTF("Back to normal transmit mode.\n\r");
}




