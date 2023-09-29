#include <semaphore.h>
#include <signal.h>
#include <memory>
#include <errno.h>
#include "aifw/timer.h"
#include "aifw/aifw_log.h"

#define AIFW_TIMER_SIGNAL 17

static sem_t gTimerSemaphore;
static int gSignalReceivedCounter = 0;

static void aifw_timer_cb(int signo, siginfo_t *info, void *ucontext);
static void *aifw_timerthread_cb(void *parameter);

system_result timerCreate(timer *timer, void *timer_function, void *function_args, unsigned int interval)
{
	if (timer == NULL || timer_function == NULL || interval == 0 || interval > 0x7FFFFFFF) {
		return SYSTEM_INVALID_ARGS;
	}
	/* Fill value in timer structure */
	timer->function = (timer_callback)timer_function;
	timer->function_args = function_args;
	timer->interval = interval;
	timer->enable = false;
	return SYSTEM_SUCCESS;
}

system_result timer_start(timer *timer)
{
	if (timer == NULL/*|| timer->id == NULL*/ || timer->function == NULL) {
		return SYSTEM_INVALID_ARGS;
	}
	AIFW_LOGV("Start Timer");
	pthread_t timerThread;
	int result = pthread_create(&timerThread, NULL, aifw_timerthread_cb, (void *)timer);
	if (result != 0) {
		AIFW_LOGE("ERROR Failed to start aifw_timerthread_cb");
		return SYSTEM_FAIL;
	}
	AIFW_LOGV("Started aifw_timerthread_cb");
	return SYSTEM_SUCCESS;
}

system_result timer_change_interval(timer *timer, unsigned int interval)
{
	if (interval <= 0) {
		AIFW_LOGE("Invalid argument interval: %d", interval);
		return SYSTEM_INVALID_ARGS;
	}
	timer->interval = interval;
	if (!timer->enable) {
		AIFW_LOGV("Timer not started/enabled yet");
		return timer_start(timer);
	}
	struct itimerspec its;
	uint32_t interval_secs = interval / 1000;
	uint64_t interval_nsecs = (interval % 1000) * 1000000;
	AIFW_LOGV("setInterval: %d %d %lld", interval, interval_secs, interval_nsecs);
	its.it_value.tv_sec = interval_secs;
	its.it_value.tv_nsec = interval_nsecs;
	its.it_interval.tv_sec = interval_secs;
	its.it_interval.tv_nsec = interval_nsecs;
	int status = timer_settime(timer->id, 0, &its, NULL);
	if (status != OK) {
		AIFW_LOGE("setInterval: timer_settime failed, errno: %d", errno);
		return SYSTEM_FAIL;
	}
	return SYSTEM_SUCCESS;
}


system_result timer_stop(timer *timer)
{
	AIFW_LOGV("Stop Timer");
	if (sem_post(&gTimerSemaphore) != 0) {
		AIFW_LOGE("Timer stop failed, error: %d", errno);
		return SYSTEM_FAIL;
	}
	timer->enable = false;
	gSignalReceivedCounter = 0;
	AIFW_LOGV("Stop Timer posted");	
	return SYSTEM_SUCCESS;
}

system_result timer_destroy(timer *timer)
{
	if (timer == NULL) {
		return SYSTEM_INVALID_ARGS;
	}
	memset(timer, 0, sizeof(struct::timer));
	return SYSTEM_SUCCESS;
}

static void *aifw_timerthread_cb(void *parameter)
{
	sigset_t sigset;
	struct sigaction act;
	struct sigevent notify;
	struct itimerspec its;
	timer_t timerId;
	int status;

	if (!parameter) {
		AIFW_LOGE("aifw_timerthread_cb: invalid argument");
		return NULL;
	}

	uint32_t interval_secs = ((timer *)parameter)->interval / 1000;
	uint64_t interval_nsecs = (((timer *)parameter)->interval % 1000) * 1000000;

	AIFW_LOGV("aifw_timerthread_cb: Initializing semaphore to 0");
	sem_init(&gTimerSemaphore, 0, 0);

	/* Start waiter thread  */
	AIFW_LOGV("aifw_timerthread_cb: Unmasking signal %d", AIFW_TIMER_SIGNAL);
	(void)sigemptyset(&sigset);
	(void)sigaddset(&sigset, AIFW_TIMER_SIGNAL);
	status = sigprocmask(SIG_UNBLOCK, &sigset, NULL);
	if (status != OK) {
		AIFW_LOGE("aifw_timerthread_cb: ERROR sigprocmask failed, status=%d", status);
		return NULL;
	}

	AIFW_LOGV("aifw_timerthread_cb: Registering signal handler");
	act.sa_sigaction = aifw_timer_cb;
	act.sa_flags = SA_SIGINFO;

	(void)sigfillset(&act.sa_mask);
	(void)sigdelset(&act.sa_mask, AIFW_TIMER_SIGNAL);
	status = sigaction(AIFW_TIMER_SIGNAL, &act, NULL);
	if (status != OK) {
		AIFW_LOGE("aifw_timerthread_cb: ERROR sigaction failed, status=%d", status);
		return NULL;
	}

	/* Create the POSIX timer */
	AIFW_LOGV("aifw_timerthread_cb: Creating timer");
	notify.sigev_notify = SIGEV_SIGNAL;
	notify.sigev_signo = AIFW_TIMER_SIGNAL;
	notify.sigev_value.sival_ptr = parameter;
	status = timer_create(CLOCK_REALTIME, &notify, &(((timer *)parameter)->id));
	timerId = ((timer *)parameter)->id;
	if (status != OK) {
		AIFW_LOGE("aifw_timerthread_cb: timer_create failed, errno=%d", errno);
		goto errorout;
	}

	/* Start the POSIX timer */
	AIFW_LOGV("aifw_timerthread_cb: Starting timer");
	AIFW_LOGV("aifw_timerthread_cb: %d %d %lld", ((timer *)parameter)->interval, interval_secs, interval_nsecs);
	its.it_value.tv_sec = interval_secs;
	its.it_value.tv_nsec = interval_nsecs;
	its.it_interval.tv_sec = interval_secs;
	its.it_interval.tv_nsec = interval_nsecs;

	status = timer_settime(((timer *)parameter)->id, 0, &its, NULL);
	if (status != OK) {
		AIFW_LOGE("aifw_timerthread_cb: timer_settime failed, errno=%d", errno);
		goto errorout;
	}
	AIFW_LOGV("aifw_timerthread_cb: exiting function");
	((timer *)parameter)->enable = true;
	/* Take the semaphore */
	while (1) {
		AIFW_LOGV("aifw_timerthread_cb: Waiting on semaphore");
		status = sem_wait(&gTimerSemaphore);
		if (status != 0) {
			int error = errno;
			if (error == EINTR) {
				AIFW_LOGV("aifw_timerthread_cb: sem_wait() successfully interrupted by signal");
			} else {
				AIFW_LOGE("aifw_timerthread_cb: ERROR sem_wait failed, errno=%d", error);
			}
		} else {
			AIFW_LOGI("aifw_timerthread_cb: ERROR awakened with no error!");
			break;
		}
		AIFW_LOGV("aifw_timerthread_cb: Signal received counter: %d", gSignalReceivedCounter);
	}
errorout:
	AIFW_LOGV("sem_destroy");
	sem_destroy(&gTimerSemaphore);
	/* Then delete the timer */
	AIFW_LOGV("aifw_timerthread_cb: Deleting timer");
	status = timer_delete(timerId);
	if (status != OK) {
		AIFW_LOGE("aifw_timerthread_cb: timer_delete failed, errno=%d", errno);
	}
	/* Detach the signal handler */
	act.sa_handler = SIG_DFL;
	status = sigaction(AIFW_TIMER_SIGNAL, &act, NULL);
	AIFW_LOGV("aifw_timerthread_cb: done");
	return NULL;
}

static void aifw_timer_cb(int signo, siginfo_t *info, void *ucontext)
{
	AIFW_LOGV("aifw_timer_cb: Received signal %d", signo);
	if (signo != AIFW_TIMER_SIGNAL) {
		AIFW_LOGE("aifw_timer_cb: ERROR expected signo: %d", AIFW_TIMER_SIGNAL);
		return;
	}
	if (info->si_signo != AIFW_TIMER_SIGNAL) {
		AIFW_LOGE("aifw_timer_cb: ERROR expected si_signo=%d, got=%d", AIFW_TIMER_SIGNAL, info->si_signo);
		return;
	}
	if (info->si_code != SI_TIMER) {
		AIFW_LOGE("aifw_timer_cb: ERROR si_code=%d, expected SI_TIMER=%d", info->si_code, SI_TIMER);
		return;
	}
	AIFW_LOGV("aifw_timer_cb: si_code=%d (SI_TIMER)", info->si_code);
	gSignalReceivedCounter++;
	timer *timer = (struct::timer *)info->si_value.sival_ptr;
	timer->function(timer->function_args);
}
