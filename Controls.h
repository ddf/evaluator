//
//  Controls.h
//  Evaluator
//
//  Created by Damien Quartz on 10/23/17.
//

#pragma once

#include "IControl.h"

class Interface;

// originally cribbed from the IPlugEEL example
class ITextEdit : public IControl
{
public:
	ITextEdit(IPlugBase* pPlug, IRECT pR, int paramIdx, IText* pText, const char* str, ETextEntryOptions textEntryOptions);
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

	void SetTextFromPlug(const char * text)
	{
		mStr.Set(text);
	}

	int GetTextLength() const
	{
		return mStr.GetLength();
	}

protected:
	int        mIdx;
	WDL_String mStr;
};

class TextBox : public ICaptionControl
{
public:
	TextBox(IPlugBase* pPlug, IRECT pR, int paramIdx, IText* pText, IRECT textRect);

	bool Draw(IGraphics* pGraphics) override;
	void OnMouseDown(int x, int y, IMouseMod* pMod) override;

private:
	IRECT mTextRect;
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

class KnobLineCoronaControl : public IKnobLineControl
{
public:
	KnobLineCoronaControl(IPlugBase* pPlug, IRECT pR, int paramIdx,
		const IColor* pLineColor, const IColor* pCoronaColor,
		float coronaThickness, double innerRadius = 0.0, double outerRadius = 0.0,
		double minAngle = -0.75 * PI, double maxAngle = 0.75 * PI,
		EDirection direction = kVertical, double gearing = DEFAULT_GEARING);

	bool Draw(IGraphics* pGraphics) override;

private:
	IColor        mCoronaColor;
	IChannelBlend mCoronaBlend;
};

class Oscilloscope : public IControl
{
public:
	Oscilloscope(IPlugBase* pPlug, IRECT pR, const IColor* backgroundColor, const IColor* lineColorLeft, const IColor* lineColorRight);
	~Oscilloscope();

	bool Draw(IGraphics* pGraphics) override;

	void AddSample(double left, double right);

private:
	IColor mBackgroundColor;
	IColor mLineColorLeft;
	IColor mLineColorRight;

	double* mBuffer;
	int		mBufferSize;
	int		mBufferBegin;
};

class LoadButton : public IBitmapControl
{
public:
	LoadButton(IPlugBase* pPlug, int x, int y, IBitmap* pButtonBack, IText* pButtonTextStyle, IRECT menuRect, IText* pMenuTextStyle, Interface* pInterface);

	bool Draw(IGraphics* pGraphics) override;
	void OnMouseDown(int x, int y, IMouseMod* pMod) override;
	void OnMouseOver(int x, int y, IMouseMod* pMod) override;

private:

	enum
	{
		kClosed,
		kOpen,
	} 
	mState;

	Interface* mInterface;
	IRECT mButtonRect; // where the button goes
	IRECT mMenuRect; // where the menu goes
	// rects for all the selections in the menu
	WDL_TypedBuf<IRECT> mRECTs;
	IText mButtonText;
	IText mMenuText;
	int mSelection;
};

class SaveButton : public IBitmapControl
{
public:
	SaveButton(IPlugBase* pPlug, int x, int y, IBitmap* pButtonBack, IText* pButtonTextStyle, Interface* pInterface);

	bool Draw(IGraphics* pGraphics) override;
	void OnMouseDown(int x, int y, IMouseMod* pMod) override;

private:
	Interface* mInterface;
	IText mButtonText;
};