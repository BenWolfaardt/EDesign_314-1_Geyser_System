/*
 * user.c
 *
 * Parts of code taken from Lecturers code on SunLearn from first demo
 *
 *  Created on: 05 Mar 2018
 *      Author: 18321933
 */

#include "user.h"

#define cmdBufL 50   		// maximum length of a command string received on the UART
#define maxTxL  50  		// maximum length of transmit buffer (replies sent back to UART)

volatile bool uartRxFlag;	// use 'volatile' keyword because the variable is changed from interrupt handler
volatile bool systickFlag;

char uartRxChar;			// temporary storage
char cmdBuf[cmdBufL];  		// buffer in which to store commands that are received from the UART
char txBuf[maxTxL]; 		// buffer for replies that are to be sent out on UART

char* txStudentNo = "$A,18321933\r\n";

uint8_t numberMap[10];
uint8_t pinsValue[4];
uint8_t segements[4];

int16_t tempSetpoint;		// the current temperature set point

uint16_t cmdBufPos;  		// this is the position in the cmdB where we are currently writing to

void UserInitialise(void)
{
	uartRxFlag = false;
	systickFlag = false;
	tempSetpoint = 60;		// initial value

	numberMap[0] = 0b00111111;
	numberMap[1] = 0b00000110;
	numberMap[2] = 0b01011011;
	numberMap[3] = 0b01001111;
	numberMap[4] = 0b01100110;
	numberMap[5] = 0b01101101;
	numberMap[6] = 0b01111101;
	numberMap[7] = 0b00000111;
	numberMap[8] = 0b01111111;
	numberMap[9] = 0b01100111;

	segements[0] = 0b0001;
	segements[1] = 0b0010;
	segements[2] = 0b0100;
	segements[3] = 0b1000;

	pinsValue[0] = numberMap[1];
	pinsValue[1] = numberMap[9];
	pinsValue[2] = numberMap[5];
	pinsValue[3] = numberMap[2];

	HAL_UART_Receive_IT(&huart1, (uint8_t*)&uartRxChar, 1);	// UART interrupt after 1 character was received
}

void User(void)
{

	if (uartRxFlag)
	{
		if (uartRxChar == '$')
			cmdBufPos = 0;

		// add character to command buffer, but only if there is more space in the command buffer
		if (cmdBufPos < cmdBufL)
			cmdBuf[cmdBufPos++] = uartRxChar;

		if ((cmdBufPos >= 4) && (cmdBuf[0] == '$') && (cmdBuf[cmdBufPos-2] == '\r') && (cmdBuf[cmdBufPos-1] == '\n'))
		{
			DecodeCmd();
			cmdBufPos = 0;	// clear buffer
		}
		uartRxFlag = false;  // clear the flag - the 'receive character' event has been handled.
		HAL_UART_Receive_IT(&huart1, (uint8_t*)&uartRxChar, 1);	// UART interrupt after 1 character was received
	}
	if(systickFlag == 1U)
	{
		systickFlag = 0U;

		writeToPins(segements, pinsValue, 4);
	}
}

void DecodeCmd()
{
	uint8_t charsL;

	switch (cmdBuf[1])
	{
	case 'A' :
		HAL_UART_Transmit(&huart1, (uint8_t*)txStudentNo, 13, 1000);
		break;

	case 'F':
		String2Int(cmdBuf+3, &tempSetpoint);

		txBuf[0] = '$';
		txBuf[1] = 'F';
		txBuf[2] = '\r';
		txBuf[3] = '\n';

		HAL_UART_Transmit(&huart1, (uint8_t*)txBuf, 4, 1000);
		break;

	case 'G':
		txBuf[0] = '$';
		txBuf[1] = 'G';
		txBuf[2] = ',';
		charsL = Int2String(txBuf+3, tempSetpoint, 4);
		txBuf[3 + charsL] = '\r';
		txBuf[4 + charsL] = '\n';

		HAL_UART_Transmit(&huart1, (uint8_t*)txBuf, charsL+5, 1000);
		break;

	case 'R':
		resetAll();

		break;
	}
}

uint8_t String2Int(char* inputString, int16_t* outputInt)
{
	int returnValue = 0;
	int sign = 1;

	if (*inputString == '\0')
		return 0;

	if (*inputString == '-')
	{
		sign = -1;
		inputString++;
	}

	while ((*inputString >= '0') && (*inputString <= '9'))
	{
		returnValue *= 10;
		returnValue += (*inputString - 48);

		if (((sign == 1) && (returnValue >= 32768)) ||
				((sign == -1) && (returnValue >= 32769)))
			return 0;

		inputString++;
	}
	*outputInt = (int16_t)(sign * returnValue);
	return 1;
}

// convert integer var to ASCII string
uint8_t Int2String(char* outputString, int16_t value, uint8_t maxL)
{
	int numWritten = 0;
	int writePosition = 0;
	uint8_t digits = 0;

	if (maxL == 0)
		return 0;

	if (value < 0)
	{
		outputString[0] = '-';
		outputString++;
		maxL--;
		value = -value;
		numWritten = 1;
	}

	if (value < 10)
		digits = 1;
	else if (value < 100)
		digits = 2;
	else if (value < 1000)
		digits = 3;
	else if (value < 10000)
		digits = 4;
	else
		digits = 5;

	if (digits > maxL)
		return 0; // error - not enough space in output string!

	writePosition = digits;
	while (writePosition > 0)
	{
		outputString[writePosition-1] = (char) ((value % 10) + 48);
		value /= 10;
		writePosition--;
		numWritten++;
	}

	return numWritten;
}

void resetAll(void)
{
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_5,GPIO_PIN_SET); //a
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_6,GPIO_PIN_SET); //b
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_7,GPIO_PIN_SET); //c
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_6,GPIO_PIN_SET); //d
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_7,GPIO_PIN_SET); //e
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_8,GPIO_PIN_SET); //f
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_9,GPIO_PIN_SET); //g
}

void writeToPins(uint8_t segments[], uint8_t pins[], int segmentsL)
{
	int i = 0;

	while (i <= segmentsL)
	{
		//HAL_Delay(1);
		//Use 1ms flag as HAL delay adjust timing of board and makes it either quicker or slower on eachcycle

		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_10,(~(segements[i] >> 0) & 0b00000001)); //1
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_4,(~(segements[i] >> 1) & 0b00000001)); //2
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_5,(~(segements[i] >> 2) & 0b00000001)); //3
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_3,(~(segements[i] >> 3) & 0b00000001)); //4

		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_5, (~(pins[i] >> 0) & 0b00000001)); //a
		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_6, (~(pins[i] >> 1) & 0b00000001)); //b
		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_7, (~(pins[i] >> 2) & 0b00000001)); //c
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_6, (~(pins[i] >> 3) & 0b00000001)); //d
		HAL_GPIO_WritePin(GPIOC,GPIO_PIN_7, (~(pins[i] >> 4) & 0b00000001)); //e
		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_8, (~(pins[i] >> 5) & 0b00000001)); //f
		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_9, (~(pins[i] >> 6) & 0b00000001)); //g

		i++;

		if(i == 4)
			i = 0;
	}
}

//----------------------------------------------------------------------------------------------//
//											Interupts											//
//----------------------------------------------------------------------------------------------//

// HAL_UART_RxCpltCallback - callback function that will be called from the UART interrupt handler.
// This function will execute whenever a character is received from the UART
void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart)
{
	// the interrupt handler will automatically put the received character in the uartRXChar variable (no need to write any code for that).
	// so all we do it set flag to indicate character was received, and then process the received character further in the main loop
	uartRxFlag = true;
}

void HAL_SYSTICK_Callback(void)
{
	systickFlag = true;
}