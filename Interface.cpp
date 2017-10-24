#include "Interface.h"
#include "Evaluator.h"
#include "IControl.h"
#include "Controls.h"

enum ELayout
{
	kEditorWidth = GUI_WIDTH,
	kEditorHeight = GUI_HEIGHT,
  
  kVolumeLabel_X = 10,
  kVolumeLabel_Y = 10,
  kVolumeLabel_W = 30,
  kVolumeLabel_H = 15,
  
  kVolumeKnob_X = kVolumeLabel_X,
  kVolumeKnob_Y = kVolumeLabel_Y + kVolumeLabel_H,
  kVolumeKnob_W = kVolumeLabel_W,
  kVolumeKnob_H = kVolumeLabel_W,
  
  kBitDepth_X = kVolumeKnob_X + kVolumeKnob_W + 10,
  kBitDepth_Y = kVolumeKnob_Y,
  
  kProgramLabel_X = 10,
  kProgramLabel_Y = kVolumeKnob_Y + kVolumeKnob_H + 5,
  kProgramLabel_W = kEditorWidth - 20,
  kProgramLabel_H = 15,

	kProgramText_X = 10,
	kProgramText_Y = kProgramLabel_Y + kProgramLabel_H,
	kProgramText_W = kEditorWidth - 20,
	kProgramText_H = 200,
  
  kExprLogTitle_X = kProgramText_X,
  kExprLogTitle_Y = kProgramText_Y + kProgramText_H + 15,
  kExprLogTitle_W = kProgramText_W,
  kExprLogTitle_H = 15,

	// the log window that shows the internal state of the expression
	kExprLog_X = kProgramText_X,
	kExprLog_Y = kExprLogTitle_Y + kExprLogTitle_H,
	kExprLog_W = kProgramText_W,
	kExprLog_H = 150,

	kExprLog_M = 5,   // margin
	kExprLog_TH = 12,  // text height
	kExprLog_TW = kExprLog_W - kExprLog_M * 2, // text width
};

// note: ICOLOR is ARGB
const IColor kBackgroundColor(255, 30, 30, 30);
const IColor kExprBackgroundColor(255, 100, 100, 100);
const IColor kTextColor(255, 180, 180, 180);
const IColor kGreenColor(255, 0, 210, 10);

IText  kExpressionTextStyle(11,
							&kGreenColor,
							"Courier",
							IText::kStyleNormal,
							IText::kAlignNear,
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

IText kTitleTextStyle(12,
                      &kTextColor,
                      "Arial",
                      IText::kStyleBold,
                      IText::kAlignNear,
                      0, // orientation
                      IText::kQualityDefault);

Interface::Interface(Evaluator* plug, IGraphics* pGraphics)
: mPlug(plug)
{
	pGraphics->AttachPanelBackground(&kBackgroundColor);

	//--- Text input for the expression ------
  {
    pGraphics->AttachControl(new ITextControl(mPlug, MakeIRect(kProgramLabel), &kTitleTextStyle, "PROGRAM"));
    textEdit = new ITextEdit(mPlug, MakeIRect(kProgramText), kExpression, &kExpressionTextStyle, "t*128");
    pGraphics->AttachControl(textEdit);
  }

	//-- "window" displaying internal state of the expression
	{
    pGraphics->AttachControl(new ITextControl(mPlug, MakeIRect(kExprLogTitle), &kTitleTextStyle, "STATE"));
		IRECT LogRect = MakeIRect(kExprLog);
		consoleTextControl = new ConsoleText(mPlug, LogRect, &kExprLogTextStyle, &COLOR_BLACK, 5);
		pGraphics->AttachControl(consoleTextControl);
	}

	//---Volume--------------------
	{
    //---Volume Label--------
    pGraphics->AttachControl(new ITextControl(mPlug, MakeIRect(kVolumeLabel), &kLabelTextStyle, "VOL"));
    
		//---Volume Knob-------
    IColor coronaColor(kGreenColor);
		pGraphics->AttachControl(new KnobLineCoronaControl(mPlug, MakeIRect(kVolumeKnob), kGain, &kGreenColor, &coronaColor, 1.5));
	}

	//---Bit Depth--------------
	{
		IBitmap numberBoxArrowUp = pGraphics->LoadIBitmap(NUMBERBOX_ARROW_UP_ID, NUMBERBOX_ARROW_UP_FN, 2);
		IBitmap numberBoxArrowDown = pGraphics->LoadIBitmap(NUMBERBOX_ARROW_DOWN_ID, NUMBERBOX_ARROW_DOWN_FN, 2);
		IBitmap numberBack = pGraphics->LoadIBitmap(NUMBERBOX_BACK_ID, NUMBERBOX_BACK_FN);

		//--Bit Depth Number Box Background
		IRECT backSize(kBitDepth_X, kBitDepth_Y, kBitDepth_X + numberBack.W, kBitDepth_Y + numberBack.H);

		pGraphics->AttachControl(new IBitmapControl(mPlug, backSize.L, backSize.T, &numberBack));

		//---Bit Depth Number Box Value--------
		const int textHH = 5;
		IRECT numberSize(backSize.L + 5, backSize.T + numberBack.H / 2 - textHH, backSize.L + 25, backSize.T + numberBack.H / 2 + textHH);
		bitDepthControl = new ICaptionControl(mPlug, numberSize, kBitDepth, &kExprLogTextStyle);
		pGraphics->AttachControl(bitDepthControl);

		//---Number Box Buttons
		int arrowX = backSize.R - numberBoxArrowUp.W;
		int arrowY = backSize.T + numberBack.H / 2 - numberBoxArrowUp.H / 2;
		pGraphics->AttachControl(new IIncrementControl(mPlug, arrowX, arrowY, kBitDepth, &numberBoxArrowUp, 1));
		pGraphics->AttachControl(new IIncrementControl(mPlug, arrowX, arrowY + numberBoxArrowUp.H / 2, kBitDepth, &numberBoxArrowDown, -1));

		//--Bit Depth Number Box Label
		IRECT boxLabelSize(backSize.L,
			kVolumeLabel_Y,
			backSize.R,
			backSize.T);
		pGraphics->AttachControl(new ITextControl(mPlug, boxLabelSize, &kLabelTextStyle, "BITS"));
	}
}


Interface::~Interface()
{
}

void Interface::SetDirty(int paramIdx, bool pushToPlug)
{
	switch (paramIdx)
	{
	case kBitDepth:
		if (bitDepthControl)
		{
			bitDepthControl->SetDirty(pushToPlug);
		}
		break;
	}
}

const char * Interface::GetProgramText() const
{
	return textEdit->GetText();
}

void Interface::SetProgramText(const char * programText)
{
	textEdit->TextFromTextEntry(programText);
}

void Interface::SetConsoleText(const char * consoleText)
{
	consoleTextControl->SetTextFromPlug(const_cast<char*>(consoleText));
}
