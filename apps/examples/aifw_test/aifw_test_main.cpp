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

#include <stdio.h>
#include "aifw/aifw.h"
#include "aifw/aifw_log.h"
#include "aifw/aifw_csv_reader.h"
#include "aifw_test_main.h"
#include "ai_helper.h"

extern uint32_t gSineWaveCode;
static uint16_t gSensorValueCount;
static float *gSensorValues = NULL;
static void *gHandle = NULL;
static uint16_t gResultValueCount;
static float *gResultValues = NULL;
static void *gResultHandle = NULL;

/**
 * @brief: Deinitilizes AI Helper module and stops the service for the modelset.
 * Application calls this function if fetching raw data from data source fails.
*/
static void aifw_test_deinit(void)
{
	/* Deinitialize CSV data source for input raw data */
	free(gSensorValues);
	gSensorValues = NULL;
	AIFW_RESULT retValue = csvDeinit(&gHandle);
	if (retValue != AIFW_OK) {
		AIFW_LOGE("Input CSV deinit failed with error: %d", retValue);
	}

	/* Deinitialize CSV data source for expected inference result data */
	free(gResultValues);
	gResultValues = NULL;
	retValue = csvDeinit(&gResultHandle);
	if (retValue != AIFW_OK) {
		AIFW_LOGE("Result CSV deinit failed with error: %d", retValue);
	}

	if (ai_helper_stop(gSineWaveCode) != AIFW_OK) {
		AIFW_LOGE("AI helper stop failed");
	}
	ai_helper_deinit();
	AIFW_LOGV("AI helper deinit done.");
}

/**
 * @brief: AI Framework calls this function to collect the raw data and pass it for inference.
 * This callback is called when timer expires. Time interval is set in 'inferenceInterval' field of json.
*/
static void sine_collectRawDataListener(void)
{
	memset(gSensorValues, '\0', gSensorValueCount * sizeof(float));
	AIFW_RESULT result = readCSVData(gHandle, gSensorValues);
	if (result == AIFW_OK) {
		result = ai_helper_push_data(gSineWaveCode, (void *)gSensorValues, gSensorValueCount);
		if (result == AIFW_OK) {
			AIFW_LOGV("push data operation OK");
		} else {
			AIFW_LOGE("push data operation failed. ret: %d", result);
		}
		return;
	}
	AIFW_LOGE("reading input CSV data return error result : %d", result);
	aifw_test_deinit();
}

/**
 * @brief: Application will receive inference result in this function.
 * It is mandatory to be defined by each application.
 * @param [in] res: Successful inference operation 'res' is set to AIFW_OK. Errors are set as per AIFW_RESULT enum values.
 * @param [in] values: inference result values after ensembling 
 * @param [in] count: count of values in inference result
*/
static void sine_inferenceResultListener(AIFW_RESULT res, void *values, uint16_t count)
{
	if (res != AIFW_OK) {
		AIFW_LOGE("Inference failed for this cycle, ret: %d", res);
		return;
	}
	float *predictedResult = (float *)values;
	/* Read expected inference result from result CSV to compare it from predicted inference result */
	memset(gResultValues, '\0', gResultValueCount * sizeof(float));
	AIFW_RESULT result = readCSVData(gResultHandle, gResultValues);
	if (result != AIFW_OK) {
		AIFW_LOGE("reading result CSV data failed, inference result values cannot be verfied for this inference cycle. ret: %d", result);
	}
	AIFW_LOGI("Expected value: %f, AIFW prediction result : %f", gResultValues[1], predictedResult[0]);
}

int aifw_test_main(int argc, char *argv[])
{
	/* Initialize CSV data source for input raw data */
	AIFW_RESULT res = csvInit(&gHandle, "/mnt/AI/SineWave_packet.csv", FLOAT32, false);
	if (res != AIFW_OK) {
		AIFW_LOGE("FILE NOT FOUND || ERROR OPENING CSV. ret: %d", res);
		return -1;
	}
	res = getColumnCount(gHandle, &gSensorValueCount);
	if (res != AIFW_OK) {
		AIFW_LOGE("Fetching sensor value count failed. ret: %d\n", res);
		return -1;
	}
	gSensorValues = (float *)malloc(gSensorValueCount * sizeof(float));
	if (!gSensorValues) {
		AIFW_LOGE("Memory allocation failed for sensor values buffer");
		return -1;
	}
	AIFW_LOGV("Raw input data csv initialization OK");

	/* Initialize CSV data source for expected inference result data */
	res = csvInit(&gResultHandle, "/mnt/AI/SineWave_resultPacket.csv", FLOAT32, false);
	if (res != AIFW_OK) {
		AIFW_LOGE("FILE NOT FOUND || ERROR OPENING CSV. ret: %d", res);
		return -1;
	}
	res = getColumnCount(gResultHandle, &gResultValueCount);
	if (res != AIFW_OK) {
		AIFW_LOGE("Fetching result value count failed. ret: %d\n", res);
		return -1;
	}
	gResultValues = (float *)malloc(gResultValueCount * sizeof(float));
	if (!gResultValues) {
		AIFW_LOGE("Memory allocation failed for result values buffer");
		return -1;
	}
	AIFW_LOGV("Result data csv initialization OK");

	if (ai_helper_init(1) != AIFW_OK) {
		AIFW_LOGE("AI helper init failed");
		return -1;
	}
	if (ai_helper_load_model(gSineWaveCode, sine_inferenceResultListener, sine_collectRawDataListener) != AIFW_OK) {
		AIFW_LOGE("Load model failed");
		return -1;
	}
	ai_helper_start(gSineWaveCode);
	return 0;
}

