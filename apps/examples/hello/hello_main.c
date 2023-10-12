/****************************************************************************
 *
 * Copyright 2016 Samsung Electronics All Rights Reserved.
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
/****************************************************************************
 * examples/hello/hello_main.c
 *
 *   Copyright (C) 2008, 2011-2012 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <tinyara/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#define BUFFER_SIZE 5
#define MAX_ITEMS 20

int sharedBuffer[BUFFER_SIZE];
int producingIndex;
int consumingIndex;
int produced_count;
int consumed_count;

sem_t semaphore;
sem_t consumerSemaphore;
sem_t producerSemaphore;

void *producer(void *arg)
{
	int itemNumber = 1;

	while (produced_count < MAX_ITEMS) {
		sem_wait(&producerSemaphore);
		sem_wait(&semaphore);

		sharedBuffer[producingIndex] = itemNumber;
		printf("Produced item number: %d\n", itemNumber);
		itemNumber++;
		producingIndex = (producingIndex + 1) % BUFFER_SIZE;

		produced_count++;

		sem_post(&semaphore);
		sem_post(&consumerSemaphore);
	}

	pthread_exit(NULL);
}

void *consumer(void *arg)
{
	while (consumed_count < MAX_ITEMS) {
		sem_wait(&consumerSemaphore);
		sem_wait(&semaphore);

		int itemNumber = sharedBuffer[consumingIndex];
		printf("Consumed item number: %d\n", itemNumber);
		consumingIndex = (consumingIndex + 1) % BUFFER_SIZE;

		consumed_count++;

		sem_post(&semaphore);
		sem_post(&producerSemaphore);
	}

	pthread_exit(NULL);
}

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int hello_main(int argc, char *argv[])
#endif
{
	producingIndex = 0;
	consumingIndex = 0;
	produced_count = 0;
	consumed_count = 0;

	pthread_t producerThread, consumerThread;

	sem_init(&semaphore, 0, 1);
	sem_init(&consumerSemaphore, 0, 0);
	sem_init(&producerSemaphore, 0, BUFFER_SIZE);

	pthread_create(&producerThread, NULL, producer, NULL);
	pthread_create(&consumerThread, NULL, consumer, NULL);

	pthread_join(producerThread, NULL);
	pthread_join(consumerThread, NULL);

	sem_destroy(&semaphore);
	sem_destroy(&consumerSemaphore);
	sem_destroy(&producerSemaphore);

	return 0;
}
