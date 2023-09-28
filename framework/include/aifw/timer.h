#pragma once

#include <time.h>
#include <semaphore.h>

typedef void *(*timer_callback)(void *);

/**
 * @brief function result constant
 *
 */
typedef enum {
	SYSTEM_FAIL = -1,			/**<  fail				*/
	SYSTEM_SUCCESS = 0,			/**<  success				*/
	SYSTEM_INVALID_ARGS = 1,		/**<  invalid parameter (argument)	*/
	SYSTEM_TIMEOUT = 2,			/**<  occur time out			*/
	SYSTEM_RESOURCE_BUSY = 3,		/**<  resource using another task	*/
	SYSTEM_TLS_CERT_VERIFY_FAIL =100,	/**< TLS Cert fail			*/
} system_result;

struct timer{
    timer_t id;
    timer_callback function;
    void *function_args;
    unsigned int interval;
    sem_t semaphore;
    bool enable;
};

system_result timerCreate(timer *timer, void *timer_function, void *function_args, unsigned int interval);
system_result timer_start(timer *timer);
system_result timer_change_interval(timer *timer, unsigned int interval);
system_result timer_stop(timer *timer);
system_result timer_destroy(timer *timer);
