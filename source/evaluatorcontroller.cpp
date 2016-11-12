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
#include "evaluatorprocessor.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/base/ibstream.h"
#include "vstparameters.h"
#include "evaluatoreditor.hpp"

#if TARGET_OS_IPHONE
#include "interappaudio/iosEditor.h"
#endif

using namespace Steinberg::Vst;

namespace Compartmental
{
    namespace Vst
    {

        //-----------------------------------------------------------------------------
        tresult PLUGIN_API EvaluatorController::initialize (FUnknown* context)
        {
            tresult result = EditControllerEx1::initialize (context);
            if (result == kResultTrue)
            {
                UnitInfo uinfo;
                uinfo.id = kRootUnitId;
                uinfo.parentUnitId = kNoParentUnitId;
                uinfo.programListId = kPresetId;
                UString name (uinfo.name, 128);
                name.fromAscii("Root");
                addUnit (new Unit (uinfo));
                
                StringListParameter* presetParam = new StringListParameter(
                    STR16("Presets"),
                    kPresetId,
                    0, // units
                    ParameterInfo::kCanAutomate | ParameterInfo::kIsProgramChange | ParameterInfo::kIsList
                );
                for (int32 i = 0; i < EvaluatorProcessor::numPresets; i++)
                {
                    presetParam->appendString(UString128(EvaluatorProcessor::presets[i].name));
                }
                parameters.addParameter(presetParam);
                
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
                parameters.addParameter( new RangeParameter( bitInfo, kBitDepthMin, kBitDepthMax ) );
                
                parameters.addParameter(STR16("EvalT"), STR16("t"), 0, 0, ParameterInfo::kIsReadOnly, kEvalTId);
                parameters.addParameter(STR16("EvalM"), STR16("m"), 0, 0, ParameterInfo::kIsReadOnly, kEvalMId);
                parameters.addParameter(STR16("EvalR"), STR16("r"), 0, 0, ParameterInfo::kIsReadOnly, kEvalRId);
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
                
                memset(defaultExpression, 0, sizeof(char)*128);
                int8 byteOrder;
                if ((state->read (&byteOrder, sizeof (int8))) == kResultTrue)
                {
                    state->read (defaultExpression, sizeof(char)*128);
                    
                    // if the byteorder doesn't match, byte swap the text array ...
                    if (byteOrder != BYTEORDER)
                    {
                        for (int32 i = 0; i < 128; i++)
                        {
                            SWAP_16 (defaultExpression[i])
                        }
                    }
                }
            }

            return kResultOk;
        }
        
        tresult PLUGIN_API EvaluatorController::setParamNormalized (ParamID tag, ParamValue value)
        {
            tresult res = EditControllerEx1::setParamNormalized (tag, value);
            if (res == kResultOk) // preset change
            {
                if ( tag == kPresetId )
                {
                    int32 program = (int32)parameters.getParameter (tag)->toPlain (value);
                    const EvaluatorProcessor::Preset& preset = EvaluatorProcessor::presets[program];
                    setDefaultExpression(preset.expression);
                    sendTextMessage(preset.expression);
                    for (int32 i = 0; i < viewsArray.total (); i++)
                    {
                        if (viewsArray.at (i))
                        {
                            viewsArray.at (i)->messageTextChanged();
                        }
                    }
                    setParamNormalized(kVolumeId, preset.volume);
                    setParamNormalized(kBitDepthId, preset.bitDepth);
                    componentHandler->restartComponent (kParamValuesChanged);
                }
                else
                {
                    for (int32 i = 0; i < viewsArray.total (); i++)
                    {
                        if (viewsArray.at (i))
                        {
                            viewsArray.at (i)->update (tag, value);
                        }
                    }
                }
            }
            return res;
        }
        
        //-----------------------------------------------------------------------------
        int32 PLUGIN_API EvaluatorController::getProgramListCount ()
        {
            return 1;
        }
        
        //-----------------------------------------------------------------------------
        tresult PLUGIN_API EvaluatorController::getProgramListInfo (int32 listIndex, ProgramListInfo& info /*out*/)
        {
            Parameter* param = parameters.getParameter (kPresetId);
            if (param && listIndex == 0)
            {
                info.id = kPresetId;
                info.programCount = (int32)param->toPlain (1) + 1;
                UString name (info.name, 128);
                name.fromAscii("Presets");
                return kResultTrue;
            }
            return kResultFalse;
        }
        
        //-----------------------------------------------------------------------------
        tresult PLUGIN_API EvaluatorController::getProgramName (ProgramListID listId, int32 programIndex, String128 name /*out*/)
        {
            if (listId == kPresetId)
            {
                Parameter* param = parameters.getParameter (kPresetId);
                if (param)
                {
                    ParamValue normalized = param->toNormalized (programIndex);
                    param->toString (normalized, name);
                    return kResultTrue;
                }
            }
            return kResultFalse;
        }
        
        //------------------------------------------------------------------------
        void EvaluatorController::addDependentView (EvaluatorEditor* view)
        {
            viewsArray.add (view);
        }
        
        //------------------------------------------------------------------------
        void EvaluatorController::removeDependentView (EvaluatorEditor* view)
        {
            for (int32 i = 0; i < viewsArray.total (); i++)
            {
                if (viewsArray.at (i) == view)
                {
                    viewsArray.removeAt (i);
                    break;
                }
            }
        }
        
        //------------------------------------------------------------------------
        void EvaluatorController::editorAttached (EditorView* editor)
        {
            EvaluatorEditor* view = dynamic_cast<EvaluatorEditor*> (editor);
            if (view)
            {
                addDependentView (view);
            }
        }
        
        //------------------------------------------------------------------------
        void EvaluatorController::editorRemoved (EditorView* editor)
        {
            EvaluatorEditor* view = dynamic_cast<EvaluatorEditor*> (editor);
            if (view)
            {
                removeDependentView (view);
            }
        }
    
    } // namespace Vst
} // namespace Compartmental
