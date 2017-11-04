#include "Interface.h"
#include "Evaluator.h"
#include "IControl.h"
#include "Controls.h"

enum ELayout
{
	kEditorWidth = GUI_WIDTH,
	kEditorHeight = GUI_HEIGHT,
	kEditorMargin = 10,

	kVolumeLabel_X = kEditorMargin*2,
	kVolumeLabel_Y = 10,
	kVolumeLabel_W = 30,
	kVolumeLabel_H = 15,

	kVolumeKnob_X = kVolumeLabel_X,
	kVolumeKnob_Y = kVolumeLabel_Y + kVolumeLabel_H,
	kVolumeKnob_W = kVolumeLabel_W,
	kVolumeKnob_H = kVolumeLabel_W,

	kBitDepth_X = kVolumeKnob_X + kVolumeKnob_W + 20,
	kBitDepth_Y = kVolumeKnob_Y,

	// rect for label & buttons
	kTimeType_X = kBitDepth_X + 60,
	kTimeType_Y = kVolumeLabel_Y,
	kTimeType_W = 27 * TTCount,
	kTimeType_H = kVolumeKnob_H + kVolumeLabel_H,

	kVControl_W = 30,
	kVControl_H = 30,
	kVControl_S = kVControl_W + 10,
	// x coord of the *first* knob on the left
	kVControl_X = kEditorWidth - kEditorMargin*2 - kVControl_S*(kVControl7 - kVControl0) - kVControl_W,
	kVControl_Y = kVolumeKnob_Y,

	kProgramLabel_X = kEditorMargin,
	kProgramLabel_Y = kVolumeKnob_Y + kVolumeKnob_H + 5,
	kProgramLabel_W = 75,
	kProgramLabel_H = 17,

	kProgramText_X = kEditorMargin,
	kProgramText_Y = kProgramLabel_Y + kProgramLabel_H,
	kProgramText_W = kEditorWidth - kEditorMargin*2,
	kProgramText_H = 200,

	kConsoleTitle_X = kProgramText_X,
	kConsoleTitle_Y = kProgramText_Y + kProgramText_H + 10,
	kConsoleTitle_W = kProgramText_W,
	kConsoleTitle_H = 15,

	// the log window that shows the internal state of the expression
	kConsole_X = kProgramText_X,
	kConsole_Y = kConsoleTitle_Y + kConsoleTitle_H,
	kConsole_W = 375,
	kConsole_H = 140,

	kConsole_M = 5,   // margin
	kConsole_TH = 12,  // text height
	kConsole_TW = kConsole_W - kConsole_M * 2, // text width

	kWatchLabel_X = kConsoleTitle_X + kConsole_W + 10,
	kWatchLabel_Y = kConsoleTitle_Y,
	kWatchLabel_W = 25,
	kWatchLabel_H = 15,

	// rect for a text edit
	kWatch_X = kWatchLabel_X,
	kWatch_Y = kConsole_Y + kConsole_M,
	kWatch_W = 50,
	kWatch_H = 11,
	kWatch_S = 2,

	// rect for the watch results
	kWatchWindow_X = kWatch_X + kWatch_W + 5,
	kWatchWindow_Y = kConsole_Y,
	kWatchWindow_W = kEditorWidth - 10 - kWatchWindow_X,
	kWatchWindow_H = kConsole_H,
  
  kScopeTitle_X = kConsoleTitle_X,
  kScopeTitle_Y = kConsole_Y + kConsole_H + 10,
  kScopeTitle_W = 60,
  kScopeTitle_H = 15,
  
  kScopeWindowLabel_X = kScopeTitle_X + kScopeTitle_W,
  kScopeWindowLabel_Y = kScopeTitle_Y + 2,
  kScopeWindowLabel_W = 52,
  kScopeWindowLabel_H = 15,
  
  kScopeWindow_X = kScopeWindowLabel_X + kScopeWindowLabel_W,
  kScopeWindow_Y = kScopeWindowLabel_Y - 3,
  kScopeWindow_W = 15,
  kScopeWindow_H = 15,

	kScope_X = kScopeTitle_X,
	kScope_Y = kScopeTitle_Y + kScopeTitle_H,
	kScope_W = kProgramText_W,
	kScope_H = 150,

};

// note: ICOLOR is ARGB
const IColor kBackgroundColor(255, 19, 44, 34);
const IColor kExprBackgroundColor(255, 32, 56, 59);
const IColor kTextColor(255, 218, 228, 226);
const IColor kGreenColor(255, 52, 209, 112);
const IColor kConsoleBackgroundColor(255, 11, 26, 19);
const IColor kScopeBackgroundColor(255, 14, 21, 26);
const IColor kScopeLineColorLeft(255, 194, 218, 150);
const IColor kScopeLineColorRight(255, 60, 169, 198);

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

IText  kConsoleTextStyle(11,
						&kGreenColor,
						"Courier",
						IText::kStyleNormal,
						IText::kAlignNear,
						0, // orientation
						IText::kQualityDefault);

IText  kWatchTextStyle(11,
						&kGreenColor,
						"Courier",
						IText::kStyleNormal,
						IText::kAlignNear,
						0, // orientation
						IText::kQualityDefault,
						&kExprBackgroundColor,
						&kGreenColor);

IText kLabelTextStyle(12,
					&kTextColor,
					"Arial",
					IText::kStyleBold,
					IText::kAlignCenter,
					0, // orientation
					IText::kQualityDefault);

IText kTitleTextStyle(16,
                      &kTextColor,
                      "Arial",
                      IText::kStyleBold,
                      IText::kAlignNear,
                      0, // orientation
                      IText::kQualityDefault);

static const char* kTModeDescription[] = {
	"increment 't' always",
	"increment 't' while note on",
	"increment 't' while note on, reset 't' every note on",
	"set 't' to project time"
};

Interface::Interface(Evaluator* plug, IGraphics* pGraphics)
: mPlug(plug)
{
	pGraphics->AttachPanelBackground(&kBackgroundColor);

	//--- Text input for the expression ------
	{
		pGraphics->AttachControl(new ITextControl(mPlug, MakeIRect(kProgramLabel), &kTitleTextStyle, "PROGRAM:"));
		textEdit = new ITextEdit(mPlug, MakeIRect(kProgramText), kExpression, &kExpressionTextStyle, "t*128", ETextEntryOptions(kTextEntryMultiline | kTextEntryEnterKeyInsertsCR));
		pGraphics->AttachControl(textEdit);
	}

	//-- "window" displaying internal state of the expression
	{
		pGraphics->AttachControl(new ITextControl(mPlug, MakeIRect(kConsoleTitle), &kTitleTextStyle, "AUTO"));
		IRECT LogRect = MakeIRect(kConsole);
		consoleTextControl = new ConsoleText(mPlug, LogRect, &kConsoleTextStyle, &kConsoleBackgroundColor, kConsole_M);
		pGraphics->AttachControl(consoleTextControl);
	}

	//-- watch window section
	{
		pGraphics->AttachControl(new ITextControl(mPlug, MakeIRect(kWatchLabel), &kTitleTextStyle, "WATCH"));

		watches = new ITextEdit*[kWatchNum];
		IRECT watchRect = MakeIRect(kWatch);
		for (int i = 0; i < kWatchNum; ++i)
		{
			watches[i] = new ITextEdit(mPlug, watchRect, kWatch + i, &kWatchTextStyle, "", kTextEntrySelectTextWhenFocused);
			watches[i]->SetTextEntryLength(5);
			pGraphics->AttachControl(watches[i]);
			watchRect.T += kWatch_H + kWatch_S;
			watchRect.B += kWatch_H + kWatch_S;
		}

		watchConsole = new ConsoleText(mPlug, MakeIRect(kWatchWindow), &kConsoleTextStyle, &kConsoleBackgroundColor, kConsole_M);
		pGraphics->AttachControl(watchConsole);
	}
	
	// -- Oscilloscope display
	{
		pGraphics->AttachControl(new ITextControl(mPlug, MakeIRect(kScopeTitle), &kTitleTextStyle, "SCOPE"));
		oscilloscope = new Oscilloscope(mPlug, MakeIRect(kScope), &kScopeBackgroundColor, &kScopeLineColorLeft, &kScopeLineColorRight);
		pGraphics->AttachControl(oscilloscope);
    
		pGraphics->AttachControl(new ITextControl(mPlug, MakeIRect(kScopeWindowLabel), &kLabelTextStyle, "WINDOW"));

		IRECT updateRect = MakeIRect(kScopeWindow);
		IControl* updateControl = new KnobLineCoronaControl(mPlug, updateRect, kScopeWindow, &kGreenColor, &kGreenColor, 0.5);

		int w = updateRect.W() + 15;
		updateRect.L += w;
		updateRect.R += w;
		updateRect.T = kScopeWindowLabel_Y;
		IControl* caption = new ICaptionControl(mPlug, updateRect, kScopeWindow, &kLabelTextStyle);
		updateControl->SetValDisplayControl(caption);

		pGraphics->AttachControl(updateControl);
		pGraphics->AttachControl(caption);
	}

	//---Volume--------------------
	{
    //---Volume Label--------
    pGraphics->AttachControl(new ITextControl(mPlug, MakeIRect(kVolumeLabel), &kLabelTextStyle, "VOL"));
    
		//---Volume Knob-------
    IColor coronaColor(kGreenColor);
		pGraphics->AttachControl(new KnobLineCoronaControl(mPlug, MakeIRect(kVolumeKnob), kGain, &kGreenColor, &coronaColor, 1.5, 0, 14));
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
		bitDepthControl = new ICaptionControl(mPlug, numberSize, kBitDepth, &kConsoleTextStyle);
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

	// ---Time Type------------------------------
	{
		IBitmap radioButton = pGraphics->LoadIBitmap(RADIO_BUTTON_ID, RADIO_BUTTON_FN, 2);
		IRECT buttonRect = MakeIRect(kTimeType);
		IText textStyle = kLabelTextStyle;
		textStyle.mAlign = IText::kAlignNear;

		ITextControl* label = new ITextControl(mPlug, buttonRect, &kLabelTextStyle, "T-MODE");

		IRECT captionRect = MakeIRect(kProgramLabel);
		captionRect.L += kProgramLabel_W;
		captionRect.R += kProgramLabel_W;
		// some fudge to the top of the rect so this smaller text looks vertically centered on the bigger PROGRAM: label
		captionRect.T += 2; 
		tmodeText = new ITextControl(mPlug, captionRect, &textStyle);

		buttonRect.T = kVolumeKnob_Y;
		buttonRect.B = kVolumeKnob_Y + kVolumeKnob_H;
		IRadioButtonsControl* radios = new IRadioButtonsControl(mPlug, buttonRect, kTimeType, TTCount, &radioButton, kHorizontal);

		pGraphics->AttachControl(label);
		pGraphics->AttachControl(radios);
		pGraphics->AttachControl(tmodeText);
	}

	// ---V Control Knobs-------------------------
	{
		IRECT knobRect = MakeIRect(kVControl);
		IRECT labelRect = MakeIRect(kVolumeLabel);
		for (int paramIdx = kVControl0; paramIdx <= kVControl7; ++paramIdx)
		{
			labelRect.L = knobRect.L;
			labelRect.R = knobRect.R;

			ITextControl* label = new ITextControl(mPlug, labelRect, &kLabelTextStyle, mPlug->GetParam(paramIdx)->GetNameForHost());
			KnobLineCoronaControl* knob = new KnobLineCoronaControl(mPlug, knobRect, paramIdx, &kGreenColor, &kGreenColor, 1.5, 0, 14);

			pGraphics->AttachControl(label);
			pGraphics->AttachControl(knob);

			knobRect.L += kVControl_S;
			knobRect.R += kVControl_S;
		}
	}
}


Interface::~Interface()
{
	delete[] watches;
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

	case kTimeType:
		if (tmodeText)
		{
			int tmodeIdx = mPlug->GetParam(kTimeType)->Int();
			tmodeText->SetTextFromPlug(const_cast<char*>(kTModeDescription[tmodeIdx]));
		}
		break;
	}
}

const char * Interface::GetProgramText() const
{
	return textEdit->GetText();
}

size_t Interface::GetProgramMemorySize() const
{
	return 1024 * 64;
}

void Interface::SetProgramText(const char * programText)
{
	textEdit->TextFromTextEntry(programText);
}

void Interface::SetConsoleText(const char * consoleText)
{
	consoleTextControl->SetTextFromPlug(const_cast<char*>(consoleText));
}

void Interface::SetWatchText(const char * watchText)
{
	watchConsole->SetTextFromPlug(const_cast<char*>(watchText));
}

void Interface::UpdateOscilloscope(double left, double right)
{
	oscilloscope->AddSample(left, right);
}

int Interface::GetOscilloscopeWidth() const
{
  return oscilloscope->GetRECT()->W();
}

const char * Interface::GetWatch(int idx) const
{
	return watches[idx]->GetText();
}

void Interface::SetWatch(int idx, const char * text) 
{
	if (idx >= 0 && idx < kWatchNum)
	{
		watches[idx]->SetTextFromPlug(text);
	}
}