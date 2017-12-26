#pragma once

#include "Params.h"

class Evaluator;
class IGraphics;
class ITextEdit;
class ITextControl;
class ConsoleText;
class IControl;
class Oscilloscope;
class TransportButtons;

class Interface
{
public:
	Interface(Evaluator* plug, IGraphics* pGraphics);
	~Interface();

	void SetDirty(int paramIdx, bool pushToPlug);

	const char * GetProgramText() const;
	unsigned int GetProgramMemorySize() const;

	void SetProgramName(const char * programName);
	const char * GetProgramName() const;

	void SetProgramText(const char * programText);
	void SetConsoleText(const char * consoleText);
	void SetWatchValue(int idx, const char* watchText);

	void UpdateOscilloscope(double left, double right);
	int GetOscilloscopeWidth() const;

	const char * GetWatch(int idx) const;
	void SetWatch(int idx, const char * text);

	// made this so that the interface can update the program name when we load a preset
	void LoadPreset(int idx);

	TransportState GetTransportState() const;

	void ToggleHelp();

private:

	void CreateControls(IGraphics* pGraphics);

	Evaluator* mPlug;

	ITextEdit*		textEdit;
	ITextControl*	programName;
	ConsoleText*	consoleTextControl;
	IControl*		bitDepthControl;
	Oscilloscope*   oscilloscope;
	TransportButtons* transportButtons;
	IControl*		timeResetLabel;
	IControl*		timeResetToggle;
	IControl*		compilePrompt;
	
	struct Watch
	{
		ITextEdit* var;
		ConsoleText* val;
	};
	
	Watch watches[kWatchNum];	
};

