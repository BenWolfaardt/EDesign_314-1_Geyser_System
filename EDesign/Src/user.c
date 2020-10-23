/*
 * user.c
 *
 * Parts of code taken from Lecturers code on SunLearn from first demo
 *
 *  Created on: 05 Mar 2018
 *      Author: 18321933
 */

#include "user.h"
#include "math.h"

#define cmdBufL 50   		// maximum length of a command string received on the UART
#define maxTxL  50  		// maximum length of transmit buffer (replies sent back to UART)

bool displayDelay2ms = 0;

char cmdBuf[cmdBufL];  		// buffer in which to store commands that are received from the UART
char uartRxChar;			// temporary storage
char txBuf[maxTxL]; 		// buffer for replies that are to be sent out on UART
char* txStudentNo = "$A,18321933\r\n";

extern ADC_ChannelConfTypeDef adcChannel12;
extern ADC_ChannelConfTypeDef adcChannel13;
extern ADC_HandleTypeDef hadc1;

int i = 0;
int j = 0;
uint8_t my_counter;
int16_t tempSetpoint;		// the current temperature set point

uint8_t numberMap[10];
uint8_t pinsValue[4];
uint8_t segements[4];

uint16_t cmdBufPos;  		// this is the position in the cmdB where we are currently writing to

uint32_t adc12;
uint32_t adc13;
uint32_t adcBuf12;
uint32_t adcBuf13;
uint32_t measuredRMS12;
uint32_t iRMS12;
uint32_t vRMS12;
uint32_t measuredRMS13;
uint32_t iRMS13;
uint32_t vRMS13;

volatile bool uartRxFlag;	// use 'volatile' keyword because the variable is changed from interrupt handler
volatile bool systickFlag;



uint8_t in = 0;



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
	pinsValue[1] = numberMap[8];
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

		writeToPins(segements, pinsValue, 3);
	}
}

void DecodeCmd()
{	//uint32_t temp;
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

	case 'K':
			//display IRMS

//		vRMS12 = measuredRMS12*79.18793901;
//
//		measuredRMS13 = 3.3/((4096-1)*sqrt(20))*adcBuf13;
//		iRMS13 = measuredRMS13*4.679287305;
//		vRMS13 = measuredRMS13*79.18793901;

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
	//nt i = 0;

	//while (i <= segmentsL)
	//{
		//HAL_Delay(1);
		//		if (displayDelay2ms == 1)
		//		{
		//			displayDelay2ms = 0;

		//Use 1ms flag as HAL delay adjust timing of board and makes it either quicker or slower on eachcycle



		//if (displayDelay2ms == 1)
		//{
			//displayDelay2ms = 0;

			if(in == segmentsL)
			{
				in = 0;
			}
			else
			{
				in++;
			}

	switch(in)
	{
	case 1 :
	{
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_10,(~(segements[0] >> 0) & 0b00000001)); //1
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_4,(~(segements[0] >> 1) & 0b00000001)); //2
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_5,(~(segements[0] >> 2) & 0b00000001)); //3
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_3,(~(segements[0] >> 3) & 0b00000001)); //4

	}
	break;
	case 2:
	{
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_10,(~(segements[1] >> 0) & 0b00000001)); //1
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_4,(~(segements[1] >> 1) & 0b00000001)); //2
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_5,(~(segements[1] >> 2) & 0b00000001)); //3
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_3,(~(segements[1] >> 3) & 0b00000001)); //4
	}
	break;
	case 3:
	{
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_10,(~(segements[2] >> 0) & 0b00000001)); //1
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_4,(~(segements[2] >> 1) & 0b00000001)); //2
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_5,(~(segements[2] >> 2) & 0b00000001)); //3
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_3,(~(segements[2] >> 3) & 0b00000001)); //4
	}
	break;
	case 4:
	{
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_10,(~(segements[3] >> 0) & 0b00000001)); //1
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_4,(~(segements[3] >> 1) & 0b00000001)); //2
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_5,(~(segements[3] >> 2) & 0b00000001)); //3
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_3,(~(segements[3] >> 3) & 0b00000001)); //4
	}
	break;
	}

//			HAL_GPIO_WritePin(GPIOB,GPIO_PIN_10,(~(segements[i] >> 0) & 0b00000001)); //1
//			HAL_GPIO_WritePin(GPIOB,GPIO_PIN_4,(~(segements[i] >> 1) & 0b00000001)); //2
//			HAL_GPIO_WritePin(GPIOB,GPIO_PIN_5,(~(segements[i] >> 2) & 0b00000001)); //3
//			HAL_GPIO_WritePin(GPIOB,GPIO_PIN_3,(~(segements[i] >> 3) & 0b00000001)); //4
//
	i = (int)(in - 1);
			HAL_GPIO_WritePin(GPIOA,GPIO_PIN_5, (~(pins[i] >> 0) & 0b00000001)); //a
			HAL_GPIO_WritePin(GPIOA,GPIO_PIN_6, (~(pins[i] >> 1) & 0b00000001)); //b
			HAL_GPIO_WritePin(GPIOA,GPIO_PIN_7, (~(pins[i] >> 2) & 0b00000001)); //c
			HAL_GPIO_WritePin(GPIOB,GPIO_PIN_6, (~(pins[i] >> 3) & 0b00000001)); //d
			HAL_GPIO_WritePin(GPIOC,GPIO_PIN_7, (~(pins[i] >> 4) & 0b00000001)); //e
			HAL_GPIO_WritePin(GPIOA,GPIO_PIN_8, (~(pins[i] >> 5) & 0b00000001)); //f
			HAL_GPIO_WritePin(GPIOA,GPIO_PIN_9, (~(pins[i] >> 6) & 0b00000001)); //g
		//}
		//i++;

		//if(i == segmentsL)
		//	i = 0;
	//}
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
	uint32_t temp;
	j++;
	if (j == 1)
	{
		j = 0;
		displayDelay2ms = 1;
	}

		if (HAL_ADC_ConfigChannel(&hadc1, &adcChannel12) != HAL_OK)
		{
			_Error_Handler(__FILE__, __LINE__);
		}
		HAL_ADC_Start(&hadc1);
		HAL_ADC_PollForConversion(&hadc1,1000);
		HAL_ADC_Stop(&hadc1);
		adc12 = HAL_ADC_GetValue(&hadc1);


		if (HAL_ADC_ConfigChannel(&hadc1, &adcChannel13) != HAL_OK)
		{
			_Error_Handler(__FILE__, __LINE__);
		}
		HAL_ADC_Start(&hadc1);
		HAL_ADC_PollForConversion(&hadc1,1000);
		HAL_ADC_Stop(&hadc1);
		adc13 = HAL_ADC_GetValue(&hadc1);
		adc13 *= adc13;
		adcBuf13 += adc13;

		if (my_counter == 20)
		{
			adc12 *= adc12;
			adcBuf12 += adc12;
			temp = adcBuf12;
			temp/=20;
			temp = sqrt(temp);
			temp*=3400;
			temp/=4095;
			measuredRMS12 = temp;
			iRMS12 = measuredRMS12*4.679287305;
			my_counter = 0;
		}
		else
		{
			adc12 *= adc12;
			adcBuf12 += adc12;
			my_counter++;
		}
}
