//
//  Controls.cpp
//  Evaluator
//
//  Created by Damien Quartz on 10/23/17.
//

#include "Controls.h"

#include "Evaluator.h"
#include "Interface.h"
#include "Presets.h"

////////////////////////////////////////////
// ITextEdit
//
ITextEdit::ITextEdit(IPlugBase* pPlug, IRECT pR, int paramIdx, IText* pText, const char* str, ETextEntryOptions textEntryOptions)
	: IControl(pPlug, pR)
	, mIdx(paramIdx)
{
	mDisablePrompt = true;
	mText = *pText;
	mStr.Set(str);
	mTextEntryLength = kExpressionLengthMax;
	mTextEntryOptions = textEntryOptions;
}

ITextEdit::~ITextEdit() {}

bool ITextEdit::Draw(IGraphics* pGraphics)
{
	pGraphics->FillIRect(&mText.mTextEntryBGColor, &mRECT);
	return pGraphics->DrawIText(&mText, mStr.Get(), &mRECT);
}

void ITextEdit::OnMouseDown(int x, int y, IMouseMod* pMod)
{
	mPlug->GetGUI()->CreateTextEntry(this, &mText, &mRECT, mStr.Get());
}

void ITextEdit::TextFromTextEntry(const char* txt)
{
	// don't set if it is the same, we don't want the project to become modified in this case
	if (strcmp(mStr.Get(), txt))
	{
		mStr.Set(txt);
		SetDirty(false);
	
		mPlug->OnParamChange(mIdx);
		// we have to call this or else the host will not mark the project as modified
		mPlug->InformHostOfParamChange(mIdx, 0);
	}
}
//
/////////////////////////////////////

/////////////////////////////////////
// ConsoleText
//
TextBox::TextBox(IPlugBase* pPlug, IRECT pR, int paramIdx, IText* pText, IRECT textRect)
	: ICaptionControl(pPlug, pR, paramIdx, pText, false)
	, mTextRect(textRect)
{
}

bool TextBox::Draw(IGraphics* pGraphics)
{
	pGraphics->FillIRect(&mText.mTextEntryBGColor, &mRECT);
	pGraphics->DrawRect(&mText.mTextEntryFGColor, &mRECT);

	IRECT ourRect = mRECT;
	mRECT = mTextRect;
	bool result = ICaptionControl::Draw(pGraphics);
	mRECT = ourRect;

	return result;
}

void TextBox::OnMouseDown(int x, int y, IMouseMod* pMod)
{
	if (pMod->L)
	{
		mPlug->GetGUI()->PromptUserInput(this, mPlug->GetParam(mParamIdx), &mTextRect);
		Redraw();
	}
}

void TextBox::OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod)
{
	if (pMod->R)
	{
		OnMouseWheel(x, y, pMod, -dY);
	}
}

void TextBox::OnMouseWheel(int x, int y, IMouseMod* pMod, int d)
{
#ifdef PROTOOLS
	if (pMod->C)
	{
		mValue += GetParam()->GetStep() * 0.001 * d;
	}
#else
	if (pMod->C || pMod->S)
	{
		mValue += GetParam()->GetStep() * 0.001 * d;
	}
#endif
	else
	{
		mValue += GetParam()->GetStep() * 0.01 * d;
	}

	SetDirty();
}

//
/////////////////////////////////////

/////////////////////////////////////
// ConsoleText
//
ConsoleText::ConsoleText(IPlugBase* plug, IRECT pR, IText* textStyle, const IColor* backgroundColor, int margin)
	: IControl(plug, pR)
	, mPanel(plug, pR, backgroundColor)
	, mText(plug, pR.GetPadded(-margin), textStyle, "")
{}

bool ConsoleText::Draw(IGraphics* pGraphics)
{
	return mPanel.Draw(pGraphics) && mText.Draw(pGraphics);
}

void ConsoleText::SetTextFromPlug(const char * text)
{
	mText.SetTextFromPlug(const_cast<char*>(text));
	if (mText.IsDirty())
	{
		SetDirty(false);
		Redraw();
	}
}
//
////////////////////////////////////

////////////////////////////////////
// IncrementControl (used for numberboxx)
//
IIncrementControl::IIncrementControl(IPlugBase* pPlug, int x, int y, int paramIdx, IBitmap* pBitmap, int direction)
	: IBitmapControl(pPlug, x, y, paramIdx, pBitmap)
	, mPressed(0)
{
	IParam* param = GetParam();
	mInc = direction * 1.0 / (param->GetMax() - param->GetMin());
	mDblAsSingleClick = true;
}

bool IIncrementControl::Draw(IGraphics* pGraphics)
{
	return pGraphics->DrawBitmap(&mBitmap, &mRECT, mPressed + 1, &mBlend);
}

void IIncrementControl::OnMouseDown(int x, int y, IMouseMod* pMod)
{
	mPressed = 1;
	mValue = GetParam()->GetNormalized() + mInc;
	SetDirty();
}

void IIncrementControl::OnMouseUp(int x, int y, IMouseMod* pMod)
{
	mPressed = 0;
}
//
//////////////////////////////////////////

//////////////////////////////////////////
// KnobLineCoronaControl
//
KnobLineCoronaControl::KnobLineCoronaControl(IPlugBase* pPlug, IRECT pR, int paramIdx,
	const IColor* pColor, const IColor* pCoronaColor,
	float coronaThickness,
	double innerRadius, double outerRadius,
	double minAngle, double maxAngle,
	EDirection direction, double gearing)
	: IKnobLineControl(pPlug, pR, paramIdx, pColor, innerRadius, outerRadius, minAngle, maxAngle, direction, gearing)
	, mCoronaColor(*pCoronaColor)
	, mCoronaBlend(IChannelBlend::kBlendAdd, coronaThickness)
{
}

bool KnobLineCoronaControl::Draw(IGraphics* pGraphics)
{
	float cx = mRECT.MW(), cy = mRECT.MH();
	float v = mMinAngle + (float)mValue * (mMaxAngle - mMinAngle);
	for (int i = 0; i <= mCoronaBlend.mWeight; ++i)
	{
		IColor color = mCoronaColor;
		pGraphics->DrawArc(&color, cx, cy, mOuterRadius - i, mMinAngle, v, nullptr, true);
		color.R /= 2;
		color.G /= 2;
		color.B /= 2;
		pGraphics->DrawArc(&color, cx, cy, mOuterRadius - i, v, mMaxAngle, nullptr, true);
	}
	return IKnobLineControl::Draw(pGraphics);
}
//
//////////////////////////////////////////

//////////////////////////////////////////
// Oscilloscope
Oscilloscope::Oscilloscope(IPlugBase* pPlug, IRECT pR, const IColor* backgroundColor, const IColor* lineColorLeft, const IColor* lineColorRight)
	: IControl(pPlug, pR)
	, mBackgroundColor(*backgroundColor)
	, mLineColorLeft(*lineColorLeft)
	, mLineColorRight(*lineColorRight)
{
	mBufferSize = pR.W() * 2;
	mBuffer = new double[mBufferSize];
	memset(mBuffer, 0, mBufferSize*sizeof(double));
	mBufferBegin = 0;
}

Oscilloscope::~Oscilloscope()
{
	delete[] mBuffer;
}

bool Oscilloscope::Draw(IGraphics* pGraphics)
{
	pGraphics->FillIRect(&mBackgroundColor, &mRECT, &mBlend);

	const float midY = mRECT.MH();
	const float halfH = mRECT.H()*0.5f;
	float px1 = (float)mRECT.L;
	float pyl1 = midY;
	float pyr1 = midY;

	pGraphics->DrawLine(&COLOR_GRAY, (float)mRECT.L, midY, (float)mRECT.R, midY);
	IChannelBlend lineBlend = IChannelBlend(IChannelBlend::kBlendAdd);
	const float fade = 0.25f;
	IColor lineGhostLeft(mLineColorLeft.A, 
		(int)((float)mLineColorLeft.R*fade),
		(int)((float)mLineColorLeft.G*fade),
		(int)((float)mLineColorLeft.B*fade));

	IColor lineGhostRight(mLineColorRight.A, 
		(int)((float)mLineColorRight.R*fade),
		(int)((float)mLineColorRight.G*fade),
		(int)((float)mLineColorRight.B*fade));

	for (int x = 0; x < mRECT.W(); x++)
	{
		const int lidx = (mBufferBegin + x * 2) % mBufferSize;
		const int ridx = lidx + 1;
		const float px2 = (float)(mRECT.L + x);
		const float pyl2 = midY - (float)mBuffer[lidx] * halfH;
		const float pyr2 = midY - (float)mBuffer[ridx] * halfH;

		pGraphics->DrawLine(&lineGhostLeft, px2, midY, px2, pyl2, &lineBlend);
		pGraphics->DrawLine(&lineGhostRight, px2, midY, px2, pyr2, &lineBlend);

		pGraphics->DrawLine(&mLineColorLeft, px1, pyl1, px2, pyl2, &lineBlend, true);
		pGraphics->DrawLine(&mLineColorRight, px1, pyr1, px2, pyr2, &lineBlend, true);

		px1 = px2;
		pyl1 = pyl2;
		pyr1 = pyr2;
	}

	return true;
}

void Oscilloscope::AddSample(double left, double right)
{
	mBuffer[mBufferBegin] = left;
	mBuffer[mBufferBegin + 1] = right;
	mBufferBegin = (mBufferBegin + 2) % mBufferSize;
	SetDirty(false);
}
//
//////////////////////////////////////////

//////////////////////////////////////////
// Load Button
static const int kMenuPadding = 5;
LoadButton::LoadButton(IPlugBase* pPlug, int x, int y, IBitmap* pButtonBack, IText* pButtonTextStyle, IRECT menuRect, IText* pMenuTextStyle, Interface* pInterface)
	: IBitmapControl(pPlug, x, y, -1, pButtonBack)
	, mState(kClosed)
	, mButtonRect(x,y,pButtonBack)
	, mMenuRect(menuRect.GetPadded(kMenuPadding))
	, mButtonText(*pButtonTextStyle)
	, mMenuText(*pMenuTextStyle)
	, mInterface(pInterface)
{
	// we make the entire window our rect, otherwise our background fade doesn't render correctly
	mRECT = IRECT(0, 0, GUI_WIDTH, GUI_HEIGHT);
	// but only our button should get mouse clicks to begin with
	SetTargetArea(mButtonRect);
	mDisablePrompt = true;
	mMenuRect.T += kMenuPadding;
	mMenuRect.B += kMenuPadding;
	const int selectionCount = Presets::Count() + 1;
	const int selectionHeight = menuRect.H() / (selectionCount-1);
	for (int i = 0; i < selectionCount; ++i)
	{
		const int T = mMenuRect.T + kMenuPadding + i*selectionHeight;
		IRECT rect = IRECT(menuRect.L + kMenuPadding, T, menuRect.R - kMenuPadding, T + selectionHeight);
		mRECTs.Add(rect);
	}

	mMenuRect.B += kMenuPadding*2;
}

bool LoadButton::Draw(IGraphics* pGraphics)
{
	pGraphics->DrawBitmap(&mBitmap, &mButtonRect, 1, &mBlend);
	IRECT textRect(mButtonRect);
	textRect.T += 2; // fudge so the text looks vertically centered
	pGraphics->DrawIText(&mButtonText, "LOAD...", &textRect);

	if (mState == kOpen)
	{
		IColor fadeColor(128, 0, 0, 0);
		pGraphics->FillIRect(&fadeColor, &mTargetRECT);
		pGraphics->FillIRect(&COLOR_BLACK, &mMenuRect);
		pGraphics->DrawRect(&mMenuText.mColor, &mMenuRect);
		IColor selectionColor(255, 30, 30, 30);
		for (int i = 0; i < mRECTs.GetSize(); ++i)
		{
			IRECT itemRect = mRECTs.Get()[i];
			if (i == mSelection)
			{
				IRECT itemBack(itemRect);
				itemBack.L -= 2;
				itemBack.R += 2;
				pGraphics->FillIRect(&selectionColor, &itemBack);
			}
			if (i < Presets::Count())
			{
				pGraphics->DrawIText(&mMenuText, const_cast<char*>(Presets::Get(i).name), &itemRect);
			}
			else
			{
				pGraphics->DrawIText(&mMenuText, "load from file...", &itemRect);
			}
		}

		Redraw();
	}

	return true;
}

void LoadButton::OnMouseDown(int x, int y, IMouseMod* pMod)
{
	switch (mState)
	{
	case kClosed:
		mState = kOpen;
		// now we want to capture the mouse in the entire window
		SetTargetArea(mRECT);
		mSelection = -1;
		SetDirty(false);
		mPlug->GetGUI()->HandleMouseOver(true);
		break;

	case kOpen:
		mState = kClosed;
		SetTargetArea(mButtonRect);
		SetDirty(false);
		if (mSelection >= 0 )
		{
			if (mSelection < Presets::Count())
			{
				mInterface->LoadPreset(mSelection);
			}
			else
			{
				WDL_String fileName("");
				WDL_String directory("");
				mPlug->GetGUI()->PromptForFile(&fileName, kFileOpen, &directory, "txt fxp");
				if (fileName.GetLength() > 0)
				{
					if (strcmp(fileName.get_fileext(), ".fxp") == 0)
					{
						mPlug->LoadProgramFromFXP(&fileName);
						mInterface->SetProgramName(fileName.get_filepart());
					}
					else
					{
						FILE* fp = fopen(fileName.Get(), "rb");

						if (fp)
						{
							long fileSize;

							fseek(fp, 0, SEEK_END);
							fileSize = ftell(fp);
							rewind(fp);

							char* contents = new char[fileSize + 1];
							fread(contents, fileSize, 1, fp);

							fclose(fp);

							contents[fileSize] = 0;

							mInterface->SetProgramName(fileName.get_filepart());
							mInterface->SetProgramText(contents);
							// note: for some reason this doesn't set the Reaper project as modified in this case.
							mPlug->DirtyParameters();
							delete[] contents;
						}
					}
				}
			}
		}
		mPlug->GetGUI()->HandleMouseOver(false);
		break;
	}
}

void LoadButton::OnMouseOver(int x, int y, IMouseMod* pMod)
{
	if (mState == kOpen)
	{
		mSelection = -1;
		for (int i = 0; i < mRECTs.GetSize(); ++i)
		{
			if (mRECTs.Get()[i].Contains(x, y))
			{
				mSelection = i;
			}
		}
	}
}
//
//////////////////////////////////////////

SaveButton::SaveButton(IPlugBase* pPlug, int x, int y, IBitmap* pButtonBack, IText* pButtonTextStyle, Interface* pInterface)
	: IBitmapControl(pPlug, x, y, -1, pButtonBack)
	, mButtonText(*pButtonTextStyle)
	, mInterface(pInterface)
{

}

bool SaveButton::Draw(IGraphics* pGraphics)
{
	pGraphics->DrawBitmap(&mBitmap, &mRECT, 1, &mBlend);
	IRECT textRect(mRECT);
	textRect.T += 2; // fudge so the text looks vertically centered
	pGraphics->DrawIText(&mButtonText, "SAVE...", &textRect);

	return true;
}

void SaveButton::OnMouseDown(int x, int y, IMouseMod* pMod)
{
	WDL_String fileName("");
	WDL_String directory("");
	mPlug->GetGUI()->PromptForFile(&fileName, kFileSave, &directory, "fxp");
	if (fileName.GetLength() > 0)
	{
		mPlug->SaveProgramAsFXP(&fileName);
	}
}

/////////////////////////////////////////////////////////////////
// TransportButtons
TransportButtons::TransportButtons(IPlugBase* pPlug, IRECT rect, const IColor& backgroundColor, const IColor& foregroundColor)
	: IControl(pPlug, rect)
	, mState(kTransportStopped)
	, mBack(backgroundColor)
	, mFore(foregroundColor)
{
	mPlayRect = rect.SubRectHorizontal(3, 0);
	mPauseRect = rect.SubRectHorizontal(3, 1);
	mStopRect = rect.SubRectHorizontal(3, 2);
}

bool TransportButtons::Draw(IGraphics* pGraphics)
{
	// play button
	{
		bool active = mState == kTransportPlaying || kTransportPaused;
		pGraphics->FillIRect(active ? &mFore : &mBack, &mPlayRect);
		pGraphics->FillTriangle(active ? &mBack : &mFore,
			mPlayRect.L + 5, mPlayRect.T + 5, 
			mPlayRect.L + 5, mPlayRect.B - 5, 
			mPlayRect.R - 10, mPlayRect.MW(),
			&mBlend
		);
	}

	// pause button
	{
		bool active = mState == kTransportPaused;
		pGraphics->FillIRect(active ? &mFore : &mBack, &mPauseRect);
		const IColor* color = active ? &mBack : &mFore;
		IRECT slab = mPauseRect.SubRectHorizontal(5, 1);
		slab.T += 5;
		slab.B -= 5;
		pGraphics->FillIRect(color, &slab, &mBlend);
		slab = mPauseRect.SubRectHorizontal(5, 3);
		slab.T += 5;
		slab.B -= 5;
		pGraphics->FillIRect(color, &slab, &mBlend);
	}

	// stop button
	{
		const int cx = mStopRect.MW();
		const int cy = mStopRect.MH();
		const int r = 10;
		IRECT button(cx - 5, cy - r, cx + 5, cy + 5);
		pGraphics->FillIRect(&mBack, &mStopRect);
		pGraphics->FillIRect(&mFore, &button);
	}

	return true;
}

void TransportButtons::OnMouseDown(int x, int y, IMouseMod* pMod)
{
	if (mPlayRect.Contains(x, y))
	{
		mState = kTransportPlaying;
		mPlug->OnParamChange(kTransportState);
	}
	else if (mPauseRect.Contains(x, y))
	{
		mState = kTransportPaused;
		mPlug->OnParamChange(kTransportState);
	}
	else if (mStopRect.Contains(x, y))
	{
		mState = kTransportStopped;
		mPlug->OnParamChange(kTransportState);
	}
}

TransportState TransportButtons::GetTransportState() const
{
	return mState;
}

//////////////////////////////////////////////////////////////////
