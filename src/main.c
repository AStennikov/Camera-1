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

#define DEBUG

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

//these set and clear test patterns
void set_test_pattern();
void test_pattern_red();
void test_pattern_green();
void test_pattern_blue();
void clear_pattern();

void set_shutter_speed();

void take_picture(int shutter_speed_ms);

int process_command(char *cmd);


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

	i2c_init();

	delay_ms(8000);

	int DCMI_init_status = camera_init(&uart);
#ifdef DEBUG
	if (DCMI_init_status == 0) {
		uart_putline(&uart, "DCMI init OK");
	} else {
		sprintf(buf, "DCMI init status: %x", DCMI_init_status);
		uart_putline(&uart, buf);
	}
#endif

	int msg_counter = 0;

	//set_test_pattern();
	//test_pattern_blue();

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

			if (strncmp(msg, "cmd", 3) == 0) {	//this is UART command start
				//uart_putline(&uart, msg);
				process_command(msg);


			} else if (strcmp(msg, "h") == 0) {
				uart_putline(&uart, "HELP");
				uart_putline(&uart, "Commands are entered by typing command and pressing enter");
				uart_putline(&uart, "Press 'h' to get help");
				uart_putline(&uart, "Type 'start' to get 'OK' response");
				uart_putline(&uart, "Typing any other command will cause error response");
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

//gets integer from string. Stops at first non-numeric character.
int str_to_i(int offset, char *str) {
	int return_value = 0;
	while ((offset < strlen(str)) && (str[offset]>=48) && (str[offset]<=57)) {
		return_value = return_value*10 + ((int) str[offset] - 48);
		++offset;
	}
	return return_value;
}

//command syntax:
//cmd -xxx yyy
//where cmd is header indicating start of command frame, -xxx is command and y is argument
//command list
//-row xxxx 		- sets start row where xxxx is value
//-column xxxx 		- sets start row where xxxx is value
//-width xxxx 		- sets image width where xxxx is odd value
//-height xxxx 		- sets image height where xxxx is odd value
//-speed xxxx 		- sets shutter speed where xxxx is shutter speed in ms
//-pic 				- orders camera to take picture with parameters given above
//IMPORTANT: width is actually vertical dimension and height is horizontal. W=99 H=1 will give 1 frame and 2 lines. So max sensor width is 2005 and height 2751.
//row and column seems correct
int process_command(char *cmd) {
	int offset = 4; //skipped first space and go directly to first command

	int argument = 0;

	while (offset < strlen(cmd)) {
		if (cmd[offset] == '-') {	//start of command detected
			if (strncmp(&cmd[offset+1], "row ", 4) == 0) {	//user wants to set row
				offset += 5;
				argument = str_to_i(offset, cmd);
				if (camera_set_row(argument)) {
					uart_putline(&uart, "OK");
					sprintf(buf, "Row set to %d", argument);
					uart_putline(&uart, buf);
				} else {
					uart_putline(&uart, "ERROR: UNABLE TO SET ROW");
				}
			} else if (strncmp(&cmd[offset+1], "column ", 7) == 0) {
				offset += 8;
				argument = str_to_i(offset, cmd);
				if (camera_set_column(argument)) {
					uart_putline(&uart, "OK");
					sprintf(buf, "Column set to %d", argument);
					uart_putline(&uart, buf);
				} else {
					uart_putline(&uart, "ERROR: UNABLE TO SET COLUMN");
				}
			} else if (strncmp(&cmd[offset+1], "width ", 6) == 0) {
				offset += 7;
				argument = str_to_i(offset, cmd);
				if (camera_set_width(argument)) {
					uart_putline(&uart, "OK");
					sprintf(buf, "Width set to %d", argument);
					uart_putline(&uart, buf);
				} else {
					uart_putline(&uart, "ERROR: UNABLE TO SET WIDTH");
				}
			} else if (strncmp(&cmd[offset+1], "height ", 7) == 0) {
				offset += 8;
				argument = str_to_i(offset, cmd);
				if (camera_set_height(argument)) {
					uart_putline(&uart, "OK");
					sprintf(buf, "Height set to %d", argument);
					uart_putline(&uart, buf);
				} else {
					uart_putline(&uart, "ERROR: UNABLE TO SET HEIGHT");
				}
			} else if (strncmp(&cmd[offset+1], "speed ", 6) == 0) {
				offset += 7;
				argument = str_to_i(offset, cmd);
				if (camera_set_shutter_speed(argument)) {
					uart_putline(&uart, "OK");
					sprintf(buf, "Shutter speed set to %d", argument);
					uart_putline(&uart, buf);
				}
			} else if (strncmp(&cmd[offset+1], "barpattern", 10) == 0) {
				set_test_pattern();
				uart_putline(&uart, "Bar pattern set");
			} else if (strncmp(&cmd[offset+1], "redpattern", 10) == 0) {
				test_pattern_red();
				uart_putline(&uart, "Red pattern set");
			} else if (strncmp(&cmd[offset+1], "greenpattern", 12) == 0) {
				test_pattern_green();
				uart_putline(&uart, "Green pattern set");
			} else if (strncmp(&cmd[offset+1], "bluepattern", 11) == 0) {
				test_pattern_blue();
				uart_putline(&uart, "Blue pattern set");
			} else if (strncmp(&cmd[offset+1], "nopattern", 9) == 0) {
				clear_pattern();
				uart_putline(&uart, "Pattern cleared");
			} else if (strncmp(&cmd[offset+1], "pic", 3) == 0) {
				uart_putline(&uart, "Taking picture");
				camera_take_picture();
				uart_putline(&uart, "DONE");
			} else if (strncmp(&cmd[offset+1], "i2cwr", 5) == 0) {	//command for writing sensor registers
				//syntax: -i2cwr xx yyyy, where xx is register id and yyyy is value
				offset += 7;
				int reg = str_to_i(offset, cmd);
				while ((cmd[offset] != ' ') && (offset<strlen(cmd))) { ++offset;}	//scan to next space
				++offset;	//skip space
				int value = str_to_i(offset, cmd);
				sensor_set(reg, value);
				uart_putline(&uart, "OK");
			} else if (strncmp(&cmd[offset+1], "i2crd", 5) == 0) {	//command for reading sensor registers
				//syntax: -i2crd xx, where xx is register id.
				offset += 7;
				int reg = str_to_i(offset, cmd);
				int value = sensor_get(reg);
				sprintf(buf, "%d", value);
				uart_putline(&uart, buf);
				uart_putline(&uart, "OK");
			} else if (strncmp(&cmd[offset+1], "rshift", 6) == 0) {	//command for reading sensor registers
				offset += 8;
				int value = str_to_i(offset, cmd);
				camera_set_rshift(value);
				uart_putline(&uart, "OK");
			} else if (strncmp(&cmd[offset+1], "getdata", 7) == 0) {
				int green1 = 0;
				int green2 = 0;
				int red = 0;
				int blue = 0;

				int array_height = 0;
				int array_width = 0;

				//size of output image depends on skipping settings
				int skip = 3;
				if ((camera_frame_height()+1)%((skip+1)*2) > 0 ) {	//ceil adds 1
					array_height = 2*((camera_frame_height()+1)/((skip+1)*2) + 1);
				} else {	//ceil does not add 1
					array_height = 2*((camera_frame_height()+1)/((skip+1)*2));
				}
				if ((camera_frame_width()+1)%((skip+1)*2) > 0 ) {	//ceil adds 1
					array_width = 2*((camera_frame_width()+1)/((skip+1)*2) + 1);
				} else {	//ceil does not add 1
					array_width = 2*((camera_frame_width()+1)/((skip+1)*2));
				}
				sprintf(buf, "%d:%d:%d", array_height/2, array_width/2, (array_height/2)*(array_width/2));
				uart_putline(&uart, buf);

				//array_height = (camera_frame_height()+1)/2;
				array_height = array_height/2;
				//j<(camera_frame_width()+1)
				int i, j;
				/*for (j=0; j<array_width; j+=2) {
					for (i=0; i<array_height; ++i) {
						//not including 2 upper bits when masking because DCMI_D10 is always on. Reason unknown.
						green1 = image[array_height*j+i] & 0x000003ff;
						red = (image[array_height*j+i]>>16) & 0x000003ff;

						blue = image[array_height*j+array_height+i] & 0x000003ff;
						green2 = (image[array_height*j+array_height+i]>>16) & 0x000003ff;

						sprintf(buf, "%d %d %x %x %x   %d %d %x %x %x", i, array_height*j, image[array_height*j+i], red, green1, i+array_height, array_height*j, image[array_height*j+i+array_height], green2, blue);
						uart_putline(&uart, buf);
						sprintf(buf, "%x,%x,%x", red>>2, (green1+green2)>>3, blue>>2);
						uart_putline(&uart, buf);

						delay_ms(1);
					}
				}*/

				for (i=0; i<(array_height)*(array_width/2); ++i) {
					sprintf(buf, "%x,%x,%x,%x", (image[i]>>24)&0xff, (image[i]>>16)&0xff, (image[i]>>8)&0xff, (image[i]>>0)&0xff);
					uart_putline(&uart, buf);
				}


				//working but for 1-dimensional arrays only
				/*for (i=0; i<array_height; ++i) {
					//not including 2 upper bits when masking because DCMI_D10 is always on. Reason unknown.
					green1 = image[i] & 0x000003ff;
					red = (image[i]>>16) & 0x000003ff;

					blue = image[array_height+i] & 0x000003ff;
					green2 = (image[array_height+i]>>16) & 0x000003ff;

					//sprintf(buf, "%d %x %x %x   %d %x %x %x", i, image[i], red, green1, i+array_height, image[i+array_height], green2, blue);
					//uart_putline(&uart, buf);
					sprintf(buf, "%x,%x,%x", red>>2, (green1+green2)>>3, blue>>2);
					uart_putline(&uart, buf);

					delay_ms(1);
				}*/

				/*for (i=array_height; i<array_height*2; ++i) {
					data1 = image[i] & 0x0000ffff;
					data2 = (image[i]>>16) & 0x0000ffff;

					sprintf(buf, "%d %x", i, (data1+data2)>>5);
					uart_putline(&uart, buf);
					delay_ms(1);
					//++i;
				}*/

				//get rggb values for every pixel.
				/*int column = 0, row = 0;
				int array_width = (camera_frame_width()+1)/2;	//indicates how much array elements are in one line (since 2 pixels got into 1 integer)
				int array_height = camera_frame_height();		//indicates how many lines we have.
				int red = 0, green1 = 0, green2 = 0, blue = 0;	//these stor pixel values

				int i=0;
				for (row=0; row<array_height; row+=2) {		//scan every second row
					for (column=0; column<array_width; ++column) {	//scan every array element in row
						//even row, contains red and green1 data
						green1 = image[row*array_width + column] & 0x0000ffff;
						red = (image[row*array_width + column]>>16) & 0x0000ffff;
						//odd row, contains green2 and blue data
						blue = image[(row+1)*array_width + column] & 0x0000ffff;
						green2 = (image[(row+1)*array_width + column]>>16) & 0x0000ffff;

						//wait until previous data is sent
						while (uart_outbox_not_empty(&uart)) {}

						//compose frame and transmit
						//sprintf(buf, "r:%d c:%d data1:%x r:%d c:%d data2:%x %x,%x,%x,%x;", row, column, image[row*array_width + column], row+1, column, image[(row+1)*array_width + column], red, green1, green2, blue);
						sprintf(buf, "%x,%x,%x", red>>4, (green1+green2)>>5, blue>>4);
						uart_putline(&uart, buf);
						delay_ms(1);
						++i;
					}
				}*/

				/*int i;
				for (i=0; i<100; i+=10) {
					delay_ms(10);
					sprintf(buf, "%d %x %x %x %x %x %x %x %x %x %x", i, image[i], image[i+1], image[i+2], image[i+3], image[i+4], image[i+5], image[i+6], image[i+7], image[i+8], image[i+9]);
					uart_putline(&uart, buf);
				}*/
				//sprintf(buf, "%d", i);
				//uart_putline(&uart, buf);
				//uart_putline(&uart, "DONE");
			} else {
				uart_putline(&uart, "ERROR: unknown command");
			}

		}
		++offset;

	}
	return 0;
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
	sensor_set(0xa4, 16);			//set bar width to 16
	sensor_set(0xa1, 0x11);			//set odd bar color
	sensor_set(0xa3, 0x0ff);		//set even bar color
	uart_putline(&uart, "Monochrome vertical bars test pattern set");
}
void test_pattern_red(){
	sensor_set(0xa0, 0b00000001);	//set test pattern to monochrome vertical bars
	sensor_set(0xa1, 0);	//green
	sensor_set(0xa2, 4095);	//red
	sensor_set(0xa3, 0);	//blue
}
void test_pattern_green() {
	sensor_set(0xa0, 0b00000001);	//set test pattern to monochrome vertical bars
	sensor_set(0xa1, 4095);	//green
	sensor_set(0xa2, 0);	//red
	sensor_set(0xa3, 0);	//blue
}
void test_pattern_blue(){
	sensor_set(0xa0, 0b00000001);	//set test pattern to monochrome vertical bars
	sensor_set(0xa1, 0);	//green
	sensor_set(0xa2, 0);	//red
	sensor_set(0xa3, 4095);	//blue
}
void clear_pattern(){
	sensor_set(0xa0, 0b00111000);	//disables pattern
}



void set_shutter_speed(){


}

void take_picture(int shutter_speed_ms){
	//uart_putline(&uart, "Snapshot");

	camera_take_picture(100, 0, 0, 99, 9);

	int i;
	/*for (i=0; i<50; ++i) {
		sprintf(buf, "%d %x", i, image[i]);
		uart_putline(&uart, buf);

	}*/

	for (i=0; i<5000; i+=10) {
		delay_ms(10);
		sprintf(buf, "%d %x %x %x %x %x %x %x %x %x %x", i, image[i], image[i+1], image[i+2], image[i+3], image[i+4], image[i+5], image[i+6], image[i+7], image[i+8], image[i+9]);
		uart_putline(&uart, buf);
	}

	/*volatile int i=0;

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
	}*/

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
	/*uart_putline(&uart, "++++");	//start of frame
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
	uart_putline(&uart, "+++-");	//end of frame*/



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



