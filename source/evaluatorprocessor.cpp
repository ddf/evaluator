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

#include "evaluatorprocessor.h"
#include "expr_eval.hpp"

#include "evaluatorids.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "pluginterfaces/vst/ivstevents.h"
#include <algorithm>

namespace Compartmental
{
    namespace Vst
    {
        const EvaluatorProcessor::Preset EvaluatorProcessor::presets[] =
        {
            // name, expression, volume, bit depth
            { "saw wave", "t*128", 0.1f, 0.45f },
            { "square wave", "(r-1) * ((m/10)%2)", 0.1f, 0.45f },
            { "sine wave", "$(t*128)", 0.1f, 0.45f },
            { "amplitude modulation", "t*64 | $(t)", 0.1f, 0.45f },
            { "frequency modulation", "t*64 + $(t*2)", 0.1f, 0.45f },
            { "bouncing balls", "$(t*(1000 - m%500))", 0.1f, 0.45f },
            { "overtone waterfall", "t*(128*(32-(m/50)%32)) | t*(128*((m/100)%64)) | t*128", 0.1f, 0.5f }
        };
        
        const int32 EvaluatorProcessor::numPresets = sizeof(EvaluatorProcessor::presets)/sizeof(EvaluatorProcessor::Preset);

        //-----------------------------------------------------------------------------
        EvaluatorProcessor::EvaluatorProcessor ()
        : mVolume(0.1f)
        , mEvaluator(0)
        , mExpression("")
        , mNoteOnPitch(-1)
        , mNoteOnVelocity(0)
        , mBypass (false)
        , mBitDepth(0.4f)
        {
            setControllerClass (EvaluatorControllerUID);
        }

        //-----------------------------------------------------------------------------
        tresult PLUGIN_API EvaluatorProcessor::initialize (FUnknown* context)
        {
            tresult result = AudioEffect::initialize (context);
            if (result == kResultTrue)
            {
                addAudioInput (STR16 ("AudioInput"), SpeakerArr::kStereo);
                addAudioOutput (STR16 ("AudioOutput"), SpeakerArr::kStereo);
                
                //---create Event In/Out buses (1 bus with only 1 channel)------
                // this gets us midi events
                addEventInput (STR16 ("Event In"), 1);
            }
            return result;
        }

        //-----------------------------------------------------------------------------
        tresult PLUGIN_API EvaluatorProcessor::setBusArrangements (SpeakerArrangement* inputs, int32 numIns, SpeakerArrangement* outputs, int32 numOuts)
        {
            // we only support one in and output bus and these buses must have the same number of channels
            if (numIns == 1 && numOuts == 1 && inputs[0] == outputs[0])
                return AudioEffect::setBusArrangements (inputs, numIns, outputs, numOuts);
            return kResultFalse;
        }

        //-----------------------------------------------------------------------------
        tresult PLUGIN_API EvaluatorProcessor::setActive (TBool state)
        {
            SpeakerArrangement arr;
            if (getBusArrangement (kOutput, 0, arr) != kResultTrue)
                return kResultFalse;
            int32 numChannels = SpeakerArr::getChannelCount (arr);
            if (numChannels == 0)
                return kResultFalse;

            if (state)
            {
                mEvaluator = new ExprEval();
                //strcpy(mExpression, "(t<<t/(1024*8) | t>>t/16 & t>>t/32) / (t%(t/512+1) + 1) * 32");
                //strcpy(mExpression, "(t*64 + $(t^$(m/2000))*$(m/2000)) | t*32");
                //strcpy(mExpression, "t*(m%2)");
                //strcpy(mExpression, "$(t*128)");
            }
            else
            {
                if (mEvaluator)
                {
                    delete mEvaluator;
                    mEvaluator = 0;
                }
            }
            return AudioEffect::setActive (state);
        }

        //-----------------------------------------------------------------------------
        tresult PLUGIN_API EvaluatorProcessor::process (ProcessData& data)
        {
            if (data.inputParameterChanges)
            {
                int32 numParamsChanged = data.inputParameterChanges->getParameterCount ();
                for (int32 index = 0; index < numParamsChanged; index++)
                {
                    IParamValueQueue* paramQueue = data.inputParameterChanges->getParameterData (index);
                    if (paramQueue)
                    {
                        ParamValue value;
                        int32 sampleOffset;
                        int32 numPoints = paramQueue->getPointCount ();
                        switch (paramQueue->getParameterId ())
                        {
                            case kVolumeId:
                            {
                                if (paramQueue->getPoint (numPoints - 1, sampleOffset, value) == kResultTrue)
                                {
                                    mVolume = value;
                                }
                            }
                            break;
                                
                            case kBypassId:
                            {
                                if (paramQueue->getPoint (numPoints - 1,  sampleOffset, value) == kResultTrue)
                                {
                                    mBypass = (value > 0.5f);
                                }
                            }
                            break;
                                
                            case kBitDepthId:
                            {
                                if (paramQueue->getPoint (numPoints - 1, sampleOffset, value) == kResultTrue)
                                {
                                    mBitDepth = value;
                                }
                            }
                            break;
                                
                            case kPresetId:
                            {
                                if ( paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)
                                {
                                    int32 program = std::min<int32> (numPresets-1, (int32)(value * numPresets));
                                    const Preset& preset = presets[program];
                                    receiveText(preset.expression);
                                    mVolume = preset.volume;
                                    mBitDepth = preset.bitDepth;
                                }
                            }
                            break;
                        }
                    }
                }
            }
            
            //---2) Read input events-------------
            IEventList* eventList = data.inputEvents;
            if (eventList)
            {
                int32 numEvent = eventList->getEventCount ();
                for (int32 i = 0; i < numEvent; i++)
                {
                    Event event;
                    if (eventList->getEvent (i, event) == kResultOk)
                    {
                        switch (event.type)
                        {
                            case Event::kNoteOnEvent:
                            {
                                mNoteOnPitch = event.noteOn.pitch;
                                mNoteOnVelocity = event.noteOn.velocity;
                                mTick = 0;
                                mEvaluator->SetVar('n', mNoteOnPitch);
                            }
                            break;
                                
                            case Event::kNoteOffEvent:
                            {
                                mNoteOnPitch = -1;
                            }
                            break;
                        }
                    }
                }
            }

            if (data.numSamples > 0)
            {
                SpeakerArrangement arr;
                getBusArrangement (kOutput, 0, arr);
                int32 numChannels = SpeakerArr::getChannelCount (arr);

                const int32 range = 1<<(int32)(mBitDepth*30 + 1);
                const float h = range / 2;
                const float amp = mNoteOnPitch >= 0 ? mVolume*mNoteOnVelocity : 0;
                const uint64 mdenom = (uint64)(processSetup.sampleRate/1000);
                const bool generate = amp > 0;
                
                for (int32 channel = 0; channel < numChannels; channel++)
                {
                    float* inputChannel = data.inputs[0].channelBuffers32[channel];
                    float* outputChannel = data.outputs[0].channelBuffers32[channel];
                    
                    for (int32 sample = 0; sample < data.numSamples; sample++)
                    {
                        float evalSample = 0;
                        if ( generate )
                        {
                            uint64 tempTick = mTick + sample;
                            mEvaluator->SetVar('t', tempTick);
                            mEvaluator->SetVar('m', tempTick/mdenom);
                            mEvaluator->SetVar('r', range);
                            unsigned int result = mEvaluator->Eval(mExpression);
                            evalSample = amp * (((float)(result%range)) - h) / h;
                        }
                        outputChannel[sample] = inputChannel[sample] + evalSample;
                    }
                }
                mTick += data.numSamples;
            }	
            return kResultTrue;
        }

        //------------------------------------------------------------------------
        tresult PLUGIN_API EvaluatorProcessor::setState (IBStream* state)
        {
            if (!state)
                return kResultFalse;

            // called when we load a preset, the model has to be reloaded

            float savedVolume = 0.f;
            if (state->read (&savedVolume, sizeof (float)) != kResultOk)
            {
                return kResultFalse;
            }

            int32 savedBypass = 0;
            if (state->read (&savedBypass, sizeof (int32)) != kResultOk)
            {
                // could be an old version, continue 
            }
            
            float savedBitDepth = 0.5f;
            if (state->read(&savedBitDepth, sizeof(float)) != kResultOk)
            {
                // could be old vesion, ok if not there
            }
            
            int8 byteOrder;
            if ((state->read (&byteOrder, sizeof (int8))) == kResultOk)
            {
                state->read (mExpression, sizeof(char)*128);
            
                // if the byteorder doesn't match, byte swap the text array ...
                if (byteOrder != BYTEORDER)
                {
                    for (int32 i = 0; i < 128; i++)
                    {
                        SWAP_16 (mExpression[i])
                    }
                }
            }

            #if BYTEORDER == kBigEndian
            SWAP_32 (savedVolume)
            SWAP_32 (savedBypass)
            SWAP_32 (savedBitDepth)
            #endif

            mVolume = savedVolume;
            mBypass = savedBypass > 0;
            mBitDepth = savedBitDepth;
            
            

            return kResultOk;
        }

        //------------------------------------------------------------------------
        tresult PLUGIN_API EvaluatorProcessor::getState (IBStream* state)
        {
            // here we need to save the model

            float toSaveVolume = mVolume;
            int32 toSaveBypass = mBypass ? 1 : 0;
            float toSaveBitDepth = mBitDepth;

            #if BYTEORDER == kBigEndian
            SWAP_32 (toSaveVolume)
            SWAP_32 (toSaveBypass)
            SWAP_32 (toSaveBitDepth)
            #endif

            state->write (&toSaveVolume, sizeof (float));
            state->write (&toSaveBypass, sizeof (int32));
            state->write (&toSaveBitDepth, sizeof(float));
            
            // as we save a Unicode string, we must know the byteorder when setState is called
            int8 byteOrder = BYTEORDER;
            if (state->write (&byteOrder, sizeof (int8)) == kResultOk)
            {
                return state->write (mExpression, sizeof(char)*128);
            }
            return kResultFalse;

            return kResultOk;
        }
            
        //------------------------------------------------------------------------
        tresult EvaluatorProcessor::receiveText (const char* text)
        {
            // received from Controller
            fprintf (stderr, "[Evaluator] received: ");
            fprintf (stderr, text);
            fprintf (stderr, "\n");
            
            strncpy(mExpression, text, 127);
            
            return kResultOk;
        }

    } // namespace Vst
} // namespace Compartmental
