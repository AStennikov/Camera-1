#include "stm32f4xx.h"
#include "stm32f4xx_i2c.h"
#include "stm32f4xx_rcc.h"

#include "gpio.h"

//initializes i2c to comodule pins
void i2c_init(){
	//PA9 I2C1_SCL
	//PA10 I2C1_SDA

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_I2C1);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_I2C1);
	GPIO_Init(GPIOB, &GPIO_InitStruct);

	I2C_InitTypeDef I2C_InitStruct;
	I2C_InitStruct.I2C_DutyCycle = I2C_DutyCycle_16_9;//I2C_DutyCycle_2;
	I2C_InitStruct.I2C_ClockSpeed = 10000;			//10kHz
	I2C_InitStruct.I2C_Mode = I2C_Mode_I2C;
	I2C_InitStruct.I2C_OwnAddress1 = 0x00;
	I2C_InitStruct.I2C_Ack = I2C_Ack_Enable;
	I2C_InitStruct.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	I2C_Init(I2C1, &I2C_InitStruct);

	I2C_Cmd(I2C1, ENABLE);

}

//reads chip version
int sensor_get(int reg){
	//based on https://github.com/devthrash/STM32F4-examples/blob/master/I2C%20Master/main.c

	int returned_value = 0;

	// wait until I2C1 is not busy anymore
	while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY));

	// Send I2C1 START condition
	I2C_GenerateSTART(I2C1, ENABLE);

	// wait for I2C1 EV5 --> Slave has acknowledged start condition
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));

	// Send slave Address for write
	I2C_Send7bitAddress(I2C1, 0xBA, 0);

	// wait for slave address to be sent
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_SLAVE_RECEIVER_ADDRESS_MATCHED));

	//I2C_SendData(I2C1, reg>>8);
	// wait for I2C1 EV8_2 --> byte has been transmitted
	//while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

	I2C_SendData(I2C1, reg>>0);
	// wait for I2C1 EV8_2 --> byte has been transmitted
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

	// Send I2C1 START condition
	I2C_GenerateSTART(I2C1, ENABLE);

	// wait for I2C1 EV5 --> Slave has acknowledged start condition
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));

	// Send slave Address for read
	I2C_Send7bitAddress(I2C1, 0xBA, 1);

	I2C_AcknowledgeConfig(I2C1, ENABLE);

	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));

	//gpio_set(GPIO_LED_MCU, 1);

	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED));

	returned_value |= (int) I2C_ReceiveData(I2C1)<<8;
	//gpio_set(GPIO_LED_MCU, 1);

	I2C_AcknowledgeConfig(I2C1, DISABLE);

	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED));
	returned_value |= (int) I2C_ReceiveData(I2C1)<<0;

	//gpio_set(GPIO_LED_MCU, 1);

	// Send I2C1 STOP Condition
	I2C_GenerateSTOP(I2C1, ENABLE);

	return returned_value;
}

//writes register
void sensor_set(int reg, int data){
	// wait until I2C1 is not busy anymore
	while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY));

	// Send I2C1 START condition
	I2C_GenerateSTART(I2C1, ENABLE);

	// wait for I2C1 EV5 --> Slave has acknowledged start condition
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));

	// Send slave Address for write
	I2C_Send7bitAddress(I2C1, 0xBA, 0);

	// wait for slave address to be sent
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_SLAVE_RECEIVER_ADDRESS_MATCHED));

	I2C_SendData(I2C1, reg);
	// wait for I2C1 EV8_2 --> byte has been transmitted
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

	I2C_SendData(I2C1, data>>8);
	// wait for I2C1 EV8_2 --> byte has been transmitted
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

	I2C_SendData(I2C1, data);
	// wait for I2C1 EV8_2 --> byte has been transmitted
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

	// Send I2C1 STOP Condition
	I2C_GenerateSTOP(I2C1, ENABLE);
}


//gets state of charge from fuel gauge
/*int fuel_gauge_get(int fg_cmd){
	int result = 0;	//returned value

	//write command
	I2C_TransferHandling(I2C1, 0xAA, 2, I2C_SoftEnd_Mode, I2C_Generate_Start_Write);
	while(I2C_GetFlagStatus(I2C1, I2C_FLAG_TXIS) == RESET);
	I2C_SendData(I2C1, fg_cmd>>8);
	while(I2C_GetFlagStatus(I2C1, I2C_FLAG_TXIS) == RESET);
	I2C_SendData(I2C1, fg_cmd>>0);
	while(I2C_GetFlagStatus(I2C1, I2C_FLAG_TC) == RESET);

	//read data
	I2C_TransferHandling(I2C1, 0xAB, 2, I2C_SoftEnd_Mode, I2C_Generate_Start_Read);
	while(I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) == RESET);
	result = (int) I2C_ReceiveData(I2C1)<<8;
	while(I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) == RESET);
	result += (int) I2C_ReceiveData(I2C1);

	I2C_GenerateSTOP(I2C1, ENABLE);
	while(I2C_GetFlagStatus(I2C1, I2C_FLAG_STOPF) == RESET);
	I2C_ClearFlag(I2C1, I2C_FLAG_STOPF);

	return result;
}*/


