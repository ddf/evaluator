//
//  Params.h
//  Evaluator
//
//  Created by Damien Quartz on 11/28/17.
//

#pragma once

#include <stdint.h>

enum EParams
{
	kGain = 0,
	kBitDepth = 1,
	kTimeType = 2, // enumeration to control how we advance 't' in the program
	kScopeWindow = 3, // amount of time in seconds that the scope window represents
	kVControl0 = 4,
	kVControl7 = kVControl0 + 7,
	// this is only used by the standalone,
	// but is included in the plug versions so that if someone saves an fxp from a DAW
	// and then loads that fxp into the standalone, the tempo it was saved with will be present.
	// it will be set to be not automatible, which will hide it in the VST3 version, at least.
	kTempo,
	kNumParams,
	
	// used for text edit fields so the UI can call OnParamChange
	kExpression = 101,
	kExpressionLengthMax = 1024,
	
	kWatch = 202, // starting paramIdx for watches
	kWatchNum = 10, // total number of watches available

	kTransportState = 301, // used to figure out if we should generate sound in the standalone
	
	kBitDepthMin = 1,
	kBitDepthMax = 24,
	
	// these are in milliseconds
	kScopeWindowMin = 1,
	kScopeWindowMax = 2000,
	
	kVControlMin = 0,
	kVControlMax = 255,
	
	kTempoMin = 1,
	kTempoMax = 960,
};

enum TimeType : uint8_t
{
	TTAlways,
	TTWithNoteContinuous, // continuously increment t if we have notes
	TTWithNoteResetting, // continuously increment t if we have notes, but reset to 0 with every note on
	
#if !SA_API // there is no "Project Time" in the standalone version, so we don't allow this param to have that value
	TTProjectTime,
#endif
	
	TTCount
};

enum TransportState
{
	kTransportStopped = 0,
	kTransportPaused,
	kTransportPlaying
};