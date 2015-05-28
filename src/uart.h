//Aleksandr Stennikov, FS Team Tallinn 2014

//This library allows transmission and reception of single bytes via UART.
//Using it:
// 1. Declare UART variable.
// 2. Initialize this variable, UART hardware and gpios with UART_init() function.
// 3. Transmit bytes by calling UART_transmit() function.
// 4. Receive bytes by using UART_receive() function. UART_inbox_count returns number of bytes pending reception.
// 5. Additionally, UART_set_state() in uart.c is used to perform actions when UART state changes. Like setting/clearing a led, for example.

#include "stm32f4xx.h"
#include "string_queue.h"



#define MAX_INPUT_STRING_LENGTH 1024		//max allowable length of string in characters
#define INBOX_SIZE 10					//max number of strings in inbox


typedef enum {
    UART_STATE_OK,
    UART_STATE_ERROR_GENERAL,
    UART_STATE_ERROR_TX_OVERRUN,
    UART_STATE_ERROR_RX_OVERRUN,
} UART_STATE;

typedef enum {
	//STM32F4
	USART1_PA9_PA10,	//TX at PA9, RX at PA10
	
	//STM32F0
	//USART1_PA9_PA10,	//TX at PA9, RX at PA10
	//USART2_PA2_PA3,     //TX at PA2, RX at PA3.

	//STM32F3
    /*USART2_PA2_PA3,     //TX at PA2, RX at PA3. Tested
    USART1_PA9_PA10,    //TX at PA9, RX at PA10. Tested
    USART2_PA14_PA15,   //TX at PA14, RX at PA15. Tested
    USART2_PB3_PB4,     //TX at PB3, RX at PB4. Tested
    USART1_PB6_PB7,     //TX at PB6, RX at PB7. Tested
    USART3_PB10_PB11,   //TX at PB10, RX at PB11. Tested
    USART1_PC4_PC5,     //TX at PC4, RX at PC5. Tested
    UART4_PC10_PC11,    //TX at PC10, RX at PC11. Tested
    USART3_PC10_PC11,   //TX at PC10, RX at P11. Tested
    UART5_PC12_PD2,     //TX at PC12, RX at PD2. Tested
    USART2_PD5_PD6,     //TX at PD5, RX at PD6. Tested
    USART3_PD8_PD9,     //TX at PD8, RX at PD9. Tested
    USART1_PE0_PE1,     //TX at PE0, RX at PE1. Not tested, impossible to test on a devkit*/
} UART_GPIO;

//byte queue
typedef struct {
    int first;          //points to first byte in queue
    int last;           //points to last byte in queue
    int length;         //current queue length
    int max_length;     //max allowed queue length
    char *data;           //data stored in queue
} UART_queue;

//describes single UART peripheral
typedef struct {
    USART_TypeDef *USARTx;
    UART_STATE state;
    //UART_queue inbox;

    //outbox
    UART_queue outbox;
    char outbox_busy;		//indicates that user is writing something to outbox - interrupt action postponed

    //inbox
    /*string_queue_t inbox;						//queue of received strings
    char buffer[MAX_INPUT_STRING_LENGTH];		//incoming characters are stored here
    int buffer_length;							//number of characters in buffer[]*/

    //inbox
    UART_queue inbox;
    int inbox_string_count;				//keeps track on number of strings in inbox
    int inbox_last_string_length;		//keeps track of string length to prevent null strings from being written

} UART;



//some standard baud rates
#define UART_BAUD_9600 9600
#define UART_BAUD_19200 19200
#define UART_BAUD_38400 38400
#define UART_BAUD_57600 57600
#define UART_BAUD_115200 115200

//Initializes UART peripheral
//Parameters:
//new_peripheral    - address of UART variable
//gpio              - microcontroller pins to be used
//baud_rate         - one of the standard baud rates #defined above. Or your own number.
//inbox_size        - size of inbox queue. In bytes.
//outbox_size       - size of outbox queue. In bytes.
void UART_init(UART *new_peripheral, UART_GPIO gpio, int baud_rate, int inbox_size, int outbox_size);

//Transmits one byte
void UART_transmit(UART *peripheral, char data);

//transmits one byte
void uart_putc(UART *peripheral, char data);

//transmits string
void uart_putstr(UART *peripheral, char *str);

//transmits string with 'carriage return' and 'newline' characters
void uart_putline(UART *peripheral, char *str);

//Sends string. Waits for outbox to get empty to preserve information
void UART_putstr(UART *peripheral, char *str);

//Returns number of strings awaiting reception
int uart_inbox_count(UART *peripheral);

//returns 1 if outbox has something
int uart_outbox_not_empty(UART *peripheral);

//Receives one byte. If queue is empty, returns 0.
//char UART_receive(UART *peripheral);

//gets string from inbox
void uart_getstr(UART *peripheral, char *str);







