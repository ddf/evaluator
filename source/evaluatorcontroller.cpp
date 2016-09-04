//-----------------------------------------------------------------------------
// Project     : VST SDK
// Version     : 3.6.6
//
// Category    : Examples
// Filename    : public.sdk/samples/vst/adelay/source/adelaycontroller.cpp
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

#include "evaluatorcontroller.h"
#include "evaluatorids.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/base/ibstream.h"
#include "vstparameters.h"
#include "evaluatoreditor.hpp"

#if TARGET_OS_IPHONE
#include "interappaudio/iosEditor.h"
#endif

using namespace Steinberg::Vst;

namespace Compartmental {
namespace Vst {

//-----------------------------------------------------------------------------
tresult PLUGIN_API EvaluatorController::initialize (FUnknown* context)
{
	tresult result = EditController::initialize (context);
	if (result == kResultTrue)
	{
		parameters.addParameter (STR16 ("Bypass"), 0, 1, 0, ParameterInfo::kCanAutomate|ParameterInfo::kIsBypass, kBypassId);

		parameters.addParameter (STR16 ("Volume"), STR16 (""), 0, 1, ParameterInfo::kCanAutomate, kVolumeId);
        
        ParameterInfo bitInfo = {
            .id = kBitDepthId,
            .title = STR16("Bit Depth"),
            .units = STR16("Bits"),
            .stepCount = 30,
            .defaultNormalizedValue = 0.5,
            .flags = ParameterInfo::kCanAutomate
        };
        parameters.addParameter( new RangeParameter( bitInfo, 1, 31 ) );
	}
	return kResultTrue;
}

//-----------------------------------------------------------------------------
IPlugView* PLUGIN_API EvaluatorController::createView (FIDString name)
{
	if (strcmp (name, ViewType::kEditor) == 0)
	{
        return new Compartmental::Vst::EvaluatorEditor(this);
	}
	return 0;
}

//------------------------------------------------------------------------
tresult PLUGIN_API EvaluatorController::setComponentState (IBStream* state)
{
	// we receive the current state of the component (processor part)
	// we read only the gain and bypass value...
	if (state)
	{
		float savedVolume = 0.f;
		if (state->read (&savedVolume, sizeof (float)) != kResultOk)
		{
			return kResultFalse;
		}

#if BYTEORDER == kBigEndian
		SWAP_32 (savedVolume)
#endif
		setParamNormalized (kVolumeId, savedVolume);

		// read the bypass
		int32 bypassState;
		if (state->read (&bypassState, sizeof (bypassState)) == kResultTrue)
		{
#if BYTEORDER == kBigEndian
			SWAP_32 (bypassState)
#endif
			setParamNormalized (kBypassId, bypassState ? 1 : 0);
		}
        
        float savedBitDepth = 0.5f;
        if ( state->read(&savedBitDepth, sizeof(float)) == kResultTrue )
        {
#if BYTEORDER == kBigEndian
            SWAP_32 (savedBitDepth)
#endif
            setParamNormalized (kBitDepthId, savedBitDepth);
        }
        
        memset(defaultMessageText, 0, sizeof(char)*128);
        int8 byteOrder;
        if ((state->read (&byteOrder, sizeof (int8))) == kResultTrue)
        {
            state->read (defaultMessageText, sizeof(char)*128);
            
            // if the byteorder doesn't match, byte swap the text array ...
            if (byteOrder != BYTEORDER)
            {
                for (int32 i = 0; i < 128; i++)
                {
                    SWAP_16 (defaultMessageText[i])
                }
            }
        }
	}

	return kResultOk;
}

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg
