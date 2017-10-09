#ifndef __EVALUATOR__
#define __EVALUATOR__

#include "IPlug_include_in_plug_hdr.h"
#include "Program.h"
#include "Presets.h"
#include "IMidiQueue.h"
#include <vector>

class ITextEdit;
class ITextControl;

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

private:
  // UI
  void CreateGraphics();
  void MakePresetFromData(const Presets::Data& data);
  
  ITextEdit* textEdit;
  ITextControl* timeLabel;
  ITextControl* millisLabel;
  ITextControl* quartLabel;
  ITextControl* noteLabel;
  ITextControl* rangeLabel;
  ITextControl* prevLabel;
  IControl*     bitDepthControl;
  
  // plug state
  Program*				mProgram;
  double				mGain;
  int					mBitDepth;
  Program::Value		mTick;
  IMidiQueue			mMidiQueue;
  std::vector<IMidiMsg> mNotes;
};

#endif
