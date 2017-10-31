#include "Evaluator.h"
#include "IPlug_include_in_plug_src.h"
#include "Interface.h"
#include "IControl.h"
#include "resource.h"

Evaluator::Evaluator(IPlugInstanceInfo instanceInfo)
: IPLUG_CTOR(kNumParams, Presets::Count(), instanceInfo)
, mProgram(0)
, mGain(1.)
, mBitDepth(15)
, mScopeUpdate(0)
, mTimeType(TTAlways)
{
  TRACE;

  //arguments are: name, defaultVal, minVal, maxVal, step, label
  GetParam(kGain)->InitDouble("Gain", 50., 0., 100.0, 1, "%");
  
  GetParam(kBitDepth)->InitInt("Bit Depth", 15, kBitDepthMin, kBitDepthMax);

  GetParam(kTimeType)->InitEnum("t Mode", 0, TTCount);
  GetParam(kTimeType)->SetDisplayText(TTAlways, "increment 't' always");
  GetParam(kTimeType)->SetDisplayText(TTWithNoteContinuous, "increment 't' while note on");
  GetParam(kTimeType)->SetDisplayText(TTWithNoteResetting, "increment 't' while note on, reset 't' every note on");
#if !SA_API
  GetParam(kTimeType)->SetDisplayText(TTProjectTime, "set 't' to project time");
#endif
  
  GetParam(kScopeWindow)->InitDouble("Scope Window Size", 0.5, (double)kScopeWindowMin/1000.0, (double)kScopeWindowMax/1000.0, 0.05, "s");

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
  
  mProgram->Set('w', range);
  mProgram->Set('~', (Program::Value)GetSampleRate());

  double* in1 = inputs[0];
  double* in2 = inputs[1];
  double* out1 = outputs[0];
  double* out2 = outputs[1];
  
  Program::RuntimeError error = Program::RE_NONE;
  ITimeInfo timeInfo;
  GetTime(&timeInfo);
  Program::Value results[2];
  double left = 0;
  double right = 0;
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
          if (mTimeType == TTWithNoteResetting)
          {
            mTick = 0;
          }
          mNotes.push_back(*pMsg);
          mProgram->Set('n', pMsg->NoteNumber());
		  mProgram->Set('v', pMsg->Velocity());
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
			  mProgram->Set('n', 0);
			  mProgram->Set('v', 0);
          }
          else
          {
            mProgram->Set('n', mNotes.back().NoteNumber());
			mProgram->Set('v', mNotes.back().Velocity());
          }
          break;

		case IMidiMsg::kControlChange:
			mProgram->SetCC(pMsg->mData1, pMsg->mData2);
			break;
          
        default:
          break;
      }
      
      mMidiQueue.Remove();
    }

	bool run = true;

	switch (mTimeType)
	{
	case TTWithNoteContinuous:
	case TTWithNoteResetting:
		run = !mNotes.empty(); break;
#if !SA_API
	case TTProjectTime:
		if ((run = timeInfo.mTransportIsRunning)) mTick = timeInfo.mSamplePos + s; break;
#endif
    default: break;
	}

	if (run)
	{
		mProgram->Set('t', mTick);
		mProgram->Set('m', mTick / mdenom);
		mProgram->Set('q', mTick / qdenom);
		results[0] = (*in1 + 1) * (range / 2);
		results[1] = (*in2 + 1) * (range / 2);
		error = mProgram->Run(results, 2);
		left = mGain * (-1.0 + 2.0*((double)(results[0]%range) / (range - 1)));
		right = mGain * (-1.0 + 2.0*((double)(results[1]%range) / (range - 1)));
		++mTick;
	}
	else
	{
		left = 0;
		right = 0;
	}

	*out1 = left;
	*out2 = right;
    
    if (mScopeUpdate == 0)
    {
      mInterface->UpdateOscilloscope(left, right);
      // we need to update the oscilloscope this many times every updateSeconds
      const int samplesPerInterval = mInterface->GetOscilloscopeWidth();
      const double updateInterval = GetParam(kScopeWindow)->Value();
      mScopeUpdate = (int)(GetSampleRate()*updateInterval/samplesPerInterval);
    }
    else
    {
      --mScopeUpdate;
    }
  }
  
  mMidiQueue.Flush(nFrames);

  if (mProgramIsValid && mInterface != nullptr)
  {
    if ( error == Program::RE_NONE )
    {
      mInterface->SetConsoleText(GetProgramState());
    }
    else
    {
      static const int maxError = 1024;
      static char errorDesc[maxError];
      snprintf(errorDesc, maxError,
               "Runtime Error: %s",
               Program::GetErrorString(error));
      mInterface->SetConsoleText(errorDesc);
    }
  }
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
  mScopeUpdate = 0;
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

	case kTimeType:
		mTimeType = (TimeType)GetParam(kTimeType)->Int();
		break;
      
    case kExpression:
	{
		Program::CompileError error;
		int errorPosition;
		const char* programText = mInterface->GetProgramText();
		mProgram = Program::Compile(programText, error, errorPosition);
		// we want to always have a program we can run,
		// so if compilation fails, we create one that simply evaluates to 0.
		mProgramIsValid = error == Program::CE_NONE;
		if (!mProgramIsValid)
		{
			static const int maxError = 1024;
			static char errorDesc[maxError];
			snprintf(errorDesc, maxError,
				"Compile Error:\n%s\nAt:\n%s",
				Program::GetErrorString(error),
				programText + errorPosition);
			mInterface->SetConsoleText(errorDesc);
			mProgram = Program::Compile("r/2", error, errorPosition);
		}

		// initializeeeee
		mTick = 0;
		mProgram->Set('n', 0);
		mProgram->Set('v', 0);
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

const char * Evaluator::GetProgramState() const
{
	static const int max_state = 1024;
	static char state[max_state];

	snprintf(state, max_state,
		"time                   input\n"
		"---------------------- ----------------------\n"
		"t=%-20llu w=%-20llu\n"
		"m=%-20llu n=%-20llu\n"
		"q=%-20llu v=%-20llu\n",
		mProgram->Get('t'),
		mProgram->Get('w'),
		mProgram->Get('m'),
		mProgram->Get('n'),
		mProgram->Get('q'),
		mProgram->Get('v')
	);

	return state;
}
