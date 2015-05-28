#include "event_timer.h"

#include "stm32f4xx.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_tim.h"
//#include "stm32f4xx_misc.h"
#include "gpio.h"


static char event_flags[FLAG_COUNT];         //1 = set, 0 = clear
static int event_flag_time[FLAG_COUNT];    //time passed since last flag clear, in microseconds.

static int absolute_time;					//time since beginning of program, in microseconds


void event_timer_init(){
    //initialize flags and times to 0.
	int i=0;
    for(i=0; i<FLAG_COUNT; i++){
        event_flags[i] = 0;
        event_flag_time[i] = 0;
    }

    absolute_time = 0;

	/*//I suspect that timers at high frequencies are vulnerable to unstable power supply. So here is the plan: we use two timers. One works at frequency 1000Hz (tick every millisecond) and it act as event timer. Another timer works at high frequency and provides time_us() and sleep() functions.

	//Initialize low frequency timer as event timer. TIM7

    //get APB1 frequency. It is then used to configure TIM7 prescaler, so that it counts every millisecond.
    RCC_ClocksTypeDef clock;
    RCC_GetClocksFreq(&clock);
    int prescaler = clock.PCLK1_Frequency*EVENT_TIMER_PERIOD_MS/(1000);

    //now initialize timer. It will trigger an interrupt every 1ms. In interrupt times will be added and flags will be set.
    TIM_TimeBaseInitTypeDef TIM_InitStructure;
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM7,ENABLE);

	TIM_InitStructure.TIM_Prescaler = prescaler;
	TIM_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_InitStructure.TIM_Period = EVENT_TIMER_PERIOD_MS;
	TIM_InitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInit(TIM7, &TIM_InitStructure);

	//set up TIM7 global interrupt, will be used for update events
    NVIC_InitTypeDef NVIC_init_structure;
    NVIC_init_structure.NVIC_IRQChannel = TIM7_IRQn;
    NVIC_init_structure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_init_structure.NVIC_IRQChannelSubPriority = 1;
    NVIC_init_structure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_init_structure);
    //enable update interrupt for TIM7
    TIM_ITConfig(TIM7, TIM_IT_Update, ENABLE);

	TIM_Cmd(TIM7, ENABLE);

	//Now initialize TIM6 as high frequency timer
    prescaler = clock.PCLK1_Frequency*2/1000000;		//I dont know why we must divide by 2, but it seems that actual frequency is two times larger than 16MHz. But why dont we do it with TIM6?

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6,ENABLE);

	TIM_InitStructure.TIM_Prescaler = prescaler;
	TIM_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_InitStructure.TIM_Period = 1000;
	TIM_InitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInit(TIM6, &TIM_InitStructure);

	//set up TIM6 global interrupt
    NVIC_init_structure.NVIC_IRQChannel = TIM6_DAC_IRQn;
    NVIC_init_structure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_init_structure.NVIC_IRQChannelSubPriority = 1;
    NVIC_init_structure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_init_structure);
    //enable update interrupt for TIM6
    TIM_ITConfig(TIM6, TIM_IT_Update, ENABLE);

	TIM_Cmd(TIM6, ENABLE);*/

}

void event_timer_increment(){
	int i = 0;
	for (i=0; i<FLAG_COUNT; i++){
		event_flag_time[i] += EVENT_TIMER_PERIOD_MS;
		if (event_flag_time[i] >= event_flag_periods[i]){
			event_flags[i] = EVENT_FLAG_SET;
			event_flag_time[i] = 0;
		}
	}
}

void TIM7_IRQHandler(){
    //increment time of flags, set them if necessary
    /*for (int i=0; i<FLAG_COUNT; i++){
        event_flag_time[i] += EVENT_TIMER_PERIOD_MS;
        if (event_flag_time[i] >= event_flag_periods[i]){
            event_flags[i] = EVENT_FLAG_SET;
            event_flag_time[i] = 0;
        }
    }

    //clear interrupt flag
    TIM7->SR &= ~1;*/
}

void TIM6_DAC_IRQHandler(){
	/*absolute_time += 1000;
	TIM6->SR &= ~1;			//clear interrupt flag*/
}


void event_set_flag(EVENT_FLAG flag_id){
    event_flags[flag_id] = EVENT_FLAG_SET;
}

void event_clear_flag(EVENT_FLAG flag_id){
    event_flags[flag_id] = EVENT_FLAG_CLEAR;
}


int event_flag_get(EVENT_FLAG flag_id){
    return event_flags[flag_id];
}

int event_flag_is_set(EVENT_FLAG flag_id){
    if (event_flags[flag_id] == EVENT_FLAG_SET){
        return event_flags[flag_id]--;
    }
    return EVENT_FLAG_CLEAR;
}

//produces a delay approximately equal to time_ms
int delay_ms(int time_ms){
	int j;
	for (j=0; j<time_ms*12500; ++j) {}
}

/*uint64_t time_us() {
	return absolute_time + TIM7->CNT;
}

void sleep_ms(uint32_t time) {
	uint64_t wakeup_time = time_us() + ((uint64_t) time)*1000;
	while (wakeup_time > time_us()) {}
}
void sleep_us(uint32_t time) {
	uint64_t wakeup_time = time_us() + time;
	while (wakeup_time > time_us()) {}
}*/






