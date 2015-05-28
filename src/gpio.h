#ifndef GPIO_H_
#define GPIO_H_

typedef enum {
	GPIO_LED_MCU,
	GPIO_LED_UART,
	GPIO_LED_BUSY,

	GPIO_TRIGGER,		//output that controls exposure time
	GPIO_SNAPSHOT,		//pressing this button will trigger camera

	GPIO_COUNT,			//do not delete that
} gpio_name_list;


typedef struct {
	GPIO_TypeDef* GPIOx;			//GPIOA, GPIOB, etc
	uint32_t RCC_AHBPeriph; 		//RCC_AHBPeriph_GPIOA, RCC_AHBPeriph_GPIOB, etc
	uint32_t GPIO_Pin;              //GPIO_Pin_0, GPIO_Pin_1, etc
	GPIOMode_TypeDef GPIO_Mode;     //GPIO_Mode_IN, GPIO_Mode_OUT, GPIO_Mode_AF, GPIO_Mode_AN
	GPIOSpeed_TypeDef GPIO_Speed;   //GPIO_Speed_Level_1 (2MHz), GPIO_Speed_Level_2 (10MHz), GPIO_Speed_Level_3 (50MHz)
	GPIOOType_TypeDef GPIO_OType;   //GPIO_OType_PP, GPIO_OType_OD
	GPIOPuPd_TypeDef GPIO_PuPd;     //GPIO_PuPd_NOPULL, GPIO_PuPd_UP, GPIO_PuPd_DOWN
} gpio_params_t;


void gpio_init();

void gpio_set(gpio_name_list name, uint16_t value);

void gpio_toggle(gpio_name_list name);

uint16_t gpio_read(gpio_name_list name);





#endif
