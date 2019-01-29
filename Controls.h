//
//  Controls.h
//  Evaluator
//
//  Created by Damien Quartz on 10/23/17.
//

#pragma once

#include "IControl.h"
#include "KnobLineCoronaControl.h"
#include "Params.h"
#include <string>

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
		return mStr.c_str();
	}

	void SetTextFromPlug(const char * text)
	{
		mStr = text;
	}

	int GetTextLength() const
	{
		return mStr.size();
	}

protected:
	int         mIdx;
	std::string mStr;
};

class TextBox : public ICaptionControl
{
public:
	TextBox(IPlugBase* pPlug, IRECT pR, int paramIdx, IText* pText, IRECT textRect, bool showParamUnits = false);

	bool Draw(IGraphics* pGraphics) override;
	void OnMouseDown(int x, int y, IMouseMod* pMod) override;
	void OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod) override;
	void OnMouseWheel(int x, int y, IMouseMod* pMod, int d) override;

private:
	bool  mShowParamUnits;
	IRECT mTextRect;
};

class TextTable : public IControl
{
public:
	TextTable(IPlugBase* pPlug, IRECT pR, IText* pText, const char** data, int iColumns, int iRows);

	bool Draw(IGraphics* pGraphics) override;

private:
	const char** tableData;
	int columns;
	int rows;
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

class EnumControl : public IControl
{
public:
	EnumControl(IPlugBase* pPlug, IRECT rect, int paramIdx, IText* textStyle);

	bool Draw(IGraphics* pGraphics) override;
	void OnMouseDown(int x, int y, IMouseMod* pMod) override;
};

class ToggleControl : public IControl
{
public:
	ToggleControl(IPlugBase* pPlug, IRECT rect, int paramIdx, IColor backgroundColor, IColor fillColor);

	bool Draw(IGraphics* pGraphics) override;
	void OnMouseDown(int x, int y, IMouseMod* pMod) override;
};

class Oscilloscope : public IControl
{
public:
	Oscilloscope(IPlugBase* pPlug, IRECT pR, const IColor* backgroundColor, const IColor* lineColorLeft, const IColor* lineColorRight);
	~Oscilloscope();

	bool Draw(IGraphics* pGraphics) override;

	void AddSample(double left, double right);

private:
	
	void DrawWaveform(IGraphics* pGraphics);
	void DrawGrid(IGraphics* pGraphics);
	
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
	IRECT mTextRect; // where the text in the button goes
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
	IRECT mTextRect;
};

class ManualButton : public IBitmapControl
{
public:
	ManualButton(IPlugBase* pPlug, int x, int y, IBitmap* pButtonBack, IText* pButtonTextStyle, Interface* pInterface);
	
	bool Draw(IGraphics* pGraphics) override;
	void OnMouseDown(int x, int y, IMouseMod* pMod) override;
	
private:
	Interface* mInterface;
	IText mButtonText;
	IRECT mTextRect;
};

class HelpButton : public IControl
{
public:
	HelpButton(IPlugBase* pPlug, IRECT rect, IText* pButtonTextStyle, Interface* pInterface);

	bool Draw(IGraphics* pGraphics) override;
	void OnMouseDown(int x, int y, IMouseMod* pMod) override;

private:
	Interface* mInterface;
	IText mButtonText;
	IRECT mTextRect;
};

// play, pause, and stop buttons used by the standalone to control how t increments
// essentially makes TTAlways behave like TTProjectTime
class TransportButtons : public IControl
{
public:
	// give the rect that the three buttons should occupy and we split it into three sections
	TransportButtons(IPlugBase* pPlug, IRECT rect, const IColor& backgroundColor, const IColor& foregroundColor);

	bool Draw(IGraphics* pGraphics) override;
	void OnMouseDown(int x, int y, IMouseMod* pMod) override;

	TransportState GetTransportState() const;

private:
	
	void SetTransportState( TransportState state );

	TransportState mState;

	IRECT mPlayRect;
	IRECT mPauseRect;
	IRECT mStopRect;
	IColor mBack;
	IColor mFore;	
};

// catch keyboard events and generate midi events from them.
// not a great implementation due to input limitations of IPlug.
class MidiControl : public IControl
{
public:
	MidiControl(IPlugBase* pPlug);

	bool Draw(IGraphics* pGraphics) override { return false; }
	bool OnKeyDown(int x, int y, int key) override;
};
