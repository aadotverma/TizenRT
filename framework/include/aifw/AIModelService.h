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

/**
 * @file aifw/AIModelService.h
 * @brief AIModelService class uses TizenRT software timer to invoke data request at a set interval.
 */

#pragma once

#include <memory>
#include "system/system_timer.h"
#include "aifw/aifw.h"
#include "aifw/AIInferenceHandler.h"

namespace aifw {

class AIModelService
{
public:
	/**
	 * @brief Constructs the AIModelService class instance.
	 * @param [IN] collectRawDataCallback: Callback function to call when timer expires to collect raw data.
	 * @param [IN] inferenceHandler: Associated AIInferenceHandler class object.
	 */
	AIModelService(CollectRawDataListener collectRawDataCallback, std::shared_ptr<AIInferenceHandler> inferenceHandler);

	/**
	 * @brief AIModelService class destructor.
	 */
	~AIModelService();

	/**
	 * @brief Changes the time interval of the system timer.
	 * @param [IN] interval: set interval time in milliseconds.
	 * @return: AIFW_RESULT enum object.
	 */
	AIFW_RESULT setInterval(uint16_t interval);

	/**
	 * @brief Creates and starts the system timer.
	 * @return: AIFW_RESULT enum object.
	 */
	AIFW_RESULT start(void);

	/**
	 * @brief Stops the system timer and destroys it.
	 * @return: AIFW_RESULT enum object.
	 */
	AIFW_RESULT stop(void);

	/**
	 * @brief Pushes the incoming raw data to AIInferenceHandler. AIInferenceHandler then pushes raw data to the models attached to itself.
	 * @param [IN] data: Incoming sensor data.
	 * @param [IN] count: Length of incoming sensor data array.
	 * @return: AIFW_RESULT enum object.
	 */
	AIFW_RESULT pushData(void *data, uint16_t count);

	/**
	 * @brief It calls prepare API of AIInferenceHandler which creates instances of AIModel and Process Handler(if required).
	 * Finally models are loaded and attached to AIInferenceHandler.
	 * @return: AIFW_RESULT enum object.
	 */
	AIFW_RESULT prepare(void);

	/**
	 * @brief Gives the timer expired callback which collects the raw data from the data source.
	 * @return: Returns timer expired callback.
	 */
	CollectRawDataListener getCollectRawDataCallback(void);

	/**
	 * @brief Function to be called by sytem timer every time interval it expires.
	 * Internally gives callback to application when timer expires.
	 * @param [IN] args: AIModelService class object.
	 */
	static void timerTaskHandler(void *args);

private:
	/**
	 * @brief Deletes the timer and frees memory of system timer object.
	 * @return: AIFW_RESULT enum object.
	 */
	AIFW_RESULT freeTimer(void);

	uint16_t mInterval;
	bool mServiceRunning;
	std::shared_ptr<AIInferenceHandler> mInferenceHandler;
	CollectRawDataListener mCollectRawDataCallback;
	system_timer *mTimer;
};

} /* namespace aifw */
