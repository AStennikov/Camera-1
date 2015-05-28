#include "stm32f4xx.h"
#include "gpio.h"
#include "event_timer.h"
#include "stm32f4xx_rcc.h"
#include "uart.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "i2c.h"
#include "camera.h"
#include "clock.h"

#define TMRD(x) (x << 0)  /* Load Mode Register to Active */
#define TXSR(x) (x << 4)  /* Exit Self-refresh delay */
#define TRAS(x) (x << 8)  /* Self refresh time */
#define TRC(x)  (x << 12) /* Row cycle delay */
#define TWR(x)  (x << 16) /* Recovery delay */
#define TRP(x)  (x << 20) /* Row precharge delay */
#define TRCD(x) (x << 24) /* Row to column delay */

void init_sdram();

void gpio_test();

void set_frame_size(int width, int height);

void set_test_pattern();

void set_shutter_speed();

void take_picture(int shutter_speed_ms);


//this stores last value of snapshot button. Picture is made on falling edge.
int snapshot_prev = 1, snapshot_cur = 1;
int snapshot_falling_edge();

char buf[1024];

UART uart;

//char img[100*100];

//MEMORY TESTING RESULTS:
//D0-D16 works perfectly

int main(void)
{

	gpio_init();
	gpio_set(GPIO_TRIGGER, 1);

	event_timer_init();

	//gpio_test();

	init_sdram();

	//UART uart;
	//UART_init(&uart, USART1_PA9_PA10, UART_BAUD_115200, 256, 1024);
	UART_init(&uart, USART1_PA9_PA10, 3000000, 256, 1024);

	//clock source is PLL, frequency unknown. Following function sets 1ms systick interrupt.
	SysTick_Config(22500*4);

	uart_putline(&uart, "\r\n---------------------------------------------------------");
	uart_putline(&uart, "Welcome to Camera 1. Press 'h' and hit 'Enter' for help.");


	RCC_ClocksTypeDef RCC_Clocks;
	RCC_GetClocksFreq(&RCC_Clocks);
	sprintf(buf, "SYSCLK: %d", (int) RCC_Clocks.SYSCLK_Frequency);
	uart_putline(&uart, buf);
	sprintf(buf, "HCLK: %d", (int) RCC_Clocks.HCLK_Frequency);
	uart_putline(&uart, buf);
	sprintf(buf, "PCLK1: %d", (int)RCC_Clocks.PCLK1_Frequency);
	uart_putline(&uart, buf);
	sprintf(buf, "PCLK2: %d", (int) RCC_Clocks.PCLK2_Frequency);
	uart_putline(&uart, buf);


	volatile int i = 0;
	//char buf[256];
	char msg[256];

	//init 6Mhz sensor clock at PE5 (pin 4)
	clock_init();



	/*int *p = (int *) 0xa0000000;
	*p = 42;

	if (*p == 42) {
		uart_putline(&uart, "Memory test success");
	} else {
		uart_putline(&uart, "Memory test fail");
	}

	for (i=0; i<100; i++){
		//p += 1;
		p[i] = 42;
		//delay_ms(5);
	}

	//gpio_set(GPIO_LED_BUSY, 1);
	//p = (int *) 0xa0000000;
	for (i=0; i<50; i++){
		//p += 1;
		//delay_ms(20);
		if (p[i] != 42) {
			sprintf(buf, "Memory test fail at index %d. Value: %d, addr: %x", i, p[i], &p[i]);
			uart_putline(&uart, buf);
			//break;
			delay_ms(2);
		}
	}*/

	//while(1) {}

	i2c_init();

	//counting to 100000000 takes about 8000ms
	for (i=0; i<100000000; ++i) {}

	//sensor_set(1, 100);

	for (i=0; i<1000000; ++i) {}

	int chip_version = sensor_get(0);
	sprintf(buf, "Sensor chip version %d", chip_version);
	uart_putline(&uart, buf);
	for (i=0; i<1000000; ++i) {}


	//setup sensor
	//ERS mode, bulb exposure
	sensor_set(0x1e, 0x4146);

	set_frame_size(99, 99);



	for (i=0; i<100000000; ++i) {}

	//set_test_pattern();

	//take_picture(100);

	//initialize dcmi
	camera_init(&uart);
	//camera_init_gpio();

	int msg_counter = 0;

	if (camera_test_image_array() == 0) {
		uart_putline(&uart, "Image array ready");
	}

	//while(1) {}

	while (1)
	{
		//if (camera_test_image_array()) {
		//	uart_putline(&uart, "Image array not empty");
		//}

		//reads snapshot button and makes picture if falling edge is detected
		if (snapshot_falling_edge()) {
			take_picture(100);

			//delay 80ms
			for (i=0; i<1000000; ++i) {}
		}

		if (uart_inbox_count(&uart)>0) {
			uart_getstr(&uart, msg);

			uart_putline(&uart, "---------------------------------------------------------");
			sprintf(buf, "Message #%d", msg_counter);
			uart_putline(&uart, buf);
			++msg_counter;

			if (strcmp(msg, "start") == 0) {
				uart_putline(&uart, msg);
				uart_putline(&uart, "OK");
			} else if (strcmp(msg, "h") == 0) {
				uart_putline(&uart, "HELP");
				uart_putline(&uart, "Commands are entered by typing command and pressing enter");
				uart_putline(&uart, "Press 'h' to get help");
				uart_putline(&uart, "Type 'start' to get 'OK' responce");
				uart_putline(&uart, "Typing any other command will cause error responce");
			} else if (strcmp(msg, "p") == 0) {
				take_picture(100);

			} else {
				uart_putline(&uart, msg);
				uart_putline(&uart, "ERROR");
			}


		}


		//blinks led
		if (event_flag_is_set(FLAG_LED_MCU)) {
			gpio_toggle(GPIO_LED_MCU);
		}

	}




}



void init_sdram(){
	//enabling clocks for required peripherals
	//RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);
	RCC_AHB3PeriphClockCmd(RCC_AHB3Periph_FMC, ENABLE);

	//configuring pins
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource0, GPIO_AF_FMC);	//SDNWE
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource2, GPIO_AF_FMC);	//SDNE0
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource3, GPIO_AF_FMC);	//SDCKE0
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource0, GPIO_AF_FMC);	//D2
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource1, GPIO_AF_FMC);	//D3
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource8, GPIO_AF_FMC);	//D13
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource9, GPIO_AF_FMC);	//D14
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource10, GPIO_AF_FMC);	//D15
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource14, GPIO_AF_FMC);	//D0
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource15, GPIO_AF_FMC);	//D1
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource0, GPIO_AF_FMC);	//NBL0
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource1, GPIO_AF_FMC);	//NBL1
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource7, GPIO_AF_FMC);	//D4
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource8, GPIO_AF_FMC);	//D5
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource9, GPIO_AF_FMC);	//D6
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource10, GPIO_AF_FMC);	//D7
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource11, GPIO_AF_FMC);	//D8
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource12, GPIO_AF_FMC);	//D9
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource13, GPIO_AF_FMC);	//D10
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource14, GPIO_AF_FMC);	//D11
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource15, GPIO_AF_FMC);	//D12
	GPIO_PinAFConfig(GPIOF, GPIO_PinSource0, GPIO_AF_FMC);	//A0
	GPIO_PinAFConfig(GPIOF, GPIO_PinSource1, GPIO_AF_FMC);	//A1
	GPIO_PinAFConfig(GPIOF, GPIO_PinSource2, GPIO_AF_FMC);	//A2
	GPIO_PinAFConfig(GPIOF, GPIO_PinSource3, GPIO_AF_FMC);	//A3
	GPIO_PinAFConfig(GPIOF, GPIO_PinSource4, GPIO_AF_FMC);	//A4
	GPIO_PinAFConfig(GPIOF, GPIO_PinSource5, GPIO_AF_FMC);	//A5
	GPIO_PinAFConfig(GPIOF, GPIO_PinSource11, GPIO_AF_FMC);	//SDNRAS
	GPIO_PinAFConfig(GPIOF, GPIO_PinSource12, GPIO_AF_FMC);	//A6
	GPIO_PinAFConfig(GPIOF, GPIO_PinSource13, GPIO_AF_FMC);	//A7
	GPIO_PinAFConfig(GPIOF, GPIO_PinSource14, GPIO_AF_FMC);	//A8
	GPIO_PinAFConfig(GPIOF, GPIO_PinSource15, GPIO_AF_FMC);	//A9
	GPIO_PinAFConfig(GPIOG, GPIO_PinSource0, GPIO_AF_FMC);	//A10
	GPIO_PinAFConfig(GPIOG, GPIO_PinSource1, GPIO_AF_FMC);	//A11
	GPIO_PinAFConfig(GPIOG, GPIO_PinSource4, GPIO_AF_FMC);	//BA0
	GPIO_PinAFConfig(GPIOG, GPIO_PinSource5, GPIO_AF_FMC);	//BA1
	GPIO_PinAFConfig(GPIOG, GPIO_PinSource8, GPIO_AF_FMC);	//SDCLK
	GPIO_PinAFConfig(GPIOG, GPIO_PinSource15, GPIO_AF_FMC);	//SDNCAS

	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStruct.GPIO_OType = GPIO_PuPd_NOPULL;
	GPIO_InitStruct.GPIO_PuPd = GPIO_OType_PP;
	GPIO_Init(GPIOC, &GPIO_InitStruct);

	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_Init(GPIOD, &GPIO_InitStruct);

	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_Init(GPIOE, &GPIO_InitStruct);

	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3 |GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_11|GPIO_Pin_12|GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_Init(GPIOF, &GPIO_InitStruct);

	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_4|GPIO_Pin_5 |GPIO_Pin_8|GPIO_Pin_15;
	GPIO_Init(GPIOG, &GPIO_InitStruct);

	// Enable clock for FMC
	RCC->AHB3ENR |= RCC_AHB3ENR_FMCEN;
	// Initialization step 1
	//SDCLK period is 2 HCLK periods. HCLK is 180Mhz. ONLY IN FIRST BANK REGISTER (ref manual p 1652)
	//single reads are managed as bursts. ONLY IN FIRST BANK REGISTER
	//read pipe: 2 HCLK clock cycle delay. ONLY IN FIRST BANK REGISTER
	//data bus width 16 bits
	//4 banks
	//CAS latency 3 cycles
	//write accesses ignored
	//12 bit row address
	//8 bit column address
	FMC_Bank5_6->SDCR[0] = FMC_SDCR1_SDCLK_1 | FMC_SDCR1_RBURST | FMC_SDCR1_RPIPE_1 | FMC_SDCR1_NR_0 | FMC_SDCR1_MWID_0 | FMC_SDCR1_NB | FMC_SDCR1_CAS;			//first bank
	//FMC_Bank5_6->SDCR[1] = FMC_SDCR1_NR_0 | FMC_SDCR1_MWID_0 | FMC_SDCR1_NB | FMC_SDCR1_CAS;	//second bank
	// Initialization step 2
	//TCRD - row to column delay - 16 cycles
	//TRP - row precharge delay - 3 cycles. ONLY IN FIRST BANK REGISTER (ref manual p 1653)
	//TWR - recovery delay - 16 cycles
	//TRC - row cycle delay - 8 cycles. ONLY IN FIRST BANK REGISTER
	//TRAS - self refresh time - 16 cycles
	//TXSR - exit self refresh delay - 16 cycles
	//TMRD - load mode register to active - 16 cycles
	FMC_Bank5_6->SDTR[0] = TRC(7) | TRP(2) | TMRD(2) | TXSR(7) | TRAS(4) | TWR(2) | TRCD(2);		//first bank
	//FMC_Bank5_6->SDTR[1] = TMRD(2) | TXSR(7) | TRAS(4) | TWR(2) | TRCD(2);	//second bank
	 // Initialization step 3
	while(FMC_Bank5_6->SDSR & FMC_SDSR_BUSY);
	FMC_Bank5_6->SDCMR = 1 | FMC_SDCMR_CTB1 | (1 << 5);	//clock configuration enable mode and 2 auto refresh cycles to bank 1
	 // Initialization step 4
	int tmp;
	for(tmp = 0; tmp < 1000000; tmp++);
	// Initialization step 5
	while(FMC_Bank5_6->SDSR & FMC_SDSR_BUSY);
	FMC_Bank5_6->SDCMR = 2 | FMC_SDCMR_CTB1 | (1 << 5);	//all bank precharge command and 2 auto refresh cycles to bank 1
	// Initialization step 6
	while(FMC_Bank5_6->SDSR & FMC_SDSR_BUSY);
	FMC_Bank5_6->SDCMR = 3 | FMC_SDCMR_CTB1 | (4 << 5);	//auto refresh command and 5 auto refresh cycles to bank 1
	// Initialization step 7
	while(FMC_Bank5_6->SDSR & FMC_SDSR_BUSY);
	FMC_Bank5_6->SDCMR = 4 | FMC_SDCMR_CTB1 | (1 << 5) | (0x231 << 9);	//upload value 0x231 to load mode register and 2 auto refresh cycles to bank 1. Check memory datasheet for load mode register contents.
	// Initialization step 8
	while(FMC_Bank5_6->SDSR & FMC_SDSR_BUSY);
	FMC_Bank5_6->SDRTR |= (683 << 1);			//set refresh rate. (reference manual p 1656)
	while(FMC_Bank5_6->SDSR & FMC_SDSR_BUSY);

	/*//my code that did not work
	//initialize memory
	FMC_SDRAMTimingInitTypeDef SDRAM_TimingInitStruct;
	SDRAM_TimingInitStruct.FMC_LoadToActiveDelay = 16;
	SDRAM_TimingInitStruct.FMC_ExitSelfRefreshDelay = 16;
	SDRAM_TimingInitStruct.FMC_SelfRefreshTime = 16;
	SDRAM_TimingInitStruct.FMC_RowCycleDelay = 8;
	SDRAM_TimingInitStruct.FMC_WriteRecoveryTime = 16;
	SDRAM_TimingInitStruct.FMC_RPDelay = 3;
	SDRAM_TimingInitStruct.FMC_RCDDelay = 16;

	FMC_SDRAMInitTypeDef SDRAM_InitStruct;
	SDRAM_InitStruct.FMC_Bank = FMC_Bank1_SDRAM;
	SDRAM_InitStruct.FMC_RowBitsNumber = FMC_RowBits_Number_12b;
	SDRAM_InitStruct.FMC_ColumnBitsNumber = FMC_ColumnBits_Number_8b;
	SDRAM_InitStruct.FMC_SDMemoryDataWidth = FMC_SDMemory_Width_16b;
	SDRAM_InitStruct.FMC_InternalBankNumber = FMC_InternalBank_Number_4;
	SDRAM_InitStruct.FMC_CASLatency = FMC_CAS_Latency_3;
	SDRAM_InitStruct.FMC_WriteProtection = FMC_Write_Protection_Disable;
	SDRAM_InitStruct.FMC_SDClockPeriod = FMC_SDClock_Period_2;
	SDRAM_InitStruct.FMC_ReadBurst = FMC_Read_Burst_Enable;
	SDRAM_InitStruct.FMC_ReadPipeDelay = FMC_ReadPipe_Delay_2;
	SDRAM_InitStruct.FMC_SDRAMTimingStruct = &SDRAM_TimingInitStruct;
	FMC_SDRAMInit(&SDRAM_InitStruct);

	FMC_SDRAMCommandTypeDef SDRAM_CommandStruct;
	SDRAM_CommandStruct.FMC_CommandMode = FMC_Command_Mode_normal;
	SDRAM_CommandStruct.FMC_CommandTarget = FMC_Command_Target_bank1;
	SDRAM_CommandStruct.FMC_AutoRefreshNumber = 16;
	SDRAM_CommandStruct.FMC_ModeRegisterDefinition = 0x231;
	FMC_SDRAMCmdConfig(&SDRAM_CommandStruct);*/

	/*// Enable clock for FMC
	RCC->AHB3ENR |= RCC_AHB3ENR_FMCEN;
	// Initialization step 1
	FMC_Bank5_6->SDCR[0] = FMC_SDCR1_SDCLK_1 | FMC_SDCR1_RBURST | FMC_SDCR1_RPIPE_1;
	FMC_Bank5_6->SDCR[1] = FMC_SDCR1_NR_0 | FMC_SDCR1_MWID_0 | FMC_SDCR1_NB | FMC_SDCR1_CAS;
	// Initialization step 2
	FMC_Bank5_6->SDTR[0] = TRC(7) | TRP(2);
	FMC_Bank5_6->SDTR[1] = TMRD(2) | TXSR(7) | TRAS(4) | TWR(2) | TRCD(2);
	 // Initialization step 3
	while(FMC_Bank5_6->SDSR & FMC_SDSR_BUSY);
	FMC_Bank5_6->SDCMR = 1 | FMC_SDCMR_CTB2 | (1 << 5);
	 // Initialization step 4
	volatile int tmp;
	for(tmp = 0; tmp < 1000000; tmp++);
	// Initialization step 5
	while(FMC_Bank5_6->SDSR & FMC_SDSR_BUSY);
	FMC_Bank5_6->SDCMR = 2 | FMC_SDCMR_CTB2 | (1 << 5);
	// Initialization step 6
	while(FMC_Bank5_6->SDSR & FMC_SDSR_BUSY);
	FMC_Bank5_6->SDCMR = 3 | FMC_SDCMR_CTB2 | (4 << 5);
	// Initialization step 7
	while(FMC_Bank5_6->SDSR & FMC_SDSR_BUSY);
	FMC_Bank5_6->SDCMR = 4 | FMC_SDCMR_CTB2 | (1 << 5) | (0x231 << 9);
	// Initialization step 8
	while(FMC_Bank5_6->SDSR & FMC_SDSR_BUSY);
	FMC_Bank5_6->SDRTR |= (683 << 1);
	while(FMC_Bank5_6->SDSR & FMC_SDSR_BUSY);*/
}

//configures 1 pin as pulled down input and sets led according to the inputs value. CONTAINS INFINITE LOOP. For testing electrical connections.
void gpio_test(){
	//enable clock
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);

	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStruct.GPIO_OType = GPIO_PuPd_DOWN;
	GPIO_InitStruct.GPIO_PuPd = GPIO_OType_PP;
	GPIO_Init(GPIOG, &GPIO_InitStruct);

	while (1) {
		if (GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_1) == 1) {
			gpio_set(GPIO_LED_BUSY, 1);
		} else {
			gpio_set(GPIO_LED_BUSY, 0);
		}
	}

}

void set_frame_size(int width, int height){
	sensor_set(0x03, width);
	sensor_set(0x04, height);
	sprintf(buf, "Frame size set to %d x %d (w x h)", width, height);
	uart_putline(&uart, buf);
}

void set_test_pattern(){
	sensor_set(0xa0, 0b00111001);	//set test pattern to monochrome vertical bars
	sensor_set(0xa4, 5);			//set bar width to 5
	sensor_set(0xa1, 500);			//set odd bar color
	sensor_set(0xa3, 3500);			//set even bar color
	uart_putline(&uart, "Monochrome vertical bars test pattern set");
}



void set_shutter_speed(){


}

void take_picture(int shutter_speed_ms){
	//uart_putline(&uart, "Snapshot");

	volatile int i=0;

	gpio_set(GPIO_TRIGGER, 0);

	//12500 is roughly 1ms
	for (i=0; i<shutter_speed_ms*12500; ++i) {}

	gpio_set(GPIO_TRIGGER, 1);

	int pixcnt=0;
	for (i=0; i<10000000; ++i) {
		if (DCMI->SR & 4) {
			image[pixcnt++] = DCMI->DR;
			DCMI->SR &= 3;
			if (pixcnt>=5000) {break;}
		}
	}

	/*//transmit raw information
	//start of frame
	uart_putline(&uart, "++++");
	int j;
	for (i=0; i<5000; i+=10) {
		delay_ms(10);
		sprintf(buf, "%d %x %x %x %x %x %x %x %x %x %x", i, image[i], image[i+1], image[i+2], image[i+3], image[i+4], image[i+5], image[i+6], image[i+7], image[i+8], image[i+9]);
		uart_putline(&uart, buf);
	}
	//end of frame
	uart_putline(&uart, "+++-");*/


	//transmit picture
	//average all pixels to produce grayscale image. Output image is 4 times smaller than input. Data is ready for octave.
	uart_putline(&uart, "++++");	//start of frame
	int pixel_buffer[50], j;
	for (j=0; j<100; j+=2) {		//taking 2 lines at a time
		for (i=0; i<50; ++i) {		//every number at a line since it holds 2 pixels already
			pixel_buffer[i] = (image[i + j*50]&0xffff) + ((image[i + j*50]>>16)&0xffff) + (image[i + j*50+50]&0xffff) + ((image[i + j*50+50]>>16)&0xffff);
		}

		//compose string of numbers
		memset(buf, 0, 1024);	//reset buffer values
		int k;
		char number[20];
		for (k=0; k<50; ++k) {
			sprintf(number, "%d ", pixel_buffer[k]/4);
			strcat(buf, number);
		}
		strcat(buf, ";");
		while (uart_outbox_not_empty(&uart)) {}	//waits for previous data to be sent
		uart_putline(&uart, buf);
	}
	uart_putline(&uart, "+++-");	//end of frame



}

int snapshot_falling_edge(){
	int result = 0;
	snapshot_cur = gpio_read(GPIO_SNAPSHOT);
	if ((snapshot_prev == 1) && (snapshot_cur == 0)) {
		result = 1;
	}
	snapshot_prev = snapshot_cur;
	return result;
}



