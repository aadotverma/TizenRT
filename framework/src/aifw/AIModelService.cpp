/****************************************************************************
 *
 * Copyright 2023 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/

#include <stdlib.h>
#include <memory>
#include <semaphore.h>
#include "aifw/aifw.h"
#include "aifw/aifw_log.h"
#include "aifw/AIModelService.h"
#include "aifw/AIInferenceHandler.h"

#define AIFW_TIMER_SIGNAL 17

namespace aifw {

static sem_t gTimerSemaphore;
static int gSignalReceivedCounter = 0;
static timer_t gTimerId;

static void *aifw_timerthread_cb(void *parameter);

AIModelService::AIModelService(CollectRawDataListener collectRawDataCallback, std::shared_ptr<AIInferenceHandler> inferenceHandler) :
	mInterval(0), mServiceRunning(false), mInferenceHandler(inferenceHandler), mCollectRawDataCallback(collectRawDataCallback)
{
}

AIModelService::~AIModelService()
{
	AIFW_LOGV("model service object destoyed");
}

AIFW_RESULT AIModelService::start(void)
{
	if (mServiceRunning) {
		AIFW_LOGV("Service already running.");
		return AIFW_OK;
	}
	if (mInterval == 0) {
		mServiceRunning = true;
		return AIFW_OK;
	}
	AIFW_LOGV("Start Timer");
	int result = pthread_create(&mTimerThread, NULL, aifw_timerthread_cb, (void *)this);
	if (result != 0) {
		AIFW_LOGE("ERROR Failed to start aifw_timerthread_cb");
		return AIFW_ERROR;
	}
	AIFW_LOGV("Started aifw_timerthread_cb");
	mServiceRunning = true;
	return AIFW_OK;
}

AIFW_RESULT AIModelService::stop(void)
{
	if (!mServiceRunning) {
		AIFW_LOGV("Service already stopped.");
		return AIFW_OK;
	}
	if (mInterval == 0) {
		mServiceRunning = false;
		return AIFW_OK;
	}
	AIFW_LOGV("Stop Timer");
	if (sem_post(&gTimerSemaphore) != 0) {
		AIFW_LOGE("Timer stop failed, error: %d", errno);
		return AIFW_ERROR;
	}
	gSignalReceivedCounter = 0;
	AIFW_LOGV("Stop Timer posted");
	mServiceRunning = false;
	return AIFW_OK;
}

/* ToDo: Interval needs to be updated in json file so that updated value is used after device restarts */
AIFW_RESULT AIModelService::setInterval(uint16_t interval)
{
	if (interval <= 0) {
		AIFW_LOGE("Invalid interval=%d Ignoring request", interval);
		return AIFW_ERROR;
	}
	struct itimerspec timer;
	uint32_t interval_secs = interval / 1000;
	uint64_t interval_nsecs = (interval % 1000) * 1000000;
	AIFW_LOGV("setInterval: %d %d %lld", interval, interval_secs, interval_nsecs);
	timer.it_value.tv_sec = interval_secs;
	timer.it_value.tv_nsec = interval_nsecs;
	timer.it_interval.tv_sec = interval_secs;
	timer.it_interval.tv_nsec = interval_nsecs;
	int status = timer_settime(gTimerId, 0, &timer, NULL);
	if (status != OK) {
		AIFW_LOGE("setInterval: timer_settime failed, errno: %d", errno);
		return AIFW_ERROR;
	}
	mInterval = interval;
	return AIFW_OK;
}

AIFW_RESULT AIModelService::pushData(void *data, uint16_t count)
{
	if (!mServiceRunning) {
		AIFW_LOGE("Service not running");
		return AIFW_SERVICE_NOT_RUNNING;
	}
	return mInferenceHandler->pushData(data, count);
}

AIFW_RESULT AIModelService::prepare(void)
{
	AIFW_RESULT res = mInferenceHandler->prepare();
	if (res != AIFW_OK) {
		AIFW_LOGE("inference handler prepare api failed");
		return res;
	}
	mInterval = mInferenceHandler->getModelServiceInterval();
	AIFW_LOGV("Timer interval %d", mInterval);
	return AIFW_OK;
}

CollectRawDataListener AIModelService::getCollectRawDataCallback(void)
{
	return mCollectRawDataCallback;
}

uint32_t AIModelService::getTimerInterval(void)
{
	return mInterval;
}

void AIModelService::aifw_timer_cb(int signo, siginfo_t *info, void *ucontext)
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
	arg->getCollectRawDataCallback();
}

static void *aifw_timerthread_cb(void *parameter)
{
	sigset_t sigset;
	struct sigaction act;
	struct sigevent notify;
	struct itimerspec timer;
	int status;

	if (!parameter) {
		AIFW_LOGE("aifw_timerthread_cb: invalid argument");
		return NULL;
	}

	uint32_t interval = ((AIModelService*)parameter)->getTimerInterval();
	if (interval == 0) {
		AIFW_LOGE("aifw_timerthread_cb: invalid argument");
		return NULL;
	}
	uint32_t interval_secs = interval / 1000;
	uint64_t interval_nsecs = (interval % 1000) * 1000000;

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
	act.sa_sigaction = AIModelService::aifw_timer_cb;
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
	status = timer_create(CLOCK_REALTIME, &notify, &gTimerId);
	if (status != OK) {
		AIFW_LOGE("aifw_timerthread_cb: timer_create failed, errno=%d", errno);
		goto errorout;
	}

	/* Start the POSIX timer */
	AIFW_LOGV("aifw_timerthread_cb: Starting timer");
	AIFW_LOGV("aifw_timerthread_cb: %d %d %lld", interval, interval_secs, interval_nsecs);
	timer.it_value.tv_sec = interval_secs;
	timer.it_value.tv_nsec = interval_nsecs;
	timer.it_interval.tv_sec = interval_secs;
	timer.it_interval.tv_nsec = interval_nsecs;

	status = timer_settime(gTimerId, 0, &timer, NULL);
	if (status != OK) {
		AIFW_LOGE("aifw_timerthread_cb: timer_settime failed, errno=%d", errno);
		goto errorout;
	}
	AIFW_LOGV("aifw_timerthread_cb: exiting function");

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
	status = timer_delete(gTimerId);
	if (status != OK) {
		AIFW_LOGE("aifw_timerthread_cb: timer_delete failed, errno=%d", errno);
	}
	/* Detach the signal handler */
	act.sa_handler = SIG_DFL;
	status = sigaction(AIFW_TIMER_SIGNAL, &act, NULL);
	AIFW_LOGV("aifw_timerthread_cb: done");
	return NULL;
}

} /* namespace aifw */

