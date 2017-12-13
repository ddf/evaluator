#ifndef __EVALUATOR__
#define __EVALUATOR__

#include "IPlug_include_in_plug_hdr.h"
#include "Params.h"
#include "Program.h"
#include "Presets.h"
#include "IMidiQueue.h"
#include <vector>

class Interface;

class Evaluator : public IPlug
{
public:
	Evaluator(IPlugInstanceInfo instanceInfo);
	~Evaluator();

	void Reset() override;
	void OnParamChange(int paramIdx) override;
	void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames) override;
	void ProcessMidiMsg(IMidiMsg* pMsg) override;

	// have to hook into the chunks so that we can include the contents of our text-entry boxes
	bool SerializeState(ByteChunk* pChunk) override;
	int UnserializeState(ByteChunk* pChunk, int startPos) override;
	bool CompareState(const unsigned char* incomingState, int startPos) override;

	// catch the About menu item to display what we wants in a box
	bool HostRequestingAboutBox() override;

	// get a string that represents the internal state of the program we want to display in the UI
	const char * GetProgramState() const;
	void SetWatchText(Interface* forInterface) const;

private:

	void MakePresetFromData(const Presets::Data& data);
	void SerializeOurState(ByteChunk* pChunk);

	// the UI
	Interface*			mInterface;

	// plug state
	Program*				mProgram;
	int					mProgramMemorySize;
	// will be false if user input produced a compilation error.
	// we want to keep track of this so we don't update the UI in ProcessDoubleReplacing.
	bool					mProgramIsValid;
	TransportState	    mTransport;
	double				mGain;
	int					mBitDepth;
	int					mScopeUpdate;
	TimeType				mTimeType;
	Program::Value		mTick;
	IMidiQueue			mMidiQueue;
	std::vector<IMidiMsg> mNotes;
};

#endif
