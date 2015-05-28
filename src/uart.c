#include "uart.h"
#include "gpio.h"
#include <stdlib.h>
#include "core_cm4.h"
//#include "stm32f0xx.h"
#include "stm32f4xx_usart.h"
#include <string.h>

#include "string_queue.h"


//this array stores addresses of UART variables that user declared. For UART 1-5. It is required because when interrupt is called, it does not know where to put/get data from.
static UART *peripheral_address[5];



void UART_set_state(UART *peripheral, UART_STATE new_state);
void UART_IRQ_process(UART *peripheral);   //since interrupts for all peripherals are similar, save space by calling this function inside each interrupt.
void UART_enqueue(UART_queue *mailbox, char data);
char UART_dequeue(UART_queue *mailbox);



void UART_init(UART *new_peripheral, UART_GPIO gpio, int baud_rate, int inbox_size, int outbox_size){

    //at first initialize peripheral
    UART_set_state(new_peripheral, UART_STATE_ERROR_GENERAL);
    //new_peripheral->inbox.first = 0;
    //new_peripheral->inbox.last = 0;
    //new_peripheral->inbox.length = 0;
    new_peripheral->outbox.first = 0;
    new_peripheral->outbox.last = 0;
    new_peripheral->outbox.length = 0;
    new_peripheral->outbox_busy = 0;

    //new_peripheral->inbox.max_length = inbox_size;
    //new_peripheral->inbox.data = (char*) malloc(inbox_size*sizeof(char));
    new_peripheral->outbox.max_length = outbox_size;
    new_peripheral->outbox.data = (char*) malloc(outbox_size*sizeof(char));

    //inbox initialization
    /*new_peripheral->buffer_length = 0;
    int k;
    for (k=0; k<MAX_INPUT_STRING_LENGTH; k++) {
    	new_peripheral->buffer[k] = 0;
    }
    string_queue_init(&new_peripheral->inbox);*/

    new_peripheral->inbox.first = 0;
    new_peripheral->inbox.last = 0;
    new_peripheral->inbox.length = 0;
    new_peripheral->inbox.max_length = inbox_size;
    new_peripheral->inbox.data = (char*) malloc(inbox_size*sizeof(char));
    new_peripheral->inbox_last_string_length = 0;
    new_peripheral->inbox_string_count = 0;

    //now initialize gpios, uart and interrupts
    GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;

	USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = baud_rate;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;

	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

    //STM32F4
    switch (gpio) {
    	case USART1_PA9_PA10:
    		peripheral_address[0] = new_peripheral;
    		new_peripheral->USARTx = USART1;

    		//GPIO
    		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    		GPIO_Init(GPIOA, &GPIO_InitStructure);
    		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    		GPIO_Init(GPIOA, &GPIO_InitStructure);
    		GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);
    		GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);

    		//UART
    		RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    		USART_Init(USART1, &USART_InitStructure);

    		//Interrupts
    		NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    		NVIC_Init(&NVIC_InitStructure);
    		NVIC_EnableIRQ(USART1_IRQn);
    		//USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
    		USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    		//enable usart
    		USART_Cmd(USART1, ENABLE);
    		break;
    }
	
	//STM32F0
    /*switch (gpio) {
        case USART1_PA9_PA10:
        	peripheral_address[0] = new_peripheral;
        	new_peripheral->USARTx = USART1;

        	//GPIO
        	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
        	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
        	GPIO_Init(GPIOA, &GPIO_InitStructure);
        	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
        	GPIO_Init(GPIOA, &GPIO_InitStructure);
        	GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_1);
        	GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_1);

        	//UART
        	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
        	USART_Init(USART1, &USART_InitStructure);

        	//Interrupts
        	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
        	NVIC_Init(&NVIC_InitStructure);
        	NVIC_EnableIRQ(USART1_IRQn);
        	//USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
        	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

        	//enable usart
        	USART_Cmd(USART1, ENABLE);
        	break;
        case USART2_PA2_PA3:
        	peripheral_address[1] = new_peripheral;
        	new_peripheral->USARTx = USART2;

        	//GPIO
        	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
        	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
        	GPIO_Init(GPIOA, &GPIO_InitStructure);
        	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
        	GPIO_Init(GPIOA, &GPIO_InitStructure);
        	GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_1);
        	GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_1);

        	//UART
        	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
        	USART_Init(USART2, &USART_InitStructure);

        	//Interrupts
        	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
        	NVIC_Init(&NVIC_InitStructure);
        	NVIC_EnableIRQ(USART2_IRQn);
        	//USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
        	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

        	//enable usart
        	USART_Cmd(USART2, ENABLE);
        	break;
    }*/

    //STM32F3
    /*switch (gpio) {
        case USART2_PA2_PA3:    //tested
            peripheral_address[1] = new_peripheral;
            new_peripheral->USARTx = USART2;

            //GPIO
            RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
            GPIO_Init(GPIOA, &GPIO_InitStructure);
            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
            GPIO_Init(GPIOA, &GPIO_InitStructure);
            GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_7);
            GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_7);

            //UART
            RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
            USART_Init(USART2, &USART_InitStructure);

            //Interrupts
            NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
            NVIC_Init(&NVIC_InitStructure);
            NVIC_EnableIRQ(USART2_IRQn);
            //USART_ITConfig(USART2, USART_IT_TXE, ENABLE); //please note that transmit interrupt flag can be cleared only by disabling interrupt itself. Because of that we enable interrupt when data transmission starts and disable it  when data transmission ends
            USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);  //receive data register not empty

            //enable usart
            USART_Cmd(USART2, ENABLE);
            break;

        case USART1_PA9_PA10:   //tested
            peripheral_address[0] = new_peripheral;
            new_peripheral->USARTx = USART1;

            //GPIO
            RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
            GPIO_Init(GPIOA, &GPIO_InitStructure);
            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
            GPIO_Init(GPIOA, &GPIO_InitStructure);
            GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_7);
            GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_7);

            //UART
            RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
            USART_Init(USART1, &USART_InitStructure);

            //Interrupts
            NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
            NVIC_Init(&NVIC_InitStructure);
            NVIC_EnableIRQ(USART1_IRQn);
            //USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
            USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

            //enable usart
            USART_Cmd(USART1, ENABLE);
            break;

        case USART2_PA14_PA15:
            peripheral_address[1] = new_peripheral;
            new_peripheral->USARTx = USART2;

            //GPIO
            RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
            GPIO_Init(GPIOA, &GPIO_InitStructure);
            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
            GPIO_Init(GPIOA, &GPIO_InitStructure);
            GPIO_PinAFConfig(GPIOA, GPIO_PinSource14, GPIO_AF_7);
            GPIO_PinAFConfig(GPIOA, GPIO_PinSource15, GPIO_AF_7);

            //UART
            RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
            USART_Init(USART2, &USART_InitStructure);

            //Interrupts
            NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
            NVIC_Init(&NVIC_InitStructure);
            NVIC_EnableIRQ(USART2_IRQn);
            //USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
            USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

            //enable usart
            USART_Cmd(USART2, ENABLE);
            break;

        case USART2_PB3_PB4:
            peripheral_address[1] = new_peripheral;
            new_peripheral->USARTx = USART2;

            //GPIO
            RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
            GPIO_Init(GPIOB, &GPIO_InitStructure);
            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
            GPIO_Init(GPIOB, &GPIO_InitStructure);
            GPIO_PinAFConfig(GPIOB, GPIO_PinSource3, GPIO_AF_7);
            GPIO_PinAFConfig(GPIOB, GPIO_PinSource4, GPIO_AF_7);

            //UART
            RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
            USART_Init(USART2, &USART_InitStructure);

            //Interrupts
            NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
            NVIC_Init(&NVIC_InitStructure);
            NVIC_EnableIRQ(USART2_IRQn);
            //USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
            USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

            //enable usart
            USART_Cmd(USART2, ENABLE);
            break;

        case USART1_PB6_PB7:
            peripheral_address[0] = new_peripheral;
            new_peripheral->USARTx = USART1;

            //GPIO
            RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
            GPIO_Init(GPIOB, &GPIO_InitStructure);
            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
            GPIO_Init(GPIOB, &GPIO_InitStructure);
            GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_7);
            GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_7);

            //UART
            RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
            USART_Init(USART1, &USART_InitStructure);

            //Interrupts
            NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
            NVIC_Init(&NVIC_InitStructure);
            NVIC_EnableIRQ(USART1_IRQn);
            //USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
            USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

            //enable usart
            USART_Cmd(USART1, ENABLE);
            break;

        case USART3_PB10_PB11:
            peripheral_address[2] = new_peripheral;
            new_peripheral->USARTx = USART3;

            //GPIO
            RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
            GPIO_Init(GPIOB, &GPIO_InitStructure);
            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
            GPIO_Init(GPIOB, &GPIO_InitStructure);
            GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_7);
            GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_7);

            //UART
            RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
            USART_Init(USART3, &USART_InitStructure);

            //Interrupts
            NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
            NVIC_Init(&NVIC_InitStructure);
            NVIC_EnableIRQ(USART3_IRQn);
            //USART_ITConfig(USART3, USART_IT_TXE, ENABLE);
            USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);

            //enable usart
            USART_Cmd(USART3, ENABLE);
            break;

        case USART1_PC4_PC5:
            peripheral_address[0] = new_peripheral;
            new_peripheral->USARTx = USART1;

            //GPIO
            RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);
            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
            GPIO_Init(GPIOC, &GPIO_InitStructure);
            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
            GPIO_Init(GPIOC, &GPIO_InitStructure);
            GPIO_PinAFConfig(GPIOC, GPIO_PinSource4, GPIO_AF_7);
            GPIO_PinAFConfig(GPIOC, GPIO_PinSource5, GPIO_AF_7);

            //UART
            RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
            USART_Init(USART1, &USART_InitStructure);

            //Interrupts
            NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
            NVIC_Init(&NVIC_InitStructure);
            NVIC_EnableIRQ(USART1_IRQn);
            //USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
            USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

            //enable usart
            USART_Cmd(USART1, ENABLE);
            break;

        case UART4_PC10_PC11:
            peripheral_address[3] = new_peripheral;
            new_peripheral->USARTx = UART4;

            //GPIO
            RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);
            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
            GPIO_Init(GPIOC, &GPIO_InitStructure);
            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
            GPIO_Init(GPIOC, &GPIO_InitStructure);
            GPIO_PinAFConfig(GPIOC, GPIO_PinSource10, GPIO_AF_5);
            GPIO_PinAFConfig(GPIOC, GPIO_PinSource11, GPIO_AF_5);

            //UART
            RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);
            USART_Init(UART4, &USART_InitStructure);

            //Interrupts
            NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;
            NVIC_Init(&NVIC_InitStructure);
            NVIC_EnableIRQ(UART4_IRQn);
            //USART_ITConfig(USART4, USART_IT_TXE, ENABLE);
            USART_ITConfig(UART4, USART_IT_RXNE, ENABLE);

            //enable usart
            USART_Cmd(UART4, ENABLE);
            break;

        case USART3_PC10_PC11:
            peripheral_address[2] = new_peripheral;
            new_peripheral->USARTx = USART3;

            //GPIO
            RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);
            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
            GPIO_Init(GPIOC, &GPIO_InitStructure);
            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
            GPIO_Init(GPIOC, &GPIO_InitStructure);
            GPIO_PinAFConfig(GPIOC, GPIO_PinSource10, GPIO_AF_7);
            GPIO_PinAFConfig(GPIOC, GPIO_PinSource11, GPIO_AF_7);

            //UART
            RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
            USART_Init(USART3, &USART_InitStructure);

            //Interrupts
            NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
            NVIC_Init(&NVIC_InitStructure);
            NVIC_EnableIRQ(USART3_IRQn);
            //USART_ITConfig(USART3, USART_IT_TXE, ENABLE);
            USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);

            //enable usart
            USART_Cmd(USART3, ENABLE);
            break;

        case UART5_PC12_PD2:
            peripheral_address[4] = new_peripheral;
            new_peripheral->USARTx = UART5;

            //GPIO
            RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);
            RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOD, ENABLE);
            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
            GPIO_Init(GPIOC, &GPIO_InitStructure);
            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
            GPIO_Init(GPIOD, &GPIO_InitStructure);
            GPIO_PinAFConfig(GPIOC, GPIO_PinSource12, GPIO_AF_5);
            GPIO_PinAFConfig(GPIOD, GPIO_PinSource2, GPIO_AF_5);

            //UART
            RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART5, ENABLE);
            USART_Init(UART5, &USART_InitStructure);

            //Interrupts
            NVIC_InitStructure.NVIC_IRQChannel = UART5_IRQn;
            NVIC_Init(&NVIC_InitStructure);
            NVIC_EnableIRQ(UART5_IRQn);
            //USART_ITConfig(USART5, USART_IT_TXE, ENABLE);
            USART_ITConfig(UART5, USART_IT_RXNE, ENABLE);

            //enable usart
            USART_Cmd(UART5, ENABLE);
            break;

        case USART2_PD5_PD6:
            peripheral_address[1] = new_peripheral;
            new_peripheral->USARTx = USART2;

            //GPIO
            RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOD, ENABLE);
            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
            GPIO_Init(GPIOD, &GPIO_InitStructure);
            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
            GPIO_Init(GPIOD, &GPIO_InitStructure);
            GPIO_PinAFConfig(GPIOD, GPIO_PinSource5, GPIO_AF_7);
            GPIO_PinAFConfig(GPIOD, GPIO_PinSource6, GPIO_AF_7);

            //UART
            RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
            USART_Init(USART2, &USART_InitStructure);

            //Interrupts
            NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
            NVIC_Init(&NVIC_InitStructure);
            NVIC_EnableIRQ(USART2_IRQn);
            //USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
            USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

            //enable usart
            USART_Cmd(USART2, ENABLE);
            break;

        case USART3_PD8_PD9:
            peripheral_address[2] = new_peripheral;
            new_peripheral->USARTx = USART3;

            //GPIO
            RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOD, ENABLE);
            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
            GPIO_Init(GPIOD, &GPIO_InitStructure);
            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
            GPIO_Init(GPIOD, &GPIO_InitStructure);
            GPIO_PinAFConfig(GPIOD, GPIO_PinSource8, GPIO_AF_7);
            GPIO_PinAFConfig(GPIOD, GPIO_PinSource9, GPIO_AF_7);

            //UART
            RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
            USART_Init(USART3, &USART_InitStructure);

            //Interrupts
            NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
            NVIC_Init(&NVIC_InitStructure);
            NVIC_EnableIRQ(USART3_IRQn);
            //USART_ITConfig(USART3, USART_IT_TXE, ENABLE);
            USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);

            //enable usart
            USART_Cmd(USART3, ENABLE);
            break;

        case USART1_PE0_PE1:
            peripheral_address[0] = new_peripheral;
            new_peripheral->USARTx = USART1;

            //GPIO
            RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOE, ENABLE);
            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
            GPIO_Init(GPIOE, &GPIO_InitStructure);
            GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
            GPIO_Init(GPIOE, &GPIO_InitStructure);
            GPIO_PinAFConfig(GPIOE, GPIO_PinSource0, GPIO_AF_7);
            GPIO_PinAFConfig(GPIOE, GPIO_PinSource1, GPIO_AF_7);

            //UART
            RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
            USART_Init(USART1, &USART_InitStructure);

            //Interrupts
            NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
            NVIC_Init(&NVIC_InitStructure);
            NVIC_EnableIRQ(USART1_IRQn);
            //USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
            USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

            //enable usart
            USART_Cmd(USART1, ENABLE);
            break;
    }*/

    UART_set_state(new_peripheral, UART_STATE_OK);
}

//updates UART state, also can be used for error processing
void UART_set_state(UART *peripheral, UART_STATE new_state) {
	peripheral->state = new_state;

	//Detect which UART has changed is state. Do something, like setting a led or sending error message.
	switch ((int) peripheral->USARTx) {
		case (int) USART1:	//BMS
			if (new_state == UART_STATE_OK) {
				gpio_set(GPIO_LED_UART, 1);
			} else {
				gpio_set(GPIO_LED_UART, 0);
			}
			break;
		case (int) USART2:
			//Your code here...

			break;
		case (int) USART3:	//Steering wheel & XBee
			if (new_state == UART_STATE_OK) {
				//gpio_set_output(GPIO_LED_XBEE_OK);
			} else {
				//gpio_clear_output(GPIO_LED_XBEE_OK);
			}
			break;
		case (int) UART4:
			//Your code here...

			break;
		case (int) UART5:
			//Your code here...

			break;
	}

    /*peripheral->state = new_state;
    if (new_state == UART_STATE_OK) {
        gpio_set_output(GPIO_LED_IMD_BPD_ERR);
    } else {
        gpio_clear_output(GPIO_LED_IMD_BPD_ERR);
    }*/
}

//transmits one byte
void UART_transmit(UART *peripheral, char data) {
    /*if (peripheral->state == UART_STATE_OK) {
        //If hardware is available, write directly to it. If not, add byte to queue.
        //if (USART_GetFlagStatus(peripheral->USARTx, USART_FLAG_TXE) == SET) {
        //    USART_SendData(peripheral->USARTx, (char) data);
        //    USART_ITConfig(peripheral->USARTx, USART_IT_TXE, ENABLE);
        //} else {
            if (peripheral->outbox.length < peripheral->outbox.max_length) {
                UART_enqueue(&peripheral->outbox, data);
                USART_ITConfig(peripheral->USARTx, USART_IT_TXE, ENABLE);
            } else {
                UART_set_state(peripheral, UART_STATE_ERROR_TX_OVERRUN);
            }
        //}
    }

    //if hardware is empty, dequeue to it and enable transmit data register empty interrupt
    if (USART_GetFlagStatus(peripheral->USARTx, USART_FLAG_TXE) == SET) {
    	USART_SendData(peripheral->USARTx, UART_dequeue(&peripheral->outbox));
    	USART_ITConfig(peripheral->USARTx, USART_IT_TXE, ENABLE);
    }*/

	//there was some bug with interrupts, so I made this simple but slow version
	while (1) {
		if (USART_GetFlagStatus(peripheral->USARTx, USART_FLAG_TXE) == SET) {
			USART_SendData(peripheral->USARTx, (char) data);
			break;
		}
	}
}

//transmits one byte
void uart_putc(UART *peripheral, char data){
	peripheral->outbox_busy = 1;

	//add byte to queue
	UART_enqueue(&peripheral->outbox, data);

	peripheral->outbox_busy = 0;

	//enable 'hardware empty' interrupt
	USART_ITConfig(peripheral->USARTx, USART_IT_TXE, ENABLE);
}

//transmits string
void uart_putstr(UART *peripheral, char *str){
	int i=0;
	while (i < strlen(str)) {
		uart_putc(peripheral, str[i]);
		++i;
	}
}

//transmits string with 'carriage return' and 'newline' characters appended
void uart_putline(UART *peripheral, char *str){
	uart_putstr(peripheral, str);
	uart_putc(peripheral, '\r');
	uart_putc(peripheral, '\n');
}

//Sends string. Waits for outbox to get empty to preserve information
void UART_putstr(UART *peripheral, char *str){
	int len = strlen(str);
	int pos = 0;	//position in string
	while (pos < len) {
		if (peripheral->outbox.length < peripheral->outbox.max_length) {	//only if queue has empty slots
			UART_transmit(peripheral, str[pos]);
			++pos;
		}
	}
	//Now write carriage return and newline characters.
	//Wait for at least 2 slots to get empty
	while ((peripheral->outbox.length + 2) > peripheral->outbox.max_length) {}
	UART_transmit(peripheral, '\r');
	UART_transmit(peripheral, '\n');
}

//returns number of strings awaiting reception
int uart_inbox_count(UART *peripheral){
	return peripheral->inbox_string_count;
}

//receives one string from queue
void uart_getstr(UART *peripheral, char *str){
	if (peripheral->inbox_string_count > 0) {
		int i = 0;
		while (i < peripheral->inbox.max_length) {	//compare i to inbox length for safety (prevents endless loops)
			str[i] = UART_dequeue(&peripheral->inbox);
			if (str[i] == 0) {
				--peripheral->inbox_string_count;
				break;
			}
			++i;
		}
	}
	/*if (string_queue_size(&peripheral->inbox) > 0) {
		string_dequeue(&peripheral->inbox, str);
	}*/
}

//returns 1 if outbox has something
int uart_outbox_not_empty(UART *peripheral){
	if (peripheral->outbox.length > 0) {
		return 1;
	}
	return 0;
}

//processes byte received by hardware
void process_reception(UART *peripheral, char data){
	if ((data == '\r') || (data == '\n') || (data == 0)) {		//end of string detected
		if (peripheral->inbox_last_string_length > 0)  {
			UART_enqueue(&peripheral->inbox, 0);				//adds terminating character
			peripheral->inbox_last_string_length = 0;
			++peripheral->inbox_string_count;
		}
	} else {	//normal character, save it to queue
		UART_enqueue(&peripheral->inbox, data);
		++peripheral->inbox_last_string_length;
	}

	/*//if buffer is full, clear it
	if (peripheral->buffer_length >= MAX_INPUT_STRING_LENGTH - 1) {
		strcpy(peripheral->buffer, "");
		peripheral->buffer_length = 0;
	}

	if ((data == '\r') || (data == '\n')) {		//end of string detected
		if (peripheral->buffer_length != 0) {

			//add terminating character
			peripheral->buffer[peripheral->buffer_length] = 0;
			++peripheral->buffer_length;

			//enqueue buffer contents in string queue
			string_enqueue(&peripheral->inbox, peripheral->buffer);

			//clear buffer
			strcpy(peripheral->buffer, "");
			peripheral->buffer_length = 0;
		}
	} else {	//valid data, save it to buffer
		peripheral->buffer[peripheral->buffer_length] = data;
		++peripheral->buffer_length;
	}*/
}

void UART_IRQ_process(UART *peripheral) {
    //transmit data register empty
    if (USART_GetITStatus(peripheral->USARTx, USART_IT_TXE) == SET) {

    	if (peripheral->outbox_busy == 0) {
    		//if there is something in queue, transmit it. Otherwise clear TXE interrupt.
    		if (peripheral->outbox.length > 0) {
    			USART_SendData(peripheral->USARTx, (char) UART_dequeue(&peripheral->outbox));
    		} else {
    			USART_ITConfig(peripheral->USARTx, USART_IT_TXE, DISABLE);
    		}
    	} else {
    		USART_ITConfig(peripheral->USARTx, USART_IT_TXE, DISABLE);
    	}

        /*//if there is something in queue, transmit it. Otherwise clear TXE interrupt.
        if (peripheral->outbox.length > 0) {
            USART_SendData(peripheral->USARTx, (char) UART_dequeue(&peripheral->outbox));
        } else {
            USART_ITConfig(peripheral->USARTx, USART_IT_TXE, DISABLE);
        }*/
    }

    //receive data register not empty
    if (USART_GetITStatus(peripheral->USARTx, USART_IT_RXNE) == SET) {
    	process_reception(peripheral, (char) USART_ReceiveData(peripheral->USARTx));
    }
}

void USART1_IRQHandler(void) {UART_IRQ_process(peripheral_address[0]);}
void USART2_IRQHandler(void) {UART_IRQ_process(peripheral_address[1]);}
void USART3_IRQHandler(void) {UART_IRQ_process(peripheral_address[2]);}
void UART4_IRQHandler(void) {UART_IRQ_process(peripheral_address[3]);}
void UART5_IRQHandler(void) {UART_IRQ_process(peripheral_address[4]);}


//adds byte to queue
void UART_enqueue(UART_queue *mailbox, char data) {
    mailbox->data[mailbox->last++] = data;                          //add byte to queue, increment pointer
    if (mailbox->last == mailbox->max_length) {mailbox->last = 0;}  //if pointer reaches array end, set it back to array start.
    ++(mailbox->length);                                            //increase number of bytes in array
}

//extracts byte from queue
char UART_dequeue(UART_queue *mailbox) {
	char data = mailbox->data[mailbox->first++];                        //get byte, increment pointer
    if (mailbox->first == mailbox->max_length) {mailbox->first = 0;}    //if pointer reaches array end, set it back to array start.
    --(mailbox->length);                                                //decrease number of bytes in array
    return data;
}
























