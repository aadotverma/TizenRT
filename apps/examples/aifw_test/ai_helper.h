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
 * @brief This function initializes AI Helper module. It allocates memory and sets internal data structures for 
 *        AI helper to work properly. This function is pre-requiste before any application calls ai_helper_load_model API.
 * @param [in] maxModelSetCount: Maximum number of model sets to be loaded by all applications.
 * @return: AIFW_RESULT enum object.
 */
AIFW_RESULT ai_helper_init(uint16_t maxModelSetCount);

/**
 * @brief This function deinitializes AI Helper & further no API of AI Helper should be called.
 * @return: AIFW_RESULT enum object.
 */
AIFW_RESULT ai_helper_deinit(void);

/**
 * @brief Starts service for the corresponding modelSet. After this application will start receiving callback in Collect Raw Data listener.
 * @param [in] modelCode: modelCode for which service is to be started.
 * @return: AIFW_RESULT enum object.
 */
AIFW_RESULT ai_helper_start(uint32_t modelCode);

/**
 * @brief Stops the modelService for the corresponding modelSet. Appplication will stop receiving callback in Collect Raw Data listener.
 * @param [in] modelCode: modelCode for which service is to be stopped.
 * @return: AIFW_RESULT enum object.
 */
AIFW_RESULT ai_helper_stop(uint32_t modelCode);

/**
 * @brief This API loads all AI models in model set corresponding to model code. A service is created to perform operation on loaded model set.
 * It instantiates application components such as inference and process handlers. The loaded models are mapped with application inference handler.
 * @param [in] modelCode: modelCode for the modelSet.
 * @param [in] resultCallback: Function to receive inference result from AI Framework.
 * @param [in] collectRawDataCallback: This function is called by service for data collection and inference operation.
 * @return: AIFW_RESULT enum object.
 */
AIFW_RESULT ai_helper_load_model(uint32_t modelCode, InferenceResultListener resultCallback, CollectRawDataListener collectRawDataCallback);

/**
 * @brief Helper function to push raw data to model service for pre-processing, inference and post processing.
 * @param [in] modelCode: modelCode for the modelSet.
 * @param [in] data: Incoming sensor data to be passed for processing.
 * @param [in] len: Length of incoming sensor data array.
 * @return: AIFW_RESULT enum object.
 */
AIFW_RESULT ai_helper_push_data(uint32_t modelCode, void *data, uint16_t len);

#ifdef __cplusplus
}
#endif

