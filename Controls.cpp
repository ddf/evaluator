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

#pragma region ITextEdit 
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
	IRECT textRect = mRECT.GetHPadded(-3);
	return pGraphics->DrawIText(&mText, mStr.Get(), &textRect);
}

void ITextEdit::OnMouseDown(int x, int y, IMouseMod* pMod)
{
	// stupid hack to prevent the edit window from displaying 1 pixel wider and taller
	IRECT entryRect = mRECT.GetPadded(0,0,-1,-1);
	IText entryText = mText;
#if defined(OS_OSX)
	entryRect.L += 3;
	entryText.mSize -= 4;
#endif
	mPlug->GetGUI()->CreateTextEntry(this, &entryText, &entryRect, mStr.Get());

	if (mNameDisplayControl)
	{
		mNameDisplayControl->Hide(false);
	}
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

	if (mNameDisplayControl)
	{
		mNameDisplayControl->Hide(true);
	}
}
#pragma  endregion ITextEdit

#pragma  region TextBox
TextBox::TextBox(IPlugBase* pPlug, IRECT pR, int paramIdx, IText* pText, IRECT textRect, bool showParamUnits)
	: ICaptionControl(pPlug, pR, paramIdx, pText, showParamUnits)
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
		IText ourText = mText;
		IRECT promptRect = mTextRect;
#if defined(OS_OSX)
		mText.mSize  -= 2;
		promptRect.T -= 1;
#endif
		mPlug->GetGUI()->PromptUserInput(this, mPlug->GetParam(mParamIdx), &promptRect);
		mText = ourText;
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
#pragma  endregion TextBox

#pragma  region TextTable
TextTable::TextTable(IPlugBase* pPlug, IRECT pR, IText* pText, const char** data, int iColumns, int iRows)
	: IControl(pPlug, pR)
	, tableData(data)
	, columns(iColumns)
	, rows(iRows)
{
	SetText(pText);
}

bool TextTable::Draw(IGraphics* pGraphics)
{
	for (int r = 0; r < rows; ++r)
	{
		IRECT rowRect = mRECT.SubRectVertical(rows, r);
		for (int c = 0; c < columns; ++c)
		{
			IRECT cellRect = rowRect.SubRectHorizontal(columns*2, c);
			char* contents = const_cast<char*>(tableData[r*columns + c]);
			pGraphics->DrawIText(&mText, contents, &cellRect);
		}
	}

	return true;
}
#pragma  endregion TextTable

#pragma  region ConsoleText
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
#pragma  endregion ConsoleText

#pragma region EnumControl (used for RunMode)
EnumControl::EnumControl(IPlugBase* pPlug, IRECT rect, int paramIdx, IText* textStyle)
	: IControl(pPlug, rect, paramIdx)
{
	SetText(textStyle);
	mDblAsSingleClick = true;
	mDisablePrompt = false;
}

bool EnumControl::Draw(IGraphics* pGraphics)
{
	pGraphics->FillIRect(&mText.mTextEntryBGColor, &mRECT);
	pGraphics->DrawRect(&mText.mTextEntryFGColor, &mRECT);
	static char display[16];
	GetParam()->GetDisplayForHost(display);
	IRECT textRect = mRECT;
	textRect.T += 3;
	pGraphics->DrawIText(&mText, display, &textRect);

	return true;
}

void EnumControl::OnMouseDown(int x, int y, IMouseMod* pMod)
{
	if (pMod->R)
	{
		PromptUserInput();
	}
	else
	{
		int count = GetParam()->GetNDisplayTexts();
		if (count > 1)
		{
			mValue += 1.0 / (double)(count - 1);
		}
		else
		{
			mValue += 1.0;
		}

		if (mValue > 1.001)
		{
			mValue = 0.0;
		}
		SetDirty();
	}
}
#pragma  endregion EnumControl

#pragma  region KnobLineCoronaControl
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

void KnobLineCoronaControl::OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod)
{
	double gearing = mGearing;

#ifdef PROTOOLS
#ifdef OS_WIN
	if (pMod->C) gearing *= 10.0;
#else
	if (pMod->R) gearing *= 10.0;
#endif
#else
	if (pMod->C || pMod->S) gearing *= 10.0;
#endif

	mValue += (double)dY / (double)(mRECT.T - mRECT.B) / gearing;
	mValue += (double)dX / (double)(mRECT.R - mRECT.L) / gearing;

	SetDirty();
}
#pragma  endregion KnobLineCoronaControl

#pragma  region Oscilloscope
static const int gridLineWidth = 4;
Oscilloscope::Oscilloscope(IPlugBase* pPlug, IRECT pR, const IColor* backgroundColor, const IColor* lineColorLeft, const IColor* lineColorRight)
	: IControl(pPlug, pR)
	, mBackgroundColor(*backgroundColor)
	, mLineColorLeft(*lineColorLeft)
	, mLineColorRight(*lineColorRight)
{
	mBufferSize = pR.W() * 2;
	// buffer size for Grid
	//mBufferSize = pR.W() * pR.H() / gridLineWidth + pR.H();
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

	DrawWaveform(pGraphics);
	//DrawGrid(pGraphics);

	return true;
}

void Oscilloscope::DrawWaveform(IGraphics* pGraphics)
{
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
}

void Oscilloscope::DrawGrid(IGraphics* pGraphics)
{
	const float midY = mRECT.MH();
	const int  halfH = mRECT.H() / 2;
	const int  columns = mRECT.W() / gridLineWidth;
	const int  columnH = halfH*2;
	const int  readStart = (mBufferBegin + columnH - 1) / columnH * columnH;
	
	for(int x = 0; x < columns; ++x)
	{
		const int off = x*halfH*2;
		const float px = (float)(mRECT.L + x*gridLineWidth);
		for(int i = 0; i < halfH; ++i)
		{
			const int lidx = (readStart + off + i * 2) % mBufferSize;
			const int ridx = lidx + 1;
			const float pyl = midY - i;
			const float pyr = midY + halfH - i;
			
			float fade = (mBuffer[lidx]+1) / 2.0f;
			IColor colorLeft(mLineColorLeft.A,
							 (int)((float)mLineColorLeft.R*fade),
							 (int)((float)mLineColorLeft.G*fade),
							 (int)((float)mLineColorLeft.B*fade));
			
			fade = (mBuffer[ridx]+1) / 2.0f;
			IColor colorRight(mLineColorRight.A,
							 (int)((float)mLineColorRight.R*fade),
							 (int)((float)mLineColorRight.G*fade),
							 (int)((float)mLineColorRight.B*fade));
			
			pGraphics->DrawLine(&colorLeft, px, pyl, px+gridLineWidth, pyl);
			pGraphics->DrawLine(&colorRight, px, pyr, px+gridLineWidth, pyr);
		}
	}
}

void Oscilloscope::AddSample(double left, double right)
{
	mBuffer[mBufferBegin] = left;
	mBuffer[mBufferBegin + 1] = right;
	mBufferBegin = (mBufferBegin + 2) % mBufferSize;
	SetDirty(false);
}
#pragma  endregion Oscilloscope

#pragma  region LoadButton
static const int kMenuPadding = 5;
static char* kLoadText = "LOAD...";
LoadButton::LoadButton(IPlugBase* pPlug, int x, int y, IBitmap* pButtonBack, IText* pButtonTextStyle, IRECT menuRect, IText* pMenuTextStyle, Interface* pInterface)
	: IBitmapControl(pPlug, x, y, -1, pButtonBack)
	, mState(kClosed)
	, mButtonRect(x,y,pButtonBack)
	, mTextRect(mButtonRect)
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

	mInterface->GetGUI()->MeasureIText(&mButtonText, kLoadText, &mTextRect);
	mTextRect.T += (mButtonRect.H() - mTextRect.H()) / 2;
}

bool LoadButton::Draw(IGraphics* pGraphics)
{
	pGraphics->DrawBitmap(&mBitmap, &mButtonRect, 1, &mBlend);
	pGraphics->DrawIText(&mButtonText, kLoadText, &mTextRect);

	if (mState == kOpen)
	{
		IColor fadeColor(128, 0, 0, 0);
		pGraphics->FillIRect(&fadeColor, &mTargetRECT);
		pGraphics->FillIRect(&COLOR_BLACK, &mMenuRect);
		pGraphics->DrawRect(&mMenuText.mColor, &mMenuRect);
		IColor selectionColor(255, 30, 30, 30);
		char text[128];
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
				const auto& preset = Presets::Get(i);
				if (preset.runMode == kRunModeMIDI || preset.midiNoteResetsTime)
				{
					sprintf(text, "%s [MIDI]", preset.name);
				}
				else
				{
					sprintf(text, "%s", preset.name);
				}
				pGraphics->DrawIText(&mMenuText, text, &itemRect);
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
#pragma  endregion LoadButton

#pragma region SaveButton
static char* kSaveText = "SAVE...";
SaveButton::SaveButton(IPlugBase* pPlug, int x, int y, IBitmap* pButtonBack, IText* pButtonTextStyle, Interface* pInterface)
	: IBitmapControl(pPlug, x, y, -1, pButtonBack)
	, mButtonText(*pButtonTextStyle)
	, mTextRect(mRECT)
	, mInterface(pInterface)
{
	mInterface->GetGUI()->MeasureIText(&mButtonText, kSaveText, &mTextRect);
	mTextRect.T += (mRECT.H() - mTextRect.H()) / 2;
}

bool SaveButton::Draw(IGraphics* pGraphics)
{
	pGraphics->DrawBitmap(&mBitmap, &mRECT, 1, &mBlend);
	pGraphics->DrawIText(&mButtonText, kSaveText, &mTextRect);

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
#pragma  endregion SaveButton

#pragma  region ManualButton
static char* kManualText = "MANUAL";
ManualButton::ManualButton(IPlugBase* pPlug, int x, int y, IBitmap* pButtonBack, IText* pButtonTextStyle, Interface* pInterface)
: IBitmapControl(pPlug, x, y, -1, pButtonBack)
, mButtonText(*pButtonTextStyle)
, mTextRect(mRECT)
, mInterface(pInterface)
{
	mInterface->GetGUI()->MeasureIText(&mButtonText, kManualText, &mTextRect);
	mTextRect.T += (mRECT.H() - mTextRect.H()) / 2;
}

bool ManualButton::Draw(IGraphics* pGraphics)
{
	pGraphics->DrawBitmap(&mBitmap, &mRECT, 1, &mBlend);
	pGraphics->DrawIText(&mButtonText, kManualText, &mTextRect);
	
	return true;
}

void ManualButton::OnMouseDown(int x, int y, IMouseMod* pMod)
{
	WDL_String fileName("");
	bool success = false;

	// check in the same folder as the app first because
	// this is where it will be if installed by Itch
	// or if the user downloaded the standalone directly.
#if SA_API
	mPlug->GetGUI()->HostPath(&fileName);
	fileName.Append("/Evaluator_manual.pdf");
	success = mPlug->GetGUI()->OpenURL(fileName.Get());
#endif
	
	// if we didn't find it, check in the support path,
	// which is where it will be if this was installed
	// by the installer.
	if ( !success )
	{
		success = mInterface->GetSupportPath(&fileName);

		if (success)
		{
			fileName.Append("/Evaluator_manual.pdf");
			success = mPlug->GetGUI()->OpenURL(fileName.Get());
		}
	}

	if ( !success )
	{
		static char msg[256];
		sprintf(msg, "Sorry, couldn't find the manual at %s.", fileName.Get());
		mPlug->GetGUI()->ShowMessageBox(msg, "Error", MB_OK);
	}
}
#pragma  endregion ManualButton

#pragma  region HelpButton
static char* kHelpText = "?";
HelpButton::HelpButton(IPlugBase* pPlug, IRECT rect, IText* pButtonTextStyle, Interface* pInterface)
	: IControl(pPlug, rect)
	, mButtonText(*pButtonTextStyle)
	, mTextRect(mRECT)
	, mInterface(pInterface)
{
	mInterface->GetGUI()->MeasureIText(&mButtonText, kHelpText, &mTextRect);
	mTextRect.T += (mRECT.H() - mTextRect.H()) / 2;
	// haxxxxx
#if defined(OS_OSX)
	mTextRect.T += 1;
#endif
}

bool HelpButton::Draw(IGraphics* pGraphics)
{
	if ( mValue )
	{
		pGraphics->FillIRect(&mButtonText.mTextEntryFGColor, &mRECT);
		pGraphics->DrawRect(&mButtonText.mTextEntryBGColor, &mRECT);
	}
	else
	{
		pGraphics->FillIRect(&mButtonText.mTextEntryBGColor, &mRECT);
		pGraphics->DrawRect(&mButtonText.mTextEntryFGColor, &mRECT);
	}

	pGraphics->DrawIText(&mButtonText, kHelpText, &mTextRect);

	return true;
}

void HelpButton::OnMouseDown(int x, int y, IMouseMod* pMod)
{
	mInterface->ToggleHelp();
	mValue = 1 - mValue;
}
#pragma  endregion HelpButton

#pragma  region TransportButtons
TransportButtons::TransportButtons(IPlugBase* pPlug, IRECT rect, const IColor& backgroundColor, const IColor& foregroundColor)
	: IControl(pPlug, rect)
	, mState(kTransportStopped)
	, mBack(backgroundColor)
	, mFore(foregroundColor)
{
	mDblAsSingleClick = true;
	mPlayRect = rect.SubRectHorizontal(3, 0);
	mPauseRect = rect.SubRectHorizontal(3, 1);
	mStopRect = rect.SubRectHorizontal(3, 2);
}

bool TransportButtons::Draw(IGraphics* pGraphics)
{
	// play button
	{
		const bool active = (mState == kTransportPlaying || mState == kTransportPaused);
		const int cx = (int)mPlayRect.MW();
		const int cy = (int)mPlayRect.MH();
		IRECT back = mPlayRect.GetPadded(-1);
		pGraphics->FillIRect(active ? &mFore : &mBack, &back);
		pGraphics->FillTriangle(active ? &mBack : &mFore,
			cx - 6, cy - 5,
			cx - 6, cy + 5,
			cx + 8, cy,
			&mBlend
		);
	}

	// pause button
	{
		bool active = mState == kTransportPaused;
		IRECT back = mPauseRect.GetPadded(-1);
		pGraphics->FillIRect(active ? &mFore : &mBack, &back);
		const IColor* color = active ? &mBack : &mFore;
		const int cx = (int)mPauseRect.MW();
		const int cy = (int)mPauseRect.MH();
		const int w = 4;
		const int h = 6;
		IRECT slab(cx - (int)(w*1.5f), cy - h, cx - (int)(w*0.5f), cy + h);
		pGraphics->FillIRect(color, &slab, &mBlend);
		slab.L += w*2;
		slab.R += w*2;
		pGraphics->FillIRect(color, &slab, &mBlend);
	}

	// stop button
	{
		const int cx = (int)mStopRect.MW();
		const int cy = (int)mStopRect.MH();
		const int r = 6;
		IRECT back = mStopRect.GetPadded(-1);
		IRECT button(cx - r, cy - r, cx + r, cy + r);
		pGraphics->FillIRect(&mBack, &back);
		pGraphics->FillIRect(&mFore, &button);
	}

	return true;
}

void TransportButtons::OnMouseDown(int x, int y, IMouseMod* pMod)
{
	if (mPlayRect.Contains(x, y))
	{
		// if pause we stop and then start
		// so that tick will be reset to zero.
		// this matches transport behavior in reaper
		// where pressing the play button while paused
		// will cause it to start playing from the timeline marker
		// rather than resuming from the current playhead position
		if ( mState == kTransportPaused )
		{
			SetTransportState(kTransportStopped);
		}
		SetTransportState(kTransportPlaying);
	}
	else if (mPauseRect.Contains(x, y))
	{
		if ( mState == kTransportPaused )
		{
			SetTransportState(kTransportPlaying);
		}
		else
		{
			SetTransportState(kTransportPaused);
		}
	}
	else if (mStopRect.Contains(x, y))
	{
		SetTransportState(kTransportStopped);
	}
}

void TransportButtons::SetTransportState( TransportState state )
{
	mState = state;
	SetDirty(false);
	mPlug->OnParamChange(kTransportState);
}

TransportState TransportButtons::GetTransportState() const
{
	return mState;
}
#pragma  endregion TransportButtons

#pragma  region ToggleControl
ToggleControl::ToggleControl(IPlugBase* pPlug, IRECT rect, int paramIdx, IColor backgroundColor, IColor fillColor)
	: IControl(pPlug, rect, paramIdx)
{
	mText.mTextEntryBGColor = backgroundColor;
	mText.mTextEntryFGColor = fillColor;
}

bool ToggleControl::Draw(IGraphics* pGraphics)
{
	pGraphics->FillIRect(&mText.mTextEntryBGColor, &mRECT);
	pGraphics->DrawRect(&mText.mTextEntryFGColor, &mRECT);
	
	if (mValue)
	{
		IRECT fill = mRECT.GetPadded(-2);
		fill.T += 1;
		fill.L += 1;
		pGraphics->FillIRect(&mText.mTextEntryFGColor, &fill);
	}

	return true;
}

void ToggleControl::OnMouseDown(int x, int y, IMouseMod* pMod)
{
	mValue = 1 - mValue;
	SetDirty();
}
#pragma  endregion ToggleControl

#pragma  region MidiControl
int KeyToNote(int key)
{
	int note = -1;
	if (key >= KEY_DIGIT_0 && key <= KEY_DIGIT_9)
	{
		key = '0' + key - KEY_DIGIT_0;
	}
	else if (key >= KEY_ALPHA_A && key <= KEY_ALPHA_Z)
	{
		key += 50;
	}
	switch (key)
	{
	case 'Z': note = 48; break;
	case 'S': note = 49; break;
	case 'X': note = 50; break;
	case 'D': note = 51; break;
	case 'C': note = 52; break;
	case 'V': note = 53; break;
	case 'G': note = 54; break;
	case 'B': note = 55; break;
	case 'H': note = 56; break;
	case 'N': note = 57; break;
	case 'J': note = 58; break;
	case 'M': note = 59; break;
	case 'K': note = 60; break;

	case 'Q': note = 60; break;
	case '2': note = 61; break;
	case 'W': note = 62; break;
	case '3': note = 63; break;
	case 'E': note = 64; break;
	case 'R': note = 65; break;
	case '5': note = 66; break;
	case 'T': note = 67; break;
	case '6': note = 68; break;
	case 'Y': note = 69; break;
	case '7': note = 70; break;
	case 'U': note = 71; break;
	case 'I': note = 72; break;
	case '9': note = 73; break;
	case 'O': note = 74; break;
	case '0': note = 75; break;
	case 'P': note = 76; break;

	default:
		break;
	}
	return note;
}

MidiControl::MidiControl(IPlugBase* pPlug)
	: IControl(pPlug, IRECT(0,0,0,0))
{

}

bool MidiControl::OnKeyDown(int x, int y, int key)
{
	const int note = KeyToNote(key);
	const bool handled = note != -1;
	if (handled)
	{
		IMidiMsg msg;
		msg.MakeNoteOnMsg(note, 127, 0);
		mPlug->ProcessMidiMsg(&msg);
		msg.MakeNoteOffMsg(note, (int)mPlug->GetSampleRate()/8);
		mPlug->ProcessMidiMsg(&msg);
	}

	return handled;
}
#pragma  endregion MidiControl
