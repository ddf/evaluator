//
//  Controls.h
//  Evaluator
//
//  Created by Damien Quartz on 10/23/17.
//

#pragma once

#include "IControl.h"

// originally cribbed from the IPlugEEL example
class ITextEdit : public IControl
{
public:
  ITextEdit(IPlugBase* pPlug, IRECT pR, int paramIdx, IText* pText, const char* str);
  ~ITextEdit();
  
// IControl overrides
  bool Draw(IGraphics* pGraphics) override;
  void OnMouseDown(int x, int y, IMouseMod* pMod) override;
  void TextFromTextEntry(const char* txt) override;
  
// text accessss
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

class ConsoleText : public IControl
{
public:
  ConsoleText(IPlugBase* plug, IRECT pR, IText* textStyle, const IColor* backgroundColor, int margin);
  
  bool Draw(IGraphics* pGraphics) override;
  void SetTextFromPlug(const char * text);

private:
  IPanelControl mPanel;
  ITextControl  mText;
};

class IIncrementControl : public IBitmapControl
{
public:
  IIncrementControl(IPlugBase* pPlug, int x, int y, int paramIdx, IBitmap* pBitmap, int direction);
  
  bool Draw(IGraphics* pGraphics) override;
  void OnMouseDown(int x, int y, IMouseMod* pMod) override;
  void OnMouseUp(int x, int y, IMouseMod* pMod) override;
  
private:
  double mInc;
  int   mPressed;
};
