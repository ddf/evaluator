#pragma once

class Evaluator;
class IGraphics;
class ITextEdit;
class ITextControl;
class IControl;

class Interface
{
public:
	Interface(Evaluator* plug, IGraphics* pGraphics);
	~Interface();

	void SetDirty(int paramIdx, bool pushToPlug);

	const char * GetProgramText() const;

	void SetProgramText(const char * programText);
private:
	Evaluator* mPlug;

	ITextEdit* textEdit;
	ITextControl* timeLabel;
	ITextControl* millisLabel;
	ITextControl* quartLabel;
	ITextControl* noteLabel;
	ITextControl* rangeLabel;
	ITextControl* prevLabel;
	IControl*     bitDepthControl;
};

