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

#include "aifw/aifw.h"
#include "aifw/aifw_log.h"
#include "aifw/AIModel.h"
#include "aifw/AIProcessHandler.h"
#include "inc/SineWaveInferenceHandler.h"
#ifndef CONFIG_PRODUCT_AI_MODEL_FILE_SUPPORT
#include "inc/SineWaveAIModel.h"
#endif

#define PREDICT_TIME 0x05

extern "C" {
uint32_t gSineWaveCode = 0x00000000; /* We don't use it for now */
}

#ifdef CONFIG_PRODUCT_AI_MODEL_FILE_SUPPORT
struct Model_Attributes {
	uint32_t modelCode;
	const char *scriptPath;
};
static const struct Model_Attributes gModelCodeMap[] = {
	{gSineWaveCode, "/mnt/AI/REF_TempPred.json"}};

static uint16_t gModelCount = sizeof(gModelCodeMap) / sizeof(struct Model_Attributes);
#else
static uint16_t gModelCount = 1;
#endif

SineWaveInferenceHandler::SineWaveInferenceHandler(InferenceResultListener listener) :
	AIInferenceHandler(gModelCount, listener)
{
}

SineWaveInferenceHandler::~SineWaveInferenceHandler()
{
}

AIFW_RESULT SineWaveInferenceHandler::prepare(void)
{
	// std::shared_ptr<aifw::AIProcessHandler> refTempPredictionProcessHandler = std::make_shared<RefTempPredictionProcessHandler>();
	// if (!refTempPredictionProcessHandler) {
	// 	AIFW_LOGE("Temperature prediction process handler memory allocation failed.");
	// 	return AIFW_NO_MEM;
	// }
	// mSWModel = std::make_shared<aifw::AIModel>(refTempPredictionProcessHandler);
	mSWModel = std::make_shared<aifw::AIModel>();
	if (!mSWModel) {
		AIFW_LOGE("Temperature prediction model memory allocation failed.");
		return AIFW_NO_MEM;
	}
	AIFW_RESULT result;
#ifdef CONFIG_PRODUCT_AI_MODEL_FILE_SUPPORT
	result = mSWModel->loadModel(gModelCodeMap[0].scriptPath);
	if (result != AIFW_OK) {
		AIFW_LOGE("Temperatur prediction load failed");
		return result;
	}
#else
	result = mSWModel->loadModel(gSineWaveModelAttribute);
	if (result != AIFW_OK) {
		AIFW_LOGE("Temperature prediction load failed. ret: %d", result);
		return result;
	}
#endif
	attachModel(mSWModel);
	return AIFW_OK;
}

/* onInferenceFinished will be called when inference finished properly */
AIFW_RESULT SineWaveInferenceHandler::onInferenceFinished(uint16_t idx, void *finalResult)
{
	return AIFW_OK;
}

