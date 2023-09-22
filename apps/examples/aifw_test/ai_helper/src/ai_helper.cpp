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

#include <memory>
#include "aifw/aifw_log.h"
#include "aifw/AIModelService.h"
#include "aifw/AIModel.h"
#include "aifw/AIProcessHandler.h"
#include "aifw/AIInferenceHandler.h"
#include "inc/ai_helper.h"
#include "inc/SineWaveInferenceHandler.h"
using namespace aifw;

/**
 * @brief ModelSetInfo keeps pointers to various model related objects and  variables.
 */
struct ModelSetInfo {
	uint32_t modelCode;
	std::shared_ptr<AIInferenceHandler> aiInferenceHandler;
	std::shared_ptr<AIModelService> aiModelService;
};

template <typename T>
struct arrayDeleter {
	void operator()(T const *p)
	{
		delete[] p;
	}
};

static std::shared_ptr<ModelSetInfo> gModelSetList;
static uint16_t gModelSetListOffset = 0;
static uint16_t gMaxModelSetCount = 0;

static int16_t findModelSetInfoIndex(uint32_t modelCode)
{
	for (int i = 0; i < gModelSetListOffset; i++) {
		if (gModelSetList.get()[i].modelCode == modelCode)
			return i;
	}
	return -1;
}

AIFW_RESULT ai_helper_init(uint16_t maxModelSetCount)
{
	gMaxModelSetCount = maxModelSetCount;
	std::shared_ptr<ModelSetInfo> modelSetList(new ModelSetInfo[gMaxModelSetCount], arrayDeleter<ModelSetInfo>());
	if (!modelSetList) {
		AIFW_LOGE("model set list Memory Allocation failed.");
		return AIFW_NO_MEM;
	}
	gModelSetList = modelSetList;
	return AIFW_OK;
}

/* App to call this function */
AIFW_RESULT ai_helper_deinit(void)
{
	return AIFW_OK;
}

AIFW_RESULT ai_helper_load_model(uint32_t modelCode, InferenceResultListener resultCallback, CollectRawDataListener collectRawDataCallback)
{
	int index = findModelSetInfoIndex(modelCode);
	if (index != -1) {
		AIFW_LOGE("Model set with modelCode %d loaded already", modelCode);
		return AIFW_OK;
	}
	gModelSetList.get()[gModelSetListOffset].aiInferenceHandler = std::make_shared<SineWaveInferenceHandler>(resultCallback);
	if (!gModelSetList.get()[gModelSetListOffset].aiInferenceHandler) {
		AIFW_LOGE("Model code not suppported or Memory allocation failed for aiInferenceHandler");
		return AIFW_ERROR;
	}
	gModelSetList.get()[gModelSetListOffset].aiModelService = std::make_shared<AIModelService>(collectRawDataCallback, gModelSetList.get()[gModelSetListOffset].aiInferenceHandler);
	if (!gModelSetList.get()[gModelSetListOffset].aiModelService) {
		gModelSetList.get()[gModelSetListOffset].aiInferenceHandler = nullptr;
		AIFW_LOGE("Memory allocation failed for aiModelService");
		return AIFW_NO_MEM;
	}
	AIFW_RESULT res = gModelSetList.get()[gModelSetListOffset].aiModelService->prepare();
	if (res != AIFW_OK) {
		gModelSetList.get()[gModelSetListOffset].aiInferenceHandler = nullptr;
		gModelSetList.get()[gModelSetListOffset].aiModelService = nullptr;
		AIFW_LOGE("AI model service prepare api failed");
		return res;
	}
	gModelSetList.get()[gModelSetListOffset].modelCode = modelCode;
	gModelSetListOffset++;
	return AIFW_OK;
}

AIFW_RESULT ai_helper_start(uint32_t modelCode)
{
	int index = findModelSetInfoIndex(modelCode);
	if (index == -1) {
		AIFW_LOGE("model info not found for model code %d", modelCode);
		return AIFW_ERROR;
	}
	AIFW_RESULT ret = gModelSetList.get()[index].aiModelService->start();
	return ret;
}

AIFW_RESULT ai_helper_stop(uint32_t modelCode)
{
	int index = findModelSetInfoIndex(modelCode);
	if (index == -1) {
		AIFW_LOGE("model info not found for model code %d", modelCode);
		return AIFW_ERROR;
	}
	AIFW_RESULT ret = gModelSetList.get()[index].aiModelService->stop();
	return ret;
}

AIFW_RESULT ai_helper_push_data(uint32_t modelCode, void *data, uint16_t len)
{
	int index = findModelSetInfoIndex(modelCode);
	if (index == -1) {
		AIFW_LOGE("no model registered with modelcode %d", modelCode);
		return AIFW_ERROR;
	}
	return gModelSetList.get()[index].aiModelService->pushData(data, len);
}

