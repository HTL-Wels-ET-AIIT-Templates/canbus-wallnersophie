/**
 ******************************************************************************
 * @file           : can.c
 * @brief          : CAN handling functions
 * @author: L. Hubmer
 ******************************************************************************
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "main.h"
#include "stm32f429i_discovery_lcd.h"
#include "tempsensor.h"
#include "ringbuffer.h"

/* Private define ------------------------------------------------------------*/
#define CAN1_CLOCK_PRESCALER  16   // Adjust for correct bitrate

/* Private variables ---------------------------------------------------------*/
CAN_HandleTypeDef canHandle;
uint8_t OtherTransmissionInProgress = 0;

/* TX/RX buffers */
static CAN_TxHeaderTypeDef txHeader;
static uint8_t  txData[8];
static uint32_t txMailbox;

static CAN_RxHeaderTypeDef rxHeader;
static uint8_t  rxData[8];

#define CAN_RX_MSG_BUFFER_LEN 256

static RingBuffer_t CanRxMsgStdId;
static RingBuffer_t CanRxMsgByte0;
static RingBuffer_t CanRxMsgByte1;
static RingBuffer_t CanRxMsgByte2;
static RingBuffer_t CanRxMsgByte3;
static RingBuffer_t CanRxMsgByte4;
static RingBuffer_t CanRxMsgByte5;
static RingBuffer_t CanRxMsgByte6;
static RingBuffer_t CanRxMsgByte7;

static uint8_t CanRxMsgStdId_Array[CAN_RX_MSG_BUFFER_LEN] = {0};
static uint8_t CanRxMsgByte0_Array[CAN_RX_MSG_BUFFER_LEN] = {0};
static uint8_t CanRxMsgByte1_Array[CAN_RX_MSG_BUFFER_LEN] = {0};
static uint8_t CanRxMsgByte2_Array[CAN_RX_MSG_BUFFER_LEN] = {0};
static uint8_t CanRxMsgByte3_Array[CAN_RX_MSG_BUFFER_LEN] = {0};
static uint8_t CanRxMsgByte4_Array[CAN_RX_MSG_BUFFER_LEN] = {0};
static uint8_t CanRxMsgByte5_Array[CAN_RX_MSG_BUFFER_LEN] = {0};
static uint8_t CanRxMsgByte6_Array[CAN_RX_MSG_BUFFER_LEN] = {0};
static uint8_t CanRxMsgByte7_Array[CAN_RX_MSG_BUFFER_LEN] = {0};


/* Private function prototypes -----------------------------------------------*/
static void initGpio(void);
static void initCanPeripheral(void);
void canSaveToBuffer();

/**
 * Initialize GPIO and CAN peripheral
 */

void canInitHardware(void) {
	initGpio();
	initCanPeripheral();
}

/**
 * CAN initialization (including display)
 */
void canInit(void) {

	canInitHardware();

	ringBufferInit(&CanRxMsgStdId, CanRxMsgStdId_Array, CAN_RX_MSG_BUFFER_LEN);
	ringBufferInit(&CanRxMsgByte0, CanRxMsgByte0_Array, CAN_RX_MSG_BUFFER_LEN);
	ringBufferInit(&CanRxMsgByte1, CanRxMsgByte1_Array, CAN_RX_MSG_BUFFER_LEN);
	ringBufferInit(&CanRxMsgByte2, CanRxMsgByte2_Array, CAN_RX_MSG_BUFFER_LEN);
	ringBufferInit(&CanRxMsgByte3, CanRxMsgByte3_Array, CAN_RX_MSG_BUFFER_LEN);
	ringBufferInit(&CanRxMsgByte4, CanRxMsgByte4_Array, CAN_RX_MSG_BUFFER_LEN);
	ringBufferInit(&CanRxMsgByte5, CanRxMsgByte5_Array, CAN_RX_MSG_BUFFER_LEN);
	ringBufferInit(&CanRxMsgByte6, CanRxMsgByte6_Array, CAN_RX_MSG_BUFFER_LEN);
	ringBufferInit(&CanRxMsgByte7, CanRxMsgByte7_Array, CAN_RX_MSG_BUFFER_LEN);


	LCD_SetFont(&Font12);
	LCD_SetColors(LCD_COLOR_WHITE, LCD_COLOR_BLACK);
	LCD_SetPrintPosition(3,1);
	printf("CAN1: Send-Recv");

	LCD_SetColors(LCD_COLOR_GREEN, LCD_COLOR_BLACK);
	LCD_SetPrintPosition(5,1);
	printf("Send-Cnt:");
	LCD_SetPrintPosition(5,15);
	printf("%5d", 0);

	LCD_SetPrintPosition(7,1);
	printf("Recv-Cnt:");
	LCD_SetPrintPosition(7,15);
	printf("%5d", 0);

	LCD_SetPrintPosition(9,1);
	printf("Send-Data:");

	LCD_SetPrintPosition(15,1);
	printf("Recv-Data:");

	LCD_SetPrintPosition(30,1);
	printf("Bit-Timing-Register: 0x%lx", CAN1->BTR);



	/* ----------------------
	 * Initialize DS18B20
	 * --------------------- */
	//   ds18b20_init();
	//   ds18b20_startMeasure();
}

/**
 * Task: Send a CAN message every cycle
 */


void canSendLetter(char Letter, uint16_t check_number) {
	static unsigned int sendCnt = 0;

//	while(OtherTransmissionInProgress)	{};

	/* Prepare CAN header */
	txHeader.StdId = 0x003;
	txHeader.IDE   = CAN_ID_STD;
	txHeader.RTR   = CAN_RTR_DATA;
	txHeader.DLC   = 3;

	/* Data payload (temperature) */
	txData[0] = Letter;
	txData[1] = ((uint8_t)check_number >> 8) & 0xFF;
	txData[2] = (uint8_t)check_number & 0xFF;



	/* Send CAN frame */
	if (HAL_CAN_AddTxMessage(&canHandle, &txHeader, txData, &txMailbox) == HAL_OK) {
		sendCnt++;

		/* Display send counter */
		LCD_SetColors(LCD_COLOR_GREEN, LCD_COLOR_BLACK);
		LCD_SetPrintPosition(5,15);
		printf("%5d", sendCnt);

		/* Display temperature sent */
		LCD_SetPrintPosition(9,1);
		printf("Send-Data: %c",txData[0]);
	}
}

void canSendBegin(char Sender[8]) {
	static unsigned int sendCnt = 0;
//	while(OtherTransmissionInProgress){};

	/* Prepare CAN header */
	txHeader.StdId = 0x001;
	txHeader.IDE   = CAN_ID_STD;
	txHeader.RTR   = CAN_RTR_DATA;
	txHeader.DLC   = 8;

	/* Data payload (temperature) */
	for( int i = 0; i < 8; i++)
	{
		txData[i] = Sender[i];
	}


	/* Send CAN frame */
	if (HAL_CAN_AddTxMessage(&canHandle, &txHeader, txData, &txMailbox) == HAL_OK) {
		sendCnt++;

		/* Display send counter */
		LCD_SetColors(LCD_COLOR_GREEN, LCD_COLOR_BLACK);
		LCD_SetPrintPosition(5,15);
		printf("%5d", sendCnt);

		/* Display temperature sent */
		LCD_SetPrintPosition(9,1);
		printf("Send-Data: %c",txData[0]);
	}
}
void canSendEnd() {
	static unsigned int sendCnt = 0;
//	while(OtherTransmissionInProgress){};

	/* Prepare CAN header */
	txHeader.StdId = 0x002;
	txHeader.IDE   = CAN_ID_STD;
	txHeader.RTR   = CAN_RTR_DATA;
	txHeader.DLC   = 1;

	/* Data payload (temperature) */
	txData[0] = 'E';


	/* Send CAN frame */
	if (HAL_CAN_AddTxMessage(&canHandle, &txHeader, txData, &txMailbox) == HAL_OK) {
		sendCnt++;

		/* Display send counter */
		LCD_SetColors(LCD_COLOR_GREEN, LCD_COLOR_BLACK);
		LCD_SetPrintPosition(5,15);
		printf("%5d", sendCnt);

		/* Display temperature sent */
		LCD_SetPrintPosition(9,1);
		printf("Send-Data: %c",txData[0]);
	}
}
/**
 * Task: Receive CAN messages if available
 */
void canReceiveTask(RingBuffer_t* MsgRecieve) {
	static unsigned int recvCnt = 0;
	static uint16_t PrevCheckNumber;

	/* Check for RX pending */
	if (ringBufferLen(&CanRxMsgByte0) == 0)
		return;

	recvCnt++;

	uint8_t RxDataStdId = ringBufferGetOne(&CanRxMsgStdId);
	uint8_t RxDataByte[8];
	RxDataByte[0] = ringBufferGetOne(&CanRxMsgByte0);
	RxDataByte[1] = ringBufferGetOne(&CanRxMsgByte1);
	RxDataByte[2] = ringBufferGetOne(&CanRxMsgByte2);
	RxDataByte[3] = ringBufferGetOne(&CanRxMsgByte3);
	RxDataByte[4] = ringBufferGetOne(&CanRxMsgByte4);
	RxDataByte[5] = ringBufferGetOne(&CanRxMsgByte5);
	RxDataByte[6] = ringBufferGetOne(&CanRxMsgByte6);
	RxDataByte[7] = ringBufferGetOne(&CanRxMsgByte7);

	/* Extract temperature */
	int16_t checkNumber = (RxDataByte[1] << 8) | RxDataByte[2];
	int16_t Head = RxDataStdId;


	LCD_SetColors(LCD_COLOR_GREEN, LCD_COLOR_BLACK);
	LCD_SetPrintPosition(7,15);
	printf("%5d", recvCnt);

	LCD_SetPrintPosition(15,1);
	printf("Recv-Data: %02X %02X %02X %02X ",rxData[0], rxData[1], rxData[2], rxData[3]);
	LCD_SetPrintPosition(16,1);
	printf("Recv-Data: %c %c %c %c ",rxData[0], rxData[1], rxData[2], rxData[3]);
	LCD_SetPrintPosition(17,1);
	printf("Recv-Head: 0x%04X ",Head);

	//If recieving a Begin

	if(RxDataStdId == 0x001)
	{
		ringBufferAppendOne(MsgRecieve, 0x0D);
		ringBufferAppendOne(MsgRecieve, '<');
		for(int i = 0; i < 8; i++)
		{
			ringBufferAppendOne(MsgRecieve, RxDataByte[i]);
		}
		ringBufferAppendOne(MsgRecieve, '>');
		ringBufferAppendOne(MsgRecieve, ':');
		ringBufferAppendOne(MsgRecieve, ' ');
	}
	//If recieving an End
	if(RxDataStdId == 0x002)
	{
		PrevCheckNumber = 0;
		ringBufferAppendOne(MsgRecieve, 0x0D);
	}

	//If recieveing a letter send to uart
	if(RxDataStdId == 0x003)
	{
		if(PrevCheckNumber + 1 == checkNumber)
		{
			ringBufferAppendOne(MsgRecieve, RxDataByte[0]);
			PrevCheckNumber = checkNumber;
		}else
		{
			LCD_SetPrintPosition(22,1);
			printf("Lost Letter ERROR");
		}
	}

}

/*
 * Initialize GPIO pins PB8 (RX) and PB9 (TX)
 */
static void initGpio(void) {
	GPIO_InitTypeDef canPins;

	__HAL_RCC_GPIOB_CLK_ENABLE();

	canPins.Alternate = GPIO_AF9_CAN1;
	canPins.Mode = GPIO_MODE_AF_OD;
	canPins.Pin = GPIO_PIN_8 | GPIO_PIN_9;
	canPins.Pull = GPIO_PULLUP;
	canPins.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(GPIOB, &canPins);
}

/**
 * Initialize CAN peripheral
 */
static void initCanPeripheral(void) {

	CAN_FilterTypeDef canFilter;

	__HAL_RCC_CAN1_CLK_ENABLE();

	canHandle.Instance = CAN1;
	canHandle.Init.TimeTriggeredMode = DISABLE;
	canHandle.Init.AutoBusOff = DISABLE;
	canHandle.Init.AutoWakeUp = DISABLE;
	canHandle.Init.AutoRetransmission = ENABLE;
	canHandle.Init.ReceiveFifoLocked = DISABLE;
	canHandle.Init.TransmitFifoPriority = DISABLE;
	canHandle.Init.Mode = CAN_MODE_LOOPBACK;
	canHandle.Init.SyncJumpWidth = CAN_SJW_1TQ;

	canHandle.Init.TimeSeg1 = CAN_BS1_15TQ;
	canHandle.Init.TimeSeg2 = CAN_BS2_6TQ;
	canHandle.Init.Prescaler = CAN1_CLOCK_PRESCALER;

	if (HAL_CAN_Init(&canHandle) != HAL_OK)
		Error_Handler();

	/* Accept all messages */
	canFilter.FilterBank = 0;
	canFilter.FilterMode = CAN_FILTERMODE_IDMASK;
	canFilter.FilterScale = CAN_FILTERSCALE_32BIT;
	canFilter.FilterIdHigh = 0x0000;
	canFilter.FilterIdLow = 0x0000;
	canFilter.FilterMaskIdHigh = 0x0000;
	canFilter.FilterMaskIdLow = 0x0000;
	canFilter.FilterFIFOAssignment = CAN_RX_FIFO0;
	canFilter.FilterActivation = ENABLE;
	canFilter.SlaveStartFilterBank = 14;

	if (HAL_CAN_ConfigFilter(&canHandle, &canFilter) != HAL_OK)
		Error_Handler();

	if (HAL_CAN_Start(&canHandle) != HAL_OK)
		Error_Handler();

	//Activate Rx Interrupt
	if (HAL_CAN_ActivateNotification(&canHandle, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
	{
		printf("/* Notification Error */\r\n");
		Error_Handler();
	}
	HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 5, 0);
	HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
}

/**
 * CAN RX interrupt handler
 */
void CAN1_RX0_IRQHandler(void) {
	HAL_CAN_IRQHandler(&canHandle);

}

/**
 * FIFO0 message pending callback (unused)
 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
	//Save Message in Ringbuffer
	canSaveToBuffer();


}

void canSaveToBuffer() {
	//Save Message in Ringbuffer

	if(HAL_CAN_GetRxMessage(&canHandle, CAN_RX_FIFO0, &rxHeader, rxData) != HAL_OK)
		return;
	ringBufferAppendOne(&CanRxMsgStdId, rxHeader.StdId);
	ringBufferAppendOne(&CanRxMsgByte0, rxData[0]);
	ringBufferAppendOne(&CanRxMsgByte1, rxData[1]);
	ringBufferAppendOne(&CanRxMsgByte2, rxData[2]);
	ringBufferAppendOne(&CanRxMsgByte3, rxData[3]);
	ringBufferAppendOne(&CanRxMsgByte4, rxData[4]);
	ringBufferAppendOne(&CanRxMsgByte5, rxData[5]);
	ringBufferAppendOne(&CanRxMsgByte6, rxData[6]);
	ringBufferAppendOne(&CanRxMsgByte7, rxData[7]);

	if(rxHeader.StdId == 0x001)
		OtherTransmissionInProgress = 1;

	if(rxHeader.StdId == 0x003)
		OtherTransmissionInProgress = 0;
}
