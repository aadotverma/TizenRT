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

static void aifw_test_deinit(void)
{
	/* Deinitialize input CSV */
	free(gSensorValues);
	gSensorValues = NULL;
	AIFW_RESULT retValue = csvDeinit(&gHandle);
	if (retValue != AIFW_OK) {
		AIFW_LOGE("Input CSV deinit failed with error: %d", retValue);
	}

	/* Deinitialize result CSV */
	free(gResultValues);
	gResultValues = NULL;
	retValue = csvDeinit(&gResultHandle);
	if (retValue != AIFW_OK) {
		AIFW_LOGE("Result CSV deinit failed with error: %d", retValue);
	}

	if (ai_helper_stop(gSineWaveCode) != AIFW_OK) {
		AIFW_LOGE("AI helper stop failed");
	}
}

/* Each Model will be inferenced for every predefined interval. AI F/W parse metadata of each model, and call this callback function 
to get the raw data from source. Raw data is passed to AI FW for pre-process, inference, post-process operations. Final result is 
received in sine_inferenceResultListener callback function
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

static void sine_inferenceResultListener(AIFW_RESULT res, void *values, uint16_t count)
{
	if (res != AIFW_OK) {
		AIFW_LOGE("Inference failed for this cycle, ret: %d", res);
		return;
	}
	float *predictedResult = (float *)values;
	/* Read expected result data from result CSV */
	memset(gResultValues, '\0', gResultValueCount * sizeof(float));
	AIFW_RESULT result = readCSVData(gResultHandle, gResultValues);
	if (result != AIFW_OK) {
		AIFW_LOGE("reading result CSV data return error result : %d", result);
		aifw_test_deinit();
	}
	AIFW_LOGI("Expected value: %f, AIFW prediction result : %f", gResultValues[1], predictedResult[0]);
}

int aifw_test_main(int argc, char *argv[])
{
	/* Initialize raw input data CSV */
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

	/* Initialize result data CSV */
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

