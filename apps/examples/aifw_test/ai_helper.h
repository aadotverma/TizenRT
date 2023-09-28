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
 * @file ai_helper.h
 * @brief Helper APIs of AI Framework for C to CPP calls.
 */

#pragma once

#include "stdint.h"
#include "aifw/aifw.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief It creates a dynamic list of modelSets.
 * @param [IN] maxModelSetCount: number of modelSets needed in the list.
 * @return: AIFW_RESULT enum object.
 */
AIFW_RESULT ai_helper_init(uint16_t maxModelSetCount);

/**
 * @brief It resets factory creator pointer.
 * @return: AIFW_RESULT enum object.
 */
AIFW_RESULT ai_helper_deinit(void);

/**
 * @brief Starts the modelService for the corresponding modelSet.
 * @param [IN] modelCode: modelCode for the modelSet.
 * @return: AIFW_RESULT enum object.
 */
AIFW_RESULT ai_helper_start(uint32_t modelCode);

/**
 * @brief Stops the modelService for the corresponding modelSet.
 * @param [IN] modelCode: modelCode for the modelSet.
 * @return: AIFW_RESULT enum object.
 */
AIFW_RESULT ai_helper_stop(uint32_t modelCode);

/**
 * @brief Creates AIInferenceHandler and AIModelService class instances for the modelSet.
 * It then calls the prepare API of modelservice which creates instances of AIModel and Process Handler(if required).
 * Finally models are loaded and attached to AIInferenceHandler.
 * @param [IN] modelCode: modelCode for the modelSet.
 * @param [IN] resultCallback: Callback function to call when inference cycle of model set is completed.
 * @param [IN] collectRawDataCallback: Callback function to call when timer expires to collect raw data.
 * @return: AIFW_RESULT enum object.
 */
AIFW_RESULT ai_helper_load_model(uint32_t modelCode, InferenceResultListener resultCallback, CollectRawDataListener collectRawDataCallback);

/**
 * @brief Pushes the incoming raw data from app to AIModelService associated to modelSet.
 * @param [IN] modelCode: modelCode for the modelSet.
 * @param [IN] data: Incoming sensor data from app.
 * @param [IN] len: Length of incoming sensor data array.
 * @return: AIFW_RESULT enum object.
 */
AIFW_RESULT ai_helper_push_data(uint32_t modelCode, void *data, uint16_t len);

#ifdef __cplusplus
}
#endif

