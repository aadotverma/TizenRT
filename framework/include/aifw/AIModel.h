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

#ifndef __AIFW_AIMODEL_H__
#define __AIFW_AIMODEL_H__

#include <memory>
#include "aifw/aifw.h"

namespace aifw {

class AIEngine;
class AIDataBuffer;
class AIProcessHandler;

class AIModel
{
public:
	/**
	 * @brief Constructs the AIModel class instance.
	 */
	AIModel(void);

	/**
	 * @brief (Parameterized) Constructs the AIModel class when data processor is provided.
	 * @param [IN] dataProcessor: Pointer of AIProcessHandler to handle/process data before and after invoke.
	 */
	AIModel(std::shared_ptr<AIProcessHandler> dataProcessor);

	/**
	 * @brief AIModel class destructor.
	 */
	~AIModel();

	/**
	 * @brief Load the TFLITE model from a file.
	 * @param [IN] scriptPath: Path of TFLITE model file.
	 * @return: AIFW_RESULT enum object.
	 */
	AIFW_RESULT loadModel(const char *scriptPath);

	/**
	 * @brief Load array based model.
	 * @param [IN] modelAttribute: Model attributes.
	 * @return: AIFW_RESULT enum object.
	 */
	AIFW_RESULT loadModel(const AIModelAttribute &modelAttribute);

	/**
	 * @brief Pushes incoming raw data for processing and invoke.
	 * @param [IN] data: Incoming sensor data array.
	 * @param [IN] count: Size of sensor data array.
	 * @return: AIFW_RESULT enum object.
	 */
	AIFW_RESULT pushData(void *data, uint16_t count);

	/**
	 * @brief Get AIModelAttribute corresponding to the particular model.
	 * @return: Returns AIModelAttribute value of model.
	 */
	AIModelAttribute getModelAttribute(void);

	/**
	 * @brief Get result data of last inference cycle of model.
	 * @param [OUT] data: Pointer of buffer to fill the inference result.
	 * @param [IN] count: Number of values that buffer can hold.
	 * @return: AIFW_RESULT enum object.
	 */
	AIFW_RESULT getResultData(float *data, uint16_t count);

	/**
	 * @brief Get latest parsed raw data stored in AIDataBuffer.
	 * @param [OUT] data: Pointer of buffer to fill latest parsed raw data.
	 * @param [IN] count: Number of values that buffer can hold.
	 * @return: AIFW_RESULT enum object.
	 */
	AIFW_RESULT getRawData(float *data, uint16_t count);

private:
	/**
	 * @brief Creates and initializes buffer corresponding to the model.
	 * @return: AIFW_RESULT enum object.
	 */
	AIFW_RESULT createDataBuffer(void);

	/**
	 * @brief Process data and run the inference on processed data.
	 * @return: AIFW_RESULT enum object.
	 */
	AIFW_RESULT invoke(void);

	/**
	 * @brief Set model attribute using AIManifestParser class to parse file from given path.
	 * @param [IN] path: Manifest file path.
	 * @return: AIFW_RESULT enum object.
	 */
	AIFW_RESULT setModelAttributes(const char *path);

	/**
	 * @brief Clears memory allocated to members of AIModelAttribute structure.
	 */
	void clearModelAttribute(void);

	/**
	 * @brief Allocates memory to buffers required to store data at different stages of inference.
	 * @return: AIFW_RESULT enum object.
	 */
	AIFW_RESULT allocateMemory(void);

	AIFW_RESULT fillModelAttribute(const AIModelAttribute &modelAttribute);

	AIModelAttribute mModelAttribute;
	std::shared_ptr<AIDataBuffer> mBuffer;
	std::shared_ptr<AIEngine> mAIEngine;
	float *mInvokeInput;
	float *mInvokeOutput;
	float *mParsedData;
	float *mPostProcessedData;
	std::shared_ptr<AIProcessHandler> mDataProcessor;
};

} /* namespace aifw */

#endif /* __AIFW_AIMODEL_H__ */
