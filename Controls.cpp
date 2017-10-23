//
//  Controls.cpp
//  Evaluator
//
//  Created by Damien Quartz on 10/23/17.
//

#include "Controls.h"

#include "Evaluator.h"

////////////////////////////////////////////
// ITextEdit
//
ITextEdit::ITextEdit(IPlugBase* pPlug, IRECT pR, int paramIdx, IText* pText, const char* str)
: IControl(pPlug, pR)
, mIdx(paramIdx)
{
  mDisablePrompt = true;
  mText = *pText;
  mStr.Set(str);
  mTextEntryLength = kExpressionLengthMax;
  mTextEntryOptions = ETextEntryOptions(kTextEntryMultiline | kTextEntryEnterKeyInsertsCR);
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
  mStr.Set(txt);
  SetDirty(false);
  mPlug->OnParamChange(mIdx);
}
//
/////////////////////////////////////

/////////////////////////////////////
// ConsoleText
//
ConsoleText::ConsoleText(IPlugBase* plug, IRECT pR, IText* textStyle, const IColor* backgroundColor, int margin)
: IControl(plug, pR)
, mPanel(plug, pR, backgroundColor)
, mText(plug, pR.GetPadded(-margin), textStyle, "Program State")
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
