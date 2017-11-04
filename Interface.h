#pragma once

class Evaluator;
class IGraphics;
class ITextEdit;
class ITextControl;
class ConsoleText;
class IControl;
class Oscilloscope;

class Interface
{
public:
	Interface(Evaluator* plug, IGraphics* pGraphics);
	~Interface();

	void SetDirty(int paramIdx, bool pushToPlug);

	const char * GetProgramText() const;
	size_t GetProgramMemorySize() const;

	void SetProgramText(const char * programText);
	void SetConsoleText(const char * consoleText);
	void SetWatchText(const char* watchText);

	void UpdateOscilloscope(double left, double right);
	int GetOscilloscopeWidth() const;

	const char * GetWatch(int idx) const;
	void SetWatch(int idx, const char * text);

private:

	Evaluator* mPlug;

	ITextEdit*		textEdit;
	ITextControl*   tmodeText;
	ConsoleText*	consoleTextControl;
	ITextEdit**		watches;
	ConsoleText*	watchConsole;
	IControl*		bitDepthControl;
	Oscilloscope*   oscilloscope;
};

