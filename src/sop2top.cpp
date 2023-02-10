/* 
   author: Rasmus Hoeberg
   custom touchdesigner plugin for converting a SOP directly to a TOP without having to go through CHOP
   
   LICENSE
   This file is licensed with MIT, see LICENSE.txt for more information
   Copyright 2023 Rasmus Hoeberg

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "sop2top.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <cmath>
#include <random>
#include <chrono>

extern "C"
{

DLLEXPORT
void
FillTOPPluginInfo(TOP_PluginInfo *info)
{
	// This must always be set to this constant
	info->apiVersion = TOPCPlusPlusAPIVersion;

	// Change this to change the executeMode behavior of this plugin.
	info->executeMode = TOP_ExecuteMode::CPUMem;

	// The opType is the unique name for this TOP. It must start with a
	// capital A-Z character, and all the following characters must lower case
	// or numbers (a-z, 0-9)
	info->customOPInfo.opType->setString("Sop2top");

	// The opLabel is the text that will show up in the OP Create Dialog
	info->customOPInfo.opLabel->setString("SOP to TOP");

	// Will be turned into a 3 letter icon on the nodes
	info->customOPInfo.opIcon->setString("S2T");

	// Information about the author of this OP
	info->customOPInfo.authorName->setString("Rasmus Hoeberg");
	// info->customOPInfo.authorEmail->setString("mail@mail.com");

	// This TOP works with 0 or 1 inputs connected
	info->customOPInfo.minInputs = 0;
	info->customOPInfo.maxInputs = 1;
}

DLLEXPORT
TOP_CPlusPlusBase*
CreateTOPInstance(const OP_NodeInfo* info, TOP_Context* context)
{
	// Return a new instance of your class every time this is called.
	// It will be called once per TOP that is using the .dll
	return new Sop2Top(info, context);
}

DLLEXPORT
void
DestroyTOPInstance(TOP_CPlusPlusBase* instance, TOP_Context *context)
{
	// Delete the instance here, this will be called when
	// Touch is shutting down, when the TOP using that instance is deleted, or
	// if the TOP loads a different DLL
	delete (Sop2Top*)instance;
}

};

Sop2Top::Sop2Top(const OP_NodeInfo* info, TOP_Context* context) :
	myNodeInfo(info),
	myThread(nullptr),
	myThreadShouldExit(false),
	myStartWork(false),
	myContext(context),
	myFrameQueue(context)
{
	myExecuteCount = 0;
}

Sop2Top::~Sop2Top()
{
}

void
Sop2Top::getGeneralInfo(TOP_GeneralInfo* ginfo, const OP_Inputs* inputs, void* reserved1)
{
	ginfo->cookEveryFrameIfAsked = true;
}

void
Sop2Top::execute(TOP_Output* output, const OP_Inputs* inputs, void* reserved1)
{
	myExecuteCount++;

	const OP_SOPInput *sop = inputs->getParSOP("Sop");
	// if(sop != NULL) {
	// }

	if(sop != NULL) {
		const Position *points = sop->getPointPositions();
		int pointCount = sop->getNumPoints();

		TOP_UploadInfo info;
		info.textureDesc.texDim = OP_TexDim::e2D;
		int width = 128;
		int height = 256;
		info.textureDesc.width = width;
		info.textureDesc.height = height;
		info.textureDesc.pixelFormat = OP_PixelFormat::RGBA32Float;
		info.colorBufferIndex = 0;

		uint64_t layerBytes = uint64_t(info.textureDesc.width) * info.textureDesc.height * 4 * sizeof(float);
		uint64_t byteSize = layerBytes;
		OP_SmartRef<TOP_Buffer> buf = myContext->createOutputBuffer(byteSize, TOP_BufferFlags::None, nullptr);

		char* bytePtr = (char*)buf->data;

		if(buf) {

			int i = 0;
			float* mem = (float*)bytePtr;
			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					float* pixel = &mem[4*(y*width + x)];
					if (i < pointCount) {
						pixel[0] = points[i].x;
						pixel[1] = points[i].y;
						pixel[2] = points[i].z;
						pixel[3] = 1;
					}
					else {
						pixel[0] = 0;
						pixel[1] = 0;
						pixel[2] = 0;
						pixel[3] = 1;
					}

					i++;
				}
			}
			output->uploadBuffer(&buf, info, nullptr);
		}
	}
}

int32_t
Sop2Top::getNumInfoCHOPChans(void *reserved1)
{
	// We return the number of channel we want to output to any Info CHOP
	// connected to the TOP. In this example we are just going to send one channel.
	return 2;
}

void
Sop2Top::getInfoCHOPChan(int32_t index, OP_InfoCHOPChan* chan, void* reserved1)
{
}

bool		
Sop2Top::getInfoDATSize(OP_InfoDATSize* infoSize, void* reserved1)
{
	infoSize->rows = 3;
	infoSize->cols = 2;
	// Setting this to false means we'll be assigning values to the table
	// one row at a time. True means we'll do it one column at a time.
	infoSize->byColumn = false;
	return true;
}

void
Sop2Top::getInfoDATEntries(int32_t index,
								int32_t nEntries,
								OP_InfoDATEntries* entries,
								void *reserved1)
{
	char tempBuffer[4096];

	if (index == 0)
	{
		// Set the value for the first column
#ifdef _WIN32
		strcpy_s(tempBuffer, "executeCount");
#else // macOS
		strlcpy(tempBuffer, "executeCount", sizeof(tempBuffer));
#endif
		entries->values[0]->setString(tempBuffer);

		// Set the value for the second column
#ifdef _WIN32
		sprintf_s(tempBuffer, "%d", myExecuteCount);
#else // macOS
		snprintf(tempBuffer, sizeof(tempBuffer), "%d", myExecuteCount);
#endif
		entries->values[1]->setString(tempBuffer);
	}

	if (index == 1)
	{
#ifdef _WIN32
		strcpy_s(tempBuffer, "step");
#else // macOS
		strlcpy(tempBuffer, "step", sizeof(tempBuffer));
#endif
		entries->values[0]->setString(tempBuffer);
		entries->values[1]->setString(tempBuffer);
	}

	if (index == 2)
	{
#ifdef _WIN32
		strcpy_s(tempBuffer, "points");
#else // macOS
		strlcpy(tempBuffer, "points", sizeof(tempBuffer));
#endif
		entries->values[0]->setString(tempBuffer);
	}
}

void
Sop2Top::setupParameters(OP_ParameterManager* manager, void *reserved1)
{
	// input sop
	OP_StringParameter np;
	np.name = "Sop";
	np.label = "Sop";
	OP_ParAppendResult res = manager->appendSOP(np);
	assert(res == OP_ParAppendResult::Success);
}

void
Sop2Top::pulsePressed(const char* name, void *reserved1)
{
}

