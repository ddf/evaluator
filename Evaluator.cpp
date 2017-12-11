#include "Evaluator.h"
#include "IPlug_include_in_plug_src.h"
#include "Interface.h"
#include "IControl.h"
#include "resource.h"

#if SA_API
static const char * kAboutBoxText = "Version " VST3_VER_STR "\nCreated by Damien Quartz\nBuilt on " __DATE__;
#endif

Evaluator::Evaluator(IPlugInstanceInfo instanceInfo)
	: IPLUG_CTOR(kNumParams, Presets::Count(), instanceInfo)
	, mProgram(0)
	, mProgramMemorySize(0)
	, mProgramIsValid(false)
	, mGain(1.)
	, mBitDepth(15)
	, mScopeUpdate(0)
	, mTimeType(TTAlways)
{
	TRACE;

	//arguments are: name, defaultVal, minVal, maxVal, step, label
	GetParam(kGain)->InitDouble("volume", 50., 0., 100.0, 1, "%");

	GetParam(kBitDepth)->InitInt("bit depth", 15, kBitDepthMin, kBitDepthMax);

	GetParam(kTimeType)->InitEnum("t-mode", 0, TTCount);
	GetParam(kTimeType)->SetDisplayText(TTAlways, "always");
	GetParam(kTimeType)->SetDisplayText(TTWithNoteContinuous, "with note on (continuous)");
	GetParam(kTimeType)->SetDisplayText(TTWithNoteResetting, "with note on (resetting)");
#if !SA_API
	GetParam(kTimeType)->SetDisplayText(TTProjectTime, "follow project time");
#endif

	GetParam(kScopeWindow)->InitDouble("scope window size", 0.5, (double)kScopeWindowMin / 1000.0, (double)kScopeWindowMax / 1000.0, 0.05, "s");

	char vcName[3];
	for (int paramIdx = kVControl0; paramIdx <= kVControl7; ++paramIdx)
	{
		sprintf(vcName, "V%d", paramIdx - kVControl0);
		GetParam(paramIdx)->InitInt(vcName, kVControlMin, kVControlMin, kVControlMax);
	}

	GetParam(kTempo)->InitDouble("tempo (ignored)", DEFAULT_TEMPO, kTempoMin, kTempoMax, 0.01, "bpm");
	GetParam(kTempo)->SetCanAutomate(false);

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

	const Program::Value range = (Program::Value)1 << mBitDepth;
	const double mdenom = GetSampleRate() / 1000.0;
#if !SA_API
	if ( GetParam(kTempo)->Value() != GetTempo() )
	{
		BeginInformHostOfParamChange(kTempo);
		GetParam(kTempo)->Set(GetTempo());
		InformHostOfParamChange(kTempo, GetParam(kTempo)->GetNormalized());
		EndInformHostOfParamChange(kTempo);
	}
#endif
	const double qdenom = (GetSampleRate() / (GetParam(kTempo)->Value() / 60.0)) / 128.0;

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
			switch (pMsg->StatusMsg())
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
						mNotes.erase((iter + 1).base());
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
			if ((run = timeInfo.mTransportIsRunning)) mTick = (Program::Value)(timeInfo.mSamplePos + s); break;
#endif
		default: break;
		}

		if (run)
		{
			mProgram->Set('t', mTick);
			mProgram->Set('m', (Program::Value)round(mTick / mdenom));
			mProgram->Set('q', (Program::Value)round(mTick / qdenom));
			results[0] = (Program::Value)((*in1 + 1) * (range / 2));
			results[1] = (Program::Value)((*in2 + 1) * (range / 2));
			error = mProgram->Run(results, 2);
			left = mGain * (-1.0 + 2.0*((double)(results[0] % range) / (range - 1)));
			right = mGain * (-1.0 + 2.0*((double)(results[1] % range) / (range - 1)));
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
			mScopeUpdate = (int)(GetSampleRate()*updateInterval / samplesPerInterval);
		}
		else
		{
			--mScopeUpdate;
		}
	}

	mMidiQueue.Flush(nFrames);

	if (mProgramIsValid && mInterface != nullptr)
	{
		if (error == Program::RE_NONE)
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

		SetWatchText(mInterface);
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
		mProgramIsValid = false;
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
		mInterface->SetDirty(kTimeType, false);
		break;

	case kExpression:
	{
		Program::CompileError error;
		int errorPosition;
		const char* programText = mInterface->GetProgramText();
		// we get the memory size from the interface because we *might* expose this in the UI.
		// but I'm not totally convinced there is much utility in doing so.
		mProgramMemorySize = mInterface->GetProgramMemorySize();
		mProgram = Program::Compile(programText, mProgramMemorySize, error, errorPosition);
		// we want to always have a program we can run,
		// so if compilation fails, we create one that simply evaluates to silence.
		mProgramIsValid = error == Program::CE_NONE;
		if (!mProgramIsValid)
		{
			static const int maxError = 1024;
			static char errorDesc[maxError];
			static const int maxLoc = 47;
			static char programLoc[maxLoc];
			int len = strlen(programText + errorPosition);
			if (len > maxLoc - 2) len = maxLoc - 2;
			memset(programLoc, '\0', maxLoc);
			strncpy(programLoc, programText + errorPosition, len);
			for (int i = 0; i < maxLoc; ++i)
			{
				if (programLoc[i] == '\n')
				{
					programLoc[i] = '\0';
					break;
				}
			}
			snprintf(errorDesc, maxError,
				"Compile Error:\n\n%s\n\nAt:\n\n%s",
				Program::GetErrorString(error),
				programLoc);
			mInterface->SetConsoleText(errorDesc);
			mProgram = Program::Compile("[*] = w/2", 0, error, errorPosition);
		}

		// initializeeeee
		mTick = 0;
		if (mProgramIsValid)
		{
			for (paramIdx = kVControl0; paramIdx <= kVControl7; ++paramIdx)
			{
				Program::Value vidx = paramIdx - kVControl0;
				mProgram->SetVC(vidx, GetParam(paramIdx)->Int());
			}
		}
		RedrawParamControls();
	}
	break;

	default:
		if (paramIdx >= kWatch && paramIdx < kWatch + kWatchNum && mInterface != nullptr)
		{
			SetWatchText(mInterface);
			RedrawParamControls();
		}
		else if (paramIdx >= kVControl0 && paramIdx <= kVControl7 && mProgramIsValid)
		{
			Program::Value vidx = paramIdx - kVControl0;
			mProgram->SetVC(vidx, GetParam(paramIdx)->Int());
			RedrawParamControls();
		}
		break;
	}
}

// need to start the version at a really high number cuz
// i didn't include it at first so unversioned data will have length 
// of the expression string as the first bit of data.
static const int kStateFirstVersion = 100000; // add the watch strings to the serialized state
static const int kStateVCParams = kStateFirstVersion + 1;
static const int kStateProgramName = kStateVCParams + 1;
static const int kStateTempo = kStateProgramName + 1;
static const int kStateVersion = kStateTempo;

void Evaluator::MakePresetFromData(const Presets::Data& data)
{
	// set params.
	GetParam(kGain)->Set(data.volume);
	GetParam(kBitDepth)->Set(data.bitDepth);
	GetParam(kTimeType)->Set(data.timeType);

	const int* vc = &data.V0;
	for (int paramIdx = kVControl0; paramIdx <= kVControl7; ++paramIdx)
	{
		const int vcIdx = paramIdx - kVControl0;
		GetParam(paramIdx)->Set(vc[vcIdx]);
	}

	// create serialized version
	ByteChunk chunk;
	chunk.Put(&kStateVersion);
	chunk.PutStr(data.program);
	int watchNum = kWatchNum;
	chunk.Put(&watchNum);
	const char* const* watches = &data.W0;
	for (int i = 0; i < kWatchNum; ++i)
	{
		chunk.PutStr(watches[i]);
	}
	chunk.PutStr(data.name);
	IPlugBase::SerializeParams(&chunk);

	// create it - const cast on data.name because this method take char*, even though it doesn't change it
	MakePresetFromChunk(const_cast<char*>(data.name), &chunk);
}

void Evaluator::SerializeOurState(ByteChunk* pChunk)
{
	pChunk->Put(&kStateVersion);
	pChunk->PutStr(mInterface->GetProgramText());
	int watchNum = kWatchNum;
	pChunk->Put(&watchNum);
	for (int i = 0; i < watchNum; ++i)
	{
		pChunk->PutStr(mInterface->GetWatch(i));
	}
	pChunk->PutStr(mInterface->GetProgramName());
}

// this over-ridden method is called when the host is trying to store the plug-in state and needs to get the current data from your algorithm
bool Evaluator::SerializeState(ByteChunk* pChunk)
{
	TRACE;
	IMutexLock lock(this);

	SerializeOurState(pChunk);

	return IPlugBase::SerializeParams(pChunk); // must remember to call SerializeParams at the end
}

// this over-ridden method is called when the host is trying to load the plug-in state and you need to unpack the data into your algorithm
int Evaluator::UnserializeState(ByteChunk* pChunk, int startPos)
{
	TRACE;
	IMutexLock lock(this);

	int version = 0;
	int nextPos = pChunk->Get(&version, startPos);
	// if it's not a valid version number, then we need to read our expression from the startPos
	if (version >= kStateFirstVersion)
	{
		startPos = nextPos;
	}

	// initialize from empty string in case the get fails
	WDL_String stringData("");
	nextPos = pChunk->GetStr(&stringData, startPos);

	if (nextPos > startPos)
	{
		mInterface->SetProgramText(stringData.Get());
	}
	else
	{
		mInterface->SetProgramText("");
	}

	startPos = nextPos;

	if (version >= kStateFirstVersion)
	{
		int watchNum = 0;
		nextPos = pChunk->Get(&watchNum, startPos);
		for (int i = 0; i < watchNum; ++i)
		{
			// clear the string cuz if there is a zero length string in the chunk it won't clear it.
			stringData.Set("");
			startPos = nextPos;
			nextPos = pChunk->GetStr(&stringData, startPos);
			if (nextPos > startPos)
			{
				mInterface->SetWatch(i, stringData.Get());
			}
			else
			{
				mInterface->SetWatch(i, "");
			}
		}
	}
	else
	{
		for (int i = 0; i < kWatchNum; ++i)
		{
			mInterface->SetWatch(i, "");
		}
	}

	startPos = nextPos;

	if (version >= kStateProgramName)
	{
		stringData.Set("");
		nextPos = pChunk->GetStr(&stringData, startPos);
		if (nextPos > startPos)
		{
			mInterface->SetProgramName(stringData.Get());
		}
		else
		{
			mInterface->SetProgramName("");
		}
	}

	startPos = nextPos;

	const int numParams = version < kStateVCParams ? kScopeWindow + 1
						: version < kStateTempo ? kVControl7 + 1
						: kNumParams;

	return IPlugBase::UnserializeParams(pChunk, startPos, numParams); // must remember to call UnserializeParams at the end
}

bool Evaluator::CompareState(const unsigned char* incomingState, int startPos)
{
	bool isEqual = true;
	// create serialized representation of our string
	ByteChunk chunk;
	SerializeOurState(&chunk);
	// see if it's the same as the incoming state
	int stateSize = chunk.Size();
	isEqual = (memcmp(incomingState + startPos, chunk.GetBytes(), stateSize) == 0);
	isEqual &= IPlugBase::CompareState(incomingState, startPos + stateSize); // fuzzy compare regular params

	return isEqual;
}

bool Evaluator::HostRequestingAboutBox()
{
#if SA_API
#ifdef OS_WIN
	GetGUI()->ShowMessageBox(kAboutBoxText, BUNDLE_NAME, MB_OK);
#else
  // sadly, on osx, ShowMessageBox uses an alert style box that does not show the app icon,
  // which is different from the default About window that is shown.
  // *that* code uses swell's MessageBox, so we use that directly on mac.
  MessageBox(0, kAboutBoxText, BUNDLE_NAME, MB_OK);
#endif
	return true;
#endif
  return false;
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

void Evaluator::SetWatchText(Interface* forInterface) const
{
	static const int max_text = 1024;
	static char text[max_text];

	char* printTo = text;
	for (int i = 0; i < kWatchNum; ++i)
	{
		const char * watch = forInterface->GetWatch(i);
		size_t len = strlen(watch);
		switch (len)
		{
		case 0:
			printTo += sprintf(printTo, "\n");
			break;

		case 1:
		{
			char var = *watch;
			if (isalpha(var) && islower(var))
			{
				printTo += sprintf(printTo, "%llu\n", mProgram->Get(var));
			}
			else
			{
				printTo += sprintf(printTo, "'%s' is not a variable\n", watch);
				//goto nan;
			}
		}
		break;

		default:
			if (watch[0] == '@')
			{
				char var = watch[1];
				Program::Value addr = 0;
				if (isalpha(var) && islower(var))
				{
					addr = mProgram->Get(var);
				}
				else // try to parse the number
				{
					const Program::Char* startPtr = watch + 1;
					Program::Char* endPtr = nullptr;
					addr = (Program::Value)strtoull(startPtr, &endPtr, 10);
					// failed to parse a number
					if (endPtr == startPtr)
					{
						//goto nan;
						printTo += sprintf(printTo, "unknown address\n");
						break;
					}
				}
				printTo += sprintf(printTo, "%llu\n", mProgram->Peek(addr));
			}
			else if (watch[0] == 'C')
			{
				char var = watch[1];
				Program::Value cc = 0;
				if (isalpha(var) && islower(var))
				{
					cc = mProgram->Get(var);
				}
				else // try to parse the number
				{
					const Program::Char* startPtr = watch + 1;
					Program::Char* endPtr = nullptr;
					cc = (Program::Value)strtoull(startPtr, &endPtr, 10);
					// failed to parse a number
					if (endPtr == startPtr)
					{
						//goto nan;
						printTo += sprintf(printTo, "unknown MIDI CC\n");
						break;
					}
				}
				printTo += sprintf(printTo, "%llu\n", mProgram->GetCC(cc));
			}
			else if (watch[0] == 'V')
			{
				char var = watch[1];
				Program::Value vc = 0;
				if (isalpha(var) && islower(var))
				{
					vc = mProgram->Get(var);
				}
				else // try to parse the number
				{
					const Program::Char* startPtr = watch + 1;
					Program::Char* endPtr = nullptr;
					vc = (Program::Value)strtoull(startPtr, &endPtr, 10);
					// failed to parse a number
					if (endPtr == startPtr)
					{
						//goto nan;
						printTo += sprintf(printTo, "unknown V control\n");
						break;
					}
				}
				printTo += sprintf(printTo, "%llu\n", mProgram->GetVC(vc));
			}
			else
			{
				printTo += sprintf(printTo, "unknown watch string\n");
			}
			break;
		}
    forInterface->SetWatchValue(i, text);
    printTo = text;
	}
}
