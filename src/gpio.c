#ifndef GPIO_C_
#define GPIO_C_

#include "stm32f4xx.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "gpio.h"


//MUST be in same order as gpio_name_list
gpio_params_t gpio_config_list[GPIO_COUNT] = {
		{GPIOH, RCC_AHB1Periph_GPIOH, GPIO_Pin_4, GPIO_Mode_OUT, GPIO_Speed_2MHz, GPIO_OType_PP, GPIO_PuPd_NOPULL},	//GPIO_LED_MCU
		{GPIOA, RCC_AHB1Periph_GPIOA, GPIO_Pin_7, GPIO_Mode_OUT, GPIO_Speed_2MHz, GPIO_OType_PP, GPIO_PuPd_NOPULL},	//GPIO_LED_UART
		{GPIOH, RCC_AHB1Periph_GPIOH, GPIO_Pin_5, GPIO_Mode_OUT, GPIO_Speed_2MHz, GPIO_OType_PP, GPIO_PuPd_NOPULL},	//GPIO_LED_BUSY

		{GPIOB, RCC_AHB1Periph_GPIOB, GPIO_Pin_15, GPIO_Mode_OUT, GPIO_Speed_2MHz, GPIO_OType_OD, GPIO_PuPd_NOPULL},	//GPIO_TRIGGER
		{GPIOB, RCC_AHB1Periph_GPIOB, GPIO_Pin_13, GPIO_Mode_IN, GPIO_Speed_2MHz, GPIO_OType_PP, GPIO_PuPd_UP},	//GPIO_SNAPSHOT
};

void gpio_init(){
	int i=0;

	GPIO_InitTypeDef GPIO_InitStruct;

	for (i=0; i<GPIO_COUNT; i++){
		//enable clocks
		RCC_AHB1PeriphClockCmd(gpio_config_list[i].RCC_AHBPeriph, ENABLE);

		//configure pins
		GPIO_InitStruct.GPIO_Mode = gpio_config_list[i].GPIO_Mode;
		GPIO_InitStruct.GPIO_OType = gpio_config_list[i].GPIO_OType;
		GPIO_InitStruct.GPIO_Pin = gpio_config_list[i].GPIO_Pin;
		GPIO_InitStruct.GPIO_PuPd = gpio_config_list[i].GPIO_PuPd;
		GPIO_InitStruct.GPIO_Speed = gpio_config_list[i].GPIO_Speed;
		GPIO_Init(gpio_config_list[i].GPIOx, &GPIO_InitStruct);
	}
}



void gpio_set(gpio_name_list name, uint16_t value){
	GPIO_WriteBit(gpio_config_list[name].GPIOx, gpio_config_list[name].GPIO_Pin, value);
}

void gpio_toggle(gpio_name_list name){
	if (gpio_read(name) == 1) {
		GPIO_WriteBit(gpio_config_list[name].GPIOx, gpio_config_list[name].GPIO_Pin, 0);
	} else {
		GPIO_WriteBit(gpio_config_list[name].GPIOx, gpio_config_list[name].GPIO_Pin, 1);
	}

}

uint16_t gpio_read(gpio_name_list name){
	return GPIO_ReadInputDataBit(gpio_config_list[name].GPIOx, gpio_config_list[name].GPIO_Pin);
}

#endif
