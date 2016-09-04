//-----------------------------------------------------------------------------
// Project     : VST SDK
// Version     : 3.6.6
//
// Category    : Examples
// Filename    : public.sdk/samples/vst/adelay/source/factory.cpp
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

#include "public.sdk/source/main/pluginfactoryvst3.h"
#include "evaluatorcontroller.h"
#include "evaluatorprocessor.h"
#include "evaluatorids.h"
#include "exampletest.h"
#include "version.h"	// for versioning

#define stringPluginName "Evaluator"


BEGIN_FACTORY_DEF ("Compartmental",
				   "http://www.compartmental.net",
				   "mailto:info@compartmental.net")

    DEF_CLASS2 (INLINE_UID_FROM_FUID(Compartmental::Vst::EvaluatorProcessorUID),
				PClassInfo::kManyInstances,
				kVstAudioEffectClass,
				stringPluginName,
				Vst::kDistributable,
				"Fx|Delay",
				FULL_VERSION_STR,		// Plug-in version (to be changed)
				kVstVersionString,
                Compartmental::Vst::EvaluatorProcessor::createInstance)

	DEF_CLASS2 (INLINE_UID_FROM_FUID(Compartmental::Vst::EvaluatorControllerUID),
				PClassInfo::kManyInstances,
				kVstComponentControllerClass,
				stringPluginName "Controller",	// controller name (could be the same than component name)
				0,						// not used here
				"",						// not used here
				FULL_VERSION_STR,		// Plug-in version (to be changed)
				kVstVersionString,
				Compartmental::Vst::EvaluatorController::createInstance)

	// add Test Factory
	DEF_CLASS2 (INLINE_UID_FROM_FUID(Compartmental::Vst::EvaluatorTestFactory::cid),
				PClassInfo::kManyInstances,
				kTestClass,
				stringPluginName "Test Factory",
				0,
				"",
				"",
				"",
				Compartmental::Vst::EvaluatorTestFactory::createInstance)

END_FACTORY

bool InitModule () { return true; }
bool DeinitModule () { return true; }

