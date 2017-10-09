#include "Evaluator.h"
#include "IPlug_include_in_plug_src.h"
#include "Interface.h"
#include "IControl.h"
#include "resource.h"

Evaluator::Evaluator(IPlugInstanceInfo instanceInfo)
  :	IPLUG_CTOR(kNumParams, Presets::Count(), instanceInfo)
  , mProgram(0)
  , mGain(1.)
  , mBitDepth(15)
{
  TRACE;

  //arguments are: name, defaultVal, minVal, maxVal, step, label
  GetParam(kGain)->InitDouble("Gain", 50., 0., 100.0, 1, "%");
  
  GetParam(kBitDepth)->InitInt("Bit Depth", 15, kBitDepthMin, kBitDepthMax);

  for (int i = 0; i < Presets::Count(); ++i)
  {
	  MakePresetFromData(Presets::Get(i));
  }
  IGraphics* pGraphics = MakeGraphics(this, GUI_WIDTH, GUI_HEIGHT);
  mInterface = new Interface(this, pGraphics);
  AttachGraphics(pGraphics);
}

Evaluator::~Evaluator() 
{
	delete mInterface;
}

void Evaluator::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
  // Mutex is already locked for us.

  const Program::Value range = 1<<mBitDepth;
  const uint64_t mdenom = (uint64_t)(GetSampleRate()/1000);
  const uint64_t qdenom = (uint64_t)(GetSampleRate()/(GetTempo()/60.0))/128;
  
  mProgram->Set('r', range);
  mProgram->Set('~', (Program::Value)GetSampleRate());

  double* in1 = inputs[0];
  double* in2 = inputs[1];
  double* out1 = outputs[0];
  double* out2 = outputs[1];
  
  double amp = !mNotes.empty() ? mGain*mNotes.back().Velocity()/127 : 0;

  for (int s = 0; s < nFrames; ++s, ++in1, ++in2, ++out1, ++out2)
  {
    while (!mMidiQueue.Empty())
    {
      IMidiMsg* pMsg = mMidiQueue.Peek();
      if (pMsg->mOffset > s) break;
      
      // To-do: Handle the MIDI message
      switch(pMsg->StatusMsg())
      {
        case IMidiMsg::kNoteOn:
          if (mNotes.empty())
          {
            mProgram->Set('p', 0);
            mTick = 0;
          }
          mNotes.push_back(*pMsg);
          mProgram->Set('n',pMsg->NoteNumber());
          amp = mGain*pMsg->Velocity()/127;
          break;
          
        case IMidiMsg::kNoteOff:
          for (auto iter = mNotes.crbegin(); iter != mNotes.crend(); ++iter)
          {
            // remove the most recent note on with the same pitch
            if (pMsg->NoteNumber() == iter->NoteNumber())
            {
              mNotes.erase((iter+1).base());
              break;
            }
          }
          
          if (mNotes.empty())
          {
            mTick = 0;
            amp   = 0;
          }
          else
          {
            mProgram->Set('n', mNotes.back().NoteNumber());
            amp = mGain*mNotes.back().Velocity()/127;
          }
          break;
          
        default:
          break;
      }
      
      mMidiQueue.Remove();
    }
    
    ++mTick;
    mProgram->Set('t', mTick);
    mProgram->Set('m', mTick/mdenom);
    mProgram->Set('q', mTick/qdenom);
	Program::Value result;
	// TODO: report the error to the UI if need be
	Program::RuntimeError error = mProgram->Run(result);
    mProgram->Set('p', result);
    double evalSample = amp * (-1.0 + 2.0*((double)(result%range)/(range-1)) );
    
    *out1 = *in1 + evalSample * amp;
    *out2 = *in2 + evalSample * amp;
  }
  
  mMidiQueue.Flush(nFrames);
}

void Evaluator::Reset()
{
  TRACE;
  IMutexLock lock(this);
  
  // re-init vars
  if (mProgram != nullptr)
  {
	  delete mProgram;
	  mProgram = nullptr;
  }
  // force recompile
  OnParamChange(kExpression);
  
  mMidiQueue.Resize(GetBlockSize());
  mNotes.clear();
}

void Evaluator::ProcessMidiMsg(IMidiMsg *pMsg)
{
  mMidiQueue.Add(pMsg);
}

void Evaluator::OnParamChange(int paramIdx)
{
  IMutexLock lock(this);

  switch (paramIdx)
  {
    case kGain:
      mGain = GetParam(kGain)->Value() / 100.;
      break;
      
    case kBitDepth:
      mBitDepth = GetParam(kBitDepth)->Int();
	  mInterface->SetDirty(kBitDepth, false);
      break;
      
    case kExpression:
	{
		Program::CompileError error;
		int errorPosition;
		mProgram = Program::Compile(mInterface->GetProgramText(), error, errorPosition);
		// we want to always have a program we can run,
		// so if compilation fails, we create one that simply evaluates to 0.
		if (mProgram == nullptr)
		{
			mProgram = Program::Compile("0", error, errorPosition);
		}
	}
      break;
      
    default:
      break;
  }
}

void Evaluator::MakePresetFromData(const Presets::Data& data)
{
	// set params.
	GetParam(kGain)->Set(data.volume);
	GetParam(kBitDepth)->Set(data.bitDepth);

	// create serialized version
	ByteChunk chunk;
	chunk.PutStr(data.program);
	IPlugBase::SerializeParams(&chunk);

	// create it - const cast on data.name because this method take char*, even though it doesn't change it
	MakePresetFromChunk(const_cast<char*>(data.name), &chunk);
}

// this over-ridden method is called when the host is trying to store the plug-in state and needs to get the current data from your algorithm
bool Evaluator::SerializeState(ByteChunk* pChunk)
{
  TRACE;
  IMutexLock lock(this);
  
  pChunk->PutStr(mInterface->GetProgramText());
  
  return IPlugBase::SerializeParams(pChunk); // must remember to call SerializeParams at the end
}

// this over-ridden method is called when the host is trying to load the plug-in state and you need to unpack the data into your algorithm
int Evaluator::UnserializeState(ByteChunk* pChunk, int startPos)
{
  TRACE;
  IMutexLock lock(this);
  
  // initialize from empty string in case the get fails
  WDL_String expression("");
  startPos = pChunk->GetStr(&expression, startPos);

  mInterface->SetProgramText(expression.Get());
  
  return IPlugBase::UnserializeParams(pChunk, startPos); // must remember to call UnserializeParams at the end
}

bool Evaluator::CompareState(const unsigned char* incomingState, int startPos)
{
  bool isEqual = true;
  // create serialized representation of our string
  ByteChunk chunk;
  chunk.PutStr(mInterface->GetProgramText());
  // see if it's the same as the incoming state
  startPos = chunk.Size();
  isEqual = (memcmp(incomingState, chunk.GetBytes(), startPos) == 0);
  isEqual &= IPlugBase::CompareState(incomingState, startPos); // fuzzy compare regular params
  
  return isEqual;
}