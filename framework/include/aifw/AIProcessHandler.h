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
 * @file AIProcessHandler.h
 * @brief Interface class for data processing operations.
 */

#ifndef __AIPROCESS_HANDLER_H__
#define __AIPROCESS_HANDLER_H__

#include <memory>
#include "aifw/aifw.h"

namespace aifw {

class AIDataBuffer;

class AIProcessHandler
{
public:
	/**
	 * @brief: AIProcessHandler destructor.
	 */
	virtual ~AIProcessHandler()
	{
	}

	/**
	 * @brief Parses AI data from entire of raw data and writes it to the parsedData buffer.
	 * Data can be selected or new data can be created using different raw data values.
	 * @param [IN] data: Input raw data. It is entire of sensor data set, developer should parse data from it.
	 * @param [IN] count: Length of input raw data.
	 * @param [OUT] parsedData: Pointer of buffer to store postprocessed data. AIFW should write parsed data in AIDataBuffer.
	 * @param [IN] modelAttribute: Contains AIModelAttribute value of current AI Model.
	 * @return: AIFW_RESULT enum object. On success, AIFW_OK is returned.
	 * 			On failure, a negative value is returned.
	 */
	virtual AIFW_RESULT parseData(void *data, uint16_t count, float *parsedData, AIModelAttribute *modelAttribute) = 0;

	/**
	 * @brief Performs processing on data before invoke. Processed data will not be updated on AIDataBuffer.
	 * @param [IN] buffer: Pointer of AIDataBuffer. It contains parsed raw data.
	 * @param [OUT] invokeInput: Pointer of buffer to be invoked. Developer should fill with processed data.
	 * @param [IN] modelAttribute: Contains AIModelAttribute value of current AI Model.
	 * @return: AIFW_RESULT enum object. On success, AIFW_OK is returned.
	 * 			On failure, a negative value is returned.
	 */
	virtual AIFW_RESULT preProcessData(std::shared_ptr<AIDataBuffer> buffer, float *invokeInput, AIModelAttribute *modelAttribute) = 0;

	/**
	 * @brief Performs processing on data after invoke. Processed data will not be updated on AIDataBuffer.
	 * @param [IN] buffer: Pointer of AIDataBuffer. It contains invoked output.
	 * @param [OUT] resultData: Pointer of buffer to store postprocessed data.
	 * @param [IN] modelAttribute: Contains AIModelAttribute value of current AI Model.
	 * @return: AIFW_RESULT enum object. On success, AIFW_OK is returned.
	 * 			On completion of modelSet inference cycle, AIFW_INFERENCE_FINISHED is returned.
	 * 			On failure, a negative value is returned.
	 */
	virtual AIFW_RESULT postProcessData(std::shared_ptr<AIDataBuffer> buffer, float *resultData, AIModelAttribute *modelAttribute) = 0;
};

} /* namespace aifw */

#endif /* __AIPROCESS_HANDLER_H__ */
