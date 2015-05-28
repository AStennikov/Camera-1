/*
 * clock.c
 *
 *  Created on: Apr 10, 2015
 *      Author: Aleksandr
 */

#include "clock.h"
#include "stm32f4xx.h"
#include "stm32f4xx_tim.h"
#include "stm32f4xx_rcc.h"

//initializes timer that will generate image sensor clock. TIM9, PE5
void clock_init() {

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM9, ENABLE);
	TIM_TimeBaseInitTypeDef timebase;
	timebase.TIM_ClockDivision = TIM_CKD_DIV1;
	timebase.TIM_CounterMode = TIM_CounterMode_Up;
	timebase.TIM_Period = 90;//15;
	timebase.TIM_Prescaler = 1;
	timebase.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM9, &timebase);
	TIM_Cmd(TIM9, ENABLE);

	TIM_OCInitTypeDef oc;
	oc.TIM_OCIdleState = TIM_OCIdleState_Reset;
	oc.TIM_OCMode = TIM_OCMode_PWM2;
	oc.TIM_OCPolarity = TIM_OCPolarity_Low;
	oc.TIM_OutputState = TIM_OutputState_Enable;
	oc.TIM_Pulse = 45;//8;
	TIM_OC1Init(TIM9, &oc);
	TIM_OC1PreloadConfig(TIM9, TIM_OCPreload_Enable);

	GPIO_InitTypeDef GPIO_InitStruct;
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource5, GPIO_AF_TIM9);
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_Init(GPIOE, &GPIO_InitStruct);

}


