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
 * @file AIInferenceHandler.h
 */

#ifndef __AIINFERENCE_HANDLER_H__
#define __AIINFERENCE_HANDLER_H__

#include <stdlib.h>
#include "aifw/aifw.h"
#include "aifw/AIModel.h"

namespace aifw {

class AIInferenceHandler
{
public:
	/**
	 * @brief Constructs the AIInferenceHandler class instance.
	 * @param [IN] countOfModels: Number of models attached with this AIInferenceHandler instance.
	 * @param [IN] listener: Callback function to call when inference cycle of attached models completes.
	 */
	AIInferenceHandler(uint16_t countOfModels, InferenceResultListener listener);

	/**
	 * @brief AIInferenceHandler destructor
	 */
	virtual ~AIInferenceHandler()
	{
	}

	/**
	 * @brief Pushes input raw data to all attached models for data processing and invoke.
	 * @param [IN] data: Input sensor data.
	 * @param [IN] count: Length of input sensor data array.
	 * @return: AIFW_RESULT enum object.
	 */
	AIFW_RESULT pushData(void *data, uint16_t count);

	/**
	 * @brief Gives inference interval of attached models.
	 * @return: Negative value indicates an error. Non negative value tells inference interval of attached models.
	 */
	uint16_t getModelServiceInterval(void);

	/**
	 * @brief creates instances of AIModel and Process Handler(if required).
	 * Finally models are loaded and attached to AIInferenceHandler.
	 * @return: AIFW_RESULT enum object.
	 */
	virtual AIFW_RESULT prepare(void) = 0;

protected:
	/**
	 * @brief Performs operations on post processed results of attached models in the model set.
	 * This function will be called always when inference cycle of a model set is finished successfully.
	 * @param [IN] idx: Index of last model till which inference is performed.
	 * @param [OUT] finalResult: Ensembled result of modelSet.
	 * @return: AIFW_RESULT enum object.
	 */
	virtual AIFW_RESULT onInferenceFinished(uint16_t idx, void *finalResult) = 0;

	/**
	 * @brief Attaches model to model list of AIInferenceHandler.
	 * @param [IN] model: Pointer of model to be attached.
	 * @return: AIFW_RESULT enum object.
	 */
	void attachModel(std::shared_ptr<AIModel> model);

private:
	uint16_t mMaxModelsCount;
	uint16_t mModelIndex;
	std::shared_ptr<std::shared_ptr<AIModel>> mModels;
	InferenceResultListener mInferenceResultListener;
};

} /* namespace aifw */

#endif /* __AIINFERENCE_HANDLER_H__ */
