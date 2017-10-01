#ifndef __EVALUATOR__
#define __EVALUATOR__

#include "IPlug_include_in_plug_hdr.h"

class Evaluator : public IPlug
{
public:
  Evaluator(IPlugInstanceInfo instanceInfo);
  ~Evaluator();

  void Reset();
  void OnParamChange(int paramIdx);
  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);

private:
  double mGain;
};

#endif
