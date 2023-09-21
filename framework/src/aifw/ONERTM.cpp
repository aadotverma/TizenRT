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

#include <iostream>

#include "aifw/aifw_log.h"
#include "include/ONERTM.h"

#include "luci_interpreter/Interpreter.h"

namespace aifw {

ONERTM::ONERTM() :
	mBuf(NULL), mInterpreter(NULL), mModelInputSize(0), mModelOutputSize(0)
{
}

ONERTM::~ONERTM()
{
	AIFW_LOGE(":DEINIT:");
	if (mBuf) {
		free(mBuf);
		mBuf = NULL;
	}
	mInterpreter.reset();
}

AIFW_RESULT ONERTM::_loadModel(void)
{
	this->mInterpreter = std::make_shared<luci_interpreter::Interpreter>(
		this->mBuf,
		true);

	// TODO: support multiple input/outputs
	this->mModelInputSize = this->mInterpreter->getInputDataSizeByIndex(0);
	this->mModelOutputSize = this->mInterpreter->getOutputDataSizeByIndex(0);

	AIFW_LOGV("Interpreter initialization success.");

	return AIFW_OK;
}

AIFW_RESULT ONERTM::loadModel(const char *file)
{
	AIFW_LOGV("GetModel from File:%s", file);
	FILE *fp = fopen(file, "r");
	if (fp == NULL) {
		AIFW_LOGE("File %s open operation failed errno : %d", file, errno);
		return AIFW_ERROR_FILE_ACCESS;
	}
	fseek(fp, 0, SEEK_END);
	int size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	if (size < 0) {
		fclose(fp);
		AIFW_LOGE("File %s size read as %d is invalid, errno %d", file, size, errno);
		return AIFW_ERROR_FILE_ACCESS;
	}
	AIFW_LOGV("Model File Size: %d", size);
	this->mBuf = (char *)malloc(size);
	if (!this->mBuf) {
		fclose(fp);
		AIFW_LOGE("Memory not enough to allocate %d", size);
		return AIFW_NO_MEM;
	}
	fread(this->mBuf, 1, size, fp);
	fclose(fp);

	AIFW_LOGV("GetModel from Model file");

	AIFW_LOGV("Model Loaded from file %s", file);
	
	return _loadModel();
}

AIFW_RESULT ONERTM::loadModel(const unsigned char *model)
{
	this->mBuf = reinterpret_cast<char*>(const_cast<unsigned char *>(model));
	return _loadModel();
}

/* Run inference : with input data "features" and return output data ptr(Use output dimension to parse it) */
void *ONERTM::invoke(void *inputData)
{

	float *value = (float *)(inputData);
	auto *data = this->mInterpreter->allocateInputTensor(0);
	for (uint32_t i = 0; i < this->mModelInputSize/sizeof(float); ++i) {
		reinterpret_cast<float *>(data)[i] = value[i];
	}

	this->mInterpreter->interpret();

	return this->mInterpreter->readOutputTensor(0);
}

} /* namespace aifw */

