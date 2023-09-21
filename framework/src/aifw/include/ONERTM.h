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
 * @file onert-micro.h
 * @brief AIEngine implementation for onert-micro
 */

#ifndef __ONERT_MICRO_H__
#define __ONERT_MICRO_H__

#include <memory>
#include "aifw/aifw.h"
#include "AIEngine.h"

// forward declaration
namespace luci_interpreter
{
	class Interpreter;
}
namespace aifw {

class ONERTM : public AIEngine
{
public:
	ONERTM();
	~ONERTM();
	AIFW_RESULT loadModel(const char *file);
	AIFW_RESULT loadModel(const unsigned char *model);
	void *invoke(void *inputData);

private:
	AIFW_RESULT _loadModel(void);

	char *mBuf;
	std::shared_ptr<luci_interpreter::Interpreter> mInterpreter;
	uint16_t mModelInputSize;
	uint16_t mModelOutputSize;
};

} /* namespace aifw */

#endif /* __ONERT_MICRO_H__ */
