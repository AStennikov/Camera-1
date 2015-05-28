//This code allows allows to create flags which are set after specified time has passed.
//USING IT:
//write flag names in EVENT_FLAG
//write flag periods in event_flag_periods.
//you may change EVENT_TIMER_PERIOD_MS. Changing it was not tested, do it at your own risk.
//in your program, start event timer with event_timer_init()
//then you can get flag values with event_flag_get() or event_flag_is_set() functions
//you can set and clear flag with event_set_flag() and event_clear_flag()


//TODO:
//EVENT_TIMER_PERIOD_MS - test different values
//find max and min allowed times
//perhaps if setting flag period to 0, only user can update it, not timer?


#define EVENT_FLAG_CLEAR 0
#define EVENT_FLAG_SET 1

#define EVENT_TIMER_PERIOD_MS 1


typedef enum {
    FLAG_LED_MCU,

    FLAG_COUNT,
}EVENT_FLAG;


//specify how often flags must be set. In milliseconds. Max time is 60000 ms
static int event_flag_periods[FLAG_COUNT] = {
    200,

};


void event_timer_init();

//for use in sys_tick timer
void event_timer_increment();

void event_set_flag(EVENT_FLAG flag_id);

void event_clear_flag(EVENT_FLAG flag_id);

//returns flag status. Flag is not cleared automatically
int event_flag_get(EVENT_FLAG flag_id);

//returns 1 if flag is set, 0 otherwise. Using this function clears flag automatically.
int event_flag_is_set(EVENT_FLAG flag_id);

//produces a delay approximately equal to time_ms
int delay_ms(int time_ms);

/*//returns time since start of program. In microseconds
uint64_t time_us();

void sleep_ms(uint32_t time);

void sleep_us(uint32_t time);*/





