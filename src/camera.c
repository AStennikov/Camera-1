#include "stm32f4xx.h"
#include "stm32f4xx_dcmi.h"
#include "stm32f4xx_rcc.h"

#include "gpio.h"
#include "uart.h"
#include "camera.h"

#include <stdio.h>
#include <string.h>

//points to uart, so that we can access it in this file functions
UART *dbg;

char buf[256];

//int image[2500];

//these statistics are printed out at the end of frame reception
int overflow_count = 0, sync_error_count = 0, fv_count = 0, lv_count = 0;

//initializes pins, clocks and controller
void camera_init(UART *debug){

	dbg = debug;

	memset(image, 0, 5000*sizeof(int));

	//enable clocks
	RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_DCMI, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	//RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOI, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOH, ENABLE);

	//configure pins to alternate functions
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource4, GPIO_AF_DCMI);		//hsync
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_DCMI);		//pixclk
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource8, GPIO_AF_DCMI);		//d6
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource9, GPIO_AF_DCMI);		//d7
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource8, GPIO_AF_DCMI);		//d2
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource11, GPIO_AF_DCMI);	//d4
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource3, GPIO_AF_DCMI);		//d5
	//GPIO_PinAFConfig(GPIOG, GPIO_PinSource9, GPIO_AF_DCMI);		//vsync
	GPIO_PinAFConfig(GPIOI, GPIO_PinSource5, GPIO_AF_DCMI);		//vsync
	GPIO_PinAFConfig(GPIOI, GPIO_PinSource3, GPIO_AF_DCMI);		//d10
	GPIO_PinAFConfig(GPIOH, GPIO_PinSource6, GPIO_AF_DCMI);		//d8
	GPIO_PinAFConfig(GPIOH, GPIO_PinSource7, GPIO_AF_DCMI);		//d9
	GPIO_PinAFConfig(GPIOH, GPIO_PinSource9, GPIO_AF_DCMI);		//d0
	GPIO_PinAFConfig(GPIOH, GPIO_PinSource10, GPIO_AF_DCMI);	//d1
	GPIO_PinAFConfig(GPIOH, GPIO_PinSource12, GPIO_AF_DCMI);	//d3
	GPIO_PinAFConfig(GPIOH, GPIO_PinSource15, GPIO_AF_DCMI);	//d11

	//configure pins
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_6;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStruct.GPIO_OType = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStruct);
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_Init(GPIOB, &GPIO_InitStruct);
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_11;
	GPIO_Init(GPIOC, &GPIO_InitStruct);
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_3;
	GPIO_Init(GPIOD, &GPIO_InitStruct);
	//GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9;	//i5 instead
	//GPIO_Init(GPIOG, &GPIO_InitStruct);
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_3 |GPIO_Pin_5;
	GPIO_Init(GPIOI, &GPIO_InitStruct);
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_12 | GPIO_Pin_15;
	GPIO_Init(GPIOH, &GPIO_InitStruct);

	//init controller
	DCMI_InitTypeDef dcmi_init;
	dcmi_init.DCMI_CaptureMode = DCMI_CaptureMode_Continuous;
	dcmi_init.DCMI_CaptureRate= DCMI_CaptureRate_All_Frame;
	dcmi_init.DCMI_ExtendedDataMode = DCMI_ExtendedDataMode_12b;
	dcmi_init.DCMI_HSPolarity = DCMI_HSPolarity_Low;
	dcmi_init.DCMI_PCKPolarity = DCMI_PCKPolarity_Falling;
	dcmi_init.DCMI_SynchroMode = DCMI_SynchroMode_Hardware;
	dcmi_init.DCMI_VSPolarity = DCMI_VSPolarity_Low;
	DCMI_Init(&dcmi_init);

	/*//dma is next
	//dma2, stream 1, channel 1
	RCC_AHB1PeriphResetCmd(RCC_AHB1Periph_DMA2, ENABLE);
	DMA_InitTypeDef dma_init;
	DMA_StructInit(&dma_init);
	dma_init.DMA_BufferSize = 2500;	//not sure
	dma_init.DMA_Channel = DMA_Channel_1;
	dma_init.DMA_DIR = DMA_DIR_PeripheralToMemory;
	dma_init.DMA_FIFOMode = DMA_FIFOMode_Enable;//DMA_FIFOMode_Disable;
	dma_init.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
	dma_init.DMA_Memory0BaseAddr = (int32_t) image;
	dma_init.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	dma_init.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
	dma_init.DMA_MemoryInc = DMA_MemoryInc_Enable;
	dma_init.DMA_Mode = DMA_Mode_Circular;
	dma_init.DMA_PeripheralBaseAddr = (uint32_t) &DCMI->DR;
	dma_init.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	dma_init.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
	dma_init.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	dma_init.DMA_Priority = DMA_Priority_High;
	DMA_Init(DMA2_Stream1, &dma_init);
	DMA_Cmd(DMA2_Stream1, ENABLE);*/


	//enable interrupts
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannel = DCMI_IRQn;
	NVIC_Init(&NVIC_InitStructure);
	NVIC_EnableIRQ(DCMI_IRQn);
	DCMI_ITConfig(DCMI_IT_FRAME, ENABLE);
	DCMI_ITConfig(DCMI_IT_OVF, ENABLE);
	DCMI_ITConfig(DCMI_IT_ERR, ENABLE);
	DCMI_ITConfig(DCMI_IT_LINE, ENABLE);
	DCMI_ITConfig(DCMI_IT_VSYNC, ENABLE);

	/*NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream1_IRQn;
	NVIC_Init(&NVIC_InitStructure);
	NVIC_EnableIRQ(DMA2_Stream1_IRQn);
	DMA_ITConfig(DMA2_Stream1, DMA_IT_TC, ENABLE);
	DMA_ITConfig(DMA2_Stream1, DMA_IT_HT, ENABLE);
	DMA_ITConfig(DMA2_Stream1, DMA_IT_TE, ENABLE);
	DMA_ITConfig(DMA2_Stream1, DMA_IT_FE, ENABLE);*/

	//Enable DCMI interface
	DCMI_Cmd(ENABLE);

	//start frame capture
	DCMI_CaptureCmd(ENABLE);


	uart_putline(dbg, "DCMI init finished");
}

//tells dcmi controller to start frame capture
void camera_capture_frame(){
	DCMI_CaptureCmd(ENABLE);
}



//returns 1 if image array has at least 1 non-0 element;
int camera_test_image_array(){
	int i;
	for (i=0; i<5000; ++i){
		if (image[i] != 0) {
			return 1;
		}
	}
	return 0;
}

void DCMI_IRQHandler() {
	if (DCMI_GetITStatus(DCMI_IT_FRAME) == SET) {
		//sprintf(buf, "Frame captured. OVF: %d, SYNC: %d, Frames: %d, Lines: %d", overflow_count, sync_error_count, fv_count, lv_count);
		//overflow_count = 0;
		//sync_error_count = 0;
		//fv_count = 0;
		//lv_count = 0;
		//uart_putline(dbg, buf);
		DCMI_ClearITPendingBit(DCMI_IT_FRAME);
	}
	if (DCMI_GetITStatus(DCMI_IT_OVF) == SET) {
		++overflow_count;
		//uart_putline(dbg, "DCMI overflow error");
		DCMI_ClearITPendingBit(DCMI_IT_OVF);
	}
	if (DCMI_GetITStatus(DCMI_IT_ERR) == SET) {
		++sync_error_count;
		//uart_putline(dbg, "DCMI sync error");
		DCMI_ClearITPendingBit(DCMI_IT_ERR);
	}
	if (DCMI_GetITStatus(DCMI_IT_LINE) == SET) {
		++lv_count;
		//uart_putline(dbg, "DCMI line");
		DCMI_ClearITPendingBit(DCMI_IT_LINE);
	}
	if (DCMI_GetITStatus(DCMI_IT_VSYNC) == SET) {
		++fv_count;

		sprintf(buf, "Frame captured. OVF: %d, SYNC: %d, Frames: %d, Lines: %d", overflow_count, sync_error_count, fv_count, lv_count);
		overflow_count = 0;
		sync_error_count = 0;
		fv_count = 0;
		lv_count = 0;
		uart_putline(dbg, buf);

		//uart_putline(dbg, "DCMI vsync");
		DCMI_ClearITPendingBit(DCMI_IT_VSYNC);
	}
}

void DMA2_Stream1_IRQHandler() {
	if (DMA_GetITStatus(DMA2_Stream1, DMA_IT_TC)) {

		uart_putline(dbg, "DMA IT TC");
		DMA_ClearITPendingBit(DMA2_Stream1, DMA_IT_TC);
	}
	if (DMA_GetITStatus(DMA2_Stream1, DMA_IT_HT)) {

		uart_putline(dbg, "DMA_IT_HT");
		DMA_ClearITPendingBit(DMA2_Stream1, DMA_IT_HT);
	}
	if (DMA_GetITStatus(DMA2_Stream1, DMA_IT_TE)) {

		uart_putline(dbg, "DMA_IT_TE");
		DMA_ClearITPendingBit(DMA2_Stream1, DMA_IT_TE);
	}
	if (DMA_GetITStatus(DMA2_Stream1, DMA_IT_FE)) {

		uart_putline(dbg, "DMA_IT_FE");
		DMA_ClearITPendingBit(DMA2_Stream1, DMA_IT_FE);
	}
}


