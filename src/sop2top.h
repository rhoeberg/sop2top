/* Shared Use License: This file is owned by Derivative Inc. (Derivative)
* and can only be used, and/or modified for use, in conjunction with
* Derivative's TouchDesigner software, and only if you are a licensee who has
* accepted Derivative's TouchDesigner license or assignment agreement
* (which also govern the use of this file). You may share or redistribute
* a modified version of this file provided the following conditions are met:
*
* 1. The shared file or redistribution must retain the information set out
* above and this list of conditions.
* 2. Derivative's name (Derivative Inc.) or its trademarks may not be used
* to endorse or promote products derived from this file without specific
* prior written permission from Derivative.
*/

#include "TOP_CPlusPlusBase.h"
#include "FrameQueue.h"
#include <thread>
#include <atomic>
using namespace TD;

class Sop2Top : public TOP_CPlusPlusBase
{
public:
	Sop2Top(const OP_NodeInfo *info, TOP_Context* context);
	virtual ~Sop2Top();

	virtual void		getGeneralInfo(TOP_GeneralInfo *, const OP_Inputs*, void*) override;

	virtual void		execute(TOP_Output*,
							const OP_Inputs*,
							void* reserved1) override;

	static void			fillBuffer(OP_SmartRef<TOP_Buffer>& mem, uint64_t byteOffset, int width, int height, double step, double brightness);


	virtual int32_t		getNumInfoCHOPChans(void *reserved1) override;
	virtual void		getInfoCHOPChan(int32_t index,
								OP_InfoCHOPChan *chan, void* reserved1) override;

	virtual bool		getInfoDATSize(OP_InfoDATSize *infoSize, void *reserved1) override;
	virtual void		getInfoDATEntries(int32_t index,
									int32_t nEntries,
									OP_InfoDATEntries *entries,
									void *reserved1) override;

	virtual void		setupParameters(OP_ParameterManager *manager, void *reserved1) override;
	virtual void		pulsePressed(const char *name, void *reserved1) override;

	void				waitForMoreWork();

private:

	void				fillAndUpload(TOP_Output* output, double speed, int width, int height, OP_TexDim texDim, int numLayers, int colorBufferIndex);

	void				startMoreWork();
	// We don't need to store this pointer, but we do for the example.
	// The OP_NodeInfo class store information about the node that's using
	// this instance of the class (like its name).
	const OP_NodeInfo*	myNodeInfo;

	// In this example this value will be incremented each time the execute()
	// function is called, then passes back to the TOP
	int					myExecuteCount;

	std::mutex			mySettingsLock;

	// Used for threading example
	// Search for #define THREADING_EXAMPLE to enable that example
	FrameQueue			myFrameQueue;
	std::thread*		myThread;
	std::atomic<bool>	myThreadShouldExit;

	std::condition_variable	myCondition;
	std::mutex			myConditionLock;
	std::atomic<bool>	myStartWork;

	TOP_Context*		myContext;
	OP_SmartRef<OP_TOPDownloadResult> myPrevDownRes;
};
