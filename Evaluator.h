#ifndef __EVALUATOR__
#define __EVALUATOR__

#include "IPlug_include_in_plug_hdr.h"

class ITextEdit;
class ITextControl;

class Evaluator : public IPlug
{
public:
  Evaluator(IPlugInstanceInfo instanceInfo);
  ~Evaluator();

  void Reset();
  void OnParamChange(int paramIdx);
  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);

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
  double mGain;
  int    mBitDepth;
};

#endif
