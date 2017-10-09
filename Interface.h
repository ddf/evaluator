#pragma once

class Evaluator;
class IGraphics;
class ITextEdit;
class ConsoleText;
class IControl;

class Interface
{
public:
	Interface(Evaluator* plug, IGraphics* pGraphics);
	~Interface();

	void SetDirty(int paramIdx, bool pushToPlug);

	const char * GetProgramText() const;

	void SetProgramText(const char * programText);

	void SetConsoleText(const char * consoleText);

private:	

	Evaluator* mPlug;

	ITextEdit*		textEdit;
	ConsoleText*	consoleTextControl;
	IControl*		bitDepthControl;
};

