#include "Evaluator.h"
#include "IPlug_include_in_plug_src.h"
#include "IControl.h"
#include "resource.h"

const size_t MAX_ALG_LENGTH = 256;
const int kNumPrograms = 1;

enum EParams
{
  kGain = 0,
  kBitDepth = 1,
  kNumParams,
  
  // used for text edit fields so the UI can call OnParamChange
  kExpression = 101,
  
  kBitDepthMin = 1,
  kBitDepthMax = 24
};

enum ELayout
{
  kEditorWidth = GUI_WIDTH,
  kEditorHeight = GUI_HEIGHT,
  
  kExpression_X = 10,
  kExpression_Y = 10,
  kExpression_W = kEditorWidth - 20,
  kExpression_H = 20,
  
  kExprMsg_X = 10,
  kExprMsg_Y = kExpression_Y + 20,
  kExprMsg_W = kExpression_W,
  kExprMsg_H = 20,
  
  // the log window that shows the internal state of the expression
  kExprLog_X = 10,
  kExprLog_Y = kExprMsg_Y + 20,
  kExprLog_W = 140,
  kExprLog_H = 150,

  kExprLog_M = 5,   // margin
  kExprLog_TH = 12,  // text height
  kExprLog_TW = kExprLog_W - kExprLog_M*2, // text width
};

// note: ICOLOR is ARGB
const IColor kBackgroundColor(255, 30, 30, 30);
const IColor kExprBackgroundColor(255, 100, 100, 100);
const IColor kTextColor(255, 180, 180, 180);
const IColor kGreenColor(255, 0, 210, 10);

IText  kExpressionTextStyle(12,
                                  &kGreenColor,
                                  "Courier",
                                  IText::kStyleNormal,
                                  IText::kAlignCenter,
                                  0, // orientation
                                  IText::kQualityDefault,
                                  &kExprBackgroundColor,
                                  &kGreenColor);

IText  kExprMsgTextStyle(11,
                                  &kTextColor,
                                  "Arial",
                                  IText::kStyleBold,
                                  IText::kAlignNear,
                                  0, // orientation
                                  IText::kQualityDefault);

IText  kExprLogTextStyle(11,
                         &kGreenColor,
                         "Courier",
                         IText::kStyleNormal,
                         IText::kAlignNear,
                         0, // orientation
                         IText::kQualityDefault);

IText kLabelTextStyle(12,
                      &kTextColor,
                      "Arial",
                      IText::kStyleBold,
                      IText::kAlignCenter,
                      0, // orientation
                      IText::kQualityDefault);

// originally cribbed from the IPlugEEL example
class ITextEdit : public IControl
{
public:
  ITextEdit(IPlugBase* pPlug, IRECT pR, int paramIdx, IText* pText, const char* str)
  : IControl(pPlug, pR)
  , mIdx(paramIdx)
  {
    mDisablePrompt = true;
    mText = *pText;
    mStr.Set(str);
	mTextEntryLength = MAX_ALG_LENGTH;
  }
  
  ~ITextEdit() {}
  
  bool Draw(IGraphics* pGraphics) override
  {
    return pGraphics->DrawIText(&mText, mStr.Get(), &mRECT);
  }
  
  void OnMouseDown(int x, int y, IMouseMod* pMod) override
  {
    mPlug->GetGUI()->CreateTextEntry(this, &mText, &mRECT, mStr.Get());
  }
  
  void TextFromTextEntry(const char* txt) override
  {
    mStr.Set(txt, MAX_ALG_LENGTH);
    SetDirty(false);
    mPlug->OnParamChange(mIdx);
  }
  
  const char * GetText() const
  {
    return mStr.Get();
  }
  
  int GetTextLength() const
  {
    return mStr.GetLength();
  }
  
protected:
  int        mIdx;
  WDL_String mStr;
};

class IIncrementControl : public IBitmapControl
{
public:
  IIncrementControl(IPlugBase* pPlug, int x, int y, int paramIdx, IBitmap* pBitmap, int direction)
  : IBitmapControl(pPlug, x, y, paramIdx, pBitmap)
  , mPressed(0)
  {
    IParam* param = GetParam();
    mInc = direction * 1.0 / (param->GetMax() - param->GetMin());
    mDblAsSingleClick = true;
  }
  
  bool Draw(IGraphics* pGraphics) override
  {
    return pGraphics->DrawBitmap(&mBitmap, &mRECT, mPressed+1, &mBlend);
  }
  
  void OnMouseDown(int x, int y, IMouseMod* pMod) override
  {
    mPressed = 1;
    mValue = GetParam()->GetNormalized() + mInc;
    SetDirty();
  }
  
  void OnMouseUp(int x, int y, IMouseMod* pMod) override
  {
    mPressed = 0;
  }
  
private:
  double mInc;
  int   mPressed;
};

Evaluator::Evaluator(IPlugInstanceInfo instanceInfo)
  :	IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo)
  , textEdit(0)
  , bitDepthControl(0)
  , mProgram(0)
  , mGain(1.)
  , mBitDepth(15)
{
  TRACE;

  //arguments are: name, defaultVal, minVal, maxVal, step, label
  GetParam(kGain)->InitDouble("Gain", 50., 0., 100.0, 0.01, "%");
  
  GetParam(kBitDepth)->InitInt("Bit Depth", 15, kBitDepthMin, kBitDepthMax);

  CreateGraphics();

  //MakePreset("preset 1", ... );
  //MakeDefaultPreset((char *) "-", kNumPrograms);
  // TODO: initialize presets better
  textEdit->TextFromTextEntry("t*128");
  ByteChunk chunk;
  SerializeState(&chunk);
  MakePresetFromChunk("saw wave", &chunk);
}

Evaluator::~Evaluator() {}

// helper for adding text controls to the log window
ITextControl* AttachLogText(IPlugBase* pPlug, IGraphics* pGraphics, int& y, const char* defaultText)
{
  const int xl = kExprLog_X + kExprLog_M;
  const int xr = xl + kExprLog_TW;
  
  ITextControl* control = new ITextControl(pPlug, IRECT(xl, y, xr, y+kExprLog_TH), &kExprLogTextStyle, defaultText);
  pGraphics->AttachControl(control);
  
  y += kExprLog_TH;
  
  return control;
}

void Evaluator::CreateGraphics()
{
  IGraphics* pGraphics = MakeGraphics(this, kEditorWidth, kEditorHeight);
  
  pGraphics->AttachPanelBackground(&kBackgroundColor);

  //--- Text input for the expression ------
  textEdit = new ITextEdit (this, MakeIRect(kExpression), kExpression, &kExpressionTextStyle, "t*128");
  pGraphics->AttachControl(textEdit);

  ITextControl* textResult = new ITextControl(this, MakeIRect(kExprMsg), &kExprMsgTextStyle);
  pGraphics->AttachControl(textResult);

  //-- "window" displaying internal state of the expression
  pGraphics->AttachControl(new IPanelControl(this, MakeIRect(kExprLog), &COLOR_BLACK));
  {
    int y = kExprLog_Y + kExprLog_M;
    timeLabel = AttachLogText(this, pGraphics, y, "t=0");
    millisLabel = AttachLogText(this, pGraphics, y, "m=0");
    quartLabel = AttachLogText(this, pGraphics, y, "q=0");
    rangeLabel = AttachLogText(this, pGraphics, y, "r=0");
    noteLabel = AttachLogText(this, pGraphics, y, "n=0");
    prevLabel = AttachLogText(this, pGraphics, y, "p=0");
  }

  //---Volume--------------------
  {
    //---Volume Knob-------
    const int knobSize(35);
    const int knobLeft = kEditorWidth - knobSize - 10;
    const int knobTop = kEditorHeight - knobSize - 20;
    IRECT size (knobLeft, knobTop, knobLeft + knobSize, knobTop + knobSize);
    pGraphics->AttachControl(new IKnobLineControl(this, size, kGain, &kGreenColor));
    
//    volumeKnob = new CKnob(size, this, kVolumeTag, 0, 0, CPoint(0, 0), CKnob::kCoronaDrawing | CKnob::kCoronaOutline);
//    volumeKnob->setCoronaInset(2);
//    volumeKnob->setCoronaColor(greenColor);
//    volumeKnob->setColorShadowHandle(kBlackCColor);
//    volumeKnob->setHandleLineWidth(2);
//    volumeKnob->setColorHandle(greenColor);
//    frame->addView (volumeKnob);
    
    //---Volume Label--------
    IRECT labelSize(size.L-10, size.B-5, size.R+10, size.B+10);
    pGraphics->AttachControl(new ITextControl(this, labelSize, &kLabelTextStyle, "VOL"));
//    size(size.getBottomLeft().x - 10, size.getBottomLeft().y-5, size.getBottomRight().x + 10, size.getBottomRight().y + 10);
//    CTextLabel* label = new CTextLabel(size, "VOL", 0, kNoFrame);
//    label->setBackColor(kTransparentCColor);
//    label->setFont(kLabelFont);
//    label->setFontColor(textColor);
//    label->setHoriAlign(kCenterText);
//    frame->addView(label);
  }

  //---Bit Depth--------------
  //if ( 0 )
  {
    IBitmap numberBoxArrowUp = pGraphics->LoadIBitmap(NUMBERBOX_ARROW_UP_ID, NUMBERBOX_ARROW_UP_FN, 2);
    IBitmap numberBoxArrowDown = pGraphics->LoadIBitmap(NUMBERBOX_ARROW_DOWN_ID, NUMBERBOX_ARROW_DOWN_FN, 2);
    IBitmap numberBack = pGraphics->LoadIBitmap(NUMBERBOX_BACK_ID, NUMBERBOX_BACK_FN);
    
    //--Bit Depth Number Box Background
    IRECT backSize(0,0,numberBack.W,numberBack.H);
    const int offX = kEditorWidth - backSize.W() - 10;
    const int offY = 50;
    backSize.L += offX;
    backSize.R += offX;
    backSize.T += offY;
    backSize.B += offY;
    
    pGraphics->AttachControl(new IBitmapControl(this, backSize.L, backSize.T, &numberBack));
    
    //---Bit Depth Number Box Value--------
    const int textHH = 5;
    IRECT numberSize(backSize.L + 5, backSize.T + numberBack.H/2 - textHH, backSize.L + 25, backSize.T + numberBack.H/2 + textHH);
    bitDepthControl = new ICaptionControl(this, numberSize, kBitDepth, &kExprLogTextStyle);
    pGraphics->AttachControl(bitDepthControl);
//    bitDepthLabel = new CTextLabel(size, "", 0, kNoFrame);
//    bitDepthLabel->setBackColor(kBlackCColor);
//    bitDepthLabel->setFont(kDataFont);
//    bitDepthLabel->setFontColor(greenColor);
//    bitDepthLabel->setHoriAlign(kRightText);
//    numberBox->addView(bitDepthLabel);
    
    //---Number Box Buttons
    int arrowX = backSize.R - numberBoxArrowUp.W;
    int arrowY = backSize.T + numberBack.H/2 - numberBoxArrowUp.H/2;
    pGraphics->AttachControl(new IIncrementControl(this, arrowX, arrowY, kBitDepth, &numberBoxArrowUp, 1));
    pGraphics->AttachControl(new IIncrementControl(this, arrowX, arrowY + numberBoxArrowUp.H/2, kBitDepth, &numberBoxArrowDown, -1));
    
    //--Bit Depth Number Box Label
    IRECT boxLabelSize(backSize.L,
                       backSize.B + 5,
                       backSize.R,
                       backSize.B + 25);
    pGraphics->AttachControl(new ITextControl(this, boxLabelSize, &kLabelTextStyle, "BITS"));
//    CTextLabel* label = new CTextLabel(size, "BITS", 0, kNoFrame);
//    label->setBackColor(kTransparentCColor);
//    label->setFont(kLabelFont);
//    label->setFontColor(textColor);
//    label->setHoriAlign(kCenterText);
//    frame->addView(label);
  }
  
  AttachGraphics(pGraphics);
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
      if ( bitDepthControl )
      {
        bitDepthControl->SetDirty(false);
      }
      break;
      
    case kExpression:
	{
		char expr[MAX_ALG_LENGTH+1];
		const char * text = textEdit->GetText();
		const size_t textLen = textEdit->GetTextLength();
		strncpy(expr, text, textLen);
		expr[textLen] = '\0';
		Program::CompileError error;
		int errorPosition;
		mProgram = Program::Compile(expr, error, errorPosition);
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

// this over-ridden method is called when the host is trying to store the plug-in state and needs to get the current data from your algorithm
bool Evaluator::SerializeState(ByteChunk* pChunk)
{
  TRACE;
  IMutexLock lock(this);
  
  pChunk->PutStr(textEdit->GetText());
  
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

  textEdit->TextFromTextEntry(expression.Get());
  
  return IPlugBase::UnserializeParams(pChunk, startPos); // must remember to call UnserializeParams at the end
}

bool Evaluator::CompareState(const unsigned char* incomingState, int startPos)
{
  bool isEqual = true;
  // create serialized representation of our string
  ByteChunk chunk;
  chunk.PutStr(textEdit->GetText());
  // see if it's the same as the incoming state
  startPos = chunk.Size();
  isEqual = (memcmp(incomingState, chunk.GetBytes(), startPos) == 0);
  isEqual &= IPlugBase::CompareState(incomingState, startPos); // fuzzy compare regular params
  
  return isEqual;
}
