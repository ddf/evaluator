#include "Evaluator.h"
#include "IPlug_include_in_plug_src.h"
#include "IControl.h"
#include "resource.h"

#define MAX_ALG_LENGTH 128

const int kNumPrograms = 1;

enum EParams
{
  kGain = 0,
  kBitDepth = 1,
  kNumParams
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
  ITextEdit(IPlugBase* pPlug, IRECT pR, IText* pText, const char* str)
  : IControl(pPlug, pR)
  {
    mDisablePrompt = true;
    mText = *pText;
    mStr.Set(str);
  }
  
  ~ITextEdit() {}
  
  bool Draw(IGraphics* pGraphics)
  {
    return pGraphics->DrawIText(&mText, mStr.Get(), &mRECT);
  }
  
  void OnMouseDown(int x, int y, IMouseMod* pMod)
  {
    mPlug->GetGUI()->CreateTextEntry(this, &mText, &mRECT, mStr.Get());
  }
  
  void TextFromTextEntry(const char* txt)
  {
    mStr.Set(txt, MAX_ALG_LENGTH);
    
    //TODO: update alg
    
    SetDirty(false);
  }
  
protected:
  WDL_String mStr;
};

Evaluator::Evaluator(IPlugInstanceInfo instanceInfo)
  :	IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo)
  , mGain(1.)
  , textEdit(0)
{
  TRACE;

  //arguments are: name, defaultVal, minVal, maxVal, step, label
  GetParam(kGain)->InitDouble("Gain", 50., 0., 100.0, 0.01, "%");
  GetParam(kGain)->SetShape(2.);
  
  GetParam(kBitDepth)->InitEnum("Bit Depth", 15, 24);

  CreateGraphics();

  //MakePreset("preset 1", ... );
  MakeDefaultPreset((char *) "-", kNumPrograms);
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
  textEdit = new ITextEdit (this, MakeIRect(kExpression), &kExpressionTextStyle, "t*128");
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
  if ( 0 )
  {
    IBitmap numberBoxArrowUp = pGraphics->LoadIBitmap(NUMBERBOX_ARROW_UP_ID, NUMBERBOX_ARROW_UP_FN);
    IBitmap numberBoxArrowDown = pGraphics->LoadIBitmap(NUMBERBOX_ARROW_DOWN_ID, NUMBERBOX_ARROW_DOWN_FN);
    
    //--Bit Depth Number Box Background
    IBitmap numberBack = pGraphics->LoadIBitmap(NUMBERBOX_BACK_ID, NUMBERBOX_BACK_FN);
    IRECT backSize(0,0,numberBack.W,numberBack.H);
    const int offX = kEditorWidth - backSize.W() - 10;
    const int offY = 50;
    backSize.L += offX;
    backSize.R += offX;
    backSize.T += offY;
    backSize.B += offY;
    
    pGraphics->AttachControl(new IBitmapControl(this, backSize.L, backSize.T, &numberBack));
    
    //---Bit Depth Number Box Value--------
    const int textHH = 9;
    IRECT numberSize(backSize.L + 2, backSize.T + numberBack.H/2 - textHH, backSize.L + 20, backSize.T + numberBack.H/2 + textHH);
    pGraphics->AttachControl(new ICaptionControl(this, numberSize, kBitDepth, &kExprLogTextStyle));
//    bitDepthLabel = new CTextLabel(size, "", 0, kNoFrame);
//    bitDepthLabel->setBackColor(kBlackCColor);
//    bitDepthLabel->setFont(kDataFont);
//    bitDepthLabel->setFontColor(greenColor);
//    bitDepthLabel->setHoriAlign(kRightText);
//    numberBox->addView(bitDepthLabel);
    
    //---Number Box Buttons
//    const int ah = numberBoxArrowUp->getHeight()/2;
//    size(size.getTopRight().x, numberBox->getHeight()/2 - ah, numberBox->getWidth()-2, numberBox->getHeight()/2);
//    CKickButton* button = new CKickButton(size, this, kBitIncrementTag, ah, numberBoxArrowUp);
//    numberBox->addView(button);
//
//    size.offset(0,size.getHeight());
//    button = new CKickButton(size, this, kBitDecrementTag, ah, numberBoxArrowDown);
//    numberBox->addView(button);
//
//    frame->addView(numberBox);
    
    //--Bit Depth Number Box Label
    IRECT boxLabelSize(backSize.L + kEditorWidth - numberBack.W - 10,
                       backSize.T + numberBack.H,
                       backSize.R,
                       backSize.T + numberBack.H + 20);
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

  double* in1 = inputs[0];
  double* in2 = inputs[1];
  double* out1 = outputs[0];
  double* out2 = outputs[1];

  for (int s = 0; s < nFrames; ++s, ++in1, ++in2, ++out1, ++out2)
  {
    *out1 = *in1 * mGain;
    *out2 = *in2 * mGain;
  }
}

void Evaluator::Reset()
{
  TRACE;
  IMutexLock lock(this);
}

void Evaluator::OnParamChange(int paramIdx)
{
  IMutexLock lock(this);

  switch (paramIdx)
  {
    case kGain:
      mGain = GetParam(kGain)->Value() / 100.;
      break;

    default:
      break;
  }
}
