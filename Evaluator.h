#ifndef __EVALUATOR__
#define __EVALUATOR__

#include "IPlug_include_in_plug_hdr.h"
#include "source/expression.hpp"
#include "IMidiQueue.h"
#include <vector>

using namespace Compartmental::Vst;

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
  
  ITextEdit* textEdit;
  ITextControl* timeLabel;
  ITextControl* millisLabel;
  ITextControl* quartLabel;
  ITextControl* noteLabel;
  ITextControl* rangeLabel;
  ITextControl* prevLabel;
  IControl*     bitDepthControl;
  
  // plug state
  Expression mExpression;
  double    mGain;
  int       mBitDepth;
  EvalValue mTick;
  IMidiQueue mMidiQueue;
  std::vector<IMidiMsg> mNotes;
};

#endif
