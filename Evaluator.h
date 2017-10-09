#ifndef __EVALUATOR__
#define __EVALUATOR__

#include "IPlug_include_in_plug_hdr.h"
#include "Program.h"
#include "Presets.h"
#include "IMidiQueue.h"
#include <vector>

class Interface;

enum EParams
{
	kGain = 0,
	kBitDepth = 1,
	kNumParams,

	// used for text edit fields so the UI can call OnParamChange
	kExpression = 101,

	kExpressionLengthMax = 256,

	kBitDepthMin = 1,
	kBitDepthMax = 24
};

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

  // get a string that represents the internal state of the program we want to display in the UI
  const char * GetProgramState() const;

private:

  void MakePresetFromData(const Presets::Data& data);
  
  // the UI
  Interface*			mInterface;
  
  // plug state
  Program*				mProgram;
  // will be false if user input produced a compilation error.
  // we want to keep track of this so we don't update the UI in ProcessDoubleReplacing.
  bool					mProgramIsValid;
  double				mGain;
  int					mBitDepth;
  Program::Value		mTick;
  IMidiQueue			mMidiQueue;
  std::vector<IMidiMsg> mNotes;
};

#endif
