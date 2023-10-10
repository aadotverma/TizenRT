#pragma once

#include <time.h>
#include <semaphore.h>

typedef void *(*timer_callback)(void *);

/**
 * @brief function result constant
 *
 */
typedef enum {
	TIMER_FAIL = -1,			/**<  fail				*/
	TIMER_SUCCESS = 0,			/**<  success				*/
	TIMER_INVALID_ARGS = 1,		/**<  invalid parameter (argument)	*/
} timer_result;

struct timer{
    timer_t id;
    timer_callback function;
    void *function_args;
    unsigned int interval;
    bool enable;
    int signalReceivedCounter;
};

timer_result create_timer(timer *timer, void *timer_function, void *function_args, unsigned int interval);
timer_result timer_start(timer *timer);
timer_result timer_change_interval(timer *timer, unsigned int interval);
timer_result timer_stop(timer *timer);
timer_result timer_destroy(timer *timer);

