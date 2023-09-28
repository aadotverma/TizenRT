#include <semaphore.h>
#include <signal.h>
#include "aifw/timer.h"
#include "aifw/aifw_log.h"
#include "aifw/AIModelService.h"

#define AIFW_TIMER_SIGNAL 17

using namespace aifw;

static sem_t gTimerSemaphore;
static int gSignalReceivedCounter = 0;
static bool enable_init = false;

static void aifw_timer_cb(int signo, siginfo_t *info, void *ucontext);
static void *aifw_timerthread_cb(void *args);

system_result timerCreate(timer *timer, void *timer_function, void *function_args, unsigned int interval)
{
	if (timer == NULL || timer_function == NULL || interval == 0 || interval > 0x7FFFFFFF) {
		return SYSTEM_INVALID_ARGS;
	}

	sigset_t sigset;
	struct sigaction act;
	struct sigevent notify;
	int status;

	AIFW_LOGV("aifw_timerthread_cb: Initializing semaphore to 0");
	sem_init(&gTimerSemaphore, 0, 0);

	/* Start waiter thread  */
	AIFW_LOGV("aifw_timerthread_cb: Unmasking signal %d", AIFW_TIMER_SIGNAL);
	(void)sigemptyset(&sigset);
	(void)sigaddset(&sigset, AIFW_TIMER_SIGNAL);
	status = sigprocmask(SIG_UNBLOCK, &sigset, NULL);
	if (status != OK) {
		AIFW_LOGE("aifw_timerthread_cb: ERROR sigprocmask failed, status=%d", status);
		return SYSTEM_FAIL;
	}

	AIFW_LOGV("aifw_timerthread_cb: Registering signal handler");
	act.sa_sigaction = aifw_timer_cb;
	act.sa_flags = SA_SIGINFO;

	(void)sigfillset(&act.sa_mask);
	(void)sigdelset(&act.sa_mask, AIFW_TIMER_SIGNAL);
	status = sigaction(AIFW_TIMER_SIGNAL, &act, NULL);
	if (status != OK) {
		AIFW_LOGE("aifw_timerthread_cb: ERROR sigaction failed, status=%d", status);
		return SYSTEM_FAIL;
	}

	/* Fill value in timer structure */
	timer->function = (timer_callback)timer_function;
	timer->function_args = function_args;
	timer->interval = interval;
	timer->enable = false;

	/* Create the POSIX timer */
	AIFW_LOGV("aifw_timerthread_cb: Creating timer");
	notify.sigev_notify = SIGEV_SIGNAL;
	notify.sigev_signo = AIFW_TIMER_SIGNAL;
	notify.sigev_value.sival_ptr = function_args;
	status = timer_create(CLOCK_REALTIME, &notify, &(timer->id));
	if (status != OK) {
		AIFW_LOGE("aifw_timerthread_cb: timer_create failed, errno=%d", errno);
		goto errorout;
	}
	return SYSTEM_SUCCESS;

errorout:
	AIFW_LOGV("sem_destroy");
	sem_destroy(&gTimerSemaphore);
	/* Then delete the timer */
	AIFW_LOGV("aifw_timerthread_cb: Deleting timer");
	status = timer_delete(timer->id);
	if (status != OK) {
		AIFW_LOGE("aifw_timerthread_cb: timer_delete failed, errno=%d", errno);
	}
	/* Detach the signal handler */
	act.sa_handler = SIG_DFL;
	status = sigaction(AIFW_TIMER_SIGNAL, &act, NULL);
	AIFW_LOGV("aifw_timerthread_cb: done");
	return SYSTEM_FAIL;
}

system_result timer_start(timer *timer)
{
	if (timer == NULL || timer->id == NULL || timer->function == NULL) {
		return SYSTEM_INVALID_ARGS;
	}
	struct itimerspec its;
	/* Start the POSIX timer */
	uint32_t interval_secs = timer->interval / 1000;
	uint64_t interval_nsecs = (timer->interval % 1000) * 1000000;
	AIFW_LOGV("aifw_timerthread_cb: Starting timer");
	AIFW_LOGV("aifw_timerthread_cb: %d %d %lld", timer->interval, interval_secs, interval_nsecs);
	its.it_value.tv_sec = interval_secs;
	its.it_value.tv_nsec = interval_nsecs;
	its.it_interval.tv_sec = interval_secs;
	its.it_interval.tv_nsec = interval_nsecs;

	int status = timer_settime(timer->id, 0, &its, NULL);
	if (status != OK) {
		AIFW_LOGE("aifw_timerthread_cb: timer_settime failed, errno=%d", errno);
		return SYSTEM_FAIL;
	}
	AIFW_LOGV("aifw_timerthread_cb: exiting function");	
	if (enable_init == false) {
		pthread_t timerThread;
		int status = pthread_create(&timerThread, NULL, aifw_timerthread_cb, NULL);
		if (status != 0) {
			AIFW_LOGE("ERROR Failed to start aifw_timerthread_cb");
			return SYSTEM_FAIL;
		}
		AIFW_LOGV("Started aifw_timerthread_cb");
		enable_init = true;
	}
	return SYSTEM_SUCCESS;
}

system_result timer_change_interval(timer *timer, unsigned int interval)
{
	timer->interval = interval;

	system_result result = SYSTEM_SUCCESS;

	result = timer_stop(timer);
	if (result != SYSTEM_SUCCESS) {
		return result;
	}

	result = timer_start(timer);
	return result;
}


system_result timer_stop(timer *timer)
{
	AIFW_LOGV("Stop Timer");
	if (sem_post(&gTimerSemaphore) != 0) {
		AIFW_LOGE("Timer stop failed, error: %d", errno);
		return SYSTEM_FAIL;
	}
	gSignalReceivedCounter = 0;
	AIFW_LOGV("Stop Timer posted");	
	return SYSTEM_SUCCESS;
}

system_result timer_destroy(timer *timer)
{
	if (timer == NULL) {
		return SYSTEM_INVALID_ARGS;
	}
	system_result res = SYSTEM_SUCCESS;
	int status;
	AIFW_LOGV("Destroying semaphore");
	status = sem_destroy(&gTimerSemaphore);
	if (status != OK) {
		AIFW_LOGE("Destroying semaphore failed. errno: %d", errno);
		res = SYSTEM_FAIL;
	}

	AIFW_LOGV("Deleting timer");
	status = timer_delete(timer->id);
	if (status != OK) {
		AIFW_LOGE("timer_delete failed, errno=%d", errno);
		res = SYSTEM_FAIL;
	}
	return res;	
}

static void *aifw_timerthread_cb(void *args)
{
	int status;
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
	AIModelService *arg = (AIModelService *)info->si_value.sival_ptr;
	if (!arg) {
		AIFW_LOGE("null argument received");
		return;
	}
	(arg->getCollectRawDataCallback())();
}
