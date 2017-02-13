//-----------------------------------------------------------------------------
// Project     : VST SDK
// Version     : 3.6.6
//
// Category    : Examples
// Filename    : public.sdk/samples/vst/adelay/source/adelayprocessor.cpp
// Created by  : Steinberg, 06/2009
// Description : 
//
//-----------------------------------------------------------------------------
// LICENSE
// (c) 2016, Steinberg Media Technologies GmbH, All Rights Reserved
//-----------------------------------------------------------------------------
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
//   * Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation 
//     and/or other materials provided with the distribution.
//   * Neither the name of the Steinberg Media Technologies nor the names of its
//     contributors may be used to endorse or promote products derived from this 
//     software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"
#include "pluginterfaces/vst/ivstevents.h"
#include <vector>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace Compartmental {
namespace Vst {
    
class Expression;

//-----------------------------------------------------------------------------
class EvaluatorProcessor : public AudioEffect
{
public:
	EvaluatorProcessor ();
	
	tresult PLUGIN_API initialize (FUnknown* context);
	tresult PLUGIN_API setBusArrangements (SpeakerArrangement* inputs, int32 numIns, SpeakerArrangement* outputs, int32 numOuts);

	tresult PLUGIN_API setActive (TBool state);
	tresult PLUGIN_API process (ProcessData& data);
	
	//------------------------------------------------------------------------
	tresult PLUGIN_API setState (IBStream* state);
	tresult PLUGIN_API getState (IBStream* state);
    
    /** Test of a communication channel between controller and component */
    tresult receiveText (const char* text);

	static FUnknown* createInstance (void*) { return (IAudioProcessor*)new EvaluatorProcessor (); }
    
    struct Preset
    {
        const char* name;
        const char* expression;
        const float volume;
        const float bitDepth;
    };
    
    static const Preset presets[];
    static const int32  numPresets;

protected:
    uint64 mTick;
    Expression* mEvaluator;
    char mExpression[128];
    
	// we track up to 16 active midi notes
	// if we receive a note on and this list is full, we ignore it.
	enum { kMaxNotes = 16 };
	std::vector<Event>  mNotes;
    
	ParamValue mVolume;
    ParamValue mBitDepth;
	bool mBypass;
};

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Compartmental
